/*
    casino.inc - all four casino games in one file

        Blackjack   (/blackjack or /casino -> Blackjack)
        Poker       (5 card draw vs the house, Jacks or Better paytable)
        Roulette    (red / black / even / odd / zero)
        Slots       (3 reels)

    Every game:
        - server side RNG (random())
        - min/max bet
        - money checked before every bet
        - every win/loss written to casino_log
*/

#if defined _INC_casino
    #endinput
#endif
#define _INC_casino

#include <a_samp>
#include <a_mysql>
#include <sscanf2>
#include "playerdata.inc"

#define CASINO_MIN_BET (100)
#define CASINO_MAX_BET (100000)

// Dialog ids (2000-2099 = casino)
#define DIALOG_CASINO          (2000)
#define DIALOG_BJ_BET          (2001)
#define DIALOG_BJ_ACTION       (2002)

#define DIALOG_ROULETTE_BET    (2010)
#define DIALOG_ROULETTE_AMOUNT (2011)

#define DIALOG_SLOTS_BET       (2020)

#define DIALOG_POKER_BET       (2030)
#define DIALOG_POKER_HOLD      (2031)
#define DIALOG_POKER_AGAIN     (2032)

// Blackjack
#define BJ_MAX_CARDS (11)
#define BJ_DEALER_STAND (17)

// Poker hand ranks
#define POKER_NOTHING       (0)
#define POKER_JACKS         (1)
#define POKER_TWO_PAIR      (2)
#define POKER_TRIPS         (3)
#define POKER_STRAIGHT      (4)
#define POKER_FLUSH         (5)
#define POKER_FULL_HOUSE    (6)
#define POKER_QUADS         (7)
#define POKER_STRAIGHT_FLUSH (8)

// Roulette bet types
#define ROULETTE_RED    (0)
#define ROULETTE_BLACK  (1)
#define ROULETTE_EVEN   (2)
#define ROULETTE_ODD    (3)
#define ROULETTE_ZERO   (4)

enum E_CASINO
{
    cBJActive,
    cBJBet,
    cBJPlayerCards[BJ_MAX_CARDS],
    cBJPlayerCount,
    cBJDealerCards[BJ_MAX_CARDS],
    cBJDealerCount,

    cRouletteType,
    cRouletteAmount,

    cPokerBet,
    cPokerHeld[5],
    cPokerCards[5]
};

new CasinoData[MAX_PLAYERS][E_CASINO];
new const gSlotsNames[][] = { "Cherry", "Lemon", "Bell", "Bar", "Seven" };


// =====================================================================
//  Shared helpers
// =====================================================================

// 0..51 card. value = card % 13 (0 = ace, 12 = king), suit = card / 13
stock Casino_NewCard()
{
    return random(52);
}

stock Casino_CardValue(card)
{
    new value = (card % 13) + 1;        // 1..13
    if (value > 10) value = 10;         // J/Q/K count as 10
    return value;
}

// Raw value 1..13 (ace low), used by the straight check
stock Casino_RawValue(card)
{
    return (card % 13) + 1;
}

stock Casino_IsAce(card)
{
    return ((card % 13) == 0);
}

stock Casino_CardName(card, output[], len)
{
    static const values[] = { 'A','2','3','4','5','6','7','8','9','1','J','Q','K' };
    static const suits[] = { 'S','H','D','C' };

    new value = card % 13;
    new suit = card / 13;

    if (value == 9)                     // "10"
    {
        format(output, len, "10%c", suits[suit]);
    }
    else
    {
        format(output, len, "%c%c", values[value], suits[suit]);
    }
    return 1;
}

stock Casino_Log(playerid, const game[], amount)
{
    if (PlayerData[playerid][pID] <= 0) return 0;

    new query[192];
    mysql_format(gDB, query, sizeof(query),
        "INSERT INTO casino_log (account_id, game, amount, balance) VALUES (%d, '%e', %d, %d)",
        PlayerData[playerid][pID], game, amount, Player_GetMoney(playerid));
    mysql_tquery(gDB, query);
    return 1;
}

