/*
 * Copyright (c) 2011, Ben Noordhuis <info@bnoordhuis.nl>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "lua_uv.h"
#include <assert.h>


static char object_registry[0];


static int traceback(lua_State *L) {
  if (!lua_isstring(L, 1)) return 1;

  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }

  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }

  lua_pushvalue(L, 1);    /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);      /* call debug.traceback */

  return 1;
}


/**
 *
 * Function signature: [-0, +0, ?]
 */
static void create_object_registry(lua_State* L) {
  lua_pushlightuserdata(L, object_registry);
  lua_newtable(L);

  /* Make registry values weak. Allows the Lua GC
   * to clean up our stale objects for us.
   */
  lua_createtable(L, 0, 1);
  lua_pushstring(L, "v");
  lua_setfield(L, -2, "__mode");

  lua_setmetatable(L, -2);
  lua_rawset(L, LUA_REGISTRYINDEX);
}


/**
 *
 * Function signature: [-0, +1, ?]
 */
void* new_object(lua_State* L, size_t size, const char* clazz) {
  void* object;

  /* Create object with metatable. */
  object = lua_newuserdata(L, size);
  luaL_getmetatable(L, clazz);
  lua_setmetatable(L, -2);

  /* Create storage for callbacks. */
  lua_createtable(L, 1, 0);
  lua_setfenv(L, -2);

  push_registry(L);

  /* Associate our object with the Lua object. */
  lua_pushlightuserdata(L, object);
  lua_pushvalue(L, -3); /* object */
  lua_rawset(L, -3); /* registry */

  /* Pop our object registry from the stack. */
  lua_pop(L, 1);

  return object;
}


/**
 * Set callback.
 *
 * `name` is the key to save the callback under.
 * `index` is the stack index of the Lua callback.
 *
 * This function assumes that the first element on the stack
 * is the Lua object to attach the callback to.
 *
 * Function signature: [-0, +0, ?]
 */
void set_callback(lua_State* L, const char* name, int index) {
  index = abs_index(L, index);
  luaL_checktype(L, index, LUA_TFUNCTION);

  lua_getfenv(L, 1);
  lua_pushstring(L, name);
  lua_pushvalue(L, index);
  lua_rawset(L, -3);
  lua_pop(L, 1);
}


void clear_callback(lua_State* L, const char* name, void* object) {
  push_object(L, object);
  lua_getfenv(L, -1);
  lua_pushstring(L, name);
  lua_pushnil(L);
  lua_rawset(L, -3);
  lua_pop(L, 2);
}


/**
 * Push our object registry onto the stack.
 *
 * Function signature: [-0, +1, ?]
 */
void push_registry(lua_State* L) {
  /* Push our object registry onto the stack. */
  lua_pushlightuserdata(L, object_registry);
  lua_rawget(L, LUA_REGISTRYINDEX);
  assert(lua_istable(L, -1));
}


/**
 * Find Lua object by handle and push it onto the stack.
 *
 * Function signature: [-0, +1, ?]
 */
void push_object(lua_State* L, void* object) {
  /* Push our object registry onto the stack. */
  lua_pushlightuserdata(L, object_registry);
  lua_rawget(L, LUA_REGISTRYINDEX);
  assert(lua_istable(L, -1));

  /* Look up the Lua object associated with this handle. */
  lua_pushlightuserdata(L, object);
  lua_rawget(L, -2);

  /* STACK: <registry> <object> */
  lua_remove(L, -2);
  /* STACK: <object> */
}


/**
 * Pushes "<traceback> <callback> <object>" onto the stack.
 *
 * Function signature: [-0, +3, ?]
 */
void push_callback(lua_State* L, void* object, const char* name) {
  lua_pushcfunction(L, traceback);

  push_object(L, object);

  /* Get the callback table. */
  lua_getfenv(L, -1);
  assert(lua_istable(L, -1));

  /* Look up callback. */
  lua_pushstring(L, name);
  lua_rawget(L, -2);
  assert(lua_isfunction(L, -1));

  /* STACK: <object> <table> <callback> */
  lua_remove(L, -2);
  /* STACK: <object> <callback> */

  /* STACK: <object> <callback> */
  lua_pushvalue(L, -2);
  lua_remove(L, -3);
  /* STACK: <callback> <object> */
}


#ifdef WIN32
__declspec(dllexport)
#endif
int luaopen_uv(lua_State *L) {
  luaL_reg functions[] = {{NULL, NULL}};

  uv_default_loop()->data = L;
  create_object_registry(L);

  luaL_register(L, "uv", functions);
  luaopen_uv_core(L);
  luaopen_uv_tcp(L);

  return 1;
}
