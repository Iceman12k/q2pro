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

#pragma once

#include "shared/list.h"
#include "common/bsp.h"

//
// game.h -- game dll information visible to server
//

#if AQTION_EXTENSION
#define GAME_API_VERSION        4
#else
#define GAME_API_VERSION        3
#endif

// edict->svflags

#define SVF_NOCLIENT            BIT(0)      // don't send entity to clients, even if it has effects
#define SVF_DEADMONSTER         BIT(1)      // treat as CONTENTS_DEADMONSTER for collision
#define SVF_MONSTER             BIT(2)      // only used by server as entity priority hint

#if USE_PROTOCOL_EXTENSIONS
#define SVF_PLAYER              BIT(3)      // treat as CONTENTS_PLAYER for collision
#define SVF_BOT                 BIT(4)
#define SVF_NOBOTS              BIT(5)
#define SVF_RESPAWNING          BIT(6)
#define SVF_PROJECTILE          BIT(7)      // treat as CONTENTS_PROJECTILE for collision
#define SVF_INSTANCED           BIT(8)
#define SVF_DOOR                BIT(9)
#define SVF_NOCULL              BIT(10)     // always send entity to clients (no PVS checks)
#define SVF_HULL                BIT(11)
#endif

// edict->solid values

typedef enum {
    SOLID_NOT,          // no interaction with other objects
    SOLID_TRIGGER,      // only touch when inside, after moving
    SOLID_BBOX,         // touch on edge
    SOLID_BSP           // bsp clip, touch on edge
} solid_t;

// extended features

// R1Q2 and Q2PRO specific
#define GMF_CLIENTNUM               BIT(0)      // game sets clientNum gclient_s field
#define GMF_PROPERINUSE             BIT(1)      // game maintains edict_s inuse field properly
#define GMF_MVDSPEC                 BIT(2)      // game is dummy MVD client aware
#define GMF_WANT_ALL_DISCONNECTS    BIT(3)      // game wants ClientDisconnect() for non-spawned clients

// Q2PRO specific
#define GMF_ENHANCED_SAVEGAMES      BIT(10)     // game supports safe/portable savegames
#define GMF_VARIABLE_FPS            BIT(11)     // game supports variable server FPS
#define GMF_EXTRA_USERINFO          BIT(12)     // game wants extra userinfo after normal userinfo
#define GMF_IPV6_ADDRESS_AWARE      BIT(13)     // game supports IPv6 addresses
#define GMF_ALLOW_INDEX_OVERFLOW    BIT(14)     // game wants PF_FindIndex() to return 0 on overflow
#define GMF_PROTOCOL_EXTENSIONS     BIT(15)     // game supports protocol extensions

//===============================================================

#define MAX_ENT_CLUSTERS    16

// link_t is only used for entity area links now
typedef struct link_s
{
  struct link_s *prev, *next;
}
link_t;

typedef struct edict_s edict_t;
typedef struct gclient_s gclient_t;

#ifndef GAME_INCLUDE

struct gclient_s {
    player_state_t  ps;     // communicated by server to clients
    int             ping;

    // set to (client POV entity number) - 1 by game,
    // only valid if g_features has GMF_CLIENTNUM bit
    int             clientNum;

    // the game dll can add anything it wants after
    // this point in the structure

#if AQTION_EXTENSION
	cvarsyncvalue_t cl_cvar[CVARSYNC_MAX];
#endif
};

struct edict_s {
    entity_state_t  s;
    struct gclient_s    *client;
    qboolean    inuse;
    int         linkcount;

    // FIXME: move these fields to a server private sv_entity_t
    list_t      area;               // linked to a division node or leaf

    int         num_clusters;       // if -1, use headnode instead
    int         clusternums[MAX_ENT_CLUSTERS];
    int         headnode;           // unused if num_clusters != -1
    int         areanum, areanum2;

    //================================

    int         svflags;            // SVF_NOCLIENT, SVF_DEADMONSTER, SVF_MONSTER, etc
    vec3_t      mins, maxs;
    vec3_t      absmin, absmax, size;
    solid_t     solid;
    int         clipmask;
    edict_t     *owner;

    //================================

#if USE_PROTOCOL_EXTENSIONS
    // extra entity state communicated to clients
    // only valid if g_features has GMF_PROTOCOL_EXTENSIONS bit
    entity_state_extension_t    x;
#endif

    // the game dll can add anything it wants after
    // this point in the structure
};

#endif      // GAME_INCLUDE

//===============================================================

// action start
// making real copies for bot compatibility
extern void (*real_cprintf) (struct edict_s * ent, int printlevel, const char *fmt, ...);
extern void (*real_centerprintf) (struct edict_s * ent, const char *fmt, ...);

