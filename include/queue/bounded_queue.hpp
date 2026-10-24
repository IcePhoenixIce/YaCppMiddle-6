#pragma once
#include "queue/queue.hpp"

#include <condition_variable>
#include <deque>
#include <mutex>

namespace dispatcher::queue {

class BoundedQueue : public IQueue {
    // здесь ваш код
    std::deque<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    int capacity_;

public:
    explicit BoundedQueue(int capacity);

    void push(std::function<void()> task) override;

    std::optional<std::function<void()>> try_pop() override;

    bool empty() const override;

    ~BoundedQueue() override;
};

}  // namespace dispatcher::queue