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
/*
==============================================================================

BERSERK

==============================================================================
*/

#include "g_local.h"
#include "m_berserk.h"

static int sound_pain;
static int sound_die;
static int sound_idle;
static int sound_punch;
static int sound_sight;
static int sound_search;
static int model_wounded_noarm_r;
static int model_normal;

void berserk_sight(edict_t *self, edict_t *other)
{
    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void berserk_search(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void berserk_fidget(edict_t *self);
static const mframe_t berserk_frames_stand[] = {
    { ai_stand, 0, berserk_fidget },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL }
};
const mmove_t berserk_move_stand = {FRAME_stand1, FRAME_stand5, berserk_frames_stand, NULL};

void berserk_stand(edict_t *self)
{
    self->monsterinfo.currentmove = &berserk_move_stand;
}

static const mframe_t berserk_frames_stand_fidget[] = {
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL },
    { ai_stand, 0, NULL }
};
const mmove_t berserk_move_stand_fidget = {FRAME_standb1, FRAME_standb20, berserk_frames_stand_fidget, berserk_stand};

void berserk_fidget(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        return;
    if (random() > 0.15f)
        return;

    self->monsterinfo.currentmove = &berserk_move_stand_fidget;
    gi.sound(self, CHAN_WEAPON, sound_idle, 1, ATTN_IDLE, 0);
}

static const mframe_t berserk_frames_walk[] = {
    { ai_walk, 9.1, NULL },
    { ai_walk, 6.3, NULL },
    { ai_walk, 4.9, NULL },
    { ai_walk, 6.7, NULL },
    { ai_walk, 6.0, NULL },
    { ai_walk, 8.2, NULL },
    { ai_walk, 7.2, NULL },
    { ai_walk, 6.1, NULL },
    { ai_walk, 4.9, NULL },
    { ai_walk, 4.7, NULL },
    { ai_walk, 4.7, NULL },
    { ai_walk, 4.8, NULL }
};
const mmove_t berserk_move_walk = {FRAME_walkc1, FRAME_walkc11, berserk_frames_walk, NULL};

void berserk_walk(edict_t *self)
{
    self->monsterinfo.currentmove = &berserk_move_walk;
}

/*

  *****************************
  SKIPPED THIS FOR NOW!
  *****************************

   Running -> Arm raised in air

void()  berserk_runb1   =[  $r_att1 ,   berserk_runb2   ] {{ ai_run(21);};
void()  berserk_runb2   =[  $r_att2 ,   berserk_runb3   ] {{ ai_run(11);};
void()  berserk_runb3   =[  $r_att3 ,   berserk_runb4   ] {{ ai_run(21);};
void()  berserk_runb4   =[  $r_att4 ,   berserk_runb5   ] {{ ai_run(25);};
void()  berserk_runb5   =[  $r_att5 ,   berserk_runb6   ] {{ ai_run(18);};
void()  berserk_runb6   =[  $r_att6 ,   berserk_runb7   ] {{ ai_run(19);};
// running with arm in air : start loop
void()  berserk_runb7   =[  $r_att7 ,   berserk_runb8   ] {{ ai_run(21);};
void()  berserk_runb8   =[  $r_att8 ,   berserk_runb9   ] {{ ai_run(11);};
void()  berserk_runb9   =[  $r_att9 ,   berserk_runb10  ] {{ ai_run(21);};
void()  berserk_runb10  =[  $r_att10 ,  berserk_runb11  ] {{ ai_run(25);};
void()  berserk_runb11  =[  $r_att11 ,  berserk_runb12  ] {{ ai_run(18);};
void()  berserk_runb12  =[  $r_att12 ,  berserk_runb7   ] {{ ai_run(19);};
// running with arm in air : end loop
*/

#if 0
static const mframe_t berserk_frames_run1[] = {
    { ai_run, 21, NULL },
    { ai_run, 11, NULL },
    { ai_run, 21, NULL },
    { ai_run, 25, NULL },
    { ai_run, 18, NULL },
    { ai_run, 19, NULL }
};
#else // Reki (June 28 2024): faster run speed
static const mframe_t berserk_frames_run1[] = {
    { ai_run, 26, NULL },
    { ai_run, 16, NULL },
    { ai_run, 26, NULL },
    { ai_run, 30, NULL },
    { ai_run, 23, NULL },
    { ai_run, 24, NULL }
};
#endif
const mmove_t berserk_move_run1 = {FRAME_run1, FRAME_run6, berserk_frames_run1, NULL};

