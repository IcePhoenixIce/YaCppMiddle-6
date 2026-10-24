#include "queue/priority_queue.hpp"

#include <utility>

namespace dispatcher::queue {

// здесь ваш код

bool PriorityQueue::HasTasksLocked() const {
    for (const auto &[priority, queue] : queues_) {
        (void)priority;
        if (!queue->empty()) {
            return true;
        }
    }
    return false;
}

PriorityQueue::PriorityQueue(const std::unordered_map<TaskPriority, QueueOptions> &options) {
    for (const auto &[priority, option] : options) {
        if (option.bounded) {
            if (!option.capacity) {
                throw std::invalid_argument("Bounded queue requires capacity");
            }
            queues_.emplace(priority, std::make_unique<BoundedQueue>(*option.capacity));
        } else {
            queues_.emplace(priority, std::make_unique<UnboundedQueue>(0));
        }
    }
    if (queues_.empty()) {
        throw std::invalid_argument("PriorityQueue requires at least one queue configuration");
    }
}

void PriorityQueue::push(TaskPriority priority, std::function<void()> task) {
    std::lock_guard lock(mutex_);
    auto it = queues_.find(priority);
    if (it == queues_.end()) {
        throw std::invalid_argument("No queue configured for given priority");
    }
    it->second->push(std::move(task));
    has_tasks_.notify_one();
}

std::optional<std::function<void()>> PriorityQueue::pop() {
    std::unique_lock lock(mutex_);
    has_tasks_.wait(lock, [this] { return shutdown_.load(std::memory_order_acquire) || HasTasksLocked(); });
    for (auto it = queues_.begin(); it != queues_.end(); ++it) {
        auto task = it->second->try_pop();
        if (task) {
            return task;
        }
    }
    return std::nullopt;
}

void PriorityQueue::shutdown() {
    shutdown_.store(true, std::memory_order_release);
    has_tasks_.notify_all();
}

PriorityQueue::~PriorityQueue() = default;

} // namespace dispatcher::queue