// true when the bet is inside the limits and the player has the money
stock Casino_CanBet(playerid, bet)
{
    if (bet < CASINO_MIN_BET)
    {
        new string[96];
        format(string, sizeof(string), "Minimum bet is $%d.", CASINO_MIN_BET);
        SendClientMessage(playerid, COLOR_ERROR, string);
        return 0;
    }
    if (bet > CASINO_MAX_BET)
    {
        new string[96];
        format(string, sizeof(string), "Maximum bet is $%d.", CASINO_MAX_BET);
        SendClientMessage(playerid, COLOR_ERROR, string);
        return 0;
    }
    if (Player_GetMoney(playerid) < bet)
    {
        SendClientMessage(playerid, COLOR_ERROR, "You do not have that much money.");
        return 0;
    }
    return 1;
}

stock Casino_ShowMenu(playerid)
{
    ShowPlayerDialog(playerid, DIALOG_CASINO, DIALOG_STYLE_LIST, "Casino",
        "Blackjack\nPoker\nRoulette\nSlots", "Play", "Close");
    return 1;
}


// =====================================================================
//  Blackjack
// =====================================================================

stock BJ_HandValue(const cards[], count)
{
    new total = 0;
    new aces = 0;

    for (new i = 0; i < count; i++)
    {
        total += Casino_CardValue(cards[i]);
        if (Casino_IsAce(cards[i])) aces++;
    }

    // one ace may count as 11 instead of 1
    if (aces > 0 && total + 10 <= 21) total += 10;
    return total;
}

stock BJ_HandString(const cards[], count, output[], len)
{
    new card[8];
    output[0] = EOS;

    for (new i = 0; i < count; i++)
    {
        Casino_CardName(cards[i], card, sizeof(card));
        if (i > 0) strcat(output, " ", len);
        strcat(output, card, len);
    }
    return 1;
}

static stock BJ_ShowAction(playerid)
{
    new pCards[64];
    new dCards[8];
    new header[160];
    new string[256];

    BJ_HandString(CasinoData[playerid][cBJPlayerCards], CasinoData[playerid][cBJPlayerCount], pCards, sizeof(pCards));
    Casino_CardName(CasinoData[playerid][cBJDealerCards][0], dCards, sizeof(dCards));

    format(header, sizeof(header), "Your hand: %s (%d)  |  Dealer shows: %s  |  Bet: $%d",
        pCards,
        BJ_HandValue(CasinoData[playerid][cBJPlayerCards], CasinoData[playerid][cBJPlayerCount]),
        dCards,
        CasinoData[playerid][cBJBet]);

    format(string, sizeof(string), "%s\nHit\nStand\nDouble", header);
    ShowPlayerDialog(playerid, DIALOG_BJ_ACTION, DIALOG_STYLE_LIST, "Blackjack", string, "Choose", "");
    return 1;
}

