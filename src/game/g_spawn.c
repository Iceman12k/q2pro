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

#include "g_local.h"

typedef struct {
    char    *name;
    void (*spawn)(edict_t *ent);
} spawn_func_t;

typedef struct {
    char    *name;
    unsigned ofs;
    fieldtype_t type;
} spawn_field_t;

void SP_item_health(edict_t *self);
void SP_item_health_small(edict_t *self);
void SP_item_health_large(edict_t *self);
void SP_item_health_mega(edict_t *self);

void SP_info_player_start(edict_t *ent);
void SP_info_player_deathmatch(edict_t *ent);
void SP_info_player_coop(edict_t *ent);
void SP_info_player_intermission(edict_t *ent);

void SP_func_plat(edict_t *ent);
void SP_func_rotating(edict_t *ent);
void SP_func_button(edict_t *ent);
void SP_func_door(edict_t *ent);
void SP_func_door_secret(edict_t *ent);
void SP_func_door_rotating(edict_t *ent);
void SP_func_water(edict_t *ent);
void SP_func_train(edict_t *ent);
void SP_func_conveyor(edict_t *self);
void SP_func_wall(edict_t *self);
void SP_func_object(edict_t *self);
void SP_func_explosive(edict_t *self);
void SP_func_timer(edict_t *self);
void SP_func_areaportal(edict_t *ent);
void SP_func_clock(edict_t *ent);
void SP_func_killbox(edict_t *ent);

void SP_trigger_always(edict_t *ent);
void SP_trigger_once(edict_t *ent);
void SP_trigger_multiple(edict_t *ent);
void SP_trigger_relay(edict_t *ent);
void SP_trigger_push(edict_t *ent);
void SP_trigger_hurt(edict_t *ent);
void SP_trigger_key(edict_t *ent);
void SP_trigger_counter(edict_t *ent);
void SP_trigger_elevator(edict_t *ent);
void SP_trigger_gravity(edict_t *ent);
void SP_trigger_monsterjump(edict_t *ent);

void SP_target_temp_entity(edict_t *ent);
void SP_target_speaker(edict_t *ent);
void SP_target_explosion(edict_t *ent);
void SP_target_changelevel(edict_t *ent);
void SP_target_secret(edict_t *ent);
void SP_target_goal(edict_t *ent);
void SP_target_splash(edict_t *ent);
void SP_target_spawner(edict_t *ent);
void SP_target_blaster(edict_t *ent);
void SP_target_crosslevel_trigger(edict_t *ent);
void SP_target_crosslevel_target(edict_t *ent);
void SP_target_laser(edict_t *self);
void SP_target_help(edict_t *ent);
void SP_target_actor(edict_t *ent);
void SP_target_lightramp(edict_t *self);
void SP_target_earthquake(edict_t *ent);
void SP_target_character(edict_t *ent);
void SP_target_string(edict_t *ent);

void SP_worldspawn(edict_t *ent);
void SP_viewthing(edict_t *ent);

void SP_light(edict_t *self);
void SP_light_mine1(edict_t *ent);
void SP_light_mine2(edict_t *ent);
void SP_info_null(edict_t *self);
void SP_info_notnull(edict_t *self);
void SP_path_corner(edict_t *self);
void SP_point_combat(edict_t *self);

void SP_misc_explobox(edict_t *self);
void SP_misc_banner(edict_t *self);
void SP_misc_satellite_dish(edict_t *self);
void SP_misc_actor(edict_t *self);
void SP_misc_gib_arm(edict_t *self);
void SP_misc_gib_leg(edict_t *self);
void SP_misc_gib_head(edict_t *self);
void SP_misc_insane(edict_t *self);
void SP_misc_deadsoldier(edict_t *self);
void SP_misc_viper(edict_t *self);
void SP_misc_viper_bomb(edict_t *self);
void SP_misc_bigviper(edict_t *self);
void SP_misc_strogg_ship(edict_t *self);
void SP_misc_teleporter(edict_t *self);
void SP_misc_teleporter_dest(edict_t *self);
void SP_misc_blackhole(edict_t *self);
void SP_misc_eastertank(edict_t *self);
void SP_misc_easterchick(edict_t *self);
void SP_misc_easterchick2(edict_t *self);