//
// functions provided by the main engine
//
typedef struct {
    // special messages
    void (* q_printf(2, 3) bprintf)(int printlevel, const char *fmt, ...);
    void (* q_printf(1, 2) dprintf)(const char *fmt, ...);
    void (* q_printf(3, 4) cprintf)(edict_t *ent, int printlevel, const char *fmt, ...);
    void (* q_printf(2, 3) centerprintf)(edict_t *ent, const char *fmt, ...);
    void (*sound)(edict_t *ent, int channel, int soundindex, float volume, float attenuation, float timeofs);
    void (*positioned_sound)(const vec3_t origin, edict_t *ent, int channel, int soundindex, float volume, float attenuation, float timeofs);

    // config strings hold all the index strings, the lightstyles,
    // and misc data like the sky definition and cdtrack.
    // All of the current configstrings are sent to clients when
    // they connect, and changes are sent to all connected clients.
    void (*configstring)(int num, const char *string);

    void (* q_noreturn_ptr q_printf(1, 2) error)(const char *fmt, ...);

    // the *index functions create configstrings and some internal server state
    int (*modelindex)(const char *name);
    int (*soundindex)(const char *name);
    int (*imageindex)(const char *name);

    void (*setmodel)(edict_t *ent, const char *name);

    // collision detection
    trace_t (* q_gameabi trace)(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, edict_t *passent, int contentmask);
    int (*pointcontents)(const vec3_t point);
    qboolean (*inPVS)(const vec3_t p1, const vec3_t p2);
    qboolean (*inPHS)(const vec3_t p1, const vec3_t p2);
    void (*SetAreaPortalState)(int portalnum, qboolean open);
    qboolean (*AreasConnected)(int area1, int area2);

    // an entity will never be sent to a client or used for collision
    // if it is not passed to linkentity.  If the size, position, or
    // solidity changes, it must be relinked.
    void (*linkentity)(edict_t *ent);
    void (*unlinkentity)(edict_t *ent);     // call before removing an interactive edict
    int (*BoxEdicts)(const vec3_t mins, const vec3_t maxs, edict_t **list, int maxcount, int areatype);
    void (*Pmove)(pmove_t *pmove);          // player movement code common with client prediction

    // network messaging
    void (*multicast)(const vec3_t origin, multicast_t to);
    void (*unicast)(edict_t *ent, qboolean reliable);
    void (*WriteChar)(int c);
    void (*WriteByte)(int c);
    void (*WriteShort)(int c);
    void (*WriteLong)(int c);
    void (*WriteFloat)(float f);
    void (*WriteString)(const char *s);
    void (*WritePosition)(const vec3_t pos);    // some fractional bits
    void (*WriteDir)(const vec3_t pos);         // single byte encoded, very coarse
    void (*WriteAngle)(float f);

    // managed memory allocation
    void *(*TagMalloc)(unsigned size, unsigned tag);
    void (*TagFree)(void *block);
    void (*FreeTags)(unsigned tag);

    // console variable interaction
    cvar_t *(*cvar)(const char *var_name, const char *value, int flags);
    cvar_t *(*cvar_set)(const char *var_name, const char *value);
    cvar_t *(*cvar_forceset)(const char *var_name, const char *value);

    // ClientCommand and ServerCommand parameter access
    int (*argc)(void);
    char *(*argv)(int n);
    char *(*args)(void);     // concatenation of all argv >= 1

    // add commands to the server console as if they were typed in
    // for map changing, etc
    void (*AddCommandString)(const char *text);

    void (*DebugGraph)(float value, int color);

#if AQTION_EXTENSION
	void *(*CheckForExtension)(char *text);
#endif
//rekkie -- BSP -- s
    bsp_t* (*Bsp)(void);
    //rekkie -- BSP -- e
    //rekkie -- surface data -- s
    nav_t* (*Nav)(void);
    //rekkie -- debug drawing -- s
#if DEBUG_DRAWING
    debug_draw_t* (*Draw)(void);
//#if USE_REF
//    void (*GL_DrawArrow)(vec3_t start, vec3_t end, const uint32_t color, float line_width);
//#endif
#endif
    surface_data_t* (*SurfaceData)(void);
    //rekkie -- surface data -- e

    //rekkie -- Fake Bot Client -- s
    void (*SV_BotUpdateInfo)(char* name, int ping, int score);
    void (*SV_BotConnect)(char* name);
    void (*SV_BotDisconnect)(char* name);
    void (*SV_BotClearClients)(void);
    //rekkie -- Fake Bot Client -- e
} game_import_t;
//
// functions exported by the game subsystem
//
typedef struct {
    int         apiversion;

    // the init function will only be called when a game starts,
    // not each time a level is loaded.  Persistant data for clients
    // and the server can be allocated in init
    void (*Init)(void);
    void (*Shutdown)(void);

    // each new level entered will cause a call to SpawnEntities
    void (*SpawnEntities)(const char *mapname, const char *entstring, const char *spawnpoint);

    // Read/Write Game is for storing persistant cross level information
    // about the world state and the clients.
    // WriteGame is called every time a level is exited.
    // ReadGame is called on a loadgame.
    void (*WriteGame)(const char *filename, qboolean autosave);
    void (*ReadGame)(const char *filename);

    // ReadLevel is called after the default map information has been
    // loaded with SpawnEntities
    void (*WriteLevel)(const char *filename);
    void (*ReadLevel)(const char *filename);

    qboolean (*ClientConnect)(edict_t *ent, char *userinfo);
    void (*ClientBegin)(edict_t *ent);
    void (*ClientUserinfoChanged)(edict_t *ent, char *userinfo);
    void (*ClientDisconnect)(edict_t *ent);
    void (*ClientCommand)(edict_t *ent);
    void (*ClientThink)(edict_t *ent, usercmd_t *cmd);

    void (*RunFrame)(void);

    // ServerCommand will be called when an "sv <command>" command is issued on the
    // server console.
    // The game can issue gi.argc() / gi.argv() commands to get the rest
    // of the parameters
    void (*ServerCommand)(void);

#if AQTION_EXTENSION
	void* (*FetchGameExtension)(char *name);
#endif


    //
    // global variables shared between game and server
    //

    // The edict array is allocated in the game dll so it
    // can vary in size from one game to another.
    //
    // The size will be fixed when ge->Init() is called
    struct edict_s  *edicts;
    int         edict_size;
    int         num_edicts;     // current number, <= max_edicts
    int         max_edicts;
} game_export_t;