static stock BJ_Finish(playerid, bool:dealerplays)
{
    new string[512];
    new pCards[64];
    new dCards[64];

    new pCount = CasinoData[playerid][cBJPlayerCount];
    new dCount = CasinoData[playerid][cBJDealerCount];

    if (dealerplays)
    {
        while (BJ_HandValue(CasinoData[playerid][cBJDealerCards], dCount) < BJ_DEALER_STAND && dCount < BJ_MAX_CARDS)
        {
            CasinoData[playerid][cBJDealerCards][dCount] = Casino_NewCard();
            dCount++;
        }
        CasinoData[playerid][cBJDealerCount] = dCount;
    }

    new pValue = BJ_HandValue(CasinoData[playerid][cBJPlayerCards], pCount);
    new dValue = BJ_HandValue(CasinoData[playerid][cBJDealerCards], dCount);
    new bet = CasinoData[playerid][cBJBet];
    new payout = 0;
    new result[48];

    if (pValue > 21)
    {
        payout = 0;
        result = "Bust - you lose";
    }
    else if (dValue > 21)
    {
        payout = bet * 2;
        result = "Dealer bust - you win";
    }
    else if (pValue > dValue)
    {
        payout = bet * 2;
        result = "You win";
    }
    else if (pValue == dValue)
    {
        payout = bet;
        result = "Push - bet returned";
    }
    else
    {
        payout = 0;
        result = "Dealer wins";
    }

    // natural blackjack pays 3:2 (only when the player was not bust)
    if (pValue == 21 && pCount == 2 && dValue != 21 && payout == bet * 2)
    {
        payout = bet + (bet * 3 / 2);
        result = "Blackjack! 3:2";
    }

    if (payout > 0) Player_AddMoney(playerid, payout);
    Casino_Log(playerid, "blackjack", payout - bet);

    BJ_HandString(CasinoData[playerid][cBJPlayerCards], pCount, pCards, sizeof(pCards));
    BJ_HandString(CasinoData[playerid][cBJDealerCards], dCount, dCards, sizeof(dCards));

    format(string, sizeof(string),
        "Your hand: %s (%d)\nDealer hand: %s (%d)\n\n%s\nBet: $%d  |  %s",
        pCards, pValue, dCards, dValue, result, bet,
        (payout > bet) ? "You won" : ((payout == bet) ? "Money back" : "You lost"));

    if (payout > bet)
    {
        new msg[160];
        format(msg, sizeof(msg), "Blackjack: %s (+$%d)", result, payout - bet);
        SendClientMessage(playerid, COLOR_OK, msg);
    }
    else if (payout == bet)
    {
        SendClientMessage(playerid, COLOR_INFO, "Blackjack: push, your bet is returned.");
    }
    else
    {
        new msg[160];
        format(msg, sizeof(msg), "Blackjack: %s (-$%d)", result, bet);
        SendClientMessage(playerid, COLOR_ERROR, msg);
    }

    CasinoData[playerid][cBJActive] = 0;
    CasinoData[playerid][cBJBet] = 0;

    strcat(string, "\n\nPlay again? Bet $100 - $100000", sizeof(string));
    ShowPlayerDialog(playerid, DIALOG_BJ_BET, DIALOG_STYLE_INPUT, "Blackjack - result", string, "Deal", "Cancel");
    return 1;
}

stock BJ_Start(playerid, bet)
{
    new string[128];

    Player_AddMoney(playerid, -bet);

    CasinoData[playerid][cBJActive] = 1;
    CasinoData[playerid][cBJBet] = bet;
    CasinoData[playerid][cBJPlayerCount] = 0;
    CasinoData[playerid][cBJDealerCount] = 0;

    CasinoData[playerid][cBJPlayerCards][0] = Casino_NewCard();
    CasinoData[playerid][cBJDealerCards][0] = Casino_NewCard();
    CasinoData[playerid][cBJPlayerCards][1] = Casino_NewCard();
    CasinoData[playerid][cBJDealerCards][1] = Casino_NewCard();

    CasinoData[playerid][cBJPlayerCount] = 2;
    CasinoData[playerid][cBJDealerCount] = 2;

    new pCards[64];
    BJ_HandString(CasinoData[playerid][cBJPlayerCards], 2, pCards, sizeof(pCards));

    format(string, sizeof(string), "Bet $%d accepted. Your cards: %s (%d)",
        bet, pCards, BJ_HandValue(CasinoData[playerid][cBJPlayerCards], 2));
    SendClientMessage(playerid, COLOR_INFO, string);

    if (BJ_HandValue(CasinoData[playerid][cBJPlayerCards], 2) == 21)
    {
        BJ_Finish(playerid, true);
        return 1;
    }

    BJ_ShowAction(playerid);
    return 1;
}


// =====================================================================
//  Roulette
// =====================================================================

static stock bool:Roulette_IsRed(number)
{
    switch (number)
    {
        case 1, 3, 5, 7, 9, 12, 14, 16, 18, 19, 21, 23, 25, 27, 30, 32, 34, 36: return true;
    }
    return false;
}

