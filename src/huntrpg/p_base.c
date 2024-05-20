
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

	Com_Printf("%s\n", userinfo);

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

	// cleanup detail ents
	CL_DetailCleanup(ent);

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
	client->time += (float)ucmd->msec / 1000;

	// set up for pmove
	memset(&pm, 0, sizeof(pm));

	client->ps.pmove.pm_type = PM_NORMAL;
	
	if (client->inv_open)
	{
		I_Input(ent, ucmd);
	}

	client->cmd_lastbuttons = client->cmd_buttons;
	client->cmd_buttons = ucmd->buttons;

	for (i = 0; i < 3; i++) {
		client->cmd_angles[i] = SHORT2ANGLE(ucmd->angles[i]);
	}

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


	// client item physics
	if (client->hotbar_selected < INVEN_HOTBAR_START || client->hotbar_selected >= INVEN_TOTALSLOTS) // bounds check
		client->hotbar_selected = INVEN_HOTBAR_START;

	if (client->hotbar_wanted < INVEN_HOTBAR_START || client->hotbar_wanted >= INVEN_TOTALSLOTS) // bounds check
		client->hotbar_wanted = INVEN_HOTBAR_START;

	if (client->hotbar_selected != client->hotbar_wanted)
	{
		if (level.framenum >= ent->attack_finished)
		{
			client->hotbar_selected = client->hotbar_wanted;
			memset(ent->weapon_data, 0, sizeof(ent->weapon_data));
		}
	}

	item_t *item = client->inv_content[client->hotbar_selected];
	if (!item || !client->hotbar_open)
	{
		client->ps.gunindex = 0;
	}
	else
	{
		int old_ind = client->ps.gunindex;
		client->ps.gunindex = gi.modelindex(item->viewmodel);

		if (old_ind != client->ps.gunindex)
		{
			client->hotbar_raisetime = level.framenum + game.framediv;// client->time + 0.2;
		}

		if (!ent->weaponthunk)
		{
			if (item->frame)
				item->frame(ent, item, &ent->weaponthunk);
		}
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

	client->inv_content[0] = &lantern;
	client->inv_content[INVEN_HOTBAR_START] = &pickaxe;
	//client->hotbar_open = true;
	client->hotbar_selected = INVEN_HOTBAR_START;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->health = 100;

	CL_DetailCreate(ent);
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

	gi.WriteByte(svc_stufftext);
	gi.WriteString("bind mwheelup invprev\nbind mwheeldown invnext\nbind 1 invuse 1\nbind 2 invuse 2\nbind 3 invuse 3\nbind 4 invuse 4\nbind 5 invuse 5\n");
	gi.unicast(ent, true);

	gi.WriteByte(svc_stufftext);
	gi.WriteString("bind tab inventory\nbind q inv_open\n");
	gi.unicast(ent, true);

	// initialize our janky download system
	ent->client->download_cooldown = level.framenum + game.framediv;
	ent->client->download_progress = 0;
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

	// gun model
	if (FRAMESYNC)
	{
		if (ent->client->inv_open || !client->hotbar_open)
		{
			client->ps.gunindex = 0;
		}
		else
		{
			item_t *item = client->inv_content[client->hotbar_selected];
			if (item)
			{
				if (!ent->weaponthunk)
				{
					ent->weaponthunk = true;
					if (item->frame)
						item->frame(ent, item, &ent->weaponthunk);
				}
			}
		}
		ent->weaponthunk = false;
	}

	client->levelframenum++;
	client->leveltime = client->levelframenum * FRAMETIME;
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
	vec3_t forward, right, up;
	gclient_t *client = ent->client;

	if (!client)
		return;

	// sync up frame nums
	client->levelframenum = level.framenum;
	client->leveltime = level.time;
	//
	
	if (FRAMESYNC)
	{
		client->ps.fov = client->pers.fov;
		VectorSet(client->ps.viewoffset, 0, 0, ent->viewheight);

		// view kick
		VectorCopy(client->kick_angles, client->ps.kick_angles);
		VectorClear(client->kick_angles);

		// set gun position
		item_t *item = client->inv_content[client->hotbar_selected];
		if (item && client->hotbar_open) // do item endframe
		{
			if (item->endframe)
				item->endframe(ent, item);
		}

		if (client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			VectorSet(client->ps.gunoffset, 0, 0, -2);
		}
		else
		{
			VectorSet(client->ps.gunoffset, 0, 0, 0);
		}

		AngleVectors(client->ps.viewangles, forward, right, up);
		VectorMA(client->ps.gunoffset, -1, forward, client->ps.gunoffset);

		if (client->ps.gunindex == 0)
			client->ps.gunoffset[2] = -4;

		if (level.framenum <= client->hotbar_raisetime)
		{
			float frac = ((float)(client->hotbar_raisetime - level.framenum) / game.framediv);
			client->ps.gunoffset[2] -= 4 * frac;
		}
	}
	//

	// crosshair colors
	if (client->inv_open)
	{
		if (client->inv_highlighted == -1)
			client->ps.stats[STAT_HEALTH] = CH_INTERACT;
		else
			client->ps.stats[STAT_HEALTH] = CH_INVENTORY;
	}
	else
	{
		client->ps.stats[STAT_HEALTH] = CH_DEFAULT;
	}
	//

	//client->chunks_visible = ~0ULL;
	// figure out visible chunks
	{
		int current_chunk = ORG_TO_CHUNK(ent->s.origin);
		client->chunks_visible = 0ULL;
		client->chunks_visible |= 1ULL << current_chunk;
		client->chunks_visible |= (1ULL << current_chunk) << 1ULL;
		client->chunks_visible |= (1ULL << current_chunk) >> 1ULL;
		client->chunks_visible |= (1ULL << current_chunk) << 8ULL;
		client->chunks_visible |= (1ULL << current_chunk) >> 8ULL;

		// FIXME: Reki (February 10 2024): this is probably a really shit way to do this...
		// brain is too fried to figure out a proper way tho
		float ang = round(client->ps.viewangles[YAW] / 25) * 25;
		float angs[] = {ang - 50, ang - 37.5, 
					ang - 25, ang - 12.5,
					ang, ang + 12.5, ang + 25,
					ang + 37.5, ang + 50};
		for(int i = 0; i < (sizeof(angs) / sizeof(angs[0])); i++)
		{
			vec3_t vorg;
			vec3_t uvector;

			uvector[0] = cos(DEG2RAD(angs[i]));
			uvector[1] = sin(DEG2RAD(angs[i]));
			uvector[2] = 0;

			VectorMA(ent->s.origin, CHUNK_SIZE * 0.6, uvector, vorg);
			client->chunks_visible |= 1ULL << ORG_TO_CHUNK(vorg);
			VectorMA(ent->s.origin, CHUNK_SIZE * 0.9, uvector, vorg);
			client->chunks_visible |= 1ULL << ORG_TO_CHUNK(vorg);
			VectorMA(ent->s.origin, CHUNK_SIZE * 1.2, uvector, vorg);
			client->chunks_visible |= 1ULL << ORG_TO_CHUNK(vorg);
		}

		// debug
		// debug chunks
		#if 0
		Com_Printf("\n\nDetail bitboards: %i\n", __builtin_ctz(0b000010));
		for(int i = 0; i < CHUNK_HEIGHT; i++)
		{
			for(int j = 0; j < CHUNK_WIDTH; j++)
			{
				uint64_t bit = 1ULL << (j + (i << 3ULL));// + (i << 3));
				if (client->chunks_visible & bit)
				{
					Com_Printf("X");
				}
				else
				{
					Com_Printf("O");
				}
			}
			Com_Printf("\n");
		}
		#endif
		//
	}

	// generate detail queue
	if (FRAMESYNC)
	{
		//D_GenerateQueue(ent);
		Scene_Generate(ent);
	}

	// passive item flags
	client->passive_flags = 0;
	for (int i = INVEN_HOTBAR_START; i < INVEN_TOTALSLOTS; i++)
	{
		item_t *item = client->inv_content[i];
		if (!item)
			continue;

		client->passive_flags |= item->passive_flags;
	}

	// send new hud
	H_Update(ent, client);

	// environment update
	Environment_ClientUpdate(ent);

	ent->s.effects &= EF_HYPERBLASTER;
	if (client->passive_flags & PASSIVE_LIGHT)
	{
		ent->s.effects |= EF_HYPERBLASTER;
	}
	
	//AngleVectors(client->ps.viewangles, forward, right, up);
	//VectorMA(client->ps.viewoffset, -32, forward, client->ps.viewoffset);
	//VectorMA(client->ps.viewoffset, 48, up, client->ps.viewoffset);

	/*
	if (ent->client->download_progress >= 0)
	{
		if (level.framenum >= ent->client->download_cooldown)
		{
			char cmd[256];
			Q_snprintf(cmd, sizeof(cmd), "download %s\n", downloadlist[ent->client->download_progress]);
			gi.WriteByte(svc_stufftext);
			gi.WriteString(cmd);
			gi.unicast(ent, true);

			ent->client->download_progress++;
			if (ent->client->download_progress >= downloadlist_size)
				ent->client->download_progress = -1;

			ent->client->download_cooldown = level.framenum + (game.framediv * 5);
		}
	}
	*/
}


