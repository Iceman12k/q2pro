
#include "g_local.h"

#define INVEN_WIDTH		3
#define INVEN_HEIGHT	3
#define INVEN_HOTBAR	5
edict_t *inven_slot[INVEN_WIDTH][INVEN_HEIGHT];
edict_t *inven_hotbar[INVEN_HOTBAR];

static float Slot_Predraw(edict_t *v, edict_t *e, entity_state_t *s)
{
	gclient_t *client = v->client;
	if (!client->inv_open)
		return false;

	vec3_t forward, right, up;
	vec3_t inv_dir;
	VectorSet(inv_dir, 0, client->inv_angle, 0);
	VectorCopy(inv_dir, s->angles);
	AngleVectors(inv_dir, forward, right, up);

	for (int i = 0; i < 3; i++) {
		s->origin[i] = SHORT2COORD(client->ps.pmove.origin[i]);
	}
	VectorAdd(s->origin, client->ps.viewoffset, s->origin);

	int x = e->movedir[0] - 2;
	int y = e->movedir[1] - 1;

	VectorMA(s->origin, 48, forward, s->origin);
	VectorMA(s->origin, x * 8, right, s->origin);
	VectorMA(s->origin, y * 8, up, s->origin);
	VectorCopy(s->origin, s->old_origin);

	s->renderfx = RF_FULLBRIGHT | RF_NOSHADOW | RF_DEPTHHACK;

	return true;
}

void I_Initialize(void)
{
	for (int x = 0; x < INVEN_WIDTH; x++)
	{
		for (int y = 0; y < INVEN_HEIGHT; y++)
		{
			edict_t *slot = G_Spawn();
			inven_slot[x][y] = slot;
			
			slot->classname = "inven_slot";

			slot->s.renderfx = RF_DEPTHHACK;
			slot->predraw = Slot_Predraw;
			slot->movedir[0] = x;
			slot->movedir[1] = y;

			slot->s.modelindex = gi.modelindex("models/inven/square.md2");
			gi.linkentity(slot);
		}
	}

	for (int i = 0; i < INVEN_HOTBAR; i++)
	{
		edict_t *slot = G_Spawn();

		slot->classname = "hotbar_slot";

		slot->s.renderfx = RF_DEPTHHACK;
		slot->predraw = Slot_Predraw;
		slot->movedir[0] = INVEN_WIDTH + 1;
		slot->movedir[1] = i - 1;

		slot->s.modelindex = gi.modelindex("models/inven/square.md2");
		gi.linkentity(slot);
	}
}