void SP_monster_berserk(edict_t *self);
void SP_monster_gladiator(edict_t *self);
void SP_monster_gunner(edict_t *self);
void SP_monster_infantry(edict_t *self);
void SP_monster_soldier_light(edict_t *self);
void SP_monster_soldier(edict_t *self);
void SP_monster_soldier_ss(edict_t *self);
void SP_monster_tank(edict_t *self);
void SP_monster_medic(edict_t *self);
void SP_monster_flipper(edict_t *self);
void SP_monster_chick(edict_t *self);
void SP_monster_parasite(edict_t *self);
void SP_monster_flyer(edict_t *self);
void SP_monster_brain(edict_t *self);
void SP_monster_floater(edict_t *self);
void SP_monster_hover(edict_t *self);
void SP_monster_mutant(edict_t *self);
void SP_monster_supertank(edict_t *self);
void SP_monster_boss2(edict_t *self);
void SP_monster_makron(edict_t *self);
void SP_monster_jorg(edict_t *self);
void SP_monster_boss3_stand(edict_t *self);

void SP_monster_commander_body(edict_t *self);

void SP_turret_breach(edict_t *self);
void SP_turret_base(edict_t *self);
void SP_turret_driver(edict_t *self);

void SP_monster_infantry_energy(edict_t *self);

