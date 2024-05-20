

#include "g_local.h"

#define DETAIL_HEALTHBAR 	0
#define DETAIL_LOWLOD		1
#define DETAIL_BODY1		2
#define DETAIL_BODY2		3
#define DETAIL_BODY3		4

// fuckin solid32 hacky bullshit :/
static float monster_solidhack_predraw(edict_t *viewer, edict_t *ent, entity_state_t *state)
{
	vec3_t diff;
	VectorSubtract(viewer->s.origin, ent->s.origin, diff);
	if (VectorLengthSquared(diff) > 4096)
		return false;
	return true;
}

int monster_actor_addtoscene(actor_t *actor, int score)
{
	if (score > 768) // if we're far away and a low lod is available, use that one
	{
		if (actor->details[DETAIL_LOWLOD])
		{
			Actor_AddDetail(actor, actor->details[DETAIL_LOWLOD]);
			return true;
		}
	}
	else
	{
		if (score < 384)
		{
			if (actor->details[DETAIL_HEALTHBAR])
			{
				Actor_AddDetail(actor, actor->details[DETAIL_HEALTHBAR]);
			}
		}
	}


	for(int i = DETAIL_BODY1; i < ACTOR_MAX_DETAILS; i++)
	{
		if (actor->details[i] == NULL)
			break;

		if (Actor_AddDetail(actor, actor->details[i]))
			break;
	}

	return true;
}

static void monster_actor_update(actor_t *actor)
{
	edict_t *ent = actor->owner;
	VectorCopy(ent->s.origin, actor->origin);
	Actor_Link(actor, 256);

	if (actor->details[DETAIL_LOWLOD])
	{
		VectorCopy(ent->s.origin, actor->details[DETAIL_LOWLOD]->s.origin);
		actor->details[DETAIL_LOWLOD]->s.angles[YAW] = ent->s.angles[YAW];
		//actor->details[DETAIL_LOWLOD]->s.event = EV_OTHER_TELEPORT;
	}

	if (actor->details[DETAIL_BODY1])
	{
		VectorCopy(ent->s.origin, actor->details[DETAIL_BODY1]->s.origin);
		VectorCopy(ent->s.angles, actor->details[DETAIL_BODY1]->s.angles);
	}

	if (actor->details[DETAIL_HEALTHBAR])
	{
		VectorCopy(actor->details[DETAIL_HEALTHBAR]->s.origin, actor->details[DETAIL_HEALTHBAR]->s.old_origin);
		VectorSet(actor->details[DETAIL_HEALTHBAR]->s.origin, ent->s.origin[0], ent->s.origin[1], ent->absmax[2] + 10);
		float frac = 1 - ((float)ent->health / (float)(ent->max_health ? ent->max_health : 100));
		clamp(frac, 0, 1);
		actor->details[DETAIL_HEALTHBAR]->s.frame = (int)(33 * frac);
	}
}

static float monster_healthbar_predraw(edict_t *viewer, edict_t *ent, entity_state_t *state)
{
	state->angles[PITCH] = viewer->client->ps.viewangles[PITCH] * -1;
	state->angles[YAW] = viewer->client->ps.viewangles[YAW] + 180;
	state->angles[ROLL] = 0;

	return true;
}

static void monster_spawn_healthbar(actor_t *actor)
{
	detail_edict_t *hpbar = D_Spawn();
	actor->details[DETAIL_HEALTHBAR] = hpbar;

	hpbar->s.modelindex = gi.modelindex("models/hud/bar.md2");
	hpbar->s.renderfx = RF_NOSHADOW | RF_FULLBRIGHT | RF_FRAMELERP;
	hpbar->predraw = monster_healthbar_predraw;
}

static void monster_takedamage(edict_t *self, edict_t *attacker, int dmg, int power, vec3_t direction)
{
	self->health = max(0, self->health - dmg);

	vec3_t knock_dir;
	if (VectorCompare(vec3_origin, direction))
		VectorSubtract(self->s.origin, attacker->s.origin, knock_dir);
	else
		VectorCopy(direction, knock_dir);
	VectorNormalize(knock_dir);
	float knock_amount = ((float)power / (self->mass == 0 ? 1 : self->mass)) + ((float)dmg / 30);
	VectorMA(self->velocity, knock_amount, knock_dir, self->velocity);
	self->velocity[2] += 40;
}


void monster_skeleton_physics(edict_t *self)
{
	m_skeletoninfo_t *data = (m_skeletoninfo_t*)self->monster_data;

	self->damage_flags = DAMAGEFLAG_YES;

	trace_t trace;
	vec3_t goal_pos;
	VectorMA(self->s.origin, FRAMETIME, self->velocity, goal_pos);

	SV_AddGravity(self);
	if (self->groundentity)
		SV_Friction(self);
	SV_FlyMove(self, FRAMETIME, MASK_MONSTERSOLID);

	//trace = gi.trace(self->s.origin, self->mins, self->maxs, goal_pos, self, MASK_MONSTERSOLID);
	//VectorCopy(trace.endpos, self->s.origin);


	gi.linkentity(self);
}

void SP_monster_skeleton(edict_t *self)
{
	actor_t *actor = Actor_Spawn();
	self->actor = actor;
	actor->owner = self;
	actor->physics = monster_actor_update;
	actor->addtoscene = monster_actor_addtoscene;
	monster_spawn_healthbar(actor);
	detail_edict_t *body = D_Spawn();
	actor->details[DETAIL_BODY1] = body;
	body->s.modelindex = gi.modelindex("models/monsters/wraith.md2");

	VectorSet(self->mins, -16, -16, -26);
	VectorSet(self->maxs, 16, 16, 20);
	self->solid = SOLID_BBOX;
	self->svflags = SVF_MONSTER;
	self->max_health = 30;
	self->health = self->max_health;
	self->s.modelindex = null_model;
	self->takedamage = monster_takedamage;
	self->predraw = monster_solidhack_predraw;
	self->physics = monster_skeleton_physics;
	VectorCopy(self->s.origin, self->pos1);

	gi.linkentity(self);
}




