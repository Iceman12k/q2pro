

#include "g_local.h"

enum poolball_e {
	BALL_CUE,
	BALL_SOLID,
	BALL_EIGHT = 8,
	BALL_STRIPE,
	BALL_MAX = 16,
	POOLCUE = BALL_MAX,
};

typedef struct {
	vec3_t origin;
	vec3_t velocity;
	float frame;
	qboolean sunk;
} poolball_t;

typedef struct {
	actor_t *actor;
	poolball_t ball[BALL_MAX + 1];
	edict_t *user;
	vec3_t mins;
	vec3_t maxs;

	uint16_t balls_active;

	float cue_angle;
	float cue_lastdot;

	float place_holdtime;
	int place_valid;
	vec3_t place_org;

	int reset_frame;
} pooltable_t;

#define POOL_STOPSPEED 2.1
#define POOL_FRICTION 0.48
#define POOL_WALLDAMPING 0.87
#define POOL_BALLDAMPING 0.98
#define BALL_RADIUS 1.9
#define BALL_SIZE BALL_RADIUS * 2

#define BALL_ANIM_CHUNK 16

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

	table->reset_frame = 0;
	table->balls_active = 0x0000;
	
	for(int i = 0; i < BALL_MAX; i++)
	{
		VectorCopy(ballpositions[i], table->ball[i].origin);
		VectorClear(table->ball[i].velocity);
		table->ball[i].sunk = false;
	}

	//VectorSet(table->ball[BALL_CUE].velocity, 170 + (random() * 8), (random() - 0.5) * 20, 0);
}

