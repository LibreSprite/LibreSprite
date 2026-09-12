#include "Interpreter.hpp"
#include "Shared.hpp"
#include "di.hpp"
#include <cstdio>
#include <iostream>
#include <list>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <sstream>
#include <fstream>

extern "C" {
#include "quickjs.h"
#include "quickjs-libc.h"
}

static Shared<std::vector<std::pair<JSContext*, JSValue>>> deleteQueue;
static void clearQueue() {
    deleteQueue << [](auto& queue) {
        for (auto& [ctx, value] : queue) {
            JS_FreeValue(ctx, value);
        }
        queue.clear();
    };
}

// Unhandled promise rejections, buffered instead of handled by
// js_std_promise_rejection_tracker: the std tracker feeds
// js_std_promise_rejection_check, which prints and calls exit(1) inside
// js_std_loop() — fatal for an embedded app. Entries are drained (and
// reported to stderr) by eval()/tick() and cleared at teardown.
static std::vector<std::tuple<JSContext*, JSValue, JSValue>> unhandledRejections;

static void rejectionTracker(JSContext* ctx, JSValueConst promise, JSValueConst reason, bool is_handled, void* opaque) {
    if (is_handled) {
        for (auto it = unhandledRejections.begin(); it != unhandledRejections.end(); ++it) {
            auto& [c, p, r] = *it;
            if (JS_IsSameValue(ctx, p, promise)) {
                JS_FreeValue(c, p);
                JS_FreeValue(c, r);
                unhandledRejections.erase(it);
                break;
            }
        }
        return;
    }
    for (auto& [c, p, r] : unhandledRejections) {
        if (JS_IsSameValue(ctx, p, promise))
            return;
    }
    unhandledRejections.emplace_back(ctx, JS_DupValue(ctx, promise), JS_DupValue(ctx, reason));
}

// Reports and frees the buffered unhandled rejections.
static void drainUnhandledRejections() {
    for (auto& [ctx, promise, reason] : unhandledRejections) {
        fprintf(stderr, "Possibly unhandled promise rejection: ");
        const char* msg = JS_ToCString(ctx, reason);
        if (msg) {
            fputs(msg, stderr);
            JS_FreeCString(ctx, msg);
        }
        fputs("\n", stderr);
        JS_FreeValue(ctx, promise);
        JS_FreeValue(ctx, reason);
    }
    unhandledRejections.clear();
}

struct JSHandle {
    JSContext* ctx;
    JSValue value;

    JSHandle(JSContext* ctx, JSValue value) : ctx{ctx}, value{value} {}

    JSHandle(JSHandle&& other) : ctx{other.ctx}, value{other.value} {
        other.ctx = nullptr;
    }

    JSHandle(const JSHandle& other) : ctx{other.ctx} {
        if (ctx)
            value = JS_DupValue(ctx, other.value);
    }

    ~JSHandle() {
        reset();
    }

    JSValue release() {
        ctx = nullptr;
        return value;
    }

    void reset() {
        if (!ctx)
            return;
        JS_FreeValue(ctx, value);
        ctx = nullptr;
    }

    JSHandle& operator=(JSHandle&& other) {
        if (this != &other) {
            reset();
            ctx = other.ctx;
            value = other.value;
            other.ctx = nullptr;
        }
        return *this;
    }

    JSHandle& operator=(const JSHandle& other) {
        if (this != &other) {
            reset();
            ctx = other.ctx;
            value = JS_DupValue(ctx, other.value);
        }
        return *this;
    }

    operator JSValue& () {
        return value;
    }
};

struct JSString {
    JSContext* ctx{};
    const char* value{};

    JSString(JSContext* ctx, const char* value) : ctx{ctx}, value{value} {}

    JSString(JSString&& other) : ctx{other.ctx}, value{other.value} {
        other.value = nullptr;
    }

    JSString(const JSString& other) = delete;

    ~JSString() {
        if (value)
            JS_FreeCString(ctx, value);
    }

    operator const char* () {
        return value;
    }
};


