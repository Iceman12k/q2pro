
#include "g_local.h"

detail_edict_t detail_ents[MAX_DETAILS];
edict_t *detail_reserved[RESERVED_DETAILEDICTS];
detail_edict_t *detail_map[MAX_CLIENTS][RESERVED_DETAILEDICTS];

static float D_Predraw(edict_t *v, edict_t *ref, entity_state_t *s)
{
	int successful = true;
	int clientnum = v - g_edicts;
	//gclient_t *client = v->client;
	detail_edict_t *entry = detail_map[clientnum][ref->health];//client->detail_current;
	if (!entry || clientnum < 0)
		return false;

	// increment seek in the queue
	//client->detail_current = entry->next;
	//
	
	// find the proper origin
	int hold_num = s->number;
	*s = entry->s;

	//VectorCopy(entry->old_origin[clientnum], s->old_origin);

	if (entry->predraw)
		successful = entry->predraw(v, entry, s);
	//

	// save our current origin out for next time
	//VectorCopy(s->origin, entry->old_origin[clientnum]);
	VectorCopy(s->origin, s->old_origin);
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
		
		res->health = i;
		res->s.renderfx = RF_DEPTHHACK;
		res->s.modelindex = null_model;
		res->predraw = D_Predraw;
		gi.linkentity(res);
	}
}

static void D_InsertToQueue(gclient_t *client, detail_list_t *entry)
{
	int bucket = (int)(entry->score / (DETAIL_MAXRANGE / DETAIL_BUCKETS));
	//clamp(bucket, 0, (DETAIL_BUCKETS - 1));
	if (bucket < 0)
		bucket = 0;
	else if (bucket >= DETAIL_BUCKETS)
		bucket = DETAIL_BUCKETS - 1;

	detail_list_t *list = client->detail_bucket[bucket];

	if (!list)
		client->detail_bucket[bucket] = entry;
#if 1
	else if (bucket >= DETAIL_BUCKETS * 0.9) // we don't care much about sorting very far away ents
	{
		entry->next = list;
		client->detail_bucket[bucket] = entry;
	}
#endif
	else // yuck, we need to do some sorting!
	{
		detail_list_t *hold = NULL;
		while (list)
		{
			if (list->score < entry->score)
			{
				hold = list;
				list = list->next;
				continue;
			}

			if (hold)
				hold->next = entry;
			else
				client->detail_bucket[bucket] = entry;
			entry->next = list;
			return;
		}

		hold->next = entry; // we are the worstestest
	}

	//entry->next_list = client->detail_queue;

	/*
	detail_list_t *queue = client->detail_queue;
	if (queue) // if we have a queue started, add it in
	{
		detail_list_t *hold = NULL;
		while (queue)
		{
			if (queue->score < entry->score)
			{
				hold = queue;
				queue = queue->next;
				continue;
			}
			
			if (hold)
				hold->next = entry;
			else
				client->detail_queue = entry;
			entry->next = queue;
			return;
		}

		hold->next = entry; // we are the worstestest
	}
	else
	{
		client->detail_queue = entry;
	}
	*/
}

void D_GenerateQueue(edict_t *ent)
{
	gclient_t *client = ent->client;
	if (!client)
		return;
	
	int clientnum = ent - g_edicts;
	detail_edict_t *oldmap[RESERVED_DETAILEDICTS];
	memcpy(oldmap, detail_map[clientnum], sizeof(oldmap));
	memset(detail_map[clientnum], 0, sizeof(detail_map[clientnum]));


	detail_list_t *lst;
	// cleanup old buckets
	/*
	detail_list_t *lst = client->detail_queue;
	while (lst)
	{
		detail_list_t *cleanup = lst;
		lst = lst->next;
		gi.TagFree(cleanup);
	}
	*/

	for (int i = 0; i < DETAIL_BUCKETS; i++)
	{
		lst = client->detail_bucket[i];
		while (lst)
		{
			detail_list_t *cleanup = lst;
			lst = lst->next;
			gi.TagFree(cleanup);
		}
		client->detail_bucket[i] = NULL;
	}

	//client->detail_queue = NULL;
	//client->detail_current = NULL;
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
		detail->valid = false; // mark as invalid for now

		if (!detail->isused)
			continue;

		VectorSubtract(camera_pos, detail->s.origin, diff);
		float score = VectorLengthSquared(diff);
		float scorelimit = DETAIL_MAXRANGE;

		if (detail->detailflags & DETAIL_NEARLIMIT)
			scorelimit *= 0.25;

		if (score > DETAIL_MAXRANGE && !(detail->detailflags & DETAIL_NOLIMIT))
			continue;

		if ((detail->detailflags & DETAIL_PRIORITY_MAX) == DETAIL_PRIORITY_MAX)
			score = 0;
		else if (detail->detailflags & DETAIL_PRIORITY_HIGH)
			score *= 0.3;
		else if (detail->detailflags & DETAIL_PRIORITY_LOW)
			score *= 2;

		detail_list_t *entry = gi.TagMalloc(sizeof(detail_list_t), TAG_LEVEL);
		entry->score = score;
		entry->e = detail;
		entry->next = NULL;
		entry->next_list = NULL;

		//entry->next = client->detail_queue;
		//client->detail_queue = entry;
		D_InsertToQueue(client, entry);
	}

	// mark the first (RESERVED_DETAILEDICTS) as sendable, so we can find them a home
	
	/*
	lst = client->detail_queue;
	for (int i = 0; lst && i < RESERVED_DETAILEDICTS; i++, lst = lst->next_list)
	{
		lst->e->valid = true;
		lst->e->mapped = false;
	}
	*/

	for (int i = 0, cnt = 0; i < DETAIL_BUCKETS; i++)
	{
		for (lst = client->detail_bucket[i]; lst; lst = lst->next)
		{
			cnt++;

			if (cnt >= RESERVED_DETAILEDICTS)
				break;

			lst->e->valid = true;
			lst->e->mapped = false;
		}

		if (cnt >= RESERVED_DETAILEDICTS)
			break;
	}

#if 1
	for (int i = 0; i < RESERVED_DETAILEDICTS; i++)
	{
		detail_edict_t *old_d = oldmap[i];
		if (!old_d)
			continue;

		if (!old_d->valid)
			continue;

		old_d->mapped = true;
		detail_map[clientnum][i] = old_d;
	}
#endif

	for (int i = 0; i < RESERVED_DETAILEDICTS; i++)
	{
		if (detail_map[clientnum][i] != NULL)
			continue;

		if (oldmap[i] != NULL)
			continue;
		
		for (int j = 0; j < DETAIL_BUCKETS; j++)
		{
			if (detail_map[clientnum][i] != NULL)
				break;

			for (lst = client->detail_bucket[j]; lst; lst = lst->next)
			{
				if (lst->e->valid && !lst->e->mapped)
				{
					lst->e->mapped = true;
					detail_map[clientnum][i] = lst->e;
					break;
				}
			}
		}
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

	memset(&detail_ents[0], 0, sizeof(detail_edict_t));
	detail_ents[0].isused = true;
	return &detail_ents[0]; // uh oh... we have a LOT of details
}

void D_Free(detail_edict_t *detail)
{
	detail->isused = false;
}

