//
// q2 pmove lua extension
//
// g_lua.h
//
// Sam "Reki" Piper - 9/16/22
//

#include "client.h"
#ifdef LUA_VM
#ifdef WIN32
/* dlfcn.c */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define RTLD_GLOBAL 0x100 /* do not hide entries in this module */
#define RTLD_LOCAL  0x000 /* hide entries in this module */

#define RTLD_LAZY   0x000 /* accept unresolved externs */
#define RTLD_NOW    0x001 /* abort if module has unresolved externs */

static struct {
	long lasterror;
	const char *err_rutin;
} var = {
	0,
	NULL
};

void *dlopen(const char *filename, int flags)
{
	HINSTANCE hInst;

	hInst = LoadLibrary(filename);
	if (hInst == NULL) {
		var.lasterror = GetLastError();
		var.err_rutin = "dlopen";
	}
	return hInst;
}

int dlclose(void *handle)
{
	BOOL ok;
	int rc = 0;

	ok = FreeLibrary((HINSTANCE)handle);
	if (!ok) {
		var.lasterror = GetLastError();
		var.err_rutin = "dlclose";
		rc = -1;
	}
	return rc;
}

void *dlsym(void *handle, const char *name)
{
	FARPROC fp;

	fp = GetProcAddress((HINSTANCE)handle, name);
	if (!fp) {
		var.lasterror = GetLastError();
		var.err_rutin = "dlsym";
	}
	return (void *)(intptr_t)fp;
}

const char *dlerror(void)
{
	static char errstr[88];

	if (var.lasterror) {
		sprintf(errstr, "%s error #%ld", var.err_rutin, var.lasterror);
		return errstr;
	}
	else {
		return NULL;
	}
}
#else
#include <dlfcn.h>
#endif

#include <assert.h>
#include "client/lua.h"
#include "client/luaconf.h"
#include "client/lauxlib.h"
#include "client/lualib.h"

lua_State *L;
void *lua_dll = NULL;


int gamelua_call(lua_State *L, int nargs, int nresults, int msgh)
{
	if (lua_pcall(L, nargs, nresults, msgh) != 0) {
		char *err_msg = (char *)lua_tostring(L, -1);
		Com_Printf("cslua: calling function failed: %s\n", err_msg);
		//q2a_fpu_q2();
		return false;
	}

	return true;
}


static int l_dprint(lua_State *L)
{
	char *str;
	str = (char *)lua_tostring(L, 1);
	Com_Printf("%s", str);
	return 0;
}


static int l_pmove_getvalue(lua_State *L)
{
	byte *pm = lua_touserdata(L, 1);
	int offs = lua_tonumber(L, 2);
	int size = lua_tonumber(L, 3);

	if (offs < 0 || offs > sizeof(pmove_t))
	{
		Com_Printf("lua: l_pmove_getvalue tried to cause a memory access violation (INVALID OFFSET)\n");
		return 0;
	}

	if (size == 1)
	{
		byte ret = 0;
		memcpy(&ret, pm + offs, size);
		lua_pushinteger(L, ret);
	}
	else if (size == 2)
	{
		short ret = 0;
		memcpy(&ret, pm + offs, size);
		lua_pushinteger(L, ret);
	}
	else
	{
		int ret = 0;
		memcpy(&ret, pm + offs, size);
		lua_pushnumber(L, ret);
	}
	return 1;
}


static int l_pmove_setvalue(lua_State *L)
{
	byte *pm = lua_touserdata(L, 1);
	int offs = lua_tonumber(L, 2);
	int size = lua_tonumber(L, 3);
	int value = lua_tonumber(L, 4);

	if (offs < 0 || offs > sizeof(pmove_t))
	{
		Com_Printf("lua: l_pmove_setvalue tried to cause a memory access violation (INVALID OFFSET)\n");
		return 0;
	}

	memcpy(pm + offs, &value, size);
	return 0;
}

