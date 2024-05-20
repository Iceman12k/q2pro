
#include "g_local.h"

detail_edict_t *inven_slot[INVEN_TOTALSLOTS];
detail_edict_t *inven_hotbar[INVEN_HOTBAR];

void pickaxe_frame(edict_t *ent, item_t *item, int *do_think);
void pickaxe_endframe(edict_t *ent, item_t *item);
item_t pickaxe = {
	"Pickaxe", // display name
	"models/inven/i_pickaxe.md2", // icon model
	"i_pickaxe", // icon image
	0, // icon skin
	0, // icon frame
	"models/weapons/v_pickaxe.md2", // viewmodel
	0, // passive item flag
	pickaxe_frame, // frame func
	pickaxe_endframe, // end server frame func
};

item_t lantern = {
	"Lantern", // display name
	"models/inven/i_pickaxe.md2", // icon model
	"i_pickaxe", // icon image
	0, // icon skin
	0, // icon frame
	"models/weapons/v_lantern.md2", // viewmodel
	PASSIVE_LIGHT, // passive item flag
	NULL, // frame func
	NULL, // end server frame func
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



// pickaxe
enum pickaxe_mode_e {
	PICKAXE_MODE_STATIC,
	PICKAXE_MODE_SWING,
	PICKAXE_MODE_MINE,
	PICKAXE_MODE_DEFLECT,
};

enum pickaxe_frame_e {
	pickaxe_frame_idle,
	pickaxe_frame_idle_end = 50,
	pickaxe_frame_mine = pickaxe_frame_idle_end,
	pickaxe_frame_mine_hit = pickaxe_frame_mine + 1,
	pickaxe_frame_mine_end = pickaxe_frame_mine + 5,
	pickaxe_frame_swing = pickaxe_frame_mine_end,
	pickaxe_frame_swing_end = pickaxe_frame_swing + 8,
	pickaxe_frame_deflect = pickaxe_frame_swing_end + 1,
	pickaxe_frame_deflect_end = pickaxe_frame_deflect + 7,
};

typedef struct {
	uint8_t frame : 8;
	uint8_t mode : 3;
	short	swingpos[3];
	short 	swinghits[6];
} pickaxe_t;

#define PICKAXE_MINE_RANGE 64
#define PICKAXE_SWING_BBOX 10
#define PICKAXE_DAMAGE 10

void pickaxe_mine(edict_t *ent)
{
	pickaxe_t *data = (pickaxe_t*)&ent->weapon_data;
	gclient_t *client = ent->client;

	trace_t trace;
	vec3_t camera_org, targ_org, forward;
	VectorAdd(ent->s.origin, client->ps.viewoffset, camera_org);
	AngleVectors(client->ps.viewangles, forward, NULL, NULL);
	VectorMA(camera_org, PICKAXE_MINE_RANGE, forward, targ_org);

	trace = gi.trace(camera_org, vec3_origin, vec3_origin, targ_org, ent, CONTENTS_SOLID);
	if (trace.fraction < 1)
	{
		gi.sound(ent, CHAN_AUTO, gi.soundindex("western/pick_hit.wav"), 1, ATTN_IDLE, 0);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_GUNSHOT);
		gi.WritePosition(trace.endpos);
		gi.WriteDir(trace.plane.normal);
		gi.multicast(trace.endpos, MULTICAST_PVS);
	}
}

//#define PICKAXE_DEBUG
#ifdef PICKAXE_DEBUG
actor_t *pickaxe_debug;
#endif

void pickaxe_deflect(edict_t *ent, trace_t *trace)
{
	pickaxe_t *data = (pickaxe_t*)&ent->weapon_data;
	gclient_t *client = ent->client;

	ent->attack_finished = client->levelframenum + (8 * FRAMEDIV);
	data->mode = PICKAXE_MODE_DEFLECT;
	data->frame = pickaxe_frame_deflect;

	client->kick_angles[YAW] += -3;
	client->kick_angles[PITCH] -= 1;
	client->kick_angles[ROLL] += 2;
	gi.sound(ent, CHAN_AUTO, gi.soundindex("western/deflect.wav"), 1, ATTN_STATIC, trace->fraction * BASE_FRAMETIME_1000);
}

