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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <lua.h>
#include <lauxlib.h>


#define UNWRAP(h) \
  tcp_obj* self = container_of((h), tcp_obj, handle); \
  lua_State* L = self->handle.loop->data;


typedef struct {
  uv_tcp_t handle;
  uv_connect_t connect_req; /* TODO alloc on as needed basis */
} tcp_obj;


static uv_buf_t on_alloc(uv_handle_t* handle, size_t suggested_size) {
  puts(__func__);
  return uv_buf_init(malloc(suggested_size), suggested_size);
}


static void on_connect(uv_connect_t* req, int status) {
  UNWRAP(req->handle);
  push_callback(L, self, "on_connect");
  lua_pushinteger(L, status);
  lua_pcall(L, 2, 0, -4);
}


static void on_connection(uv_stream_t* handle, int status) {
  UNWRAP(handle);
  puts(__func__);
  push_callback(L, self, "on_connection");
  lua_pushinteger(L, status);
  lua_pcall(L, 2, 0, -4);
}


static void on_read(uv_stream_t* handle, ssize_t nread, uv_buf_t buf) {
  UNWRAP(handle);
  puts(__func__);
  push_callback(L, self, "on_read");
  lua_pushinteger(L, nread);
  if (nread >= 0)
    lua_pushlstring(L, buf.base, nread);
  else
    lua_pushnil(L);
  lua_pcall(L, 3, 0, -5);
}


static void on_close(uv_handle_t* handle) {
  UNWRAP(handle);
  push_callback(L, self, "on_close");
  lua_pcall(L, 1, 0, -3);
}


static int tcp_new(lua_State* L) {
  uv_loop_t* loop;
  tcp_obj* self;

  loop = uv_default_loop();
  assert(L == loop->data);

  self = new_object(L, sizeof *self, "uv.tcp");
  uv_tcp_init(loop, &self->handle);

  return 1;
}


static int tcp_accept(lua_State* L) {
  tcp_obj* self;
  tcp_obj* obj;
  int r;

  self = luaL_checkudata(L, 1, "uv.tcp");

  obj = new_object(L, sizeof *obj, "uv.tcp");
  uv_tcp_init(self->handle.loop, &obj->handle);

  r = uv_accept((uv_stream_t*)&self->handle,
                (uv_stream_t*)&obj->handle);
  lua_pushinteger(L, r);

  return 2;
}


static int tcp_bind(lua_State* L) {
  struct sockaddr_in addr;
  const char* host;
  tcp_obj* self;
  int port;
  int r;

  self = luaL_checkudata(L, 1, "uv.tcp");
  host = luaL_checkstring(L, 2);
  port = luaL_checkint(L, 3);

  addr = uv_ip4_addr(host, port);

  r = uv_tcp_bind(&self->handle, addr);
  lua_pushinteger(L, r);

  return 1;
}


static int tcp_connect(lua_State* L) {
  struct sockaddr_in addr;
  const char* host;
  tcp_obj* self;
  int port;
  int r;

  self = luaL_checkudata(L, 1, "uv.tcp");
  host = luaL_checkstring(L, 2);
  port = luaL_checkint(L, 3);
  set_callback(L, "on_connect", 4);

  addr = uv_ip4_addr(host, port);

  r = uv_tcp_connect(&self->connect_req, &self->handle, addr, on_connect);
  lua_pushinteger(L, r);

  return 1;
}


static int tcp_close(lua_State* L) {
  tcp_obj* self;

  self = luaL_checkudata(L, 1, "uv.tcp");

  if (!lua_isnil(L, 2))
    set_callback(L, "on_close", 2);

  uv_close((uv_handle_t*)&self->handle, on_close);

  return 0;
}


static int tcp_listen(lua_State* L) {
  tcp_obj* self;
  int backlog;
  int r;

  self = luaL_checkudata(L, 1, "uv.tcp");

  if (lua_isnumber(L, 2)) {
    backlog = luaL_checkinteger(L, 2);
    set_callback(L, "on_connection", 3);
  }
  else {
    backlog = 128;
    set_callback(L, "on_connection", 2);
  }

  r = uv_listen((uv_stream_t*)&self->handle, backlog, on_connection);
  lua_pushinteger(L, r);

  return 1;
}


static int tcp_read_start(lua_State* L) {
  tcp_obj* self;
  int r;

  self = luaL_checkudata(L, 1, "uv.tcp");
  set_callback(L, "on_read", 2);

  r = uv_read_start((uv_stream_t*)&self->handle, on_alloc, on_read);
  lua_pushinteger(L, r);

  return 1;
}


static int tcp_read_stop(lua_State* L) {
  tcp_obj* self;
  int r;

  self = luaL_checkudata(L, 1, "uv.tcp");

  r = uv_read_stop((uv_stream_t*)&self->handle);
  lua_pushinteger(L, r);

  if (r == 0)
    clear_callback(L, "on_read", self);

  return 1;
}


static int tcp_write(lua_State* L) {
  tcp_obj* self;

  self = luaL_checkudata(L, 1, "uv.tcp");
  lua_pushinteger(L, -1);

  return 1;
}


static luaL_reg methods[] = {
  { "accept",     tcp_accept      },
  { "bind",       tcp_bind        },
  { "connect",    tcp_connect     },
  { "close",      tcp_close       },
  { "listen",     tcp_listen      },
  { "read_start", tcp_read_start  },
  { "read_stop",  tcp_read_stop   },
  { "write",      tcp_write       },
  { NULL,         NULL            }
};


static luaL_reg functions[] = {
  { "new", tcp_new },
  { NULL, NULL }
};


void luaopen_uv_tcp(lua_State *L) {
  luaL_newmetatable(L, "uv.tcp");
  luaL_register(L, NULL, methods);
  lua_setfield(L, -1, "__index");

  lua_createtable(L, 0, ARRAY_SIZE(functions) - 1);
  luaL_register(L, NULL, functions);
  lua_setfield(L, -2, "tcp");
}