static int l_pmove_getvaluef(lua_State *L)
{
	byte *pm = lua_touserdata(L, 1);
	int offs = lua_tonumber(L, 2);

	if (offs < 0 || (offs + 4) > sizeof(pmove_t))
	{
		Com_Printf("lua: l_pmove_getvaluef tried to cause a memory access violation (INVALID OFFSET)\n");
		return 0;
	}

	float ret = 0;
	memcpy(&ret, pm + offs, 4);
	lua_pushnumber(L, ret);
	return 1;
}


static int l_pmove_setvaluef(lua_State *L)
{
	byte *pm = lua_touserdata(L, 1);
	int offs = lua_tonumber(L, 2);
	float value = lua_tonumber(L, 3);

	if (offs < 0 || (offs + 4) > sizeof(pmove_t))
	{
		Com_Printf("lua: l_pmove_setvaluef tried to cause a memory access violation (INVALID OFFSET)\n");
		return 0;
	}

	memcpy(pm + offs, &value, 4);
	return 0;
}

static int l_pstate_getvalue(lua_State *L)
{
	byte *pm = lua_touserdata(L, 1);
	int offs = lua_tonumber(L, 2);
	int size = lua_tonumber(L, 3);

	if (offs < 0 || offs + size > sizeof(player_state_t))
	{
		Com_Printf("lua: l_pstate_getvalue tried to cause a memory access violation (INVALID OFFSET)\n");
		return 0;
	}

	if (size == 1)
	{
		byte ret = 0;
		memcpy(&ret, pm + offs, size);
		lua_pushinteger(L, ret);
	}
	else if (size == 2)
	{
		short ret = 0;
		memcpy(&ret, pm + offs, size);
		lua_pushinteger(L, ret);
	}
	else
	{
		int ret = 0;
		memcpy(&ret, pm + offs, size);
		lua_pushnumber(L, ret);
	}
	return 1;
}

static int l_pstate_getvaluef(lua_State *L)
{
	byte *pm = lua_touserdata(L, 1);
	int offs = lua_tonumber(L, 2);

	if (offs < 0 || (offs + 4) > sizeof(player_state_t))
	{
		Com_Printf("lua: l_pstate_getvalue tried to cause a memory access violation (INVALID OFFSET)\n");
		return 0;
	}

	float ret = 0;
	memcpy(&ret, pm + offs, 4);
	lua_pushnumber(L, ret);
	return 1;
}


static int l_pstate_setvalue(lua_State *L)
{
	byte *pm = lua_touserdata(L, 1);
	int offs = lua_tonumber(L, 2);
	int size = lua_tonumber(L, 3);
	int value = lua_tonumber(L, 4);

	if (offs < 0 || (offs + size) > sizeof(player_state_t))
	{
		Com_Printf("lua: l_pstate_setvalue tried to cause a memory access violation (INVALID OFFSET)\n");
		return 0;
	}

	memcpy(pm + offs, &value, size);
	return 0;
}

static int l_pstate_setvaluef(lua_State *L)
{
	byte *pm = lua_touserdata(L, 1);
	int offs = lua_tonumber(L, 2);
	float value = lua_tonumber(L, 3);

	if (offs < 0 || (offs + 4) > sizeof(player_state_t))
	{
		Com_Printf("lua: l_pstate_setvalue tried to cause a memory access violation (INVALID OFFSET)\n");
		return 0;
	}

	memcpy(pm + offs, &value, 4);
	return 0;
}


