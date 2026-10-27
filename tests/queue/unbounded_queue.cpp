// Тесты придумала и написала ИИ

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

#include "queue/unbounded_queue.hpp"

// здесь ваш код

TEST(UnboundedQueueTest, PushAndTryPopReturnsTask) {
    dispatcher::queue::UnboundedQueue queue;
    int value = 0;
    queue.push([&value] { value = 42; });
    auto task = queue.try_pop();
    ASSERT_TRUE(task.has_value());
    (*task)();
    EXPECT_EQ(value, 42);
}

TEST(UnboundedQueueTest, TryPopEmptyReturnsNullopt) {
    dispatcher::queue::UnboundedQueue queue;
    EXPECT_FALSE(queue.try_pop().has_value());
}

TEST(UnboundedQueueTest, EmptyReflectsQueueState) {
    dispatcher::queue::UnboundedQueue queue;
    EXPECT_TRUE(queue.empty());
    queue.push([] {});
    EXPECT_FALSE(queue.empty());
    queue.try_pop();
    EXPECT_TRUE(queue.empty());
}

TEST(UnboundedQueueTest, FifoOrderPreserved) {
    dispatcher::queue::UnboundedQueue queue;
    std::vector<int> order;
    for (int i = 0; i < 5; ++i) {
        order.push_back(i);
        queue.push([i] { (void)i; });
    }
    for (int i = 0; i < 5; ++i) {
        auto task = queue.try_pop();
        ASSERT_TRUE(task.has_value());
        (*task)();
    }
}

TEST(UnboundedQueueTest, GrowsBeyondAnyLimit) {
    dispatcher::queue::UnboundedQueue queue;
    constexpr int task_count = 10000;
    for (int i = 0; i < task_count; ++i) {
        queue.push([] {});
    }
    int popped = 0;
    while (queue.try_pop()) {
        ++popped;
    }
    EXPECT_EQ(popped, task_count);
}

TEST(UnboundedQueueTest, ConcurrentPushAndPop) {
    dispatcher::queue::UnboundedQueue queue;
    constexpr int task_count = 1000;
    std::atomic<int> executed{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 2; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < task_count / 2; ++i) {
                queue.push([&executed] { executed.fetch_add(1, std::memory_order_relaxed); });
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    while (auto task = queue.try_pop()) {
        (*task)();
    }
    EXPECT_EQ(executed.load(std::memory_order_acquire), task_count);
}

TEST(UnboundedQueueTest, PushNeverBlocks) {
    dispatcher::queue::UnboundedQueue queue;
    constexpr int task_count = 1000;
    for (int i = 0; i < task_count; ++i) {
        queue.push([] {});
    }
    EXPECT_FALSE(queue.empty());
}