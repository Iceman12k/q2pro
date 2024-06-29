
#include "g_local.h"

//
#ifdef linux
#include <time.h>
#endif
//

detail_edict_t detail_ents[MAX_DETAILS];
edict_t *detail_reserved[RESERVED_DETAILEDICTS];
detail_edict_t *detail_map[MAX_CLIENTS][RESERVED_DETAILEDICTS];

int D_IsVisible(edict_t *v, edict_t *ref)
{
	int clientnum = v - g_edicts;
	detail_edict_t *entry = detail_map[clientnum][ref->health];//client->detail_current;
	if (!entry || clientnum < 0)
		return false;
	return true;
}

int D_Predraw(edict_t *v, edict_t *ref, entity_state_t *s, entity_state_extension_t *x)
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

	if (entry->dpredraw)
		successful = entry->dpredraw(v, entry, s, x);
	//

	if (entry->is_new[clientnum])
	{
		s->event = EV_OTHER_TELEPORT;
		entry->is_new[clientnum] = false;
	}

	// save our current origin out for next time
	//VectorCopy(s->origin, entry->old_origin[clientnum]);
	//VectorCopy(s->origin, s->old_origin);
	VectorCopy(s->origin, s->old_origin);
	//

	s->number = hold_num;
	
	return successful;
}

int null_model;
void D_Initialize(void)
{
	null_model = gi.modelindex("models/null.md2");

	for (int i = 0; i < RESERVED_DETAILEDICTS; i++)
	{
		edict_t *res = G_Spawn();
		detail_reserved[i] = res; // add to the reserved list
		
		res->health = i;
		res->svflags = SVF_NOCULL;
		res->s.modelindex = null_model;
		res->predraw = D_Predraw;
		res->isvisible = D_IsVisible;
		gi.linkentity(res);
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
	if (actor->aaddtoscene)
	{
		if (actor->aaddtoscene(actor, score))
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

	if (!gi.inPVS(viewer_origin, actor->origin))
		return ACTOR_DO_NOT_ADD;

	vec3_t diff;
	VectorSubtract(viewer_origin, actor->origin, diff);
	int score = (int)(VectorLength(diff)) + 1;

	if (actor->aevaluate)
		actor->aevaluate(actor, &score);

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

void Actor_Cleanup(actor_t *actor) // like free, but also removes the details
{
	for(int i = 0; i < ACTOR_MAX_DETAILS; i++)
	{
		if (!actor->details[i])
			continue;
		
		D_Free(actor->details[i]);
	}

	Actor_Free(actor);
}

bool Actor_RunThink(actor_t *actor)
{
    int     thinktime;

    thinktime = actor->anextthink;
    if (thinktime <= 0)
        return true;
    if (thinktime > level.framenum)
        return true;

    actor->anextthink = 0;
    if (!actor->athink)
        gi.error("NULL actor->athink");
    actor->athink(actor);

    return false;
}

void Scene_Generate(edict_t *viewer)
{
	if (viewer->client == NULL)
		return;

	#ifdef linux
	struct timespec tstart={0,0}, tend={0,0};
    clock_gettime(CLOCK_MONOTONIC, &tstart);
	#endif

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

	#ifdef linux
    clock_gettime(CLOCK_MONOTONIC, &tend);
    printf("frame took about %.5f ms\n",
           (((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - 
           ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec)) * 1000);
	#endif
}