JSValue makeWeakRef(JSContext* ctx, JSValueConst target) {
    JSHandle global {ctx, JS_GetGlobalObject(ctx)};
    JSHandle weakRefCtor {ctx, JS_GetPropertyStr(ctx, global, "WeakRef")};

    if (JS_IsException(weakRefCtor)) {
        return weakRefCtor.release();
    }

    if (JS_IsUndefined(weakRefCtor) || JS_IsNull(weakRefCtor)) {
        return JS_ThrowTypeError(ctx, "WeakRef is not available in this runtime");
    }

    return JS_CallConstructor(ctx, weakRefCtor, 1, &target);
}

JSValue lockWeakRef(JSContext* ctx, JSValueConst weakRefObj) {
    JSValue derefFn = JS_GetPropertyStr(ctx, weakRefObj, "deref");
    if (JS_IsException(derefFn))
        return derefFn;

    JSValue res = JS_Call(ctx, derefFn, weakRefObj, 0, NULL);
    JS_FreeValue(ctx, derefFn);
    return res; // target or undefined (or exception)
}

class QuickJSInterpreter : public Interpreter {
public:
    struct VoidWrapper;
    std::unordered_map<void*, VoidWrapper*> wrappers;
    std::shared_ptr<JSRuntime> rt;
    std::shared_ptr<JSContext> ctx;
    JSClassID firstClassId{0};
    std::size_t classIdCount{0};
    std::vector<std::string> moduleSearchPaths;

    static inline std::unordered_map<JSContext*, QuickJSInterpreter*> contextMap;

    QuickJSInterpreter() {
        rt = std::shared_ptr<JSRuntime>{JS_NewRuntime(), [](auto* rt){
            if (rt)
                JS_FreeRuntime(rt);
        }};
        if (!rt)
            throw std::runtime_error{"Failed to create JSRuntime"};

        js_std_init_handlers(rt.get());
        JS_SetHostPromiseRejectionTracker(rt.get(), rejectionTracker, NULL);

        ctx = std::shared_ptr<JSContext>{JS_NewContext(rt.get()), [](auto* ctx){
            if (ctx)
                JS_FreeContext(ctx);
        }};

        if (!ctx)
            throw std::runtime_error{"Failed to create JSContext"};

        contextMap[ctx.get()] = this;

        JS_SetModuleLoaderFunc(rt.get(), NULL, module_loader, NULL);
    }

