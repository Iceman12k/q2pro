#pragma once


// detail
#define MAX_DETAILS				8192
#define RESERVED_DETAILEDICTS	96
#define MAX_ACTORS				MAX_DETAILS
#define DETAIL_BUCKETS			100
#define DETAIL_MAXRANGE			(1024 * 1024)

#define DETAIL_PRIORITY_LOW		0x01
#define DETAIL_PRIORITY_HIGH	0x02
#define DETAIL_PRIORITY_MAX		0x03
#define DETAIL_NOLIMIT			0x04
#define DETAIL_NEARLIMIT		0x08

// vis chunks
#define CHUNK_SIZE 1024
#define CHUNK_WIDTH 8
#define CHUNK_HEIGHT 8
#define ORG_TO_CHUNK(v) ((uint64_t)((int)((v[0] / CHUNK_SIZE) + (CHUNK_WIDTH / 2)) + ((int)((v[1] / CHUNK_SIZE) + (CHUNK_HEIGHT / 2)) * CHUNK_WIDTH)))

typedef size_t bitfield_t;
#define BITS_PER_NUM (sizeof(bitfield_t) * 8)

#define DETAIL_USAGE_BOARDS (MAX_DETAILS + (BITS_PER_NUM - 1)) / BITS_PER_NUM
#define DETAIL_EDICT_BOARDS (RESERVED_DETAILEDICTS + (BITS_PER_NUM - 1)) / BITS_PER_NUM
typedef bitfield_t detailusagefield_t[DETAIL_USAGE_BOARDS];
typedef bitfield_t detailedictfield_t[DETAIL_EDICT_BOARDS];

//extern detailusagefield_t detail_sent_to_client[MAX_CLIENTS];
//extern detailedictfield_t reserved_edicts_available[MAX_CLIENTS];

extern edict_t *viewer_edict;
extern vec3_t viewer_origin;
extern int viewer_clientnum;

// actors
#define ACTOR_DO_NOT_ADD 0
#define ACTOR_MAX_DETAILS 64
typedef struct actor_s {
	qboolean inuse;
	uint64_t chunks_visible;
	
	vec3_t origin;
	vec3_t mins;
	vec3_t maxs;

	detail_edict_t *details[ACTOR_MAX_DETAILS];
	void(*evaluate)(struct actor_s *actor, int *score);
	int(*addtoscene)(struct actor_s *actor, int score);
} actor_t;




