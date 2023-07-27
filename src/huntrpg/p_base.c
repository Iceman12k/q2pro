
#include "g_local.h"



/*
===========
ClientUserInfoChanged

called whenever the player updates a userinfo variable.

The game can override any of the settings in place
(forcing skins or names, etc) before copying it off.
============
*/
void ClientUserinfoChanged(edict_t *ent, char *userinfo)
{
	char    *s;
	int     playernum;

	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo)) {
		strcpy(userinfo, "\\name\\badinfo\\skin\\male/grunt");
	}

	// set name
	s = Info_ValueForKey(userinfo, "name");
	Q_strlcpy(ent->client->pers.netname, s, sizeof(ent->client->pers.netname));

	// set spectator
	s = Info_ValueForKey(userinfo, "spectator");
	ent->client->pers.spectator = false;

	// set skin
	s = Info_ValueForKey(userinfo, "skin");

	playernum = ent - g_edicts - 1;

	// combine name and skin into a configstring
	gi.configstring(CS_PLAYERSKINS + playernum, va("%s\\%s", ent->client->pers.netname, s));

	// fov
	ent->client->pers.fov = atoi(Info_ValueForKey(userinfo, "fov"));
	if (ent->client->pers.fov < 1)
		ent->client->pers.fov = 90;
	else if (ent->client->pers.fov > 160)
		ent->client->pers.fov = 160;

	// save off the userinfo in case we want to check something later
	Q_strlcpy(ent->client->pers.userinfo, userinfo, sizeof(ent->client->pers.userinfo));
}

/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
qboolean ClientConnect(edict_t *ent, char *userinfo)
{
	char    *value;

	// check to see if they are on the banned IP list
	value = Info_ValueForKey(userinfo, "ip");
	if (SV_FilterPacket(value)) {
		Info_SetValueForKey(userinfo, "rejmsg", "Banned.");
		return false;
	}

	// check for a password
	value = Info_ValueForKey(userinfo, "password");
	if (*password->string && strcmp(password->string, "none") &&
		strcmp(password->string, value)) {
		Info_SetValueForKey(userinfo, "rejmsg", "Password required or incorrect.");
		return false;
	}


	// they can connect
	ent->client = game.clients + (ent - g_edicts - 1);
	ClientUserinfoChanged(ent, userinfo);

	if (game.maxclients > 1)
		gi.dprintf("%s connected\n", ent->client->pers.netname);

	ent->svflags = 0; // make sure we start with known default
	ent->client->pers.connected = true;
	return true;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void ClientDisconnect(edict_t *ent)
{
	//int     playernum;

	if (!ent->client)
		return;

	gi.bprintf(PRINT_HIGH, "%s disconnected\n", ent->client->pers.netname);

	// send effect
	if (ent->inuse) {
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_LOGOUT);
		gi.multicast(ent->s.origin, MULTICAST_PVS);
	}

	gi.unlinkentity(ent);
	ent->s.modelindex = 0;
	ent->s.modelindex2 = 0;
	ent->s.sound = 0;
	ent->s.event = 0;
	ent->s.effects = 0;
	ent->s.renderfx = 0;
	ent->s.solid = 0;
	ent->solid = SOLID_NOT;
	ent->inuse = false;
	ent->classname = "disconnected";
	ent->client->pers.connected = false;

	// FIXME: don't break skins on corpses, etc
	//playernum = ent-g_edicts-1;
	//gi.configstring (CS_PLAYERSKINS+playernum, "");
}
//


edict_t *pm_passent;

