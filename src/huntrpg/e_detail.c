
#include "g_local.h"

//
#include <time.h>
//

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

	if (entry->is_new[clientnum])
	{
		s->event = EV_OTHER_TELEPORT;
		entry->is_new[clientnum] = false;
	}

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

		if (score > scorelimit && !(detail->detailflags & DETAIL_NOLIMIT))
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

// Reki (February 8 2024): Start of actor stuff...
// basic idea is to use 'chunks' to group up actors, as kind of a shitty PVS
actor_t actor_list[MAX_ACTORS];
edict_t *viewer_edict;
vec3_t viewer_origin;
int viewer_clientnum;

typedef struct actortree_s {
	int score;
	actor_t *actor;
	struct actortree_s *above;
	struct actortree_s *higher;
	struct actortree_s *lower;
} actortree_t;

actortree_t tree_data[MAX_ACTORS];
actortree_t *tree_head;
int tree_seek;

detailusagefield_t scene_detailused;
detailedictfield_t scene_redictused;
int scene_detailcount;

void Actor_Link(actor_t *actor, int size)
{
	actor->chunks_visible = 1ULL << ORG_TO_CHUNK(actor->origin);

	if (size <= 1)
	{
		for(int ang = 0; ang < 360; ang += 22.5)
		{
			vec3_t vorg;
			vec3_t uvector;

			uvector[0] = cos(DEG2RAD(ang));
			uvector[1] = sin(DEG2RAD(ang));
			uvector[2] = 0;

			VectorMA(actor->origin, CHUNK_SIZE * 0.6, uvector, vorg);
			actor->chunks_visible |= 1ULL << ORG_TO_CHUNK(vorg);
			VectorMA(actor->origin, CHUNK_SIZE * 1, uvector, vorg);
			actor->chunks_visible |= 1ULL << ORG_TO_CHUNK(vorg);
			VectorMA(actor->origin, CHUNK_SIZE * 1.4, uvector, vorg);
			actor->chunks_visible |= 1ULL << ORG_TO_CHUNK(vorg);
		}
	}
}

int Actor_AddDetail(actor_t *actor, detail_edict_t *detail)
{
	int index = detail - detail_ents;
	int index_board = (int)(index / BITS_PER_NUM);
	int index_bit = (index % BITS_PER_NUM);
	scene_detailused[index_board] |= (size_t)1 << index_bit;
	scene_detailcount++;
	if (scene_detailcount >= RESERVED_DETAILEDICTS)	
		return true; // abort! we've capped out
	return false;
}

int Actor_AddToScene(actor_t *actor, int score)
{
	if (actor->addtoscene)
	{
		if (actor->addtoscene(actor, score))
			return false;
	}

	for(int i = 0; i < ACTOR_MAX_DETAILS; i++)
	{
		if (actor->details[i] == NULL)
			break;

		// really hope this detail ent is in the master list...
		if (Actor_AddDetail(actor, actor->details[i]))
			return true;
	}

	return false;
}

// returns a score, used to assemble the binary tree, <0 for do not add
int Actor_Evaluate(actor_t *actor)
{
	if ((viewer_edict->client->chunks_visible & actor->chunks_visible) == 0) // no overlap in our chunks...
		return ACTOR_DO_NOT_ADD;

	vec3_t diff;
	VectorSubtract(viewer_origin, actor->origin, diff);
	int score = (int)(VectorLength(diff)) + 1;

	if (actor->evaluate)
		actor->evaluate(actor, &score);

	return score;
}

// inserts into the binary tree
void Actor_AddToTree(actor_t *actor, int score)
{
	actortree_t *tree = &tree_data[tree_seek];
	tree_seek++;

	tree->score = score;
	tree->actor = actor;
	tree->higher = NULL;
	tree->lower = NULL;

	if (!tree_head)
	{
		tree_head = tree;
	}
	else
	{
		actortree_t *lst = tree_head;
		while(1)
		{
			if (score > lst->score)
			{
				if (lst->higher)
					goto tree_higher;
				tree->above = lst;
				lst->higher = tree;
				break;
			}
			else
			{
				if (lst->lower)
					goto tree_lower;
				tree->above = lst;
				lst->lower = tree;
				break;
			}

			tree_higher:
			lst = lst->higher;
			continue;
			tree_lower:
			lst = lst->lower;
			continue;
		}
	}
}

void Actor_GenerateTree(void)
{
	tree_head = NULL;
	tree_seek = 0;

	for(int i = 0; i < MAX_ACTORS; i++)
	{
		actor_t *actor = &actor_list[i];
		if (!actor->inuse)
			continue;

		int score = Actor_Evaluate(actor);
		if (score == ACTOR_DO_NOT_ADD)
			continue;

		Actor_AddToTree(actor, score);
	}
}

