// LibreSprite - Copyright (C) 2023  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <memory>
#include <type_traits>
#include <typeinfo>

class Handle {
  std::weak_ptr<void*> ref;
  void (*m_dispose)(void*);
  std::size_t typeHash;

public:
  Handle() = default;

  template<typename Type>
  Handle(const std::shared_ptr<Type*>& ptr) :
    ref{std::shared_ptr<void*>(ptr, reinterpret_cast<void**>(ptr.get()))},
    m_dispose{[](void* ptr){delete reinterpret_cast<Type*>(ptr);}},
    typeHash{typeid(Type).hash_code()}
    {}

  template<typename Type>
  Handle(Type* ptr) : Handle{ptr->handle()} {}

  template<typename Type, typename Derived = Type>
  Derived* get() const {
    if (ref.expired())
      return nullptr;
    if constexpr (std::is_void<Type>::value) {
      return *ref.lock();
    } else {
      if (typeid(Type).hash_code() != typeHash)
        return nullptr;
      return static_cast<Derived*>(reinterpret_cast<Type*>(*ref.lock()));
    }
  }

  operator bool () const {
    return !ref.expired() && !!ref.lock();
  }

  void reset() {
    ref.reset();
  }

  void dispose() {
    if (ref.expired())
      return;
    auto ptr = *ref.lock();
    if (ptr && m_dispose)
      m_dispose(ptr);
  }
};

template <typename Base>
class WithHandle {
    std::shared_ptr<Base*> m_self;

public:
  using Handle = ::Handle;
  Handle handle() {
    if (!m_self)
      m_self = std::make_shared<Base*>(static_cast<Base*>(this));
    return {m_self};
  }
};
