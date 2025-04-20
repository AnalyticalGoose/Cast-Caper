#ifndef ABILITY_TYPE_H
#define ABILITY_TYPE_H

#include "xy.h"
#include <stdint.h>

typedef enum
{
	ABILITY_DAMAGE,
	ABILITY_HEAL,
	ABILITY_BUFF,
	ABILITY_DEBUFF,
} AbilityType;

typedef enum
{
	DAMAGE_SINGLE_TARGET,
	DAMAGE_DOT,
	DAMAGE_AOE,
	DAMAGE_AOE_DOT,
	DAMAGE_EXECUTE,
	DAMAGE_MASOCHIST,
	DAMAGE_LEECH,
	DAMAGE_SCALE_EFFECTS,
	DAMAGE_WARD_BUILDER,
	DAMAGE_TANK_BUSTER,

	HEAL_SINGLE_TARGET,
	HEAL_HOT,
	HEAL_ST_AND_HOT, // 1 person
	HEAL_SMART_HOT, // 3 person smart AOE
	HEAL_AOE_HOT, // Party AOE
	HEAL_AOE,

	BUFF_DR,
	BUFF_DAMAGE,
	BUFF_GIVE_SECONDARY,
	BUFF_EXTEND_EFFECTS,
	BUFF_RES,
	BUFF_LUST,
} AbilitySubtype;


typedef struct
{
	int id;
	int target;
	float amount;
	AbilityType type;
	uint32_t iconCol;
	uint32_t borderCol;
	int duration;
	xy uiLocation;
} OverTimeEffect;


#endif // ABILITY_TYPE_H
