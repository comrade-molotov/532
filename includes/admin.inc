/*
    admin.inc - admin levels, admin commands and a light anticheat

    Levels
        1   /kick /mute /goto
        2   + /ban /get /weather /time
        3   + /setscore

    Every admin also gets /a (admin chat).
*/

#if defined _INC_admin
    #endinput
#endif
#define _INC_admin

#include <a_samp>
#include <a_mysql>
#include <sscanf2>
#include "playerdata.inc"

#define AC_MAX_WARNINGS (3)
#define AC_TICK_MS      (1000)

// Cars/planes above this speed are flagged (km/h)
#define AC_MAX_SPEED    (500.0)
// On foot: moving further than this in one tick is a teleport hack
#define AC_MAX_DISTANCE (300.0)

enum E_AC
{
    Float:acX,
    Float:acY,
    Float:acZ,
    acWarnings,
    acIgnoreUntil,
    acIgnored
};

new ACData[MAX_PLAYERS][E_AC];

forward Admin_OnBanCheck(playerid);
forward Admin_OnAnticheatTick();


// =====================================================================
//  Helpers
// =====================================================================

stock Admin_SendToAdmins(const message[])
{
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (IsPlayerConnected(i) && Player_IsAdmin(i, 1))
        {
            SendClientMessage(i, COLOR_INFO, message);
        }
    }
    return 1;
}

// Used by commands that move a player around, so the anticheat does not flag us
stock Admin_IgnorePosition(playerid, seconds)
{
    ACData[playerid][acIgnoreUntil] = gettime() + seconds;
    ACData[playerid][acIgnored] = true;

    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    ACData[playerid][acX] = x;
    ACData[playerid][acY] = y;
    ACData[playerid][acZ] = z;
    return 1;
}

stock Admin_ShowCommands(playerid)
{
    SendClientMessage(playerid, COLOR_INFO, "Admin level 1: /kick /mute /goto /a");
    SendClientMessage(playerid, COLOR_INFO, "Admin level 2: /ban /get /weather /time");
    SendClientMessage(playerid, COLOR_INFO, "Admin level 3: /setscore");
    return 1;
}


// =====================================================================
//  Callbacks
// =====================================================================

stock Admin_OnPlayerConnect(playerid)
{
    ACData[playerid][acWarnings] = 0;
    ACData[playerid][acIgnoreUntil] = 0;
    ACData[playerid][acIgnored] = true;     // ignore until the first spawn settles
    ACData[playerid][acX] = 0.0;
    ACData[playerid][acY] = 0.0;
    ACData[playerid][acZ] = 0.0;

    new query[192];
    new ip[16];
    GetPlayerIp(playerid, ip, sizeof(ip));

    mysql_format(gDB, query, sizeof(query),
        "SELECT id FROM bans WHERE username = '%e' OR ip = '%e' LIMIT 1",
        PlayerData[playerid][pName], ip);

    mysql_tquery(gDB, query, "Admin_OnBanCheck", "i", playerid);
    return 1;
}

public Admin_OnBanCheck(playerid)
{
    if (!IsPlayerConnected(playerid)) return 1;
    if (cache_num_rows() == 0) return 1;

    SendClientMessage(playerid, COLOR_ERROR, "You are banned from this server.");
    Kick(playerid);
    return 1;
}

public Admin_OnAnticheatTick()
{
    new now = gettime();
    new Float:x, Float:y, Float:z;
    new Float:health, Float:armour;

    for (new playerid = 0; playerid < MAX_PLAYERS; playerid++)
    {
        if (!IsPlayerConnected(playerid)) continue;
        if (!Player_IsLogged(playerid)) continue;
        if (Player_IsAdmin(playerid, 1)) continue;          // admins teleport around

        GetPlayerPos(playerid, x, y, z);

        // Skip a tick right after a spawn/teleport, then start measuring
        if (ACData[playerid][acIgnored] || now < ACData[playerid][acIgnoreUntil])
        {
            ACData[playerid][acX] = x;
            ACData[playerid][acY] = y;
            ACData[playerid][acZ] = z;
            if (now >= ACData[playerid][acIgnoreUntil])
            {
                ACData[playerid][acIgnored] = false;
            }
            continue;
        }

        // teleport / airbreak on foot
        if (!IsPlayerInAnyVehicle(playerid))
        {
            new Float:dist = floatsqroot(
                ((x - ACData[playerid][acX]) * (x - ACData[playerid][acX])) +
                ((y - ACData[playerid][acY]) * (y - ACData[playerid][acY])) +
                ((z - ACData[playerid][acZ]) * (z - ACData[playerid][acZ])));

            if (dist > AC_MAX_DISTANCE && !IsPlayerInAnyVehicle(playerid))
            {
                ACData[playerid][acWarnings]++;
            }
        }
        else
        {
            new vehicleid = GetPlayerVehicleID(playerid);
            new Float:vx, Float:vy, Float:vz;
            GetVehicleVelocity(vehicleid, vx, vy, vz);
            new Float:speed = floatsqroot((vx * vx) + (vy * vy) + (vz * vz)) * 180.0;

            if (speed > AC_MAX_SPEED)
            {
                ACData[playerid][acWarnings]++;
            }
        }

        // godmode / health hack
        GetPlayerHealth(playerid, health);
        GetPlayerArmour(playerid, armour);
        if (health > 100.0 || armour > 100.0)
        {
            SetPlayerHealth(playerid, 100.0);
            SetPlayerArmour(playerid, 0.0);
            ACData[playerid][acWarnings]++;
        }

        if (ACData[playerid][acWarnings] >= AC_MAX_WARNINGS)
        {
            new string[128];
            format(string, sizeof(string), "[AC] %s kicked: movement/health anomaly (%d warnings)",
                PlayerData[playerid][pName], ACData[playerid][acWarnings]);
            Admin_SendToAdmins(string);
            Kick(playerid);
            continue;
        }

        ACData[playerid][acX] = x;
        ACData[playerid][acY] = y;
        ACData[playerid][acZ] = z;
        ACData[playerid][acIgnored] = false;
    }
    return 1;
}

