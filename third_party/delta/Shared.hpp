#pragma once

#include <mutex>
#include <shared_mutex>
#include <condition_variable>

template <typename T,
          typename Mutex = std::shared_mutex,
          typename ReadLock = std::shared_lock<Mutex>,
          typename WriteLock = std::unique_lock<Mutex>>
class Shared {
    T value;
    mutable Mutex mutex;
    std::condition_variable_any cv;

public:

    template<typename CB>
    auto& operator << (CB&& cb) {
        {
            WriteLock lock{mutex};
            cb(value);
        }
        cv.notify_all();
        return *this;
    }

    template<typename CB>
    auto& operator >> (CB&& cb) const {
        ReadLock lock{mutex};
        cb(value);
        return *this;
    }

    template<typename Predicate>
    void wait(Predicate&& pred) {
        WriteLock lock{mutex};
        cv.wait(lock, [&]{ return pred(value); });
    }

    template<typename Predicate, typename Rep, typename Period>
    bool waitFor(const std::chrono::duration<Rep, Period>& duration, Predicate&& pred) {
        WriteLock lock{mutex};
        return cv.wait_for(lock, duration, [&]{ return pred(value); });
    }

    void notify() {
        cv.notify_all();
    }
};

template<typename T>
using RecursiveShared = Shared<
    T,
    std::recursive_mutex,
    std::unique_lock<std::recursive_mutex>,
    std::unique_lock<std::recursive_mutex>>;