void berserk_run(edict_t *self);
static const mframe_t berserk_frames_run2[] = {
    { ai_run, 9.1, NULL },
    { ai_run, 6.3, NULL },
    { ai_run, 4.9, NULL },
    { ai_run, 6.7, NULL },
    { ai_run, 6.0, NULL },
    { ai_run, 8.2, NULL },
    { ai_run, 7.2, NULL },
    { ai_run, 6.1, NULL },
    { ai_run, 4.9, NULL },
    { ai_run, 4.7, NULL },
    { ai_run, 4.7, NULL },
    { ai_run, 4.8, NULL }
};
const mmove_t berserk_move_run2 = {FRAME_walkc1, FRAME_walkc11, berserk_frames_run2, berserk_run};

void berserk_run(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        self->monsterinfo.currentmove = &berserk_move_stand;
    else
    {
        self->monsterinfo.currentmove = &berserk_move_run1;
        if (self->s.modelindex == model_wounded_noarm_r)
        {
            if (self->health < self->max_health * 0.5)
            {
                self->monsterinfo.currentmove = &berserk_move_run2;
                if (!self->dmg_bleedout_frame)
                {
                    self->dmg_bleedout_frame = level.framenum + (BASE_FRAMERATE * (3 + (random() * 12)));
                }
                else if (level.framenum > self->dmg_bleedout_frame)
                {
                    self->dmg_bleedout_frame = 0;
                    T_Damage(self, world, world, vec3_origin, vec3_origin, vec3_origin, self->health, 0, DAMAGE_NO_KNOCKBACK | DAMAGE_NO_ARMOR, MOD_UNKNOWN);
                }
            }
        }
    }
}

void berserk_attack_spike(edict_t *self)
{
    vec3_t  aim = { MELEE_DISTANCE, 0, -24 };

    fire_hit(self, aim, (14 + (Q_rand() % 6)), 400);    //  Faster attack -- upwards and backwards
}

void berserk_swing(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_punch, 1, ATTN_NORM, 0);
}

static const mframe_t berserk_frames_attack_spike[] = {
    { ai_charge, 0, NULL },
    { ai_charge, 0, berserk_swing },
    { ai_charge, 0, berserk_attack_spike },
    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL }
};
const mmove_t berserk_move_attack_spike = {FRAME_att_c2, FRAME_att_c8, berserk_frames_attack_spike, berserk_run};

void berserk_attack_club(edict_t *self)
{
    vec3_t  aim = { MELEE_DISTANCE, self->mins[0], -4 };

    fire_hit(self, aim, (8 + (Q_rand() % 6)), 400);     // Slower attack
}

static const mframe_t berserk_frames_attack_club[] = {
    { ai_charge, 0, berserk_swing },
    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL },
    { ai_charge, 0, berserk_attack_club },
    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL }
};
const mmove_t berserk_move_attack_club = {FRAME_att_c12, FRAME_att_c20, berserk_frames_attack_club, berserk_run};

static const mframe_t berserk_frames_attack_club_run[] = {
    { ai_charge, 32, NULL },
    { ai_charge, 22, NULL },
    { ai_charge, 32, NULL },
    { ai_charge, 36, NULL },
    { ai_charge, 29, NULL },
    { ai_charge, 30, NULL },
    { ai_charge, 32, NULL },
    { ai_charge, 22, NULL},
    { ai_charge, 32, NULL },
    { ai_charge, 36, NULL },
    { ai_charge, 29, NULL },
    { ai_charge, 30, NULL },
    { ai_charge, 32, NULL },
    { ai_move, 22, berserk_swing },
    { ai_move, 32, NULL },
    { ai_move, 36, berserk_attack_club },
    { ai_move, 29, NULL },
    { ai_move, 30, NULL },
    { ai_charge, 32, NULL }
};
const mmove_t berserk_move_attack_club_run = {FRAME_r_attb1, FRAME_r_attb18, berserk_frames_attack_club_run, berserk_run};

