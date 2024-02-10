
#include "g_local.h"
#include "md3.h"

static float PL_DetailFrame(detail_edict_t *e)
{
	edict_t *ent = e->owner;
	gclient_t *client = ent->client;
	detail_edict_t *legs = client->details[CD_LEGS];
	detail_edict_t *torso = client->details[CD_TORSO];
	detail_edict_t *head = client->details[CD_HEAD];
	detail_edict_t *wep = client->details[CD_WEAPON];

	(client->details[CD_LEGS])->s.frame = 544 + (((level.framenum / FRAMEDIV) * 2) % 18);

	if (FRAMESYNC)
	{
		for (int i = 0; i < CD_MAX; i++)
		{
			(client->details[i])->md3anim.frame1 = (client->details[i])->md3anim.frame2;
			(client->details[i])->md3anim.frame2 = (client->details[i])->s.frame;
		}
	}

	VectorCopy(ent->s.origin, legs->s.origin);
	
	//
	float lean_scale;
	vec3_t forward, right;
	vec3_t vel_to_use;
	AngleVectors(ent->s.angles, forward, right, NULL);

	if (ent->client->ps.pmove.pm_flags & PMF_ON_GROUND)
		lean_scale = 1;
	else
		lean_scale = 2;

	VectorCopy(ent->velocity, vel_to_use);
	vel_to_use[2] = 0;

	// determine leg dir
	if (VectorLength(vel_to_use) > 30)
	{
		legs->angle = vectoyaw(vel_to_use);
	}

	VectorCopy(client->cmd_angles, ent->s.angles);

	float angdiff = anglediff(ent->s.angles[1], legs->angle);
	if (angdiff > 120 || angdiff < -120)
	{
		legs->angle = vectoyaw(vel_to_use) + 180;
	}
	else if (angdiff > 110)
	{
		legs->angle = ent->s.angles[1] + 45;
	}
	else if (angdiff > 45)
	{
		if (VectorLength(vel_to_use) > 30)
			legs->angle = ent->s.angles[1] - 45;
		else
			legs->angle = ent->s.angles[1] - 15;
	}
	else if (angdiff < -110)
	{
		legs->angle = ent->s.angles[1] - 45;
	}
	else if (angdiff < -45)
	{
		if (VectorLength(vel_to_use) > 30)
			legs->angle = ent->s.angles[1] + 45;
		else
			legs->angle = ent->s.angles[1] - 15;
	}

	angdiff = anglediff(legs->s.angles[1], legs->angle);
	float absdiff = fabs(angdiff);
	float spd = 50;
	if (absdiff > 40)
		spd = 600;
	else if (absdiff > 20)
		spd = 400;
	else if (absdiff > 5)
		spd = 200;
	spd = min(spd * FRAMETIME, absdiff);
#if 1
	if (angdiff < 0)
		legs->s.angles[1] = legs->s.angles[1] + spd;
	else if (angdiff > 0)
		legs->s.angles[1] = legs->s.angles[1] - spd;
#endif

	angdiff = anglediff(ent->s.angles[1], legs->s.angles[1]);
	if (angdiff > 60)
		legs->s.angles[1] = ent->s.angles[1] - 60;
	else if (angdiff < -60)
		legs->s.angles[1] = ent->s.angles[1] + 60;
	//



	//legs->s.angles[0] = DotProduct(vel_to_use, forward) * 0.025 * lean_scale;
	legs->s.angles[2] = DotProduct(vel_to_use, right) * 0.03 * lean_scale;
	//

	MD3_AttachEdict(MD3_LoadModel("models/players/m1/lower.md3"), "tag_torso", &legs->md3anim, &legs->s, &torso->s);
	//LerpAngles(legs->s.angles, client->cmd_angles, 0.6, torso->s.angles);

	if (ent->deadflag == DEAD_NO)
	{
		torso->s.angles[0] = (legs->s.angles[0] * 0.1) + (ent->s.angles[0] * 1.1);
		torso->s.angles[1] = ent->s.angles[1];
		torso->s.angles[2] = legs->s.angles[2] * 0.3;
	}

	MD3_AttachEdict(MD3_LoadModel("models/players/m1/upper.md3"), "tag_head", &torso->md3anim, &torso->s, &head->s);
	MD3_AttachEdict(MD3_LoadModel("models/players/m1/upper.md3"), "tag_weapon", &torso->md3anim, &torso->s, &wep->s);
	
	if (ent->deadflag == DEAD_NO)
	{
		head->s.angles[0] = client->cmd_angles[0];//(ent->s.angles[0] * 0.8);
	}
	

	//VectorCopy(ent->s.origin, torso->s.origin);
	//VectorCopy(ent->s.origin, head->s.origin);
	//VectorCopy(ent->s.origin, wep->s.origin);



	// do weapon lights
	wep->s.effects = 0;
	if (client->passive_flags & PASSIVE_LIGHT)
	{
		wep->s.effects = EF_HYPERBLASTER;
	}
	//

	return true;
}

float PL_DetailPredraw(edict_t *v, detail_edict_t *e, entity_state_t *s)
{
	if (v == e->owner)
		return false;
	return true;
}

void CL_DetailCreate(edict_t *ent)
{
	gclient_t *client = ent->client;

	for (int i = 0; i < CD_MAX; i++)
	{
		detail_edict_t *ed = D_Spawn();
		client->details[i] = ed;
		ed->classname = "player_detail";
		ed->s.modelindex = gi.modelindex("models/null.md2");
		ed->owner = ent;
		ed->predraw = PL_DetailPredraw;
	}

	(client->details[CD_TORSO])->physics = PL_DetailFrame;

	//
	(client->details[CD_HEAD])->s.modelindex = gi.modelindex("models/players/m1/head.md2");
	
	//
	(client->details[CD_TORSO])->s.modelindex = gi.modelindex("models/players/m1/upper.md2");
	(client->details[CD_TORSO])->s.frame = 516;
	(client->details[CD_TORSO])->detailflags |= DETAIL_PRIORITY_HIGH;
	
	//
	(client->details[CD_LEGS])->s.modelindex = gi.modelindex("models/players/m1/lower.md2");
	(client->details[CD_LEGS])->detailflags |= DETAIL_PRIORITY_HIGH;

	//
	(client->details[CD_WEAPON])->s.modelindex = gi.modelindex("models/weapons/h_lantern.md2");
	(client->details[CD_WEAPON])->detailflags |= DETAIL_NEARLIMIT;
}

void CL_DetailCleanup(edict_t *ent)
{
	gclient_t *client = ent->client;

	for (int i = 0; i < CD_MAX; i++)
	{
		detail_edict_t *ed = client->details[i];
		if (!ed)
			continue;
		client->details[i] = NULL;
		D_Free(ed);
	}
}


