// LibreSprite Scripting Library
// Copyright (c) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <string>

namespace script {
  class ScriptObject;

  class Value {
  public:
    enum class Type {
      UNDEFINED,
      INT,
      DOUBLE,
      STRING,
      OBJECT
    } type = Type::UNDEFINED;

    union {
      int int_v;
      double double_v;
      std::string* string_v;
      ScriptObject* object_v;
    } data;

    Value() = default;
    Value(const Value& other) {*this = other;}
    Value(Value&& other) {*this = std::move(other);}
    ~Value() {makeUndefined();}

    Value& operator = (const Value& other) {
      makeUndefined();
      type = other.type;
      if (type == Type::STRING) {
        data.string_v = new std::string(*other.data.string_v);
      } else {
        data = other.data;
      }
      return *this;
    }

    Value& operator = (Value&& other) {
      makeUndefined();
      type = other.type;
      data = other.data;
      other.type = Type::UNDEFINED;
      return *this;
    }

// UNDEFINED

    void makeUndefined() {
      if (type == Type::STRING)
        delete data.string_v;
      type = Type::UNDEFINED;
    }

// BOOL

    operator bool () const {
      switch (type) {
      case Type::UNDEFINED:
        return false;
      case Type::INT:
        return data.int_v != 0;
      case Type::DOUBLE:
        return data.double_v != 0;
      case Type::STRING:
        return !data.string_v->empty();
      case Type::OBJECT:
        return data.object_v != 0;
      default:
        return false;
      }
    }

// INT
    Value(int i) { *this = i; }
    Value(unsigned int i) { *this = static_cast<int>(i); }

    Value& operator = (int i) {
      makeUndefined();
      type = Type::INT;
      data.int_v = i;
      return *this;
    }

    operator int () const {
      if (type == Type::DOUBLE) return data.double_v;
      if (type == Type::STRING) return atoi(data.string_v->c_str());
      return type == Type::INT ? data.int_v : int{};
    }

    operator unsigned int () const {return static_cast<int>(*this);}
    operator short () const {return static_cast<int>(*this);}
    operator unsigned short () const {return static_cast<int>(*this);}
    operator char () const {return static_cast<int>(*this);}
    operator unsigned char () const {return static_cast<int>(*this);}

// DOUBLE
    Value(double i) { *this = i; }

    Value& operator = (double i) {
      makeUndefined();
      type = Type::DOUBLE;
      data.double_v = i;
      return *this;
    }

    operator double () const {
      if (type == Type::INT) return data.int_v;
      if (type == Type::STRING) return atof(data.string_v->c_str());
      return type == Type::DOUBLE ? data.double_v : double{};
    }

// STRING
    Value(std::string&& i) { *this = std::move(i); }
    Value(const std::string& i) { *this = i; }

    Value& operator = (const std::string& i) {
      if (type == Type::STRING) {
        *data.string_v = i;
      } else {
        makeUndefined();
        type = Type::STRING;
        data.string_v = new std::string(i);
      }
      return *this;
    }


    Value& operator = (std::string&& i) {
      if (type == Type::STRING) {
        *data.string_v = std::move(i);
      } else {
        makeUndefined();
        type = Type::STRING;
        data.string_v = new std::string(std::move(i));
      }
      return *this;
    }

    std::string str() const {
      if (type == Type::INT) return std::to_string(data.int_v);
      if (type == Type::DOUBLE) return std::to_string(data.double_v);
      return type == Type::STRING ? *data.string_v : std::string{};
    }

    operator std::string () const {
      return str();
    }

    operator const char* () const {
      if (type == Type::STRING)
        return data.string_v->c_str();
      return "";
    }

// OBJECT
    Value(ScriptObject* object) { *this = object; }

    Value& operator = (ScriptObject* object) {
      makeUndefined();
      type = Type::OBJECT;
      data.object_v = object;
      return *this;
    }

    operator ScriptObject* () const {
      if (type == Type::OBJECT) return data.object_v;
      return nullptr;
    }

  };
}
