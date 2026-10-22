/*
    vehicles.inc - vehicle spawn (/v), drift mode (/drift) and drift duels (/duel)

    Drift scoring is done server side with a 250 ms timer and drawn in a
    textdraw per player. No persistence: drift scores are session only.
*/

#if defined _INC_vehicles
    #endinput
#endif
#define _INC_vehicles

#include <a_samp>
#include <sscanf2>
#include "playerdata.inc"

#define DIALOG_DUEL (2050)

#define DRIFT_MIN_SPEED     (30.0)      // km/h before a drift starts counting
#define DRIFT_MIN_ANGLE     (0.25)      // 1.0 - cos(angle); ~41 degrees sideways
#define DRIFT_TICK_MS       (250)
#define DRIFT_DUEL_TIME     (60)        // seconds
#define DRIFT_DUEL_MIN_BET  (100)
#define DRIFT_CAR_MODEL     (560)       // Sultan

#define VEHICLE_FIRST_MODEL (400)
#define VEHICLE_LAST_MODEL  (611)

// Vehicles only admins may spawn
static const gAdminOnlyVehicles[] =
{
    425, 432, 447, 464, 465, 476, 501, 520, 577, 592, 593
};

enum E_DRIFT
{
    dActive,
    dScore,
    dBest,
    dStarted,
    dVehicle,
    PlayerText:dTextDraw,
    dOpponent,          // INVALID_PLAYER_ID when not dueling
    dBet
};

new DriftData[MAX_PLAYERS][E_DRIFT];
new gPersonalVehicle[MAX_PLAYERS];

forward Vehicles_OnDriftTick();
forward Vehicles_OnDuelEnd(playerid, opponentid);
forward Vehicles_OnDuelPickup(playerid);


// =====================================================================
//  Vehicle spawn
// =====================================================================

stock bool:Vehicles_IsModelAllowed(model, adminlevel)
{
    if (model < VEHICLE_FIRST_MODEL || model > VEHICLE_LAST_MODEL) return false;

    // trains and a few special models that must not be spawned
    switch (model)
    {
        case 449, 537, 538, 569, 570: return false;
    }

    if (adminlevel <= 0)
    {
        for (new i = 0; i < sizeof(gAdminOnlyVehicles); i++)
        {
            if (gAdminOnlyVehicles[i] == model) return false;
        }
    }
    return true;
}

stock Vehicles_SpawnPersonal(playerid, model)
{
    if (gPersonalVehicle[playerid] != INVALID_VEHICLE_ID && IsValidVehicle(gPersonalVehicle[playerid]))
    {
        DestroyVehicle(gPersonalVehicle[playerid]);
    }

    new Float:x, Float:y, Float:z, Float:angle;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, angle);

    new vehicleid = CreateVehicle(model, x + 2.0, y + 2.0, z + 0.5, angle, random(126), random(126), 60);
    if (vehicleid == INVALID_VEHICLE_ID)
    {
        SendClientMessage(playerid, COLOR_ERROR, "Could not spawn that vehicle, try another model.");
        return 0;
    }

    gPersonalVehicle[playerid] = vehicleid;
    PutPlayerInVehicle(playerid, vehicleid, 0);
    SendClientMessage(playerid, COLOR_OK, "Vehicle spawned. Use /v [model] to replace it.");
    return 1;
}


// =====================================================================
//  Drift mode
// =====================================================================

stock Drift_Stop(playerid, bool:announce = true)
{
    if (!DriftData[playerid][dActive]) return 0;

    DriftData[playerid][dActive] = 0;
    if (DriftData[playerid][dScore] > DriftData[playerid][dBest])
    {
        DriftData[playerid][dBest] = DriftData[playerid][dScore];
    }

    if (DriftData[playerid][dTextDraw] != INVALID_PLAYER_TEXT_DRAW)
    {
        PlayerTextDrawDestroy(playerid, DriftData[playerid][dTextDraw]);
        DriftData[playerid][dTextDraw] = INVALID_PLAYER_TEXT_DRAW;
    }

    if (announce)
    {
        new string[128];
        format(string, sizeof(string), "Drift session finished. Score: %d (best this session: %d)",
            DriftData[playerid][dScore], DriftData[playerid][dBest]);
        SendClientMessage(playerid, COLOR_INFO, string);
    }
    return 1;
}