void pickaxe_swing(edict_t *ent)
{
	pickaxe_t *data = (pickaxe_t*)&ent->weapon_data;
	gclient_t *client = ent->client;

	int index = data->frame - pickaxe_frame_swing;

	#ifdef PICKAXE_DEBUG
	if (!pickaxe_debug)
	{
		pickaxe_debug = Actor_Spawn();
		pickaxe_debug->details[0] = D_Spawn();
	}
	pickaxe_debug->details[0]->s.modelindex = 0;
	#endif

	if (index >= 4)
	{
		return;
	}

	#ifdef PICKAXE_DEBUG
	pickaxe_debug->details[0]->s.modelindex = gi.modelindex("models/objects/gibs/head2/tris.md2");
	#endif

	vec3_t swing_pos[] = {
		{27, 4, 24},
		{15, -2, 29},
		{-4, -8, 35},
		{-27, -16, 28},
	};

	vec3_t swing_last;
	vec3_t camera_org, swing_org;
	vec3_t forward, right, up;
	AngleVectors(client->ps.viewangles, forward, right, up);
	VectorAdd(ent->s.origin, client->ps.viewoffset, camera_org);

	VectorCopy(camera_org, swing_org);
	VectorMA(swing_org, swing_pos[index][0], right, swing_org);
	VectorMA(swing_org, swing_pos[index][1], up, swing_org);
	VectorMA(swing_org, swing_pos[index][2], forward, swing_org);

	// add some delta view angle
	//client->ps.pmove.delta_angles[YAW] += ANGLE2SHORT(2);
	client->kick_angles[YAW] += (3 - index);
	client->kick_angles[PITCH] += 0.5 - (index * 0.25);
	client->kick_angles[ROLL] -= (1 - (index / 2));

	// unpack old swingpos
	swing_last[0] = SHORT2COORD(data->swingpos[0]);
	swing_last[1] = SHORT2COORD(data->swingpos[1]);
	swing_last[2] = SHORT2COORD(data->swingpos[2]);

	// trace
	if ((swing_last[0] + swing_last[1] + swing_last[2]))
	{
		vec3_t smins = {-PICKAXE_SWING_BBOX, -PICKAXE_SWING_BBOX, -PICKAXE_SWING_BBOX};
		vec3_t smaxs = {PICKAXE_SWING_BBOX, PICKAXE_SWING_BBOX, PICKAXE_SWING_BBOX};
		trace_t trace;
		trace = gi.trace(swing_last, smins, smaxs, swing_org, ent, MASK_SOLID);

		if (trace.fraction < 1 || trace.startsolid)
		{
			pickaxe_deflect(ent, &trace);
		}
		else // no geometry in the way, time to hit!
		{
			VectorScale(smins, 1.4, smins);
			VectorScale(smaxs, 1.4, smaxs);
			trace = gi.trace(swing_last, smins, smaxs, swing_org, ent, CONTENTS_MONSTER);
			if (!(trace.ent->damage_flags & trace.ent->damage_flags & DAMAGEFLAG_YES))
				trace = gi.trace(camera_org, smins, smaxs, swing_org, ent, CONTENTS_MONSTER);

			if ((trace.fraction < 1 || trace.allsolid || trace.startsolid) && (trace.ent->damage_flags & DAMAGEFLAG_YES))
			{
				int ent_num = NUM_FOR_EDICT(trace.ent);
				int ignore_hit = false;
				for(int i = 0; i < 6; i++) // check in our ignore list for this ent
				{
					if (ent_num != data->swinghits[i])
						continue;
					ignore_hit = true;
					break;
				}

				if (!ignore_hit)
				{
					for(int i = 0; i < 6; i++) // add to our ignore list
					{
						if (data->swinghits[i])
							continue;
						data->swinghits[i] = ent_num;
					}

					if (trace.ent->damage_flags & DAMAGEFLAG_BLOCKING)
					{
						pickaxe_deflect(ent, &trace);
					}
					else
					{
						if (trace.ent->takedamage)
						{
							trace.ent->takedamage(trace.ent, ent, PICKAXE_DAMAGE, 120, vec3_origin);
						}
					}
				}
			}
		}

		VectorCopy(trace.endpos, swing_org);
	}

	// pack swingpos into shorts
	data->swingpos[0] = COORD2SHORT(swing_org[0]);
	data->swingpos[1] = COORD2SHORT(swing_org[1]);
	data->swingpos[2] = COORD2SHORT(swing_org[2]);

	#ifdef PICKAXE_DEBUG
	// debug
	VectorCopy(swing_org, pickaxe_debug->details[0]->s.origin);
	VectorCopy(swing_org, pickaxe_debug->origin);
	Actor_Link(pickaxe_debug, 0);
	#endif
}

