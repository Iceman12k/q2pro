#include "../g_local.h"

extern huntweapon_t t_dusters;

#define DUSTER_RANGE			82
#define DUSTER_DAMAGE			77
#define DUSTER_MONSTERDAMAGE	21

static void T_Dusters_Draw(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	client->hunt_attack_finished = client->hunt_client_time + 0.2;
	client->hunt_busy_finished = client->hunt_client_time + 0.1;
}

static void T_Dusters_Frame(edict_t *ent, usercmd_t *ucmd)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;
	
	vec3_t forward, start;

	client->hunt_weapon_flags &= ~HUNT_WEPFLAGS_RELOAD;

	if (client->hunt_weapon_anim == WEP_ANIM_RELOAD1)
	{
		client->hunt_attack_finished = client->hunt_client_time + 1;
		if (!(ucmd->buttons & BUTTON_ATTACK))
		{
			Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_FIRE2);
			client->hunt_attack_finished = client->hunt_client_time + 0.4;
			client->hunt_busy_finished = client->hunt_client_time + 0.2;

			AngleVectors(client->v_angle, forward, NULL, NULL);
			VectorCopy(ent->s.origin, start);
			start[2] += ent->viewheight;
			Hunt_FireTrace(ent, start, forward, 0, DUSTER_DAMAGE, DUSTER_MONSTERDAMAGE, DUSTER_RANGE, HDAMAGETYPE_BLUNT, 0, 0);

			if (!Hunt_HunterHasTrait(ent, &trait_silentkiller))
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 0.4, ATTN_IDLE, 0);
				PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
			}

			char buf[128];
			Q_snprintf(buf, sizeof(buf), "weapons/fists/fists_miss%i.wav", (int)(random() * 4) + 1);
			gi.sound(ent, CHAN_WEAPON, gi.soundindex(buf), 1, ATTN_GUN_MECHANICAL, 0);
		}
	}

	if (client->hunt_client_time < client->hunt_attack_finished)
		return;

	if (ucmd->buttons & BUTTON_ATTACK)
	{
		Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_FIRE1);
		client->hunt_attack_finished = client->hunt_client_time + 1;
	}
}

void T_Dusters_Init(void)
{
	huntweapon_t *wep = &t_dusters;
	wanim_t *anim;

	gi.soundindex("weapons/fists/fists_miss1.wav");
	gi.soundindex("weapons/fists/fists_miss2.wav");
	gi.soundindex("weapons/fists/fists_miss3.wav");
	gi.soundindex("weapons/fists/fists_miss4.wav");

	wep->view_offset[0] = -3;
	wep->view_offset[1] = -1;
	wep->view_offset[2] = -2;

	wep->nosprintanim = true;
	wep->maxammo = 0;
	wep->ammotype = 0;
	wep->framefunc = T_Dusters_Frame;

	// draw
	anim = &wep->animations[WEP_ANIM_DRAW];
	anim->time_end = 0.28;
	anim->anim_next = WEP_ANIM_IDLE;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 1, 3, 0.098);
	Hunt_AddWeaponAnimationEvent(anim, 0, 0, T_Dusters_Draw, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0, 0, Hunt_DisableCrosshair, 0);

	// putaway
	anim = &wep->animations[WEP_ANIM_PUTAWAY];
	anim->time_end = 0.2;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 47, 3, 0.098);

	// idle
	anim = &wep->animations[WEP_ANIM_IDLE];
	anim->time_end = 3;
	anim->loop = true;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 13, 30, 0.098);
	Hunt_AddWeaponAnimationEvent(anim, 0, 0, Hunt_DisableCrosshair, 0);


	// cock
	anim = &wep->animations[WEP_ANIM_FIRE1];
	anim->time_end = 0.198;
	anim->anim_next = WEP_ANIM_RELOAD1;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 4, 2, 0.098);

	// hold
	anim = &wep->animations[WEP_ANIM_RELOAD1];
	anim->time_end = 0.05;
	anim->loop = true;
	Hunt_AddWeaponAnimationEvent(anim, 0, 6, NULL, 0);

	// punch
	anim = &wep->animations[WEP_ANIM_FIRE2];
	anim->time_end = 0.48;
	anim->anim_next = WEP_ANIM_IDLE;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 8, 4, 0.098);
}



