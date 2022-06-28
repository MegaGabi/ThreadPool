#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class ThreadPool {
  using task_type = std::function<void()>;

public:
  ThreadPool(size_t num = std::thread::hardware_concurrency()) {
    for (size_t i = 0; i < num; ++i) {
      workers_.emplace_back(std::thread([this] {
        while (true) {
          task_type task;
          {
            std::unique_lock<std::mutex> lock(task_mutex_);
            task_cond_.wait(lock, [this] { return !tasks_.empty(); });
            task = std::move(tasks_.front());
            tasks_.pop();
          }
          if (!task) {
            std::cout << "worker #" << std::this_thread::get_id() << " exited" << std::endl;
            push_stop_task();
            return;
          }
          task();
        }
      }));
      std::cout << "worker #" << workers_.back().get_id() << " started" << std::endl;
    }
  }

  ~ThreadPool() {
    Stop();
  }

  void Stop() {
    push_stop_task();
    for (auto& worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }

    // clear all pending tasks
    std::queue<task_type> empty{};
    std::swap(tasks_, empty);
  }

  void Push(std::function<void()> fun) {
    {
      std::lock_guard<std::mutex> lock(task_mutex_);
      tasks_.emplace([fun] { fun(); });
    }
    task_cond_.notify_one();
  }

private:
  void push_stop_task() {
    std::lock_guard<std::mutex> lock(task_mutex_);
    tasks_.push(task_type{});
    task_cond_.notify_one();
  }

  std::vector<std::thread> workers_;
  std::queue<task_type> tasks_;
  std::mutex task_mutex_;
  std::condition_variable task_cond_;
};

int main()
{
    ThreadPool tp{};
    
    tp.Push([](){while(true){std::cout << "1";}});
    tp.Push([](){while(true){std::cout << "2";}});
    tp.Push([](){while(true){std::cout << "3";}});
    tp.Push([](){while(true){std::cout << std::endl;}});
    
    return 0;
}