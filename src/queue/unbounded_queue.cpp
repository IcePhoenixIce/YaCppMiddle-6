#include "queue/unbounded_queue.hpp"

#include <functional>
#include <mutex>
#include <queue>
#include <semaphore>

namespace dispatcher::queue {

// здесь ваш код

UnboundedQueue::UnboundedQueue() = default;

void UnboundedQueue::push(std::function<void()> task) {
    std::lock_guard lock(mutex_);
    tasks_.push_back(std::move(task));
}

std::optional<std::function<void()>> UnboundedQueue::try_pop() {
    std::lock_guard lock(mutex_);
    if (tasks_.empty()) {
        return std::nullopt;
    }
    std::function<void()> task = std::move(tasks_.front());
    tasks_.pop_front();
    return task;
}

bool UnboundedQueue::empty() const {
    std::lock_guard lock(mutex_);
    return tasks_.empty();
}

UnboundedQueue::~UnboundedQueue() = default;

}  // namespace dispatcher::queue