void poolball_resolvecollision(poolball_t *b1, poolball_t *b2)
{
	vec3_t diff, diffu;
	VectorSubtract(b2->origin, b1->origin, diff);
	VectorNormalize2(diff, diffu);

	#if 0
	vec3_t aggr_vel;
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

qboolean poolball_pocketcheck(poolball_t *ball)
{
	vec3_t pocket_bounds[] = {
		// top right pocket
		{0, 64},
		{4, 68},
		{-4, 60},
		{0, 68},

		// bottom right
		{0, -4},
		{4, 0},
		{0, 4},
		{-4, 0},

		// bottom left
		{124, -4},
		{128, 0},
		{128, -4},
		{132, 4},

		// top left
		{124, 64},
		{128, 68},
		{128, 60},
		{132, 64},

		// center top
		{60, 128},
		{66, 132},

		// center bottom
		{60, -4},
		{66, 0},
	};

	float radius = BALL_RADIUS + 0.7;
	vec3_t ball_points[] = {
		{ball->origin[0], ball->origin[1]},
		{ball->origin[0] - radius, ball->origin[1] - radius},
		{ball->origin[0] + radius, ball->origin[1] - radius},
		{ball->origin[0] - radius, ball->origin[1] + radius},
		{ball->origin[0] + radius, ball->origin[1] + radius}
	};

	for(int i = 0; i < sizeof(pocket_bounds)/sizeof(pocket_bounds[0]); i += 2)
	{
		// shitty aabb test, isn't really good
		for(int j = 0; j < sizeof(ball_points)/sizeof(ball_points[0]); j++)
		{
			if ((ball_points[j][0] > pocket_bounds[i][0] && ball_points[j][1] > pocket_bounds[i][1]) &&
				(ball_points[j][0] < pocket_bounds[i + 1][0] && ball_points[j][1] < pocket_bounds[i + 1][1]))
			{
				return true;
			}
		}
	}

	return false;
}

void pooltable_user(edict_t *tent, pooltable_t *table)
{
	int ang = tent->s.angles[YAW] - 90;

	if (!table->user->inuse) // uhoh, they got removed!
	{
		table->user = NULL;
		table->cue_lastdot = 0;
		return;
	}
	
	if (table->user->deadflag != DEAD_NO) // uhoh, they got killed!
	{
		table->user->client->using_vehicle = false;
		table->user = NULL;
		table->cue_lastdot = 0;
		return;
	}

	if (!(table->user->client->cmd_buttons & BUTTON_ATTACK))
	{
		table->user->client->using_vehicle = false;
		table->user = NULL;
		table->cue_lastdot = 0;
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

	vec3_t rel_org;
	VectorAdd3(contact, ent->s.origin, client->ps.viewoffset, contact);
	VectorSubtract(contact, tent->s.origin, contact);

	// fgsfds :( too complicated for me just hack it
	// when I try to make a pooltable later on a different orientation I'll cry
	RotatePointAroundVector(rel_org, plate_norm, contact, -(ang - 90));
	rel_org[0] *= -1; // uh... :/
	
	if (table->ball[BALL_CUE].sunk) // place mode
	{
		if (rel_org[0] < 8 || rel_org[0] > 120 || rel_org[1] < 8 || rel_org[1] > 56)
		{
			table->place_valid = false;
			return;
		}

		table->place_valid = true;
		table->place_holdtime += FRAMETIME;
		VectorCopy(rel_org, table->ball[BALL_CUE].origin);
		VectorCopy(rel_org, table->place_org);
	}
	else // shoot mode
	{
		vec3_t cue_oldorg;

		clamp(rel_org[0], 1, 127);
		clamp(rel_org[1], 1, 63);
		VectorCopy(table->ball[POOLCUE].origin, cue_oldorg);
		VectorCopy(rel_org, table->ball[POOLCUE].origin);
		
		if (VectorLength(table->ball[BALL_CUE].velocity) < 1)
		{
			vec3_t forward, right;
			vec3_t cue_ang;
			vec3_t diff;
			float dot, rdot;
			VectorSet(cue_ang, 0, -table->cue_angle, 0);
			VectorSubtract(table->ball[POOLCUE].origin, table->ball[BALL_CUE].origin, diff);
			AngleVectors(cue_ang, forward, right, NULL);

			dot = DotProduct(forward, diff) + BALL_RADIUS;
			rdot = DotProduct(right, diff);

			if (dot >= 0 && table->cue_lastdot < 0 && fabs(rdot) < BALL_RADIUS)
			{
				VectorSubtract(table->ball[POOLCUE].origin, cue_oldorg, diff);
				VectorScale(diff, 1.0/FRAMETIME, diff);
				float mag = VectorLength(diff);
				float linedot = 0;

				//VectorMA(table->ball[POOLCUE].origin, dot, forward, diff);
				VectorSubtract(table->ball[BALL_CUE].origin, cue_oldorg, diff);
				VectorNormalize(diff);

				VectorMA(table->ball[BALL_CUE].velocity, mag, diff, table->ball[BALL_CUE].velocity);
				table->ball[BALL_CUE].velocity[2] = 0;
				table->balls_active |= 0x0001;
			}

			table->cue_lastdot = dot;
		}
	}
}

void pooltable_physics(edict_t *ent)
{
	pooltable_t *table = ent->data;
	int ang = ent->s.angles[YAW] - 90;

	if (table->reset_frame && level.framenum > table->reset_frame)
		pooltable_reset(table);

	// check for players trying to interact
	if (!table->user)
	{
		vec3_t table_center;
		VectorAdd(table->mins, table->maxs, table_center);
		VectorScale(table_center, 0.5, table_center);
		
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

			vec3_t diff;
			VectorSubtract(pl->s.origin, table_center, diff);
			if (VectorLength(diff) > 150)
				continue;

			//Com_Printf("User taken!\n");
			table->user = pl;
			pl->client->using_vehicle = true;

			table->cue_angle = pl->client->ps.viewangles[YAW] - ent->s.angles[YAW];
		}
	}

	edict_t *o_user = table->user;
	if (table->user)
	{
		pooltable_user(ent, table);
	}

	if (table->ball[BALL_CUE].sunk)
	{
		if (!table->user)
		{
			if (o_user && table->place_holdtime <= 0.2 && table->place_valid)
			{
				table->ball[BALL_CUE].sunk = false;
				VectorCopy(table->place_org, table->ball[BALL_CUE].origin);
			}
			table->place_holdtime = 0;
		}
		
		if (table->place_valid)
		{
			table->actor->details[BALL_CUE]->s.event = EV_OTHER_TELEPORT;
		}
	}
	//

	float phys_step = FRAMETIME;
	while(phys_step > 0)
	{
		float delta = min(phys_step, 0.01);
		phys_step -= delta;
		// run physics for the balls
		for(int i = 0; i < BALL_MAX; i++)
		{	
			poolball_t *ball = &table->ball[i];
			if (ball->sunk)
				continue;

			if (!(table->balls_active & (1 << i)))
				continue;

			// add velocity to origin
			VectorMA(ball->origin, delta, ball->velocity, ball->origin);
			
			// friction
			vec3_t friction;
			VectorScale(ball->velocity, delta * POOL_FRICTION, friction);
			VectorSubtract(ball->velocity, friction, ball->velocity);

			// stop speed first
			if (VectorLength(ball->velocity) < POOL_STOPSPEED)
			{
				table->balls_active &= ~(1 << i);
				VectorClear(ball->velocity);
			}

			// table collision
			if (poolball_pocketcheck(ball))
			{
				ball->sunk = true;
				VectorClear(ball->velocity);
				table->balls_active &= ~(1 << i);

				if (i == BALL_EIGHT)
					table->reset_frame = level.framenum + (FRAMEDIV * 30);
				continue;
			}
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
					if (other->sunk)
						continue;

					vec3_t diff;
					vec3_t diffu;
					VectorSubtract(ball->origin, other->origin, diff);
					VectorNormalize2(diff, diffu);

					if (VectorLength(diff) < BALL_SIZE) // collision
					{
						//Com_Printf("%f\n", VectorLength(diff));
						poolball_resolvecollision(ball, other);
						table->balls_active |= (1 << i);
						table->balls_active |= (1 << j);
					}
				}
			}
		}
	}

	// commit positions to the detail ents
	int sunk_count = 0;
	for(int i = 0; i <= BALL_MAX; i++)
	{
		if (i < BALL_MAX)
		{
			if (table->ball[i].sunk && (i != BALL_CUE || !(table->user && table->place_valid)))
			{
				//if (i == BALL_CUE)
				//	continue;

				vec3_t sunk_org;
				VectorCopy(ent->s.origin, sunk_org);
				sunk_org[2] -= 22;
				sunk_org[0] += 4 + (sunk_count++ * 4);
				sunk_org[1] -= 12;

				//table->actor->details[i]->s.modelindex = 0;
				VectorCopy(sunk_org, table->actor->details[i]->s.origin);
				table->actor->details[i]->s.event = EV_OTHER_TELEPORT;
				continue;
			}
			
			if (table->actor->details[i]->s.modelindex == 0) // fix null models
				table->actor->details[i]->s.modelindex = gi.modelindex("models/props/poolball.md2");

			// ball circum is 12.566
			// 8 frames in a rotation
			// vel / 12.566 * 8?
			float vel_length = VectorLength(table->ball[i].velocity);
			vec3_t vel_norm; VectorNormalize2(table->ball[i].velocity, vel_norm);
			float anim_delta = (vel_length / 12.566) * BALL_ANIM_CHUNK * FRAMETIME;
			table->ball[i].frame += min(anim_delta, (BALL_ANIM_CHUNK / 4));

			if (vel_length > 2 && FRAMESYNC)
			{
				if (vel_norm[0] > 0.935)
				{
					table->actor->details[i]->s.frame = ((int)(-table->ball[i].frame) % BALL_ANIM_CHUNK) + BALL_ANIM_CHUNK;
				}
				else if (vel_norm[0] < -0.935)
				{
					table->actor->details[i]->s.frame = ((int)table->ball[i].frame % BALL_ANIM_CHUNK);
				}
				else if (vel_norm[1] < -0.935)
				{
					table->actor->details[i]->s.frame = ((int)(-table->ball[i].frame) % BALL_ANIM_CHUNK) + (BALL_ANIM_CHUNK) + BALL_ANIM_CHUNK;
				}
				else if (vel_norm[1] > 0.935)
				{
					table->actor->details[i]->s.frame = ((int)table->ball[i].frame % BALL_ANIM_CHUNK) + (BALL_ANIM_CHUNK);
				}
				else if (vel_norm[0] < 0 && vel_norm[1] > 0)
				{
					table->actor->details[i]->s.frame = ((int)table->ball[i].frame % BALL_ANIM_CHUNK) + (BALL_ANIM_CHUNK * 2);
				}
				else if (vel_norm[0] > 0 && vel_norm[1] > 0)
				{
					table->actor->details[i]->s.frame = ((int)(-table->ball[i].frame) % BALL_ANIM_CHUNK) + (BALL_ANIM_CHUNK * 3) + BALL_ANIM_CHUNK;
				}
				else if (vel_norm[0] < 0 && vel_norm[1] < 0)
				{
					table->actor->details[i]->s.frame = ((int)table->ball[i].frame % BALL_ANIM_CHUNK) + (BALL_ANIM_CHUNK * 3);
				}
				else if (vel_norm[0] > 0 && vel_norm[1] < 0)
				{
					table->actor->details[i]->s.frame = ((int)(-table->ball[i].frame) % BALL_ANIM_CHUNK) + (BALL_ANIM_CHUNK * 2) + BALL_ANIM_CHUNK;
				}
			}

			table->ball[i].origin[2] = 0;
			table->ball[i].velocity[2] = 0;
			VectorCopy(ent->s.angles, table->actor->details[i]->s.angles);
			table->actor->details[i]->s.angles[YAW] += 180;
		}
		else if (i == POOLCUE)
		{
			if (!table->user || table->ball[BALL_CUE].sunk) 
			{
				table->actor->details[i]->s.modelindex = 0;
				continue;
			}
			
			if (table->actor->details[i]->s.modelindex == 0) // fix null models
			{	
				VectorCopy(table->actor->details[i]->s.origin, table->actor->details[i]->s.old_origin);
				table->actor->details[i]->s.modelindex = gi.modelindex("models/props/poolcue.md2");
			}

			VectorCopy(ent->s.angles, table->actor->details[i]->s.angles);
			table->actor->details[i]->s.angles[YAW] += table->cue_angle;
			table->actor->details[i]->s.angles[YAW] = roundf(table->actor->details[i]->s.angles[YAW]);
		}

		vec3_t org;
		vec3_t finalorg;
		VectorCopy(table->ball[i].origin, org);

		finalorg[0] = (org[0] * sin(DEG2RAD(ang))) + (org[1] * cos(DEG2RAD((ang))));
		finalorg[1] = (org[1] * sin(DEG2RAD(ang))) + (org[0] * cos(DEG2RAD((ang))));
		finalorg[2] = 2;

		if (table->ball[i].sunk)
			finalorg[2] += 3;

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

	// spawn pool cue
	detail_edict_t *cue = D_Spawn();
	cue->s.modelindex = gi.modelindex("models/props/poolcue.md2");
	pooltable->actor->details[POOLCUE] = cue;

}




