#pragma once

#define DAMAGEFLAG_YES				0x00000001
#define DAMAGEFLAG_BLOCKING			0x00000002
//#define MONSTERFLAG_BLOCKING		0x00000001

typedef struct monsterinfo_s monsterinfo_t;
typedef monsterinfo_t monsterinfo_s;

struct monsterinfo_s{
	uint32_t flags;
	
};

typedef struct m_skeletoninfo_s {
	monsterinfo_t base;
	
} m_skeletoninfo_t;

#define MONSTERDATA_SIZE \
	sizeof(union {monsterinfo_t m; m_skeletoninfo_t s;})

