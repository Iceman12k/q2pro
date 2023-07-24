#include "g_local.h"

#define H_WEAPON(id, wmodel, hmodel, icon)	\
huntweapon_t id = {						\
	#id,								\
	wmodel,								\
	hmodel,								\
	icon,								\
	1									\
};
WEAPON_LIST
#undef H_WEAPON

#define HITBOX_HEAD		0.2
#define HITBOX_TORSO	0.3
#define HITBOX_STOMACH	0.2
#define HITBOX_LEGS		0.3

#define MULT_HEAD		2
#define MULT_TORSO		1
#define MULT_STOMACH	0.7
#define MULT_LEGS		0.4

void Hunt_T_Damage(edict_t *targ, edict_t *inflictor, edict_t *attacker, const vec3_t dir, vec3_t point, const vec3_t normal, int damage, int knockback, int dflags, int dtype, int mod)
{
	if (targ->hunt_damagemults[dtype])
	{
		damage *= targ->hunt_damagemults[dtype];
	}

	if (targ->client)
	{
		if (dtype == HDAMAGETYPE_POISON)
		{
			int length = (min(damage / 10, 10) / FRAMETIME) + 1;
			targ->client->hunt_poison_framenum = max(level.framenum + length, targ->client->hunt_poison_framenum + length);
			targ->client->hunt_poison_framenum = min(targ->client->hunt_poison_framenum, level.framenum + (30 / FRAMETIME));
			targ->client->hunt_poison_framenum = max(targ->client->hunt_poison_framenum, level.framenum + (5 / FRAMETIME));
		}
	}

	T_Damage(targ, inflictor, attacker, dir, point, normal, damage, knockback, dflags, mod);
}

