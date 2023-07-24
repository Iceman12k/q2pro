#include "../g_local.h"

extern huntweapon_t w_m1garand;
// magic number is 31.6384u per meter
#define M1GARAND_DAMAGE				127
#define M1GARAND_MONSTERDAMAGE		80
#define M1GARAND_SPREAD				22
#define M1GARAND_RANGE_START		768
#define M1GARAND_RANGE_FALLOFF		0.01012

static void W_M1Garand_Draw(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	client->hunt_attack_finished = client->hunt_client_time + 1;
	client->hunt_busy_finished = client->hunt_client_time + 0.6;
}

static void W_M1Garand_Fire(edict_t *ent)
{
	vec3_t forward, up, start;
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

	AngleVectors(client->v_angle, forward, NULL, up);

	if ((client->hunt_weapon_anim >= WEP_ANIM_ADSIN) && (client->hunt_weapon_anim <= WEP_ANIM_ADS))
	{
		Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_ADSFIRE);
		client->kick_angles[PITCH] -= 12;

		for (int i = 0; i < 3; i++)
		{
			client->kick_origin[i] += forward[i] * -4;
			client->kick_origin[i] += up[i] * -2;
		}
	}
	else
	{
		if (client->hunt_invselected->ammo > 0)
			Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_FIRE1);
		else
			Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_FIRE2);
		client->kick_angles[PITCH] -= 6;
		
		for (int i = 0; i < 3; i++)
		{
			client->kick_origin[i] += forward[i] * -8;
			client->kick_origin[i] += up[i] * 2;
		}
	}

	client->hunt_attack_finished = client->hunt_client_time + 0.2;
	client->hunt_busy_finished = client->hunt_client_time + 0.12;
	client->hunt_weapon_recoil += 6;

	char buf[128];
	Q_snprintf(buf, sizeof(buf), "weapons/m1garand/fire%i.wav", (int)(random() * 1.99) + 1);

	gi.sound(ent, CHAN_WEAPON, gi.soundindex(buf), 1, ATTN_NORM, 0);

	if (client->hunt_invselected->ammo <= 0)
	{
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/m1garand/ping.wav"), 1, ATTN_GUN_MECHANICAL, 0);
	}

	AngleVectors(client->hunt_weapon_angle, forward, NULL, NULL);
	VectorCopy(ent->s.origin, start);
	start[2] += ent->viewheight;
	Hunt_FireBullet(ent, start, forward, client->hunt_weapon_inaccuracy * M1GARAND_SPREAD, M1GARAND_DAMAGE, M1GARAND_MONSTERDAMAGE, M1GARAND_RANGE_START, M1GARAND_RANGE_FALLOFF);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(0);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
}

static void W_M1Garand_Frame(edict_t *ent, usercmd_t *ucmd)
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

	if ((client->hunt_weapon_flags & HUNT_WEPFLAGS_RELOAD) && (client->hunt_client_time >= client->hunt_busy_finished) && !(client->hunt_invselected->ammo))
	{
		client->hunt_weapon_flags &= ~HUNT_WEPFLAGS_ADS;
		Hunt_StartAnimation(ent, client->hunt_invselected->def, WEP_ANIM_RELOAD1);
		client->hunt_busy_finished = client->hunt_client_time + 1.5;
		client->hunt_attack_finished = client->hunt_busy_finished;
	}

	if (client->hunt_client_time < client->hunt_attack_finished)
		return;

	if (client->ps.pmove.pm_flags & PMF_HUNT_SPRINT)
		client->hunt_attack_finished = client->hunt_client_time + 0.1;

	if ((ucmd->buttons & BUTTON_ATTACK) && !(client->lastcmd.buttons & BUTTON_ATTACK))
	{
		W_M1Garand_Fire(ent);
	}
}

static void W_M1Garand_Reload(edict_t *ent)
{
	gclient_t *client;
	client = ent->client;
	if (!client)
		return;

	if (client->hunt_invammo[client->hunt_invselected->def->ammotype] <= 0)
		return;

	client->hunt_invselected->ammo = min(client->hunt_invammo[client->hunt_invselected->def->ammotype], client->hunt_invselected->def->maxammo);
	client->hunt_invammo[client->hunt_invselected->def->ammotype] -= client->hunt_invselected->ammo;
}

