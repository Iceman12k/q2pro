
#include "g_local.h"

detail_edict_t *inven_slot[INVEN_TOTALSLOTS];
detail_edict_t *inven_hotbar[INVEN_HOTBAR];

item_t pickaxe = {
	"Pickaxe", // display name
	"models/inven/i_pickaxe.md2", // icon model
	"i_pickaxe", // icon image
	0, // icon skin
	0, // icon frame
	"models/weapons/v_pickaxe.md2", // viewmodel
	0, // passive item flag
};

item_t lantern = {
	"Lantern", // display name
	"models/inven/i_pickaxe.md2", // icon model
	"i_pickaxe", // icon image
	0, // icon skin
	0, // icon frame
	"models/weapons/v_lantern.md2", // viewmodel
	PASSIVE_LIGHT, // passive item flag
};

static float Slot_Predraw(edict_t *v, detail_edict_t *e, entity_state_t *s)
{
	gclient_t *client = v->client;
	if (!client->inv_open)
		return false;

	vec3_t forward, right, up;
	vec3_t inv_dir;
	VectorSet(inv_dir, 0, client->inv_angle, 0);
	AngleVectors(inv_dir, forward, right, up);

	for (int i = 0; i < 3; i++) {
		s->origin[i] = SHORT2COORD(client->ps.pmove.origin[i]);
	}
	VectorAdd(s->origin, client->ps.viewoffset, s->origin);

	//int x = e->movedir[0] - ((INVEN_WIDTH / 2) + 1);
	//int y = ((INVEN_HEIGHT / 2)) - e->movedir[1]; // top down, please

	VectorMA(s->origin, INVEN_DISTANCE, forward, s->origin);
	VectorMA(s->origin, e->movedir[0], right, s->origin);
	VectorMA(s->origin, e->movedir[1], up, s->origin);
	//VectorCopy(s->origin, s->old_origin);

	VectorSet(inv_dir, 0, client->inv_angle + 180, 0);
	VectorCopy(inv_dir, s->angles);

	s->renderfx = RF_FULLBRIGHT | RF_TRANSLUCENT | RF_NOSHADOW | RF_DEPTHHACK;

	if (e->type == 0) // slot itself
	{
		if (client->inv_selected == e->movedir[2])
			s->skinnum = 2;
		else if (client->inv_highlighted == e->movedir[2])
			s->skinnum = 1;
	}
	else if (e->type == 1)
	{
		item_t *item;
		if (e->movedir[2] < 0)
			return false;

		item = client->inv_content[(int)e->movedir[2]];
		if (!item) // null item means we can ignore it
			return false;

		s->renderfx &= ~RF_TRANSLUCENT;
		s->modelindex = gi.modelindex(item->icon_model);
		s->frame = item->icon_frame;
		s->skinnum = item->icon_skin;
	}

	return true;
}

static detail_edict_t *InvenSlot_SpawnItem(detail_edict_t *slot)
{
	detail_edict_t *item = D_Spawn();

	item->classname = "inven_item";
	item->detailflags = slot->detailflags;
	item->s.renderfx = slot->s.renderfx;

	item->predraw = slot->predraw;
	item->type = 1;

	VectorCopy(slot->movedir, item->movedir);
	item->s.modelindex = gi.modelindex("models/null.md2");

	return item;
}

void I_Evaluate(actor_t *actor, int *score)
{
	if (!viewer_edict->client->inv_open)
	{
		*score = ACTOR_DO_NOT_ADD;
		return;
	}

	*score = 1;
}

void I_Initialize(void)
{
	actor_t *actor = Actor_Spawn();
	actor->chunks_visible = ~0ULL;
	actor->evaluate = I_Evaluate;

	int actor_detail_seek = 0;
	vec3_t offs;
	VectorClear(offs);
	offs[0] = -((INVEN_WIDTH - 2) * INVEN_SIZE);
	offs[1] = (1 * INVEN_SIZE);

#if 1
#if 1
	for (int x = 0; x < (INVEN_HOTBAR_START); x++)
	{
		detail_edict_t *slot = D_Spawn();
		inven_slot[x] = slot;
		actor->details[actor_detail_seek] = slot; actor_detail_seek++;
			
		slot->classname = "inven_slot";

		slot->detailflags = DETAIL_NOLIMIT | DETAIL_PRIORITY_MAX;
		slot->s.renderfx = RF_DEPTHHACK;
		slot->predraw = Slot_Predraw;

		slot->movedir[0] = ((x % INVEN_WIDTH) - 1) * INVEN_SIZE;
		slot->movedir[1] = (floor(x / INVEN_WIDTH)) * -INVEN_SIZE;
		VectorAdd(slot->movedir, offs, slot->movedir);
		slot->movedir[2] = x;

		slot->s.modelindex = gi.modelindex("models/inven/square.md2");

		slot = InvenSlot_SpawnItem(slot);
		actor->details[actor_detail_seek] = slot; actor_detail_seek++;
		//gi.linkentity(slot);
	}
#endif

	for (int x = INVEN_HOTBAR_START; x < INVEN_TOTALSLOTS; x++)
	{
		detail_edict_t *slot = D_Spawn();
		inven_slot[x] = slot;
		actor->details[actor_detail_seek] = slot; actor_detail_seek++;

		slot->classname = "inven_slot";

		slot->detailflags = DETAIL_NOLIMIT | DETAIL_PRIORITY_MAX;
		slot->s.renderfx = RF_DEPTHHACK;
		slot->predraw = Slot_Predraw;

		slot->movedir[0] = 3 * INVEN_SIZE;
		slot->movedir[1] = (x - INVEN_HOTBAR_START) * -INVEN_SIZE;
		VectorAdd(slot->movedir, offs, slot->movedir);
		slot->movedir[2] = x;

		slot->s.modelindex = gi.modelindex("models/inven/circle.md2");

		slot = InvenSlot_SpawnItem(slot);
		actor->details[actor_detail_seek] = slot; actor_detail_seek++;
	}

	/*
	for (int i = 0; i < INVEN_HOTBAR; i++)
	{
		detail_edict_t *slot = D_Spawn();

		slot->classname = "hotbar_slot";

		slot->detailflags = DETAIL_NOLIMIT | DETAIL_PRIORITY_MAX;
		slot->s.renderfx = RF_DEPTHHACK;
		slot->predraw = Slot_Predraw;
		slot->movedir[0] = INVEN_WIDTH + 1;
		slot->movedir[1] = i;

		slot->s.modelindex = gi.modelindex("models/inven/square.md2");
		//gi.linkentity(slot);
	}
	*/
#endif

	#if 0
	for(int x = 0; x < 32; x++)
	{
		for(int y = 0; y < 32; y++)
		{
			for(int z = 0; z < 4; z++)
			{
				actor_t *actor = Actor_Spawn();
				actor->details[0] = D_Spawn();
				actor->details[0]->s.modelindex = gi.modelindex("models/objects/gibs/head2/tris.md2");
				VectorSet(actor->details[0]->s.origin, (x) * 16, (y) * 16, (z) * 16);
				VectorCopy(actor->details[0]->s.origin, actor->origin);
				Actor_Link(actor, 16);
			}
		}
	}
	#endif
}

