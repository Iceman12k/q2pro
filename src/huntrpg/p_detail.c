
#include "g_local.h"
#include "md3.h"

#define CD_MODEL	0
#define CD_WEAPON	1

void CL_DetailCleanup(edict_t *ent)
{
	gclient_t *client = ent->client;
	actor_t *actor = ent->actor;

	Actor_Cleanup(actor);
}

float CL_WeaponPredraw(edict_t *v, detail_edict_t *e, entity_state_t *s)
{
	if (v == e->owner)
		s->modelindex = null_model;
	return true;
}

int CL_ActorAddToScene(actor_t *actor, int score)
{
	if (viewer_edict != actor->owner)
		return false;
	
	Actor_AddDetail(actor, actor->details[CD_WEAPON]);
	return true;
}

void CL_ActorEvaluate(actor_t *actor, int *score)
{
	if (viewer_edict == actor->owner)
	{
		if (viewer_edict->client->passive_flags & PASSIVE_LIGHT)
			*score = 1;
		else
			*score = ACTOR_DO_NOT_ADD;
	}
}

int CL_ActorFrame(actor_t *actor)
{
	edict_t *ent = actor->owner;
	gclient_t *client = ent->client;
	detail_edict_t *model = actor->details[CD_MODEL];
	detail_edict_t *weapon = actor->details[CD_WEAPON];
	VectorCopy(actor->owner->s.origin, model->s.origin);
	model->s.angles[YAW] = actor->owner->client->ps.viewangles[YAW];

	VectorCopy(actor->owner->s.origin, weapon->s.origin);
	weapon->s.modelindex = 0;
	weapon->s.effects = 0;
	if (client->passive_flags & PASSIVE_LIGHT)
	{
		weapon->s.effects |= EF_HYPERBLASTER;
		weapon->s.modelindex = null_model;
	}


	int chunk_num = ORG_TO_CHUNK(actor->origin);
	actor->chunks_visible = 1ULL << chunk_num |
							(1ULL << chunk_num) << 1ULL |
							(1ULL << chunk_num) >> 1ULL |
							(1ULL << chunk_num) << 8ULL |
							(1ULL << chunk_num) >> 8ULL |
							(1ULL << chunk_num) << 9ULL |
							(1ULL << chunk_num) << 7ULL |
							(1ULL << chunk_num) >> 9ULL |
							(1ULL << chunk_num) >> 7ULL;
	
	if (client->ps.pmove.pm_flags & PMF_DUCKED)
		model->s.frame = 144;
	else
		model->s.frame = 0;

	return false;
}

void CL_DetailCreate(edict_t *ent)
{
	detail_edict_t *detail, *weapon;
	gclient_t *client = ent->client;
	actor_t *actor = Actor_Spawn();
	ent->actor = actor;
	actor->owner = ent;
	actor->physics = CL_ActorFrame;
	actor->evaluate = CL_ActorEvaluate;
	actor->addtoscene = CL_ActorAddToScene;

	detail = D_Spawn();
	actor->details[CD_MODEL] = detail;
	detail->s.modelindex = gi.modelindex("players/female/tris.md2");

	weapon = D_Spawn();
	actor->details[CD_WEAPON] = weapon;
	weapon->predraw = CL_WeaponPredraw;
	weapon->s.modelindex = null_model;
}

/*
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
*/


