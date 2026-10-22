/*
    freeroam.pwn - Freeroam gamemode (GL style, SA-MP 0.3.7)

    Modules
        includes/playerdata.inc    money / score / admin level, MySQL handle, /stats /top
        includes/accounts.inc      register / login (username + password, SHA256)
        includes/vehicles.inc      /v, drift mode, drift duel
        includes/casino.inc        blackjack, poker, roulette, slots
        includes/admin.inc         admin levels, admin commands, anticheat

    This file only wires things together: MySQL setup, table creation,
    SA-MP callbacks, timers, the teleport menu (/tp) and the skin menu (/skin).
*/

#include <a_samp>
#include <a_mysql>
#include <sscanf2>

// ==================== DATABASE CONFIG ====================
// Edit these four lines to match your MySQL server.
#define DB_HOST "127.0.0.1"
#define DB_USER "root"
#define DB_PASS ""
#define DB_NAME "freeroam"

// ==================== SCRIPT INCLUDES ====================
// Paths are relative to this file, so it compiles from any working directory.
#include "../includes/playerdata.inc"
#include "../includes/accounts.inc"
#include "../includes/vehicles.inc"
#include "../includes/casino.inc"
#include "../includes/admin.inc"

// ==================== TELEPORT / SKIN MENUS ====================
#define DIALOG_TELEPORT (3000)
#define DIALOG_SKIN     (3001)

enum E_TELEPORT
{
    tName[32],
    Float:tX,
    Float:tY,
    Float:tZ,
    Float:tAngle
};

static const gTeleports[][E_TELEPORT] =
{
    { "Grove Street (LS)",       2495.0,  -1687.0,  13.5,  0.0 },
    { "Los Santos Airport",      1685.0,  -2333.0,  13.5,  0.0 },
    { "Los Santos Beach",        375.0,   -2035.0,   7.5,  0.0 },
    { "Mount Chiliad",          -2330.0,  -1637.0, 483.0,  0.0 },
    { "San Fierro",             -1980.0,    -500.0,  35.0,  0.0 },
    { "San Fierro Airport",     -1200.0,   -200.0,  14.0,  0.0 },
    { "Las Venturas Strip",      2100.0,   1600.0,  12.0,  0.0 },
    { "Las Venturas Airport",    1680.0,   1450.0,  10.5,  0.0 },
    { "Area 51",                 220.0,    1850.0,  17.0,  0.0 },
    { "Big Ear",                -300.0,    1500.0,  75.0,  0.0 },
    { "Sherman Dam",            -800.0,    2000.0,  60.0,  0.0 },
    { "Mount Chilliad Top",    -2324.0,  -2760.0, 100.0,  0.0 }
};

enum E_SKIN
{
    sName[32],
    sID
};

static const gSkins[][E_SKIN] =
{
    { "Carl Johnson",    0 },
    { "Sweet",           270 },
    { "Big Smoke",       271 },
    { "Ryder",           272 },
    { "Kendl",           273 },
    { "Cesar",           274 },
    { "Woozie",          294 },
    { "Truth",           296 },
    { "Tenpenny",        265 },
    { "Pulaski",         267 },
    { "Cop",             280 },
    { "SWAT",            285 },
    { "Army",            287 },
    { "Medic",           274 },
    { "Prostitute",      3 },
    { "Beach girl",      93 }
};

forward OnDatabaseReady();


main()
{
    print("\n--------------------------------------");
    print("  SA-MP Freeroam 0.3.7");
    print("--------------------------------------\n");
}

