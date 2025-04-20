#ifndef UI_H
#define UI_H

#include "map.h"
#include "party.h"

#define MAX_MESSAGES 9

typedef struct
{
	char *message;
	int colour;
} Message;

typedef struct
{
	Message *messages[MAX_MESSAGES];
	int count;
	int start;
} MessageLog;

//struct PartyMember;
//struct MemberState;

void DrawPartyFrame(void);
void DrawPartyMemberFrame(const int i, bool turn);
void DrawDeadPartyMemberFrame(const int i);
void DrawSecondaryProgressBar(int partyIndex);
void DrawXPProgressBar(int partyIndex);
void PushMessage(const char *msg, const int colour);
void DrawCentreCombatUI(void);
void ClearMessageLog();
void DrawFullUI(const int dir);
void DrawActionMenuEnemy(void);
bool DrawActionMenuParty(PartyMember *member, MemberState *state, int selectedIdx, int prevIdx);
void DrawHealthBar(int index);
void DrawDeadPartyMemberIndicator(const int index);
void DrawArrow(const int i, const uint32_t col);
void DrawCompass(const int dir);
void DrawMap(MapData *map);

#endif //UI_H
