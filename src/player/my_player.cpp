#include "my_player.hpp"
#include <cstdlib>
#include <algorithm>
namespace ttt::my_player {

void MyPlayer::set_sign(Sign sign) { m_sign = sign; }
const char *MyPlayer::get_name() const { return m_name; }

Point MyPlayer::make_move(const State &state) {
    const int cols = state.get_opts().cols;
    const int rows = state.get_opts().rows;
    const int total_cells = cols * rows;

    Sign* board = new Sign[total_cells];
    int count_x = 0;
    int count_o = 0;
    
    int min_x = cols, max_x = 0;
    int min_y = rows, max_y = 0;

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            Sign val = state.get_value(x, y);
            board[y * cols + x] = val;
            
            if (val == Sign::X || val == Sign::O) {
                if (val == Sign::X) count_x++;
                else count_o++;
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
            }
        }
    }

    if (count_x == 0 && count_o == 0) {
        int cx = cols / 2, cy = rows / 2;
        if (board[cy * cols + cx] == Sign::NONE) {
            delete[] board; return {cx, cy};
        } else {
            int best_dist = 1000000;
            Point best_start = {-1, -1};
            for (int y = 0; y < rows; ++y) {
                for (int x = 0; x < cols; ++x) {
                    if (board[y * cols + x] == Sign::NONE) {
                        int dist = (x - cx)*(x - cx) + (y - cy)*(y - cy);
                        if (dist < best_dist) {
                            best_dist = dist;
                            best_start = {x, y};
                        }
                    }
                }
            }
            delete[] board; return best_start;
        }
    }

    const Sign my_sign = (count_x == count_o) ? Sign::X : Sign::O;
    const Sign opp_sign = (my_sign == Sign::X) ? Sign::O : Sign::X;

    min_x = std::max(0, min_x - 4);
    max_x = std::min(cols - 1, max_x + 4);
    min_y = std::max(0, min_y - 4);
    max_y = std::min(rows - 1, max_y + 4);

    auto has_neighbor = [&](int x, int y) -> bool {
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                if (dx == 0 && dy == 0) continue;
                int nx = x + dx, ny = y + dy;
                if (nx >= 0 && nx < cols && ny >= 0 && ny < rows) {
                    Sign v = board[ny * cols + nx];
                    if (v == Sign::X || v == Sign::O) return true;
                }
            }
        }
        return false;
    };

    auto evaluate_static = [&]() -> long long {
        long long my_score = 0;
        long long opp_score = 0;
        
        auto eval_window = [&](int x, int y, int dx, int dy) {
            int my_c = 0, opp_c = 0;
            for (int i = 0; i < 5; ++i) {
                Sign cell = board[(y + i * dy) * cols + (x + i * dx)];
                if (cell == my_sign) my_c++;
                else if (cell == opp_sign) opp_c++;
                else if (cell != Sign::NONE) return; 
            }
            if (my_c > 0 && opp_c > 0) return;
            
            if (my_c > 0) {
                if (my_c == 5) my_score += 100000000000000LL;
                else if (my_c == 4) my_score += 1000000000000LL;
                else if (my_c == 3) my_score += 50000000LL;
                else if (my_c == 2) my_score += 1000000LL;
                else my_score += 10000LL;
            } else if (opp_c > 0) {
                if (opp_c == 5) opp_score += 100000000000000LL;
                else if (opp_c == 4) opp_score += 1000000000000LL;
                else if (opp_c == 3) opp_score += 50000000LL;
                else if (opp_c == 2) opp_score += 1000000LL;
                else opp_score += 10000LL;
            }
        };

        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                if (x <= cols - 5) eval_window(x, y, 1, 0); 
                if (y <= rows - 5) eval_window(x, y, 0, 1); 
                if (x <= cols - 5 && y <= rows - 5) eval_window(x, y, 1, 1); 
                if (x <= cols - 5 && y >= 4) eval_window(x, y, 1, -1);       
            }
        }
        return my_score - opp_score - (opp_score / 2); 
    };

    auto evaluate_point = [&](int cx, int cy, Sign color) -> long long {
        int win5 = 0, win4 = 0, win3 = 0, win2 = 0;
        int dx_list[] = {1, 0, 1, 1};
        int dy_list[] = {0, 1, 1, -1};
        
        for (int dir = 0; dir < 4; ++dir) {
            for (int offset = -4; offset <= 0; ++offset) {
                int start_x = cx + offset * dx_list[dir];
                int start_y = cy + offset * dy_list[dir];
                
                int color_count = 0;
                bool blocked = false;
                
                for (int i = 0; i < 5; ++i) {
                    int nx = start_x + i * dx_list[dir];
                    int ny = start_y + i * dy_list[dir];
                    
                    if (nx < 0 || nx >= cols || ny < 0 || ny >= rows) { blocked = true; break; }
                    
                    Sign cell = board[ny * cols + nx];
                    if (nx == cx && ny == cy) color_count++;
                    else if (cell == color) color_count++;
                    else if (cell != Sign::NONE) { blocked = true; break; }
                }
                if (!blocked) {
                    if (color_count == 5) win5++;
                    else if (color_count == 4) win4++;
                    else if (color_count == 3) win3++;
                    else if (color_count == 2) win2++;
                }
            }
        }
        
        if (win5 >= 1) return 100000000000000LL;
        if (win4 >= 2) return 3000000000000LL;
        if (win4 >= 1 && win3 >= 2) return 2000000000000LL;
        if (win4 >= 1) return 1000000000000LL;
        if (win3 >= 4) return 500000000000LL;
        if (win3 >= 2) return 150000000LL;
        if (win3 == 1) return 50000000LL;
        if (win2 >= 2) return 3000000LL;                  
        if (win2 == 1) return 1000000LL;                  
        return 10000LL;
    };

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            if (board[y * cols + x] == Sign::NONE && has_neighbor(x, y)) {
                if (evaluate_point(x, y, my_sign) >= 100000000000000LL) {
                    delete[] board; return {x, y};
                }
            }
        }
    }
    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            if (board[y * cols + x] == Sign::NONE && has_neighbor(x, y)) {
                if (evaluate_point(x, y, opp_sign) >= 100000000000000LL) {
                    delete[] board; return {x, y};
                }
            }
        }
    }

    struct Candidate { 
        int x; 
        int y; 
        long long score; 
    };
    
    const int MAX_CANDIDATES = 20;

    auto minimax = [&](auto& self, int depth, long long alpha, long long beta, bool is_maximizing) -> long long {
        long long current_eval = evaluate_static();
        
        if (current_eval >= 50000000000000LL) return current_eval + depth * 1000;
        if (current_eval <= -50000000000000LL) return current_eval - depth * 1000;
        if (depth == 0) return current_eval;

        Sign current_sign = is_maximizing ? my_sign : opp_sign;
        Sign next_sign = is_maximizing ? opp_sign : my_sign;

        Candidate cands[MAX_CANDIDATES];
        int cand_count = 0;

        for (int y = min_y; y <= max_y; ++y) {
            for (int x = min_x; x <= max_x; ++x) {
                if (board[y * cols + x] == Sign::NONE && has_neighbor(x, y)) {
                    long long atk = evaluate_point(x, y, current_sign);
                    long long def = evaluate_point(x, y, next_sign);

                    if (atk >= 100000000000000LL) {
                        return is_maximizing ? atk + depth * 1000 : -atk - depth * 1000;
                    }

                    long long urgency = atk + def + (def / 2);
                    
                    if (cand_count < MAX_CANDIDATES || urgency > cands[MAX_CANDIDATES - 1].score) {
                        int pos = cand_count < MAX_CANDIDATES ? cand_count++ : MAX_CANDIDATES - 1;
                        while (pos > 0 && cands[pos - 1].score < urgency) {
                            cands[pos] = cands[pos - 1];
                            pos--;
                        }
                        cands[pos] = {x, y, urgency};
                    }
                }
            }
        }

        if (cand_count == 0) return current_eval;
        long long best_eval = is_maximizing ? -200000000000000LL : 200000000000000LL;

        for (int i = 0; i < cand_count; ++i) {
            int cx = cands[i].x;
            int cy = cands[i].y;
            
            board[cy * cols + cx] = current_sign;
            long long eval = self(self, depth - 1, alpha, beta, !is_maximizing);
            board[cy * cols + cx] = Sign::NONE;

            if (is_maximizing) {
                if (eval > best_eval) best_eval = eval;
                if (best_eval > alpha) alpha = best_eval;
            } else {
                if (eval < best_eval) best_eval = eval;
                if (best_eval < beta) beta = best_eval;
            }
            if (beta <= alpha) break; 
        }
        return best_eval;
    };

    Candidate root_cands[MAX_CANDIDATES];
    int root_cand_count = 0;

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            if (board[y * cols + x] == Sign::NONE && has_neighbor(x, y)) {
                long long atk = evaluate_point(x, y, my_sign);
                long long def = evaluate_point(x, y, opp_sign);

                long long urgency = atk + def + (def / 2); 

                if (root_cand_count < MAX_CANDIDATES || urgency > root_cands[MAX_CANDIDATES - 1].score) {
                    int pos = root_cand_count < MAX_CANDIDATES ? root_cand_count++ : MAX_CANDIDATES - 1;
                    while (pos > 0 && root_cands[pos - 1].score < urgency) {
                        root_cands[pos] = root_cands[pos - 1];
                        pos--;
                    }
                    root_cands[pos] = {x, y, urgency};
                }
            }
        }
    }

    long long best_score = -200000000000000LL;
    Point best_move = {-1, -1};

    for (int i = 0; i < root_cand_count; ++i) {
        int cx = root_cands[i].x;
        int cy = root_cands[i].y;

        board[cy * cols + cx] = my_sign;
        long long move_score = minimax(minimax, 2, -200000000000000LL, 200000000000000LL, false);
        board[cy * cols + cx] = Sign::NONE;

        if (move_score > best_score || best_move.x == -1) {
            best_score = move_score;
            best_move = {cx, cy};
        }
    }

    delete[] board;

    if (best_move.x == -1) {
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                if (state.get_value(x, y) == Sign::NONE) return {x, y};
            }
        }
    }

    return best_move;
}
}; // namespace ttt::my_player