
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

static void H_HotbarJob(edict_t *ent, gclient_t *client)
{
	hudjob_t *job = H_AllocJob();
	char temp_s[HUD_JOB_SIZE];
	job->s[0] = 0; // null terminate the end really quick

	float frac = ((ent->client->hotbar_animtime - level.time) / HOTBAR_RAISETIME);
	int hotbar_height = -256;
	hotbar_height *= clamp(frac, 0, 1);

	if (!client->hotbar_open)
	{
		Q_scnprintf(temp_s, HUD_JOB_SIZE,
			// info block
			S("xr -64")
			S("yt %i")
			S("picn %s")
			,
			hotbar_height,
			"hotbarc"
		);
		Q_strlcat(job->s, temp_s, HUD_JOB_SIZE); temp_s[0] = 0;
	}
	else
	{
		Q_scnprintf(temp_s, HUD_JOB_SIZE,
			// info block
			S("xr -64")
			S("yt %i")
			S("picn %s")
			,
			hotbar_height,
			"hotbar"
		);
		Q_strlcpy(job->s, temp_s, HUD_JOB_SIZE); temp_s[0] = 0;

		for (int i = INVEN_HOTBAR_START; i < INVEN_TOTALSLOTS; i++)
		{
			if (client->hotbar_selected == i)
			{
				Q_scnprintf(temp_s, HUD_JOB_SIZE,
					" "
					S("xr %i")
					S("yt %i")
					S("picn %s")
					,
					-64,
					hotbar_height + ((i - INVEN_HOTBAR_START) * 57),
					"hotbars"
				);
				Q_strlcat(job->s, temp_s, HUD_JOB_SIZE); temp_s[0] = 0;
			}

			item_t *item = client->inv_content[i];
			if (!item)
				continue;

			Q_scnprintf(temp_s, HUD_JOB_SIZE,
				" "
				S("xr %i")
				S("yt %i")
				S("picn %s")
				,
				-64 + 11,
				hotbar_height + 6 + ((i - INVEN_HOTBAR_START) * 57),
				item->icon_image
			);
			Q_strlcat(job->s, temp_s, HUD_JOB_SIZE); temp_s[0] = 0;
		}
	}
}

static void H_InventoryJob(edict_t *ent, gclient_t *client)
{
	if (client->inv_open == false)
		return;

	//if (client->inv_highlighted[0] != 0 || client->inv_highlighted[1] != 0)
	//	return;
	if (client->inv_highlighted < 0)
		return;

	if (client->inv_content[client->inv_highlighted] == NULL)
		return;

	hudjob_t *job = H_AllocJob();
	item_t *item = client->inv_content[client->inv_highlighted];

	Q_scnprintf(job->s, HUD_JOB_SIZE,
		// info block
		S("xv 32")
		S("yb -128")
		S("picn %s")

		// item pic
		S("xv 46")
		S("yb -114")
		S("picn %s")

		// text
		S("xv 106")
		S("yb -116")
		S("string2 %s")

		// stat1
		S("xv 106")
		S("yb -101")
		S("string2 \"%s: %i\"")

		// stat2
		S("xv 106")
		S("yb -87")
		S("string2 \"%s: %i\"")

		// stat3
		S("xv 106")
		S("yb -73")
		S("string2 \"%s: %i\"")
		,
		"infoblock",
		item->icon_image,
		item->name,
		"DMG", 5,
		"SPD", 100,
		"DUR", 32
		);
}

static void H_ClockJob(edict_t *ent, gclient_t *client)
{
	hudjob_t *job = H_AllocJob();
#if 0
	int h, m;
	Environment_GetTime(&h, &m);

	Q_scnprintf(job->s, HUD_JOB_SIZE,
		S("xl 8")
		S("yb -16")
		S("string %.2i:%.2i")
		,
		h,
		m
	);
#else
	int h, m;
	char title[64];
	Environment_GetTime(&h, &m, title, sizeof(title));

	Q_scnprintf(job->s, HUD_JOB_SIZE,
		S("xl 8")
		S("yb -32")
		S("string \"%.2i:%.2i\"")

		S("xl 8")
		S("yb -16")
		S("string \"%s\"")
		,
		h,
		m,
		title
	);
#endif
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
	H_HotbarJob(ent, client);
	H_ClockJob(ent, client);
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

	if (memcmp(hudstr, client->hud_oldstring, HUD_MAX_SIZE) || ((level.framenum - client->hud_lastsync) > (game.framediv * 40)))
	{
		// determine how long we should wait before updating again
		int cost = 1;
		cost += (int)((client->ping / (game.frametime * 1000)) * 0.6667);

		// biiiig boys
		if (hudsz > 1200)
			cost = game.framediv * 3;
		else if (hudsz > 800)
			cost = game.framediv * 2;
		else if (hudsz > 400)
			cost = game.framediv;
		//

		client->hud_updateframe = level.framenum + cost;
		memcpy(client->hud_oldstring, hudstr, HUD_MAX_SIZE);

		//Com_Printf("%s\n", hudstr);

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