stock Roulette_Play(playerid, type, bet)
{
    new number = random(37);            // 0..36
    new multiplier = 0;
    new string[192];

    switch (type)
    {
        case ROULETTE_ZERO:
        {
            if (number == 0) multiplier = 14;
        }
        case ROULETTE_RED:
        {
            if (number != 0 && Roulette_IsRed(number)) multiplier = 2;
        }
        case ROULETTE_BLACK:
        {
            if (number != 0 && !Roulette_IsRed(number)) multiplier = 2;
        }
        case ROULETTE_EVEN:
        {
            if (number != 0 && (number % 2) == 0) multiplier = 2;
        }
        case ROULETTE_ODD:
        {
            if (number != 0 && (number % 2) == 1) multiplier = 2;
        }
    }

    new payout = bet * multiplier;
    if (payout > 0) Player_AddMoney(playerid, payout);
    Casino_Log(playerid, "roulette", payout - bet);

    format(string, sizeof(string), "The ball lands on %d (%s). You %s $%d.",
        number,
        (number == 0) ? "green" : (Roulette_IsRed(number) ? "red" : "black"),
        (payout > bet) ? "win" : "lose",
        (payout > bet) ? (payout - bet) : bet);

    SendClientMessage(playerid, (payout > bet) ? COLOR_OK : COLOR_ERROR, string);
    SendClientMessage(playerid, COLOR_INFO, "Type /casino to play again.");
    return 1;
}


// =====================================================================
//  Slots
// =====================================================================

stock Slots_Play(playerid, bet)
{
    new r1 = random(5);
    new r2 = random(5);
    new r3 = random(5);
    new multiplier = 0;
    new string[192];

    if (r1 == r2 && r2 == r3)
    {
        multiplier = (r1 == 4) ? 50 : 10;
    }
    else if (r1 == r2 || r2 == r3 || r1 == r3)
    {
        multiplier = 2;
    }

    new payout = bet * multiplier;
    if (payout > 0) Player_AddMoney(playerid, payout);
    Casino_Log(playerid, "slots", payout - bet);

    format(string, sizeof(string), "[ %s | %s | %s ]",
        gSlotsNames[r1], gSlotsNames[r2], gSlotsNames[r3]);
    SendClientMessage(playerid, COLOR_WHITE, string);

    if (payout > bet)
    {
        format(string, sizeof(string), "You win $%d!", payout - bet);
        SendClientMessage(playerid, COLOR_OK, string);
    }
    else
    {
        format(string, sizeof(string), "You lose $%d.", bet);
        SendClientMessage(playerid, COLOR_ERROR, string);
    }
    SendClientMessage(playerid, COLOR_INFO, "Type /casino to play again.");
    return 1;
}


// =====================================================================
//  Poker (5 card draw vs the house - not against other players)
// =====================================================================

stock Poker_Evaluate(const cards[])
{
    new counts[14];
    new suits[4];

    for (new i = 0; i < 5; i++)
    {
        new value = Casino_RawValue(cards[i]);      // 1..13
        new suit = cards[i] / 13;
        counts[value]++;
        suits[suit]++;
    }

    new pairs = 0;
    new trips = 0;
    new quads = 0;
    new highPairValue = 0;

    for (new v = 1; v <= 13; v++)
    {
        if (counts[v] == 4) quads++;
        else if (counts[v] == 3) trips++;
        else if (counts[v] == 2)
        {
            pairs++;
            if (v > highPairValue) highPairValue = v;
        }
    }

    new bool:flush = false;
    for (new s = 0; s < 4; s++)
    {
        if (suits[s] == 5) flush = true;
    }

    new bool:straight = false;
    new consecutive = 0;
    for (new v = 1; v <= 13; v++)
    {
        if (counts[v] > 0)
        {
            consecutive++;
            if (consecutive == 5) straight = true;
        }
        else
        {
            consecutive = 0;
        }
    }
    // A-2-3-4-5 (ace low) is covered above; A-10-J-Q-K:
    if (counts[1] > 0 && counts[10] > 0 && counts[11] > 0 && counts[12] > 0 && counts[13] > 0) straight = true;

    if (straight && flush) return POKER_STRAIGHT_FLUSH;
    if (quads) return POKER_QUADS;
    if (trips && pairs) return POKER_FULL_HOUSE;
    if (flush) return POKER_FLUSH;
    if (straight) return POKER_STRAIGHT;
    if (trips) return POKER_TRIPS;
    if (pairs >= 2) return POKER_TWO_PAIR;
    if (pairs == 1 && highPairValue >= 11) return POKER_JACKS;   // jacks or better
    return POKER_NOTHING;
}

