#ifndef ITEM_H
#define ITEM_H

#include "enemy.h"

typedef enum {
	SLOT_HEAD,
	SLOT_SHOULDER,
	SLOT_CHEST,
	SLOT_HANDS,
	SLOT_LEGS,
	SLOT_FEET,
	SLOT_NECK,
	SLOT_RING,
	SLOT_WEAPON,
} Slot;

typedef enum {
	RARITY_NORMAL,
	RARITY_MAGIC,
	RARITY_RARE,
	RARITY_EPIC,
} Rarity;

const uint32_t rarityColours[4];

typedef enum {
	MAINSTAT_STR,
	MAINSTAT_AGI,
	MAINSTAT_INT,
} MainStat;


typedef struct {
	MainStat mainStatType;
	int mainStat;
	int crit;
	int haste;
	int mastery;
	int res;
	int avoid;
	int leech;
} ItemStats;


typedef struct {
	char name[128];
	ItemStats stats;
	int ilvl;
	Slot slot;
	Rarity rarity;
} Item;


typedef struct {
	int mainStat;
	int secondaryStats[2];
} GearStat;


typedef struct {
	int key;             
	const char *suffix;  
} SecondarySuffix;


typedef struct {
	Rarity rarity;                  // 1: Magic, 2: Rare, 3: Epic
	SecondarySuffix *suffixes;      // Array of suffix entries.
	int count;                      // Number of entries in the array.
} SecondarySuffixList;


typedef struct {
	char *baseName;
	int ilvl;
	Slot slot;
	MainStat mainStat;
	Rarity rarity;
} ItemGenData;

Item GenerateItem(const MainStat mainstat, const Slot slot, const int ilvl, const Rarity rarity, const char *base);
ItemGenData *BuildItemGenDataArray(const Enemy **enemies, int numEnemies);

#endif // ITEM_H
