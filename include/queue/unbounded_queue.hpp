#pragma once
#include "queue/queue.hpp"

#include <deque>
#include <mutex>

namespace dispatcher::queue {

class UnboundedQueue : public IQueue {
    // здесь ваш код
    std::deque<std::function<void()>> tasks_;
    std::mutex mutex_;

public:
    explicit UnboundedQueue(int capacity);

    void push(std::function<void()> task) override;

    std::optional<std::function<void()>> try_pop() override;

    bool empty() const override;

    ~UnboundedQueue() override;
};

}  // namespace dispatcher::queue