// pmove doesn't need to know about passent and contentmask
trace_t q_gameabi PM_trace(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end)
{
	if (pm_passent->health > 0)
		return gi.trace(start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
	else
		return gi.trace(start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
}

void ClientThink(edict_t *ent, usercmd_t *ucmd)
{
	gclient_t   *client;
	edict_t *other;
	int     i, j;
	pmove_t pm;

	level.current_entity = ent;
	client = ent->client;

	pm_passent = ent;

	// set up for pmove
	memset(&pm, 0, sizeof(pm));

	client->ps.pmove.pm_type = PM_NORMAL;

	client->ps.pmove.gravity = sv_gravity->value;
	pm.s = client->ps.pmove;

	for (i = 0; i < 3; i++) {
		pm.s.origin[i] = COORD2SHORT(ent->s.origin[i]);
		pm.s.velocity[i] = COORD2SHORT(ent->velocity[i]);
	}

	if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s))) {
		pm.snapinitial = true;
		//      gi.dprintf ("pmove changed!\n");
	}

	pm.cmd = *ucmd;

	// walk slower if in inventory (not a problem since we disable prediction anyway
	if (client->inv_open)
	{
		float dist;
		vec3_t move;
		VectorSet(move, pm.cmd.forwardmove, pm.cmd.sidemove, pm.cmd.upmove);
		if (move[2] > 0) // no jumps! we're inventorying here
			move[2] = 0;

		dist = VectorNormalize(move);
		if (dist > 150)
			dist = 150;
		VectorScale(move, dist, move);

		pm.cmd.forwardmove = move[0];
		pm.cmd.sidemove = move[1];
		pm.cmd.upmove = move[2];
	}
	//

	pm.trace = PM_trace;    // adds default parms
	pm.pointcontents = gi.pointcontents;

	// perform a pmove
	gi.Pmove(&pm);

	for (i = 0; i < 3; i++) {
		ent->s.origin[i] = SHORT2COORD(pm.s.origin[i]);
		ent->velocity[i] = SHORT2COORD(pm.s.velocity[i]);
	}

	VectorCopy(pm.mins, ent->mins);
	VectorCopy(pm.maxs, ent->maxs);

	/*
	client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
	client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
	client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);
	*/

	if (~client->ps.pmove.pm_flags & pm.s.pm_flags & PMF_JUMP_HELD && pm.waterlevel == 0) { // jump sound
		//gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
		//PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
	}

	// save results of pmove
	client->ps.pmove = pm.s;
	client->old_pmove = pm.s;

	ent->viewheight = pm.viewheight;
	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;
	ent->groundentity = pm.groundentity;
	if (pm.groundentity)
		ent->groundentity_linkcount = pm.groundentity->linkcount;

	if (ent->deadflag) {
		client->ps.viewangles[ROLL] = 40;
		client->ps.viewangles[PITCH] = -15;
	}
	else {
		VectorCopy(pm.viewangles, client->v_angle);
		VectorCopy(pm.viewangles, client->ps.viewangles);
	}

	// decide if prediction should stay on
	client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
	if (client->inv_open)
	{
		client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
	}
	//

	gi.linkentity(ent);
}

/*
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
void PutClientInServer(edict_t *ent)
{
	gclient_t *client = ent->client;

	if (!client)
		return;

	ent->s.modelindex = gi.modelindex("models/null.md2");
	client->ps.gunindex = gi.modelindex("models/weapons/v_pickaxe.md2");
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the game.  This will happen every level load.
============
*/
void ClientBegin(edict_t *ent)
{
	int     i;

	ent->client = game.clients + (ent - g_edicts - 1);

	// fix q2pro defaulting to maxpackets 30
	gi.WriteByte(svc_stufftext);
	gi.WriteString("cl_maxpackets 60\n");
	gi.unicast(ent, true);
	//

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent->inuse == true) {
		// the client has cleared the client side viewangles upon
		// connecting to the server, which is different than the
		// state when the game is saved, so we need to compensate
		// with deltaangles
		for (i = 0; i < 3; i++)
			ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->client->ps.viewangles[i]);
	}
	else {
		// a spawn point will completely reinitialize the entity
		// except for the persistant data that was initialized at
		// ClientConnect() time
		G_InitEdict(ent);
		ent->classname = "player";
		PutClientInServer(ent);
	}

	// send effect if in a multiplayer game
	if (game.maxclients > 1) {
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_LOGIN);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		gi.bprintf(PRINT_HIGH, "%s entered the game\n", ent->client->pers.netname);
	}

	// make sure all view stuff is valid
	ClientEndServerFrame(ent);
}

/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
void ClientBeginServerFrame(edict_t *ent)
{
	gclient_t *client = ent->client;

	if (!client)
		return;

}

/*
=================
ClientEndServerFrame

Called for each player at the end of the server frame
and right after spawning
=================
*/
#define CH_DEFAULT		100
#define CH_INTERACT		70
#define CH_PICKUP		20
#define CH_INVENTORY	5

void ClientEndServerFrame(edict_t *ent)
{
	gclient_t *client = ent->client;

	if (!client)
		return;
	
	client->ps.fov = client->pers.fov;
	VectorSet(client->ps.viewoffset, 0, 0, ent->viewheight);

	// gun model
	client->ps.gunindex = gi.modelindex("models/weapons/v_pickaxe.md2");
	if (ent->client->inv_open)
	{
		client->ps.gunindex = 0;
	}


	// set gun position
	if (client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		VectorSet(client->ps.gunoffset, 0, 0, -2);
	}
	else
	{
		VectorSet(client->ps.gunoffset, 0, 0, 0);
	}

	vec3_t forward, right, up;
	AngleVectors(client->ps.viewangles, forward, right, up);
	VectorMA(client->ps.gunoffset, -1, forward, client->ps.gunoffset);
	//

	// crosshair colors
	if (ent->client->inv_open)
	{
		ent->client->ps.stats[STAT_HEALTH] = CH_INVENTORY;
	}
	else
	{
		ent->client->ps.stats[STAT_HEALTH] = CH_DEFAULT;
	}
	//

	// generate detail queue
	D_GenerateQueue(ent);
	client->detail_current = client->detail_queue; // reset for our reserved ents to loop through
}