stock Drift_Start(playerid, bool:duel)
{
    if (!Player_IsLogged(playerid))
    {
        SendClientMessage(playerid, COLOR_ERROR, "You must be logged in.");
        return 0;
    }

    if (DriftData[playerid][dActive]) Drift_Stop(playerid, false);

    // Make sure the player is driving something
    if (!GetPlayerVehicleID(playerid) || !IsPlayerInAnyVehicle(playerid))
    {
        new Float:x, Float:y, Float:z, Float:angle;
        GetPlayerPos(playerid, x, y, z);
        GetPlayerFacingAngle(playerid, angle);

        new vehicleid = CreateVehicle(DRIFT_CAR_MODEL, x + 2.0, y + 2.0, z + 0.5, angle, random(126), random(126), 60);
        if (vehicleid == INVALID_VEHICLE_ID)
        {
            SendClientMessage(playerid, COLOR_ERROR, "Could not spawn a drift car.");
            return 0;
        }
        PutPlayerInVehicle(playerid, vehicleid, 0);
    }

    DriftData[playerid][dScore] = 0;
    DriftData[playerid][dStarted] = gettime();
    DriftData[playerid][dVehicle] = GetPlayerVehicleID(playerid);
    DriftData[playerid][dActive] = 1;

    new PlayerText:td = CreatePlayerTextDraw(playerid, 320.0, 360.0, "DRIFT 0");
    PlayerTextDrawAlignment(playerid, td, 2);
    PlayerTextDrawLetterSize(playerid, td, 0.35, 1.6);
    PlayerTextDrawSetOutline(playerid, td, 1);
    PlayerTextDrawSetShadow(playerid, td, 0);
    PlayerTextDrawBackgroundColor(playerid, td, 255);
    PlayerTextDrawFont(playerid, td, 1);
    PlayerTextDrawSetProportional(playerid, td, 1);
    PlayerTextDrawColor(playerid, td, 0xFFFFFFFF);
    PlayerTextDrawShow(playerid, td);
    DriftData[playerid][dTextDraw] = td;

    if (!duel) SendClientMessage(playerid, COLOR_OK, "Drift started! Keep the car sideways. /stopdrift to finish.");
    return 1;
}

// Rough drift score: faster + bigger slide angle = more points
stock Drift_AddScore(playerid)
{
    new vehicleid = GetPlayerVehicleID(playerid);
    if (!vehicleid) return 0;

    new Float:vx, Float:vy, Float:vz;
    GetVehicleVelocity(vehicleid, vx, vy, vz);

    new Float:speed = floatsqroot((vx * vx) + (vy * vy) + (vz * vz)) * 180.0;
    if (speed < DRIFT_MIN_SPEED) return 0;

    new Float:zangle;
    GetVehicleZAngle(vehicleid, zangle);

    new Float:fx = -floatsin(zangle, degrees);
    new Float:fy = floatcos(zangle, degrees);

    new Float:vlen = floatsqroot((vx * vx) + (vy * vy));
    if (vlen < 0.001) return 0;

    new Float:dot = ((vx * fx) + (vy * fy)) / vlen;
    new Float:sideways = 1.0 - dot;
    if (sideways < DRIFT_MIN_ANGLE) return 0;

    DriftData[playerid][dScore] += floatround(speed * sideways * 0.5);
    return 1;
}

stock Drift_UpdateTextDraw(playerid)
{
    if (!DriftData[playerid][dActive]) return 0;

    new string[96];
    if (DriftData[playerid][dOpponent] != INVALID_PLAYER_ID)
    {
        new opponent = DriftData[playerid][dOpponent];
        format(string, sizeof(string), "DRIFT %d  |  vs %s: %d",
            DriftData[playerid][dScore], PlayerData[opponent][pName], DriftData[opponent][dScore]);
    }
    else
    {
        format(string, sizeof(string), "DRIFT %d", DriftData[playerid][dScore]);
    }
    PlayerTextDrawSetString(playerid, DriftData[playerid][dTextDraw], string);
    return 1;
}

public Vehicles_OnDriftTick()
{
    for (new playerid = 0; playerid < MAX_PLAYERS; playerid++)
    {
        if (!DriftData[playerid][dActive]) continue;
        if (!IsPlayerConnected(playerid))
        {
            DriftData[playerid][dActive] = 0;
            continue;
        }

        if (!IsPlayerInAnyVehicle(playerid))
        {
            Drift_Stop(playerid);
            continue;
        }

        Drift_AddScore(playerid);
        Drift_UpdateTextDraw(playerid);
    }
    return 1;
}


// =====================================================================
//  Drift duel
// =====================================================================