void Hunt_FireTrace(edict_t *self, vec3_t start, vec3_t aimdir, float spread, float damage, float monster_damage, float range, float damagetype, float falloff_start, float falloff_factor)
{
	trace_t     tr;
	vec3_t      dir;
	vec3_t      forward, right, up;
	vec3_t      end;
	float       r;
	float       u;
	vec3_t      water_start;
	bool        water = false;
	int         content_mask = MASK_HUNTSHOT | MASK_WATER;
	int			mod = MOD_BLASTER;
	float		damage_original = damage;

	tr = gi.trace(self->s.origin, NULL, NULL, start, self, MASK_HUNTSHOT);
	if (!(tr.fraction < 1.0f)) {
		vectoangles(aimdir, dir);
		AngleVectors(dir, forward, right, up);

		r = crandom() * spread;
		u = crandom() * spread;
		VectorMA(start, range, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);

		if (gi.pointcontents(start) & MASK_WATER) {
			water = true;
			VectorCopy(start, water_start);
			content_mask &= ~MASK_WATER;
		}

		tr = gi.trace(start, NULL, NULL, end, self, content_mask);

		// see if we hit water
		if (tr.contents & MASK_WATER) {
			int     color;

			water = true;
			VectorCopy(tr.endpos, water_start);

			if (!VectorCompare(start, tr.endpos)) {
				if (tr.contents & CONTENTS_WATER) {
					if (strcmp(tr.surface->name, "*brwater") == 0)
						color = SPLASH_BROWN_WATER;
					else
						color = SPLASH_BLUE_WATER;
				}
				else if (tr.contents & CONTENTS_SLIME)
					color = SPLASH_SLIME;
				else if (tr.contents & CONTENTS_LAVA)
					color = SPLASH_LAVA;
				else
					color = SPLASH_UNKNOWN;

				if (color != SPLASH_UNKNOWN) {
					gi.WriteByte(svc_temp_entity);
					gi.WriteByte(TE_SPLASH);
					gi.WriteByte(8);
					gi.WritePosition(tr.endpos);
					gi.WriteDir(tr.plane.normal);
					gi.WriteByte(color);
					gi.multicast(tr.endpos, MULTICAST_PVS);
				}

				// change bullet's course when it enters water
				VectorSubtract(end, start, dir);
				vectoangles(dir, dir);
				AngleVectors(dir, forward, right, up);
				r = crandom() * spread * 2;
				u = crandom() * spread * 2;
				VectorMA(water_start, 8192, forward, end);
				VectorMA(end, r, right, end);
				VectorMA(end, u, up, end);
			}

			// re-trace ignoring water this time
			tr = gi.trace(water_start, NULL, NULL, end, self, MASK_HUNTSHOT);
		}
	}

	// send gun puff / flash
	if (!((tr.surface) && (tr.surface->flags & SURF_SKY))) {
		if (tr.fraction < 1.0f) {
			if (tr.ent->takedamage) {
				float dmg_range = tr.fraction * 8192;
				dmg_range -= falloff_start;
				damage -= (dmg_range * falloff_factor);
				damage = max(0, damage);

				if (tr.ent->svflags & SVF_MONSTER)
				{
					damage = monster_damage * (damage / damage_original);
				}

				if (tr.ent->hunt_hitboxstyle <= 0)
				{
					damage *= MULT_TORSO;
				}
				else
				{
					float frac = tr.endpos[2] - tr.ent->absmax[2];
					frac /= -(tr.ent->absmax[2] - tr.ent->absmin[2]);
					

					if (frac < HITBOX_HEAD)
					{
						damage *= MULT_HEAD;
						//Com_Printf("MULT_HEAD\n");
					}
					else if ((frac - (HITBOX_HEAD)) < HITBOX_TORSO)
					{
						damage *= MULT_TORSO;
						//Com_Printf("MULT_TORSO\n");
					}
					else if ((frac - (HITBOX_HEAD + HITBOX_TORSO)) < HITBOX_STOMACH)
					{
						damage *= MULT_STOMACH;
						//Com_Printf("MULT_STOMACH\n");
					}
					else
					{
						damage *= MULT_LEGS;
						//Com_Printf("MULT_LEGS\n");
					}

					//Com_Printf("dmg: %g\n", damage);
				}

				Hunt_T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, 7, DAMAGE_BULLET, damagetype, mod);
			}
			else {
				if (strncmp(tr.surface->name, "sky", 3) != 0) {
					gi.WriteByte(svc_temp_entity);
					gi.WriteByte(TE_GUNSHOT);
					gi.WritePosition(tr.endpos);
					gi.WriteDir(tr.plane.normal);
					gi.multicast(tr.endpos, MULTICAST_PVS);

					if (self->client)
						PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
				}
			}
		}
	}

	// if went through water, determine where the end and make a bubble trail
	if (water) {
		vec3_t  pos;

		VectorSubtract(tr.endpos, water_start, dir);
		VectorNormalize(dir);
		VectorMA(tr.endpos, -2, dir, pos);
		if (gi.pointcontents(pos) & MASK_WATER)
			VectorCopy(pos, tr.endpos);
		else
			tr = gi.trace(pos, NULL, NULL, water_start, tr.ent, MASK_WATER);

		VectorAdd(water_start, tr.endpos, pos);
		VectorScale(pos, 0.5f, pos);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BUBBLETRAIL);
		gi.WritePosition(water_start);
		gi.WritePosition(tr.endpos);
		gi.multicast(pos, MULTICAST_PVS);
	}
}

void Hunt_FireBullet(edict_t *self, vec3_t start, vec3_t aimdir, float spread, float damage, float monster_damage, float falloff_start, float falloff_factor)
{
	PlayerNoise(self, self->s.origin, PNOISE_WEAPON);
	Hunt_FireTrace(self, start, aimdir, spread, damage, monster_damage, 8192, HDAMAGETYPE_BULLET, falloff_start, falloff_factor);
}

void Hunt_AddWeaponAnimationEvent(wanim_t *anim, float timestamp, int frame, void(*func)(edict_t *ent), int sound)
{
	animevent_t *new_ev = gi.TagMalloc(sizeof(animevent_t), TAG_LEVEL);
	new_ev->time = timestamp;
	new_ev->frame = frame;
	new_ev->func = func;
	new_ev->sound = sound;

	for (animevent_t *ev = anim->events; ev; ev = ev->next)
	{
		if (!ev->next)
		{
			ev->next = new_ev;
			return;
		}
		else if (ev->next->time > timestamp)
		{
			new_ev->next = ev->next;
			ev->next = new_ev;
			return;
		}
	}

	if (anim->events)
		new_ev->next = anim->events;
	anim->events = new_ev;
}