void lua_pmoveinit()
{
	if (!L) return;

	// pmove_t
	lua_pushinteger(L, cl.pme.fieldcount);
	lua_setglobal(L, "pmove_extfields");

	lua_pushcfunction(L, l_pmove_getvalue);
	lua_setglobal(L, "Pmove_GetValue");

	lua_pushcfunction(L, l_pmove_setvalue);
	lua_setglobal(L, "Pmove_SetValue");

	lua_pushcfunction(L, l_pmove_getvaluef);
	lua_setglobal(L, "Pmove_GetValueF");

	lua_pushcfunction(L, l_pmove_setvaluef);
	lua_setglobal(L, "Pmove_SetValueF");

#define CMD_FIELD(field) lua_newtable(L); lua_pushinteger(L, sizeof(((usercmd_t*)0)->field)); lua_setfield(L, 3, "size"); lua_pushinteger(L, offsetof(pmove_t, cmd) + offsetof(usercmd_t, field)); lua_setfield(L, 3, "offset"); lua_setfield(L, 2, #field);
#define P_FIELD(field) lua_newtable(L); lua_pushinteger(L, sizeof(((pmove_t*)0)->field)); lua_setfield(L, 2, "size"); lua_pushinteger(L, offsetof(pmove_t, field)); lua_setfield(L, 2, "offset"); lua_setfield(L, 1, #field);
#define PM_FIELD(field) lua_newtable(L); lua_pushinteger(L, sizeof(((pmove_state_t*)0)->field)); lua_setfield(L, 2, "size"); lua_pushinteger(L, offsetof(pmove_t, s) + offsetof(pmove_state_t, field)); lua_setfield(L, 2, "offset"); lua_setfield(L, 1, #field);
	lua_newtable(L); // the master "pmove" table
	// pmove_t, pmove_state_t, and pmove_state_t extensions
	P_FIELD(viewangles);
	P_FIELD(viewheight);
	P_FIELD(groundentity);
	P_FIELD(watertype);
	P_FIELD(waterlevel);
	PM_FIELD(pm_type);
	PM_FIELD(origin);
	PM_FIELD(velocity);
	PM_FIELD(pm_flags);
	PM_FIELD(pm_time);
	PM_FIELD(gravity);
	PM_FIELD(delta_angles);
	PM_FIELD(pm_aq2_flags);
	PM_FIELD(pm_timestamp);
	PM_FIELD(pm_aq2_leghits);
	for (int j = 0; j < cl.pme.fieldcount; j++)
	{
		lua_newtable(L);
		lua_pushinteger(L, cl.pme.field[j].size); lua_setfield(L, 2, "size");
		lua_pushinteger(L, offsetof(pmove_t, s) + offsetof(pmove_state_t, efields) + cl.pme.field[j].fieldoffset); lua_setfield(L, 2, "offset");
		lua_setfield(L, 1, cl.pme.field[j].name);
	}
	//
	// add cmd
	lua_newtable(L);
	CMD_FIELD(msec);
	CMD_FIELD(buttons);
	CMD_FIELD(angles);
	CMD_FIELD(forwardmove);
	CMD_FIELD(sidemove);
	CMD_FIELD(upmove);
	CMD_FIELD(impulse);
	CMD_FIELD(lightlevel);
	lua_setfield(L, 1, "cmd");
	//
	lua_setglobal(L, "pmove"); // pop into the "pmove" table
	//

	///*
	// player_state_t
	lua_pushcfunction(L, l_pstate_getvalue);
	lua_setglobal(L, "Pstate_GetValue");

	lua_pushcfunction(L, l_pstate_getvaluef);
	lua_setglobal(L, "Pstate_GetValueF");

	lua_pushcfunction(L, l_pstate_setvalue);
	lua_setglobal(L, "Pstate_SetValue");

	lua_pushcfunction(L, l_pstate_setvaluef);
	lua_setglobal(L, "Pstate_SetValueF");

	///*
#define CSTATE_FIELD(field) lua_newtable(L); lua_pushinteger(L, sizeof(((player_state_t*)0)->field)); lua_setfield(L, 2, "size"); lua_pushinteger(L, offsetof(player_state_t, field)); lua_setfield(L, 2, "offset"); lua_setfield(L, 1, #field);
	lua_newtable(L); // the master "player_state" table
	CSTATE_FIELD(pmove);
	CSTATE_FIELD(viewangles);
	CSTATE_FIELD(viewoffset);
	CSTATE_FIELD(kick_angles);
	CSTATE_FIELD(gunangles);
	CSTATE_FIELD(gunoffset);
	CSTATE_FIELD(gunindex);
	CSTATE_FIELD(gunframe);
	CSTATE_FIELD(blend);
	CSTATE_FIELD(fov);
	CSTATE_FIELD(rdflags);
	CSTATE_FIELD(stats);
	lua_setglobal(L, "pstate"); // pop into the "pmove" table
	//
	//*/

	lua_getglobal(L, "pmove_init");
	if (gamelua_call(L, 0, 0, 0))
	{

	}
}











