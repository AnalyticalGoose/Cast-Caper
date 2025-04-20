#ifndef ENEMY_H
#define ENEMY_H

#define NUM_ENEMIES 18
#include "ability_type.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
	const char *name;
	const int cooldown;
	const AbilityType type;
	const AbilitySubtype subtype;
	const int damage[6];
} EnemyAbility;

typedef struct
{
	const int ilvl[2];
	const int rarity[4];
	const char *baseName;
} LootTable;

typedef struct 
{
	const char *singularPrefix;
	const char *nameSingular;
	const char *nameplural;
	const int health[3];
	const int haste;
	const EnemyAbility abilities[3];
	const LootTable loot;
	const unsigned char (*sprite)[4];
} Enemy;


typedef struct
{
	int health;
	int cooldownState[3];
	size_t otRecieved; // recieved otEffects
	bool alive;
	bool greyOutNeeded;
} EnemyState;


extern const Enemy enemies[NUM_ENEMIES];


#endif // ENEMY_H