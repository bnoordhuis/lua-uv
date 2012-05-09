package = "lua-uv"
version = "scm-1"
source = {
   url = "git://github.com/bnoordhuis/lua-uv.git",
   branch = "master"
}
description = {
   summary = "Lua + libuv = sweet async goodness",
   homepage = "http://github.com/bnoordhuis/lua-uv",
   license = "MIT/X11",
}
dependencies = {
   "lua >= 5.1"
}
build = {
   type = "make"
}