void Hunt_AddWeaponAnimationPeriod(wanim_t *anim, float timestamp_start, int frame_start, int frames, float framerate)
{
	for (int i = 0; i < frames; i++)
	{
		Hunt_AddWeaponAnimationEvent(anim, timestamp_start, frame_start, NULL, 0);
		timestamp_start += framerate;
		frame_start++;
	}
}

void Hunt_EnableCrosshair(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	client->hunt_forcecrosshair = 1;
}

void Hunt_DisableCrosshair(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	client->hunt_forcecrosshair = 0 | HUNT_FORCECROSSHAIR;
}

void Hunt_DotCrosshair(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	client->hunt_forcecrosshair = 3 | HUNT_FORCECROSSHAIR;
}

void Hunt_CrossbowCrosshair(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	client->hunt_forcecrosshair = 2 | HUNT_FORCECROSSHAIR;
}

void Hunt_AdsIn(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	client->hunt_weapon_flags |= HUNT_WEPFLAGS_ADS;
}

void Hunt_AdsOut(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	client->hunt_weapon_flags &= ~HUNT_WEPFLAGS_ADS;
}

void Hunt_AdsLateCheck(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	if (client->hunt_weapon_flags & HUNT_WEPFLAGS_ADSWANTED)
		return;

	if (client->hunt_weapon_anim >= WEP_ANIM_ADSIN && client->hunt_weapon_anim <= WEP_ANIM_ADS)
	{
		Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_ADSOUT);
	}
}

void Hunt_AdsLateCheckIdle(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	if (client->hunt_weapon_flags & HUNT_WEPFLAGS_ADSWANTED)
		return;

	if (client->hunt_weapon_anim >= WEP_ANIM_ADSIN && client->hunt_weapon_anim <= WEP_ANIM_ADS)
	{
		Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_IDLE);
	}
}


void Hunt_WeaponInit(void)
{
#define H_WEAPON(id, wmodel, hmodel)	\
		gi.modelindex(wmodel)				\
		gi.modelindex(hmodel)				\
	WEAPON_LIST
#undef H_WEAPON

	W_PeaceMaker_Init();
	W_Springfield_Init();
	W_Winfield_Init();
	W_M1Garand_Init();
	T_Dusters_Init();
	T_Knife_Init();

	gi.imageindex("crosshair_0");
	gi.imageindex("crosshair_1");
	gi.imageindex("crosshair_2");
	gi.imageindex("crosshair_3");
	gi.imageindex("crosshair_4");
	gi.imageindex("crosshair_c0");
	gi.imageindex("crosshair_dot");

	gi.imageindex("t_a");
	gi.imageindex("t_r");
	gi.imageindex("t_h");
	gi.imageindex("t_w");
}


void Hunt_StartAnimation(edict_t *ent, huntweapon_t *wep, int anim)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	client->hunt_weapon_animtime = 0;
	client->hunt_weapon_anim = anim;
	for (animevent_t *ev = wep->animations[anim].events; ev; ev = ev->next)
	{
		if (ev->time > 0)
			continue;
		if (ev->frame)
			client->hunt_weapon_animframe = ev->frame;
		if (ev->func)
			ev->func(ent);
		if (ev->sound)
			gi.sound(ent, CHAN_AUTO, ev->sound, 1, ATTN_GUN_MECHANICAL, 0);
	}
}


void Hunt_SetWeapon(edict_t *ent, huntweaponinstance_t *wep)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	client->hunt_invselected = wep;
	client->hunt_forcecrosshair = 0;
	//client->hunt_weapon_animtime = -0.001;
	//client->hunt_weapon_anim = WEP_ANIM_DRAW;
	Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_DRAW);
}


