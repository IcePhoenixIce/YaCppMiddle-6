#include "queue/bounded_queue.hpp"

#include <stdexcept>

namespace dispatcher::queue {

// здесь ваш код

BoundedQueue::BoundedQueue(int capacity) : capacity_(capacity) {
    if (capacity <= 0) {
        throw std::invalid_argument("BoundedQueue capacity must be positive");
    }
}

void BoundedQueue::push(std::function<void()> task) {
    std::unique_lock lock(mutex_);
    not_full_.wait(lock, [this] { return static_cast<int>(tasks_.size()) < capacity_; });
    tasks_.push_back(std::move(task));
    not_empty_.notify_one();
}

std::optional<std::function<void()>> BoundedQueue::try_pop() {
    std::lock_guard lock(mutex_);
    if (tasks_.empty()) {
        return std::nullopt;
    }
    std::function<void()> task = std::move(tasks_.front());
    tasks_.pop_front();
    not_full_.notify_one();
    return task;
}

bool BoundedQueue::empty() const {
    std::lock_guard lock(mutex_);
    return tasks_.empty();
}

BoundedQueue::~BoundedQueue() = default;

} // namespace dispatcher::queue