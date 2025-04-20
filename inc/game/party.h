#ifndef PARTY_H
#define PARTY_H

#include "ability_type.h"
#include "class.h"
#include "item.h"
#include "xy.h"
#include <stdbool.h>

typedef struct {
	Item items[9];
} EquippedItems;

typedef struct
{
	int mainStat;
	int crit;
	int mastery;
	int haste;
	int res;
	int avoid;
	int leech;
} TotalStats;

typedef struct
{
	float crit;
	float mastery;
	float haste;
} BaseSecondaries;

typedef struct
{
	float coeff;
	float crit;
	float damageMulti;
	float haste;
	float damageTaken;
	float avoid;
	float leech;
} CombatStats;

typedef struct
{
	int cooldownState[10];
	int damageDone;
	int healingDone;
	size_t otCount; // owned otEffect
	size_t otCapacity;
	size_t otRecieved; // recieved otEffects
	CombatStats combatStats;
	OverTimeEffect *otEffects;
	bool alive;
} MemberState;

typedef struct
{
	const Class *classData;
	int specIndex;
	EquippedItems gear;
	TotalStats stats;
	xy health;
	xy secondary;
	int level;
	xy xp;
} PartyMember;


PartyMember party[6];
int mapIndex;
bool encountersEnabled;
bool inSafezone;
struct xy pos;
int direction;


void EquipItem(PartyMember *member, Item *item);
void RecalculateMemberStats(PartyMember *member);
void GenerateAndEquipItemSet(PartyMember *member, const int ilvl, const Rarity rarity, const char *base);

#endif // PARTY_H