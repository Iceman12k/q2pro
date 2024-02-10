
#include "g_local.h"

const char *cycle_values[] = {
	"c",
	"d",
	"d",
	"d",
	"e",
	"e",
	"f",
	"f",
	"g",
	"g",
	"h",
	"h",
	"i",
	"i",
	"j",
	"j",
	"k",
	"k",
	"l",
	"l",
	"m",
	"m",
	"n",
	"n",
	"o",
	"o",
	"o",
	"p",
	"p",
	"q",
	"q",
	"r",
};

const char *cycle_title[] = {
	"Midnight",
	"Early Mornin\'",
	"Mornin\'",
	"High Noon",
	"Evenin\'",
	"Night",
};

const char *cycle_skybox[] = {
	"space1",
	"unit1_",
	"western_",
	"unit4_",
	"space1",
};

int cycle_skyboxdir[] = {
	110,  180, -10, 20, 40, 70, 130, 180, -20, 50
};

const float cycle_timebrightness[] = {
	0.06, //12am
	0.05, //1am
	0.04, //2am
	0.05, //3am
	0.1, //4am
	0.3, //5am
	0.6, //6am
	0.83, //7am
	0.85, //8am
	0.87, //9am
	0.92, //10am
	0.96, //11am
	0.99, //12pm
	0.99, //1pm
	0.99, //2pm
	0.95, //3pm
	0.9, //4pm
	0.8, //5pm
	0.6, //6pm
	0.5, //7pm
	0.4, //8pm
	0.12, //9pm
	0.1, //10pm
	0.1, //11pm
};

const char *timeofday_title;
const char *timeofday_brightness;
char timeofday_skybox;
char timeofday_skyboxdir;

#define TITLE_ENTRIES (sizeof(cycle_title) / sizeof(cycle_title[0]))
#define SKYBOX_ENTRIES (sizeof(cycle_skybox) / sizeof(cycle_skybox[0]))
#define SKYBOXDIR_ENTRIES (sizeof(cycle_skyboxdir) / sizeof(cycle_skyboxdir[0]))
#define BRIGHTNESS_ENTRIES (sizeof(cycle_values) / sizeof(cycle_values[0]))
#define BRIGHTNESS_STRING(x) cycle_values[(int)(x * (sizeof(cycle_values) / sizeof(cycle_values[0])))]
#define GET_TIME ((level.time / 2) + 720) // 48 minute day, starting at noon
#define GET_HOUR (((int)GET_TIME / 60) % 24)
#define GET_MINUTE ((int)GET_TIME % 60)

static void DayNight_SetBrightness(float hour)
{
	//const char *style = BRIGHTNESS_STRING(hour);
	float bright1 = cycle_timebrightness[(int)floor(hour) % 24];
	float bright2 = cycle_timebrightness[(int)ceil(hour) % 24];
	float frac = hour - floor(hour);
	float brightness = bright1 + ((bright2 - bright1) * frac);

	int index = brightness * BRIGHTNESS_ENTRIES;
	timeofday_brightness = cycle_values[index];
	//gi.configstring(CS_LIGHTS + 0, style);
}

static void DayNight_SetSkybox(float hour)
{
	int index;
	int dirindex;
	float time;

	time = (float)((int)(round(GET_TIME / 60)) % 24) / 24;
	index = (time * SKYBOX_ENTRIES);
	dirindex = (time * SKYBOXDIR_ENTRIES);

	timeofday_skybox = index;
	timeofday_skyboxdir = dirindex;
}

void Environment_GetTime(int *hour, int *minute, char *title, size_t len)
{
	if (hour)
		*hour = GET_HOUR;

	if (minute)
		*minute = GET_MINUTE;

	if (title)
	{
		int index;
		float time;
		const char *t;

		time = (float)((int)(round(GET_TIME / 60)) % 24) / 24;
		index = (time * TITLE_ENTRIES);

		t = cycle_title[index];
		Q_strlcpy(title, t, len);
	}
}

void Environment_Update(void)
{
	float hour = GET_TIME / 60;
	hour = hour - (floor(hour / 24) * 24);

	//Com_Printf("%.2f\n", hour);
	DayNight_SetBrightness(hour);
	DayNight_SetSkybox(hour);
}

void Environment_ClientUpdate(edict_t *ent)
{
	gclient_t *client = ent->client;
	char brightness = timeofday_brightness[0];

	if (client->passive_flags & PASSIVE_LIGHT)
	{
		if (brightness < 'm')
			brightness++;
		if (brightness < 'g')
			brightness++;
	}

	// if our brightness is mismatched or we need to resync because it's been a few seconds
	if ((brightness != client->light_oldvalue) || ((level.framenum - client->light_lastsync) > (FRAMEDIV * 40)))
	{
		client->light_oldvalue = brightness;
		client->light_lastsync = level.framenum;

		gi.WriteByte(svc_configstring);
		gi.WriteShort(CS_LIGHTS);
		gi.WriteByte(brightness);
		gi.WriteByte(0);

		gi.unicast(ent, false);
	}

	// if our skybox is mismatched or we need to resync because it's been a few seconds
	if ((timeofday_skybox != client->skybox_oldvalue) || ((level.framenum - client->skybox_lastsync) > (FRAMEDIV * 100)))
	{
		client->skybox_oldvalue = timeofday_skybox;
		client->skybox_lastsync = level.framenum;

		gi.WriteByte(svc_configstring);
		gi.WriteShort(CS_SKY);
		gi.WriteString(cycle_skybox[(int)timeofday_skybox]);

		//gi.WriteByte(svc_configstring);
		//gi.WriteShort(CS_SKYAXIS);
		//gi.WriteString(va("%.2f 0 0", (float)cycle_skyboxdir[(int)timeofday_skyboxdir]));

		gi.WriteByte(svc_stufftext);
		gi.WriteString(va("sky \"%s\"\n", cycle_skybox[(int)timeofday_skybox]));

		gi.unicast(ent, false);
	}
}