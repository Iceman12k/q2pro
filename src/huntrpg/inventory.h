
#define INVEN_MAXANGLE	20
#define INVEN_DISTANCE	64
#define INVEN_SIZE		8
#define INVEN_WIDTH		3
#define INVEN_HEIGHT	4
#define INVEN_HOTBAR	5
#define INVEN_HOTBAR_START (INVEN_WIDTH * INVEN_HEIGHT)
#define INVEN_TOTALSLOTS (INVEN_HOTBAR_START + INVEN_HOTBAR)

#define INVEN_POS_TOP	-16
#define INVEN_POS_LEFT	-24

#define MAX_ITEMNAME	24

#define HOTBAR_RAISETIME 0.2

enum passiveflags_e
{
	PASSIVE_LIGHT = 1 << 0,

};


typedef struct item_s item_t;
typedef item_t item_s;

struct item_s {
	char name[MAX_ITEMNAME];
	char icon_model[MAX_QPATH];
	char icon_image[MAX_QPATH];

	int icon_frame;
	int icon_skin;

	char viewmodel[MAX_QPATH];
	int passive_flags;

	// actual attributes here
	void(*frame)(edict_t *ent, item_t *item, int *do_think);
	void(*endframe)(edict_t *ent, item_t *item);
};

extern item_t pickaxe;
extern item_t lantern;


