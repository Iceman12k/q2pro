
#include "g_local.h"

void SP_worldspawn(edict_t *ent)
{
	ent->solid = SOLID_BSP;
	ent->inuse = true;          // since the world doesn't use G_Spawn()
	ent->s.modelindex = 1;      // world model is always index 1

	// make some data visible to the server

	if (ent->message && ent->message[0]) {
		gi.configstring(CS_NAME, ent->message);
		Q_strlcpy(level.level_name, ent->message, sizeof(level.level_name));
	}
	else
		Q_strlcpy(level.level_name, level.mapname, sizeof(level.level_name));

	if (st.sky && st.sky[0])
		gi.configstring(CS_SKY, st.sky);
	else
		gi.configstring(CS_SKY, "unit1_");

	gi.configstring(CS_SKYROTATE, va("%f", st.skyrotate));

	gi.configstring(CS_SKYAXIS, va("%f %f %f",
		st.skyaxis[0], st.skyaxis[1], st.skyaxis[2]));

	gi.configstring(CS_MAXCLIENTS, va("%i", (int)(maxclients->value)));

	//---------------

	// precache assets
	gi.modelindex("models/null.md2");
	gi.modelindex("models/hud/bar.md2");
	gi.modelindex("models/weapons/v_pickaxe.md2");
	gi.modelindex("models/inven/square.md2");

	gi.modelindex("models/objects/gibs/head2/tris.md2");

	//
	// initialize stuff
	//
	I_Initialize();
	D_Initialize();

	//for (int i = -256; i <= 256; i += 32)
	{
		//for (int j = -64; j <= 64; j += 32)
		{
			for (int k = -128; k <= 128; k += 32)
			{
				detail_edict_t *head = D_Spawn();
				head->s.modelindex = gi.modelindex("models/objects/gibs/head2/tris.md2");
				head->classname = "detail_head";
				VectorSet(head->s.origin, 0, k, 0);
			}
		}
	}



	//
	// Setup light animation tables. 'a' is total darkness, 'z' is doublebright.
	//

	// 0 normal
	gi.configstring(CS_LIGHTS + 0, "m");

	// 1 FLICKER (first variety)
	gi.configstring(CS_LIGHTS + 1, "mmnmmommommnonmmonqnmmo");

	// 2 SLOW STRONG PULSE
	gi.configstring(CS_LIGHTS + 2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");

	// 3 CANDLE (first variety)
	gi.configstring(CS_LIGHTS + 3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");

	// 4 FAST STROBE
	gi.configstring(CS_LIGHTS + 4, "mamamamamama");

	// 5 GENTLE PULSE 1
	gi.configstring(CS_LIGHTS + 5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");

	// 6 FLICKER (second variety)
	gi.configstring(CS_LIGHTS + 6, "nmonqnmomnmomomno");

	// 7 CANDLE (second variety)
	gi.configstring(CS_LIGHTS + 7, "mmmaaaabcdefgmmmmaaaammmaamm");

	// 8 CANDLE (third variety)
	gi.configstring(CS_LIGHTS + 8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");

	// 9 SLOW STROBE (fourth variety)
	gi.configstring(CS_LIGHTS + 9, "aaaaaaaazzzzzzzz");

	// 10 FLUORESCENT FLICKER
	gi.configstring(CS_LIGHTS + 10, "mmamammmmammamamaaamammma");

	// 11 SLOW PULSE NOT FADE TO BLACK
	gi.configstring(CS_LIGHTS + 11, "abcdefghijklmnopqrrqponmlkjihgfedcba");

	// styles 32-62 are assigned by the light program for switchable lights

	// 25 testing
	gi.configstring(CS_LIGHTS + 25, "m");
}