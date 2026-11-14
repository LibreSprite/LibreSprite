#pragma once

#include "StringUtils.hpp"
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <functional>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <string>
#include <variant>
#include <memory>
#include <vector>
#include <any>

class JSON {
public:
    enum EncodingFlags : uint32_t {
        DEFAULT = 0,
        NO_UNDEFINED = 1 << 0
    };

private:
    struct Location {
        using Codepoint = unsigned long;
        std::function<bool(unsigned char&)> read;
        std::vector<Codepoint> queue;
        std::string fileName;
        std::size_t line{};
        std::size_t column{};
        bool eof;

        std::string toString() {
            return (fileName.empty() ? "(string)" : fileName) + ":" + std::to_string(line + 1) + ":" + std::to_string(column + 1);
        }

        template <typename Iterator>
        Location(Iterator begin, Iterator end, const std::string& fileName = "") : fileName{fileName}, eof{begin == end} {
            read = [this, cursor = begin, end](unsigned char& ch) mutable {
                if (cursor == end) {
                    eof = true;
                    return false;
                }
                ch = *cursor++;
                return true;
            };
        }

        operator bool () const {
            return !eof;
        }

        Codepoint pop() {
            if (!queue.empty()) {
                auto cp = queue.back();
                queue.pop_back();
                return cp;
            }
            Codepoint codepoint{};
            unsigned char c;
            while (read(c)) {
                if (c <= 0x7F) {
                    codepoint = c;
                    break;
                }
                if ((c & 0xE0) == 0xC0) {
                    unsigned char c2;
                    if (!read(c2) || (c2 & 0xC0) != 0x80) {
                        // Invalid byte
                        codepoint = 0;
                        break;
                    }
                    codepoint = ((c & 0x1F) << 6) | (c2 & 0x3F);
                    break;
                }
                if ((c & 0xF0) == 0xE0) {
                    unsigned char c2, c3;
                    if (!read(c2) || (c2 & 0xC0) != 0x80 ||
                        !read(c3) || (c3 & 0xC0) != 0x80) {
                        // Invalid byte
                        codepoint = 0;
                        break;
                    }
                    codepoint = ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
                    break;
                }
                if ((c & 0xF8) == 0xF0) {
                    unsigned char c2, c3, c4;
                    if (!read(c2) || (c2 & 0xC0) != 0x80 ||
                        !read(c3) || (c3 & 0xC0) != 0x80 ||
                        !read(c4) || (c4 & 0xC0) != 0x80) {
                        // Invalid byte
                        codepoint = 0;
                        break;
                    }
                    codepoint = ((c & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
                    break;
                }

                // Invalid byte
                codepoint = 0;
                break;
            }
            if (codepoint == '\n') {
                line++;
                column = 0;
            }
            return codepoint;
        }

        Codepoint peek() {
            auto cp = pop();
            queue.push_back(cp);
            return cp;
        }

        bool pop(Codepoint c) {
            auto cp = pop();
            if (cp == c)
                return true;
            queue.push_back(cp);
            return false;
        }

        [[noreturn]]
        void throwExpect(Codepoint c) {
            throw std::runtime_error{toString() + ": Expected \"" + static_cast<char>(c) + "\" but got \"" + static_cast<char>(peek()) + "\"."};
        }

        void expect(Codepoint c) {
            if (pop(c))
                return;
            throwExpect(c);
        }
    };

public:
    class Value;
    struct Object;
    using ObjectRef = std::shared_ptr<Object>;

    enum class Special {
        Undefined,
        Null
    };

    using Array = std::vector<Value>;
    using ArrayRef = std::shared_ptr<Array>;
    using ByteArray = std::shared_ptr<std::vector<uint8_t>>;
    using Function = std::function<Value(const Array&)>;

    struct FunctionRef {
        std::shared_ptr<Function> func;
        std::any memo;

        FunctionRef() = default;

        FunctionRef(const std::shared_ptr<Function>& func) : func{func} {}

        template<typename C, typename T>
        FunctionRef(C&& func, T&& memo) : func{std::make_shared<Function>(std::forward<C>(func))},
                                          memo{std::forward<T>(memo)} {}

        operator Function* () {return func.get();}

    };
    using NativeRef = std::pair<std::shared_ptr<void>, std::type_index>;

    class Value {
    public:
        struct String {
            std::string str;
            bool bin{};
            String(std::string&& str, bool bin = false) : str{std::move(str)}, bin{bin} {}
            String(const std::string& str, bool bin = false) : str{str}, bin{bin} {}
            String(const char* str, bool bin = false) : str{str}, bin{bin} {}
            String(const String& o) : str{o.str}, bin{o.bin} {}
            String(String&& o) : str{std::move(o.str)}, bin{o.bin} {}
            String(std::string_view str, bool bin = false) : str{str}, bin{bin} {}
            String& operator = (String&&) = default;
            String& operator = (const String&) = default;
        };

        std::variant<Special,
                     double,
                     String,
                     bool,
                     ObjectRef,
                     ArrayRef,
                     FunctionRef,
                     NativeRef,
                     ByteArray> value;

        Value() = default;
        Value(Special v) : value{v} {}
        Value(double v) : value{v} {}
        Value(String v) : value{v} {}
        Value(const char* v) : value{String{v}} {}
        Value(const std::string& v) : value{String{v}} {}
        Value(bool v) : value{v} {}
        Value(ObjectRef v) : value{v} {}
        Value(ArrayRef v) : value{v} {}
        Value(FunctionRef v) : value{v} {}
        Value(NativeRef v) : value{v} {}
        Value(ByteArray v) : value{v} {}

        bool isNull() {
            if (auto v = std::get_if<Special>(&value))
                return *v == Special::Null;
            return false;
        }

        bool isUndefined() {
            if (auto v = std::get_if<Special>(&value))
                return *v == Special::Undefined;
            return false;
        }

        bool isBoolean() {
            return std::get_if<bool>(&value);
        }

        bool isNumber() {
            return std::get_if<double>(&value);
        }

        bool isString() {
            return std::get_if<String>(&value);
        }

        bool isBinString() {
            auto str = std::get_if<String>(&value);
            if (!str)
                return false;
            return str->bin;
        }

        bool isObject() {
            return std::get_if<ObjectRef>(&value);
        }

        bool isArray() {
            return std::get_if<ArrayRef>(&value);
        }

        bool isByteArray() {
            return std::get_if<ByteArray>(&value);
        }

        bool isFunction() {
            return std::get_if<FunctionRef>(&value);
        }

        bool isNative() {
            return std::get_if<NativeRef>(&value);
        }

        Special& special() {
            auto ptr = std::get_if<Special>(&value);
            if (ptr) return *ptr;
            value = Special{};
            return std::get<Special>(value);
        }

        double& number(double def = 0.0) {
            auto ptr = std::get_if<double>(&value);
            if (ptr) return *ptr;
            value = def;
            return std::get<double>(value);
        }

        std::string& string(std::string&& def = "") {
            auto ptr = std::get_if<String>(&value);
            if (ptr) return ptr->str;
            value = String{std::move(def), false};
            return std::get<String>(value).str;
        }

        bool& boolean(bool def = false) {
            auto ptr = std::get_if<bool>(&value);
            if (ptr) return *ptr;
            value = def;
            return std::get<bool>(value);
        }

        Function& function() {
            auto ptr = std::get_if<FunctionRef>(&value);
            if (ptr) return **ptr;
            auto obj = std::make_shared<Function>();
            value = obj;
            return *obj;
        }

        std::any& functionMemo() {
            auto ptr = std::get_if<FunctionRef>(&value);
            if (ptr)
                return ptr->memo;
            auto obj = std::make_shared<Function>();
            value = obj;
            return std::get_if<FunctionRef>(&value)->memo;
        }

        NativeRef& native() {
            auto ptr = std::get_if<NativeRef>(&value);
            if (ptr) return *ptr;
            value = NativeRef{{}, typeid(void)};
            return std::get<NativeRef>(value);
        }

        Object& object() {
            auto ptr = std::get_if<ObjectRef>(&value);
            if (ptr) return **ptr;
            auto obj = std::make_shared<Object>();
            value = obj;
            return *obj;
        }

        Array& array() {
            auto ptr = std::get_if<ArrayRef>(&value);
            if (ptr) return **ptr;
            auto arr = std::make_shared<Array>();
            value = arr;
            return *arr;
        }

        std::vector<uint8_t>& byteArray() {
            auto ptr = std::get_if<ByteArray>(&value);
            if (ptr) return **ptr;
            auto arr = std::make_shared<std::vector<uint8_t>>();
            value = arr;
            return *arr;
        }

        explicit operator Special () const {
            auto ptr = std::get_if<Special>(&value);
            if (ptr) return *ptr;
            return Special::Undefined;
        }

        explicit operator double () const {
            auto ptr = std::get_if<double>(&value);
            if (ptr) return *ptr;
            return 0.0;
        }

        explicit operator int64_t () const {
            auto ptr = std::get_if<double>(&value);
            if (ptr) return static_cast<int64_t>(*ptr);
            return 0;
        }

        explicit operator float () const {
            auto ptr = std::get_if<double>(&value);
            if (ptr) return static_cast<float>(*ptr);
            return 0.0f;
        }

        explicit operator int () const {
            auto ptr = std::get_if<double>(&value);
            if (ptr) return static_cast<int>(*ptr);
            return 0;
        }

        operator const std::string& () const {
            auto ptr = std::get_if<String>(&value);
            if (ptr) return ptr->str;
            static const std::string emptyString{};
            return emptyString;
        }

        explicit operator bool () const {
            auto ptr = std::get_if<bool>(&value);
            if (ptr) return *ptr;
            return false;
        }

        explicit operator FunctionRef() {
            auto ptr = std::get_if<FunctionRef>(&value);
            if (ptr) return *ptr;
            return {};
        }

        explicit operator ObjectRef() {
            auto ptr = std::get_if<ObjectRef>(&value);
            if (ptr) return *ptr;
            return {};
        }

        explicit operator ArrayRef() {
            auto ptr = std::get_if<ArrayRef>(&value);
            if (ptr) return *ptr;
            return {};
        }

        explicit operator ByteArray() {
            auto ptr = std::get_if<ByteArray>(&value);
            if (ptr) return *ptr;
            return {};
        }

        template<typename T>
        explicit operator std::shared_ptr<T> () const {
            auto ptr = std::get_if<NativeRef>(&value);
            if (!ptr) return {};
            if (typeid(T) != ptr->second) {
                std::cout << "Type mismatch in native value access: requested " << typeid(T).name() << " but actual type is " << ptr->second.name() << std::endl;
                throw std::runtime_error{"Type mismatch"};
                return {};
            }
            return std::static_pointer_cast<T>(ptr->first);
        }

        template<typename T>
        std::shared_ptr<T> asNative() const {
            auto ptr = std::get_if<NativeRef>(&value);
            if (!ptr) return {};
            if (typeid(T) != ptr->second) {
                return {};
            }
            return std::static_pointer_cast<T>(ptr->first);
        }

        Value& operator = (const Special& nval) {value = nval; return *this;}
        Value& operator = (double nval) {value = nval; return *this;}
        Value& operator = (const std::string& nval) {value = String{nval}; return *this;}
        Value& operator = (const char* nval) {value = String{nval}; return *this;}
        Value& operator = (bool nval) {value = nval; return *this;}
        Value& operator = (ObjectRef nval) {value = nval; return *this;}
        Value& operator = (ArrayRef nval) {value = nval; return *this;}
        Value& operator = (FunctionRef nval) {value = nval; return *this;}
        Value& operator = (const NativeRef& nval) {value = nval; return *this;}
        Value& operator = (ByteArray nval) {value = nval; return *this;}

        template<typename T>
        Value& operator = (std::shared_ptr<T> ptr) {
            value = NativeRef{ptr, typeid(T)};
            return *this;
        }

        Value operator() (Array& args) {
            auto ptr = std::get_if<FunctionRef>(&value);
            if (!ptr || !*ptr)
                return {Special::Undefined};
            return (**ptr)(args);
        }

        Value& push_back(const Value& value) {
            auto& a = array();
            a.push_back(value);
            return a.back();
        }

        Value& operator [] (size_t s) {
            if (auto arr = std::get_if<ArrayRef>(&value)) {
                auto size = (*arr)->size();
                if (s >= size)
                    (*arr)->resize(s + 1);
                return (**arr)[s];
            }
            if (auto obj = std::get_if<ObjectRef>(&value)) {
                auto strkey = std::to_string(s);
                return (**obj)[strkey];
            }
            auto arr = std::make_shared<Array>();
            value = arr;
            arr->resize(s + 1);
            return (*arr)[s];
        }

        Value& operator [] (const std::string& strkey) {
            if (auto arr = std::get_if<ArrayRef>(&value)) {
                auto s = std::stoi(strkey);
                auto size = (*arr)->size();
                while (s >= static_cast<int>(size++))
                    (*arr)->emplace_back(Special::Undefined);
                return (**arr)[s];
            }
            if (auto obj = std::get_if<ObjectRef>(&value)) {
                return (**obj)[strkey];
            }
            auto obj = std::make_shared<Object>();
            value = obj;
            return (*obj)[strkey];
        }

        std::string type() {
            if (isString())
                return "String";
            if (isNumber())
                return "Number";
            if (isBoolean())
                return "Boolean";
            if (isNull())
                return "Null";
            if (isUndefined())
                return "Undefined";
            if (isArray())
                return "Array";
            if (isObject())
                return "Object";
            if (isFunction())
                return "Function";
            if (isNative())
                return "NativeRef";
            if (isByteArray())
                return "ByteArray";
            return "UNKNOWN:" + std::to_string(value.index());
        }

        std::string toString() {
            if (isString())
                return string();
            if (isNumber())
                return std::to_string(number());
            if (isBoolean())
                return boolean() ? "true" : "false";
            if (isNull())
                return "null";
            if (isUndefined())
                return "undefined";
            if (isArray())
                return "[Array]";
            if (isObject())
                return "[Object]";
            if (isFunction())
                return "[Function]";
            if (isNative())
                return "[Native]";
            return "ERROR:" + std::to_string(value.index());
        }

        void toJSON(std::ostream& s, int depth = 0, int inc = 0, uint32_t encoding = DEFAULT);

        std::string toJSON(int inc = 0, uint32_t encoding = DEFAULT) {
            std::ostringstream s;
            toJSON(s, 0, inc, encoding);
            return s.str();
        }
    };

    using ObjectData = std::unordered_map<std::string, Value>;
    struct Object {
        ObjectData map;
        std::any memo;

        Object() = default;
        Object(Object&&) = default;
        Object(ObjectData&& init) : map{std::move(init)} {}
        Object(const ObjectData& init) : map{init} {}

        Value& operator[] (const std::string& key) {
            return map[key];
        }

        Object& operator= (ObjectData&& init) {
            map = std::move(init);
            return *this;
        }

        Object& operator= (Object&& init) {
            map = std::move(init.map);
            memo = std::move(init.memo);
            return *this;
        }

        auto begin() {return map.begin();}
        auto end() {return map.end();}
        auto find(const std::string& val) {return map.find(val);}
        auto count(const std::string& val) {return map.count(val);}
        auto contains(const std::string& val) {return map.contains(val);}
        auto size() const {return map.size();}
        auto empty() const {return map.empty();}
        auto insert(auto begin, auto end) {return map.insert(begin, end);}
    };

    static Value makeObject(Object&& obj) {return Value{std::make_shared<Object>(std::move(obj))};}

    static Value makeObject(const ObjectData& obj) {return Value{std::make_shared<Object>(obj)};}

    static Value makeObject(ObjectData&& obj) {return Value{std::make_shared<Object>(std::move(obj))};}

    template<typename T>
    static Value makeNative(std::shared_ptr<T> ptr) {
        if (!ptr)
            return Value{Special::Null};
        return Value{NativeRef{ptr, typeid(T)}};
    }

    static Value parse(const std::string& str) {
        JSON json;
        Location loc{str.begin(), str.end()};
        return json.parseValue(loc);
    }

    static Value parse(std::istream&& stream, const std::string& fileName = "") {
        JSON json;
        Location loc{
            std::istreambuf_iterator<char>(stream),
            std::istreambuf_iterator<char>(),
            fileName
        };
        return json.parseValue(loc);
    }

    template<typename Iterator>
    static Value parse(Iterator begin, Iterator end) {
        JSON json;
        Location loc{begin, end};
        return json.parseValue(loc);
    }

private:
    JSON() = default;

    bool isFirstNumericChar(char ch) {
        return ch == '-' || isNumericChar(ch);
    }

    bool isNumericChar(char ch) {
        return (ch >= '0' && ch <= '9') || ch == '.';
    }

    Value parseValue(Location& location) {
        if (!location)
            return {Special::Undefined};
        parseWhitespace(location);
        auto next = location.peek();
        if (next == '{')
            return parseObject(location);
        if (next == '[')
            return parseArray(location);
        if (next == '"')
            return {Value::String{parseString(location)}};
        if (isFirstNumericChar(next))
            return parseNumber(location);
        if (next == 't')
            return parseLiteral(location, "true", {true});
        if (next == 'f')
            return parseLiteral(location, "false", {false});
        if (next == 'n')
            return parseLiteral(location, "null", {Special::Null});
        if (next == 'u')
            return parseLiteral(location, "undefined", {Special::Undefined});
        error(std::string{"Unexpected "} + static_cast<char>(location.peek()), location);
    }

    Value parseObject(Location& location) {
        auto obj = std::make_shared<Object>();
        location.expect('{');
        parseWhitespace(location);
        if (location.pop('}'))
            return {obj};
        while (location.peek() == '"') {
            auto key = parseString(location);
            parseWhitespace(location);
            location.expect(':');
            auto value = parseValue(location);
            parseWhitespace(location);
            (*obj)[key] = value;
            if (location.pop('}'))
                return {obj};
            location.expect(',');
            parseWhitespace(location);
        }
        location.throwExpect('"');
    }

    Value parseArray(Location& location) {
        auto arr = std::make_shared<std::vector<Value>>();
        location.expect('[');
        parseWhitespace(location);
        if (location.pop(']')) {
            return {arr};
        }
        while (location) {
            arr->push_back(parseValue(location));
            parseWhitespace(location);
            if (location.pop(']')) {
                return {arr};
            }
            location.expect(',');
            parseWhitespace(location);
        }
        location.throwExpect(']');
    }

    std::string parseString(Location& location) {
        std::string str;
        location.expect('"');
        bool escape = false;
        while (location) {
            auto ch = location.pop();
            if (escape) {
                switch (ch) {
                case 'n': str += '\n'; break;
                case 'r': str += '\r'; break;
                case 't': str += '\t'; break;
                case '"': str +=  '"'; break;
                case '\\': str += '\\'; break;
                case '\n': break;
                default: str += '\\'; str += ch; break;
                }
                escape = false;
                continue;
            }
            if (ch == '\\') {
                escape = true;
                continue;
            }
            if (ch == '"') {
                return str;
            }
            str += su::toUTF8(ch);
        }
        location.throwExpect('"');
    }

    Value parseNumber(Location& location) {
        std::string num;
        if (location.pop('-'))
            num += '-';
        while (location) {
            auto ch = location.peek();
            if (!isNumericChar(ch))
                break;
            num += ch;
            location.pop();
        }
        return {std::stod(num)};
    }

    Value parseLiteral(Location& location, const std::string& ref, Value value) {
        auto it = ref.begin();
        while (location) {
            if (it != ref.end()) {
                location.expect(*it++);
            } else {
                break;
            }
        }
        if (it != ref.end())
            location.throwExpect(*it);
        return value;
    }

    void parseWhitespace(Location& location) {
        while (location) {
            if (location.peek() <= ' ') {
                location.pop();
                continue;
            }
            if (location.pop('/')) {
                if (location.pop('*')) {
                    bool star = false;
                    while (location) {
                        if (star) {
                            if (location.pop('/'))
                                break;
                        }
                        star = location.pop() == '*';
                    }
                } else if (location.pop('/')) {
                    while (location && location.pop() != '\n');
                } else {
                    error(std::string{"Unexpected "} + static_cast<char>(location.peek()), location);
                }
                continue;
            }
            break;
        }
    }

    [[noreturn]] void error(const std::string& str, Location& location) {
        throw std::runtime_error{location.toString() + ": " + str};
    }
};
