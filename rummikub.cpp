#include "rummikub.h"

std::string colorToString(Color color){
    switch(color){
        case Color::RED:
            return "red";
        case Color::BLUE:
            return "blue";
        case Color::BLACK:
            return "black";
        case Color::YELLOW:
            return "yellow";
        case Color::JOKER:
            return "joker";
        default:
            throw std::runtime_error("Invalid color");
    }
}

std::string state_hash(const std::vector<Tile>& remaining_tiles, const std::vector<std::vector<Tile>>& groups) {
    std::string hash = "";
    for (const auto& tile : remaining_tiles) {
        hash += std::to_string(static_cast<int>(tile.color)) + "," + std::to_string(tile.number) + ";";
    }
    hash += "|";
    for (const auto& group : groups) {
        for (const auto& tile : group) {
            hash += std::to_string(static_cast<int>(tile.color)) + "," + std::to_string(tile.number) + ";";
        }
        hash += "/";
    }
    return hash;
}

bool is_set(const std::vector<Tile>& group) {
    int group_size = group.size();
    int joker_count = 0;
    for (const Tile& tile : group) {
        if (tile.color == Color::JOKER) {
            --group_size;
            ++joker_count;
        }
    }
    if (group_size < MIN_GROUP_SIZE - joker_count) return false;
    std::set<Color> unique_colors;
    int first_number = -1;
    for (const Tile& tile : group) {
        if (tile.color != Color::JOKER) {
            if (first_number == -1) first_number = tile.number;
            unique_colors.insert(tile.color);
            if (tile.number != first_number) {
                return false;
            }
        }
    }
    return unique_colors.size() == group_size;
}

bool is_run(const std::vector<Tile>& group) {
    int group_size = group.size();
    int joker_count = 0;
    for (const Tile& tile : group) {
        if (tile.color == Color::JOKER) {
            --group_size;
            ++joker_count;
        }
    }
    if (group_size < MIN_GROUP_SIZE - joker_count) return false;
    Color first_color = group[0].color;
    std::vector<int> numbers;
    for (const Tile& tile : group) {
        if (tile.color != Color::JOKER) {
            if (first_color == Color::JOKER) first_color = tile.color;
            if (tile.color != first_color) {
                return false;
            }
            // Check if the number is already in the vector (duplicate)
            if (std::find(numbers.begin(), numbers.end(), tile.number) != numbers.end()) {
                return false;
            }
            numbers.push_back(tile.number);
        }
    }
    std::sort(numbers.begin(), numbers.end());
    int allowed_gap = joker_count;
    for (size_t i = 1; i < numbers.size(); ++i) {
        int gap = numbers[i] - numbers[i - 1] - 1;
        if (gap > 0) {
            allowed_gap -= gap;
            if (allowed_gap < 0) {
                return false;
            }
        }
    }
    return true;
}

bool can_form_group(const Tile& tile, const std::vector<Tile>& group) {
    std::vector<Tile> new_group = group;
    new_group.push_back(tile);
    return is_set(new_group) || is_run(new_group);
}

void valid_rummikub_arrangement_helper(   std::vector<Tile>& remaining_tiles, 
                                          std::vector<std::vector<Tile>>& groups, 
                                          std::map<std::string, std::vector<std::vector<Tile>>>& memo,
                                          std::vector<std::vector<Tile>>& result,
                                          size_t start_idx,
                                          size_t end_idx) {
                                                                        
    std::string curr_state = state_hash(remaining_tiles, groups);
    if (memo.find(curr_state) != memo.end()) {
        result = memo[curr_state];
        return;
    }

    if (remaining_tiles.empty()) {
        result = groups;
        return;
    }

    Tile current_tile = remaining_tiles.back();
    remaining_tiles.pop_back();

    for (auto& group : groups) {
        if (can_form_group(current_tile, group)) {
            group.push_back(current_tile);
            valid_rummikub_arrangement_helper(remaining_tiles, groups, memo, result, start_idx, end_idx);
            if (!result.empty()) {
                return;
            }
            group.pop_back();
        }
    }

    for (size_t i = start_idx; i < end_idx; ++i) {
        for (size_t j = i + 1; j < remaining_tiles.size(); ++j) {
            std::vector<Tile> new_group = {current_tile, remaining_tiles[i], remaining_tiles[j]};
            if (is_set(new_group) || is_run(new_group)) {
                groups.push_back(new_group);
                Tile tile_i = remaining_tiles[i];
                Tile tile_j = remaining_tiles[j];
                remaining_tiles.erase(remaining_tiles.begin() + j);
                remaining_tiles.erase(remaining_tiles.begin() + i);
                valid_rummikub_arrangement_helper(remaining_tiles, groups, memo, result, start_idx, end_idx);
                if (!result.empty()) {
                    return;
                }
                remaining_tiles.insert(remaining_tiles.begin() + i, tile_i);
                remaining_tiles.insert(remaining_tiles.begin() + j, tile_j);
                groups.pop_back();
            }
        }
    }

    remaining_tiles.push_back(current_tile);

    memo[curr_state] = {};
}

