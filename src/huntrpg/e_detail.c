
#include "g_local.h"

#define MAX_DETAILS				8192
#define RESERVED_DETAILEDICTS	256

detail_edict_t detail_ents[MAX_DETAILS];
edict_t *detail_reserved[RESERVED_DETAILEDICTS];

static float D_Predraw(edict_t *v, edict_t *e, entity_state_t *s)
{
	int successful = true;
	int clientnum = g_edicts - v;
	gclient_t *client = v->client;
	detail_list_t *entry = client->detail_current;
	if (!entry)
		return false;


	// increment seek in the queue
	client->detail_current = entry->next;
	//
	
	// find the proper origin
	int hold_num = s->number;
	*s = entry->e->s;
	VectorCopy(entry->e->old_origin[clientnum], s->old_origin);

	if (entry->e->predraw)
		successful = entry->e->predraw(v, entry->e, s);
	//

	// save our current origin out for next time
	//VectorCopy(s->origin, entry->e->old_origin[clientnum]);
	//

	s->number = hold_num;
	return successful;
}

void D_Initialize(void)
{
	int null_model = gi.modelindex("models/null.md2");

	for (int i = 0; i < RESERVED_DETAILEDICTS; i++)
	{
		edict_t *res = G_Spawn();
		detail_reserved[i] = res; // add to the reserved list
		
		res->s.renderfx = RF_DEPTHHACK;
		res->s.modelindex = null_model;
		res->predraw = D_Predraw;
		gi.linkentity(res);
	}
}

static void D_InsertToQueue(detail_list_t **list, detail_list_t *entry)
{
	detail_list_t *queue = *list;

	if (queue) // if we have a queue started, add it in
	{
		if (queue->score > entry->score)
		{
			entry->next = queue;
			*list = entry;
		}
		else
		{
			detail_list_t *hold = queue;
			while (queue)
			{
				if (queue->score < entry->score)
				{
					hold = queue;
					queue = queue->next;
					continue;
				}
				
				hold->next = entry;
				entry->next = queue;
				return;
			}

			hold->next = entry; // we are the worstestest
		}
	}
	else
	{
		*list = entry;
	}
}

void D_GenerateQueue(edict_t *ent)
{
	gclient_t *client = ent->client;
	if (!client)
		return;
	
	// cleanup old queue
	detail_list_t *lst = client->detail_queue;
	while (lst)
	{
		detail_list_t *cleanup = lst;
		lst = lst->next;
		gi.TagFree(cleanup);
	}

	client->detail_queue = NULL;
	//

	// set camera pos
	vec3_t camera_pos;
	for (int i = 0; i < 3; i++)
		camera_pos[i] = SHORT2COORD(client->ps.pmove.origin[i]);
	VectorAdd(camera_pos, client->ps.viewoffset, camera_pos);
	//

	for (int i = 0; i < MAX_DETAILS; i++)
	{
		vec3_t diff;
		detail_edict_t *detail = &detail_ents[i];
		if (!detail->isused)
			continue;

		VectorSubtract(camera_pos, detail->s.origin, diff);
		float score = VectorLengthSquared(diff);

		if (score > 65536)
			continue;

		detail_list_t *entry = gi.TagMalloc(sizeof(detail_list_t), TAG_LEVEL);
		entry->score = score;
		entry->e = detail;
		entry->next = NULL;
		D_InsertToQueue(&client->detail_queue, entry);
	}
}



detail_edict_t* D_Spawn(void)
{
	for (int i = 0; i < MAX_DETAILS; i++)
	{
		detail_edict_t *detail = &detail_ents[i];
		if (detail->isused)
			continue;

		memset(detail, 0, sizeof(detail_edict_t));
		detail->isused = true;
		return detail;
	}

	return &detail_ents[0]; // uh oh... we have a LOT of details
}

void D_Free(detail_edict_t *detail)
{
	detail->isused = false;
}

