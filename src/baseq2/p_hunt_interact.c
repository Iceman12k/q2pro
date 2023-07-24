
#include "g_local.h"

static void Int_Medkit(edict_t *ent)
{
	ent->health = min(ent->health + 100, ent->max_health);
}

huntinteractable_t int_medkit = { TIP_HEALTH, 3, Int_Medkit };



void Hunt_Interact_Render(edict_t *ent, char *string, char *entry)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	if (client->hunt_interact_busy)
	{
		if (client->hunt_interactprogress)
		{
			const char* prog[] = {
				"r_0",
				"r_1",
				"r_2",
				"r_3",
				"r_4",
				"r_5",
				"r_6",
				"r_7",
				"r_8",
			};

			int icon = client->hunt_interactprogress * 9;
			clamp(icon, 0, 8);

			Q_snprintf(entry, 1024, "xv %i yv %i picn %s ", 160 - 16, 170, prog[icon]);
			Q_strlcat(string, entry, 1400);
		}
	}

	if (!client->hunt_interactablehover)
		return;

	huntinteractable_t *interact = client->hunt_interactablehover->def;

	const char *tip;
	int offs = 0;

	if (interact->tip == TIP_AMMO)
	{
		offs = 96;
		tip = "t_a";
	}
	else if (interact->tip == TIP_HEALTH)
	{
		offs = 96;
		tip = "t_h";
	}
	else if (interact->tip == TIP_REVIVE)
	{
		offs = 96;
		tip = "t_r";
	}
	else if (interact->tip == TIP_WEAPON)
	{
		offs = 96;
		tip = "t_w";
	}

	if (offs)
	{
		Q_snprintf(entry, 1024, "xv %i yv %i picn %s ", 160 - offs, 205, tip);
		Q_strlcat(string, entry, 1400);
	}
}

#define HUNT_INTERACTRANGE	64
static void Hunt_Interact_FindInteractable(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	edict_t *closest = NULL;
	int closest_dist = 99999;
	vec3_t camera_pos;
	vec3_t camera_dir;

	VectorCopy(ent->s.origin, camera_pos);
	camera_pos[2] += ent->viewheight;
	AngleVectors(client->v_angle, camera_dir, NULL, NULL);
	VectorMA(camera_pos, HUNT_INTERACTRANGE, camera_dir, camera_dir);

	edict_t *rad = world;
	while ((rad = findradius(rad, ent->s.origin, HUNT_INTERACTRANGE * 4)) != NULL) {
		if (!rad->hunt_interact.def)
			continue;

		trace_t tr;
		vec3_t o_mins; VectorCopy(rad->mins, o_mins);
		vec3_t o_maxs; VectorCopy(rad->maxs, o_maxs);
		int o_svflags = rad->svflags;
		int o_solid = rad->solid;
		huntinteractinstance_t *intr = &rad->hunt_interact;
		
		VectorCopy(intr->mins, rad->mins);
		VectorCopy(intr->maxs, rad->maxs);
		rad->svflags = SVF_MONSTER;
		rad->solid = SOLID_BBOX;
		gi.linkentity(rad);

		tr = gi.trace(camera_pos, NULL, NULL, camera_dir, ent, CONTENTS_MONSTER);

		rad->solid = o_solid;
		rad->svflags = o_svflags;
		VectorCopy(o_mins, rad->mins);
		VectorCopy(o_maxs, rad->maxs);
		gi.linkentity(rad);

		if (tr.fraction >= 1.0 && !tr.startsolid)
			continue;
		
		if (tr.ent != rad)
			continue;

		if (tr.fraction * HUNT_INTERACTRANGE > closest_dist)
			continue;

		closest = rad;
		closest_dist = tr.fraction * HUNT_INTERACTRANGE;
	}

	client->hunt_interactablehover = NULL;
	if (closest)
		client->hunt_interactablehover = &closest->hunt_interact;
}

void Hunt_Interact_Simulate(edict_t *ent, usercmd_t *ucmd)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	Hunt_Interact_FindInteractable(ent);

	if (!client->hunt_interactablehover)
	{
		client->hunt_interact_busy = false;
		client->hunt_interactprogress = 0;
		return;
	}

	huntinteractinstance_t *interact_obj = client->hunt_interactablehover;
	huntinteractable_t *interact = interact_obj->def;

	if (ucmd->buttons & BUTTON_USE)
	{
		client->hunt_interactprogress += ((float)ucmd->msec / 1000) / interact->time;
		client->hunt_interact_busy = true;
	}
	else
	{
		client->hunt_interact_busy = false;
		client->hunt_interactprogress = 0;
	}

	if (client->hunt_interactprogress >= interact->time)
	{
		client->hunt_interact_busy = false;
		client->hunt_interactprogress = 0;
		client->hunt_interactablehover = NULL;
		if (interact->usefunc)
			interact->usefunc(ent);
	}
}





void Hunt_Inventory_Render(edict_t *ent, char *string, char *entry)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	if (client->hunt_client_time > client->hunt_inventory_hide)
		return;

	int x = -80;
	int y = -190;
	
	for (int i = 0; i < 10; i++)
	{
		huntweaponinstance_t *inv = &client->hunt_invslots[i];

		if (!inv->def)
			continue;

		if (!inv->def->invicon)
			continue;

		Q_snprintf(entry, 1024, "xv %i yb %i picn %s ", x, y, inv->def->invicon);
		Q_strlcat(string, entry, 1400);

		Q_snprintf(entry, 1024, "xv %i yb %i string %i ", x + 4, y + 4, i + 1);
		Q_strlcat(string, entry, 1400);

		if (inv == client->hunt_invselected)
		{
			if (i < 2) // big inventory slot
				Q_snprintf(entry, 1024, "xv %i yb %i picn %s ", x, y, "i_slct");
			else // small inventory slot (consumables + tools)
				Q_snprintf(entry, 1024, "xv %i yb %i picn %s ", x, y, "i_sslct");
			Q_strlcat(string, entry, 1400);
		}

		if (i < 2)
			x += 120;
		else
			x += 90;
	}
}




