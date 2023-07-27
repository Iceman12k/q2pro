
#include "g_local.h"

typedef struct {
	char    *name;
	void(*spawn)(edict_t *ent);
} spawn_func_t;

typedef struct {
	char    *name;
	unsigned ofs;
	fieldtype_t type;
} spawn_field_t;

void SP_worldspawn(edict_t *ent);

static const spawn_func_t spawn_funcs[] = {
	{"worldspawn", SP_worldspawn},

	{NULL, NULL}
};

static const spawn_field_t spawn_fields[] = {
	{"classname", FOFS(classname), F_LSTRING},
	{"model", FOFS(model), F_LSTRING},
	{"spawnflags", FOFS(spawnflags), F_INT},
	{"speed", FOFS(speed), F_FLOAT},
	{"accel", FOFS(accel), F_FLOAT},
	{"decel", FOFS(decel), F_FLOAT},
	{"target", FOFS(target), F_LSTRING},
	{"targetname", FOFS(targetname), F_LSTRING},
	{"pathtarget", FOFS(pathtarget), F_LSTRING},
	{"deathtarget", FOFS(deathtarget), F_LSTRING},
	{"killtarget", FOFS(killtarget), F_LSTRING},
	{"combattarget", FOFS(combattarget), F_LSTRING},
	{"message", FOFS(message), F_LSTRING},
	{"random", FOFS(random), F_FLOAT},
	{"style", FOFS(style), F_INT},
	{"health", FOFS(health), F_INT},
	{"light", 0, F_IGNORE},
	{"volume", FOFS(volume), F_FLOAT},
	{"attenuation", FOFS(attenuation), F_FLOAT},
	{"origin", FOFS(s.origin), F_VECTOR},
	{"angles", FOFS(s.angles), F_VECTOR},
	{"angle", FOFS(s.angles), F_ANGLEHACK},

	{NULL}
};

// temp spawn vars -- only valid when the spawn function is called
static const spawn_field_t temp_fields[] = {
	{"lip", STOFS(lip), F_INT},
	{"distance", STOFS(distance), F_INT},
	{"height", STOFS(height), F_INT},
	{"noise", STOFS(noise), F_LSTRING},
	{"pausetime", STOFS(pausetime), F_FLOAT},
	{"item", STOFS(item), F_LSTRING},

	{"gravity", STOFS(gravity), F_LSTRING},
	{"sky", STOFS(sky), F_LSTRING},
	{"skyrotate", STOFS(skyrotate), F_FLOAT},
	{"skyaxis", STOFS(skyaxis), F_VECTOR},
	{"minyaw", STOFS(minyaw), F_FLOAT},
	{"maxyaw", STOFS(maxyaw), F_FLOAT},
	{"minpitch", STOFS(minpitch), F_FLOAT},
	{"maxpitch", STOFS(maxpitch), F_FLOAT},
	{"nextmap", STOFS(nextmap), F_LSTRING},
	{"musictrack", STOFS(musictrack), F_LSTRING},

	{NULL}
};

/*
=================
G_InitEdict
=================
*/

void G_InitEdict(edict_t *e)
{
	e->inuse = true;
	e->classname = "noclass";
	e->gravity = 1.0f;
	e->s.number = e - g_edicts;
}

/*
=================
G_Spawn

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t *G_Spawn(void)
{
	int         i;
	edict_t     *e;

	e = &g_edicts[game.maxclients + 1];
	for (i = game.maxclients + 1; i < globals.num_edicts; i++, e++) {
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!e->inuse && (e->freetime < 2 || level.time - e->freetime > 0.5f)) {
			G_InitEdict(e);
			return e;
		}
	}

	if (i == game.maxentities)
		gi.error("ED_Alloc: no free edicts");

	globals.num_edicts++;
	G_InitEdict(e);
	return e;
}

/*
=================
G_FreeEdict

Marks the edict as free
=================
*/
void G_FreeEdict(edict_t *ed)
{
	gi.unlinkentity(ed);        // unlink from world

	memset(ed, 0, sizeof(*ed));
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = false;
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
	char        *com_token;
	int         i;

	gi.FreeTags(TAG_LEVEL);

	memset(&level, 0, sizeof(level));
	memset(g_edicts, 0, game.maxentities * sizeof(g_edicts[0]));

	Q_strlcpy(level.mapname, mapname, sizeof(level.mapname));
	Q_strlcpy(game.spawnpoint, spawnpoint, sizeof(game.spawnpoint));

	// set client fields on player ents
	for (i = 0; i < game.maxclients; i++)
		g_edicts[i + 1].client = game.clients + i;

	ent = NULL;

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

		ED_CallSpawn(ent);
	}
}

/*
===============
ED_CallSpawn

Finds the spawn function for the entity and calls it
===============
*/
void ED_CallSpawn(edict_t *ent)
{
	const spawn_func_t *s;

	if (!ent->classname) {
		gi.dprintf("ED_CallSpawn: NULL classname\n");
		return;
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
		}
		else
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
				*(int *)(b + f->ofs) = atoi(value);
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