void berserk_strike(edict_t *self)
{
    //FIXME play impact sound
}

static const mframe_t berserk_frames_attack_strike[] = {
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, berserk_swing },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, berserk_strike },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 9.7, NULL },
    { ai_move, 13.6, NULL }
};

const mmove_t berserk_move_attack_strike = {FRAME_att_c21, FRAME_att_c34, berserk_frames_attack_strike, berserk_run};

void berserk_melee(edict_t *self)
{
    if (self->s.modelindex == model_wounded_noarm_r)
    {
        self->monsterinfo.currentmove = &berserk_move_attack_club;
    }
    else
    {
        if ((Q_rand() % 2) == 0)
            self->monsterinfo.currentmove = &berserk_move_attack_spike;
        else
            self->monsterinfo.currentmove = &berserk_move_attack_club;
    }
}

void berserk_overhead_melee(edict_t *self)
{
    if (!FacingIdeal(self))
        return;
    
    if (self->monsterinfo.currentmove != &berserk_move_run1)
        return;

    if (!self->enemy)
        return;
    
    vec3_t enemydist_v;
    VectorSubtract(self->s.origin, self->enemy->s.origin, enemydist_v);
    float enemydist = VectorLength(enemydist_v);
    if (enemydist < (MELEE_DISTANCE * 2))
        return;
    if (enemydist > (MELEE_DISTANCE * 4))
        return;

    self->monsterinfo.currentmove = &berserk_move_attack_club_run;
}

/*
void()  berserk_atke1   =[  $r_attb1,   berserk_atke2   ] {{ ai_run(9);};
void()  berserk_atke2   =[  $r_attb2,   berserk_atke3   ] {{ ai_run(6);};
void()  berserk_atke3   =[  $r_attb3,   berserk_atke4   ] {{ ai_run(18.4);};
void()  berserk_atke4   =[  $r_attb4,   berserk_atke5   ] {{ ai_run(25);};
void()  berserk_atke5   =[  $r_attb5,   berserk_atke6   ] {{ ai_run(14);};
void()  berserk_atke6   =[  $r_attb6,   berserk_atke7   ] {{ ai_run(20);};
void()  berserk_atke7   =[  $r_attb7,   berserk_atke8   ] {{ ai_run(8.5);};
void()  berserk_atke8   =[  $r_attb8,   berserk_atke9   ] {{ ai_run(3);};
void()  berserk_atke9   =[  $r_attb9,   berserk_atke10  ] {{ ai_run(17.5);};
void()  berserk_atke10  =[  $r_attb10,  berserk_atke11  ] {{ ai_run(17);};
void()  berserk_atke11  =[  $r_attb11,  berserk_atke12  ] {{ ai_run(9);};
void()  berserk_atke12  =[  $r_attb12,  berserk_atke13  ] {{ ai_run(25);};
void()  berserk_atke13  =[  $r_attb13,  berserk_atke14  ] {{ ai_run(3.7);};
void()  berserk_atke14  =[  $r_attb14,  berserk_atke15  ] {{ ai_run(2.6);};
void()  berserk_atke15  =[  $r_attb15,  berserk_atke16  ] {{ ai_run(19);};
void()  berserk_atke16  =[  $r_attb16,  berserk_atke17  ] {{ ai_run(25);};
void()  berserk_atke17  =[  $r_attb17,  berserk_atke18  ] {{ ai_run(19.6);};
void()  berserk_atke18  =[  $r_attb18,  berserk_run1    ] {{ ai_run(7.8);};
*/

static const mframe_t berserk_frames_pain1[] = {
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL }
};
const mmove_t berserk_move_pain1 = {FRAME_painc1, FRAME_painc4, berserk_frames_pain1, berserk_run};

static const mframe_t berserk_frames_pain2[] = {
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL }
};
const mmove_t berserk_move_pain2 = {FRAME_painb1, FRAME_painb20, berserk_frames_pain2, berserk_run};

