#pragma once


// detail
#define MAX_DETAILS				8192
#define RESERVED_DETAILEDICTS	64
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

typedef struct actor_s actor_t;
typedef actor_t actor_s;

// actors
#define ACTOR_DO_NOT_ADD 0
#define ACTOR_MAX_DETAILS 64
typedef struct actor_s {
	qboolean inuse;
	uint64_t chunks_visible;
	
	vec3_t origin;
	vec3_t mins;
	vec3_t maxs;

	edict_t *owner;
	detail_edict_t *details[ACTOR_MAX_DETAILS];
	void(*aevaluate)(actor_t *actor, int *score);
	int(*aaddtoscene)(actor_t *actor, int score);

	int(*aphysics)(actor_t *actor);

    // begin fields
    int cnt;
    int anextthink;
    void (*athink)(actor_t *self);
} actor_t;

extern actor_t actor_list[MAX_ACTORS];

// msvc count trailing zeroes
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>

static __forceinline int __builtin_ctz(unsigned x)
{
#if defined(_M_ARM) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64) || defined(_M_ARM64EC)
    return (int)_CountTrailingZeros(x);
#elif defined(__AVX2__) || defined(__BMI__)
    return (int)_tzcnt_u32(x);
#else
    unsigned long r;
    _BitScanForward(&r, x);
    return (int)r;
#endif
}

static __forceinline int __builtin_ctzll(unsigned long long x)
{
#if defined(_M_ARM) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64) || defined(_M_ARM64EC)
    return (int)_CountTrailingZeros64(x);
#elif defined(_WIN64)
#if defined(__AVX2__) || defined(__BMI__)
    return (int)_tzcnt_u64(x);
#else
    unsigned long r;
    _BitScanForward64(&r, x);
    return (int)r;
#endif
#else
    int l = __builtin_ctz((unsigned)x);
    int h = __builtin_ctz((unsigned)(x >> 32)) + 32;
    return !!((unsigned)x) ? l : h;
#endif
}

static __forceinline int __builtin_ctzl(unsigned long x)
{
    return sizeof(x) == 8 ? __builtin_ctzll(x) : __builtin_ctz((unsigned)x);
}

static __forceinline int __builtin_clz(unsigned x)
{
#if defined(_M_ARM) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64) || defined(_M_ARM64EC)
    return (int)_CountLeadingZeros(x);
#elif defined(__AVX2__) || defined(__LZCNT__)
    return (int)_lzcnt_u32(x);
#else
    unsigned long r;
    _BitScanReverse(&r, x);
    return (int)(r ^ 31);
#endif
}

static __forceinline int __builtin_clzll(unsigned long long x)
{
#if defined(_M_ARM) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64) || defined(_M_ARM64EC)
    return (int)_CountLeadingZeros64(x);
#elif defined(_WIN64)
#if defined(__AVX2__) || defined(__LZCNT__)
    return (int)_lzcnt_u64(x);
#else
    unsigned long r;
    _BitScanReverse64(&r, x);
    return (int)(r ^ 63);
#endif
#else
    int l = __builtin_clz((unsigned)x) + 32;
    int h = __builtin_clz((unsigned)(x >> 32));
    return !!((unsigned)(x >> 32)) ? h : l;
#endif
}

static __forceinline int __builtin_clzl(unsigned long x)
{
    return sizeof(x) == 8 ? __builtin_clzll(x) : __builtin_clz((unsigned)x);
}
#endif // defined(_MSC_VER) && !defined(__clang__)