static stock Poker_Multiplier(rank)
{
    switch (rank)
    {
        case POKER_JACKS: return 1;
        case POKER_TWO_PAIR: return 2;
        case POKER_TRIPS: return 3;
        case POKER_STRAIGHT: return 4;
        case POKER_FLUSH: return 6;
        case POKER_FULL_HOUSE: return 9;
        case POKER_QUADS: return 25;
        case POKER_STRAIGHT_FLUSH: return 50;
    }
    return 0;
}

static stock Poker_RankName(rank, output[], len)
{
    switch (rank)
    {
        case POKER_JACKS: format(output, len, "Jacks or better");
        case POKER_TWO_PAIR: format(output, len, "Two pair");
        case POKER_TRIPS: format(output, len, "Three of a kind");
        case POKER_STRAIGHT: format(output, len, "Straight");
        case POKER_FLUSH: format(output, len, "Flush");
        case POKER_FULL_HOUSE: format(output, len, "Full house");
        case POKER_QUADS: format(output, len, "Four of a kind");
        case POKER_STRAIGHT_FLUSH: format(output, len, "Straight flush");
        default: format(output, len, "Nothing");
    }
    return 1;
}

static stock Poker_ShowHold(playerid)
{
    new string[256];
    new line[64];
    new card[8];

    format(string, sizeof(string), "Bet: $%d\n\n", CasinoData[playerid][cPokerBet]);

    for (new i = 0; i < 5; i++)
    {
        Casino_CardName(CasinoData[playerid][cPokerCards][i], card, sizeof(card));
        format(line, sizeof(line), "Card %d: %s  %s\n", i + 1, card,
            CasinoData[playerid][cPokerHeld][i] ? "[HOLD]" : "[discard]");
        strcat(string, line, sizeof(string));
    }
    strcat(string, "\nDRAW", sizeof(string));

    ShowPlayerDialog(playerid, DIALOG_POKER_HOLD, DIALOG_STYLE_LIST, "Poker - hold and draw",
        string, "Select", "");
    return 1;
}

stock Poker_Start(playerid, bet)
{
    new i;

    Player_AddMoney(playerid, -bet);
    CasinoData[playerid][cPokerBet] = bet;

    for (i = 0; i < 5; i++)
    {
        CasinoData[playerid][cPokerCards][i] = Casino_NewCard();
        CasinoData[playerid][cPokerHeld][i] = 0;
    }

    new string[128];
    format(string, sizeof(string), "Bet $%d accepted. Select the cards you want to keep, then DRAW.", bet);
    SendClientMessage(playerid, COLOR_INFO, string);

    Poker_ShowHold(playerid);
    return 1;
}

stock Poker_Draw(playerid)
{
    new card[8];
    new string[512];
    new line[64];

    for (new i = 0; i < 5; i++)
    {
        if (!CasinoData[playerid][cPokerHeld][i])
        {
            CasinoData[playerid][cPokerCards][i] = Casino_NewCard();
        }
    }

    new rank = Poker_Evaluate(CasinoData[playerid][cPokerCards]);
    new multiplier = Poker_Multiplier(rank);
    new bet = CasinoData[playerid][cPokerBet];
    new payout = bet * (multiplier + 1);

    if (multiplier > 0) Player_AddMoney(playerid, payout);
    Casino_Log(playerid, "poker", (multiplier > 0) ? (payout - bet) : -bet);

    format(string, sizeof(string), "Your hand:\n");
    for (new i = 0; i < 5; i++)
    {
        Casino_CardName(CasinoData[playerid][cPokerCards][i], card, sizeof(card));
        format(line, sizeof(line), "%s ", card);
        strcat(string, line, sizeof(string));
    }

    new rankname[32];
    Poker_RankName(rank, rankname, sizeof(rankname));

    if (multiplier > 0)
    {
        format(line, sizeof(line), "\n\n%s - you win $%d", rankname, payout - bet);
        strcat(string, line, sizeof(string));
        ShowPlayerDialog(playerid, DIALOG_POKER_AGAIN, DIALOG_STYLE_MSGBOX, "Poker - you won",
            string, "Play again", "Close");
        SendClientMessage(playerid, COLOR_OK, "Poker: you won.");
    }
    else
    {
        format(line, sizeof(line), "\n\n%s - you lose $%d", rankname, bet);
        strcat(string, line, sizeof(string));
        ShowPlayerDialog(playerid, DIALOG_POKER_AGAIN, DIALOG_STYLE_MSGBOX, "Poker - no hand",
            string, "Play again", "Close");
        SendClientMessage(playerid, COLOR_ERROR, "Poker: no winning hand.");
    }

    CasinoData[playerid][cPokerBet] = 0;
    return 1;
}