public OnGameModeInit()
{
    SetGameModeText("Freeroam 1.0");
    ShowPlayerMarkers(1);
    ShowNameTags(1);
    SetNameTagDrawDistance(70.0);
    AllowInteriorWeapons(0);
    SetWeather(10);
    SetWorldTime(12);

    // Errors and warnings only, so the server log stays readable
    mysql_log(ERROR | WARNING);

    gDB = mysql_connect(DB_HOST, DB_USER, DB_PASS, DB_NAME);
    if (mysql_errno(gDB) != 0)
    {
        print("[MySQL] Connection failed. Check DB_HOST/DB_USER/DB_PASS/DB_NAME in gamemodes/freeroam.pwn");
        print("[MySQL] The server will now shut down instead of running without a database.");
        SendRconCommand("exit");
        return 1;
    }
    print("[MySQL] Connected to the database.");

    // Schema. Kept as single literals, one query per table.
    mysql_tquery(gDB, "CREATE TABLE IF NOT EXISTS accounts (id INT UNSIGNED NOT NULL AUTO_INCREMENT, username VARCHAR(24) NOT NULL, password CHAR(64) NOT NULL, admin_level TINYINT UNSIGNED NOT NULL DEFAULT 0, money INT NOT NULL DEFAULT 0, score INT NOT NULL DEFAULT 0, created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, PRIMARY KEY (id), UNIQUE KEY uq_username (username)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4", "OnDatabaseReady", "");

    mysql_tquery(gDB, "CREATE TABLE IF NOT EXISTS casino_log (id INT UNSIGNED NOT NULL AUTO_INCREMENT, account_id INT UNSIGNED NOT NULL, game VARCHAR(16) NOT NULL, amount INT NOT NULL, balance INT NOT NULL, created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, PRIMARY KEY (id), KEY idx_account (account_id)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4", "OnDatabaseReady", "");

    mysql_tquery(gDB, "CREATE TABLE IF NOT EXISTS bans (id INT UNSIGNED NOT NULL AUTO_INCREMENT, username VARCHAR(24) NOT NULL, ip VARCHAR(16) NOT NULL, reason VARCHAR(128) NOT NULL, admin VARCHAR(24) NOT NULL, created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, PRIMARY KEY (id), KEY idx_username (username), KEY idx_ip (ip)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4", "OnDatabaseReady", "");

    SetTimer("Vehicles_OnDriftTick", DRIFT_TICK_MS, true);
    SetTimer("Admin_OnAnticheatTick", AC_TICK_MS, true);

    return 1;
}

public OnGameModeExit()
{
    if (gDB != MYSQL_INVALID_HANDLE) mysql_close(gDB);
    return 1;
}

// accounts / casino_log / bans table ready (or the query failed - OnQueryError reports that)
public OnDatabaseReady()
{
    static bool:announced = false;

    if (!announced)
    {
        announced = true;
        print("[MySQL] tables ready.");
    }
    return 1;
}

// =====================================================================
//  Player callbacks
// =====================================================================

public OnPlayerConnect(playerid)
{
    Player_Reset(playerid);
    Vehicles_OnPlayerConnect(playerid);
    Casino_OnPlayerConnect(playerid);

    // nickname is needed by the ban check, so it runs after Player_Reset
    Account_OnPlayerConnect(playerid);
    Admin_OnPlayerConnect(playerid);
    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    // money/score must be written before the data is cleared
    Account_OnPlayerDisconnect(playerid);

    Vehicles_OnPlayerDisconnect(playerid);
    Casino_OnPlayerDisconnect(playerid);
    Player_Reset(playerid);
    return 1;
}

public OnPlayerSpawn(playerid)
{
    SetPlayerInterior(playerid, 0);
    Admin_IgnorePosition(playerid, 3);
    return 1;
}

// Players must not pick a class / spawn before they are logged in
public OnPlayerRequestClass(playerid, classid)
{
    if (!PlayerData[playerid][pLogged]) return 0;
    return 1;
}

public OnPlayerRequestSpawn(playerid)
{
    if (!PlayerData[playerid][pLogged]) return 0;
    return 1;
}

// Chat: muted players are silenced
public OnPlayerText(playerid, text[])
{
    if (!Player_IsLogged(playerid))
    {
        SendClientMessage(playerid, COLOR_ERROR, "You must be logged in to chat.");
        return 0;
    }
    if (PlayerData[playerid][pMuted])
    {
        SendClientMessage(playerid, COLOR_ERROR, "You are muted.");
        return 0;
    }

    new string[144];
    format(string, sizeof(string), "%s: %s", PlayerData[playerid][pName], text);
    SendClientMessageToAll(COLOR_WHITE, string);
    return 0;
}

// =====================================================================
//  Dialogs
// =====================================================================

