// Aseprite Base Library
// Copyright (c) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <cstddef>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>
#include <iostream>
#include <vector>

#ifdef __GNUG__
#include <cxxabi.h>
#define HAS_DEMANGLE
#endif

template<typename BaseClass_>
class inject {
public:
  using BaseClass = BaseClass_;

  inject(const std::string& name = "");

  inject(inject&& other) {
    std::swap(m_ptr, other.m_ptr);
    std::swap(onDetach, other.onDetach);
  }

  inject(const inject& other) = delete;

  ~inject() {onDetach(m_ptr);}

  BaseClass* operator -> () {return m_ptr;}
  BaseClass& operator * () {return *m_ptr;}
  operator bool () {return m_ptr;}
  operator BaseClass* () {return m_ptr;}

  template<typename Derived = BaseClass>
  Derived* get() {return dynamic_cast<Derived*>(m_ptr);}

private:
  BaseClass *m_ptr = nullptr;
  std::function<void(BaseClass*)> onDetach = [](BaseClass*){};
};

template<typename BaseClass_>
class Injectable {
public:
  using BaseClass = BaseClass_;
  using AttachFunction = std::function<BaseClass*()>;
  using DetachFunction = std::function<void(BaseClass*)>;

  struct RegistryEntry {
    AttachFunction attach;
    DetachFunction detach;
    void* data;
    std::unordered_set<std::string> flags;
    bool hasFlag(const std::string& flag) {
      return flags.find(flag) != flags.end();
    }
  };

  using Registry = std::unordered_map<std::string, RegistryEntry>;

  virtual std::string getName() const {
    int status;
    std::string result = typeid(*this).name();
#ifdef HAS_DEMANGLE
    auto name = abi::__cxa_demangle(result.c_str(), 0, 0, &status);
    if (status == 0) result = name;
    free(name);
#endif
    return result;
  }

  virtual ~Injectable() = default;

  static Registry& getRegistry() {
    static Registry registry;
    return registry;
  }

  static std::vector<inject<BaseClass>> getAllWithFlag(const std::string& flag) {
    std::vector<std::string> temp;
    std::vector<inject<BaseClass>> all;
    auto& registry = getRegistry();

    temp.reserve(registry.size());
    for (auto& entry : registry) {
      if (!entry.first.empty() && entry.second.hasFlag(flag))
        temp.emplace_back(entry.first);
    }

    all.reserve(temp.size());
    for (auto& entry : temp) {
      all.emplace_back(entry);
    }
    return all;
  }

  static bool setDefault(const std::string& name, bool canFallback = true) {
    auto& registry = getRegistry();
    auto it = registry.find(name);
    if (it == registry.end()) {
      std::cout << "Invalid default: " << name << std::endl;
      if (registry.size() && canFallback) {
        it = registry.begin();
        std::cout << "Falling back to: " << it->first << std::endl;
      } else {
        std::cout << "Nothing to fall back to." << std::endl;
        return false;
      }
    }
    registry[""] = it->second;
    return true;
  }

  template<typename DerivedClass>
  class Regular {
  public:
    Regular(const std::string& name, const std::unordered_set<std::string>& flags = {}) {
      // std::cout << "Registered regular class " << name << std::endl;
      Injectable<BaseClass>::getRegistry()[name] = {
        []{
          auto ret = new DerivedClass();
          std::cout << "Constructed " << ret->getName() << std::endl;
          return ret;
        },
        [](BaseClass* instance){
          auto name = instance->getName();
          delete instance;
          std::cout << "Destroyed " << name << std::endl;
        },
        nullptr,
        flags
      };
    }
  };

  template<typename DerivedClass>
  class Singleton {
  public:
    Singleton(const std::string& name, const std::unordered_set<std::string>& flags = {}) {
      Injectable<BaseClass>::getRegistry()[name] = {
        []{
          static DerivedClass instance;
          return &instance;
        },
        [](BaseClass* ptr){},
        nullptr,
        flags
      };
    }
  };

  class Provides {
  public:
    std::string m_name;

    ~Provides(){
      auto& registry = Injectable<BaseClass>::getRegistry();
      auto iterator = registry.find(m_name);
      if (iterator != registry.end() && iterator->second.data == this) {
        registry.erase(iterator);
      }
    }

    Provides(BaseClass* instance, const std::string& name = "", const std::unordered_set<std::string>& flags = {}) {
      m_name = name;
      Injectable<BaseClass>::getRegistry()[name] = {
        [=] {return instance;},
        [](BaseClass* ptr) {},
        this,
        flags
      };
    }
  };
};

template<typename BaseClass_>
inject<BaseClass_>::inject(const std::string& name) {
  auto& registry = Injectable<BaseClass>::getRegistry();
  auto it = registry.find(name);
  if (it != registry.end()) {
    auto& registryEntry = it->second;
    onDetach = registryEntry.detach;
    m_ptr = registryEntry.attach();
  } else {
    std::cout << "Could not create " << name << std::endl;
  }
}
