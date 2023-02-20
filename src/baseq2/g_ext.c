

//
// Reki
// API extensions from AQTION_EXTENSION
//

#include "g_local.h"
#ifdef AQTION_EXTENSION
//
// Reki
// Setup struct and macro for defining engine-callable entrypoints
// basically a copy-paste of the same system for game-to-engine calls,
// just in reverse
typedef struct extension_func_s
{
	char		name[MAX_QPATH];
	void*		func;
	struct extension_func_s *n;
} extension_func_t;
extension_func_t *g_extension_funcs;

// the do {} while here is a bizarre C-ism to allow for our local variable, probably not the best way to do this
#define g_addextension(ename, efunc) \
				do { \
				extension_func_t *ext = gi.TagMalloc(sizeof(extension_func_t), TAG_GAME); \
				strcpy(ext->name, ename); \
				ext->func = efunc; \
				ext->n = g_extension_funcs; \
				g_extension_funcs = ext; \
				} while (0);

//
// declare engine trap pointers, to make the compiler happy
int(*engine_Client_GetVersion)(edict_t *ent);
int(*engine_Client_GetProtocol)(edict_t *ent);

void(*engine_Pmove_AddField)(char *name, int size);


//
// Pmove Extensions
//
void G_InitPmoveFields(void)
{
	pmove_extfields = 0;
#define PME_I(field) Pmove_AddField(#field, sizeof(((pmoveExtend_t *)0)->field));
#define PME_F(field) Pmove_AddField(#field, sizeof(((pmoveExtend_t *)0)->field));
#define PME_S(field) Pmove_AddField(#field, sizeof(((pmoveExtend_t *)0)->field));
#define PME_B(field) Pmove_AddField(#field, sizeof(((pmoveExtend_t *)0)->field));
#define PME_V(field) Pmove_AddField(#field, sizeof(((pmoveExtend_t *)0)->field));
	PMOVE_EXT_FIELDS
}


//
// optional new entrypoints the engine may want to call
edict_t *xerp_ent;
trace_t q_gameabi XERP_trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	return gi.trace(start, mins, maxs, end, xerp_ent, MASK_PLAYERSOLID);
}

int G_customizeentityforclient(edict_t *client, edict_t *ent, entity_state_t *state)
{
	return true;
}


void G_InitExtEntrypoints(void)
{
	g_addextension("customizeentityforclient", G_customizeentityforclient);
}


void* G_FetchGameExtension(char *name)
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



// 
// new engine functions we can call from the game

//
// client network querying
//
int Client_GetVersion(edict_t *ent)
{
	if (!engine_Client_GetVersion)
		return 0;

	return engine_Client_GetVersion(ent);
}


int Client_GetProtocol(edict_t *ent)
{
	if (!engine_Client_GetProtocol)
		return 34;

	return engine_Client_GetProtocol(ent);
}


//
// dynamic pmove fields
//
int pmove_extfields;
void Pmove_AddField(char *name, int size)
{
	if (!engine_Pmove_AddField)
		return;

	pmove_extfields++;
	engine_Pmove_AddField(name, size);
}
#endif