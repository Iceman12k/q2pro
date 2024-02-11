

#include "g_local.h"

enum poolball_e {
	BALL_CUE,
	BALL_SOLID,
	BALL_EIGHT = 8,
	BALL_STRIPE,
	BALL_MAX = 16,
};

typedef struct {
	vec3_t origin;
	vec3_t velocity;
} poolball_t;

typedef struct {
	actor_t *actor;
	poolball_t ball[BALL_MAX];
	edict_t *user;
	vec3_t mins;
	vec3_t maxs;
} pooltable_t;

#define POOL_STOPSPEED 2.1
#define POOL_FRICTION 0.48
#define POOL_WALLDAMPING 0.87
#define POOL_BALLDAMPING 0.98
#define BALL_RADIUS 1.9
#define BALL_SIZE BALL_RADIUS * 2

void pooltable_reset(pooltable_t *table)
{
	vec3_t ballpositions[] = {
		{28, 32}, // cue
		{84, 32}, // 1 // solids
		{88, 30}, // 2
		{92, 36}, // 3
		{96, 26}, // 4
		{96, 34}, // 5
		{100, 40}, // 6
		{100, 28}, // 7
		{92, 32}, // eight ball
		{100, 32}, // 9 // stripes
		{88, 34}, // 10
		{92, 28}, // 11 
		{96, 38}, // 12
		{96, 30}, // 13
		{100, 24}, // 14
		{100, 36}, // 15
	};

	
	for(int i = 0; i < BALL_MAX; i++)
	{
		VectorCopy(ballpositions[i], table->ball[i].origin);
	}

	VectorSet(table->ball[BALL_CUE].velocity, 170 + (random() * 8), (random() - 0.5) * 20, 0);
}

void poolball_resolvecollision(poolball_t *b1, poolball_t *b2)
{
	vec3_t diff, diffu, aggr_vel;
	VectorSubtract(b2->origin, b1->origin, diff);
	VectorNormalize2(diff, diffu);

	#if 0
	VectorAdd(b1->velocity, b2->velocity, aggr_vel);
	float mag = DotProduct(aggr_vel, diffu);
	if (mag > 0)
	{
		VectorMA(b1->velocity, -mag * POOL_BALLDAMPING, diffu, b1->velocity);
		VectorMA(b2->velocity, mag * POOL_BALLDAMPING, diffu, b2->velocity);
	}
	else
	{
		VectorMA(b1->velocity, mag * POOL_BALLDAMPING, diffu, b1->velocity);
		VectorMA(b2->velocity, -mag * POOL_BALLDAMPING, diffu, b2->velocity);
	}
	#else
	float mag = DotProduct(b1->velocity, diffu);
	if (mag > 0)
	{
		VectorMA(b1->velocity, -mag * POOL_BALLDAMPING, diffu, b1->velocity);
		VectorMA(b2->velocity, mag * POOL_BALLDAMPING, diffu, b2->velocity);
	}
	#endif

	VectorMA(b1->origin, (BALL_SIZE + 0.02), diffu, b2->origin);
}

void pooltable_user(edict_t *tent, pooltable_t *table)
{
	int ang = tent->s.angles[YAW] - 90;

	if (!table->user->inuse) // uhoh, they got removed!
	{
		table->user = NULL;
		return;
	}
	
	if (table->user->deadflag != DEAD_NO) // uhoh, they got killed!
	{
		table->user = NULL;
		return;
	}
	
	edict_t *ent = table->user;
	gclient_t *client = ent->client;

	vec3_t forward, diff, angles, plate_norm, plate_coord, contact, rayorg;
	float dist;

	// get dist
	VectorAdd(ent->s.origin, client->ps.viewoffset, diff);
	dist = (diff[2] - table->maxs[2]);

	VectorCopy(client->ps.viewangles, angles);
	angles[1] -= ang; // rotate to be centered around the inv_angles

	VectorClear(rayorg);
	VectorSet(plate_coord, 0, 0, -dist);
	VectorSet(plate_norm, 0, 0, 1);
	AngleVectors(angles, forward, NULL, NULL);

	float d = DotProduct(plate_norm, plate_coord);

	if (DotProduct(plate_norm, forward) == 0) {
		return; // No intersection, the line is parallel to the plane
	}

	// Compute the X value for the directed line ray intersecting the plane
	float x = (d - DotProduct(plate_norm, rayorg)) / DotProduct(plate_norm, forward);

	// output contact point
	VectorMA(rayorg, x, forward, contact);

	VectorAdd3(contact, ent->s.origin, client->ps.viewoffset, contact);

	VectorCopy(contact, table->actor->details[0]->s.origin);
}

