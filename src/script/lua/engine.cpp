// LibreSprite Scripting Library
// Copyright (C) 2021  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if SCRIPT_ENGINE_LUA

#include "base/convert_to.h"
#include "base/exception.h"
#include "base/fs.h"
#include "base/memory.h"
#include "script/engine.h"
#include "script/engine_delegate.h"

#include <cstring>
#include <stdexcept>
#include <map>
#include <iostream>
#include <string>
#include <unordered_map>

extern "C" {

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

}

struct NumArray {
  std::size_t size;
  int32_t values[];
};

static NumArray* createArray(lua_State* L, std::size_t size) {
  size_t nbytes = sizeof(NumArray) + size * sizeof(int32_t);
  NumArray *array = (NumArray *)lua_newuserdata(L, nbytes);
  luaL_getmetatable(L, "LibreSprite.array");
  lua_setmetatable(L, -2);
  array->size = size;
  return array;
}

static NumArray* checkarray(lua_State *L, bool opt = false) {
  void *ud = luaL_checkudata(L, 1, "LibreSprite.array");
  luaL_argcheck(L, ud != NULL || opt, 1, "`array' expected");
  return (NumArray *)ud;
}

static int32_t& getelem(lua_State *L) {
  static std::string errMessage;
  NumArray *a = checkarray(L);
  int index = luaL_checkinteger(L, 2);
  if (index < 1 || index > static_cast<int>(a->size)) {
    errMessage = "Index " + std::to_string(index) + " is out of range [1, " + std::to_string(a->size) + "]";
  }
  luaL_argcheck(L, 1 <= index && index <= static_cast<int>(a->size), 2, errMessage.c_str());
  return a->values[index - 1];
}

static const struct luaL_Reg arraylib [] = {
  {"new",  +[](lua_State *L){createArray(L, luaL_checkinteger(L, 1)); return 1;}},
  {"size", +[](lua_State *L){lua_pushnumber(L, checkarray(L)->size); return 1;}},
  {"set",  +[](lua_State *L){getelem(L) = luaL_checknumber(L, 3); return 0;}},
  {"get",  +[](lua_State *L){lua_pushnumber(L, getelem(L)); return 1;}},
  {nullptr, nullptr}
};

int luaopen_array (lua_State *L) {
  luaL_newmetatable(L, "LibreSprite.array");
  lua_newtable(L);
  luaL_setfuncs(L, arraylib, 0);
  lua_setglobal(L, "array");
  lua_getglobal(L, "array");

  lua_pushstring(L, "__index");
  lua_pushstring(L, "get");
  lua_gettable(L, 2); // get array.get
  lua_settable(L, 1); // metatable.__index = array.get

  lua_pushstring(L, "__newindex");
  lua_pushstring(L, "set");
  lua_gettable(L, 2); // get array.set
  lua_settable(L, 1); // metatable.__newindex = array.set

  return 0;
}

using namespace script;

class LuaEngine : public Engine {
public:
  inject<EngineDelegate> m_delegate;
  lua_State* L = nullptr;

  LuaEngine() {
    InternalScriptObject::setDefault("LuaScriptObject");
    L = luaL_newstate();
    if (L) {
      luaL_openlibs(L);
      luaopen_array(L);
      initGlobals();
    }
  }

  ~LuaEngine() {
    if (L)
      lua_close(L);
  }

  bool raiseEvent(const std::string& event) override {
    return eval("if onEvent~=nil then onEvent(\"" + event + "\") end");
  }

  bool eval(const std::string& code) override {
    bool success = true;
    try {

      if (luaL_loadstring(L, code.c_str()) == LUA_OK) {
        if (lua_pcall(L, 0, 0, 0) == LUA_OK) {
          lua_pop(L, lua_gettop(L));
        } else throw std::runtime_error(lua_tostring(L, -1));
      } else throw std::runtime_error(lua_tostring(L, -1));

    } catch (const std::exception& ex) {
      std::string err = "Error: ";
      err += ex.what();
      m_delegate->onConsolePrint(err.c_str());
      success = false;
    }
    execAfterEval(success);
    return success;
  }
};

