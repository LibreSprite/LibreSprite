#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <utility>
#include <stdexcept>
#include <functional>

#include "JSON.hpp"

class Extension {
public:
    using Exportable = std::function<JSON::Value(JSON::Array&)>;
    using ExportableMap = std::unordered_map<std::string, Exportable>;

    template<typename Exportable, bool IsConstructor = false>
    class ExportableWrapper;

    class ExportableClass {
        std::type_index base;
        std::type_index derived;

    public:
        ExportableClass(const std::type_index& base, const std::type_index& derived) : base{base}, derived{derived} {}
        virtual ~ExportableClass() = default;

        using Exportable = std::function<JSON::Value(void* self, JSON::Array&)>;
        using ExportableMap = std::unordered_map<std::string, Exportable>;

        using Constructor = std::function<std::shared_ptr<void>(JSON::Array&)>;

        using Getter = std::function<JSON::Value(void* self)>;
        using Setter = std::function<void(void* self, JSON::Value&)>;
        struct GetSet {
            Getter get;
            Setter set;
        };
        using GetSetMap = std::unordered_map<std::string, GetSet>;

        std::type_index getBaseType() const {
            return base;
        }

        std::type_index getDerivedType() const {
            return derived;
        }

        const ExportableMap& getMethods() const {
            return methods;
        }

        const Constructor& getConstructor() const {
            return constructor;
        }

        const GetSetMap& getGetSet() const {
            return getsets;
        }

        ExportableWrapper<Exportable> addMethod(const std::string& name) {
            return {methods[name]};
        }

        ExportableWrapper<Getter> addGetter(const std::string& name) {
            return {this->getsets[name].get};
        }

        ExportableWrapper<Setter> addSetter(const std::string& name) {
            return {this->getsets[name].set};
        }

        ExportableWrapper<ExportableClass::Constructor, true> setConstructor() {
            return {constructor};
        }

    protected:
        Constructor constructor;
        ExportableMap methods;
        GetSetMap getsets;
    };

    using ExportableClassMap = std::unordered_map<std::string, std::shared_ptr<ExportableClass>>;

    template<typename Exportable, bool IsConstructor>
    class ExportableWrapper {
        Exportable& exportable;

        using Value = JSON::Value;

        // For generic types, directly use the result of the signature of its 'operator()'
        template <typename T>
        struct function_traits : public function_traits<decltype(&T::operator())> {};
        template <typename T>
        struct method_traits : public method_traits<decltype(&T::operator())> {};

        template <typename ReturnType, typename ... Args>
        struct function_traits<ReturnType(&)(Args...)> {
            enum { arity = sizeof...(Args) }; // arity is the number of arguments.
            typedef ReturnType result_type;

            template <typename Func, bool, typename Tuple, unsigned int ...I>
            struct helper {
                static auto call(Func&& func, Value* args){
                    return helper<Func, sizeof...(I) + 1 == std::tuple_size<Tuple>::value, Tuple, I..., sizeof...(I)>::call(func, args);
                }
            };

            template <typename Func, unsigned int ...I>
            struct helper<Func, true, std::tuple<Args...>, I...> {
                static auto call(Func&& func, Value* args) {
                    return func(static_cast<Args>(args[I])...);
                }
            };

            template <typename Func>
            static auto call(Func&& func, Value * args) {
                return helper<Func, sizeof...(Args) == 0, std::tuple<Args...>>::call(func, args);
            }
        };

        template <typename ReturnType, typename Self, typename ... Args>
        struct method_traits<ReturnType(&)(Self&, Args...)> {
            enum { arity = sizeof...(Args) }; // arity is the number of arguments.
            typedef ReturnType result_type;
            typedef Self self_type;

            template <typename Func, bool, typename Tuple, unsigned int ...I>
            struct helper {
                static auto call(Func&& func, Self& self, Value* args){
                    return helper<Func, sizeof...(I) + 1 == std::tuple_size<Tuple>::value, Tuple, I..., sizeof...(I)>::call(func, self, args);
                }
            };

            template <typename Func, unsigned int ...I>
            struct helper<Func, true, std::tuple<Args...>, I...> {
                static auto call(Func&& func, Self& self, Value* args) {
                    return func(*self, static_cast<Args>(args[I])...);
                }
            };

            template <typename Func>
            static auto call(Func&& func, Self& self, Value *args) {
                return helper<Func, sizeof...(Args) == 0, std::tuple<Args...>>::call(func, self, args);
            }
        };

        // we specialize for pointers to member function
        template <typename ClassType, typename ReturnType, typename... Args>
        struct function_traits<ReturnType(ClassType::*)(Args...) const> {
            enum { arity = sizeof...(Args) }; // arity is the number of arguments.

            typedef ReturnType result_type;

            template <typename Func, bool, typename Tuple, unsigned int ...I>
            struct helper {
                static auto call(Func&& func, Value* args){
                    return helper<Func, sizeof...(I) + 1 == std::tuple_size<Tuple>::value, Tuple, I..., sizeof...(I)>::call(func, args);
                }
            };

            template <typename Func, unsigned int ...I>
            struct helper<Func, true, std::tuple<Args...>, I...> {
                static auto call(Func&& func, Value* args) {
                    return func(static_cast<Args>(args[I])...);
                }
            };