void W_M1Garand_Init(void)
{
	huntweapon_t *wep = &w_m1garand;
	wanim_t *anim;

	wep->view_offset[0] = -1;

	wep->maxammo = 8;
	wep->ammotype = AMMOTYPE_LONG;
	wep->framefunc = W_M1Garand_Frame;

	gi.soundindex("weapons/m1garand/fire1.wav");
	gi.soundindex("weapons/m1garand/fire2.wav");

	// draw
	anim = &wep->animations[WEP_ANIM_DRAW];
	anim->time_end = 0.4;
	anim->anim_next = WEP_ANIM_IDLE;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 1, 3, 0.098);
	Hunt_AddWeaponAnimationEvent(anim, 0.29, 37, NULL, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0, 0, W_M1Garand_Draw, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0, 0, Hunt_DisableCrosshair, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.33, 0, Hunt_EnableCrosshair, 0);

	// putaway
	anim = &wep->animations[WEP_ANIM_PUTAWAY];
	anim->time_end = 0.4;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 69, 4, 0.098);
	Hunt_AddWeaponAnimationEvent(anim, 0.15, 0, Hunt_DisableCrosshair, 0);

	// idle
	anim = &wep->animations[WEP_ANIM_IDLE];
	anim->time_end = 1;
	anim->loop = true;
	Hunt_AddWeaponAnimationEvent(anim, 0, 37, Hunt_EnableCrosshair, 0);

	// fire
	anim = &wep->animations[WEP_ANIM_FIRE1];
	anim->time_end = 0.6;
	anim->anim_next = WEP_ANIM_IDLE;
	Hunt_AddWeaponAnimationEvent(anim, 0, 0, Hunt_EnableCrosshair, 0);
	Hunt_AddWeaponAnimationPeriod(anim, 0, 5, 5, 0.098);

	// fire (last shot)
	anim = &wep->animations[WEP_ANIM_FIRE2];
	anim->time_end = 0.7;
	anim->anim_next = WEP_ANIM_IDLE;
	Hunt_AddWeaponAnimationEvent(anim, 0, 0, Hunt_EnableCrosshair, 0);
	Hunt_AddWeaponAnimationPeriod(anim, 0, 65, 3, 0.098);
	Hunt_AddWeaponAnimationPeriod(anim, 0.39, 8, 2, 0.098);

	// ads in
	anim = &wep->animations[WEP_ANIM_ADSIN];
	anim->time_end = 0.15;
	anim->anim_next = WEP_ANIM_ADS;
	Hunt_AddWeaponAnimationEvent(anim, 0.0, 39, NULL, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.05, 0, Hunt_DisableCrosshair, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.05, 87, Hunt_AdsIn, 0);

	// ads
	anim = &wep->animations[WEP_ANIM_ADS];
	anim->time_end = 0;
	anim->loop = true;
	Hunt_AddWeaponAnimationEvent(anim, 0, 87, NULL, 0);

	// ads fire
	anim = &wep->animations[WEP_ANIM_ADSFIRE];
	anim->time_end = 0.5;
	anim->anim_next = WEP_ANIM_ADS;
	Hunt_AddWeaponAnimationPeriod(anim, 0, 76, 4, 0.098);

	// ads out
	anim = &wep->animations[WEP_ANIM_ADSOUT];
	anim->time_end = 0.25;
	anim->anim_next = WEP_ANIM_IDLE;
	Hunt_AddWeaponAnimationEvent(anim, 0, 87, NULL, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.05, 80, NULL, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.1, 79, Hunt_EnableCrosshair, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.15, 109, Hunt_AdsOut, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.2, 103, NULL, 0);

	// reload
	anim = &wep->animations[WEP_ANIM_RELOAD1];
	anim->time_end = 2;
	anim->anim_next = WEP_ANIM_IDLE;
	Hunt_AddWeaponAnimationEvent(anim, 0, 4, NULL, gi.soundindex("weapons/m1garand/reload.wav"));
	Hunt_AddWeaponAnimationPeriod(anim, 0.15, 48, 10, 0.098);
	Hunt_AddWeaponAnimationPeriod(anim, 1.3, 58, 7, 0.098);
	Hunt_AddWeaponAnimationEvent(anim, 0.2, 0, Hunt_DisableCrosshair, 0);
	Hunt_AddWeaponAnimationEvent(anim, 0.95, 0, W_M1Garand_Reload, 0);
}



