
#include "g_local.h"

#define H_TRAIT(id, name, icon) hunttrait_t id = {name, icon};
TRAIT_LIST
#undef H_TRAIT

int Hunt_HunterHasTrait(edict_t *ent, hunttrait_t *trait)
{
	// do a loop to check for trait
	return true; // debug just return true for all traits

	return false;
}

void Hunt_Ammo_Render(edict_t *ent, char *string, char *entry, int x, int y)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	if (!client->hunt_invselected->def)
		return;

	if (!client->hunt_invselected->def->maxammo)
		return;

	const int spacing = 22;
	int maxammo = client->hunt_invselected->def->maxammo;
	x -= spacing * maxammo;

	for (int i = 0; i < maxammo; i++)
	{
		const char *am;
		am = "am_11";
		if (i >= client->hunt_invselected->ammo)
			am = "am_10";

		Q_snprintf(entry, 1024, "xr %i yb %i picn %s ", x + (i * spacing), y, am);
		Q_strlcat(string, entry, 1400);
	}

	x -= 24;
	Q_snprintf(entry, 1024, "xr %i yb %i picn %s ", x, y + 8, "am_m");
	Q_strlcat(string, entry, 1400);

	int ammo_count = client->hunt_invammo[client->hunt_invselected->def->ammotype];
	if (ammo_count >= 100)
		x -= 8;
	else if (ammo_count >= 10)
		x -= 4;
	Q_snprintf(entry, 1024, "xr %i yb %i string %i ", x, y - 4, ammo_count);
	Q_strlcat(string, entry, 1400);
}

void Hunt_HealthBar_Render(edict_t *ent, char *string, char *entry, int x, int y)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	huntbar_t *bar = client->hunt_healthbars;
	if (!bar)
		return;

	if (client->hunt_client_time > client->hunt_healthbar_hide)
		return;
	
	for (huntbar_t *bar = client->hunt_healthbars; bar; bar = bar->next)
	{
		int amount = bar->amount;
		if (amount && amount < 5)
			amount = 5;
		else
			amount = floor(amount / 5) * 5;

		char barname[128];
		if (level.framenum < client->hunt_poison_framenum)
		{
			Q_snprintf(barname, sizeof(barname), "pbar_%i_%02i", bar->type, amount);
		}
		else
		{
			Q_snprintf(barname, sizeof(barname), "bar_%i_%02i", bar->type, amount);
		}

		Q_snprintf(entry, 1024, "xv %i yb %i picn %s ", x, y, barname);
		Q_strlcat(string, entry, 1400);

		if (bar->type)
			x += 140;
		else
			x += 73;
	}
}

void Hunt_Crosshair_Render(edict_t *ent, char *string, char *entry, int x, int y)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	if (ent->deadflag != DEAD_NO)
		return;

	if (!client->hunt_invselected->def)
		return;

	int crosshair_setting;
	crosshair_setting = client->hunt_invselected->def->crosshair;
	if (client->hunt_forcecrosshair & HUNT_FORCECROSSHAIR)
		crosshair_setting = client->hunt_forcecrosshair & (HUNT_FORCECROSSHAIR - 1);
	else if (client->ps.pmove.pm_flags & PMF_HUNT_SPRINT)
		crosshair_setting = 0;

	const char *xhair;
	int offs;
	offs = 0;

	if (crosshair_setting == 1)
	{
		if (!(client->ps.pmove.pm_flags & PMF_ON_GROUND))
		{
			client->hunt_weapon_inaccuracy += 25;
		}

		if (client->hunt_weapon_inaccuracy < 5)
		{
			offs = 16;
			xhair = "crosshair_0";
		}
		else if (client->hunt_weapon_inaccuracy < 20)
		{
			offs = 16;
			xhair = "crosshair_1";
		}
		else if (client->hunt_weapon_inaccuracy < 30)
		{
			offs = 16;
			xhair = "crosshair_2";
		}
		else if (client->hunt_weapon_inaccuracy < 50)
		{
			offs = 32;
			xhair = "crosshair_3";
		}
		else if (client->hunt_weapon_inaccuracy < 70)
		{
			offs = 32;
			xhair = "crosshair_4";
		}
		else
		{
			offs = 64;
			xhair = "crosshair_5";
		}
	}
	else if (crosshair_setting == 2)
	{
		offs = 16;
		xhair = "crosshair_c0";
	}
	else if (crosshair_setting == 3)
	{
		offs = 4;
		xhair = "crosshair_dot";
	}

	if (offs)
	{
		Q_snprintf(entry, 1024, "xv %i yv %i picn %s ", 160 - offs, 120 - offs, xhair);
		Q_strlcat(string, entry, 1400);
	}
}

