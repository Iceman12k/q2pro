
#include "g_local.h"

static bool FloodProtect(edict_t *ent)
{
	int i, msgs = flood_msgs->value;
	gclient_t *cl = ent->client;

	if (msgs < 1)
		return false;

	if (level.time < cl->flood_locktill) {
		gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
			(int)(cl->flood_locktill - level.time));
		return true;
	}

	i = cl->flood_whenhead - min(msgs, FLOOD_MSGS) + 1;
	if (i < 0)
		i += FLOOD_MSGS;
	if (cl->flood_when[i] &&
		level.time - cl->flood_when[i] < flood_persecond->value) {
		cl->flood_locktill = level.time + flood_waitdelay->value;
		gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
			(int)flood_waitdelay->value);
		return true;
	}

	cl->flood_whenhead = (cl->flood_whenhead + 1) % FLOOD_MSGS;
	cl->flood_when[cl->flood_whenhead] = level.time;
	return false;
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f(edict_t *ent, bool team, bool arg0)
{
	int     j;
	edict_t *other;
	char    text[2048];

	if (gi.argc() < 2 && !arg0)
		return;

	if (FloodProtect(ent))
		return;

	Q_snprintf(text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0) {
		Q_strlcat(text, gi.argv(0), sizeof(text));
		Q_strlcat(text, " ", sizeof(text));
		Q_strlcat(text, gi.args(), sizeof(text));
	}
	else {
		Q_strlcat(text, COM_StripQuotes(gi.args()), sizeof(text));
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	Q_strlcat(text, "\n", sizeof(text));
	gi.cprintf(NULL, PRINT_CHAT, "%s", text);
}



/*
=================
ClientCommand
=================
*/
void ClientCommand(edict_t *ent)
{
	char    *cmd;

	if (!ent->client)
		return;     // not fully in game yet

	cmd = gi.argv(0);

	if (Q_stricmp(cmd, "say") == 0) {
		Cmd_Say_f(ent, false, false);
		return;
	}
	if (Q_stricmp(cmd, "say_team") == 0) {
		Cmd_Say_f(ent, true, false);
		return;
	}

	//if (level.intermission_framenum)
	//    return;

	/*
	if (Q_stricmp(cmd, "god") == 0)
		Cmd_God_f(ent);
	else if (Q_stricmp(cmd, "notarget") == 0)
		Cmd_Notarget_f(ent);
	else if (Q_stricmp(cmd, "noclip") == 0)
		Cmd_Noclip_f(ent);
	*/

	if (Q_stricmp(cmd, "use") == 0)
		return;
	else if (Q_stricmp(cmd, "drop") == 0)
		return;
	else if (Q_stricmp(cmd, "give") == 0)
		return;
	else if (Q_stricmp(cmd, "inven") == 0)
		return;
	else if (Q_stricmp(cmd, "invnext") == 0)
		return;
	else if (Q_stricmp(cmd, "invprev") == 0)
		return;
	else if (Q_stricmp(cmd, "invnextw") == 0)
		return;
	else if (Q_stricmp(cmd, "invprevw") == 0)
		return;
	else if (Q_stricmp(cmd, "invnextp") == 0)
		return;
	else if (Q_stricmp(cmd, "invprevp") == 0)
		return;
	else if (Q_stricmp(cmd, "invuse") == 0)
		return;
	else if (Q_stricmp(cmd, "invdrop") == 0)
		return;
	else if (Q_stricmp(cmd, "weapprev") == 0)
		return;
	else if (Q_stricmp(cmd, "weapnext") == 0)
		return;
	else if (Q_stricmp(cmd, "weaplast") == 0)
		return;
	else    // anything that doesn't match a command will be a chat
		Cmd_Say_f(ent, false, true);
}