void LUA_ClearState(void)
{
	cl.lua_active = false;

	if (!L) return;

	/* run the shutdown Lua routine */
	lua_getglobal(L, "gamelua_shutdown");
	lua_pcall(L, 0, 0, 0);

	lua_close(L);
	L = NULL;
}


void LUA_InitializeState(void)
{
	if (!cls.lua_loaded)
		return;

	if (L) return;

	L = luaL_newstate();
	luaL_openlibs(L);

	/* run the initialization Lua routine */
	FILE *main_file;
	char filepath[256];
	sprintf(filepath, "%s/gamelua/%s", "baseq2", "main.lua");
	main_file = fopen(filepath, "r");
	if (main_file == NULL)
	{
		Com_Printf("cslua: %s does not exist\n", filepath);
		return;
	}
	fclose(main_file);

	int err = luaL_dofile(L, filepath);
	if (err)
	{
		char *err_msg = (char *)lua_tostring(L, -1);
		Com_Printf("cslua: loading main.lua failed: %s\n", err_msg);
		return;
	}

	lua_pushcfunction(L, l_dprint);
	lua_setglobal(L, "Com_Print");

	lua_pushinteger(L, 1);
	lua_setglobal(L, "is_client");

	lua_pmoveinit();
	
	Com_Printf("cslua: LUA is loaded!\n");
	cl.lua_active = true;
}


void LUA_LoadLibrary(void)
{
#ifdef WIN32
	lua_dll = dlopen("liblua5.4.dll", RTLD_NOW | RTLD_LOCAL);
	if (!lua_dll) {
		lua_dll = dlopen("lua54.dll", RTLD_NOW | RTLD_LOCAL);
	}
	if (!lua_dll) {
		lua_dll = dlopen("lua5.4.dll", RTLD_NOW | RTLD_LOCAL);
	}
#else
	lua_dll = dlopen("liblua5.4.so", RTLD_NOW | RTLD_LOCAL);
	if (!lua_dll) {
		lua_dll = dlopen("liblua.so.5.4", RTLD_NOW | RTLD_LOCAL);
	}
	if (!lua_dll) {
		lua_dll = dlopen("lua54.so", RTLD_NOW | RTLD_LOCAL);
	}
	if (!lua_dll) {
		lua_dll = dlopen("lua5.4.so", RTLD_NOW | RTLD_LOCAL);
	}
#endif

	if (!lua_dll) {
		Com_Printf("gamelua: Loading Lua shared object failed\n");
		return;
	}

	cls.lua_loaded = true;
}


void LUA_PmoveRun(pmove_t *pm)
{
	if (!cl.lua_active)
		return;

	lua_getglobal(L, "pmove_run");
	lua_pushlightuserdata(L, pm);
	gamelua_call(L, 1, 0, 0);
}


void LUA_PstateRun(player_state_t *ps, pmove_t *pm)
{
	if (!cl.lua_active)
		return;

	lua_getglobal(L, "ps_run");
	lua_pushlightuserdata(L, ps);
	lua_pushlightuserdata(L, pm);
	gamelua_call(L, 2, 0, 0);
}









#endif