void Hunt_AdsFrame(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	int want_ads = (client->hunt_weapon_flags & HUNT_WEPFLAGS_ADSWANTED);
	if (client->ps.pmove.pm_flags & PMF_HUNT_SPRINT)
	{
		want_ads = false;
		if (!(client->lastpmflags & PMF_HUNT_SPRINT) && (client->hunt_weapon_flags & HUNT_WEPFLAGS_ADSTOGGLE))
			client->hunt_weapon_flags &= ~HUNT_WEPFLAGS_ADSWANTED;
	}

	if (client->hunt_client_time >= client->hunt_busy_finished)
	{
		if (want_ads && !(client->hunt_weapon_flags & HUNT_WEPFLAGS_ADS) && (client->hunt_weapon_anim != WEP_ANIM_ADSIN) && (client->hunt_weapon_anim != WEP_ANIM_ADSFIRE && client->hunt_weapon_anim != WEP_ANIM_ADSFIRE2))
			Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_ADSIN);
		else if (!want_ads && (client->hunt_weapon_flags & HUNT_WEPFLAGS_ADS) && (client->hunt_weapon_anim != WEP_ANIM_ADSOUT) && (client->hunt_weapon_anim != WEP_ANIM_ADSFIRE && client->hunt_weapon_anim != WEP_ANIM_ADSFIRE2))
			Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_ADSOUT);
	}
}


void Hunt_DryFire(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/peacemaker/peacemaker_empty.wav"), 1, ATTN_NORM, 0);
	
	if ((client->hunt_weapon_anim >= WEP_ANIM_ADSIN) && (client->hunt_weapon_anim <= WEP_ANIM_ADS))
		client->kick_angles[PITCH] += 1;

	client->hunt_attack_finished = client->hunt_client_time + 0.15;
	client->hunt_busy_finished = client->hunt_client_time + 0.15;
}


void Hunt_SetWeaponToWanted(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	if (client->hunt_invslots[client->hunt_invwanted - 1].def)
	{
		Hunt_SetWeapon(ent, &client->hunt_invslots[client->hunt_invwanted - 1]);
		client->hunt_invwanted = 0;
	}
}


void Hunt_WeaponPhysics(edict_t *ent, usercmd_t *ucmd)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	if (ent->deadflag != DEAD_NO)
	{
		client->ps.gunindex = 0;
		return;
	}

	if (client->hunt_weapon_flags & HUNT_WEPFLAGS_DS)
	{
		client->hunt_busy_finished = client->hunt_client_time + 0.2;
		client->hunt_poison_screeneffect = false;

		if (client->ps.gunframe < 3 && client->hunt_attack_finished < client->hunt_client_time)
		{
			client->hunt_attack_finished = client->hunt_client_time + 0.08;
			client->ps.gunframe++;

			if (client->ps.gunframe == 1)
			{
				// darken lighting
				gi.WriteByte(svc_configstring);
				gi.WriteShort(CS_LIGHTS + 0);
				gi.WriteString("h");
				gi.unicast(ent, true);
			}
			if (client->ps.gunframe == 2)
			{
				// darken lighting
				gi.WriteByte(svc_configstring);
				gi.WriteShort(CS_LIGHTS + 0);
				gi.WriteString("e");
				gi.unicast(ent, true);

				gi.WriteByte(svc_stufftext);
				gi.WriteString("sky black\n");
				gi.unicast(ent, true);
			}
			else if (client->ps.gunframe == 3)
			{
				// darken lighting
				gi.WriteByte(svc_configstring);
				gi.WriteShort(CS_LIGHTS + 0);
				gi.WriteString("a");
				gi.unicast(ent, true);
			}
		}

		if (!(client->hunt_weapon_flags & HUNT_WEPFLAGS_DSWANTED))
		{
			client->hunt_weapon_flags &= ~HUNT_WEPFLAGS_DS;
			client->hunt_weapon_flags &= ~HUNT_WEPFLAGS_ADS;

			Hunt_SetWeapon(ent, client->hunt_invselected);

			// fix lighting
			gi.WriteByte(svc_configstring);
			gi.WriteShort(CS_LIGHTS + 0);
			gi.WriteString("m");
			gi.unicast(ent, true);

			gi.WriteByte(svc_stufftext);
			gi.WriteString("sky \"\"\n");
			gi.unicast(ent, true);
		}
		return;
	}
	else if (client->hunt_weapon_flags & HUNT_WEPFLAGS_DSWANTED)
	{
		if (client->hunt_client_time > client->hunt_busy_finished)
		{
			client->hunt_weapon_flags |= HUNT_WEPFLAGS_DS;

			client->ps.gunindex = gi.modelindex("models/weapons/v_fists/tris.md2");
			client->ps.gunframe = 0;
			client->hunt_attack_finished = client->hunt_client_time + 0.08;
			return;
		}
	}

	if (client->hunt_invwanted && client->hunt_weapon_anim != WEP_ANIM_PUTAWAY && (client->hunt_invselected != &client->hunt_invslots[client->hunt_invwanted - 1]))
	{
		if (client->hunt_weapon_flags & HUNT_WEPFLAGS_ADS)
		{
			client->hunt_weapon_flags &= ~HUNT_WEPFLAGS_ADSWANTED;
		}
		else if (client->hunt_busy_finished < client->hunt_client_time)
		{
			Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_PUTAWAY);
		}
	}

