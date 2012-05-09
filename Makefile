B ?= build

CFLAGS = -pthread -Wall -Wextra -Wno-unused-parameter -Ideps/http_parser -Ideps/libuv/include
LDFLAGS = -lpthread -lrt

LUA_DIR ?= /usr
LUA_LIBDIR = $(LUA_DIR)/lib/lua/5.1
LUA_SHAREDIR = $(LUA_DIR)/share/lua/5.1

SOURCES = \
	src/lua_uv.c \
	src/core.c \
	src/tcp.c \

override CFLAGS += \
	$(shell pkg-config --cflags lua5.1 --silence-errors || pkg-config --cflags lua-5.1 --silence-errors || pkg-config --cflags lua) \
	$(shell apr-1-config --cflags --cppflags --includes 2>/dev/null || pkg-config --cflags apr-1) \
	$(shell apu-1-config --includes 2>/dev/null || pkg-config --cflags apr-util-1)

override LDFLAGS += \
	$(shell apr-1-config --link-ld --libs 2>/dev/null || pkg-config --libs apr-1) \
	$(shell apu-1-config --link-ld --libs --ldap-libs 2>/dev/null || pkg-config --libs apr-util-1)

ifdef DEBUG
	override CFLAGS += -g -O0
else
	override CFLAGS += -O2 -DNDEBUG
endif

.PHONY:	prereqs clean install

OBJECTS = $(patsubst src/%.c,$(B)/%.o,$(SOURCES))

$(B)/uv.so:	prereqs $(OBJECTS) $(B)/http_parser.o deps/libuv/uv.a
	$(CC) -shared -o $@ $(OBJECTS) deps/libuv/uv.a $(LDFLAGS)

$(B)/http_parser.o:	deps/http_parser/http_parser.c deps/http_parser/http_parser.h
	$(CC) -c $(CFLAGS) -fPIC $< -o $@

$(OBJECTS): $(B)/%.o: src/%.c src/lua_uv.h Makefile
	$(CC) -c $(CFLAGS) -fPIC $< -o $@

prereqs:
	[ -d $(B) ] || mkdir $(B)

clean:
	rm -rf $(B)

install:	$(B)/uv.so
	mkdir -p $(LUA_LIBDIR)
	cp $(B)/uv.so $(LUA_LIBDIR)

deps/libuv/uv.a:
	$(MAKE) -C deps/libuv CFLAGS+=-fPIC
