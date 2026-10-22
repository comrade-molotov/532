/*
    playerdata.inc - player data (money / score / admin level) + MySQL handle

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

// Starting values for a fresh account
#define START_MONEY (5000)
#define START_SCORE (0)

#define MONEY_MAX   (999999999)

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
    pName[MAX_PLAYER_NAME]  // nickname, copied on connect (valid even after disconnect)
};

new PlayerData[MAX_PLAYERS][E_PLAYER_DATA];

// Single global MySQL connection handle (created in OnGameModeInit)
new MySQL:gDB;

stock Player_Reset(playerid)
{
    PlayerData[playerid][pID] = 0;
    PlayerData[playerid][pMoney] = 0;
    PlayerData[playerid][pScore] = 0;
    PlayerData[playerid][pAdmin] = 0;
    PlayerData[playerid][pLogged] = 0;
    PlayerData[playerid][pName][0] = EOS;
    return 1;
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