// =====================================================================
//  Callbacks
// =====================================================================

stock Casino_OnPlayerConnect(playerid)
{
    CasinoData[playerid][cBJActive] = 0;
    CasinoData[playerid][cBJBet] = 0;
    CasinoData[playerid][cPokerBet] = 0;
    CasinoData[playerid][cRouletteType] = ROULETTE_RED;
    CasinoData[playerid][cRouletteAmount] = 0;
    return 1;
}

stock Casino_OnPlayerDisconnect(playerid)
{
    // A blackjack bet that is still on the table is lost, like in a real casino.
    CasinoData[playerid][cBJActive] = 0;
    CasinoData[playerid][cBJBet] = 0;
    CasinoData[playerid][cPokerBet] = 0;
    return 1;
}

stock Casino_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[])
{
    switch (dialogid)
    {
        case DIALOG_CASINO:
        {
            if (!response) return 1;

            switch (listitem)
            {
                case 0:
                {
                    ShowPlayerDialog(playerid, DIALOG_BJ_BET, DIALOG_STYLE_INPUT, "Blackjack",
                        "Your bet? (minimum $100, maximum $100000)", "Deal", "Cancel");
                }
                case 1:
                {
                    ShowPlayerDialog(playerid, DIALOG_POKER_BET, DIALOG_STYLE_INPUT, "Poker",
                        "Your bet? (minimum $100, maximum $100000)", "Deal", "Cancel");
                }
                case 2:
                {
                    ShowPlayerDialog(playerid, DIALOG_ROULETTE_BET, DIALOG_STYLE_LIST, "Roulette",
                        "Red (x2)\nBlack (x2)\nEven (x2)\nOdd (x2)\nZero (x14)", "Bet", "Cancel");
                }
                case 3:
                {
                    ShowPlayerDialog(playerid, DIALOG_SLOTS_BET, DIALOG_STYLE_INPUT, "Slots",
                        "Your bet? (minimum $100, maximum $100000)", "Spin", "Cancel");
                }
            }
            return 1;
        }

        // ---------------- blackjack ----------------
        case DIALOG_BJ_BET:
        {
            if (!response) return 1;
            if (CasinoData[playerid][cBJActive])
            {
                SendClientMessage(playerid, COLOR_ERROR, "Finish the current hand first.");
                return 1;
            }

            new bet;
            if (sscanf(inputtext, "d", bet))
            {
                SendClientMessage(playerid, COLOR_ERROR, "Enter a number.");
                return 1;
            }
            if (!Casino_CanBet(playerid, bet)) return 1;

            BJ_Start(playerid, bet);
            return 1;
        }
        case DIALOG_BJ_ACTION:
        {
            if (!CasinoData[playerid][cBJActive]) return 1;
            if (!response) return 1;

            switch (listitem)
            {
                case 1:     // hit
                {
                    if (CasinoData[playerid][cBJPlayerCount] >= BJ_MAX_CARDS) return 1;

                    CasinoData[playerid][cBJPlayerCards][CasinoData[playerid][cBJPlayerCount]] = Casino_NewCard();
                    CasinoData[playerid][cBJPlayerCount]++;

                    new value = BJ_HandValue(CasinoData[playerid][cBJPlayerCards], CasinoData[playerid][cBJPlayerCount]);
                    if (value > 21)
                    {
                        BJ_Finish(playerid, false);
                    }
                    else if (value == 21)
                    {
                        BJ_Finish(playerid, true);
                    }
                    else
                    {
                        BJ_ShowAction(playerid);
                    }
                }
                case 2:     // stand
                {
                    BJ_Finish(playerid, true);
                }
                case 3:     // double
                {
                    new bet = CasinoData[playerid][cBJBet];
                    if (CasinoData[playerid][cBJPlayerCount] != 2)
                    {
                        SendClientMessage(playerid, COLOR_ERROR, "You can only double on the first two cards.");
                        BJ_ShowAction(playerid);
                        return 1;
                    }
                    if (Player_GetMoney(playerid) < bet)
                    {
                        SendClientMessage(playerid, COLOR_ERROR, "You do not have enough money to double.");
                        BJ_ShowAction(playerid);
                        return 1;
                    }

                    Player_AddMoney(playerid, -bet);
                    CasinoData[playerid][cBJBet] = bet * 2;

                    CasinoData[playerid][cBJPlayerCards][CasinoData[playerid][cBJPlayerCount]] = Casino_NewCard();
                    CasinoData[playerid][cBJPlayerCount]++;

                    if (BJ_HandValue(CasinoData[playerid][cBJPlayerCards], CasinoData[playerid][cBJPlayerCount]) > 21)
                    {
                        BJ_Finish(playerid, false);
                    }
                    else
                    {
                        BJ_Finish(playerid, true);
                    }
                }
            }
            return 1;
        }

        // ---------------- roulette ----------------
        case DIALOG_ROULETTE_BET:
        {
            if (!response) return 1;

            CasinoData[playerid][cRouletteType] = listitem;
            ShowPlayerDialog(playerid, DIALOG_ROULETTE_AMOUNT, DIALOG_STYLE_INPUT, "Roulette",
                "How much do you bet? (minimum $100, maximum $100000)", "Bet", "Cancel");
            return 1;
        }
        case DIALOG_ROULETTE_AMOUNT:
        {
            if (!response) return 1;

            new bet;
            if (sscanf(inputtext, "d", bet))
            {
                SendClientMessage(playerid, COLOR_ERROR, "Enter a number.");
                return 1;
            }
            if (!Casino_CanBet(playerid, bet)) return 1;

            Player_AddMoney(playerid, -bet);
            Roulette_Play(playerid, CasinoData[playerid][cRouletteType], bet);
            return 1;
        }

        // ---------------- slots ----------------
        case DIALOG_SLOTS_BET:
        {
            if (!response) return 1;

            new bet;
            if (sscanf(inputtext, "d", bet))
            {
                SendClientMessage(playerid, COLOR_ERROR, "Enter a number.");
                return 1;
            }
            if (!Casino_CanBet(playerid, bet)) return 1;

            Player_AddMoney(playerid, -bet);
            Slots_Play(playerid, bet);
            return 1;
        }

        // ---------------- poker ----------------
        case DIALOG_POKER_BET:
        {
            if (!response) return 1;

            new bet;
            if (sscanf(inputtext, "d", bet))
            {
                SendClientMessage(playerid, COLOR_ERROR, "Enter a number.");
                return 1;
            }
            if (!Casino_CanBet(playerid, bet)) return 1;

            Poker_Start(playerid, bet);
            return 1;
        }
        case DIALOG_POKER_AGAIN:
        {
            if (!response) return 1;
            ShowPlayerDialog(playerid, DIALOG_POKER_BET, DIALOG_STYLE_INPUT, "Poker",
                "Your bet? (minimum $100, maximum $100000)", "Deal", "Cancel");
            return 1;
        }
        case DIALOG_POKER_HOLD:
        {
            if (!response) return 1;
            if (CasinoData[playerid][cPokerBet] <= 0) return 1;

            if (listitem == 5)      // DRAW
            {
                Poker_Draw(playerid);
                return 1;
            }

            if (listitem < 0 || listitem > 4) return 1;

            CasinoData[playerid][cPokerHeld][listitem] = !CasinoData[playerid][cPokerHeld][listitem];
            Poker_ShowHold(playerid);
            return 1;
        }
    }
    return 0;
}

stock Casino_OnCommandText(playerid, const cmdtext[])
{
    if (strcmp(cmdtext, "/casino", true) == 0)
    {
        if (!Player_IsLogged(playerid))
        {
            SendClientMessage(playerid, COLOR_ERROR, "You must be logged in.");
            return 1;
        }
        Casino_ShowMenu(playerid);
        return 1;
    }

    return 0;
}
