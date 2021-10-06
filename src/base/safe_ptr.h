// LibreSprite
// Copyright (C) 2021 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

/*

safe_ptr is a pointer that becomes null (and so do all of its copies) automatically
when its initial instance is destroyed:

struct A {
  base::safe_ptr<A> ptr{this}; // initial instance
};

auto a = new A();
auto b = a->ptr; // b is a copy of ptr
delete a;
printf("b: %p\n", b.get()); // prints 0


If the class you want to use has a virtual destructor, you can also create a safe_ptr without
adding it manually, like this:

struct A {
  virtual ~A(){} // important!
};
auto a = base::make_safe<A>(); // `a` is a safe_ptr
auto b = a; // `b` is a safe copy of `a`
delete a.get();
if (b) printf("b is alive!\n"); // b is null, nothing is printed


If you can't access a class's initial safe_ptr (such as is the case with make_safe), and you
have a raw pointer to the class, you can get a copy safe_ptr like this:
void func(A* raw) {
  auto b = base::findSafePtr(raw);
  if (b) printf("b is alive!");
}

 */

#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <vector>

namespace base {

  template<typename Type>
  class safe_ptr {
    std::shared_ptr<Type*> storage;
    const bool owning = false;

  public:
    safe_ptr(std::nullptr_t) {}

    safe_ptr() = default;

    explicit safe_ptr(Type* ptr) :
      storage{std::make_shared<Type*>(ptr)},
      owning{true} {}

    safe_ptr(const safe_ptr<Type>& other) : storage{other.storage} {}
    safe_ptr<Type>& operator = (const safe_ptr<Type>& other) {storage = other.storage; return *this;}

    safe_ptr(safe_ptr<Type>&& other) : storage{other.storage} {}
    safe_ptr<Type>& operator = (safe_ptr<Type>&& other) {storage = other.storage; return *this;}

    bool operator == (Type* other) {
      if (!storage) return !other;
      return *storage == other;
    }

    bool operator != (Type* other) {
      if (!storage) return other;
      return *storage != other;
    }

    operator bool () const {
      return storage && *storage;
    }

    template<typename Derived = Type>
    Derived* get() const {
      return storage ? *storage : nullptr;
    }

    Type* operator -> () const {
      return *storage;
    }

    Type& operator * () const {
      return **storage;
    }

    template<typename T>
    operator T () const {
      return static_cast<T>(*storage);
    }

    ~safe_ptr(){
      if (owning)
        *storage = 0;
    }
  };

  namespace detail {

    inline std::vector<std::function<void()>>& getSafePtrPurgers() {
      static std::vector<std::function<void()>> purgers;
      return purgers;
    }

    template<typename Type>
    std::unordered_map<Type*, safe_ptr<Type>>& getSafePtrIndex() {
      static std::unordered_map<Type*, safe_ptr<Type>> index;
      static bool init = ([]{
        getSafePtrPurgers().push_back([]{
          auto it = index.begin();
          while (it != index.end()) {
            if (!it->second)
              it = index.erase(it);
            else
              it++;
          }
        });
        return true;
      })();
      (void) init;
      return index;
    }
  }

  template<typename Type>
  void saveSafePtr(Type* raw, safe_ptr<Type> safe) {
    detail::getSafePtrIndex<Type>().emplace(raw, safe);
  }

  template<typename Type>
  safe_ptr<Type> findSafePtr(Type* raw) {
    auto& index = detail::getSafePtrIndex<Type>();
    auto it = index.find(raw);
    return it == index.end() ? safe_ptr<Type>{nullptr} : it->second;
  }

  inline void purgeSafePtrs() {
    for (auto& purger : detail::getSafePtrPurgers()) {
      purger();
    }
  }

  template<typename Type,
           typename ... Args,
           typename std::enable_if<std::has_virtual_destructor<Type>::value, bool>::type = 0>
  safe_ptr<Type> make_safe(Args&& ... args) {
    class Safe : public Type {
      safe_ptr<Type> _ptr{this};
    public:
      Safe(Args&& ... args) : Type{std::forward<Args>(args)...} {
        saveSafePtr<Type>(this, _ptr);
      }
      safe_ptr<Type> _get_safe_ptr() {return _ptr;}
    };
    return (new Safe{std::forward<Args>(args)...})->_get_safe_ptr();
  }
}
