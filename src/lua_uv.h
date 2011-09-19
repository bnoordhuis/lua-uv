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

#ifndef LUV_H_
#define LUV_H_

#include "uv.h"

#include <stddef.h> /* offsetof */

#include <lua.h>
#include <lauxlib.h>

/* Lifted from the Lua source. Make stack index absolute. */
#define abs_index(L, i) \
  ((i) > 0 || (i) <= LUA_REGISTRYINDEX ? (i) : lua_gettop(L) + (i) + 1)

#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offsetof(type, member)))

#define ARRAY_SIZE(a) (sizeof((a)) / sizeof((a)[0]))

void luaopen_uv_core(lua_State *L);
void luaopen_uv_tcp(lua_State *L);

void* new_object(lua_State* L, size_t size, const char* clazz);
void set_callback(lua_State* L, const char* name, int index);
void clear_callback(lua_State* L, const char* name, void* object);
void push_registry(lua_State* L);
void push_object(lua_State* L, void* object);
void push_callback(lua_State* L, void* object, const char* name);

#endif /* lua_uv.h */