static const spawn_func_t spawn_funcs[] = {
    { "item_health", SP_item_health },
    { "item_health_small", SP_item_health_small },
    { "item_health_large", SP_item_health_large },
    { "item_health_mega", SP_item_health_mega },

    { "info_player_start", SP_info_player_start },
    { "info_player_deathmatch", SP_info_player_deathmatch },
    { "info_player_coop", SP_info_player_coop },
    { "info_player_intermission", SP_info_player_intermission },

    { "func_plat", SP_func_plat },
    { "func_button", SP_func_button },
    { "func_door", SP_func_door },
    { "func_door_secret", SP_func_door_secret },
    { "func_door_rotating", SP_func_door_rotating },
    { "func_rotating", SP_func_rotating },
    { "func_train", SP_func_train },
    { "func_water", SP_func_water },
    { "func_conveyor", SP_func_conveyor },
    { "func_areaportal", SP_func_areaportal },
    { "func_clock", SP_func_clock },
    { "func_wall", SP_func_wall },
    { "func_object", SP_func_object },
    { "func_timer", SP_func_timer },
    { "func_explosive", SP_func_explosive },
    { "func_killbox", SP_func_killbox },

    { "trigger_always", SP_trigger_always },
    { "trigger_once", SP_trigger_once },
    { "trigger_multiple", SP_trigger_multiple },
    { "trigger_relay", SP_trigger_relay },
    { "trigger_push", SP_trigger_push },
    { "trigger_hurt", SP_trigger_hurt },
    { "trigger_key", SP_trigger_key },
    { "trigger_counter", SP_trigger_counter },
    { "trigger_elevator", SP_trigger_elevator },
    { "trigger_gravity", SP_trigger_gravity },
    { "trigger_monsterjump", SP_trigger_monsterjump },

    { "target_temp_entity", SP_target_temp_entity },
    { "target_speaker", SP_target_speaker },
    { "target_explosion", SP_target_explosion },
    { "target_changelevel", SP_target_changelevel },
    { "target_secret", SP_target_secret },
    { "target_goal", SP_target_goal },
    { "target_splash", SP_target_splash },
    { "target_spawner", SP_target_spawner },
    { "target_blaster", SP_target_blaster },
    { "target_crosslevel_trigger", SP_target_crosslevel_trigger },
    { "target_crosslevel_target", SP_target_crosslevel_target },
    { "target_laser", SP_target_laser },
    { "target_help", SP_target_help },
    { "target_actor", SP_target_actor },
    { "target_lightramp", SP_target_lightramp },
    { "target_earthquake", SP_target_earthquake },
    { "target_character", SP_target_character },
    { "target_string", SP_target_string },

    { "worldspawn", SP_worldspawn },
    { "viewthing", SP_viewthing },

    { "light", SP_light },
    { "light_mine1", SP_light_mine1 },
    { "light_mine2", SP_light_mine2 },
    { "info_null", SP_info_null },
    { "func_group", SP_info_null },
    { "info_notnull", SP_info_notnull },
    { "path_corner", SP_path_corner },
    { "point_combat", SP_point_combat },

    { "misc_explobox", SP_misc_explobox },
    { "misc_banner", SP_misc_banner },
    { "misc_satellite_dish", SP_misc_satellite_dish },
    { "misc_actor", SP_misc_actor },
    { "misc_gib_arm", SP_misc_gib_arm },
    { "misc_gib_leg", SP_misc_gib_leg },
    { "misc_gib_head", SP_misc_gib_head },
    { "misc_insane", SP_misc_insane },
    { "misc_deadsoldier", SP_misc_deadsoldier },
    { "misc_viper", SP_misc_viper },
    { "misc_viper_bomb", SP_misc_viper_bomb },
    { "misc_bigviper", SP_misc_bigviper },
    { "misc_strogg_ship", SP_misc_strogg_ship },
    { "misc_teleporter", SP_misc_teleporter },
    { "misc_teleporter_dest", SP_misc_teleporter_dest },
    { "misc_blackhole", SP_misc_blackhole },
    { "misc_eastertank", SP_misc_eastertank },
    { "misc_easterchick", SP_misc_easterchick },
    { "misc_easterchick2", SP_misc_easterchick2 },

    { "monster_berserk", SP_monster_berserk },
    { "monster_gladiator", SP_monster_gladiator },
    { "monster_gunner", SP_monster_gunner },
    { "monster_infantry", SP_monster_infantry },
    { "monster_soldier_light", SP_monster_soldier_light },
    { "monster_soldier", SP_monster_soldier },
    { "monster_soldier_ss", SP_monster_soldier_ss },
    { "monster_tank", SP_monster_tank },
    { "monster_tank_commander", SP_monster_tank },
    { "monster_medic", SP_monster_medic },
    { "monster_flipper", SP_monster_flipper },
    { "monster_chick", SP_monster_chick },
    { "monster_parasite", SP_monster_parasite },
    { "monster_flyer", SP_monster_flyer },
    { "monster_brain", SP_monster_brain },
    { "monster_floater", SP_monster_floater },
    { "monster_hover", SP_monster_hover },
    { "monster_mutant", SP_monster_mutant },
    { "monster_supertank", SP_monster_supertank },
    { "monster_boss2", SP_monster_boss2 },
    { "monster_boss3_stand", SP_monster_boss3_stand },
    { "monster_makron", SP_monster_makron },
    { "monster_jorg", SP_monster_jorg },

    { "monster_commander_body", SP_monster_commander_body },

    { "turret_breach", SP_turret_breach },
    { "turret_base", SP_turret_base },
    { "turret_driver", SP_turret_driver },

    // Reki (June 29 2024): New ents
    { "monster_infantry_energy", SP_monster_infantry_energy },

    { NULL, NULL }
};

static const spawn_field_t spawn_fields[] = {
    { "classname", FOFS(classname), F_LSTRING },
    { "model", FOFS(model), F_LSTRING },
    { "spawnflags", FOFS(spawnflags), F_INT },
    { "speed", FOFS(speed), F_FLOAT },
    { "accel", FOFS(accel), F_FLOAT },
    { "decel", FOFS(decel), F_FLOAT },
    { "target", FOFS(target), F_LSTRING },
    { "targetname", FOFS(targetname), F_LSTRING },
    { "pathtarget", FOFS(pathtarget), F_LSTRING },
    { "deathtarget", FOFS(deathtarget), F_LSTRING },
    { "killtarget", FOFS(killtarget), F_LSTRING },
    { "combattarget", FOFS(combattarget), F_LSTRING },
    { "message", FOFS(message), F_LSTRING },
    { "team", FOFS(team), F_LSTRING },
    { "wait", FOFS(wait), F_FLOAT },
    { "delay", FOFS(delay), F_FLOAT },
    { "random", FOFS(random), F_FLOAT },
    { "move_origin", FOFS(move_origin), F_VECTOR },
    { "move_angles", FOFS(move_angles), F_VECTOR },
    { "style", FOFS(style), F_INT },
    { "count", FOFS(count), F_INT },
    { "health", FOFS(health), F_INT },
    { "sounds", FOFS(sounds), F_INT },
    { "light", 0, F_IGNORE },
    { "dmg", FOFS(dmg), F_INT },
    { "mass", FOFS(mass), F_INT },
    { "volume", FOFS(volume), F_FLOAT },
    { "attenuation", FOFS(attenuation), F_FLOAT },
    { "map", FOFS(map), F_LSTRING },
    { "origin", FOFS(s.origin), F_VECTOR },
    { "angles", FOFS(s.angles), F_VECTOR },
    { "angle", FOFS(s.angles), F_ANGLEHACK },

    { NULL }
};