    static JSModuleDef *module_loader(JSContext *ctx, const char *module_name, void *opaque) {
        JSModuleDef *m;

        std::string path{module_name};
        std::ifstream fstr{path};
        if (!fstr) {
            // Unresolved (bare) specifiers: search the configured module
            // paths in order.
            if (auto it = contextMap.find(ctx); it != contextMap.end()) {
                for (auto& dir : it->second->moduleSearchPaths) {
                    std::string candidate = dir + "/" + module_name;
                    for (auto& tryPath : {candidate, candidate + ".js"}) {
                        fstr.open(tryPath);
                        if (fstr) {
                            path = tryPath;
                            break;
                        }
                    }
                    if (fstr)
                        break;
                }
            }
        }
        if (!fstr) {
            JS_ThrowReferenceError(ctx, "could not read module filename '%s'", module_name);
            return NULL;
        }

        std::stringstream sstr;
        sstr << fstr.rdbuf();
        auto fileString = sstr.str();
        size_t buf_len = fileString.size();
        auto buf = fileString.data();
        JSValue func_val;

        /* compile the module with the resolved path as its name, so its
           own relative imports resolve against it and stack traces show
           the real file */
        func_val = JS_Eval(ctx, buf, buf_len, path.c_str(), JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
        if (JS_IsException(func_val))
            return NULL;
        if (js_module_set_import_meta(ctx, func_val, false, false) < 0) {
            JS_FreeValue(ctx, func_val);
            return NULL;
        }
        /* the module is already referenced, so we must free it */
        m = reinterpret_cast<JSModuleDef*>(JS_VALUE_GET_PTR(func_val));
        JS_FreeValue(ctx, func_val);

        return m;
    }

    ~QuickJSInterpreter() {
        clearQueue();
        // Drop the buffered rejections (their dup refs would otherwise
        // keep objects alive and trip JS_FreeRuntime's gc assertion).
        drainUnhandledRejections();
        for (auto& [_, wrapper] : wrappers) {
            wrapper->weak.reset();
        }
        wrappers.clear();
        js_std_free_handlers(rt.get());
        contextMap.erase(ctx.get());
    }

    static JSValue valueToJSValue(JSContext* ctx, JSON::Value& value) {
        if (value.isUndefined()) {
            return JS_UNDEFINED;
        }
        if (value.isNull()) {
            return JS_NULL;
        }
        if (value.isBoolean()) {
            return JS_NewBool(ctx, value.boolean() ? 1 : 0);
        }
        if (value.isNumber()) {
            return JS_NewFloat64(ctx, value.number());
        }
        if (value.isString()) {
            auto& str = value.string();
            return value.isBinString() ? JS_NewStringLenBin(ctx, str.c_str(), str.size()) :
                JS_NewStringLen(ctx, str.c_str(), str.size());
        }
        if (value.isArray()) {
            JSValue arr = JS_NewArray(ctx);
            auto& jsonArr = value.array();
            for (size_t i = 0, max = jsonArr.size(); i < max; ++i) {
                JS_SetPropertyUint32(ctx, arr, i, valueToJSValue(ctx, jsonArr[i]));
            }
            return arr;
        }
        if (value.isObject()) {
            auto& jsonObj = value.object();
            if (jsonObj.memo.has_value()) {
                try {
                    return std::any_cast<JSHandle>(jsonObj.memo).release();
                } catch (const std::bad_any_cast&) {
                }
            }
            JSValue obj = JS_NewObject(ctx);
            for (auto& [key, val] : jsonObj) {
                JS_SetPropertyStr(ctx, obj, key.c_str(), valueToJSValue(ctx, val));
            }
            return obj;
        }
        if (value.isByteArray()) {
            auto& vec = value.byteArray();
            return JS_NewUint8ArrayCopy(ctx, vec.data(), vec.size());
        }
        if (value.isNative()) {
            auto& [ptr, type] = value.native();
            auto instance = contextMap[ctx];
            if (auto it = instance->wrappers.find(ptr.get()); it != instance->wrappers.end()) {
                if (auto strong = lockWeakRef(ctx, it->second->weak.value); !JS_IsUndefined(strong)) {
                    return strong;
                }
            }
            auto def = instance->classDefByType.find(type);
            if (def != instance->classDefByType.end()) {
                return def->second->jsCtor(ptr);
            }
            std::cout << "No class definition for type: " << type.name() << std::endl;
            return JS_UNDEFINED;
        }

        if (value.isFunction()) {
            try {
                return std::any_cast<JSHandle>(value.functionMemo()).release();
            } catch (const std::bad_any_cast&) {
            }
        }

        std::cout << "INVALID JS VALUE: " << value.type() << std::endl;
        return JS_UNDEFINED;
    }

    void JSValueToValue(JSValueConst jsValue, JSON::Value& value, std::unordered_map<void*, JSON::Value*>& map) {
        auto ctx = this->ctx.get();

        if (JS_IsUndefined(jsValue)) {
            value = JSON::Value{JSON::Special::Undefined};
            return;
        }
        if (JS_IsNull(jsValue)) {
            value = JSON::Value{JSON::Special::Null};
            return;
        }
        if (JS_IsBool(jsValue)) {
            int b = JS_ToBool(ctx, jsValue);
            value = JSON::Value{b != 0};
            return;
        }
        if (JS_IsNumber(jsValue)) {
            double num;
            JS_ToFloat64(ctx, &num, jsValue);
            value = JSON::Value{num};
            return;
        }

        if (JS_IsString(jsValue)) {
            size_t len{};
            JSString str {ctx, JS_ToCStringLen(ctx, &len, jsValue)};
            value = JSON::Value{std::string{str, len}};
            return;
        }

        int taType = JS_GetTypedArrayType(jsValue);
        if (taType == JS_TYPED_ARRAY_UINT8 || taType == JS_TYPED_ARRAY_UINT8C) {
            size_t byteLen{};
            uint8_t* bytePtr = JS_GetUint8Array(ctx, &byteLen, jsValue);
            if (bytePtr) {
                auto vec = std::make_shared<std::vector<uint8_t>>(bytePtr, bytePtr + byteLen);
                value = JSON::Value{vec};
                return;
            }
        }

        if (JS_IsException(jsValue)) {
            JSHandle exc {ctx, JS_GetException(ctx)};
            JSString msg {ctx, JS_ToCString(ctx, exc)};
            value.object();
            std::string errmsg = (msg ? msg : "<unknown>");
            JSValue stack = JS_GetPropertyStr(ctx, exc, "stack");
            if (!JS_IsUndefined(stack)) {
                JSString stackStr {ctx, JS_ToCString(ctx, stack)};
                if (stackStr.value) {
                    errmsg += "\n";
                    errmsg += stackStr.value;
                }
            }
            JS_FreeValue(ctx, stack);
            value["exception"] = std::string("JavaScript exception: ") + errmsg;
            return;
        }

        if (JS_IsFunction(ctx, jsValue)) {
            auto wrapper = [this, func = JSHandle{ctx, JS_DupValue(ctx, jsValue)}](const JSON::Array& args) -> JSON::Value {
                auto ctx = this->ctx.get();
                std::vector<JSValue> argv;
                std::vector<JSHandle> argh;
                argv.reserve(args.size());
                argh.reserve(args.size());
                for (auto& arg : args) {
                    argv.push_back(valueToJSValue(ctx, *(JSON::Value*)&arg));
                    argh.emplace_back(ctx, argv.back());
                }
                JSHandle jsret{
                    ctx,
                    JS_Call(ctx, func.value, JS_NULL, argv.size(), argv.data())
                };
                JSON::Value ret{JSON::Special::Undefined};
                std::unordered_map<void*, JSON::Value*> map;
                JSValueToValue(jsret, ret, map);
                clearQueue();
                return ret;
            };

            value = JSON::FunctionRef {
                std::move(wrapper),
                JSHandle{ctx, JS_DupValue(ctx, jsValue)}
            };

            return;
        }

        auto ptr = JS_VALUE_GET_PTR(jsValue);
        if (auto it = map.find(ptr); it != map.end()) {
            value = it->second;
            return;
        }

        if (JS_IsArray(jsValue)) {
            int64_t len;
            JS_GetLength(ctx, jsValue, &len);
            auto arr = std::make_shared<JSON::Array>();
            value = JSON::Value{arr};
            map[ptr] = &value;
            for (uint32_t i = 0; i < len; ++i) {
                JSHandle elem {ctx, JS_GetPropertyUint32(ctx, jsValue, i)};
                JSON::Value val;
                JSValueToValue(elem, val, map);
                arr->push_back(val);
            }
            return;
        }
        if (JS_IsObject(jsValue)) {
            JSClassID clsId; // = JS_GetClassID(jsValue);
            auto wrapper = static_cast<VoidWrapper*>(JS_GetAnyOpaque(jsValue, &clsId));
            if (wrapper && clsId >= firstClassId && clsId < firstClassId + classIdCount) {
                value.native() = {wrapper->native, wrapper->base};
                return;
            }

            auto obj = std::make_shared<JSON::Object>();
            obj->memo = JSHandle{ctx, JS_DupValue(ctx, jsValue)};
            value = JSON::Value{obj};
            map[ptr] = &value;
            JSPropertyEnum* props;
            uint32_t propCount;
            JS_GetOwnPropertyNames(ctx, &props, &propCount, jsValue, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY);
            for (uint32_t i = 0; i < propCount; ++i) {
                JSString key {ctx, JS_AtomToCString(ctx, props[i].atom)};
                std::string strkey{key};
                JSHandle propVal {ctx, JS_GetProperty(ctx, jsValue, props[i].atom)};
                JSON::Value val;
                JSValueToValue(propVal, val, map);
                (*obj)[strkey] = val;
                JS_FreeAtom(ctx, props[i].atom);
            }
            js_free(ctx, props);
            return;
        }
        if (JS_IsException(jsValue)) {
            JSHandle exc {ctx, JS_GetException(ctx)};
            JSString msg {ctx, JS_ToCString(ctx, exc)};
            value.object();
            value["exception"] = std::string("JavaScript exception: ") + (msg ? msg : "<unknown>");
            return;
        }
        std::cout << "Unknown var type" << std::endl;
        value = JSON::Value{JSON::Special::Undefined};
    }

    void addGlobalValue(const std::string& name, JSON::Value& value) override {
        JSHandle global{ctx.get(), JS_GetGlobalObject(ctx.get())};
        auto jsvalue = valueToJSValue(ctx.get(), value);
        JS_SetPropertyStr(ctx.get(), global, name.c_str(), jsvalue);
    }

    struct JSFunctionData {
        std::function<JSON::Value(JSON::Array&)> func;
        QuickJSInterpreter& q;
    };
    std::list<JSFunctionData> functionDataList;

    static JSValue functionEntrypoint(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic, JSValue* data) {
        JSHandle fhnd{ctx, *data};
        auto& [func, q] = *reinterpret_cast<JSFunctionData*>(JS_VALUE_GET_PTR(*data));
        JSON::Array args;
        args.resize(argc);
        std::unordered_map<void*, JSON::Value*> map;

        for (int i = 0; i < argc; ++i) {
            auto& arg = argv[i];
            q.JSValueToValue(arg, args[i], map);
        }
        try {
            JSON::Value result = func(args);
            clearQueue();
            return valueToJSValue(ctx, result);
        } catch (const std::exception& e) {
            return JS_ThrowInternalError(ctx, "API function exception: %s", e.what());
        }
    }

    void addAPIFunction(const std::string& name, const std::function<JSON::Value(JSON::Array&)>& func) override {
        functionDataList.emplace_back(func, *this);
        JSHandle global{ctx.get(), JS_GetGlobalObject(ctx.get())};
        JSValue ptr = JS_MKPTR(0, (void*)&functionDataList.back());
        JSValue jsFunc = JS_NewCFunctionData(
            ctx.get(),
            functionEntrypoint,
            0,
            0,
            1,
            &ptr
            );
        JS_SetPropertyStr(ctx.get(), global, name.c_str(), jsFunc);
    }

    void reportError(const std::exception& e) override {
        js_std_dump_error(ctx.get());
    }

    struct VoidWrapper {
        JSHandle weak;
        QuickJSInterpreter& q;
        std::shared_ptr<void> native;
        std::type_index base{typeid(void)};

        VoidWrapper(QuickJSInterpreter& q, JSValue weak, std::shared_ptr<void> native, std::type_index& base) :
            weak{q.ctx.get(), weak},
            q{q},
            native{std::move(native)},
            base{base}
            {
            q.wrappers[this->native.get()] = this;
        }

        ~VoidWrapper() {
            if (auto it = q.wrappers.find(native.get()); it != q.wrappers.end() && it->second == this) {
                q.wrappers.erase(it);
            }
        }
    };

    class QJSClassDef : public Interpreter::ClassDef {
        JSClassID clsId{};
        QuickJSInterpreter& q;
        std::type_index base;
        std::type_index derived;
        std::string name;
        JSHandle proto;

        struct MListElement { std::function<JSON::Value(void* self, JSON::Array&)> method; QJSClassDef& def; };
        struct GListElement { std::function<JSON::Value(void* self)> method; QJSClassDef& def; };
        struct SListElement { std::function<void(void* self, JSON::Value&)> method; QJSClassDef& def; };

        std::list<MListElement> methods;
        std::list<GListElement> getters;
        std::list<SListElement> setters;

    public:
        QJSClassDef(QuickJSInterpreter& q, const std::string& name, const std::type_index& base, const std::type_index& derived, const ClassCtor& ctor) :
            Interpreter::ClassDef{ctor},
            q{q},
            base{base},
            derived{derived},
            name{name},
            proto{q.ctx.get(), JS_NewObject(q.ctx.get())} {

            JS_NewClassID(q.rt.get(), &clsId);

            if (!q.firstClassId)
                q.firstClassId = clsId;
            q.classIdCount = std::max<std::size_t>(q.classIdCount, clsId - q.firstClassId + 1);

            JSClassDef def{};
            def.class_name = name.c_str();
            def.finalizer = +[](JSRuntime *rt, JSValueConst val){
                JSClassID clsId;
                auto* p = static_cast<VoidWrapper*>(JS_GetAnyOpaque(val, &clsId));
                if (p) {
                    delete p;
                }
            };
            if (JS_NewClass(q.rt.get(), clsId, &def) < 0) {
                std::cerr << "Failed to create JS class " << name << std::endl;
                return;
            }

            auto ctx = q.ctx.get();
            JSValue ptr = JS_MKPTR(0, (void*) this);
            auto jsFunc = JS_NewCFunctionData(
                ctx,
                +[](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic, JSValue* data) -> JSValue {
                    QJSClassDef* self = reinterpret_cast<QJSClassDef*>(JS_VALUE_GET_PTR(*data));
                    JSON::Array args;
                    args.resize(argc);
                    std::unordered_map<void*, JSON::Value*> map;
                    for (int i = 0; i < argc; ++i) {
                        auto& arg = argv[i];
                        self->q.JSValueToValue(arg, args[i], map);
                    }
                    try {
                        auto native = self->ctor(args);
                        return self->jsCtor(native);
                    } catch (const std::exception& e) {
                        return JS_ThrowInternalError(ctx, "Constructor exception: %s", e.what());
                    }
                },
                0,
                0,
                1,
                &ptr
            );

            JS_SetConstructor(ctx, jsFunc, proto);
            JS_SetConstructorBit(ctx, jsFunc, 1);
            JS_SetClassProto(ctx, clsId, JS_DupValue(ctx, proto));

            JSHandle global{ctx, JS_GetGlobalObject(ctx)};
            JS_SetPropertyStr(ctx, global, name.c_str(), jsFunc);
        }

        void setPrototypeIfPossible() {
            if (base != typeid(void)) {
                if (auto it = q.classDefByType.find(base); it != q.classDefByType.end()) {
                    JS_SetPrototype(q.ctx.get(), proto, it->second->proto);
                }
            }
        }

        JSValue jsCtor(std::shared_ptr<void> instance) {
            // auto val = ctor(args);
            if (!instance) {
                return JS_UNDEFINED;
            }
            auto ctx = this->q.ctx.get();
            JSValue obj = JS_NewObjectClass(ctx, clsId);
            if (JS_IsException(obj)) {
                std::cerr << "Failed to create instance of " << name << std::endl;
                return obj;
            }
            if (!ctor) {
                std::cerr << "No constructor defined for " << name << std::endl;
                return obj;
            }
            auto wrapper = new VoidWrapper(q, makeWeakRef(ctx, obj), instance, base);
            // std::cout << "Created instance of " << name << " with type " << derived.name() << "(" << base.name() << ") for " << instance << std::endl;
            JS_SetOpaque(obj, wrapper);
            return obj;
        }

        void addMethod(const std::string& name, const std::function<JSON::Value(void* self, JSON::Array&)>& func) override {
            methods.emplace_back(func, *this);
            JSValue ptr = JS_MKPTR(0, (void*) &methods.back());
            JSValue methodFunc = JS_NewCFunctionData(
                q.ctx.get(),
                +[](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic, JSValue* data) -> JSValue {
                    auto& listElement = *reinterpret_cast<MListElement*>(JS_VALUE_GET_PTR(*data));
                    auto& method = listElement.method;
                    if (JS_IsUndefined(this_val)) {
                        std::cerr << "Method called without this object" << std::endl;
                        return JS_ThrowInternalError(ctx, "Method called without this object");
                    }
                    auto voidWrapper = JS_GetOpaque(this_val, listElement.def.clsId);
                    void* selfPtr = voidWrapper ? static_cast<VoidWrapper*>(voidWrapper)->native.get() : nullptr;
                    if (!selfPtr) {
                        return JS_ThrowInternalError(ctx, "Method called on an invalid object");
                    }
                    JSON::Array args;
                    args.resize(argc);
                    std::unordered_map<void*, JSON::Value*> map;
                    auto& q = listElement.def.q;
                    for (int i = 0; i < argc; ++i) {
                        auto& arg = argv[i];
                        q.JSValueToValue(arg, args[i], map);
                    }
                    try {
                        auto result = method(selfPtr, args);
                        clearQueue();
                        return valueToJSValue(ctx, result);
                    } catch (const std::exception& e) {
                        return JS_ThrowInternalError(ctx, "Method exception: %s", e.what());
                    }
                },
                0,
                0,
                1,
                &ptr
                );
            JS_SetPropertyStr(q.ctx.get(), proto, name.c_str(), methodFunc);
        }

        void addGetSet(const std::string& name, const std::function<JSON::Value(void* self)>& get, const std::function<void(void* self, JSON::Value&)>& set) override {
            JSValue getterFunc = JS_UNDEFINED;
            JSValue setterFunc = JS_UNDEFINED;

            if (get) {
                getters.emplace_back(get, *this);
                JSValue ptr = JS_MKPTR(0, (void*) &getters.back());
                getterFunc = JS_NewCFunctionData(
                    q.ctx.get(),
                    +[](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic, JSValue* data) -> JSValue {
                        auto& listElement = *reinterpret_cast<GListElement*>(JS_VALUE_GET_PTR(*data));
                        auto& getter = listElement.method;
                        auto voidWrapper = JS_GetOpaque(this_val, listElement.def.clsId);
                        void* selfPtr = voidWrapper ? static_cast<VoidWrapper*>(voidWrapper)->native.get() : nullptr;
                        if (!selfPtr) {
                            return JS_ThrowInternalError(ctx, "Getter called on an invalid object");
                        }
                        try {
                            auto result = getter(selfPtr);
                            return valueToJSValue(ctx, result);
                        } catch (const std::exception& e) {
                            return JS_ThrowInternalError(ctx, "Getter exception: %s", e.what());
                        }
                    },
                    0,
                    0,
                    1,
                    &ptr
                    );
            }

            if (set) {
                setters.emplace_back(set, *this);
                JSValue ptr = JS_MKPTR(0, (void*) &setters.back());
                setterFunc = JS_NewCFunctionData(
                    q.ctx.get(),
                    +[](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic, JSValue* data) -> JSValue {
                        auto& listElement = *reinterpret_cast<SListElement*>(JS_VALUE_GET_PTR(*data));
                        auto& setter = listElement.method;
                        auto voidWrapper = JS_GetOpaque(this_val, listElement.def.clsId);
                        void* selfPtr = voidWrapper ? static_cast<VoidWrapper*>(voidWrapper)->native.get() : nullptr;
                        if (!selfPtr) {
                            return JS_ThrowInternalError(ctx, "Setter called on an invalid object");
                        }
                        JSON::Value arg;
                        if (argc) {
                            auto& q = listElement.def.q;
                            std::unordered_map<void*, JSON::Value*> map;
                            q.JSValueToValue(argv[0], arg, map);
                        }
                        try {
                            setter(selfPtr, arg);
                            return JS_UNDEFINED;
                        } catch (const std::exception& e) {
                            return JS_ThrowInternalError(ctx, "Setter exception: %s", e.what());
                        }
                    },
                    1,
                    0,
                    1,
                    &ptr
                    );
            }

            JSAtom atom = JS_NewAtom(q.ctx.get(), name.c_str());
            JS_DefinePropertyGetSet(q.ctx.get(), proto, atom, getterFunc, setterFunc, (get ? JS_PROP_HAS_GET : 0) | (set ? JS_PROP_HAS_SET : 0) | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
            JS_FreeAtom(q.ctx.get(), atom);
        }
    };

    std::unordered_map<std::string, std::shared_ptr<QJSClassDef>> classDefs;
    std::unordered_map<std::type_index, QJSClassDef*> classDefByType;

    ClassDef& addClass(const std::string& name, std::type_index base, std::type_index derived, const ClassCtor& ctor) override {
        auto& def = classDefs[name];
        if (!def) {
            def = std::make_shared<QJSClassDef>(*this, name, base, derived, ctor);
            classDefByType[derived] = static_cast<QJSClassDef*>(def.get());
            for (auto& [_, d] : classDefs) {
                d->setPrototypeIfPossible();
            }
        }
        return *def;
    }
    
    static std::string errorToString(JSContext* ctx, JSValueConst err) {
        std::string errmsg{"JavaScript exception"};
        JSString msg {ctx, JS_ToCString(ctx, err)};
        if (msg.value)
            errmsg = msg.value;
        JSHandle stack {ctx, JS_GetPropertyStr(ctx, err, "stack")};
        if (!JS_IsException(stack) && !JS_IsUndefined(stack) && !JS_IsNull(stack)) {
            JSString stackStr {ctx, JS_ToCString(ctx, stack)};
            if (stackStr.value)
                errmsg += "\n" + std::string{stackStr.value};
        }
        return errmsg;
    }

    JSON::Value eval(const std::string& script, const std::string& path, EvalType type) override {
        int evalType = (type == EvalType::Module) ? JS_EVAL_TYPE_MODULE : JS_EVAL_TYPE_GLOBAL;
        JSHandle result {
            ctx.get(),
            JS_Eval(ctx.get(), script.c_str(), script.size(), path.c_str(), evalType)
        };

        // Synchronous error (e.g. compile failure).
        if (JS_IsException(result)) {
            JSHandle exc {ctx.get(), JS_GetException(ctx.get())};
            throw std::runtime_error{errorToString(ctx.get(), exc)};
        }

        if (type == EvalType::Module) {
            // JS_Eval returns the module's promise; a top-level throw
            // rejects it asynchronously. Attach a no-op catch so the
            // host's rejection tracker doesn't report it, then surface
            // the error via the promise state below.
            JSHandle catchFn {ctx.get(), JS_GetPropertyStr(ctx.get(), result.value, "catch")};
            if (!JS_IsException(catchFn) && JS_IsFunction(ctx.get(), catchFn)) {
                JSValue noop = JS_NewCFunction(ctx.get(),
                    +[](JSContext*, JSValueConst, int, JSValueConst*) { return JS_UNDEFINED; },
                    "onModuleSettled", 1);
                JSHandle ignored {ctx.get(), JS_Call(ctx.get(), catchFn, result.value, 1, &noop)};
                JS_FreeValue(ctx.get(), noop);
            }
        }

        js_std_loop(ctx.get());

        // Asynchronous error: a top-level throw in a module rejects its
        // promise.
        if (type == EvalType::Module &&
            JS_IsPromise(result.value) &&
            JS_PromiseState(ctx.get(), result.value) == JS_PROMISE_REJECTED) {
            JSHandle reason {ctx.get(), JS_PromiseResult(ctx.get(), result.value)};
            // The module's failure chain may have left other unhandled
            // rejections buffered (e.g. the module's internal promise);
            // they are the same error, so drop them without reporting.
            for (auto& [c, p, r] : unhandledRejections) {
                JS_FreeValue(c, p);
                JS_FreeValue(c, r);
            }
            unhandledRejections.clear();
            throw std::runtime_error{errorToString(ctx.get(), reason)};
        }

        drainUnhandledRejections();

        JSON::Value ret;
        std::unordered_map<void*, JSON::Value*> map;
        JSValueToValue(result.value, ret, map);
        clearQueue();
        return ret;
    }

    void addModuleSearchPath(const std::string& dir) override {
        moduleSearchPaths.push_back(dir);
    }

    void tick() override {
        if (js_std_loop(ctx.get()) < 0) {
            js_std_dump_error(ctx.get());
        }
        drainUnhandledRejections();
        clearQueue();
    }
};

static di::provide<Interpreter, QuickJSInterpreter> qjs{"js"};
