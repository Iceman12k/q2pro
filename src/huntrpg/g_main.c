

#include "g_local.h"


game_locals_t   game;
level_locals_t  level;
game_import_t   gi;
game_import_ex_t gix;
game_export_t   globals;
game_export_ex_t globalsx;
spawn_temp_t    st;

edict_t     *g_edicts;

cvar_t  *maxclients;
cvar_t  *maxspectators;
cvar_t  *maxentities;
cvar_t  *filterban;

cvar_t  *password;
cvar_t  *spectator_password;
cvar_t  *needpass;

cvar_t  *flood_msgs;
cvar_t  *flood_persecond;
cvar_t  *flood_waitdelay;

cvar_t  *sv_gravity;
cvar_t  *sv_maxvelocity;


extension_func_t *g_extension_funcs;
void*(*engine_MSG_WriteData)(const void *data, size_t len);

void* WriteData(const void *data, size_t len)
{
	if (!engine_MSG_WriteData)
		return NULL;

	return engine_MSG_WriteData(data, len);
}


// this is only here so the functions in q_shared.c can link
void Com_LPrintf(print_type_t type, const char *fmt, ...)
{
	va_list     argptr;
	char        text[MAX_STRING_CHARS];

	if (type == PRINT_DEVELOPER) {
		return;
	}

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	gi.dprintf("%s", text);
}

void Com_Error(error_type_t type, const char *fmt, ...)
{
	va_list     argptr;
	char        text[MAX_STRING_CHARS];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	gi.error("%s", text);
}



static int G_customizeentityforclient(edict_t *viewer, edict_t *ent, entity_state_t *state)
{
	// do whatever 'global' filtering
	// dimension_see ?

	// predraw 
	if (ent->predraw)
		return ent->predraw(viewer, ent, state);
	return true;
}


static void* G_FetchGameExtension(char *name)
{
	Com_Printf("Game: G_FetchGameExtension for %s\n", name);
	extension_func_t *ext;
	for (ext = g_extension_funcs; ext != NULL; ext = ext->n)
	{
		if (strcmp(ext->name, name))
			continue;

		return ext->func;
	}

	Com_Printf("Game: Extension not found.\n");
	return NULL;
}

static void G_InitExtEntrypoints(void)
{
	g_addextension("customizeentityforclient", G_customizeentityforclient);
}


q_exported game_export_t *GetGameAPI(game_import_t *import)
{
	gi = *import;

	globals.apiversion = GAME_API_VERSION;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities;

	globals.WriteGame = WriteGame;
	globals.ReadGame = ReadGame;
	globals.WriteLevel = WriteLevel;
	globals.ReadLevel = ReadLevel;

	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;

	globals.RunFrame = G_RunFrame;

	globals.ServerCommand = ServerCommand;

	globals.edict_size = sizeof(edict_t);

	return &globals;
}

q_exported game_export_ex_t *GetExtendedGameAPI(game_import_ex_t *importx)
{
	gix = *importx;

	globalsx.apiversion = GAME_API_VERSION_EX;
	globalsx.CheckForExtension = G_FetchGameExtension;

#ifdef GAME_API_EXTENSIONS
	G_InitExtEntrypoints();

	engine_MSG_WriteData = gix.CheckForExtension("MSG_WriteData");
#endif



	return &globalsx;
}


/*
=================
ClientEndServerFrames
=================
*/
void ClientEndServerFrames(void)
{
	int     i;
	edict_t *ent;

	// calc the player views now that all pushing
	// and damage has been added
	for (i = 0; i < maxclients->value; i++) {
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		ClientEndServerFrame(ent);
	}

}