std::vector<std::vector<Tile>> valid_rummikub_arrangement(const std::vector<Tile>& tiles) {
    std::vector<Tile> remaining_tiles = tiles;
    std::vector<std::vector<Tile>> groups;
    std::map<std::string, std::vector<std::vector<Tile>>> memo;
    
    size_t num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(num_threads);
    std::vector<std::vector<std::vector<Tile>>> thread_results(num_threads);
    std::vector<std::map<std::string, std::vector<std::vector<Tile>>>> thread_memos(num_threads);
    std::vector<std::vector<std::vector<Tile>>> thread_groups(num_threads);

    size_t tiles_per_thread = remaining_tiles.size() / num_threads;

    for (size_t i = 0; i < num_threads; ++i) {
        size_t start_idx = i * tiles_per_thread;
        size_t end_idx = (i == num_threads - 1) ? remaining_tiles.size() : (i + 1) * tiles_per_thread;

                threads[i] = std::thread(valid_rummikub_arrangement_helper,
                                 std::ref(remaining_tiles),
                                 std::ref(thread_groups[i]),
                                 std::ref(thread_memos[i]),
                                 std::ref(thread_results[i]),
                                 start_idx,
                                 end_idx);
    }

    for (auto& t : threads) {
        t.join();
    }

    // Merge memos from different threads
    for (const auto& thread_memo : thread_memos) {
        memo.insert(thread_memo.begin(), thread_memo.end());
    }

    for (const auto& thread_result : thread_results) {
        if (!thread_result.empty()) {
            return thread_result;
        }
    }

    return {};
}

std::vector<std::vector<Tile>> getAllSubsets(const std::vector<Tile>& vec) {
    size_t numSubsets = 1 << vec.size();
    std::vector<std::vector<Tile>> subsets(numSubsets);

    for (size_t i = 0; i < numSubsets; ++i) {
        for (size_t j = 0; j < vec.size(); ++j) {
            if (i & (1 << j)) {
                subsets[i].push_back(vec[j]);
            }
        }
    }
    return subsets;
}

std::vector<std::vector<Tile>> getMoveList(std::vector<Tile> hand, std::vector<Tile> board) {
    std::vector<std::vector<Tile>> movelist{};
    SubsetsGenerator generator(hand);
    for (const std::vector<Tile>& subset : generator) {
        std::vector<Tile> tiles{board};
        tiles.insert(tiles.end(), subset.begin(), subset.end());

        auto arrangement = valid_rummikub_arrangement(tiles);
        if (!arrangement.empty()) {
            movelist.push_back(subset);
        }
    }

    return movelist;
}


int main() {
    std::vector<Tile> board{
        {Color::BLACK, 6}, {Color::BLACK, 7}, {Color::BLACK, 8}, {Color::BLACK, 9}, 
    };

    std::vector<Tile> hand{
        {Color::BLACK, 4}, {Color::BLACK, 10},
        {Color::BLUE, 1}, {Color::BLUE, 3}, 
        {Color::YELLOW, 13},
    };

    auto movelist = getMoveList(hand, board);
    std::vector<int> movescores{};
    for( auto move : movelist) {
        int score{0};

        std::cout << "Possible Move: ";
        if(move.empty()) {std::cout << "Draw "; score = -1;}

        
        for (auto tile : move) {
            score += tile.number;
            std::cout << "(" << colorToString(tile.color) << ", " << tile.number << ") ";
        }
        movescores.push_back(score);
        std::cout << score << std::endl;
    }

    int index = std::distance(movescores.begin(), std::max_element(movescores.begin(), movescores.end()));
    auto best_move = movelist[index];
    std::cout << "\nBest Move: ";
    for(auto tile : best_move) {
        std::cout << "(" << colorToString(tile.color) << ", " << tile.number << ") ";
    }
    std::cout << std::endl;

    board.insert(board.end(), best_move.begin(), best_move.end());
    std::cout << "\nBoard after making the best move: " << std::endl;
    for (const auto& group : valid_rummikub_arrangement(board)) {
        std::cout << "Group: ";
        for (const auto& tile : group) {
            std::cout << "(" << colorToString(tile.color) << ", " << tile.number << ") ";
        }
        std::cout << (is_run(group) ? "run" : "not run") << ", " << (is_set(group) ? "set" : "not set") << std::endl;
    }

    return 0;
}