            template <typename Func>
            static auto call(Func&& func, Value * args) {
                return helper<Func, sizeof...(Args) == 0, std::tuple<Args...>>::call(func, args);
            }
        };


        // we specialize for pointers to member function
        template <typename ClassType, typename ReturnType, typename Self, typename... Args>
        struct method_traits<ReturnType(ClassType::*)(Self&, Args...) const> {
            enum { arity = sizeof...(Args) }; // arity is the number of arguments.

            typedef ReturnType result_type;
            typedef Self self_type;

            template <typename Func, bool, typename Tuple, unsigned int ...I>
            struct helper {
                static auto call(Func&& func, Self* self, Value* args){
                    return helper<Func, sizeof...(I) + 1 == std::tuple_size<Tuple>::value, Tuple, I..., sizeof...(I)>::call(func, self, args);
                }
            };

            template <typename Func, unsigned int ...I>
            struct helper<Func, true, std::tuple<Args...>, I...> {
                static auto call(Func&& func, Self* self, Value* args) {
                    return func(*self, static_cast<Args>(args[I])...);
                }
            };

            template <typename Func>
            static auto call(Func&& func, Self* self, Value * args) {
                return helper<Func, sizeof...(Args) == 0, std::tuple<Args...>>::call(func, self, args);
            }
        };


        template<typename T>
        struct is_shared_ptr : std::false_type {};

        template<typename U>
        struct is_shared_ptr<std::shared_ptr<U>> : std::true_type {
            using element_type = U;
        };

    public:
        ExportableWrapper(Exportable& exportable) : exportable{exportable} {}
        ExportableWrapper(const ExportableWrapper&) = default;

        template<typename CB>
        ExportableWrapper& operator = (CB&& cb) {
            using result_type = typename function_traits<CB>::result_type;
            if constexpr (IsConstructor) {
                exportable = [cb = std::forward<CB>(cb)](JSON::Array& arguments) -> std::shared_ptr<void> {
                    auto argCount = function_traits<CB>::arity;
                    for (int i = arguments.size(); i < argCount; ++i) {
                        arguments.push_back({});
                    }
                    return function_traits<CB>::call(cb, arguments.data());
                };
            } else if constexpr (std::is_same_v<Exportable, Extension::ExportableClass::Exportable>) { // Method
                exportable = [cb = std::forward<CB>(cb)](void* vself, JSON::Array& arguments) -> JSON::Value {
                    auto argCount = method_traits<CB>::arity;
                    for (int i = arguments.size(); i < argCount; ++i) {
                        arguments.push_back({});
                    }
                    if constexpr (std::is_void_v<result_type>) {
                        method_traits<CB>::call(cb, static_cast<method_traits<CB>::self_type*>(vself), arguments.data());
                        return {};
                    } else {
                        return method_traits<CB>::call(cb, static_cast<method_traits<CB>::self_type*>(vself), arguments.data());
                    }
                };
            } else if constexpr (std::is_same_v<Exportable, Extension::ExportableClass::Getter>) { // Getter
                exportable = [cb = std::forward<CB>(cb)](void* vself) -> JSON::Value {
                    return method_traits<CB>::call(cb, static_cast<method_traits<CB>::self_type*>(vself), nullptr);
                };
            } else if constexpr (std::is_same_v<Exportable, Extension::ExportableClass::Setter>) { // Setter
                exportable = [cb = std::forward<CB>(cb)](void* vself, JSON::Value& value) -> void {
                    method_traits<CB>::call(cb, static_cast<method_traits<CB>::self_type*>(vself), &value);
                };
            } else { // if constexpr (std::is_same_v<Exportable, Extension::Exportable>) { // Function
                exportable = [cb = std::forward<CB>(cb)](JSON::Array& arguments) -> JSON::Value {
                    auto argCount = function_traits<CB>::arity;
                    for (int i = arguments.size(); i < argCount; ++i) {
                        arguments.push_back({});
                    }
                    if constexpr (std::is_void_v<result_type>) {
                        function_traits<CB>::call(cb, arguments.data());
                        return {};
                    } else {
                        return function_traits<CB>::call(cb, arguments.data());
                    }
                };
            }
            return *this;
        }

        ExportableWrapper& operator = (const ExportableWrapper&) = delete;
    };


private:
    ExportableMap functions;
    ExportableClassMap classes;

protected:
    ExportableWrapper<Exportable> addFunction(const std::string& name) {
        return {functions[name]};
    }

    Exportable& addVarArgFunction(const std::string& name) {
        return functions[name];
    }

    template<typename Class, typename Derived = Class>
    ExportableClass& addClass(const std::string& name) {
        auto& exp = classes[name];
        if (!exp) {
            auto instance = std::make_shared<ExportableClass>(typeid(Class), typeid(Derived));
            exp = instance;
        } else {
            std::cout << "DCA: " << name << std::endl;
            throw std::runtime_error{"Double class add: " + name};
        }
        return *exp;
    }

public:
    virtual ~Extension() = default;

    const ExportableMap& getFunctions() {
        return functions;
    }

    const ExportableClassMap& getClasses() {
        return classes;
    }

    virtual std::string language() {return "";}
    virtual std::string init(const std::string& language, JSON::Value& settings) {return "";}
    virtual void shutdown() {}

    virtual std::chrono::milliseconds tick() {return std::chrono::milliseconds(0);}
    virtual bool keepAlive() {return false;}
};
