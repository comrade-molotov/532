/*
    accounts.inc - registration / login (username + password only)

    - Dialog based: one dialog to login, one to register
    - Password is hashed with SHA256 before it touches the database
    - Table: accounts (id, username, password, admin_level, money, score, created_at)

    Flow:
        OnPlayerConnect  -> Account_OnPlayerConnect  -> SELECT username
                        -> DIALOG_REGISTER (no account) or DIALOG_LOGIN (account exists)
        DIALOG_LOGIN     -> Account_OnLogin      (row found = correct password)
        DIALOG_REGISTER  -> Account_OnRegister   (insert + auto login)
*/

#if defined _INC_accounts
    #endinput
#endif
#define _INC_accounts

#include <a_samp>
#include <a_mysql>
#include "playerdata.inc"

// Dialog IDs used by this file (1000-1099 = account dialogs)
#define DIALOG_LOGIN    (1000)
#define DIALOG_REGISTER (1001)

#define PASSWORD_MIN_LEN (3)
#define PASSWORD_MAX_LEN (32)

// Where a player appears after a successful login.
#define SPAWN_X     (2495.3)
#define SPAWN_Y     (-1687.1)
#define SPAWN_Z     (13.5)
#define SPAWN_SKIN  (0)


// =====================================================================
//  SHA256
//
//  Passwords are at most PASSWORD_MAX_LEN (32) bytes, so a single
//  512-bit block is always enough - no multi block loop needed.
// =====================================================================

#define SHA256_INPUT_MAX (55)       // max bytes that still fit in one block
#define SHA256_HEX_LEN   (64)       // length of the hex output

static const sSHA256K[64] =
{
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};

#define SHA256_ROTR(%1,%2) (((%1) >>> (%2)) | ((%1) << (32 - (%2))))

static stock SHA256_WriteWord(word, output[], offset)
{
    static const hexChars[] = "0123456789abcdef";

    for (new i = 0; i < 8; i++)
    {
        output[offset + i] = hexChars[(word >>> (28 - (i * 4))) & 0xF];
    }
    return 1;
}

