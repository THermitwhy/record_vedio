#ifndef THREAD_POOL
#define THREAD_POOL
#include <iostream>
#include<functional>


#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <future>

class ThreadPool {
public:
    // 首先，在构造函数中，stop_ 被初始化为 false。stop_ 变量用于控制线程池是否停止运行，如果 stop_ 被设置为 true，那么线程池中的所有任务都将被停止。

    // 接下来，使用 for 循环创建 threadNum 个线程，并将它们存储在 workers_ 容器中。每个线程的工作内容由 lambda 表达式定义，lambda 表达式的形式是：this {...}。
    // 这里的 this 指向线程池对象本身，因为在 lambda 表达式中需要访问线程池的成员变量。
    // 这个 lambda 表达式的内容比较复杂，我们可以一步一步地分析它：

    // 首先，lambda 表达式的主体是一个无限循环（for(;;)），即线程会一直运行，直到线程池被关闭。

    // 在循环中，线程首先会尝试获取一个任务。它通过以下方式实现：

    // a. 创建一个 std::function<void()> 对象 task，用于存储待执行的任务。

    // b. 获取一个互斥锁（std::unique_lockstd::mutex ul(mtx_)）来保证线程安全。

    // c. 调用条件变量（std::condition_variable cv_）的 wait() 函数等待条件变量满足。

    // d. 在等待的过程中，使用 lambda 表达式作为参数来判断线程是否应该继续等待。如果 stop_ 被设置为 true，或者任务队列 tasks_ 不为空，那么 lambda 表达式返回 true，条件变量的 wait() 函数就会结束，线程将继续执行下面的代码；否则，线程会一直等待，直到条件变量被唤醒。

    // e. 如果线程被唤醒，那么它会检查 stop_ 变量的值。如果 stop_ 被设置为 true，并且任务队列 tasks_ 为空，那么线程会立即退出；否则，线程将继续执行下面的代码。

    // f. 从任务队列 tasks_ 中获取一个任务，并将其移动到 task 变量中。

    // g. 从任务队列 tasks_ 中移除已经获取的任务。

    // 最后，线程会执行 task() 函数来执行获取的任务
    explicit ThreadPool(size_t threadNum) : stop_(false) {
        for (size_t i = 0; i < threadNum; ++i) {
            workers_.emplace_back([this]() {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> ul(mtx_);
                        cv_.wait(ul, [this]() { return stop_ || !tasks_.empty(); });
                        if (stop_ && tasks_.empty()) { return; }
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
                });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> ul(mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& worker : workers_) {
            worker.join();
        }
    }

    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        auto taskPtr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        {
            std::unique_lock<std::mutex> ul(mtx_);
            if (stop_) { throw std::runtime_error("submit on stopped ThreadPool"); }
            tasks_.emplace([taskPtr]() { (*taskPtr)(); });
        }
        cv_.notify_one();
        return taskPtr->get_future();
    }

private:
    bool stop_;
    std::vector<std::thread> workers_;//工作线程
    std::queue<std::function<void()>> tasks_;//任务队列
    std::mutex mtx_;//类的互斥锁
    std::condition_variable cv_;//多线程条件变量
};
#endif