static const mframe_t berserk_frames_pain_r_armblow[] = {
    { ai_move, 21, NULL },
    { ai_move, 11, NULL },
    { ai_move, 21, NULL },
    { ai_run, 25, NULL },
    { ai_run, 18, NULL },
    { ai_run, 19, NULL }
};
const mmove_t berserk_move_pain_r_armblow = {FRAME_pain_r_armblowoff1, FRAME_pain_r_armblowoff6, berserk_frames_pain_r_armblow, berserk_run};

static actor_t* berserk_actor_spawn(edict_t *self);
void berserk_pain(edict_t *self, edict_t *other, float kick, int damage)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum = 1;

    if (self->s.modelindex == model_normal)
    {
        if (self->dmg_taken_shred >= (self->max_health / 2))
        {
            self->dmg_taken_shred = 0; // reset shreddage
            self->health = self->max_health;
            self->s.skinnum = 0;
            self->s.modelindex = model_wounded_noarm_r;

            if (self->monsterinfo.currentmove == &berserk_move_run1 || self->monsterinfo.currentmove == &berserk_move_run2 || self->monsterinfo.currentmove == &berserk_move_attack_club_run)
            {
                self->monsterinfo.currentmove = &berserk_move_pain_r_armblow;
            }
            else
            {
                self->monsterinfo.currentmove = &berserk_move_pain2;
            }
            berserk_actor_spawn(self);
            return;
        }
    }

    if (level.framenum < self->pain_debounce_framenum)
        return;
    
    if (self->monsterinfo.currentmove == &berserk_move_attack_club_run) // don't interrupt the big swing
        return;

    self->pain_debounce_framenum = level.framenum + 3 * BASE_FRAMERATE;
    gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

    if (skill->value == 3)
        return;     // no pain anims in nightmare

    if ((damage < 20) || (random() < 0.5f))
        self->monsterinfo.currentmove = &berserk_move_pain1;
    else
        self->monsterinfo.currentmove = &berserk_move_pain2;
}

void berserk_nonsolid(edict_t *self)
{
    self->svflags |= SVF_DEADMONSTER;
    gi.linkentity(self);
}

void berserk_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, -8);
    self->movetype = MOVETYPE_TOSS;
    self->svflags |= SVF_DEADMONSTER;
    self->nextthink = 0;
    gi.linkentity(self);
}

static const mframe_t berserk_frames_death1[] = {
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, berserk_nonsolid },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL }

};
const mmove_t berserk_move_death1 = {FRAME_death1, FRAME_death13, berserk_frames_death1, berserk_dead};

static const mframe_t berserk_frames_death2[] = {
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, berserk_nonsolid },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL },
    { ai_move, 0, NULL }
};
const mmove_t berserk_move_death2 = {FRAME_deathc1, FRAME_deathc8, berserk_frames_death2, berserk_dead};

void berserk_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
    int     n;

    if (self->health <= self->gib_health) {
        gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
        for (n = 0; n < 2; n++)
            ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
        for (n = 0; n < 4; n++)
            ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
        ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
        self->deadflag = DEAD_DEAD;
        return;
    }

    if (self->deadflag == DEAD_DEAD)
        return;

    gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
    self->deadflag = DEAD_DEAD;
    self->takedamage = DAMAGE_YES;

    if (damage >= 50)
        self->monsterinfo.currentmove = &berserk_move_death1;
    else
        self->monsterinfo.currentmove = &berserk_move_death2;
}

static void berserk_precache(void)
{
    sound_pain   = gi.soundindex("berserk/berpain2.wav");
    sound_die    = gi.soundindex("berserk/berdeth2.wav");
    sound_idle   = gi.soundindex("berserk/beridle1.wav");
    sound_punch  = gi.soundindex("berserk/attack.wav");
    sound_search = gi.soundindex("berserk/bersrch1.wav");
    sound_sight  = gi.soundindex("berserk/sight.wav");

    model_normal = gi.modelindex("models/monsters/berserk/normal.md2");
    model_wounded_noarm_r = gi.modelindex("models/monsters/berserk/wounded_1.md2");
}