void pickaxe_attack(edict_t *ent, item_t *item, int *do_think)
{
	pickaxe_t *data = (pickaxe_t*)&ent->weapon_data;
	gclient_t *client = ent->client;
	int is_server_frame = (*do_think == true);

	if (is_server_frame) // animate!
	{
		if (data->mode == PICKAXE_MODE_MINE)
		{
			if (data->frame == pickaxe_frame_mine_hit)
			{
				pickaxe_mine(ent);
			}

			data->frame = min(data->frame + 1, pickaxe_frame_mine_end);
		}
		else if (data->mode == PICKAXE_MODE_SWING)
		{
			pickaxe_swing(ent);
			data->frame = min(data->frame + 1, pickaxe_frame_swing_end);
		}
		else if (data->mode == PICKAXE_MODE_DEFLECT)
		{
			data->frame = min(data->frame + 1, pickaxe_frame_deflect_end);
		}
		else
		{
			data->frame = pickaxe_frame_idle + ((data->frame + 1) % ((pickaxe_frame_idle_end - pickaxe_frame_idle) + 1));
		}
	}

	if (client->levelframenum < ent->attack_finished)
		return;

	if (client->cmd_buttons & BUTTON_ATTACK)
	{
		*do_think = true; // we did the think, so mark it so we don't do it twice this frame!
		int should_mine = false;
		
		trace_t trace;
		vec3_t camera_org, targ_org, forward;
		VectorAdd(ent->s.origin, client->ps.viewoffset, camera_org);
		AngleVectors(client->ps.viewangles, forward, NULL, NULL);
		VectorMA(camera_org, PICKAXE_MINE_RANGE * 0.9, forward, targ_org);

		trace = gi.trace(camera_org, vec3_origin, vec3_origin, targ_org, ent, CONTENTS_SOLID);

		if (trace.fraction < 1)
		{
			ent->attack_finished = client->levelframenum + (5 * FRAMEDIV);// + (is_server_frame ? 0 : 1);
			data->mode = PICKAXE_MODE_MINE;
			data->frame = pickaxe_frame_mine;
			
			gi.sound(ent, CHAN_AUTO, gi.soundindex("western/swing_small.wav"), 1, ATTN_STATIC, 0.05);
		}
		else
		{
			ent->attack_finished = client->levelframenum + (7 * FRAMEDIV);// + (is_server_frame ? 0 : 1);
			data->mode = PICKAXE_MODE_SWING;
			data->frame = pickaxe_frame_swing;

			// zero out swing
			memset(data->swingpos, 0, sizeof(data->swingpos));
			memset(data->swinghits, 0, sizeof(data->swinghits));

			gi.sound(ent, CHAN_AUTO, gi.soundindex("western/swing_big.wav"), 1, ATTN_STATIC, 0.1);
		}
	}	
	else if (is_server_frame)
	{
		//if (data->mode != PICKAXE_MODE_STATIC)
		//	data->frame = pickaxe_frame_idle;
		data->mode = PICKAXE_MODE_STATIC;
	}
}

void pickaxe_frame(edict_t *ent, item_t *item, int *do_think)
{
	pickaxe_t *data = (pickaxe_t*)&ent->weapon_data;
	gclient_t *client = ent->client;
	int is_server_frame = (*do_think == true);

	pickaxe_attack(ent, item, do_think);
}

void pickaxe_endframe(edict_t *ent, item_t *item)
{
	pickaxe_t *data = (pickaxe_t*)&ent->weapon_data;
	gclient_t *client = ent->client;

	client->ps.gunframe = data->frame;
}
//

