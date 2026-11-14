#pragma once

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <functional>
#include <vector>

// #include <iostream>

namespace di {
    using key_t = std::string_view;

    template<typename Base>
    key_t& active() {
        static key_t name;
        return name;
    }

    namespace internal {
        struct Injection {
            std::function<void*(Injection&)> constructor;
            unsigned int flags {};
            void setSingleton() {flags |= 1;}
            void clearSingleton() {flags &= ~1;}
            bool singleton() const {return flags & 1;}
        };
        using InjectionMap = std::unordered_map<key_t, Injection>;

        struct InjectionIndex {
            InjectionMap* map;
            key_t* key;
        };
        using InjectionMaps = std::vector<InjectionIndex>;

        inline InjectionMaps& getInjectionMaps() {
            static InjectionMaps maps;
            return maps;
        }

        template<typename Base>
        InjectionMap& getInjectionMap() {
            static InjectionMap map {([]{
                getInjectionMaps().push_back({
                        &map,
                        &active<Base>()
                    });
                return InjectionMap{};
            })()};
            return map;
        }

        template<typename Base>
        [[nodiscard]] Base* create(bool* singleton, key_t name = {}) {
            if (name == key_t{}) {
                name = active<Base>();
            }
            auto& map = getInjectionMap<Base>();
            if (auto it = map.find(name); it != map.end()) {
                auto& injection = it->second;
                auto instance = injection.constructor(injection);
                if (singleton)
                    *singleton = injection.singleton();
                return reinterpret_cast<Base*>(instance);
            }
            return nullptr;
        }

        template<typename Base, typename Cast>
        std::shared_ptr<Cast> create(key_t name = {}) {
            bool nonOwning;
	        auto raw = static_cast<Cast*>(internal::create<Base>(&nonOwning, name));
            if (nonOwning) {
                return std::shared_ptr<Cast>{raw, [](auto){}};
            }
            return std::shared_ptr<Cast>{raw, [&](auto ptr){delete ptr;}};
        }
    }

    template<typename Base>
    bool use(key_t n) {
        auto& map = internal::getInjectionMap<Base>();
        if (auto it = map.find(n); it != map.end()) {
            active<Base>() = it->first;
            return true;
            // } else {
            //     std::cout << "Missing " << n << std::endl;
        }
        return false;
    }

    inline bool use(key_t n) {
        bool found = false;
        for (auto& index : internal::getInjectionMaps()) {
            if (auto it = index.map->find(n); it != index.map->end()) {
                found = true;
                *index.key = n;
            }
        }
        return found;
    }

    template<typename Base, typename Derived = Base, bool singleton = false>
    class provide {
        template<typename ... Args>
        void reg(internal::Injection& injection, Args&& ... args){
            if (singleton) {
                injection.setSingleton();
                injection.constructor = [=](internal::Injection&){
                    // std::cout << "Singleton " << name << std::endl;
                    static Derived instance {args...};
                    return reinterpret_cast<void*>(&instance);
                };
            } else {
                injection.clearSingleton();
                injection.constructor = [=](internal::Injection&){
                    // std::cout << "Creating " << name << std::endl;
                    return reinterpret_cast<void*>(new Derived{args...});
                };
            }
        }

    public:
        template<typename ... Args>
        provide(const key_t name = key_t{}, Args&& ... args) {
            // std::cout << "Registered " << name << std::endl;
            auto& base = internal::getInjectionMap<Base>();
            auto& derived = internal::getInjectionMap<Derived>();
            if (name != key_t{}) {
                reg(base[name], std::forward<Args>(args)...);
                reg(derived[name], std::forward<Args>(args)...);
            } else {
                reg(base[typeid(Base).name()], std::forward<Args>(args)...);
                reg(base[typeid(Derived).name()], std::forward<Args>(args)...);
                reg(derived[typeid(Base).name()], std::forward<Args>(args)...);
                reg(derived[typeid(Derived).name()], std::forward<Args>(args)...);
            }
        }
    };

    template<typename Base, typename Derived = Base>
    using singleton = provide<Base, Derived, true>;

    template<typename Base, typename Cast = Base, bool strict = true>
    class inject : public std::shared_ptr<Cast> {
    public:
        inject(const key_t name = {}) :
            std::shared_ptr<Cast>{internal::create<Base, Cast>(name)} {
            if (!*this && strict) {
                auto str = std::string{name};
                if (str.empty())
                    str = std::string{di::active<Base>()};
                if (str.empty())
                    str = typeid(Base).name();

                std::stringstream ss;
                ss << "\nCould not create a " << str << "\n";
                ss << "Candidates:\n";
                for (auto& [key, _] : internal::getInjectionMap<Base>()) {
                    ss << "  " << key << "\n";
                }
                throw std::runtime_error{ss.str()};
            }
        }
    };

    template<typename Base>
    std::vector<std::shared_ptr<Base>> injectAll() {
        auto& map = internal::getInjectionMap<Base>();
        std::vector<std::shared_ptr<Base>> injections;
        injections.reserve(map.size());
        for (auto& [key, injection] : map) {
            auto instance = static_cast<Base*>(injection.constructor(injection));
            if (injection.singleton()) {
                injections.push_back(std::shared_ptr<Base>(instance, [](Base*){}));
            } else {
                injections.push_back(std::shared_ptr<Base>(instance));
            }
        }
        return injections;
    }
}
