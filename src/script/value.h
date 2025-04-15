// LibreSprite Scripting Library
// Copyright (c) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <string>
#include <iostream>

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
      BUFFER,
      MAP
    } type = Type::UNDEFINED;

    template<typename Data_t>
    struct RefCount {
      Data_t* _data = nullptr;
      std::shared_ptr<uint32_t> refCount;
      using data_t = Data_t;

      RefCount(Data_t* data, bool own) : _data{data} {
        if (own)
          refCount = std::make_shared<uint32_t>(1);
      }

      RefCount(const RefCount& other) : _data{other._data}, refCount{other.refCount} {
	hold();
      }

      virtual ~RefCount() {release();}

      bool canSteal() {
        return refCount && *refCount == 1;
      }

      template <typename Type = Data_t>
      Type* steal() {
        if (!canSteal()) return nullptr;
        refCount.reset();
        return reinterpret_cast<Type*>(_data);
      }

      template <typename Type = Data_t>
      Type* data() {
        return reinterpret_cast<Type*>(_data);
      }

      void hold() {
        if (refCount)
          ++*refCount;
      }

      void release() {
        if (!refCount)
          return;
        --*refCount;
	if (!*refCount) {
	    delete[] _data;
	    _data = nullptr;
	}
      }
    };

    class Buffer : public RefCount<uint8_t> {
    public:
      std::size_t _size = 0;

      Buffer(uint8_t* _data, std::size_t size, bool own) : RefCount<uint8_t>{_data, own}, _size{size} {}

      Buffer(const Buffer& other) : RefCount(other), _size(other._size) {}

      template <typename Type = uint8_t>
      Type* end() {
        return reinterpret_cast<Type*>(_data + _size);
      }

      uint8_t& operator [] (std::size_t pos) {
        return _data[pos];
      }

      bool empty() const {
        return !_data || !_size;
      }

      std::size_t size() const {
        return _size;
      }
    };

    class Map : public RefCount<std::unordered_map<std::string, Value>> {
    public:

      Map(data_t* _data, bool own) : RefCount{_data, own} {}

      Map(const Map& other) : RefCount(other) {}

      bool empty() const {
        return !_data;
      }
    };

    union {
      int int_v;
      double double_v;
      std::string* string_v;
      Buffer* buffer_v;
      ScriptObject* object_v;
      Map* map_v;
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
      } else if (type == Type::MAP) {
	data.map_v = new Map {*other.data.map_v};
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
      else if (type == Type::BUFFER)
        delete data.buffer_v;
      else if (type == Type::MAP)
	delete data.map_v;
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
      case Type::MAP:
	return !data.map_v->empty();
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
    Value(const char* s) { *this = s; }

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

    Value& operator = (const char* i) {
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
      if (type == Type::MAP) return "[Object object]";
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

// MAP
    Value(Map::data_t* ptr, bool own) {
      type = Type::MAP;
      data.map_v = new Map {ptr, own};
    }

    operator Map::data_t* () const {
      return type == Type::MAP ? data.map_v->data() : nullptr;
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
