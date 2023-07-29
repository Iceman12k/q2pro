
#include "g_local.h"

#define HUD_JOB_SIZE	1024
#define HUD_JOB_AMOUNT	12

#define S(x) x " "

typedef struct {
	char s[HUD_JOB_SIZE];
} hudjob_t;
static int hudjob_i;
static hudjob_t hudjobs[HUD_JOB_AMOUNT];

static hudjob_t* H_AllocJob(void)
{
	hudjob_t *ret;
	ret = &hudjobs[hudjob_i];
	memset(ret->s, 0, sizeof(((hudjob_t *)0)->s)); // ew weird null pointer compiler abuse

	hudjob_i = min(hudjob_i + 1, HUD_JOB_AMOUNT - 1);
	return ret;
}

static void H_InventoryJob(edict_t *ent, gclient_t *client)
{
	if (client->inv_open == false)
		return;

	//if (client->inv_highlighted[0] != 0 || client->inv_highlighted[1] != 0)
	//	return;
	if (client->inv_highlighted != 0)
		return;

	hudjob_t *job = H_AllocJob();

	Q_scnprintf(job->s, HUD_JOB_SIZE,
		// info block
		S("xv 32")
		S("yb -128")
		S("picn %s")

		// text
		S("xv 106")
		S("yb -116")
		S("string2 %s")

		// stat1
		S("xv 106")
		S("yb -99")
		S("string2 \"%s: %i\"")

		// stat2
		S("xv 106")
		S("yb -85")
		S("string2 \"%s: %i\"")

		// stat3
		S("xv 106")
		S("yb -71")
		S("string2 \"%s: %i\"")
		,
		"infoblock",
		"Pickaxe",
		"DMG", 5,
		"SPD", 100,
		"DUR", 32
		);
}

void H_Update(edict_t *ent, gclient_t *client)
{
	char hudstr[HUD_MAX_SIZE];
	size_t hudsz = 0;
	if (level.framenum < client->hud_updateframe)
		return;

	// initialize our new hud
	memset(hudstr, 0, sizeof(hudstr));
	hudjob_i = 0;

	// add jobs
	H_InventoryJob(ent, client);
	//

	// loop through jobs and add all our strings together
	for (int i = 0; i < hudjob_i; i++)
	{
		size_t oldsz = hudsz;
		hudsz = Q_strlcat(hudstr, hudjobs[i].s, HUD_MAX_SIZE);

		if (hudsz >= HUD_MAX_SIZE)
		{
			hudstr[oldsz] = 0; // terminate at last job, and continue on, this one was too damn big!
			hudsz = oldsz;
			continue;
		}
	}

	if (memcmp(hudstr, client->hud_oldstring, HUD_MAX_SIZE))
	{
		client->hud_updateframe = level.framenum + (game.framediv * 2);
		memcpy(client->hud_oldstring, hudstr, HUD_MAX_SIZE);

		Com_Printf("%s\n", hudstr);

		// pack the message and...
		gi.WriteByte(svc_configstring);
		gi.WriteShort(CS_STATUSBAR);
		WriteData(hudstr, hudsz);
		gi.WriteByte(0);

		// ship 'er off!
		gi.unicast(ent, true);
	}
}


#undef S