// temp spawn vars -- only valid when the spawn function is called
static const spawn_field_t temp_fields[] = {
    { "lip", STOFS(lip), F_INT },
    { "distance", STOFS(distance), F_INT },
    { "height", STOFS(height), F_INT },
    { "noise", STOFS(noise), F_LSTRING },
    { "pausetime", STOFS(pausetime), F_FLOAT },
    { "item", STOFS(item), F_LSTRING },

    { "gravity", STOFS(gravity), F_LSTRING },
    { "sky", STOFS(sky), F_LSTRING },
    { "skyrotate", STOFS(skyrotate), F_FLOAT },
    { "skyaxis", STOFS(skyaxis), F_VECTOR },
    { "minyaw", STOFS(minyaw), F_FLOAT },
    { "maxyaw", STOFS(maxyaw), F_FLOAT },
    { "minpitch", STOFS(minpitch), F_FLOAT },
    { "maxpitch", STOFS(maxpitch), F_FLOAT },
    { "nextmap", STOFS(nextmap), F_LSTRING },
    { "musictrack", STOFS(musictrack), F_LSTRING },

    { NULL }
};

/*
===============
ED_CallSpawn

Finds the spawn function for the entity and calls it
===============
*/
void ED_CallSpawn(edict_t *ent)
{
    const spawn_func_t *s;
    const gitem_t *item;
    int     i;

    if (!ent->classname) {
        gi.dprintf("ED_CallSpawn: NULL classname\n");
        G_FreeEdict(ent);
        return;
    }

    // check item spawn functions
    for (i = 0, item = itemlist; i < game.num_items; i++, item++) {
        if (!item->classname)
            continue;
        if (!strcmp(item->classname, ent->classname)) {
            // found it
            SpawnItem(ent, item);
            return;
        }
    }

    // check normal spawn functions
    for (s = spawn_funcs; s->name; s++) {
        if (!strcmp(s->name, ent->classname)) {
            // found it
            s->spawn(ent);
            return;
        }
    }

    gi.dprintf("%s doesn't have a spawn function\n", ent->classname);
    G_FreeEdict(ent);
}

/*
=============
ED_NewString
=============
*/
static char *ED_NewString(const char *string)
{
    char    *newb, *new_p;
    int     i, l;

    l = strlen(string) + 1;

    newb = gi.TagMalloc(l, TAG_LEVEL);

    new_p = newb;

    for (i = 0; i < l; i++) {
        if (string[i] == '\\' && i < l - 1) {
            i++;
            if (string[i] == 'n')
                *new_p++ = '\n';
            else
                *new_p++ = '\\';
        } else
            *new_p++ = string[i];
    }

    return newb;
}

