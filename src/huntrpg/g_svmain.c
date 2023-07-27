
#include "g_local.h"

/*
=================
CheckNeedPass
=================
*/
void CheckNeedPass(void)
{
	int need;

	// if password or spectator_password has changed, update needpass
	// as needed
	if (password->modified || spectator_password->modified) {
		password->modified = spectator_password->modified = false;

		need = 0;

		if (*password->string && Q_stricmp(password->string, "none"))
			need |= 1;
		if (*spectator_password->string && Q_stricmp(spectator_password->string, "none"))
			need |= 2;

		gi.cvar_set("needpass", va("%d", need));
	}
}

void G_RunEntity(edict_t *ent)
{

}

void G_RunFrame(void)
{
	int     i;
	edict_t *ent;

	level.framenum++;
	level.time = level.framenum * FRAMETIME;

	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	ent = &g_edicts[0];
	for (i = 0; i < globals.num_edicts; i++, ent++) {
		if (!ent->inuse)
			continue;

		level.current_entity = ent;

		VectorCopy(ent->s.origin, ent->s.old_origin);

		// if the ground entity moved, make sure we are still on it
		if ((ent->groundentity) && (ent->groundentity->linkcount != ent->groundentity_linkcount)) {
			ent->groundentity = NULL;
			/*
			if (ent->svflags & SVF_MONSTER) {
				M_CheckGround(ent);
			}
			*/
		}

		if (i > 0 && i <= maxclients->value) {
			ClientBeginServerFrame(ent);
			continue;
		}

		G_RunEntity(ent);
	}

	// see if needpass needs updated
	CheckNeedPass();

	// build the playerstate_t structures for all players
	ClientEndServerFrames();
}

void ShutdownGame(void)
{
	gi.dprintf("==== ShutdownGame ====\n");

	gi.FreeTags(TAG_LEVEL);
	gi.FreeTags(TAG_GAME);
}

void InitGame(void)
{
	gi.dprintf("==== InitGame ====\n");

	Q_srand(time(NULL));

	// init cvars
	maxclients = gi.cvar("maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH);
	maxspectators = gi.cvar("maxspectators", "4", CVAR_SERVERINFO);
	maxentities = gi.cvar("maxentities", "1024", CVAR_LATCH);
	filterban = gi.cvar("filterban", "1", 0);

	sv_maxvelocity = gi.cvar("sv_maxvelocity", "2000", 0);
	sv_gravity = gi.cvar("sv_gravity", "800", 0);
	password = gi.cvar("password", "", CVAR_USERINFO);
	spectator_password = gi.cvar("spectator_password", "", CVAR_USERINFO);
	needpass = gi.cvar("needpass", "0", CVAR_SERVERINFO);

	// flood control
	flood_msgs = gi.cvar("flood_msgs", "4", 0);
	flood_persecond = gi.cvar("flood_persecond", "4", 0);
	flood_waitdelay = gi.cvar("flood_waitdelay", "10", 0);


	// initialize all entities for this game
	game.maxentities = maxentities->value;
	clamp(game.maxentities, (int)maxclients->value + 1, MAX_EDICTS);
	g_edicts = gi.TagMalloc(game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
	globals.edicts = g_edicts;
	globals.max_edicts = game.maxentities;

	// initialize all clients for this game
	game.maxclients = maxclients->value;
	game.clients = gi.TagMalloc(game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	globals.num_edicts = game.maxclients + 1;

	// obtain server features
	cvar_t *cv;
	cv = gi.cvar("sv_features", NULL, 0);
	if (cv) {
		game.serverfeatures = (int)cv->value;
	}

	// setup framerate parameters
	if (game.serverfeatures & GMF_VARIABLE_FPS) {
		int framediv;

		cv = gi.cvar("sv_fps", NULL, 0);
		if (!cv)
			gi.error("GMF_VARIABLE_FPS exported but no 'sv_fps' cvar");

		framediv = (int)cv->value / BASE_FRAMERATE;

		clamp(framediv, 1, MAX_FRAMEDIV);

		game.framerate = framediv * BASE_FRAMERATE;
		game.frametime = BASE_FRAMETIME_1000 / framediv;
		game.framediv = framediv;
	}
	else {
		game.framerate = BASE_FRAMERATE;
		game.frametime = BASE_FRAMETIME_1000;
		game.framediv = 1;
	}

	// export our own features
	gi.cvar_forceset("g_features", va("%d", G_FEATURES));
}


void WriteGame(const char *filename, qboolean autosave)
{
}

void ReadGame(const char *filename)
{
}

void WriteLevel(const char *filename)
{
}

void ReadLevel(const char *filename)
{
}













