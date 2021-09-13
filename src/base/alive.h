// Aseprite Base Library
// Copyright (c) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once
#include <stdio.h>
#include <unordered_set>
#include <functional>

namespace base {

class IsAlive {
    using Registry = std::unordered_set<IsAlive*>;

    static Registry& getRegistry() {
        static Registry registry;
        return registry;
    }

public:
    IsAlive(const IsAlive&) = delete;
    IsAlive(IsAlive&&) = delete;
    IsAlive& operator = (const IsAlive&) = delete;
    IsAlive& operator = (IsAlive&&) = delete;
    IsAlive() { getRegistry().insert(this); }

    virtual ~IsAlive() {
        if (onShutdown) onShutdown();
        getRegistry().erase(this);
    }

    bool isAlive() {
        return check(*this);
    }

    operator bool () {
        return check(*this);
    }

    static bool check(IsAlive& that) {
        auto& registry = getRegistry();
        return registry.find(&that) != registry.end();
    }

    std::function<void()> onShutdown;
};

template <typename Derived>
class AliveMonitor : public Derived, public IsAlive {
public:
    template<typename ... Args>
    AliveMonitor(Args&& ... args) : Derived{std::forward<Args>(args)...} {}
};

}