// returns true if we should abort the search
int Actor_TraverseTree(actortree_t *node)
{
	if (node == NULL)
		return false;
	
	if (Actor_TraverseTree(node->lower))
		return true;

	if (Actor_AddToScene(node->actor, node->score))
		return true;

	if (Actor_TraverseTree(node->higher))
		return true;
	
	return false;
}

actor_t *Actor_Spawn(void)
{
	actor_t *actor = &actor_list[0];
	for(int i = 0; i < MAX_ACTORS; i++)
	{
		if (actor_list[i].inuse)
			continue;
		actor = &actor_list[i];
		break;
	}

	memset(actor, 0, sizeof(actor_t));
	actor->inuse = true;
	return actor;
}

void Actor_Free(actor_t *actor)
{
	actor->inuse = false;
}

void Scene_Generate(edict_t *viewer)
{
	if (viewer->client == NULL)
		return;

	struct timespec tstart={0,0}, tend={0,0};
    clock_gettime(CLOCK_MONOTONIC, &tstart);

	viewer_clientnum = viewer - g_edicts;
	viewer_edict = viewer; // set the global for other funcs to use
	VectorAdd(viewer->s.origin, viewer->client->ps.viewoffset, viewer_origin);
	memset(scene_detailused, 0, sizeof(scene_detailused)); // clear out our scratch detail bitboard
	memcpy(scene_redictused, viewer_edict->client->detail_edict_in_use, sizeof(scene_redictused));
	scene_detailcount = 0;

	// use binary tree to get our data
	Actor_GenerateTree();
	Actor_TraverseTree(tree_head);

	// figure out which details are going to remain the same, and which will be changing
	for(int i = 0; i < DETAIL_USAGE_BOARDS; i++) // loop to remove old ents, and free their redicts
	{
		bitfield_t newf = scene_detailused[i];
		bitfield_t oldf = viewer_edict->client->detail_sent_to_client[i];

		size_t delta = newf ^ oldf;
		if (delta)
		{
			size_t delta_remove = delta & ~newf;
			if (delta_remove)
			{
				do {
					int index = __builtin_ctzl(delta_remove);
					detail_edict_t *detail = &detail_ents[index + (BITS_PER_NUM * i)];
					int redict_to_free = detail->mapped_to[viewer_clientnum];
					int free_board = (int)(redict_to_free / BITS_PER_NUM);
					int free_bit = (redict_to_free % BITS_PER_NUM);
					scene_redictused[free_board] &= ~((size_t)1 << free_bit);

					detail_map[viewer_clientnum][redict_to_free] = NULL;
					detail->mapped_to[viewer_clientnum] = 0;
					delta_remove &= ~((size_t)1 << index);
				} while(delta_remove);
			}
		}
	}
	for(int i = 0; i < DETAIL_USAGE_BOARDS; i++) // loop to add new ents, using the freed redicts
	{
		bitfield_t newf = scene_detailused[i];
		bitfield_t oldf = viewer_edict->client->detail_sent_to_client[i];

		size_t delta = newf ^ oldf;
		if (delta)
		{
			size_t delta_add = delta & newf;
			if (delta_add)
			{
				do {
					int index = __builtin_ctzl(delta_add);
					detail_edict_t *detail = &detail_ents[index + (BITS_PER_NUM * i)];
					int redict_index = 0;
					for(int j = 0; j < DETAIL_EDICT_BOARDS; j++)
					{
						if (~scene_redictused[j] == 0) // all are used here...
							continue;
						
						size_t inverted = ~scene_redictused[j];
						int free_bit = __builtin_ctzl(inverted);
						redict_index = free_bit + (BITS_PER_NUM * j);
						scene_redictused[j] |= ((size_t)1 << free_bit);
						break;
					}

					VectorCopy(detail->s.origin, detail->s.old_origin);
					detail_map[viewer_clientnum][redict_index] = detail;
					detail->mapped_to[viewer_clientnum] = redict_index;
					detail->is_new[viewer_clientnum] = true;

					//Com_Printf("Adding detail %i\n", index + (BITS_PER_NUM * i));

					//Com_Printf("%.64llb\n", delta_add);
					//Com_Printf("%.64llb\n\n", ~(1ULL << (size_t)index));
					delta_add = (delta_add & ~((size_t)1 << (size_t)index));
				} while (delta_add);
			}
		}
	}

	memcpy(viewer_edict->client->detail_edict_in_use, scene_redictused, sizeof(scene_redictused));
	memcpy(viewer_edict->client->detail_sent_to_client, scene_detailused, sizeof(scene_detailused));


    clock_gettime(CLOCK_MONOTONIC, &tend);
    printf("frame took about %.5f ms\n",
           (((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - 
           ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec)) * 1000);
}