stock Drift_OnDuelAccept(playerid, opponentid)
{
    if (!IsPlayerConnected(opponentid)) return 0;

    new bet = DriftData[playerid][dBet];
    if (Player_GetMoney(playerid) < bet || Player_GetMoney(opponentid) < bet)
    {
        SendClientMessage(playerid, COLOR_ERROR, "Not enough money for this duel anymore.");
        DriftData[playerid][dOpponent] = INVALID_PLAYER_ID;
        return 0;
    }

    Player_AddMoney(playerid, -bet);
    Player_AddMoney(opponentid, -bet);

    DriftData[playerid][dOpponent] = opponentid;
    DriftData[opponentid][dOpponent] = playerid;
    DriftData[playerid][dBet] = bet;
    DriftData[opponentid][dBet] = bet;

    Drift_Start(playerid, true);
    Drift_Start(opponentid, true);

    new string[128];
    format(string, sizeof(string), "Drift duel started! $%d each, %d seconds. Good luck.", bet, DRIFT_DUEL_TIME);
    SendClientMessage(playerid, COLOR_INFO, string);
    SendClientMessage(opponentid, COLOR_INFO, string);

    SetTimerEx("Vehicles_OnDuelEnd", DRIFT_DUEL_TIME * 1000, false, "ii", playerid, opponentid);
    return 1;
}

public Vehicles_OnDuelEnd(playerid, opponentid)
{
    if (DriftData[playerid][dOpponent] != opponentid && DriftData[opponentid][dOpponent] != playerid) return 1;

    new score1 = DriftData[playerid][dScore];
    new score2 = DriftData[opponentid][dScore];
    new pot = DriftData[playerid][dBet] * 2;

    DriftData[playerid][dOpponent] = INVALID_PLAYER_ID;
    DriftData[opponentid][dOpponent] = INVALID_PLAYER_ID;

    Drift_Stop(playerid, false);
    Drift_Stop(opponentid, false);

    if (!IsPlayerConnected(playerid) || !IsPlayerConnected(opponentid))
    {
        // the connected player takes the pot
        new winner = IsPlayerConnected(playerid) ? playerid : opponentid;
        if (IsPlayerConnected(winner))
        {
            Player_AddMoney(winner, pot);
            new string[128];
            format(string, sizeof(string), "Your opponent left. You win the $%d pot.", pot);
            SendClientMessage(winner, COLOR_OK, string);
            PlayerData[winner][pScore] += 10;
            Player_SetScore(winner, PlayerData[winner][pScore]);
        }
        return 1;
    }

    new string[160];
    if (score1 == score2)
    {
        Player_AddMoney(playerid, DriftData[playerid][dBet]);
        Player_AddMoney(opponentid, DriftData[opponentid][dBet]);
        format(string, sizeof(string), "Drift duel draw (%d : %d). Bets returned.", score1, score2);
        SendClientMessage(playerid, COLOR_INFO, string);
        SendClientMessage(opponentid, COLOR_INFO, string);
        return 1;
    }

    new winner = (score1 > score2) ? playerid : opponentid;
    new loser  = (winner == playerid) ? opponentid : playerid;

    Player_AddMoney(winner, pot);
    PlayerData[winner][pScore] += 10;
    Player_SetScore(winner, PlayerData[winner][pScore]);

    format(string, sizeof(string), "You won the drift duel! %d : %d (+$%d, +10 score)",
        DriftData[winner][dScore], DriftData[loser][dScore], pot);
    SendClientMessage(winner, COLOR_OK, string);
    format(string, sizeof(string), "You lost the drift duel. %d : %d",
        DriftData[loser][dScore], DriftData[winner][dScore]);
    SendClientMessage(loser, COLOR_ERROR, string);
    return 1;
}

// Target accepted the duel dialog
public Vehicles_OnDuelPickup(playerid)
{
    new opponentid = DriftData[playerid][dOpponent];
    if (opponentid == INVALID_PLAYER_ID) return 1;
    return Drift_OnDuelAccept(playerid, opponentid);
}


// =====================================================================
//  Callbacks
// =====================================================================

stock Vehicles_OnPlayerConnect(playerid)
{
    gPersonalVehicle[playerid] = INVALID_VEHICLE_ID;
    DriftData[playerid][dActive] = 0;
    DriftData[playerid][dScore] = 0;
    DriftData[playerid][dBest] = 0;
    DriftData[playerid][dOpponent] = INVALID_PLAYER_ID;
    DriftData[playerid][dBet] = 0;
    DriftData[playerid][dTextDraw] = INVALID_PLAYER_TEXT_DRAW;
    return 1;
}

