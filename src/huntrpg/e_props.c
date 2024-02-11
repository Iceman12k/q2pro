
#include "g_local.h"

typedef struct detail_waiting_s {
	char *target;
	detail_edict_t *detail;
	struct detail_waiting_s *next;
} detail_waiting_t;

typedef struct actor_waiting_s {
	char *targetname;
	actor_t *actor;
	struct actor_waiting_s *next;
} actor_waiting_t;

actor_waiting_t *actor_waiting_list;
detail_waiting_t *detail_waiting_list;

// deal with list
void Props_CreateDetails(void)
{
	detail_waiting_t *dnext;
	for(detail_waiting_t *d = detail_waiting_list; d; gi.TagFree(d), d = dnext)
	{
		dnext = d->next;
		int assigned = false;
		
		if (d->target[0])
		{
			for(actor_waiting_t *a = actor_waiting_list; a; a = a->next)
			{
				if (strcmp(a->targetname, d->target))
					continue;
				for(int i = 0; i < ACTOR_MAX_DETAILS; i++)
				{
					if (a->actor->details[i])
						continue;
					a->actor->details[i] = d->detail;
					assigned = true;
					break;
				}
				break;
			}
		}

		if (!assigned)
		{
			actor_t *actor = Actor_Spawn();
			VectorCopy(d->detail->s.origin, actor->origin);
			Actor_Link(actor, 256);
			actor->details[0] = d->detail;
		}
	}

	detail_waiting_t *anext;
	for(actor_waiting_t *a = actor_waiting_list; a; gi.TagFree(a), a = anext)
		anext = a->next;
}

// spawnfuncs

qboolean check_aabb(vec3_t point, vec3_t mins, vec3_t maxs)
{
	return ((point[0] > mins[0] && point[1] > mins[1] && point[2] > mins[2]) && (point[0] < maxs[0] && point[1] < maxs[1] && point[2] < maxs[2]));
}

void actor_bbox_evaluate(actor_t *actor, int *score)
{
	if (check_aabb(viewer_origin, actor->mins, actor->maxs))
		return;
	*score = ACTOR_DO_NOT_ADD;
	return;
}

void SP_actor_bbox(edict_t *ent)
{
	actor_waiting_t *list = gi.TagMalloc(sizeof(actor_waiting_t), TAG_LEVEL);
	list->targetname = ent->targetname;
	list->actor = Actor_Spawn();
	list->next = actor_waiting_list;
	actor_waiting_list = list;

	gi.setmodel(ent, ent->model);
	gi.linkentity(ent);
	VectorCopy(ent->absmin, list->actor->mins);
	VectorCopy(ent->absmax, list->actor->maxs);
	list->actor->evaluate = actor_bbox_evaluate;

	vec3_t diff;
	float magdiff;
	VectorSubtract(ent->absmax, ent->absmin, diff);
	magdiff = VectorLength(diff);
	VectorNormalize(diff);
	VectorMA(ent->absmin, magdiff * 0.5, diff, list->actor->origin);
	Actor_Link(list->actor, magdiff * 0.5);

	G_FreeEdict(ent);
}

void SP_info_detail(edict_t *ent)
{
	detail_waiting_t *list = gi.TagMalloc(sizeof(detail_waiting_t), TAG_LEVEL);
	list->detail = D_Spawn();
	list->target = ent->target;
	list->next = detail_waiting_list;
	detail_waiting_list = list;
	gi.linkentity(ent);
	
	VectorCopy(ent->s.origin, list->detail->s.origin);
	VectorCopy(ent->s.angles, list->detail->s.angles);
	list->detail->s.modelindex = gi.modelindex(ent->model);
	list->detail->s.frame = ent->s.frame;
	list->detail->s.skinnum = ent->s.skinnum;

	G_FreeEdict(ent);
}







