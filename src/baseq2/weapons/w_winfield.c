#include "../g_local.h"

extern huntweapon_t w_winfield;
// magic number is 31.6384u per meter
#define WINFIELD_DAMAGE				110
#define WINFIELD_MONSTERDAMAGE		35
#define WINFIELD_SPREAD				18
#define WINFIELD_RANGE_START		425
#define WINFIELD_RANGE_FALLOFF		0.0136


static void W_Winfield_Draw(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	client->hunt_attack_finished = client->hunt_client_time + 0.8;
	client->hunt_busy_finished = client->hunt_client_time + 0.6;
}

static void W_Winfield_Fire(edict_t *ent)
{
	vec3_t forward, start;
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	if (client->hunt_invselected->ammo <= 0)
	{
		Hunt_DryFire(ent);
		return;
	}

	client->hunt_invselected->ammo--;

	if ((client->hunt_weapon_anim >= WEP_ANIM_ADSIN) && (client->hunt_weapon_anim <= WEP_ANIM_ADS))
	{
		if (Hunt_HunterHasTrait(ent, &trait_ironrepeater))
		{
			Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_ADSFIRE2);
			client->kick_angles[PITCH] -= 12;

			client->hunt_attack_finished = client->hunt_client_time + 0.9;
			client->hunt_busy_finished = client->hunt_client_time + 0.8;
		}
		else
		{
			Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_ADSFIRE);
			client->kick_angles[PITCH] -= 10;

			client->hunt_attack_finished = client->hunt_client_time + 1.1;
			client->hunt_busy_finished = client->hunt_client_time + 0.9;
		}
	}
	else
	{
		if (Hunt_HunterHasTrait(ent, &trait_levering))
		{
			Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_FIRE2);
			client->kick_angles[PITCH] -= 5;

			client->hunt_attack_finished = client->hunt_client_time + 0.51;
			client->hunt_busy_finished = client->hunt_client_time + 0.5;
		}
		else
		{
			Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_FIRE1);
			client->kick_angles[PITCH] -= 6;

			client->hunt_attack_finished = client->hunt_client_time + 0.9;
			client->hunt_busy_finished = client->hunt_client_time + 0.6;
		}
	}

	client->hunt_weapon_recoil += 10;

	gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/winchester/winchester_fire2_old.wav"), 1, ATTN_NORM, 0);

	AngleVectors(client->hunt_weapon_angle, forward, NULL, NULL);
	VectorCopy(ent->s.origin, start);
	start[2] += ent->viewheight;
	Hunt_FireBullet(ent, start, forward, client->hunt_weapon_inaccuracy * WINFIELD_SPREAD, WINFIELD_DAMAGE, WINFIELD_MONSTERDAMAGE, WINFIELD_RANGE_START, WINFIELD_RANGE_FALLOFF);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(0);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
}

static void W_Winfield_Frame(edict_t *ent, usercmd_t *ucmd)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	Hunt_AdsFrame(ent);

	if (ucmd->buttons & BUTTON_ATTACK)
		client->hunt_weapon_flags &= ~HUNT_WEPFLAGS_RELOAD;

	if (client->hunt_invselected->ammo >= client->hunt_invselected->def->maxammo || client->hunt_invammo[client->hunt_invselected->def->ammotype] <= 0)
		client->hunt_weapon_flags &= ~HUNT_WEPFLAGS_RELOAD;

	if ((client->hunt_weapon_flags & HUNT_WEPFLAGS_RELOAD) && (client->hunt_client_time >= client->hunt_busy_finished))
	{
		client->hunt_weapon_flags &= ~HUNT_WEPFLAGS_ADS;
		Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_RELOAD1);
		client->hunt_busy_finished = client->hunt_client_time + 10;
		client->hunt_attack_finished = client->hunt_busy_finished;
	}

	if (client->hunt_client_time < client->hunt_attack_finished)
		return;

	if (client->ps.pmove.pm_flags & PMF_HUNT_SPRINT)
		client->hunt_attack_finished = client->hunt_client_time + 0.1;

	if ((ucmd->buttons & BUTTON_ATTACK) && (!(client->lastcmd.buttons & BUTTON_ATTACK) || (Hunt_HunterHasTrait(ent, &trait_levering) && !(client->hunt_weapon_flags & HUNT_WEPFLAGS_ADS))))
	{
		W_Winfield_Fire(ent);
	}
}

