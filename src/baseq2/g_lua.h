//
// q2 pmove lua extension
//
// g_lua.h
//
// Sam "Reki" Piper - 9/13/22
//

#ifndef LUA_H
#include <assert.h>
#include "client/lua.h"
#include "client/luaconf.h"
#include "client/lauxlib.h"
#include "client/lualib.h"

void lua_init(void);
void q2a_fpu_q2(void);
void q2a_fpu_lua(void);

#define LUA_H
#endif