#ifndef RUMMIKUB_H
#define RUMMIKUB_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <set>
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <future>
#include <deque>
#include <condition_variable>

const int MIN_GROUP_SIZE = 3;

enum class Color {
    RED,
    BLUE,
    BLACK,
    YELLOW,
    JOKER,
};

struct Tile {
    Color color;
    int number;
};

std::string colorToString(Color color);

std::string state_hash(const std::vector<Tile>& remaining_tiles, const std::vector<std::vector<Tile>>& groups);

bool is_set(const std::vector<Tile>& group);

bool is_run(const std::vector<Tile>& group);

bool can_form_group(const Tile& tile, const std::vector<Tile>& group);

std::vector<std::vector<Tile>> valid_rummikub_arrangement(const std::vector<Tile>& tiles);

class SubsetsIterator {
public:
    SubsetsIterator(const std::vector<Tile>& tiles, size_t index)
        : tiles_(tiles), index_(index) {}

    std::vector<Tile> operator*() const {
        std::vector<Tile> subset;
        for (size_t i = 0; i < tiles_.size(); ++i) {
            if (index_ & (1 << i)) {
                subset.push_back(tiles_[i]);
            }
        }
        return subset;
    }

    SubsetsIterator& operator++() {
        ++index_;
        return *this;
    }

    bool operator!=(const SubsetsIterator& other) const {
        return index_ != other.index_;
    }

private:
    const std::vector<Tile>& tiles_;
    size_t index_;
};

class SubsetsGenerator {
public:
    SubsetsGenerator(const std::vector<Tile>& tiles) : tiles_(tiles) {}

    SubsetsIterator begin() const { return SubsetsIterator(tiles_, 0); }
    SubsetsIterator end() const { return SubsetsIterator(tiles_, 1 << tiles_.size()); }

private:
    const std::vector<Tile>& tiles_;
};

std::vector<std::vector<Tile>> getMoveList(std::vector<Tile> hand, std::vector<Tile> board);

class TaskPool {
public:
    TaskPool(size_t num_threads = std::thread::hardware_concurrency())
        : stop(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            threads.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        condition.wait(lock, [this] { return !tasks.empty() || stop; });
                        if (stop && tasks.empty()) {
                            return;
                        }
                        task = std::move(tasks.front());
                        tasks.pop_front();
                    }
                    task();
                }
            });
        }
    }

    ~TaskPool() {
        {
            std::unique_lock<std::mutex> lock(mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& t : threads) {
            t.join();
        }
    }

    template <typename F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(mutex);
            tasks.emplace_back(std::forward<F>(f));
        }
        condition.notify_one();
    }

private:
    std::vector<std::thread> threads;
    std::deque<std::function<void()>> tasks;
    std::mutex mutex;
    std::condition_variable condition;
    bool stop;
};

#endif