void Hunt_Healthbar_Simulate(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	if (level.framenum <= client->hunt_poison_framenum)
	{
		client->hunt_regen_framenum = level.framenum + 5;
		if (!client->hunt_poison_screeneffect)
		{
			client->hunt_poison_screeneffect = true;
			// darken lighting
			gi.WriteByte(svc_configstring);
			gi.WriteShort(CS_LIGHTS + 0);
			gi.WriteString("bbccddcc");
			gi.unicast(ent, true);
		}
	}
	else if (client->hunt_poison_screeneffect)
	{
		client->hunt_poison_screeneffect = false;
		// fix lighting
		gi.WriteByte(svc_configstring);
		gi.WriteShort(CS_LIGHTS + 0);
		gi.WriteString("m");
		gi.unicast(ent, true);
	}

	huntbar_t *bar = client->hunt_healthbars;
	if (!bar)
		return;

	int health = ent->health;
	for (huntbar_t *bar = client->hunt_healthbars; bar; bar = bar->next)
	{
		int amount;
		amount = health;
		if (bar->type && amount >= 50)
			amount = 50;
		else if (!bar->type && amount >= 25)
			amount = 25;
		else if (amount <= 0)
			amount = 0;
		else if (amount && health == amount) // we didn't get clamped, do some regen?
		{
			if (level.framenum >= client->hunt_regen_framenum)
			{
				ent->health++;
				//health = amount++;
				client->hunt_regen_framenum = level.framenum + 2;
			}
		}

		health -= amount;
		bar->amount = amount;
	}

	if (ent->health != ent->max_health)
	{
		client->hunt_healthbar_hide = client->hunt_client_time + 3;
	}

	if (health)
	{
		ent->health = (ent->health - health); // clamp our entity's health
	}
}

void Hunt_HealthBar_Alloc(edict_t *ent, int type)
{
	gclient_t *client;

	client = ent->client;
	if (!client)
		return;

	huntbar_t *newbar = gi.TagMalloc(sizeof(huntbar_t), TAG_LEVEL);

	if (client->hunt_healthbars)
	{
		huntbar_t *bar, *hold;
		for (bar = client->hunt_healthbars; bar; hold = bar, bar = bar->next);
		hold->next = newbar;
	}
	else
	{
		client->hunt_healthbars = newbar;
	}

	newbar->type = type;
}

static vec3_t  forward, right, up;
void Hunt_Viewmodel(edict_t *ent)
{
	int i;
	float delta;
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	AngleVectors(client->v_angle, forward, right, up);

	if (client->hunt_weapon_flags & HUNT_WEPFLAGS_ADS)
	{
		// in-air wobble
		if (!(client->ps.pmove.pm_flags & PMF_ON_GROUND))
		{
			for (i = 0; i < 3; i++) {
				client->ps.gunoffset[i] += up[i] * sin(level.time * 6);
				client->ps.gunoffset[i] += right[i] * sin(level.time * 4);
			}
		}
	}
	else
	{
		if (client->ps.pmove.pm_flags & PMF_HUNT_SPRINT)
		{
			if (!(client->hunt_invselected->def->nosprintanim))
			{
				for (i = 0; i < 3; i++) {
					client->ps.gunoffset[i] += right[i] * 3;
				}

				client->ps.gunoffset[2] -= 4;
				client->ps.gunangles[0] += 10;
				client->ps.gunangles[1] += 25;
			}
			else
			{
				client->ps.gunoffset[2] -= 2;
				for (i = 0; i < 3; i++) {
					client->ps.gunoffset[i] += up[i] * -2;
				}
			}
		}
		else if (client->ps.pmove.pm_flags & PMF_DUCKED && !(client->hunt_weapon_flags & HUNT_WEPFLAGS_DS))
		{
			if ((client->hunt_weapon_anim >= WEP_ANIM_ADSIN) && (client->hunt_weapon_anim <= WEP_ANIM_ADS))
				return;

			for (i = 0; i < 3; i++) {
				client->ps.gunoffset[i] += right[i] * -3;
				client->ps.gunoffset[i] += up[i] * 1;
			}
		}

		// weapon specific offset
		vec3_t wep_ofs;
		VectorClear(wep_ofs);
		if (client->hunt_weapon_flags & HUNT_WEPFLAGS_DS)
		{
			VectorSet(wep_ofs, -2, 4, -3);
		}
		else
		{
			if (client->hunt_invselected->def)
				VectorCopy(client->hunt_invselected->def->view_offset, wep_ofs);
		}

		for (i = 0; i < 3; i++) {
			client->ps.gunoffset[i] += wep_ofs[0] * right[i];
			client->ps.gunoffset[i] += wep_ofs[1] * up[i];
			client->ps.gunoffset[i] += wep_ofs[2] * forward[i];
		}
		//

		// in-air wobble
		if (!(client->ps.pmove.pm_flags & PMF_ON_GROUND))
		{
			for (i = 0; i < 3; i++) {
				client->ps.gunoffset[i] += up[i] * bound(-(ent->velocity[2] / 140), -4, 4);
			}

			client->ps.gunangles[PITCH] -= bound(-(ent->velocity[2] / 70), -6, 4);
			client->ps.gunangles[YAW] += sin(level.time * 6) * 2;
		}
	}


#if 0
	client->ps.gunangles[0] = 0;
	client->ps.gunangles[1] = -7 * cos(DEG2RAD(client->v_angle[0]));
	client->ps.gunangles[2] = 0;
	client->ps.gunoffset[0] = 0;
	client->ps.gunoffset[1] = 0;
	client->ps.gunoffset[2] = 0;

	for (i = 0; i < 3; i++) {
		client->ps.gunoffset[i] += right[i] * -18;
		client->ps.gunoffset[i] += up[i] * 2;
	}
#endif
}



