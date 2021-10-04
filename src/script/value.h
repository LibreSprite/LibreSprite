// LibreSprite Scripting Library
// Copyright (c) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <memory>
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
      OBJECT,
      BUFFER
    } type = Type::UNDEFINED;

    class Buffer {
    public:
      uint8_t* _data = nullptr;
      std::size_t _size = 0;
      std::shared_ptr<uint32_t> refCount;

      Buffer(uint8_t* _data, std::size_t size, bool own) : _data(_data), _size(size) {
        if (own)
          refCount = std::make_shared<uint32_t>(1);
      }

      Buffer(const Buffer& other) : _data(other._data),
                                    _size(other._size),
                                    refCount(other.refCount) {
        hold();
      }

      ~Buffer() {release();}

      bool canSteal() {
        return refCount && *refCount == 1;
      }

      template <typename Type = uint8_t>
      Type* steal() {
        if (!canSteal()) return nullptr;
        refCount.reset();
        return reinterpret_cast<Type*>(_data);
      }

      template <typename Type = uint8_t>
      Type* data() {
        return reinterpret_cast<Type*>(_data);
      }

      template <typename Type = uint8_t>
      Type* end() {
        return reinterpret_cast<Type*>(_data + _size);
      }

      uint8_t& operator [] (std::size_t pos) {
        return _data[pos];
      }

      bool empty() {
        return !_data || !_size;
      }

      std::size_t size() {
        return _size;
      }

      void hold() {
        if (refCount)
          ++*refCount;
      }

      void release() {
        if (!refCount)
          return;
        --*refCount;
        if (!*refCount)
          delete _data;
      }
    };

    union {
      int int_v;
      double double_v;
      std::string* string_v;
      Buffer* buffer_v;
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
      } else if (type == Type::BUFFER) {
        data.buffer_v = new Buffer {*other.data.buffer_v};
        data.buffer_v->hold();
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
      if (type == Type::BUFFER)
        delete data.buffer_v;
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
      case Type::BUFFER:
        return !data.buffer_v->empty();
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
      if (type == Type::BUFFER) return std::string(data.buffer_v->data(), data.buffer_v->end());
      return type == Type::STRING ? *data.string_v : std::string{};
    }

    operator std::string () const {
      return str();
    }

    operator const char* () const {
      if (type == Type::STRING)
        return data.string_v->c_str();
      if (type == Type::BUFFER)
        return reinterpret_cast<char*>(data.buffer_v->data());
      return "";
    }

// BUFFER
    Value(void* bytes, std::size_t size, bool own) {
      type = Type::BUFFER;
      data.buffer_v = new Buffer {
        static_cast<uint8_t*>(bytes),
        size,
        own
      };
    }

    operator Buffer& () const {
      static Buffer empty{nullptr, 0, false};
      if (type == Type::BUFFER)
        return *data.buffer_v;
      return empty;
    }

    operator uint8_t* () const {
      if (type == Type::BUFFER)
        return reinterpret_cast<uint8_t*>(data.buffer_v->data());
      return nullptr;
    }

    std::size_t size() const {
      if (type == Type::STRING) return data.string_v->size();
      if (type == Type::BUFFER) return data.buffer_v->size();
      return 0;
    }

    Buffer& buffer() const {
      static Buffer empty{nullptr, 0, false};
      return type == Type::BUFFER ? *data.buffer_v : empty;
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
