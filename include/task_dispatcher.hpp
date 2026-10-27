#pragma once

#include <functional>
#include <memory>
#include <unordered_map>

#include "queue/priority_queue.hpp"
#include "thread_pool/thread_pool.hpp"
#include "types.hpp"

namespace dispatcher {

class TaskDispatcher {
    // здесь ваш код
    std::shared_ptr<queue::PriorityQueue> queue_;
    std::unique_ptr<thread_pool::ThreadPool> pool_;

public:
    using QueueConfig = std::unordered_map<TaskPriority, queue::QueueOptions>;

    TaskDispatcher(size_t thread_count,
                   QueueConfig config = {{TaskPriority::High, queue::QueueOptions{true, 1000}},
                                         {TaskPriority::Normal, queue::QueueOptions{false, std::nullopt}}});

    void schedule(TaskPriority priority, std::function<void()> task);
    ~TaskDispatcher();
};

}  // namespace dispatcher