// SHA256 of a string, written as 64 lowercase hex characters into output[65].
// Returns 0 if the input is too long for a single block.
stock SHA256(const input[], output[])
{
    new len = strlen(input);

    output[0] = EOS;
    if (len > SHA256_INPUT_MAX) return 0;

    // Build the padded 512-bit block (64 bytes)
    new block[64];
    for (new i = 0; i < 64; i++) block[i] = 0;
    for (new i = 0; i < len; i++) block[i] = input[i] & 0xFF;
    block[len] = 0x80;

    // Message length in bits, 64-bit big endian. len < 2^29, so the first 4 bytes are 0.
    new bits = len * 8;
    block[60] = (bits >>> 24) & 0xFF;
    block[61] = (bits >>> 16) & 0xFF;
    block[62] = (bits >>> 8) & 0xFF;
    block[63] = bits & 0xFF;

    // Message schedule
    new w[64];
    for (new i = 0; i < 16; i++)
    {
        w[i] = (block[i * 4] << 24) | (block[i * 4 + 1] << 16) | (block[i * 4 + 2] << 8) | block[i * 4 + 3];
    }
    for (new i = 16; i < 64; i++)
    {
        new s0 = SHA256_ROTR(w[i - 15], 7) ^ SHA256_ROTR(w[i - 15], 18) ^ (w[i - 15] >>> 3);
        new s1 = SHA256_ROTR(w[i - 2], 17) ^ SHA256_ROTR(w[i - 2], 19) ^ (w[i - 2] >>> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    // Compression
    new a = 0x6A09E667;
    new b = 0xBB67AE85;
    new c = 0x3C6EF372;
    new d = 0xA54FF53A;
    new e = 0x510E527F;
    new f = 0x9B05688C;
    new g = 0x1F83D9AB;
    new h = 0x5BE0CD19;

    for (new i = 0; i < 64; i++)
    {
        new s1 = SHA256_ROTR(e, 6) ^ SHA256_ROTR(e, 11) ^ SHA256_ROTR(e, 25);
        new ch = (e & f) ^ (~e & g);
        new t1 = h + s1 + ch + sSHA256K[i] + w[i];

        new s0 = SHA256_ROTR(a, 2) ^ SHA256_ROTR(a, 13) ^ SHA256_ROTR(a, 22);
        new maj = (a & b) ^ (a & c) ^ (b & c);
        new t2 = s0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    new words[8];
    words[0] = a + 0x6A09E667;
    words[1] = b + 0xBB67AE85;
    words[2] = c + 0x3C6EF372;
    words[3] = d + 0xA54FF53A;
    words[4] = e + 0x510E527F;
    words[5] = f + 0x9B05688C;
    words[6] = g + 0x1F83D9AB;
    words[7] = h + 0x5BE0CD19;

    for (new i = 0; i < 8; i++)
    {
        SHA256_WriteWord(words[i], output, i * 8);
    }

    output[SHA256_HEX_LEN] = EOS;
    return 1;
}


// =====================================================================
//  Account flow
// =====================================================================

static stock Account_ShowLogin(playerid)
{
    new string[128];
    format(string, sizeof(string), "Welcome back, %s.\nEnter your password:", PlayerData[playerid][pName]);
    ShowPlayerDialog(playerid, DIALOG_LOGIN, DIALOG_STYLE_PASSWORD, "Login", string, "Login", "Leave");
    return 1;
}

static stock Account_ShowRegister(playerid)
{
    new string[160];
    format(string, sizeof(string),
        "Welcome, %s.\nChoose a password (%d-%d characters):",
        PlayerData[playerid][pName], PASSWORD_MIN_LEN, PASSWORD_MAX_LEN);
    ShowPlayerDialog(playerid, DIALOG_REGISTER, DIALOG_STYLE_PASSWORD, "Register", string, "Register", "Leave");
    return 1;
}

// Called from OnPlayerConnect
stock Account_OnPlayerConnect(playerid)
{
    GetPlayerName(playerid, PlayerData[playerid][pName], MAX_PLAYER_NAME);

    // Freeze the camera and keep the player out of the world until logged in
    TogglePlayerSpectating(playerid, true);

    new query[160];
    mysql_format(gDB, query, sizeof(query),
        "SELECT id FROM accounts WHERE username = '%e' LIMIT 1",
        PlayerData[playerid][pName]);

    mysql_tquery(gDB, query, "Account_OnCheck", "i", playerid);
    return 1;
}

// Called from OnPlayerDisconnect
stock Account_OnPlayerDisconnect(playerid)
{
    Player_SaveData(playerid);
    return 1;
}

// Dialog router, returns 1 if the dialog belonged to this file
stock Account_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[])
{
    #pragma unused listitem

    switch (dialogid)
    {
        case DIALOG_LOGIN:
        {
            if (!response)
            {
                Kick(playerid);
                return 1;
            }

            if (PlayerData[playerid][pLogged]) return 1;

            if (inputtext[0] == EOS)
            {
                SendClientMessage(playerid, COLOR_ERROR, "Password cannot be empty.");
                Account_ShowLogin(playerid);
                return 1;
            }

            new hash[SHA256_HEX_LEN + 1];
            SHA256(inputtext, hash);

            new query[256];
            mysql_format(gDB, query, sizeof(query),
                "SELECT id, admin_level, money, score FROM accounts WHERE username = '%e' AND password = '%s' LIMIT 1",
                PlayerData[playerid][pName], hash);

            mysql_tquery(gDB, query, "Account_OnLogin", "i", playerid);
            return 1;
        }
        case DIALOG_REGISTER:
        {
            if (!response)
            {
                Kick(playerid);
                return 1;
            }

            if (PlayerData[playerid][pLogged]) return 1;

            new len = strlen(inputtext);
            if (len < PASSWORD_MIN_LEN || len > PASSWORD_MAX_LEN)
            {
                new string[96];
                format(string, sizeof(string), "Password must be %d-%d characters.", PASSWORD_MIN_LEN, PASSWORD_MAX_LEN);
                SendClientMessage(playerid, COLOR_ERROR, string);
                Account_ShowRegister(playerid);
                return 1;
            }

            new hash[SHA256_HEX_LEN + 1];
            SHA256(inputtext, hash);

            new query[256];
            mysql_format(gDB, query, sizeof(query),
                "INSERT INTO accounts (username, password, admin_level, money, score) VALUES ('%e', '%s', 0, %d, %d)",
                PlayerData[playerid][pName], hash, START_MONEY, START_SCORE);

            mysql_tquery(gDB, query, "Account_OnRegister", "i", playerid);
            return 1;
        }
    }
    return 0;
}

static stock Account_LoginSuccess(playerid)
{
    PlayerData[playerid][pLogged] = 1;

    TogglePlayerSpectating(playerid, false);

    SetSpawnInfo(playerid, 0, SPAWN_SKIN, SPAWN_X, SPAWN_Y, SPAWN_Z, 0.0, 0, 0, 0, 0, 0, 0);
    SpawnPlayer(playerid);

    Player_SetMoney(playerid, PlayerData[playerid][pMoney]);
    Player_SetScore(playerid, PlayerData[playerid][pScore]);

    new string[160];
    format(string, sizeof(string), "Welcome, %s. Money: $%d | Score: %d",
        PlayerData[playerid][pName], PlayerData[playerid][pMoney], PlayerData[playerid][pScore]);
    SendClientMessage(playerid, COLOR_OK, string);

    if (PlayerData[playerid][pAdmin] > 0)
    {
        format(string, sizeof(string), "Admin level: %d", PlayerData[playerid][pAdmin]);
        SendClientMessage(playerid, COLOR_INFO, string);
    }
    return 1;
}


// =====================================================================
//  MySQL query callbacks
// =====================================================================

forward Account_OnCheck(playerid);
forward Account_OnLogin(playerid);
forward Account_OnRegister(playerid);

// Did we find the account name?
public Account_OnCheck(playerid)
{
    if (!IsPlayerConnected(playerid)) return 1;
    if (PlayerData[playerid][pLogged]) return 1;

    if (cache_num_rows() == 0)
    {
        Account_ShowRegister(playerid);
    }
    else
    {
        Account_ShowLogin(playerid);
    }
    return 1;
}

// Login query: one row means the password hash matched
public Account_OnLogin(playerid)
{
    if (!IsPlayerConnected(playerid)) return 1;
    if (PlayerData[playerid][pLogged]) return 1;

    if (cache_num_rows() == 0)
    {
        SendClientMessage(playerid, COLOR_ERROR, "Wrong password.");
        Account_ShowLogin(playerid);
        return 1;
    }

    cache_get_value_name_int(0, "id", PlayerData[playerid][pID]);
    cache_get_value_name_int(0, "admin_level", PlayerData[playerid][pAdmin]);
    cache_get_value_name_int(0, "money", PlayerData[playerid][pMoney]);
    cache_get_value_name_int(0, "score", PlayerData[playerid][pScore]);

    Account_LoginSuccess(playerid);
    return 1;
}

// Register query: account created, log the player in immediately
public Account_OnRegister(playerid)
{
    if (!IsPlayerConnected(playerid)) return 1;
    if (PlayerData[playerid][pLogged]) return 1;

    PlayerData[playerid][pID] = cache_insert_id();
    if (PlayerData[playerid][pID] <= 0)
    {
        SendClientMessage(playerid, COLOR_ERROR, "Registration failed. Please try again.");
        Account_ShowRegister(playerid);
        return 1;
    }

    PlayerData[playerid][pAdmin] = 0;
    PlayerData[playerid][pMoney] = START_MONEY;
    PlayerData[playerid][pScore] = START_SCORE;

    SendClientMessage(playerid, COLOR_OK, "Account created. You are now logged in.");
    Account_LoginSuccess(playerid);
    return 1;
}