/*
===============
ED_ParseField

Takes a key/value pair and sets the binary values
in an edict
===============
*/
static bool ED_ParseField(const spawn_field_t *fields, const char *key, const char *value, byte *b)
{
    const spawn_field_t *f;
    float   v;
    vec3_t  vec;

    for (f = fields; f->name; f++) {
        if (!Q_stricmp(f->name, key)) {
            // found it
            switch (f->type) {
            case F_LSTRING:
                *(char **)(b + f->ofs) = ED_NewString(value);
                break;
            case F_VECTOR:
                if (sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]) != 3) {
                    gi.dprintf("%s: couldn't parse '%s'\n", __func__, key);
                    VectorClear(vec);
                }
                ((float *)(b + f->ofs))[0] = vec[0];
                ((float *)(b + f->ofs))[1] = vec[1];
                ((float *)(b + f->ofs))[2] = vec[2];
                break;
            case F_INT:
                *(int *)(b + f->ofs) = Q_atoi(value);
                break;
            case F_FLOAT:
                *(float *)(b + f->ofs) = atof(value);
                break;
            case F_ANGLEHACK:
                v = atof(value);
                ((float *)(b + f->ofs))[0] = 0;
                ((float *)(b + f->ofs))[1] = v;
                ((float *)(b + f->ofs))[2] = 0;
                break;
            case F_IGNORE:
                break;
            default:
                break;
            }
            return true;
        }
    }
    return false;
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
====================
*/
void ED_ParseEdict(const char **data, edict_t *ent)
{
    bool        init;
    char        *key, *value;

    init = false;
    memset(&st, 0, sizeof(st));

// go through all the dictionary pairs
    while (1) {
        // parse key
        key = COM_Parse(data);
        if (key[0] == '}')
            break;
        if (!*data)
            gi.error("%s: EOF without closing brace", __func__);

        // parse value
        value = COM_Parse(data);
        if (!*data)
            gi.error("%s: EOF without closing brace", __func__);

        if (value[0] == '}')
            gi.error("%s: closing brace without data", __func__);

        init = true;

        // keynames with a leading underscore are used for utility comments,
        // and are immediately discarded by quake
        if (key[0] == '_')
            continue;

        if (!ED_ParseField(spawn_fields, key, value, (byte *)ent)) {
            if (!ED_ParseField(temp_fields, key, value, (byte *)&st)) {
                gi.dprintf("%s: %s is not a field\n", __func__, key);
            }
        }
    }

    if (!init)
        memset(ent, 0, sizeof(*ent));
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.

All but the first will have the FL_TEAMSLAVE flag set.
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams(void)
{
    edict_t *e, *e2, *chain;
    int     i, j;
    int     c, c2;

    c = 0;
    c2 = 0;
    for (i = 1, e = g_edicts + i; i < globals.num_edicts; i++, e++) {
        if (!e->inuse)
            continue;
        if (!e->team)
            continue;
        if (e->flags & FL_TEAMSLAVE)
            continue;
        chain = e;
        e->teammaster = e;
        c++;
        c2++;
        for (j = i + 1, e2 = e + 1; j < globals.num_edicts; j++, e2++) {
            if (!e2->inuse)
                continue;
            if (!e2->team)
                continue;
            if (e2->flags & FL_TEAMSLAVE)
                continue;
            if (!strcmp(e->team, e2->team)) {
                c2++;
                chain->teamchain = e2;
                e2->teammaster = e;
                chain = e2;
                e2->flags |= FL_TEAMSLAVE;
            }
        }
    }

    gi.dprintf("%i teams with %i entities\n", c, c2);
}

/*
==============
G_AddPrecache

Register new global precache function and call it (once).
==============
*/
void G_AddPrecache(void (*func)(void))
{
    precache_t *prec;

    for (prec = game.precaches; prec; prec = prec->next)
        if (prec->func == func)
            return;

    prec = gi.TagMalloc(sizeof(*prec), TAG_GAME);
    prec->func = func;
    prec->next = game.precaches;
    game.precaches = prec;

    prec->func();
}

/*
==============
G_RefreshPrecaches

Called from ReadLevel() to refresh all global precache indices registered by
spawn functions.
==============
*/
void G_RefreshPrecaches(void)
{
    precache_t *prec;

    for (prec = game.precaches; prec; prec = prec->next)
        prec->func();
}

/*
==============
G_FreePrecaches

Free precache functions from previous level.
==============
*/
static void G_FreePrecaches(void)
{
    precache_t *prec, *next;

    for (prec = game.precaches; prec; prec = next) {
        next = prec->next;
        gi.TagFree(prec);
    }

    game.precaches = NULL;
}

/*
==============
SpawnEntities

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.
==============
*/
void SpawnEntities(const char *mapname, const char *entities, const char *spawnpoint)
{
    edict_t     *ent;
    int         inhibit;
    char        *com_token;
    int         i;
    int         skill_level;

    skill_level = Q_clip(skill->value, 0, 3);
    if (skill->value != skill_level)
        gi.cvar_forceset("skill", va("%d", skill_level));

    SaveClientData();

    gi.FreeTags(TAG_LEVEL);

    G_FreePrecaches();

    memset(&level, 0, sizeof(level));
    memset(g_edicts, 0, game.maxentities * sizeof(g_edicts[0]));

    Q_strlcpy(level.mapname, mapname, sizeof(level.mapname));
    Q_strlcpy(game.spawnpoint, spawnpoint, sizeof(game.spawnpoint));

    // set client fields on player ents
    for (i = 0; i < game.maxclients; i++)
        g_edicts[i + 1].client = game.clients + i;

    ent = NULL;
    inhibit = 0;

// parse ents
    while (1) {
        // parse the opening brace
        com_token = COM_Parse(&entities);
        if (!entities)
            break;
        if (com_token[0] != '{')
            gi.error("ED_LoadFromFile: found %s when expecting {", com_token);

        if (!ent)
            ent = g_edicts;
        else
            ent = G_Spawn();
        ED_ParseEdict(&entities, ent);

        // yet another map hack
        if (!Q_stricmp(level.mapname, "command") && !Q_stricmp(ent->classname, "trigger_once") && !Q_stricmp(ent->model, "*27"))
            ent->spawnflags &= ~SPAWNFLAG_NOT_HARD;

        // remove things (except the world) from different skill levels or deathmatch
        if (ent != g_edicts) {
            if (deathmatch->value) {
                if (ent->spawnflags & SPAWNFLAG_NOT_DEATHMATCH) {
                    G_FreeEdict(ent);
                    inhibit++;
                    continue;
                }
            } else {
                if ( /* ((coop->value) && (ent->spawnflags & SPAWNFLAG_NOT_COOP)) || */
                    ((skill->value == 0) && (ent->spawnflags & SPAWNFLAG_NOT_EASY)) ||
                    ((skill->value == 1) && (ent->spawnflags & SPAWNFLAG_NOT_MEDIUM)) ||
                    (((skill->value == 2) || (skill->value == 3)) && (ent->spawnflags & SPAWNFLAG_NOT_HARD))
                ) {
                    G_FreeEdict(ent);
                    inhibit++;
                    continue;
                }
            }

            ent->spawnflags &= ~(SPAWNFLAG_NOT_EASY | SPAWNFLAG_NOT_MEDIUM | SPAWNFLAG_NOT_HARD | SPAWNFLAG_NOT_COOP | SPAWNFLAG_NOT_DEATHMATCH);
        }

        ED_CallSpawn(ent);
    }

    gi.dprintf("%i entities inhibited\n", inhibit);

#ifdef DEBUG
    i = 1;
    ent = EDICT_NUM(i);
    while (i < globals.num_edicts) {
        if (ent->inuse != 0 || ent->inuse != 1)
            Com_DPrintf("Invalid entity %d\n", i);
        i++, ent++;
    }
#endif

    G_FindTeams();

    PlayerTrail_Init();
}

//===================================================================

#if 0
// cursor positioning
xl <value>
xr <value>
yb <value>
yt <value>
xv <value>
yv <value>

// drawing
statpic <name>
pic <stat>
num <fieldwidth> <stat>
string <stat>

// control
if <stat>
ifeq <stat> <value>
ifbit <stat> <value>
endif

#endif

static const char single_statusbar[] =
"yb -24 "

// health
"xv 0 "
"hnum "
"xv 50 "
"pic 0 "

// ammo
"if 2 "
  "xv 100 "
  "anum "
  "xv 150 "
  "pic 2 "
"endif "

// armor
"if 4 "
  "xv 200 "
  "rnum "
  "xv 250 "
  "pic 4 "
"endif "

// selected item
"if 6 "
  "xv 296 "
  "pic 6 "
"endif "

"yb -50 "

// picked up item
"if 7 "
  "xv 0 "
  "pic 7 "
  "xv 26 "
  "yb -42 "
  "stat_string 8 "
  "yb -50 "
"endif "

// timer 1 (quad, enviro, breather)
"if 9 "
  "xv 262 "
  "num 2 10 "
  "xv 296 "
  "pic 9 "
"endif "

// timer 2 (pent)
"if 18 "
  "yb -76 "
  "xv 262 "
  "num 2 19 "
  "xv 296 "
  "pic 18 "
  "yb -50 "
"endif "

// help / weapon icon
"if 11 "
  "xv 148 "
  "pic 11 "
"endif "
;

static const char dm_statusbar[] =
// frags
"xr -50 "
"yt 2 "
"num 3 14 "

// spectator
"if 17 "
  "xv 0 "
  "yb -58 "
  "string2 \"SPECTATOR MODE\" "
"endif "

// chase camera
"if 16 "
  "xv 0 "
  "yb -68 "
  "string \"Chasing\" "
  "xv 64 "
  "stat_string 16 "
"endif "
;

static const char *const lightstyles[] = {
    // 0 normal
    "m",

    // 1 FLICKER (first variety)
    "mmnmmommommnonmmonqnmmo",

    // 2 SLOW STRONG PULSE
    "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba",

    // 3 CANDLE (first variety)
    "mmmmmaaaaammmmmaaaaaabcdefgabcdefg",

    // 4 FAST STROBE
    "mamamamamama",

    // 5 GENTLE PULSE 1
    "jklmnopqrstuvwxyzyxwvutsrqponmlkj",

    // 6 FLICKER (second variety)
    "nmonqnmomnmomomno",

    // 7 CANDLE (second variety)
    "mmmaaaabcdefgmmmmaaaammmaamm",

    // 8 CANDLE (third variety)
    "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa",

    // 9 SLOW STROBE (fourth variety)
    "aaaaaaaazzzzzzzz",

    // 10 FLUORESCENT FLICKER
    "mmamammmmammamamaaamammma",

    // 11 SLOW PULSE NOT FADE TO BLACK
    "abcdefghijklmnopqrrqponmlkjihgfedcba",
};

static void worldspawn_precache(void)
{
    sm_meat_index = gi.modelindex("models/objects/gibs/sm_meat/tris.md2");
}

/*QUAKED worldspawn (0 0 0) ?

Only used for the world.
"sky"   environment map name
"skyaxis"   vector axis for rotating sky
"skyrotate" speed of rotation in degrees/second
"sounds"    music cd track number
"gravity"   800 is default gravity
"message"   text to print at user logon
*/
void SP_worldspawn(edict_t *ent)
{
    ent->movetype = MOVETYPE_PUSH;
    ent->solid = SOLID_BSP;
    ent->inuse = true;          // since the world doesn't use G_Spawn()
    ent->s.modelindex = 1;      // world model is always index 1

    //---------------

    // reserve some spots for dead player bodies for coop / deathmatch
    InitBodyQue();

    // set configstrings for items
    SetItemNames();

    if (st.nextmap)
        Q_strlcpy(level.nextmap, st.nextmap, sizeof(level.nextmap));

    // make some data visible to the server

    if (ent->message && ent->message[0]) {
        gi.configstring(CS_NAME, ent->message);
        Q_strlcpy(level.level_name, ent->message, sizeof(level.level_name));
    } else
        Q_strlcpy(level.level_name, level.mapname, sizeof(level.level_name));

    if (st.sky && st.sky[0])
        gi.configstring(CS_SKY, st.sky);
    else
        gi.configstring(CS_SKY, "unit1_");

    gi.configstring(CS_SKYROTATE, va("%f", st.skyrotate));

    gi.configstring(CS_SKYAXIS, va("%f %f %f",
                                   st.skyaxis[0], st.skyaxis[1], st.skyaxis[2]));

    if (st.musictrack && st.musictrack[0])
        gi.configstring(CS_CDTRACK, st.musictrack);
    else
        gi.configstring(CS_CDTRACK, va("%i", ent->sounds));

    gi.configstring(game.csr.maxclients, va("%i", game.maxclients));

    // status bar program
    if (deathmatch->value)
        gi.configstring(CS_STATUSBAR, va("%s%s", single_statusbar, dm_statusbar));
    else
        gi.configstring(CS_STATUSBAR, single_statusbar);

    //---------------

    // help icon for statusbar
    gi.imageindex("i_help");
    level.pic_health = gi.imageindex("i_health");
    gi.imageindex("help");
    gi.imageindex("field_3");

    if (!st.gravity)
        gi.cvar_set("sv_gravity", "800");
    else
        gi.cvar_set("sv_gravity", st.gravity);

    gi.soundindex("player/fry.wav");  // standing in lava / slime
    gi.soundindex("player/lava_in.wav");
    gi.soundindex("player/burn1.wav");
    gi.soundindex("player/burn2.wav");
    gi.soundindex("player/drown1.wav");

    PrecacheItem(FindItem("Blaster"));

    gi.soundindex("player/lava1.wav");
    gi.soundindex("player/lava2.wav");

    gi.soundindex("misc/pc_up.wav");
    gi.soundindex("misc/talk.wav");
    gi.soundindex("misc/talk1.wav");

    gi.soundindex("misc/udeath.wav");

    // gibs
    gi.soundindex("items/respawn1.wav");

    // sexed sounds
    gi.soundindex("*death1.wav");
    gi.soundindex("*death2.wav");
    gi.soundindex("*death3.wav");
    gi.soundindex("*death4.wav");
    gi.soundindex("*fall1.wav");
    gi.soundindex("*fall2.wav");
    gi.soundindex("*gurp1.wav");        // drowning damage
    gi.soundindex("*gurp2.wav");
    gi.soundindex("*jump1.wav");        // player jump
    gi.soundindex("*pain25_1.wav");
    gi.soundindex("*pain25_2.wav");
    gi.soundindex("*pain50_1.wav");
    gi.soundindex("*pain50_2.wav");
    gi.soundindex("*pain75_1.wav");
    gi.soundindex("*pain75_2.wav");
    gi.soundindex("*pain100_1.wav");
    gi.soundindex("*pain100_2.wav");

    // sexed models
    // THIS ORDER MUST MATCH THE DEFINES IN g_local.h
    // you can add more, max 15
    gi.modelindex("#w_blaster.md2");
    gi.modelindex("#w_shotgun.md2");
    gi.modelindex("#w_sshotgun.md2");
    gi.modelindex("#w_machinegun.md2");
    gi.modelindex("#w_chaingun.md2");
    gi.modelindex("#a_grenades.md2");
    gi.modelindex("#w_glauncher.md2");
    gi.modelindex("#w_rlauncher.md2");
    gi.modelindex("#w_hyperblaster.md2");
    gi.modelindex("#w_railgun.md2");
    gi.modelindex("#w_bfg.md2");

    //-------------------

    gi.soundindex("player/gasp1.wav");      // gasping for air
    gi.soundindex("player/gasp2.wav");      // head breaking surface, not gasping

    gi.soundindex("player/watr_in.wav");    // feet hitting water
    gi.soundindex("player/watr_out.wav");   // feet leaving water

    gi.soundindex("player/watr_un.wav");    // head going underwater

    gi.soundindex("player/u_breath1.wav");
    gi.soundindex("player/u_breath2.wav");

    gi.soundindex("items/pkup.wav");        // bonus item pickup
    gi.soundindex("world/land.wav");        // landing thud
    gi.soundindex("misc/h2ohit1.wav");      // landing splash

    gi.soundindex("items/damage.wav");
    gi.soundindex("items/protect.wav");
    gi.soundindex("items/protect4.wav");
    gi.soundindex("weapons/noammo.wav");

    gi.soundindex("infantry/inflies1.wav");

    gi.modelindex("models/objects/gibs/sm_meat/tris.md2");
    gi.modelindex("models/objects/gibs/arm/tris.md2");
    gi.modelindex("models/objects/gibs/bone/tris.md2");
    gi.modelindex("models/objects/gibs/bone2/tris.md2");
    gi.modelindex("models/objects/gibs/chest/tris.md2");
    gi.modelindex("models/objects/gibs/skull/tris.md2");
    gi.modelindex("models/objects/gibs/head2/tris.md2");

    G_AddPrecache(worldspawn_precache);

    // Reki (June 28 2024): Our initialization here
    D_Initialize();
    //

//
// Setup light animation tables. 'a' is total darkness, 'z' is doublebright.
//
    for (int i = 0; i < q_countof(lightstyles); i++)
        gi.configstring(game.csr.lights + i, lightstyles[i]);

    // styles 32-62 are assigned by the light program for switchable lights

    // 63 testing
    gi.configstring(game.csr.lights + 63, "a");
}
