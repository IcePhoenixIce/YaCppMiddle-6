#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "queue/bounded_queue.hpp"

TEST(BasicCheck, SimpleCheck) { EXPECT_EQ(1 + 1, 2); }

// здесь ваш код

TEST(BoundedQueueTest, PushAndTryPopReturnsTask) {
    dispatcher::queue::BoundedQueue queue(2);
    int value = 0;
    queue.push([&value] { value = 42; });
    auto task = queue.try_pop();
    ASSERT_TRUE(task.has_value());
    (*task)();
    EXPECT_EQ(value, 42);
}

TEST(BoundedQueueTest, TryPopEmptyReturnsNullopt) {
    dispatcher::queue::BoundedQueue queue(2);
    EXPECT_FALSE(queue.try_pop().has_value());
}

TEST(BoundedQueueTest, EmptyReflectsQueueState) {
    dispatcher::queue::BoundedQueue queue(2);
    EXPECT_TRUE(queue.empty());
    queue.push([] {});
    EXPECT_FALSE(queue.empty());
    queue.try_pop();
    EXPECT_TRUE(queue.empty());
}

TEST(BoundedQueueTest, FifoOrderPreserved) {
    dispatcher::queue::BoundedQueue queue(3);
    std::vector<int> order;
    for (int i = 0; i < 3; ++i) {
        order.push_back(i);
        queue.push([&order, i] { EXPECT_EQ(order.back(), i); });
    }
    for (int i = 0; i < 3; ++i) {
        auto task = queue.try_pop();
        ASSERT_TRUE(task.has_value());
        (*task)();
    }
}

TEST(BoundedQueueTest, PushBlocksWhenFull) {
    dispatcher::queue::BoundedQueue queue(1);
    queue.push([] {});
    std::atomic<bool> pushed{false};
    std::thread pusher([&] {
        queue.push([] {});
        pushed.store(true, std::memory_order_release);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(pushed.load(std::memory_order_acquire));
    auto task = queue.try_pop();
    ASSERT_TRUE(task.has_value());
    pusher.join();
    EXPECT_TRUE(pushed.load(std::memory_order_acquire));
}

TEST(BoundedQueueTest, PushUnblocksAfterPop) {
    dispatcher::queue::BoundedQueue queue(1);
    queue.push([] {});
    std::thread pusher([&] { queue.push([] {}); });
    queue.try_pop();
    pusher.join();
    EXPECT_TRUE(queue.try_pop().has_value());
}

TEST(BoundedQueueTest, ConcurrentPushAndPop) {
    dispatcher::queue::BoundedQueue queue(10);
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
    for (int t = 0; t < 2; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < task_count / 2; ++i) {
                auto task = queue.try_pop();
                if (task) {
                    (*task)();
                }
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

TEST(BoundedQueueTest, InvalidCapacityThrows) {
    EXPECT_THROW(dispatcher::queue::BoundedQueue(0), std::invalid_argument);
    EXPECT_THROW(dispatcher::queue::BoundedQueue(-1), std::invalid_argument);
}