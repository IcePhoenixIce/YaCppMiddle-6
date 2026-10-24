#include "task_dispatcher.hpp"

#include <utility>

namespace dispatcher {

// здесь ваш код

TaskDispatcher::TaskDispatcher(size_t thread_count, QueueConfig config)
    : queue_(std::make_shared<queue::PriorityQueue>(config)),
      pool_(std::make_unique<thread_pool::ThreadPool>(queue_, thread_count)) {}

void TaskDispatcher::schedule(TaskPriority priority, std::function<void()> task) {
    queue_->push(priority, std::move(task));
}

TaskDispatcher::~TaskDispatcher() = default;

} // namespace dispatcher