stock Admin_OnCommandText(playerid, const cmdtext[])
{
    new cmd[16];
    new params[128];
    sscanf(cmdtext, "s[16]s[128]", cmd, params);

    // ---- admin chat (any admin level) ----
    if (strcmp(cmd, "/a", true) == 0)
    {
        if (!Player_IsAdmin(playerid, 1))
        {
            SendClientMessage(playerid, COLOR_ERROR, "You are not an admin.");
            return 1;
        }
        if (params[0] == EOS)
        {
            SendClientMessage(playerid, COLOR_INFO, "Usage: /a [message]");
            return 1;
        }

        new string[144];
        format(string, sizeof(string), "[ADMIN] %s: %s", PlayerData[playerid][pName], params);
        Admin_SendToAdmins(string);
        return 1;
    }

    if (strcmp(cmd, "/ahelp", true) == 0)
    {
        if (!Player_IsAdmin(playerid, 1)) return 1;
        Admin_ShowCommands(playerid);
        return 1;
    }

    // ---- level 1 ----
    if (strcmp(cmd, "/kick", true) == 0)
    {
        if (!Player_IsAdmin(playerid, 1)) return 0;

        new targetid, reason[64];
        if (sscanf(params, "ds[64]", targetid, reason))
        {
            if (sscanf(params, "d", targetid))
            {
                SendClientMessage(playerid, COLOR_INFO, "Usage: /kick [playerid] [reason]");
                return 1;
            }
            reason = "no reason";
        }
        if (!IsPlayerConnected(targetid) || targetid == playerid)
        {
            SendClientMessage(playerid, COLOR_ERROR, "Invalid player.");
            return 1;
        }

        new string[144];
        format(string, sizeof(string), "%s was kicked by %s (%s)",
            PlayerData[targetid][pName], PlayerData[playerid][pName], reason);
        SendClientMessageToAll(COLOR_ERROR, string);

        Kick(targetid);
        return 1;
    }

    if (strcmp(cmd, "/mute", true) == 0)
    {
        if (!Player_IsAdmin(playerid, 1)) return 0;

        new targetid;
        if (sscanf(params, "d", targetid))
        {
            SendClientMessage(playerid, COLOR_INFO, "Usage: /mute [playerid]");
            return 1;
        }
        if (!IsPlayerConnected(targetid))
        {
            SendClientMessage(playerid, COLOR_ERROR, "Invalid player.");
            return 1;
        }

        PlayerData[targetid][pMuted] = !PlayerData[targetid][pMuted];

        new string[144];
        format(string, sizeof(string), "%s has been %s by %s",
            PlayerData[targetid][pName],
            PlayerData[targetid][pMuted] ? "muted" : "unmuted",
            PlayerData[playerid][pName]);
        SendClientMessageToAll(COLOR_INFO, string);
        return 1;
    }

    if (strcmp(cmd, "/goto", true) == 0)
    {
        if (!Player_IsAdmin(playerid, 1)) return 0;

        new targetid;
        if (sscanf(params, "d", targetid))
        {
            SendClientMessage(playerid, COLOR_INFO, "Usage: /goto [playerid]");
            return 1;
        }
        if (!IsPlayerConnected(targetid) || targetid == playerid)
        {
            SendClientMessage(playerid, COLOR_ERROR, "Invalid player.");
            return 1;
        }

        new Float:x, Float:y, Float:z;
        GetPlayerPos(targetid, x, y, z);

        if (GetPlayerState(playerid) == PLAYER_STATE_DRIVER)
        {
            SetVehiclePos(GetPlayerVehicleID(playerid), x + 2.0, y + 2.0, z + 1.0);
        }
        else
        {
            SetPlayerPos(playerid, x + 1.0, y + 1.0, z + 1.0);
        }

        Admin_IgnorePosition(playerid, 3);

        new string[144];
        format(string, sizeof(string), "You teleported to %s.", PlayerData[targetid][pName]);
        SendClientMessage(playerid, COLOR_OK, string);
        return 1;
    }

    // ---- level 2 ----
    if (strcmp(cmd, "/ban", true) == 0)
    {
        if (!Player_IsAdmin(playerid, 2)) return 0;

        new targetid, reason[64];
        if (sscanf(params, "ds[64]", targetid, reason))
        {
            if (sscanf(params, "d", targetid))
            {
                SendClientMessage(playerid, COLOR_INFO, "Usage: /ban [playerid] [reason]");
                return 1;
            }
            reason = "no reason";
        }
        if (!IsPlayerConnected(targetid) || targetid == playerid)
        {
            SendClientMessage(playerid, COLOR_ERROR, "Invalid player.");
            return 1;
        }
        if (Player_IsAdmin(targetid, 1))
        {
            SendClientMessage(playerid, COLOR_ERROR, "You cannot ban another admin.");
            return 1;
        }

        new ip[16];
        GetPlayerIp(targetid, ip, sizeof(ip));

        new query[320];
        mysql_format(gDB, query, sizeof(query),
            "INSERT INTO bans (username, ip, reason, admin) VALUES ('%e', '%e', '%e', '%e')",
            PlayerData[targetid][pName], ip, reason, PlayerData[playerid][pName]);
        mysql_tquery(gDB, query);

        new string[144];
        format(string, sizeof(string), "%s was banned by %s (%s)",
            PlayerData[targetid][pName], PlayerData[playerid][pName], reason);
        SendClientMessageToAll(COLOR_ERROR, string);

        Kick(targetid);
        return 1;
    }

    if (strcmp(cmd, "/get", true) == 0)
    {
        if (!Player_IsAdmin(playerid, 2)) return 0;

        new targetid;
        if (sscanf(params, "d", targetid))
        {
            SendClientMessage(playerid, COLOR_INFO, "Usage: /get [playerid]");
            return 1;
        }
        if (!IsPlayerConnected(targetid) || targetid == playerid)
        {
            SendClientMessage(playerid, COLOR_ERROR, "Invalid player.");
            return 1;
        }

        new Float:x, Float:y, Float:z;
        GetPlayerPos(playerid, x, y, z);

        SetPlayerPos(targetid, x + 1.0, y + 1.0, z + 1.0);
        Admin_IgnorePosition(targetid, 3);

        new string[144];
        format(string, sizeof(string), "You brought %s to you.", PlayerData[targetid][pName]);
        SendClientMessage(playerid, COLOR_OK, string);

        format(string, sizeof(string), "You were brought to %s.", PlayerData[playerid][pName]);
        SendClientMessage(targetid, COLOR_INFO, string);
        return 1;
    }

    if (strcmp(cmd, "/weather", true) == 0)
    {
        if (!Player_IsAdmin(playerid, 2)) return 0;

        new id;
        if (sscanf(params, "d", id))
        {
            SendClientMessage(playerid, COLOR_INFO, "Usage: /weather [0-20]");
            return 1;
        }
        if (id < 0 || id > 20)
        {
            SendClientMessage(playerid, COLOR_ERROR, "Weather id must be 0-20.");
            return 1;
        }

        SetWeather(id);

        new string[144];
        format(string, sizeof(string), "[ADMIN] %s set the weather to %d", PlayerData[playerid][pName], id);
        SendClientMessageToAll(COLOR_INFO, string);
        return 1;
    }

    if (strcmp(cmd, "/time", true) == 0)
    {
        if (!Player_IsAdmin(playerid, 2)) return 0;

        new hour;
        if (sscanf(params, "d", hour))
        {
            SendClientMessage(playerid, COLOR_INFO, "Usage: /time [0-23]");
            return 1;
        }
        if (hour < 0 || hour > 23)
        {
            SendClientMessage(playerid, COLOR_ERROR, "Hour must be 0-23.");
            return 1;
        }

        SetWorldTime(hour);

        new string[144];
        format(string, sizeof(string), "[ADMIN] %s set the time to %02d:00", PlayerData[playerid][pName], hour);
        SendClientMessageToAll(COLOR_INFO, string);
        return 1;
    }

    // ---- level 3 ----
    if (strcmp(cmd, "/setscore", true) == 0)
    {
        if (!Player_IsAdmin(playerid, 3)) return 0;

        new targetid, score;
        if (sscanf(params, "dd", targetid, score))
        {
            SendClientMessage(playerid, COLOR_INFO, "Usage: /setscore [playerid] [score]");
            return 1;
        }
        if (!IsPlayerConnected(targetid) || targetid == playerid)
        {
            SendClientMessage(playerid, COLOR_ERROR, "Invalid player.");
            return 1;
        }

        Player_SetScore(targetid, score);

        new string[144];
        format(string, sizeof(string), "[ADMIN] %s set %s's score to %d",
            PlayerData[playerid][pName], PlayerData[targetid][pName], PlayerData[targetid][pScore]);
        SendClientMessageToAll(COLOR_INFO, string);
        return 1;
    }

    return 0;
}