void I_MouseInput(edict_t *ent, usercmd_t *ucmd)
{
	gclient_t   *client;
	client = ent->client;

	if (ent->client->inv_open)
	{
		vec3_t forward, angles, plate_norm, plate_coord, contact, rayorg;
		VectorCopy(client->ps.viewangles, angles);
		angles[1] -= ent->client->inv_angle; // rotate to be centered around the inv_angles

		VectorClear(rayorg);
		VectorSet(plate_coord, INVEN_DISTANCE, 0, 0);
		VectorSet(plate_norm, 1, 0, 0);
		AngleVectors(angles, forward, NULL, NULL);

		client->inv_highlighted = -1;

		// get d value
		float d = DotProduct(plate_norm, plate_coord);

		if (DotProduct(plate_norm, forward) == 0) {
			return; // No intersection, the line is parallel to the plane
		}

		// Compute the X value for the directed line ray intersecting the plane
		float x = (d - DotProduct(plate_norm, rayorg)) / DotProduct(plate_norm, forward);

		// output contact point
		VectorMA(rayorg, x, forward, contact);

		//int xp = ceil(((contact[1]) / INVEN_SIZE) - 0.5);
		//int yp = ceil(((contact[2]) / INVEN_SIZE) - 0.5);
		//xp -= ((INVEN_WIDTH / 2) + 1);
		//yp -= ((INVEN_HEIGHT / 2));
		//xp *= -1;
		//yp *= -1;

		for (int i = 0; i < INVEN_TOTALSLOTS; i++) // loop through slots and check all for mouse hover
		{
			vec2_t x, y;
			detail_edict_t *slot = inven_slot[i];
			if (slot == NULL) //uh... uh-oh, this shouldn't happen
				continue;

			x[0] = slot->movedir[0] - (INVEN_SIZE / 2);
			x[1] = slot->movedir[0] + (INVEN_SIZE / 2);
			y[0] = slot->movedir[1] - (INVEN_SIZE / 2);
			y[1] = slot->movedir[1] + (INVEN_SIZE / 2);

			if (-contact[1] >= x[0] && -contact[1] < x[1])
			{
				if (contact[2] >= y[0] && contact[2] < y[1])
				{
					client->inv_highlighted = slot->movedir[2];
					break;
				}
			}
		}
	}
}

void I_Input(edict_t *ent, usercmd_t *ucmd)
{
	gclient_t   *client;
	client = ent->client;

	I_MouseInput(ent, ucmd);

	if (ucmd->buttons & BUTTON_ATTACK)
	{
		#if 0
		if (client->inv_selected[0] == -1 && client->inv_selected[1] == -1 && !(client->cmd_buttons & BUTTON_ATTACK))
		{
			if (!(client->inv_highlighted[0] < 0 || client->inv_highlighted[1] < 0) && !(client->inv_highlighted[0] >= INVEN_WIDTH || client->inv_highlighted[1] >= INVEN_HEIGHT))
			{
				client->inv_selected[0] = client->inv_highlighted[0];
				client->inv_selected[1] = client->inv_highlighted[1];
			}
		}
		#else
		if (!(client->cmd_buttons & BUTTON_ATTACK))
		{
			client->inv_selected = client->inv_highlighted;
		}
		#endif
	}
	else
	{
		if (client->inv_selected >= 0 && client->inv_highlighted >= 0)
		{
			item_t *hold = client->inv_content[client->inv_highlighted];
			client->inv_content[client->inv_highlighted] = client->inv_content[client->inv_selected];
			client->inv_content[client->inv_selected] = hold;
		}

		#if 0
		client->inv_selected[0] = -1;
		client->inv_selected[1] = -1;
		#else
		client->inv_selected = -1;
		#endif
	}

	ucmd->buttons &= BUTTON_ATTACK;
}