static stock ShowTeleportMenu(playerid)
{
    new string[1024];
    new line[64];

    for (new i = 0; i < sizeof(gTeleports); i++)
    {
        format(line, sizeof(line), "%s\n", gTeleports[i][tName]);
        strcat(string, line, sizeof(string));
    }

    ShowPlayerDialog(playerid, DIALOG_TELEPORT, DIALOG_STYLE_LIST, "Teleport", string, "Go", "Cancel");
    return 1;
}

static stock ShowSkinMenu(playerid)
{
    new string[1024];
    new line[64];

    for (new i = 0; i < sizeof(gSkins); i++)
    {
        format(line, sizeof(line), "%s (skin %d)\n", gSkins[i][sName], gSkins[i][sID]);
        strcat(string, line, sizeof(string));
    }

    ShowPlayerDialog(playerid, DIALOG_SKIN, DIALOG_STYLE_LIST, "Skin", string, "Select", "Cancel");
    return 1;
}

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    // module handlers first
    if (Account_OnDialogResponse(playerid, dialogid, response, listitem, inputtext)) return 1;
    if (Vehicles_OnDialogResponse(playerid, dialogid, response, listitem, inputtext)) return 1;
    if (Casino_OnDialogResponse(playerid, dialogid, response, listitem, inputtext)) return 1;

    switch (dialogid)
    {
        case DIALOG_TELEPORT:
        {
            if (!response) return 1;
            if (!Player_IsLogged(playerid)) return 1;
            if (listitem < 0 || listitem >= sizeof(gTeleports)) return 1;

            if (IsPlayerInAnyVehicle(playerid))
            {
                SetVehiclePos(GetPlayerVehicleID(playerid),
                    gTeleports[listitem][tX], gTeleports[listitem][tY], gTeleports[listitem][tZ] + 1.0);
            }
            else
            {
                SetPlayerPos(playerid,
                    gTeleports[listitem][tX], gTeleports[listitem][tY], gTeleports[listitem][tZ]);
            }
            SetPlayerInterior(playerid, 0);
            Admin_IgnorePosition(playerid, 3);

            new string[96];
            format(string, sizeof(string), "Teleported to %s.", gTeleports[listitem][tName]);
            SendClientMessage(playerid, COLOR_OK, string);
            return 1;
        }
        case DIALOG_SKIN:
        {
            if (!response) return 1;
            if (!Player_IsLogged(playerid)) return 1;
            if (listitem < 0 || listitem >= sizeof(gSkins)) return 1;

            SetPlayerSkin(playerid, gSkins[listitem][sID]);

            new string[96];
            format(string, sizeof(string), "Skin changed to %s.", gSkins[listitem][sName]);
            SendClientMessage(playerid, COLOR_OK, string);
            return 1;
        }
    }

    return 0;
}

// =====================================================================
//  Commands
// =====================================================================

public OnPlayerCommandText(playerid, cmdtext[])
{
    new cmd[16];
    sscanf(cmdtext, "s[16]s[64]", cmd);

    // freeroam menu commands
    if (strcmp(cmd, "/tp", true) == 0)
    {
        if (!Player_IsLogged(playerid))
        {
            SendClientMessage(playerid, COLOR_ERROR, "You must be logged in.");
            return 1;
        }
        ShowTeleportMenu(playerid);
        return 1;
    }

    if (strcmp(cmd, "/skin", true) == 0)
    {
        if (!Player_IsLogged(playerid))
        {
            SendClientMessage(playerid, COLOR_ERROR, "You must be logged in.");
            return 1;
        }
        ShowSkinMenu(playerid);
        return 1;
    }

    // module commands
    if (Admin_OnCommandText(playerid, cmdtext)) return 1;
    if (Casino_OnCommandText(playerid, cmdtext)) return 1;
    if (Vehicles_OnCommandText(playerid, cmdtext)) return 1;
    if (PlayerData_OnCommandText(playerid, cmdtext)) return 1;

    return 0;
}

// Any failed query ends up here so it is visible in the server log
public OnQueryError(errorid, const error[], const callback[], const query[], MySQL:handle)
{
    printf("[MySQL] Error %d (callback: %s): %s", errorid, callback, error);
    return 1;
}
