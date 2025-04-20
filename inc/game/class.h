#ifndef CLASS_H
#define CLASS_H

#include "ability_type.h"
#include "item.h"
#include <stdbool.h>
#include <stdint.h>

#define NUM_SECONDARY 5
#define NUM_ABILITES 10
#define NUM_SPECS 3
#define NUM_CLASSES 6


typedef enum 
{
	ROLE_TANK,
	ROLE_HEALER,
	ROLE_DPS,
	ROLE_SUPPORT
} Role;

typedef enum
{
	SECONDARY_MANA,
	SECONDARY_WARD,
	SECONDARY_RAGE,
	SECONDARY_ENERGY,
	SECONDARY_SPIRIT
} SecondaryType;

typedef struct
{
	const char *name;
	const char *description;
	const AbilityType type;
	const AbilitySubtype subtype;
	const uint32_t iconColour;
	const uint32_t iconBorder;
	const int id;
	const float coeff; // damage or healing amount
	const int cost;
	const int cooldown;
	const int duration; // duration of effect, if needed
	const bool offGcd;
} Ability;

typedef struct
{
	float crit;
	float haste;
	float mastery;
} BaseStats;

typedef struct
{
	const char *name;
	const Role role;
	const MainStat mainStat;
	const BaseStats baseStats;
	const SecondaryType secondaryType;
	const Ability abilities[NUM_ABILITES];
	const uint32_t palette[7];
} Spec;

typedef struct
{
	const char *name;
	const int index;
	const int colour;
	const Spec specs[NUM_SPECS];
	const uint8_t sprite[16][13];
} Class;


const Class classes[NUM_CLASSES];
const uint32_t roleColour[4];
const uint32_t roleBackground[4];
const char *roleString[4];

#endif CLASS_H