static Engine::Regular<LuaEngine> registration("lua", {"lua"});

class LuaScriptObject : public InternalScriptObject {
public:

  static Value getValue(lua_State* L, int index) {
    auto type = lua_type(L, index);
    if (type == LUA_TNIL) return {};
    if (type == LUA_TNUMBER) return lua_tonumber(L, index);
    if (type == LUA_TBOOLEAN) return lua_toboolean(L, index);
    if (type == LUA_TSTRING) {
        size_t len;
        const char* str = lua_tolstring(L, index, &len);
        return {(void*)str, len, false};
    }
    if (type == LUA_TUSERDATA) {
      if (auto array = checkarray(L, true)) {
        return {array->values, array->size, false};
      }
    }
    printf("Unknown value type: %d\n", type);
    // LUA_TTABLE, LUA_TFUNCTION, LUA_TTHREAD, and LUA_TLIGHTUSERDATA
    return {};
  }

  static int returnValue(lua_State* L, const Value& value) {
    switch (value.type) {
    case Value::Type::UNDEFINED:
      return 0;

    case Value::Type::INT:
      lua_pushinteger(L, (int) value);
      return 1;

    case Value::Type::DOUBLE:
      lua_pushnumber(L, value);
      return 1;

    case Value::Type::STRING:
      lua_pushlstring(L, value, value.size());
      return 1;

    case Value::Type::OBJECT:
      if (auto object = static_cast<ScriptObject*>(value)) {
        static_cast<LuaScriptObject*>(object->getInternalScriptObject())->makeLocal(L);
        return 1;
      }
      return 0;

    case Value::Type::BUFFER:
      std::memcpy(createArray(L, value.size())->values, static_cast<uint8_t*>(value), value.size());
      return 1;
    }
    return 0;
  }

  static int callFunc(lua_State* L) {
    int n = lua_gettop(L);    /* number of arguments */
    auto funcptr = reinterpret_cast<script::Function*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!funcptr) return 0;
    auto& func = *funcptr;

    for (int i = 1; i <= n; i++) {
      func.arguments.push_back(getValue(L, i));
    }
    func();

    return returnValue(L, func.result);
  }

  void pushFunctions(lua_State* L) {
    for (auto& entry : functions) {
      script::Function* ptr = &entry.second;
      lua_pushlightuserdata(L, ptr);
      lua_pushcclosure(L, callFunc, 1);
      lua_setfield(L, -2, entry.first.c_str());
    }
  }

  static int getset(lua_State* L) {
    int n = lua_gettop(L);    /* number of arguments */
    auto prop = reinterpret_cast<script::ObjectProperty*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!prop) return 0;

    auto& func = n ? prop->setter : prop->getter;
    for (int i = 1; i <= n; i++) {
      func.arguments.push_back(getValue(L, i));
    }
    func();
    return returnValue(L, func.result);
  }

  void pushProperties(lua_State* L) {
    for (auto& entry : properties) {
      lua_pushlightuserdata(L, &entry.second);
      lua_pushcclosure(L, getset, 1);
      lua_setfield(L, -2, entry.first.c_str());
    }
  }

  int makeLocal(lua_State* L) {
    lua_newtable(L);
    int tableIndex = lua_gettop(L);
    pushFunctions(L);
    pushProperties(L);
    return tableIndex;
  }

  void makeGlobal(const std::string& name) override {
    auto L = m_engine.get<LuaEngine>()->L;
    lua_pushvalue(L, makeLocal(L));
    lua_setglobal(L, name.c_str());
  }
};

static InternalScriptObject::Regular<LuaScriptObject> luaSO("LuaScriptObject");

#endif
