/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
// g_phys.c

#include "g_local.h"

/*


pushmove objects do not obey gravity, and do not interact with each other or trigger fields, but block normal movement and push normal objects when they move.

onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects

doors, plats, etc are SOLID_BSP, and MOVETYPE_PUSH
bonus items are SOLID_TRIGGER touch, and MOVETYPE_TOSS
corpses are SOLID_NOT and MOVETYPE_TOSS
crates are SOLID_BBOX and MOVETYPE_TOSS
walking monsters are SOLID_SLIDEBOX and MOVETYPE_STEP
flying/floating monsters are SOLID_SLIDEBOX and MOVETYPE_FLY

solid_edge items only clip against bsp models.

*/


/*
============
SV_TestEntityPosition

============
*/
edict_t *SV_TestEntityPosition(edict_t *ent)
{
    trace_t trace;
    int     mask;

    if (ent->clipmask)
        mask = ent->clipmask;
    else
        mask = MASK_SOLID;
    trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, ent->s.origin, ent, mask);

    if (trace.startsolid)
        return g_edicts;

    return NULL;
}


/*
================
SV_CheckVelocity
================
*/
void SV_CheckVelocity(edict_t *ent)
{
    int     i;

//
// bound velocity
//
    for (i = 0 ; i < 3 ; i++) {
        if (ent->velocity[i] > sv_maxvelocity->value)
            ent->velocity[i] = sv_maxvelocity->value;
        else if (ent->velocity[i] < -sv_maxvelocity->value)
            ent->velocity[i] = -sv_maxvelocity->value;
    }
}

/*
=============
SV_RunThink

Runs thinking code for this frame if necessary
=============
*/
bool SV_RunThink(edict_t *ent)
{
    int     thinktime;

    thinktime = ent->nextthink;
    if (thinktime <= 0)
        return true;
    if (thinktime > level.framenum)
        return true;

    ent->nextthink = 0;
    if (!ent->think)
        gi.error("NULL ent->think");
    ent->think(ent);

    return false;
}

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
void SV_Impact(edict_t *e1, trace_t *trace)
{
    edict_t     *e2;
//  cplane_t    backplane;

    e2 = trace->ent;

    if (e1->touch && e1->solid != SOLID_NOT)
        e1->touch(e1, e2, &trace->plane, trace->surface);

    if (e2->touch && e2->solid != SOLID_NOT)
        e2->touch(e2, e1, NULL, NULL);
}


/*
==================
ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
#define STOP_EPSILON    0.1f

int ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
    float   backoff;
    float   change;
    int     i, blocked;

    blocked = 0;
    if (normal[2] > 0)
        blocked |= 1;       // floor
    if (!normal[2])
        blocked |= 2;       // step

    backoff = DotProduct(in, normal) * overbounce;

    for (i = 0 ; i < 3 ; i++) {
        change = normal[i] * backoff;
        out[i] = in[i] - change;
        if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
            out[i] = 0;
    }

    return blocked;
}


/*
============
SV_FlyMove

The basic solid body movement clip that slides along multiple planes
Returns the clipflags if the velocity was modified (hit something solid)
1 = floor
2 = wall / step
4 = dead stop
============
*/
#define MAX_CLIP_PLANES 5
int SV_FlyMove(edict_t *ent, float time, int mask)
{
    edict_t     *hit;
    int         bumpcount, numbumps;
    vec3_t      dir;
    float       d;
    int         numplanes;
    vec3_t      planes[MAX_CLIP_PLANES];
    vec3_t      primal_velocity, original_velocity, new_velocity;
    int         i, j;
    trace_t     trace;
    vec3_t      end;
    float       time_left;
    int         blocked;

    numbumps = 4;

    blocked = 0;
    VectorCopy(ent->velocity, original_velocity);
    VectorCopy(ent->velocity, primal_velocity);
    numplanes = 0;

    time_left = time;

    ent->groundentity = NULL;
    for (bumpcount = 0 ; bumpcount < numbumps ; bumpcount++) {
        for (i = 0 ; i < 3 ; i++)
            end[i] = ent->s.origin[i] + time_left * ent->velocity[i];

        trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, end, ent, mask);

        if (trace.allsolid) {
            // entity is trapped in another solid
            VectorClear(ent->velocity);
            return 3;
        }

        if (trace.fraction > 0) {
            // actually covered some distance
            VectorCopy(trace.endpos, ent->s.origin);
            VectorCopy(ent->velocity, original_velocity);
            numplanes = 0;
        }

        if (trace.fraction == 1)
            break;     // moved the entire distance

        hit = trace.ent;

        if (trace.plane.normal[2] > 0.7f) {
            blocked |= 1;       // floor
            if (hit->solid == SOLID_BSP) {
                ent->groundentity = hit;
                ent->groundentity_linkcount = hit->linkcount;
            }
        }
        if (!trace.plane.normal[2]) {
            blocked |= 2;       // step
        }

//
// run the impact function
//
        SV_Impact(ent, &trace);
        if (!ent->inuse)
            break;      // removed by the impact function


        time_left -= time_left * trace.fraction;

        // cliped to another plane
        if (numplanes >= MAX_CLIP_PLANES) {
            // this shouldn't really happen
            VectorClear(ent->velocity);
            return 3;
        }

        VectorCopy(trace.plane.normal, planes[numplanes]);
        numplanes++;

//
// modify original_velocity so it parallels all of the clip planes
//
        for (i = 0 ; i < numplanes ; i++) {
            ClipVelocity(original_velocity, planes[i], new_velocity, 1);

            for (j = 0 ; j < numplanes ; j++)
                if ((j != i) && !VectorCompare(planes[i], planes[j])) {
                    if (DotProduct(new_velocity, planes[j]) < 0)
                        break;  // not ok
                }
            if (j == numplanes)
                break;
        }

        if (i != numplanes) {
            // go along this plane
            VectorCopy(new_velocity, ent->velocity);
        } else {
            // go along the crease
            if (numplanes != 2) {
//              gi.dprintf ("clip velocity, numplanes == %i\n",numplanes);
                VectorClear(ent->velocity);
                return 7;
            }
            CrossProduct(planes[0], planes[1], dir);
            d = DotProduct(dir, ent->velocity);
            VectorScale(dir, d, ent->velocity);
        }

//
// if original velocity is against the original velocity, stop dead
// to avoid tiny occilations in sloping corners
//
        if (DotProduct(ent->velocity, primal_velocity) <= 0) {
            VectorClear(ent->velocity);
            return blocked;
        }
    }

    return blocked;
}