#if 0
	if (!client->hunt_invselected->def)
		return;
#else
	if (!client->hunt_invselected->def)
		return;
#endif

	client->hunt_weapon_inaccuracy = VectorLength(ent->velocity) / 7;
	if (client->hunt_weapon_flags & HUNT_WEPFLAGS_ADS)
	{
		client->hunt_weapon_inaccuracy = 0;

		if (client->hunt_invselected->def->adsfov > 0)
			client->ps.fov = client->hunt_invselected->def->adsfov;
		else if (client->hunt_invselected->def->adsfov == 0)
			client->ps.fov = 85;
		else
			client->ps.fov = client->pers.desired_fov;
	}
	else
	{
		if (client->ps.pmove.pm_flags & PMF_DUCKED)
			client->hunt_weapon_inaccuracy += 15;
		else
			client->hunt_weapon_inaccuracy += 30;

		client->ps.fov = client->pers.desired_fov;
		if (client->ps.fov < 90)
			client->ps.fov = 90;
		else if (client->ps.fov > 140)
			client->ps.fov = 140;
	}

	if (!(client->ps.pmove.pm_flags & PMF_ON_GROUND))
	{
		client->hunt_weapon_inaccuracy += 15;
	}
	else
	{
		client->hunt_weapon_inaccuracy *= 0.667;
	}

	if (client->hunt_weapon_recoil > 0)
	{
		client->hunt_weapon_recoil = bound(client->hunt_weapon_recoil - (float)ucmd->msec / 80, 0, 20);
	}

	client->hunt_weapon_inaccuracy += client->hunt_weapon_recoil * 1.2;

	VectorAdd(client->v_angle, client->ps.kick_angles, client->hunt_weapon_angle);
	client->hunt_weapon_angle[PITCH] -= client->hunt_weapon_recoil * 0.3;


	float old_animtime = client->hunt_weapon_animtime;
	client->hunt_weapon_animtime += ((float)ucmd->msec) / 1000;

	wanim_t *anim = &client->hunt_invselected->def->animations[client->hunt_weapon_anim];
	for (animevent_t *ev = anim->events; ev; ev = ev->next)
	{
		if (old_animtime >= ev->time)
			continue;

		if (client->hunt_weapon_animtime < ev->time)
			continue;

		if (ev->frame)
			client->hunt_weapon_animframe = ev->frame;
		if (ev->func)
			ev->func(ent);
		if (ev->sound)
			gi.sound(ent, CHAN_AUTO, ev->sound, 1, ATTN_GUN_MECHANICAL, 0);
	}

	if (client->hunt_weapon_animtime > anim->time_end)
	{
		client->hunt_weapon_animtime = 0;
		if (client->hunt_weapon_anim == WEP_ANIM_PUTAWAY) // putaway has special behavior
		{
			Hunt_SetWeaponToWanted(ent);
		}
		else
		{
			if (!anim->loop)
			{
				Hunt_StartAnimation(ent, client->hunt_invselected->def, anim->anim_next);
			}
		}
	}

	if (client->hunt_invselected->def->framefunc)
	{
		client->hunt_invselected->def->framefunc(ent, ucmd);
	}

	client->ps.gunframe = client->hunt_weapon_animframe;
	client->ps.gunindex = gi.modelindex(client->hunt_invselected->def->viewmodel);

	if (client->hunt_invselected->def->adsmodel && client->hunt_weapon_flags & HUNT_WEPFLAGS_ADS)
	{
		client->ps.gunindex = gi.modelindex(client->hunt_invselected->def->adsmodel);
	}
}