void pooltable_physics(edict_t *ent)
{
	pooltable_t *table = ent->data;
	int ang = ent->s.angles[YAW] - 90;

	if (level.framenum % 100 == 0)
		pooltable_reset(table);

	// check for players trying to interact
	if (table->user)
	{
		pooltable_user(ent, table);
	}
	else
	{
		for(int i = 0; i < game.maxclients; i++)
		{
			edict_t *pl = EDICT_NUM(i);
			if (!pl->inuse || !pl->client) // no player
				continue;
			
			if (pl->client->inv_open || pl->client->hotbar_open) // players cannot interact with an open inventory
				continue;

			if (pl->client->using_vehicle) // they're already using something
				continue;

			if (!(pl->client->cmd_buttons & BUTTON_ATTACK))
				continue;

			Com_Printf("User taken!\n");
			table->user = pl;
			pl->client->using_vehicle = true;
		}
	}
	//

	return;

	float phys_step = FRAMETIME;
	while(phys_step > 0)
	{
		float delta = min(phys_step, 0.02);
		phys_step -= delta;
		// run physics for the balls
		for(int i = 0; i < BALL_MAX; i++)
		{	
			poolball_t *ball = &table->ball[i];
			VectorMA(ball->origin, delta, ball->velocity, ball->origin);
			
			// friction
			vec3_t friction;
			VectorScale(ball->velocity, delta * POOL_FRICTION, friction);
			VectorSubtract(ball->velocity, friction, ball->velocity);

			// stop speed first
			if (VectorLength(ball->velocity) < POOL_STOPSPEED)
				VectorClear(ball->velocity);

			// table collision
			if (ball->origin[0] < BALL_RADIUS || ball->origin[0] > 128 - BALL_RADIUS)
			{
				ball->velocity[0] *= -POOL_WALLDAMPING;
				clamp(ball->origin[0], 2, 128 - BALL_RADIUS);
			}
			if (ball->origin[1] < BALL_RADIUS || ball->origin[01] > 64 - BALL_RADIUS)
			{
				ball->velocity[1] *= -POOL_WALLDAMPING;
				clamp(ball->origin[1], 2, 64 - BALL_RADIUS);
			}
			//

			// other ball collision
			if (VectorLength(ball->velocity) > 0.5)
			{
				for(int j = 0; j < BALL_MAX; j++)
				{
					poolball_t *other = &table->ball[j];
					if (i == j) // can't collide with ourselves
						continue;

					vec3_t diff;
					vec3_t diffu;
					VectorSubtract(ball->origin, other->origin, diff);
					VectorNormalize2(diff, diffu);

					if (VectorLength(diff) < BALL_SIZE) // collision
					{
						//Com_Printf("%f\n", VectorLength(diff));
						poolball_resolvecollision(ball, other);
					}
				}
			}
		}
	}

	// commit positions to the detail ents
	for(int i = 0; i < BALL_MAX; i++)
	{
		vec3_t org;
		vec3_t finalorg;
		VectorCopy(table->ball[i].origin, org);

		finalorg[0] = (org[0] * sin(DEG2RAD(ang))) + (org[1] * cos(DEG2RAD((ang))));
		finalorg[1] = (org[1] * sin(DEG2RAD(ang))) + (org[0] * cos(DEG2RAD((ang))));
		finalorg[2] = 2;

		VectorAdd(ent->s.origin, finalorg, finalorg);
		VectorCopy(finalorg, table->actor->details[i]->s.origin);
	}
}

void actor_bbox_evaluate(actor_t *actor, int *score);
void SP_actor_pooltable(edict_t *ent)
{
	gi.linkentity(ent);
	ent->physics = pooltable_physics;
	pooltable_t *pooltable = gi.TagMalloc(sizeof(pooltable_t), TAG_LEVEL);
	memset(pooltable, 0, sizeof(pooltable_t));
	ent->data = pooltable;

	pooltable_reset(pooltable);
	pooltable->actor = Actor_Spawn();
	VectorCopy(ent->s.origin, pooltable->actor->origin);
	Actor_Link(pooltable->actor, 0);

	pooltable->actor->evaluate = actor_bbox_evaluate;

	pooltable->actor->mins[0] = pooltable->actor->origin[0] - 512;
	pooltable->actor->mins[1] = pooltable->actor->origin[1] - 512;
	pooltable->actor->mins[2] = pooltable->actor->origin[2] - 32;
	pooltable->actor->maxs[0] = pooltable->actor->origin[0] + 512;
	pooltable->actor->maxs[1] = pooltable->actor->origin[1] + 512;
	pooltable->actor->maxs[2] = pooltable->actor->origin[2] + 512;


	{ // determine table surface size
		vec3_t finalorg;
		vec3_t org;
		int ang = ent->s.angles[YAW] - 90;
		org[0] = (128 * sin(DEG2RAD(ang))) + (64 * cos(DEG2RAD((ang))));
		org[1] = (64 * sin(DEG2RAD(ang))) + (128 * cos(DEG2RAD((ang))));
		org[2] = 2;
		VectorAdd(ent->s.origin, org, finalorg);
		pooltable->mins[0] = min(ent->s.origin[0], finalorg[0]);
		pooltable->mins[1] = min(ent->s.origin[1], finalorg[1]);
		pooltable->mins[2] = min(ent->s.origin[2], finalorg[2]);
		pooltable->maxs[0] = max(ent->s.origin[0], finalorg[0]);
		pooltable->maxs[1] = max(ent->s.origin[1], finalorg[1]);
		pooltable->maxs[2] = max(ent->s.origin[2], finalorg[2]);
	}


	
	for(int i = 0; i < BALL_MAX; i++)
	{
		detail_edict_t *ball = D_Spawn();
		ball->s.modelindex = gi.modelindex("models/props/poolball.md2");
		ball->s.skinnum = i;

		pooltable->actor->details[i] = ball;
	}

}




