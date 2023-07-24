#include "../g_local.h"

extern huntweapon_t w_peacemaker;
// magic number is 31.6384u per meter
#define PEACEMAKER_DAMAGE			110
#define PEACEMAKER_MONSTERDAMAGE	40
#define PEACEMAKER_SPREAD			13
#define PEACEMAKER_RANGE_START		384
#define PEACEMAKER_RANGE_FALLOFF	0.035


static void W_PeaceMaker_Draw(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	client->hunt_attack_finished = client->hunt_client_time + 0.6;
	client->hunt_busy_finished = client->hunt_client_time + 0.5;
}

static void W_PeaceMaker_Fire(edict_t *ent)
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
		Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_ADSFIRE);
		client->kick_angles[PITCH] -= 10;
	}
	else
	{
		Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_FIRE1);
		client->kick_angles[PITCH] -= 6;
	}

	client->hunt_attack_finished = client->hunt_client_time + 1.15;
	client->hunt_busy_finished = client->hunt_client_time + 0.3;

	char buf[128];
	Q_snprintf(buf, sizeof(buf), "weapons/peacemaker/peacemaker_single%i.wav", (int)(random() * 3) + 1);

	gi.sound(ent, CHAN_WEAPON, gi.soundindex(buf), 1, ATTN_NORM, 0);

	AngleVectors(client->hunt_weapon_angle, forward, NULL, NULL);
	VectorCopy(ent->s.origin, start);
	start[2] += ent->viewheight;
	Hunt_FireBullet(ent, start, forward, client->hunt_weapon_inaccuracy * PEACEMAKER_SPREAD, PEACEMAKER_DAMAGE, PEACEMAKER_MONSTERDAMAGE, PEACEMAKER_RANGE_START, PEACEMAKER_RANGE_FALLOFF);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(0);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
}

static void W_PeaceMaker_Frame(edict_t *ent, usercmd_t *ucmd)
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

	if ((ucmd->buttons & BUTTON_ATTACK) && !(client->lastcmd.buttons & BUTTON_ATTACK))
	{
		W_PeaceMaker_Fire(ent);
	}
}

static void W_PeaceMaker_Reload_AddBullet(edict_t *ent)
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

static void W_PeaceMaker_Reload_Loop(edict_t *ent)
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
}

static void W_PeaceMaker_Reload_Finish(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	client->hunt_weapon_flags &= ~HUNT_WEPFLAGS_RELOAD;
	client->hunt_busy_finished = client->hunt_client_time + 0.1;
	client->hunt_attack_finished = client->hunt_busy_finished;
}

void W_PeaceMaker_Init(void)
{
	huntweapon_t *wep = &w_peacemaker;
	wanim_t *anim;

	wep->maxammo = 6;
	wep->ammotype = AMMOTYPE_MEDIUM;
	wep->framefunc = W_PeaceMaker_Frame;
	wep->adsmodel = "models/weapons/gbr/v_webley/tris.md2";
	gi.modelindex("models/weapons/gbr/v_webley/tris.md2");

	// draw
	anim = &wep->animations[WEP_ANIM_DRAW];
	anim->time_end = 1;
	anim->anim_next = WEP_ANIM_IDLE;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 2, 36, 0.025);
	Hunt_AddWeaponAnimationEvent(anim, 0, 0, W_PeaceMaker_Draw, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0, 0, Hunt_DisableCrosshair, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.7, 0, Hunt_EnableCrosshair, 0);

	// putaway
	anim = &wep->animations[WEP_ANIM_PUTAWAY];
	anim->time_end = 0.4;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 179, 8, 0.05);
	Hunt_AddWeaponAnimationEvent(anim, 0.15, 0, Hunt_DisableCrosshair, 0);

	// idle
	anim = &wep->animations[WEP_ANIM_IDLE];
	anim->time_end = 6;
	anim->loop = true;
	Hunt_AddWeaponAnimationEvent(anim, 0, 119, Hunt_EnableCrosshair, 0);
	Hunt_AddWeaponAnimationPeriod(anim, 2, 119, 37, 0.05);

	// fire
	anim = &wep->animations[WEP_ANIM_FIRE1];
	anim->time_end = 1.3;
	anim->anim_next = WEP_ANIM_IDLE;
	Hunt_AddWeaponAnimationEvent(anim, 0, 0, Hunt_EnableCrosshair, 0);
	Hunt_AddWeaponAnimationPeriod(anim, 0, 39, 9, 0.025);
	Hunt_AddWeaponAnimationPeriod(anim, 0.3, 47, 20, 0.05);
	Hunt_AddWeaponAnimationEvent(anim, 0.7, 0, NULL, gi.soundindex("weapons/peacemaker/peacemaker_cock.wav"));

	// ads in
	anim = &wep->animations[WEP_ANIM_ADSIN];
	anim->time_end = 0.15;
	anim->anim_next = WEP_ANIM_ADS;
	Hunt_AddWeaponAnimationEvent(anim, 0, 72, NULL, 0);
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
	anim->time_end = 1;
	anim->anim_next = WEP_ANIM_ADS;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 84, 3, 0.1);
	Hunt_AddWeaponAnimationEvent(anim, 0.6, 0, NULL, gi.soundindex("weapons/peacemaker/peacemaker_cock.wav"));
	Hunt_AddWeaponAnimationEvent(anim, 0.6, 0, Hunt_AdsLateCheck, 0);
	

	// ads out
	anim = &wep->animations[WEP_ANIM_ADSOUT];
	anim->time_end = 0.25;
	anim->anim_next = WEP_ANIM_IDLE;
	Hunt_AddWeaponAnimationEvent(anim, 0, 87, NULL, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.05, 80, NULL, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.1, 79, Hunt_EnableCrosshair, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.2, 71, Hunt_AdsOut, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.2, 66, NULL, 0);

	// reload
	anim = &wep->animations[WEP_ANIM_RELOAD1];
	anim->time_end = 0.35;
	anim->anim_next = WEP_ANIM_RELOAD2;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 70, 13, 0.025);
	Hunt_AddWeaponAnimationEvent(anim, 0.15, 79, Hunt_DisableCrosshair, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.2, 0, NULL, gi.soundindex("weapons/peacemaker/peacemaker_open.wav"));

	anim = &wep->animations[WEP_ANIM_RELOAD2];
	anim->time_end = 1.15;
	anim->anim_next = WEP_ANIM_RELOAD3;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 84, 22, 0.05);
	Hunt_AddWeaponAnimationEvent(anim, 0.7, 0, NULL, gi.soundindex("weapons/peacemaker/peacemaker_insert1.wav"));
	Hunt_AddWeaponAnimationEvent(anim, 0.9, 0, W_PeaceMaker_Reload_AddBullet, 0);
	Hunt_AddWeaponAnimationEvent(anim, 1.15, 79, W_PeaceMaker_Reload_Loop, 0);

	anim = &wep->animations[WEP_ANIM_RELOAD3];
	anim->time_end = 0.375;
	anim->anim_next = WEP_ANIM_IDLE;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 105, 14, 0.025);
	Hunt_AddWeaponAnimationEvent(anim, 0, 79, W_PeaceMaker_Reload_Finish, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.15, 79, Hunt_EnableCrosshair, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.2, 0, NULL, gi.soundindex("weapons/peacemaker/peacemaker_close.wav"));
}