typedef game_export_t *(*game_entry_t)(game_import_t *);

//===============================================================

/*
 * GetGameAPIEx() is guaranteed to be called after GetGameAPI() and before
 * ge->Init().
 *
 * Unlike GetGameAPI(), passed game_import_ex_t * is valid as long as game
 * library is loaded. Pointed to structure can be used directly without making
 * a copy of it. If copying is neccessary, no more than structsize bytes must
 * be copied.
 *
 * New fields can be safely added at the end of game_import_ex_t and
 * game_export_ex_t structures, provided GAME_API_VERSION_EX is also bumped.
 *
 * API version history:
 * 1 - Initial release.
 * 2 - Added CustomizeEntity().
 * 3 - Added EntityVisibleToClient(), renamed CustomizeEntity() to
 * CustomizeEntityToClient() and changed the meaning of return value.
 */

#define GAME_API_VERSION_EX_MINIMUM             1
#define GAME_API_VERSION_EX_CUSTOMIZE_ENTITY    2
#define GAME_API_VERSION_EX_ENTITY_VISIBLE      3
#define GAME_API_VERSION_EX                     3

typedef enum {
    VIS_PVS     = 0,
    VIS_PHS     = 1,
    VIS_NOAREAS = 2     // can be OR'ed with one of above
} vis_t;

typedef struct {
    entity_state_t s;
#if USE_PROTOCOL_EXTENSIONS
    entity_state_extension_t x;
#endif
} customize_entity_t;

typedef struct {
    uint32_t    apiversion;
    uint32_t    structsize;

    void        (*local_sound)(edict_t *target, const vec3_t origin, edict_t *ent, int channel, int soundindex, float volume, float attenuation, float timeofs);
    const char  *(*get_configstring)(int index);
    trace_t     (*q_gameabi clip)(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, edict_t *clip, int contentmask);
    qboolean    (*inVIS)(const vec3_t p1, const vec3_t p2, vis_t vis);

    void        *(*GetExtension)(const char *name);
    void        *(*TagRealloc)(void *ptr, size_t size);
} game_import_ex_t;

typedef struct {
    uint32_t    apiversion;
    uint32_t    structsize;

    void        *(*GetExtension)(const char *name);
    qboolean    (*CanSave)(void);
    void        (*PrepFrame)(void);
    void        (*RestartFilesystem)(void); // called when fs_restart is issued
    qboolean    (*CustomizeEntityToClient)(edict_t *client, edict_t *ent, customize_entity_t *temp); // if true is returned, `temp' must be initialized
    qboolean    (*EntityVisibleToClient)(edict_t *client, edict_t *ent);
} game_export_ex_t;

typedef const game_export_ex_t *(*game_entry_ex_t)(const game_import_ex_t *);