#define BERSERK_MAX_DRIPS 4
#define BERSERK_ACTOR_DRIP_START 0
int berserk_actor_physics(actor_t *actor)
{
    edict_t *self = actor->owner;
    
    // dead berserkers don't need to drip
    if (self->health < 0)
    {
        Actor_Free(actor);
        self->actor = NULL;
    }

    // spawn a new drip?
    if (FRAMESYNC)
    {
        if (random() < 0.1 && self->monsterinfo.currentmove == &berserk_move_run2)
        {
            actor->cnt++;
            actor->cnt %= BERSERK_MAX_DRIPS;

            detail_edict_t *drip = actor->details[BERSERK_ACTOR_DRIP_START + actor->cnt];
            drip->type = 1;
            
            vec3_t fwd, right;
            AngleVectors(self->s.angles, fwd, right, NULL);
            VectorCopy(self->s.origin, drip->s.origin);
            drip->s.origin[2] += 26;
            VectorMA(drip->s.origin, 10, right, drip->s.origin);
            VectorCopy(drip->s.origin, drip->s.old_origin);
            VectorClear(drip->velocity);
            VectorMA(vec3_origin, 8 + (random() * 32), right, drip->velocity);
        }
    }
    

    for(int i = 0; i < BERSERK_MAX_DRIPS; i++)
    {
        detail_edict_t *drip = actor->details[BERSERK_ACTOR_DRIP_START + i];
        if (!drip->type)
            continue;
        
        vec3_t end;
        trace_t trace;
        drip->velocity[2] -= sv_gravity->value * FRAMETIME * 0.25;
        VectorMA(drip->s.origin, FRAMETIME, drip->velocity, end);
        trace = gi.trace(drip->s.origin, vec3_origin, vec3_origin, end, world, MASK_SOLID);
        VectorCopy(trace.endpos, drip->s.origin);

        if (trace.fraction < 0.1)
            drip->type = 0;
    }

    // relink for culling
    VectorCopy(self->s.origin, actor->origin);
    Actor_Link(actor, 256);
}

int berserk_actor_addtoscene(actor_t *actor, int score)
{
    for(int i = 0; i < BERSERK_MAX_DRIPS; i++)
    {
        detail_edict_t *drip = actor->details[BERSERK_ACTOR_DRIP_START + i];
        if (drip == NULL) // no drip, whoops!
            break;
        
        if (!drip->type)
            continue;
        
        Actor_AddDetail(actor, drip);
    }

    return true;
}

static actor_t* berserk_actor_spawn(edict_t *self)
{
    // spawn our actor for drippage
    if (self->actor)
        return;

    actor_t *a = Actor_Spawn();
    self->actor = a;
    a->owner = self;
    a->aphysics = berserk_actor_physics;
    a->aaddtoscene = berserk_actor_addtoscene;

    for(int i = 0; i < BERSERK_MAX_DRIPS; i++)
    {
        detail_edict_t *drip = D_Spawn();
        a->details[BERSERK_ACTOR_DRIP_START + i] = drip;
        drip->s.modelindex = null_model;
        drip->s.effects = EF_GIB;
        drip->type = 0;
    }
}

/*QUAKED monster_berserk (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_berserk(edict_t *self)
{
    if (deathmatch->value) {
        G_FreeEdict(self);
        return;
    }

    // pre-caches
    G_AddPrecache(berserk_precache);
    self->s.modelindex = model_normal;
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, 32);
    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;

    self->health = 220;
    self->gib_health = -90;
    self->mass = 250;

    self->pain = berserk_pain;
    self->die = berserk_die;

    self->monsterinfo.stand = berserk_stand;
    self->monsterinfo.walk = berserk_walk;
    self->monsterinfo.run = berserk_run;
    self->monsterinfo.dodge = NULL;
    self->monsterinfo.attack = berserk_overhead_melee;
    self->monsterinfo.melee = berserk_melee;
    self->monsterinfo.sight = berserk_sight;
    self->monsterinfo.search = berserk_search;

    self->monsterinfo.currentmove = &berserk_move_stand;
    self->monsterinfo.scale = MODEL_SCALE;

    gi.linkentity(self);

    walkmonster_start(self);
}
