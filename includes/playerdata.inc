/*
    playerdata.inc - player data (money / score / admin level) + MySQL handle
                     and the /stats + /top commands

    This is the lowest layer of the script: it owns the exact player data
    we keep in memory and the single global MySQL handle.

    Rule: money is ALWAYS read from PlayerData[playerid][pMoney].
    Never use GetPlayerMoney() as a source of truth.
*/

#if defined _INC_playerdata
    #endinput
#endif
#define _INC_playerdata

#include <a_samp>
#include <a_mysql>
#include <sscanf2>

// Starting values for a fresh account
#define START_MONEY (5000)
#define START_SCORE (0)

#define MONEY_MAX   (999999999)

#define DIALOG_STATS (1010)
#define DIALOG_TOP   (1011)

// Shared message colours (0xRRGGBBAA)
#define COLOR_WHITE (0xFFFFFFFF)
#define COLOR_ERROR (0xFF5555FF)
#define COLOR_OK    (0x55FF55FF)
#define COLOR_INFO  (0xFFD700FF)

enum E_PLAYER_DATA
{
    pID,                    // accounts.id, 0 = not logged in
    pMoney,                 // money, mirrored to the client with GivePlayerMoney
    pScore,                 // score, mirrored with SetPlayerScore
    pAdmin,                 // admin level, 0 = normal player, 1-3 = admin
    pLogged,                // 1 = logged in
    pMuted,                 // 1 = chat muted by an admin
    pName[MAX_PLAYER_NAME]  // nickname, copied on connect (valid even after disconnect)
};

new PlayerData[MAX_PLAYERS][E_PLAYER_DATA];

// Single global MySQL connection handle (created in OnGameModeInit)
new MySQL:gDB;

forward PlayerData_OnTopQuery(playerid);

stock Player_Reset(playerid)
{
    PlayerData[playerid][pID] = 0;
    PlayerData[playerid][pMoney] = 0;
    PlayerData[playerid][pScore] = 0;
    PlayerData[playerid][pAdmin] = 0;
    PlayerData[playerid][pLogged] = 0;
    PlayerData[playerid][pMuted] = 0;
    PlayerData[playerid][pName][0] = EOS;
    return 1;
}

stock Player_IsLogged(playerid)
{
    return PlayerData[playerid][pLogged];
}

stock Player_IsAdmin(playerid, level = 1)
{
    return (PlayerData[playerid][pLogged] && PlayerData[playerid][pAdmin] >= level);
}

stock Player_GetMoney(playerid)
{
    return PlayerData[playerid][pMoney];
}

// Single place where money changes: keeps our value and the client value in sync.
stock Player_SetMoney(playerid, amount)
{
    if (amount < 0) amount = 0;
    if (amount > MONEY_MAX) amount = MONEY_MAX;

    PlayerData[playerid][pMoney] = amount;
    ResetPlayerMoney(playerid);
    GivePlayerMoney(playerid, amount);
    return 1;
}

stock Player_AddMoney(playerid, amount)
{
    return Player_SetMoney(playerid, PlayerData[playerid][pMoney] + amount);
}

// Single place where score changes: keeps our value and the client value in sync.
stock Player_SetScore(playerid, amount)
{
    if (amount < 0) amount = 0;

    PlayerData[playerid][pScore] = amount;
    SetPlayerScore(playerid, amount);
    return 1;
}

// Writes money/score back to the accounts table. Safe to call on disconnect.
stock Player_SaveData(playerid)
{
    if (PlayerData[playerid][pID] <= 0) return 0;

    new query[192];
    mysql_format(gDB, query, sizeof(query),
        "UPDATE accounts SET money = %d, score = %d WHERE id = %d",
        PlayerData[playerid][pMoney],
        PlayerData[playerid][pScore],
        PlayerData[playerid][pID]);

    mysql_tquery(gDB, query);
    return 1;
}


// =====================================================================
//  /stats and /top
// =====================================================================

stock PlayerData_ShowStats(playerid, toplayerid)
{
    new string[512];

    format(string, sizeof(string), "Account: %s (ID %d)\nMoney: $%d\nScore: %d\nAdmin level: %d\nStatus: %s",
        PlayerData[toplayerid][pName],
        PlayerData[toplayerid][pID],
        PlayerData[toplayerid][pMoney],
        PlayerData[toplayerid][pScore],
        PlayerData[toplayerid][pAdmin],
        PlayerData[toplayerid][pLogged] ? "logged in" : "not logged in");

    ShowPlayerDialog(playerid, DIALOG_STATS, DIALOG_STYLE_MSGBOX, "Player stats", string, "Close", "");
    return 1;
}

stock PlayerData_OnCommandText(playerid, const cmdtext[])
{
    if (strcmp(cmdtext, "/stats", true) == 0)
    {
        if (!Player_IsLogged(playerid))
        {
            SendClientMessage(playerid, COLOR_ERROR, "You must be logged in.");
            return 1;
        }
        PlayerData_ShowStats(playerid, playerid);
        return 1;
    }

    if (strcmp(cmdtext, "/top", true) == 0)
    {
        if (!Player_IsLogged(playerid))
        {
            SendClientMessage(playerid, COLOR_ERROR, "You must be logged in.");
            return 1;
        }

        mysql_tquery(gDB,
            "SELECT username, money FROM accounts ORDER BY money DESC, id ASC LIMIT 10",
            "PlayerData_OnTopQuery", "i", playerid);
        return 1;
    }

    return 0;
}

public PlayerData_OnTopQuery(playerid)
{
    if (!IsPlayerConnected(playerid)) return 1;

    new string[1024];
    new line[144];
    new rows = cache_num_rows();
    new username[MAX_PLAYER_NAME];
    new money;

    format(string, sizeof(string), "Top 10 - money\n\n");
    if (rows == 0)
    {
        strcat(string, "No accounts yet.");
    }

    for (new i = 0; i < rows && i < 10; i++)
    {
        cache_get_value_name(i, "username", username, sizeof(username));
        cache_get_value_name_int(i, "money", money);
        format(line, sizeof(line), "%d. %s - $%d\n", i + 1, username, money);
        strcat(string, line, sizeof(string));
    }

    ShowPlayerDialog(playerid, DIALOG_TOP, DIALOG_STYLE_MSGBOX, "Top players", string, "Close", "");
    return 1;
}