stock Vehicles_OnPlayerDisconnect(playerid)
{
    // Leaving in the middle of a duel forfeits the bet and gives the pot
    // to the player who is still connected.
    if (DriftData[playerid][dOpponent] != INVALID_PLAYER_ID)
    {
        Vehicles_OnDuelEnd(playerid, DriftData[playerid][dOpponent]);
    }

    Drift_Stop(playerid, false);
    DriftData[playerid][dOpponent] = INVALID_PLAYER_ID;

    if (gPersonalVehicle[playerid] != INVALID_VEHICLE_ID && IsValidVehicle(gPersonalVehicle[playerid]))
    {
        DestroyVehicle(gPersonalVehicle[playerid]);
    }
    gPersonalVehicle[playerid] = INVALID_VEHICLE_ID;
    return 1;
}

stock Vehicles_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[])
{
    #pragma unused listitem, inputtext

    switch (dialogid)
    {
        case DIALOG_DUEL:
        {
            new opponentid = DriftData[playerid][dOpponent];
            DriftData[playerid][dOpponent] = INVALID_PLAYER_ID;

            if (!response || opponentid == INVALID_PLAYER_ID || !IsPlayerConnected(opponentid))
            {
                SendClientMessage(playerid, COLOR_ERROR, "Duel declined.");
                return 1;
            }
            Drift_OnDuelAccept(playerid, opponentid);
            return 1;
        }
    }
    return 0;
}

stock Vehicles_OnCommandText(playerid, const cmdtext[])
{
    new cmd[16];
    new params[64];
    sscanf(cmdtext, "s[16]s[64]", cmd, params);

    if (strcmp(cmd, "/v", true) == 0)
    {
        if (!Player_IsLogged(playerid))
        {
            SendClientMessage(playerid, COLOR_ERROR, "You must be logged in.");
            return 1;
        }

        new model;
        if (sscanf(params, "d", model))
        {
            SendClientMessage(playerid, COLOR_INFO, "Usage: /v [model id]   e.g. /v 560");
            return 1;
        }

        if (!Vehicles_IsModelAllowed(model, PlayerData[playerid][pAdmin]))
        {
            SendClientMessage(playerid, COLOR_ERROR, "Invalid or restricted vehicle model (400-611).");
            return 1;
        }

        Vehicles_SpawnPersonal(playerid, model);
        return 1;
    }

    if (strcmp(cmd, "/drift", true) == 0)
    {
        if (DriftData[playerid][dActive])
        {
            SendClientMessage(playerid, COLOR_ERROR, "You are already in a drift session. Use /stopdrift.");
            return 1;
        }
        Drift_Start(playerid, false);
        return 1;
    }

    if (strcmp(cmd, "/stopdrift", true) == 0)
    {
        if (!DriftData[playerid][dActive])
        {
            SendClientMessage(playerid, COLOR_ERROR, "You are not drifting.");
            return 1;
        }
        Drift_Stop(playerid);
        return 1;
    }

    if (strcmp(cmd, "/duel", true) == 0)
    {
        if (!Player_IsLogged(playerid))
        {
            SendClientMessage(playerid, COLOR_ERROR, "You must be logged in.");
            return 1;
        }

        new targetid, bet;
        if (sscanf(params, "dd", targetid, bet))
        {
            SendClientMessage(playerid, COLOR_INFO, "Usage: /duel [playerid] [bet]");
            return 1;
        }

        if (!IsPlayerConnected(targetid) || targetid == playerid)
        {
            SendClientMessage(playerid, COLOR_ERROR, "Invalid player.");
            return 1;
        }
        if (!Player_IsLogged(targetid))
        {
            SendClientMessage(playerid, COLOR_ERROR, "That player is not logged in.");
            return 1;
        }
        if (bet < DRIFT_DUEL_MIN_BET)
        {
            new msg[96];
            format(msg, sizeof(msg), "Minimum duel bet is $%d.", DRIFT_DUEL_MIN_BET);
            SendClientMessage(playerid, COLOR_ERROR, msg);
            return 1;
        }
        if (Player_GetMoney(playerid) < bet || Player_GetMoney(targetid) < bet)
        {
            SendClientMessage(playerid, COLOR_ERROR, "Both players need at least that much money.");
            return 1;
        }
        if (DriftData[targetid][dActive] || DriftData[targetid][dOpponent] != INVALID_PLAYER_ID)
        {
            SendClientMessage(playerid, COLOR_ERROR, "That player is already drifting.");
            return 1;
        }

        DriftData[targetid][dOpponent] = playerid;
        DriftData[targetid][dBet] = bet;

        new string[192];
        format(string, sizeof(string), "%s challenges you to a drift duel for $%d.\nAccept?", PlayerData[playerid][pName], bet);
        ShowPlayerDialog(targetid, DIALOG_DUEL, DIALOG_STYLE_MSGBOX, "Drift duel", string, "Accept", "Decline");

        SendClientMessage(playerid, COLOR_INFO, "Challenge sent.");
        return 1;
    }

    return 0;
}
