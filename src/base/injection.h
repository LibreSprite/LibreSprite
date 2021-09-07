// Aseprite Base Library
// Copyright (c) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

/*

This is an implementation of a simple container for Dependency Injection.

- What is Dependency Injection?

When you break your code up into multiple classes, they will depend on each other to be able to
carry out their own responsibilities. When a class has such a dependency, it knows it needs
a certain functionality, but it should not care about how that functionality is implemented.

For this clean separation to be possible, the "depender" should only know about an interface,
or a base class. The actual implementation should be provided to it by someone else:

  class Logger {
  public:
    void log(string) = 0;
  };

  class AccountManager {
    Logger& logger;
  public:
    AccountManager(Logger& logger) : logger(logger) {}
    bool withdraw(int amount) {
      logger.log("Withdrawing " + std::to_string(amount));
      return true;
    }
  };

With this, it is possible for the AccountManager to log its events into a file, database, terminal, etc:

  class STDOUTLogger : public Logger {
  public:
    void log(string s){ std::cout << s << std::endl; }
  };

  STDOUTLogger stdoutlogger;
  AccountManager accountManager(stdoutlogger);


- What is a Dependency Injection Container?

Receiving all the dependencies in constructors looks good enough in simple examples, but it
falls apart in real-world code, where your depenencies have their own dependencies and those
have dependencies, too. And what if you ever need a circular dependency (a GUI Widget might depend
on its parent Widget)?

This is where a container is useful. It allows you to register all your classes in one place,
and when something needs it, that dependency is constructed and made available:

  // say that other classes can depend on a Logger
  class Logger : public Injectable<Logger> {
  public:
    void log(string) = 0;
  };

  // implement a logger as you normally would...
  class STDOUTLogger : public Logger {
  public:
    void log(string s){ std::cout << s << std::endl; }
  };
  // Register STDOUTLogger as a regular class, with id "stdout":
  static Logger::Regular<STDOUTLogger> x("stdout");

  class AccountManager {
    inject<Logger> logger; // Ask the container to inject a logger of some kind
  public:
    bool withdraw(int amount) {
      logger->log("Withdrawing " + std::to_string(amount));
      return true;
    }
  };


- What if there are multiple implementations of a dependency?

Let's say now we have a logger that outputs to a file:
  // implement a logger as you normally would...
  class FileLogger : public Logger {
    std::fstream file;
  public:
    FileLogger(){ file.open("log.txt"); }
    void log(string s){ file << s << std::endl; }
  };
  // Register FileLogger as a regular class, with id "file":
  static Logger::Singleton y("file");

Before we can create an AccountManager, we configure the Container so that it
knows what your intent is:

  int main(){
    // When a Logger is requested, use the one with id "file"
    Logger::setDefault("file");
    AccountManager accountManager; // an instance of FileLogger will be created/injected
    accountManager.widthdraw(9000);
  }


- Wait, what is Logger::Singleton? And why is the STDOUTLogger "Regular"?

There are actually 3 ways to register a class (or three "policies").

"Regular" will create a new instance for each time it is injected.
This means that the injection is like a unique_ptr and it "owns" its
dependency, automatically deleting it for you. Each instance of AccountManager
would have its own instance of STDOUTLogger.

"Singleton" will create only *one* instance of that class. Any time it is injected,
the same instance will be returned. This means the injection does not own the dependency,
which is only deleted when the application shuts down. Multiple AccountManagers could
be created, but they would all share the same FileLogger.

The third policy is when a class registers itself using `Provides`. That is, it provides
itself as a dependency for anything that is constructed afterwards and it automatically
unregisters itself when it is destroyed. This is useful for situations where classes
have a two-way relationship:

  class AccountManager;

  class Person : public Injectable<Person> {
    inject<AccountManager> accountManager;
  public:
  };

  class Dude : public Person {};

  static Person::Register<Dude> p("dude");

  class AccountManager : public Injectable<AccountManager> {
    Provides p{this}; // register "this" as the default AccountManager
    inject<Person> person;
  public:
  };

  int main(){
    AccountManager am;
    // injects a Dude into AccountManager and that same AccountManager into Dude.
  }

Note that injections through Provides are also non-owning. When "am" goes out of scope,
it will be deleted and that will cause the deletion of Person. Person will not try
to delete AccountManager. Perfectly balanced...

*/

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
