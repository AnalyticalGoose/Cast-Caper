#ifndef ABILITY_H
#define ABILITY_H

#include "enemy.h"
#include "party.h"


typedef struct {
	MemberState *state;
	int originalIndex;
} MeterEntry;

typedef struct {
	PartyMember *member;
	int originalIndex;
} HealTarget;


void DoOffensivePartyAbility(MemberState *partyState, PartyMember *partyMember, MemberState *memberState, EnemyState *targetState, const int targetIdx, const char *targetName, const int abilityIdx);
bool DoDefensivePartyAbility(MemberState *partyState, PartyMember *partyMember, MemberState *memberState, MemberState *targetState, const int targetIdx, const int abilityIdx);
void DoEnemyAbility(const Enemy *enemy, EnemyState *Enemystate, MemberState *memberState);

void UpdateCombatMeters(MemberState partyState[6], const int cycle);

bool CanUseAbility(PartyMember *member, MemberState *state, Ability *ability, const int index);

void DrawAllyStatusIcons(MemberState *state);
void DrawEnemyStatusIcons(MemberState *state, const int enemyIndex);

bool TickOverTimeEffects(MemberState *state, MemberState *partyState, EnemyState *enemyState, const int index);

// old
//void DoPartyAbility(const PartyMember *partyMember, MemberState *state, EnemyState *enemyState, int abilityIdx);
//void DoPartyHeal(const PartyMember *caster, MemberState *state, int targetIdx, int abilityIdx);


int numEnemiesAbil;
bool boss;

#endif // ABILITY_H