/*
============
SV_AddGravity

============
*/
void SV_AddGravity(edict_t *ent)
{
    ent->velocity[2] -= ent->gravity * sv_gravity->value * FRAMETIME;
}

/*
===============================================================================

PUSHMOVE

===============================================================================
*/

/*
============
SV_PushEntity

Does not change the entities velocity at all
============
*/
trace_t SV_PushEntity(edict_t *ent, vec3_t push)
{
    trace_t trace;
    vec3_t  start;
    vec3_t  end;
    int     mask;

    VectorCopy(ent->s.origin, start);
    VectorAdd(start, push, end);

retry:
    if (ent->clipmask)
        mask = ent->clipmask;
    else
        mask = MASK_SOLID;

    trace = gi.trace(start, ent->mins, ent->maxs, end, ent, mask);

    VectorCopy(trace.endpos, ent->s.origin);
    gi.linkentity(ent);

    if (trace.fraction != 1.0f) {
        SV_Impact(ent, &trace);

        // if the pushed entity went away and the pusher is still there
        if (!trace.ent->inuse && ent->inuse) {
            // move the pusher back and try again
            VectorCopy(start, ent->s.origin);
            gi.linkentity(ent);
            goto retry;
        }
    }

    //if (ent->inuse)
    //    G_TouchTriggers(ent);

    return trace;
}

//FIXME: hacked in for E3 demo
#define sv_stopspeed        100
#define sv_friction         6
#define sv_waterfriction    1

void SV_AddRotationalFriction(edict_t *ent)
{
    int     n;
    float   adjustment;

    VectorMA(ent->s.angles, FRAMETIME, ent->avelocity, ent->s.angles);
    adjustment = FRAMETIME * sv_stopspeed * sv_friction;
    for (n = 0; n < 3; n++) {
        if (ent->avelocity[n] > 0) {
            ent->avelocity[n] -= adjustment;
            if (ent->avelocity[n] < 0)
                ent->avelocity[n] = 0;
        } else {
            ent->avelocity[n] += adjustment;
            if (ent->avelocity[n] > 0)
                ent->avelocity[n] = 0;
        }
    }
}

void SV_Friction(edict_t *ent)
{
	float       *vel;
    float       speed, newspeed, control;
    float       friction;

	vel = ent->velocity;
	speed = sqrtf(vel[0] * vel[0] + vel[1] * vel[1]);
	if (speed) {
		friction = sv_friction;

		control = speed < sv_stopspeed ? sv_stopspeed : speed;
		newspeed = speed - FRAMETIME * control * friction;

		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		vel[0] *= newspeed;
		vel[1] *= newspeed;
	}
}
