/*
    freeroam.pwn - Freeroam gamemode (GL style, SA-MP 0.3.7)

    Stage 1: database schema + account system
        - MySQL connection + accounts table
        - register / login dialogs (username + password, SHA256)
        - money / score / admin level loaded on login, saved on disconnect

    Nothing else is in here yet on purpose. Vehicles, teleports, casino and
    admin commands get added one stage at a time, only after the previous
    stage is confirmed working.
*/

#include <a_samp>
#include <a_mysql>

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


forward OnDatabaseReady();

main()
{
    print("\n--------------------------------------");
    print("  SA-MP Freeroam 0.3.7 - stage 1");
    print("--------------------------------------\n");
}

public OnGameModeInit()
{
    SetGameModeText("Freeroam 0.1");

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

    // Table creation is kept in one literal on purpose (no line continuation tricks)
    mysql_tquery(gDB, "CREATE TABLE IF NOT EXISTS accounts (id INT UNSIGNED NOT NULL AUTO_INCREMENT, username VARCHAR(24) NOT NULL, password CHAR(64) NOT NULL, admin_level TINYINT UNSIGNED NOT NULL DEFAULT 0, money INT NOT NULL DEFAULT 0, score INT NOT NULL DEFAULT 0, created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, PRIMARY KEY (id), UNIQUE KEY uq_username (username)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4", "OnDatabaseReady", "");

    return 1;
}

public OnGameModeExit()
{
    if (gDB != MYSQL_INVALID_HANDLE) mysql_close(gDB);
    return 1;
}

// Accounts table is ready (or the query failed - OnQueryError reports that)
public OnDatabaseReady()
{
    print("[MySQL] accounts table ready.");
    return 1;
}

public OnPlayerConnect(playerid)
{
    Player_Reset(playerid);
    Account_OnPlayerConnect(playerid);
    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    Account_OnPlayerDisconnect(playerid);
    Player_Reset(playerid);
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

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    if (Account_OnDialogResponse(playerid, dialogid, response, listitem, inputtext)) return 1;

    return 0;
}

// Any failed query ends up here so it is visible in the server log
public OnQueryError(errorid, const error[], const callback[], const query[], MySQL:handle)
{
    printf("[MySQL] Error %d (callback: %s): %s", errorid, callback, error);
    return 1;
}
