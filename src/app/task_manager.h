// LibreSprite
// Copyright (C) 2021 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

/*

This file implements the following 3 classes:
TaskManager - A singleton that runs tasks on separate threads
TaskHandle  - A class used for checking a task's status and/or aborting it
TaskMonitor - A helper class for automatically canceling a task when the monitor is destroyed

There are 3 ways to schedule a task in the TaskManager:

TaskManager::instance().delayed([]{
  // This task will be executed once, asynchronously, on the main thread
});

// Data can be any type
TaskManager::instance().addTask<Data>(
  [](std::atomic_bool& isAlive) -> Data {
    // This task will be executed multiple times on a worker thread
    // so long as isAlive isn't set to false
    isAlive = false;
    return Data{}; // each time it is executed, a Data will be passed to the consumer below
  },
  [](Data data) {
    // On the main thread the consumer receives the Data returned above
  }
);

TaskManager::instance().addTask<Data>(
  [] -> Data {
    // tasks without isAlive will only be executed once
    return Data{};
  },
  [](Data data) {
    // use data obtained from task
  }
);

 */

#pragma once

#include "ui/timer.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <deque>
#include <thread>
#include <unordered_set>

namespace app {
  namespace detail {
    struct Task {
      std::function<std::shared_ptr<void>(std::atomic_bool&)> funcTask;
      std::function<void(std::shared_ptr<void>)> funcCallback;
      std::function<void(Task&)> funcAbort;
      std::deque<std::shared_ptr<void>> data;
      std::mutex dataMutex;
      std::atomic_bool isAlive{true};
      bool isDone = false;
    };
  }

  class TaskHandle {
    std::weak_ptr<detail::Task> task;
  public:

    TaskHandle(std::shared_ptr<detail::Task> task = nullptr) : task{task} {}

    void abort() {
      if (auto strong = task.lock())
        strong->funcAbort(*strong);
    }

    bool done() {
      if (auto strong = task.lock())
        return strong->isDone;
      return true;
    }
  };

  class TaskMonitor {
    TaskHandle m_handle;
  public:
    void operator = (const TaskHandle& handle) {
      m_handle = handle;
    }

    operator bool () {
      return m_handle.done();
    }

    ~TaskMonitor(){
      m_handle.abort();
    }
  };

  class TaskManager {
    inject<ui::Timer> m_timer = ui::Timer::create(10);
    std::atomic_bool isAlive{true};
    std::array<std::thread, 8> threads;

    std::deque<std::shared_ptr<detail::Task>> pending;
    std::mutex pendingMutex;

    std::deque<std::shared_ptr<detail::Task>> ready;
    std::recursive_mutex readyMutex;

    std::array<std::shared_ptr<detail::Task>, std::tuple_size<decltype(threads)>::value> processing;
    std::mutex processingMutex;

    TaskManager() {
      m_timer->Tick.connect(&TaskManager::onTick, this);
      for (std::size_t i = 0; i < threads.size(); ++i) {
        threads[i] = std::thread([this, i]{this->thread(i);});
      }
    }

    ~TaskManager() {
      isAlive = false;
      m_timer->stop();

      {
        std::lock_guard<std::mutex> guard(processingMutex);
        for (auto task : processing) {
          if (task)
            task->funcAbort(*task);
        }
      }

      for (auto& thread : threads)
        thread.join();
    }

    void thread(int id) {
      std::shared_ptr<detail::Task> task;

      while (isAlive) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        {
          std::lock_guard<std::mutex> guard(pendingMutex);
          if (pending.empty()) {
            continue;
          }
          task = pending.front();
          pending.pop_front();
        }

        if (task->isAlive) {
          {
            std::lock_guard<std::mutex> guard(processingMutex);
            processing[id] = task;
          }

          do {
            try {
              auto data = task->funcTask(task->isAlive);
              {
                std::lock_guard<std::mutex> dataLock(task->dataMutex);
                task->data.push_back(data);
              }
              {
                std::lock_guard<std::recursive_mutex> guard(readyMutex);
                ready.push_back(task);
              }
            } catch (...) {
              task->isAlive = false;
            }
          } while (task->isAlive);
          task->isDone = true;

          {
            std::lock_guard<std::mutex> guard(processingMutex);
            processing[id].reset();
          }

        }

      }
    }

    std::shared_ptr<detail::Task> getReady(std::size_t& maxTasks) {
      if (maxTasks == 0)
        return nullptr;
      maxTasks--;
      std::lock_guard<std::recursive_mutex> guard(readyMutex);
      if (ready.empty())
        return nullptr;
      auto ret = ready.front();
      ready.pop_front();
      return ret;
    }

    void onTick() {
      std::size_t maxTasks;
      {
        std::lock_guard<std::recursive_mutex> guard(readyMutex);
        maxTasks = ready.size();
      }

      while (auto task = getReady(maxTasks)) {
        std::shared_ptr<void> data;
        {
          std::lock_guard<std::mutex> dataLock(task->dataMutex);
          if (!task->data.empty()) {
            data = task->data.front();
            task->data.pop_front();
          }
        }
        task->funcCallback(data);
      }
    }

  public:
    void delayed(std::function<void()>&& func) {
      if (!m_timer->isRunning())
        m_timer->start();
      std::lock_guard<std::recursive_mutex> guard(readyMutex);
      ready.emplace_back(new detail::Task{
          nullptr,
          [func=std::move(func)](std::shared_ptr<void>){
            func();
          }
        });
    }

    template<typename Data>
    TaskHandle addTask(std::function<Data(std::atomic_bool& isAlive)>&& worker, std::function<void(Data&&)>&& consumer) {
      if (!m_timer->isRunning())
        m_timer->start();
      std::lock_guard<std::mutex> guard(pendingMutex);
      pending.emplace_back(new detail::Task{
          [worker=std::move(worker)](std::atomic_bool& isAlive){
            return std::static_pointer_cast<void>(std::make_shared<Data>(worker(isAlive)));
          },
          [consumer=std::move(consumer)](std::shared_ptr<void> vdata){
            consumer(std::move(*std::static_pointer_cast<Data>(vdata)));
          },
          [](detail::Task& task){task.isAlive = false;}
        });
      return pending.back();
    }

    template<typename Data>
    TaskHandle addTask(std::function<Data()>&& worker, std::function<void(Data&&)>&& consumer, std::function<void()>&& aborter) {
      if (!m_timer->isRunning())
        m_timer->start();
      std::lock_guard<std::mutex> guard(pendingMutex);
      pending.emplace_back(new detail::Task{
          [worker=std::move(worker)](std::atomic_bool& isAlive){
            auto ret = std::static_pointer_cast<void>(std::make_shared<Data>(worker()));
            isAlive = false;
            return ret;
          },
          [consumer=std::move(consumer)](std::shared_ptr<void> vdata){
            consumer(std::move(*std::static_pointer_cast<Data>(vdata)));
          },
          [aborter=std::move(aborter)](detail::Task& task){
            task.isAlive = false;
            aborter();
          }
        });
      return pending.back();
    }

    static TaskManager& instance() {
      // static auto manager = new TaskManager();
      static TaskManager manager;
      return manager;
    }
  };

}
