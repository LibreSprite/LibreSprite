// LibreSprite
// Copyright (C) 2021 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>

#include "ui/timer.h"
#include "ui/widget.h"

namespace app {
  namespace detail {
    struct Task {
      std::function<std::shared_ptr<void>(std::atomic_bool&)> funcTask;
      std::function<void(std::shared_ptr<void>)> funcCallback;
      std::function<void(Task&)> funcAbort;
      std::shared_ptr<void> data;
      std::atomic_bool isAlive{true};
      bool isDone = false;
    };
  }

  class TaskMonitor {
    std::weak_ptr<detail::Task> task;
  public:

    TaskMonitor(std::shared_ptr<detail::Task> task) : task{task} {}

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

  class TaskManager : public ui::Widget {
    ui::Timer m_timer{50, this};
    std::atomic_bool isAlive{true};
    std::array<std::thread, 8> threads;

    std::queue<std::shared_ptr<detail::Task>> pending;
    std::mutex pendingMutex;

    std::queue<std::shared_ptr<detail::Task>> ready;
    std::recursive_mutex readyMutex;

    std::array<std::shared_ptr<detail::Task>, std::tuple_size<decltype(threads)>::value> processing;
    std::mutex processingMutex;

    TaskManager() {
      m_timer.Tick.connect(&TaskManager::onTick, this);
      for (std::size_t i = 0; i < threads.size(); ++i) {
        threads[i] = std::thread([this, i]{this->thread(i);});
      }
    }

    ~TaskManager() {
      isAlive = false;
      m_timer.stop();

      {
        std::lock_guard<std::mutex> guard(processingMutex);
        for (auto task : processing) {
          if (task)
            task->funcAbort(*task);
        }
      }

      for (auto& thread : threads)
        thread.join();

      onTick();
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
          task = std::move(pending.front());
          pending.pop();
        }

        if (task->isAlive) {
          {
            std::lock_guard<std::mutex> guard(processingMutex);
            processing[id] = task;
          }

          try {
            task->data = task->funcTask(task->isAlive);
          } catch (...) {}

          {
            std::lock_guard<std::mutex> guard(processingMutex);
            processing[id].reset();
          }

          if (task->data) {
            std::lock_guard<std::recursive_mutex> guard(readyMutex);
            ready.emplace(std::move(task));
          }
        }

      }
    }

    void onTick() {
      std::lock_guard<std::recursive_mutex> guard(readyMutex);
      while (!ready.empty()) {
        auto task = std::move(ready.front());
        ready.pop();
        task->funcCallback(task->data);
      }
    }

  public:
    void delayed(std::function<void()>&& func) {
      if (!m_timer.isRunning())
        m_timer.start();
      std::lock_guard<std::recursive_mutex> guard(readyMutex);
      ready.emplace(new detail::Task{
          nullptr,
          [func=std::move(func)](std::shared_ptr<void>){
            func();
          },
          nullptr
        });
    }

    template<typename Data>
    TaskMonitor addTask(std::function<Data(std::atomic_bool& isAlive)>&& worker, std::function<void(Data)>&& consumer) {
      if (!m_timer.isRunning())
        m_timer.start();
      std::lock_guard<std::mutex> guard(pendingMutex);
      pending.emplace(new detail::Task{
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
    TaskMonitor addTask(std::function<Data()>&& worker, std::function<void(Data)>&& consumer, std::function<void()>&& aborter) {
      if (!m_timer.isRunning())
        m_timer.start();
      std::lock_guard<std::mutex> guard(pendingMutex);
      pending.emplace(new detail::Task{
          [worker=std::move(worker)](std::atomic_bool& isAlive){
            return std::static_pointer_cast<void>(std::make_shared<Data>(worker()));
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