static void W_Winfield_Reload_AddBullet(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	if (client->hunt_invammo[client->hunt_invselected->def->ammotype] <= 0)
		return;

	client->hunt_invammo[client->hunt_invselected->def->ammotype]--;
	client->hunt_invselected->ammo = min(client->hunt_invselected->ammo + 1, client->hunt_invselected->def->maxammo);
}

static void W_Winfield_Reload_Loop(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	if (client->hunt_invselected->ammo >= client->hunt_invselected->def->maxammo)
		client->hunt_weapon_flags &= ~HUNT_WEPFLAGS_RELOAD;

	if (client->hunt_weapon_flags & HUNT_WEPFLAGS_RELOAD)
	{
		Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_RELOAD2);
	}
	else
	{
		Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_RELOAD3);
	}
}

static void W_Winfield_Reload_Finish(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	client->hunt_weapon_flags &= ~HUNT_WEPFLAGS_RELOAD;
	client->hunt_busy_finished = client->hunt_client_time + 0.1;
	client->hunt_attack_finished = client->hunt_busy_finished;
}

void W_Winfield_Init(void)
{
	huntweapon_t *wep = &w_winfield;
	wanim_t *anim;

	gi.soundindex("weapons/winchester/winchester_fire2_old.wav");

	wep->view_offset[1] = -4;
	wep->view_offset[2] = -3;

	wep->maxammo = 12;
	wep->ammotype = AMMOTYPE_COMPACT;
	wep->framefunc = W_Winfield_Frame;
	wep->adsmodel = "models/weapons/usa/v_bar/tris.md2";
	gi.modelindex(wep->adsmodel);

	// draw
	anim = &wep->animations[WEP_ANIM_DRAW];
	anim->time_end = 1;
	anim->anim_next = WEP_ANIM_IDLE;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 1, 37, 0.025);
	Hunt_AddWeaponAnimationEvent(anim, 0, 0, W_Winfield_Draw, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0, 0, Hunt_DisableCrosshair, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.5, 0, NULL, gi.soundindex("weapons/winchester/winchester_lever1.wav"));
	Hunt_AddWeaponAnimationEvent(anim, 0.7, 0, Hunt_EnableCrosshair, 0);

	// putaway
	anim = &wep->animations[WEP_ANIM_PUTAWAY];
	anim->time_end = 0.5;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 106, 10, 0.05);
	Hunt_AddWeaponAnimationEvent(anim, 0.15, 0, Hunt_DisableCrosshair, 0);

	// idle
	anim = &wep->animations[WEP_ANIM_IDLE];
	anim->time_end = 6;
	anim->loop = true;
	Hunt_AddWeaponAnimationEvent(anim, 0, 97, Hunt_EnableCrosshair, 0);
	Hunt_AddWeaponAnimationPeriod(anim, 1.5, 98, 7, 0.1);
	Hunt_AddWeaponAnimationEvent(anim, 3.8, 102, NULL, 0);
	Hunt_AddWeaponAnimationEvent(anim, 3.89, 100, NULL, 0);
	Hunt_AddWeaponAnimationEvent(anim, 3.96, 99, NULL, 0);
	Hunt_AddWeaponAnimationEvent(anim, 4.06, 97, NULL, 0);

	// fire
	anim = &wep->animations[WEP_ANIM_FIRE1];
	anim->time_end = 0.675;
	anim->anim_next = WEP_ANIM_IDLE;
	Hunt_AddWeaponAnimationEvent(anim, 0, 0, Hunt_EnableCrosshair, 0);
	Hunt_AddWeaponAnimationPeriod(anim, 0, 40, 25, 0.025);
	Hunt_AddWeaponAnimationEvent(anim, 0.25, 0, NULL, gi.soundindex("weapons/winchester/winchester_lever2.wav"));

	// fire (levering)
	anim = &wep->animations[WEP_ANIM_FIRE2];
	anim->time_end = 0.475;
	anim->anim_next = WEP_ANIM_IDLE;
	Hunt_AddWeaponAnimationEvent(anim, 0, 0, Hunt_EnableCrosshair, 0);
	Hunt_AddWeaponAnimationPeriod(anim, 0, 40, 5, 0.025);
	Hunt_AddWeaponAnimationPeriod(anim, 0.15, 55, 10, 0.025);
	Hunt_AddWeaponAnimationEvent(anim, 0.2, 0, NULL, gi.soundindex("weapons/winchester/winchester_lever2.wav"));

	// ads in
	anim = &wep->animations[WEP_ANIM_ADSIN];
	anim->time_end = 0.15;
	anim->anim_next = WEP_ANIM_ADS;
	Hunt_AddWeaponAnimationEvent(anim, 0, 39, NULL, 0);
	Hunt_AddWeaponAnimationPeriod(anim, 0.05, 78, 3, 0.05);
	Hunt_AddWeaponAnimationEvent(anim, 0.05, 0, Hunt_AdsIn, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.1, 0, Hunt_DisableCrosshair, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.15, 87, NULL, 0);

	// ads
	anim = &wep->animations[WEP_ANIM_ADS];
	anim->time_end = 0;
	anim->loop = true;
	Hunt_AddWeaponAnimationEvent(anim, 0, 87, NULL, 0);

	// ads fire
	anim = &wep->animations[WEP_ANIM_ADSFIRE];
	anim->time_end = 1.1;
	anim->anim_next = WEP_ANIM_ADS;
	Hunt_AddWeaponAnimationEvent(anim, 0, 84, NULL, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.3, 0, Hunt_AdsOut, 0);
	Hunt_AddWeaponAnimationPeriod(anim, 0.3, 41, 24, 0.025);
	Hunt_AddWeaponAnimationEvent(anim, 0.6, 0, NULL, gi.soundindex("weapons/winchester/winchester_lever2.wav"));
	Hunt_AddWeaponAnimationEvent(anim, 0.95, 84, Hunt_AdsLateCheckIdle, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.95, 84, Hunt_AdsIn, 0);
	//

	// ads fire (iron repeater)
	anim = &wep->animations[WEP_ANIM_ADSFIRE2];
	anim->time_end = 0.725;
	anim->anim_next = WEP_ANIM_ADS;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 84, 3, 0.1);
	Hunt_AddWeaponAnimationEvent(anim, 0.3, 0, NULL, gi.soundindex("weapons/winchester/winchester_lever2.wav"));
	Hunt_AddWeaponAnimationEvent(anim, 0.5, 0, Hunt_AdsLateCheck, 0);
	//

	// ads out
	anim = &wep->animations[WEP_ANIM_ADSOUT];
	anim->time_end = 0.15;
	anim->anim_next = WEP_ANIM_IDLE;
	Hunt_AddWeaponAnimationEvent(anim, 0, 87, NULL, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.05, 80, NULL, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.05, 79, Hunt_EnableCrosshair, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.1, 40, Hunt_AdsOut, 0);

	// reload
	anim = &wep->animations[WEP_ANIM_RELOAD1];
	anim->time_end = 0.35;
	anim->anim_next = WEP_ANIM_RELOAD2;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 65, 13, 0.025);
	Hunt_AddWeaponAnimationEvent(anim, 0.15, 79, Hunt_DisableCrosshair, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.2, 0, NULL, gi.soundindex("weapons/peacemaker/winchester_lever1.wav"));

	anim = &wep->animations[WEP_ANIM_RELOAD2];
	anim->time_end = 0.8;
	anim->anim_next = WEP_ANIM_RELOAD3;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 79, 11, 0.05);
	Hunt_AddWeaponAnimationPeriod(anim, 0.55, 74, 5, 0.05);
	Hunt_AddWeaponAnimationEvent(anim, 0.1, 0, NULL, gi.soundindex("weapons/peacemaker/peacemaker_insert1.wav"));
	Hunt_AddWeaponAnimationEvent(anim, 0.2, 0, W_Winfield_Reload_AddBullet, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.6, 79, W_Winfield_Reload_Loop, 0);

	anim = &wep->animations[WEP_ANIM_RELOAD3];
	anim->time_end = 0.375;
	anim->anim_next = WEP_ANIM_IDLE;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 90, 14, 0.025);
	Hunt_AddWeaponAnimationEvent(anim, 0, 0, W_Winfield_Reload_Finish, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.15, 0, Hunt_EnableCrosshair, 0);
}



