// LibreSprite - Copyright (C) 2023  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <memory>
#include <typeinfo>

class Handle {
  std::weak_ptr<void*> ref;
  std::size_t typeHash;

public:
  Handle() = default;

  template<typename Type>
  Handle(const std::shared_ptr<Type*>& ptr) :
    ref{std::reinterpret_pointer_cast<void*>(ptr)},
    typeHash{typeid(Type).hash_code()}
    {}

  template<typename Type, typename Derived = Type>
  Derived* get() const {
    if (ref.expired())
      return nullptr;
    if (typeid(Type).hash_code() != typeHash)
      return nullptr;
    return static_cast<Derived*>(reinterpret_cast<Type*>(*ref.lock()));
  }

  operator bool () const {
    return !ref.expired() && !!ref.lock();
  }

  void reset() {
    ref.reset();
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
