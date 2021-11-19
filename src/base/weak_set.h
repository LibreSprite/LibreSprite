#pragma once

#include <unordered_map>
#include <memory>
#include <atomic>

#include "base/safe_ptr.h"

namespace base {
/* * / // To-do: Make sure all widgets are shared
template<typename Type>
class weak_set {
  using Container = std::unordered_map<Type*, std::weak_ptr<Type>>;
  Container set;
  std::unordered_map<Type*, std::weak_ptr<Type>> pending;
  std::atomic_uint lockCount;

  void purge() {
    for (auto& entry : pending) {
      if (auto value = entry.second.lock()) {
        set[value.get()] = value;
      } else {
        set.erase(entry.first);
      }
    }
    pending.clear();
  }

  template<typename Iterator>
  class metaiterator {
    Iterator iterator;
    weak_set<Type>& ws;
    std::shared_ptr<Type> locked;

  public:
    metaiterator(Iterator it, weak_set<Type>& ws) : iterator(it), ws(ws) {
      ws.lockCount++;
      while (it != ws.set.end()) {
        locked = it->second.lock();
        if (locked)
          break;
        ws.pending[it->first] = {};
        it++;
      }
    }

    ~metaiterator() {
      if (!--ws.lockCount) {
        ws.purge();
      }
    }

    bool operator != (const metaiterator<Iterator>& other) {
      return iterator != other.iterator;
    }

    bool operator == (const metaiterator<Iterator>& other) {
      return iterator == other.iterator;
    }

    const std::shared_ptr<Type>& operator * () { return locked; }

    metaiterator<Iterator>& operator++ () {
      while(true) {
        iterator++;
        if (iterator == ws.set.end())
          break;
        auto value = iterator->second.lock();
        if (value) {
          locked = value;
          break;
        }
        ws.pending[iterator->first] = {};
      };
      return *this;
    }

    // auto& operator++ (int) { ++iterator; return *this; }
  };

public:
  using iterator = metaiterator<typename Container::iterator>;

  void insert(const std::shared_ptr<Type>& value) {
    if (lockCount) {
      pending[value.get()] = value;
    } else {
      set[value.get()] = value;
    }
  }

  void erase(Type* value) {
    if (lockCount) {
      pending[value] = {};
    } else {
      set.erase(value);
    }
  }

  void erase(const std::shared_ptr<Type>& value) {
    if (lockCount) {
      pending[value.get()] = nullptr;
    } else {
      set.erase(value.get());
    }
  }

  void clear() {
    set.clear();
    pending.clear();
  }

  iterator begin() {
    return iterator(set.begin(), *this);
  }

  iterator end() {
    return iterator(set.end(), *this);
  }
};

/*/

template<typename Type>
class weak_set {
  using Container = std::unordered_map<Type*, base::safe_ptr<Type>>;
  Container set;
  std::unordered_map<Type*, base::safe_ptr<Type>> pending;
  std::atomic_uint lockCount;

  void purge() {
    for (auto& entry : pending) {
      if (auto value = entry.second.get()) {
        set[value] = {entry.second};
      } else {
        set.erase(entry.first);
      }
    }
    pending.clear();
  }

  template<typename Iterator>
  class metaiterator {
    Iterator iterator;
    weak_set<Type>& ws;
    Type* locked;

  public:
    metaiterator(Iterator it, weak_set<Type>& ws) : iterator(it), ws(ws) {
      ws.lockCount++;
      while (it != ws.set.end()) {
        locked = it->second.get();
        if (locked)
          break;
        ws.pending[it->first] = {nullptr};
        it++;
      }
    }

    ~metaiterator() {
      if (!--ws.lockCount) {
        ws.purge();
      }
    }

    bool operator != (const metaiterator<Iterator>& other) {
      return iterator != other.iterator;
    }

    bool operator == (const metaiterator<Iterator>& other) {
      return iterator == other.iterator;
    }

    Type* operator * () { return locked; }

    metaiterator<Iterator>& operator++ () {
      while(true) {
        iterator++;
        if (iterator == ws.set.end())
          break;
        if (auto value = iterator->second.get()) {
          locked = value;
          break;
        }
        ws.pending[iterator->first] = {nullptr};
      };
      return *this;
    }

    // auto& operator++ (int) { ++iterator; return *this; }
  };

public:
  using iterator = metaiterator<typename Container::iterator>;

  void insert(const base::safe_ptr<Type>& value) {
    if (!value)
      return;
    if (lockCount) {
      pending[value.get()] = value;
    } else {
      set[value.get()] = value;
    }
  }

  void erase(Type* value) {
    if (!value)
      return;
    if (lockCount) {
      pending[value] = {nullptr};
    } else {
      set.erase(value);
    }
  }

  void erase(const std::shared_ptr<Type>& value) {
    if (lockCount) {
      pending[value.get()] = {nullptr};
    } else {
      set.erase(value.get());
    }
  }

  void clear() {
    set.clear();
    pending.clear();
  }

  iterator begin() {
    return iterator(set.begin(), *this);
  }

  iterator end() {
    return iterator(set.end(), *this);
  }
};
/* */
}
