#include "ability.h"
#include "colour.h"
#include "difficulty.h"
#include "enemy.h"
#include "framebuffer.h"
#include "movement_state.h"
#include "party.h"
#include "renderer.h"
#include "stats.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>

#define STATUS_ICONS_PER_ROW 4

int numEnemiesAbil = 0;
bool boss = false;

// Private function prototypes
static void DoDamagePartyAbility(
    const Ability *ability, 
    EnemyState *enemyState, 
    MemberState *memberState, 
    PartyMember *partyMember, 
    const char *targetName, 
    const int targetIdx);

static void DoHealPartyAbility(
    MemberState *partyState,
    const Ability *ability,
    const int targetIdx,
    const int abilityIdx,
    const char *healerName,
    MemberState *healerState,
    MemberState *targetState,
    PartyMember *partyMember);

static void DoBuffPartyAbility(
    MemberState *partyState, 
    const Ability *ability, 
    const int targetIdx, 
    const int abilityIdx, 
    const char *casterName, 
    MemberState *casterState, 
    MemberState *targetState);

static void DoSingleTargetDamage(const Ability *ability, MemberState *memberState, EnemyState *enemyState, PartyMember *partyMember, const char *targetName);
static void DoAoeDamage(const Ability *ability, MemberState *memberState, EnemyState *enemyState, PartyMember *partyMember, const int targetIdx);
static void DoSingleTargetHeal(const Ability *ability, const int targetIdx, const char *healerName, MemberState *state, PartyMember *partyMember);
static void DoAoeHeal(const Ability *ability, PartyMember *partyMember, MemberState *healerState, const char *healerName);
static void DoEnemyDamageAbility(const EnemyAbility *ability, const char *enemyName, MemberState *memberState, EnemyState *state, Enemy *enemy);

static int DoHealTick(const int amount, const int targetIdx, MemberState *state);
static int DoDamageTick(const int amount, EnemyState *enemyState, MemberState *state, const int enemyIndex);
static void GiveExp(PartyMember *partyMember, const int value);
static void SetAbilityCostOrGen(PartyMember *member, const int cost);
static void HandleDeath(MemberState *memberState, PartyMember *target, const int targetIndex);

static void AddAllyOverTimeEffect(MemberState *ownerState, MemberState *recieverState, OverTimeEffect effect);
static void RemoveAllyEffect(MemberState *ownerState, MemberState *recieverState, size_t i);
static void AddEnemyOverTimeEffect(MemberState *ownerState, EnemyState *recieverState, OverTimeEffect effect);
static void RemoveEnemyEffect(MemberState *ownerState, EnemyState *recieverState, size_t i);
static void UpdateAllyStatusIcon(OverTimeEffect *otEffect);

static int CompareDamageEntry(const void *a, const void *b);
static int CompareHealingEntry(const void *a, const void *b);
static int CompareHealTargets(const void *a, const void *b);

static void PushDamageMessage(const char *attackerName, const char *targetName, const char *abilityName, const int damage, const uint32_t colour);
static void PushHealMessage(const char *healerName, const char *targetName, const char *abilityName, const int healing, const uint32_t colour);
static void PushAvoidMessage(const char *attackerName, const char *targetName, const uint32_t colour);
static void PushDeathMessage(const char *name);

static void GetEnemyPositionAbil(const int enemyIndex, int *x, int *y);



// For offensive actions that target an enemy, i.e. a damage or debuff ability
void DoOffensivePartyAbility(MemberState *partyState, 
                             PartyMember *partyMember, 
                             MemberState *memberState, 
                             EnemyState *enemyState, 
                             const int targetIdx, 
                             const char *targetName,
                             const int abilityIdx)
{
    const Ability *ability = &partyMember->classData->specs[partyMember->specIndex].abilities[abilityIdx];
    memberState->cooldownState[abilityIdx] = ability->cooldown;
    SetAbilityCostOrGen(partyMember, ability->cost);

    switch (ability->type)
    {
        case ABILITY_DAMAGE:
        {
            DoDamagePartyAbility(ability, enemyState, memberState, partyMember, targetName, targetIdx);
        } break;
    }
}

// For defensive actions that target an ally, i.e. a buff or heal ability
bool DoDefensivePartyAbility(MemberState *partyState,
                             PartyMember *partyMember, 
                             MemberState *memberState,
                             MemberState *targetState,
                             const int targetIdx, 
                             const int abilityIdx)
{
    const Ability *ability = &partyMember->classData->specs[partyMember->specIndex].abilities[abilityIdx];
    memberState->cooldownState[abilityIdx] = ability->cooldown;
    partyMember->secondary.x -= ability->cost;

    switch (ability->type)
    {
        case ABILITY_HEAL:
        {
            DoHealPartyAbility(partyState, ability, targetIdx, abilityIdx, partyMember->classData->name, memberState, targetState, partyMember);
        } break;

        case ABILITY_BUFF:
        {
            DoBuffPartyAbility(partyState, ability, targetIdx, abilityIdx, partyMember->classData->name, memberState, targetState);
            if (ability->subtype == BUFF_RES) return true; // trigger redraw

        } break;
    }
    return false;
}



static void DoDamagePartyAbility(const Ability *ability,
                                 EnemyState *enemyState,
                                 MemberState *memberState,
                                 PartyMember *partyMember,
                                 const char *targetName,
                                 const int targetIdx)
{
    switch (ability->subtype)
    {
        case DAMAGE_SINGLE_TARGET:
        case DAMAGE_EXECUTE:
        case DAMAGE_MASOCHIST:
        case DAMAGE_LEECH:
        case DAMAGE_SCALE_EFFECTS:
        case DAMAGE_WARD_BUILDER:
        {
            DoSingleTargetDamage(ability, memberState, &enemyState[targetIdx], partyMember, targetName);
        } break;

        case DAMAGE_AOE:
        {
            DoAoeDamage(ability, memberState, enemyState, partyMember, targetIdx);
        } break;

        case DAMAGE_DOT:
        {
            DoSingleTargetDamage(ability, memberState, &enemyState[targetIdx], partyMember, targetName);

            if (memberState->otCount > 0)
            {
                for (size_t i = 0; i < memberState->otCount; i++)
                {
                    // Check if this effect has the same ID and is applied to the same target.
                    if (memberState->otEffects[i].id == ability->id &&
                        memberState->otEffects[i].target == targetIdx)
                    {
                        if (!enemyState[targetIdx].alive) continue;

                        // Refresh the duration.
                        memberState->otEffects[i].duration = ability->duration;
                        DrawEnemyStatusIcons(memberState, targetIdx);
                        return;  // Exit early, do not add a new effect.
                    }
                }
            }

            OverTimeEffect dotEffect = {
            .id = ability->id,
            .iconCol = ability->iconColour,
            .borderCol = ability->iconBorder,
            .amount = ability->coeff * memberState->combatStats.coeff,
            .target = targetIdx,
            .type = ABILITY_DAMAGE,
            .duration = ability->duration,
            .uiLocation = { enemyState[targetIdx].otRecieved, 0}
            };

            AddEnemyOverTimeEffect(memberState, &enemyState[targetIdx], dotEffect);
            DrawEnemyStatusIcons(memberState, targetIdx);
        } break;

        case DAMAGE_AOE_DOT:
        {
            OverTimeEffect dotEffect = {
           .id = ability->id,
           .iconCol = ability->iconColour,
           .borderCol = ability->iconBorder,
           .amount = ability->coeff * memberState->combatStats.coeff,
           .type = ABILITY_DAMAGE,
           .duration = ability->duration,
            };

            int enemyCount = 0;
            if (!boss)
            {
                enemyCount = 6;
            }
            else
            {
                enemyCount = 1;
            }

            for (size_t i = 0; i < enemyCount; i++)
            {
                if (!enemyState[i].alive) continue;

                if (memberState->otCount > 0)
                {
                    for (size_t j = 0; j < memberState->otCount; j++)
                    {
                        // Check if this effect has the same ID and is applied to the same target.
                        if (memberState->otEffects[j].id == ability->id &&
                            memberState->otEffects[j].target == i)
                        {
                            // Refresh the duration.
                            memberState->otEffects[j].duration = ability->duration;
                            DrawEnemyStatusIcons(memberState, i);
                            return;  // Exit early, do not add a new effect.
                        }
                    }
                }

                dotEffect.target = i;
                dotEffect.uiLocation = (xy){ enemyState[i].otRecieved, 0 };

                AddEnemyOverTimeEffect(memberState, &enemyState[i], dotEffect);
                DrawEnemyStatusIcons(memberState, i);
            }

        } break;
    }
}

static void DoHealPartyAbility(MemberState *partyState,
                               const Ability *ability, 
                               const int targetIdx, 
                               const int abilityIdx, 
                               const char *healerName,
                               MemberState *healerState,
                               MemberState *targetState,
                               PartyMember *partyMember)
{
    switch (ability->subtype)
    {
        case HEAL_SINGLE_TARGET:
        {
            DoSingleTargetHeal(ability, targetIdx, healerName, healerState, partyMember);
        } break;

        case HEAL_HOT:
        {
            DoSingleTargetHeal(ability, targetIdx, healerName, healerState, partyMember);

            if (healerState->otCount > 0)
            {
                for (size_t i = 0; i < healerState->otCount; i++)
                {
                    // Check if this effect has the same ID and is applied to the same target.
                    if (healerState->otEffects[i].id == ability->id &&
                        healerState->otEffects[i].target == targetIdx)
                    {
                        // Refresh the duration.
                        healerState->otEffects[i].duration = ability->duration;
                        DrawAllyStatusIcons(healerState);
                        return;  // Exit early, do not add a new effect.
                    }
                }
            }

            const int targetX = targetState->otRecieved % STATUS_ICONS_PER_ROW;
            const int targetY = targetState->otRecieved / STATUS_ICONS_PER_ROW;

            OverTimeEffect hotEffect = {
            .id = ability->id,
            .iconCol = ability->iconColour,
            .borderCol = ability->iconBorder,
            .amount = ability->coeff * healerState->combatStats.coeff,
            .target = targetIdx,
            .type = ABILITY_HEAL,
            .duration = ability->duration,
            .uiLocation = { targetX, targetY }
            };

            AddAllyOverTimeEffect(healerState, targetState, hotEffect);
            DrawAllyStatusIcons(healerState);

        } break;

        case HEAL_ST_AND_HOT:
        {
            DoSingleTargetHeal(ability, targetIdx, healerName, healerState, partyMember);

            if (healerState->otCount > 0)
            {
                for (size_t i = 0; i < healerState->otCount; i++)
                {
                    // Check if this effect has the same ID and is applied to the same target.
                    if (healerState->otEffects[i].id == ability->id &&
                        healerState->otEffects[i].target == targetIdx)
                    {
                        // Refresh the duration.
                        healerState->otEffects[i].duration = ability->duration;
                        DrawAllyStatusIcons(healerState);
                        return;  // Exit early, do not add a new effect.
                    }
                }
            }

            const int targetX = targetState->otRecieved % STATUS_ICONS_PER_ROW;
            const int targetY = targetState->otRecieved / STATUS_ICONS_PER_ROW;

            OverTimeEffect hotEffect = {
            .id = ability->id,
            .iconCol = ability->iconColour,
            .borderCol = ability->iconBorder,
            .amount = (ability->coeff / 3) * healerState->combatStats.coeff,
            .target = targetIdx,
            .type = ABILITY_HEAL,
            .duration = ability->duration,
            .uiLocation = { targetX, targetY }
            };

            AddAllyOverTimeEffect(healerState, targetState, hotEffect);
            DrawAllyStatusIcons(healerState);
        } break;

        case HEAL_SMART_HOT:
        {
            HealTarget targets[5];
            int count = 0;

            for (int i = 0; i < 6; i++)
            {
                if (i == targetIdx) continue;

                PartyMember *member = &party[i];

                if (member->health.x == 0) continue; // Dirty dead check
                
                targets[count].member = member;
                targets[count].originalIndex = i;
                count++;
            }

            if (count > 0)
            {
                qsort(targets, count, sizeof(HealTarget), CompareHealTargets);

                OverTimeEffect hotEffect = {
                .id = ability->id,
                .iconCol = ability->iconColour,
                .borderCol = ability->iconBorder,
                .amount = ability->coeff * healerState->combatStats.coeff,
                .type = ABILITY_HEAL,
                .duration = ability->duration,
                };

                for (int j = 0; j < 3; j++)
                {
                    MemberState *target = &partyState[targets[j].originalIndex];

                    const int targetX = target->otRecieved % STATUS_ICONS_PER_ROW;
                    const int targetY = target->otRecieved / STATUS_ICONS_PER_ROW;

                    hotEffect.target = targets[j].originalIndex;
                    hotEffect.uiLocation = (xy){ targetX, targetY };

                    AddAllyOverTimeEffect(healerState, target, hotEffect);
                }

                const int targetX = targetState->otRecieved % STATUS_ICONS_PER_ROW;
                const int targetY = targetState->otRecieved / STATUS_ICONS_PER_ROW;

                hotEffect.target = targetIdx;
                hotEffect.uiLocation = (xy){ targetX, targetY };

                AddAllyOverTimeEffect(healerState, targetState, hotEffect);
                DrawAllyStatusIcons(healerState);
            }
        } break;

        case HEAL_AOE_HOT:
        {
            OverTimeEffect hotEffect = {
            .id = ability->id,
            .iconCol = ability->iconColour,
            .borderCol = ability->iconBorder,
            .amount = ability->coeff * healerState->combatStats.coeff,
            .type = ABILITY_HEAL,
            .duration = ability->duration,
            };

            for (int i = 0; i < 6; i++)
            {
                if (party[i].health.x == 0) continue;

                DoSingleTargetHeal(ability, i, healerName, healerState, partyMember);

                MemberState *target = &partyState[i];

                const int targetX = target->otRecieved % STATUS_ICONS_PER_ROW;
                const int targetY = target->otRecieved / STATUS_ICONS_PER_ROW;

                hotEffect.target = i;
                hotEffect.uiLocation = (xy){ targetX, targetY };

                AddAllyOverTimeEffect(healerState, target, hotEffect);
                DrawAllyStatusIcons(healerState);
            }

        } break;

        case HEAL_AOE:
        {
            DoAoeHeal(ability, partyMember, healerState, healerName);
        } break;
    }
}

static void DoBuffPartyAbility(MemberState *partyState,
                               const Ability *ability,
                               const int targetIdx,
                               const int abilityIdx,
                               const char *casterName,
                               MemberState *casterState,
                               MemberState *targetState)
{
    switch (ability->subtype)
    {
        case BUFF_DR:
        {
            targetState->combatStats.damageTaken -= ability->coeff; // apply DR

            const int targetX = targetState->otRecieved % STATUS_ICONS_PER_ROW;
            const int targetY = targetState->otRecieved / STATUS_ICONS_PER_ROW;

            OverTimeEffect drEffect = {
                .id = ability->id,
                .iconCol = ability->iconColour,
                .borderCol = ability->iconBorder,
                .amount = ability->coeff,
                .target = targetIdx,
                .type = BUFF_DR,
                .duration = ability->duration,
                .uiLocation = { targetX, targetY }
            };

            AddAllyOverTimeEffect(casterState, targetState, drEffect);
            DrawAllyStatusIcons(casterState);
        } break;

        case BUFF_DAMAGE:
        {
            targetState->combatStats.damageMulti += ability->coeff;

            const int targetX = targetState->otRecieved % STATUS_ICONS_PER_ROW;
            const int targetY = targetState->otRecieved / STATUS_ICONS_PER_ROW;

            OverTimeEffect dmgEffect = {
                .id = ability->id,
                .iconCol = ability->iconColour,
                .borderCol = ability->iconBorder,
                .amount = ability->coeff,
                .target = targetIdx,
                .type = BUFF_DAMAGE,
                .duration = ability->duration,
                .uiLocation = { targetX, targetY }
            };

            AddAllyOverTimeEffect(casterState, targetState, dmgEffect);
            DrawAllyStatusIcons(casterState);
        } break;

        case BUFF_GIVE_SECONDARY:
        {
            party[targetIdx].secondary.x += (ability->coeff * party[targetIdx].secondary.y);

            if (party[targetIdx].secondary.x > party[targetIdx].secondary.y)
            {
                party[targetIdx].secondary.x = party[targetIdx].secondary.y;
            }

            DrawSecondaryProgressBar(targetIdx);

        } break;

        case BUFF_RES:
        {
            PartyMember *member = &party[targetIdx];

            targetState->alive = true;
            member->health.x = (int)(member->health.y / 2);
            member->secondary.y = 0;

            // Clear dead indicator
            const int y = (67 * targetIdx) + 16 + 11;
            const int x = (550 + 160 + (16 * 3)) + 2 + 64;
            DrawGreyscaleRectangle(x, y, 128, 49, 0);

            // Redraw bars
            DrawHealthBar(targetIdx);
            DrawSecondaryProgressBar(targetIdx);
            DrawXPProgressBar(targetIdx);
        } break;

        case BUFF_EXTEND_EFFECTS:
        {
            for (size_t i = 0; i < targetState->otCount; i++)
            {
                if (targetState->otEffects[i].duration > 7)
                {
                    targetState->otEffects[i].duration = 9;
                }
                else
                {
                    targetState->otEffects[i].duration += 2;
                }
            }

            DrawAllyStatusIcons(targetState);
        } break;

        case BUFF_LUST:
        {
            for (int i = 0; i < 6; i++)
            {
                if (party[i].health.x == 0) continue;

                MemberState *target = &partyState[i];

                target->combatStats.damageMulti += ability->coeff;

                const int targetX = target->otRecieved % STATUS_ICONS_PER_ROW;
                const int targetY = target->otRecieved / STATUS_ICONS_PER_ROW;

                OverTimeEffect dmgEffect = {
                    .id = ability->id,
                    .iconCol = ability->iconColour,
                    .borderCol = ability->iconBorder,
                    .amount = ability->coeff,
                    .target = i,
                    .type = BUFF_DAMAGE,
                    .duration = ability->duration,
                    .uiLocation = { targetX, targetY }
                };
                AddAllyOverTimeEffect(casterState, target, dmgEffect);
            }
            DrawAllyStatusIcons(casterState);
        } break;
    }
}


static void DoSingleTargetDamage(const Ability *ability, MemberState *memberState, EnemyState *enemyState, PartyMember *partyMember, const char *targetName)
{
    float randomScale = 0.95f + ((float)rand() / RAND_MAX) * 0.1f;
    int damage = (int)(ability->coeff * memberState->combatStats.coeff) * randomScale;
    damage *= memberState->combatStats.damageMulti;

    if (ability->subtype == DAMAGE_EXECUTE)
    {
        if (enemyState->health < partyMember->health.y)
        {
            damage *= 2;
        }
    }
    else if (ability->subtype == DAMAGE_MASOCHIST)
    {
        const float injuredMulti = 1.0f + ((float)(partyMember->health.y - partyMember->health.x)) / ((float)partyMember->health.y);
        damage = (int)(damage * injuredMulti);
    }
    else if (ability->subtype == DAMAGE_LEECH)
    {
        const int leechedDmg = (int)(damage / 3);
        partyMember->health.x += leechedDmg;

        if (partyMember->health.x > partyMember->health.y)
        {
            partyMember->health.x = partyMember->health.y;
        }

        memberState->healingDone += leechedDmg;
    }
    else if (ability->subtype == DAMAGE_SCALE_EFFECTS)
    {
        damage = (int)((ability->coeff * memberState->otCount) * memberState->combatStats.coeff) * randomScale;
    }
    else if (ability->subtype == DAMAGE_WARD_BUILDER)
    {
        partyMember->secondary.x += damage;

        if (partyMember->secondary.x > partyMember->secondary.y)
        {
            partyMember->secondary.x = partyMember->secondary.y;
        }
    }

    float critChance = memberState->combatStats.crit;
    float critRoll = ((float)rand() / (float)RAND_MAX) * 100.0f;
    if (critRoll < critChance)
    {
        damage *= 2.0;
        PushDamageMessage(partyMember->classData->name, targetName, ability->name, damage, 0XFF0000);
    }
    else
    {
        PushDamageMessage(partyMember->classData->name, targetName, ability->name, damage, 0XFFFFFF);
    }

    memberState->damageDone += damage;
    enemyState->health -= damage;

    int leech = (damage * memberState->combatStats.leech) / 100;
    memberState->healingDone += leech;
    partyMember->health.x += leech;

    if (partyMember->health.x > partyMember->health.y)
    {
        partyMember->health.x = partyMember->health.y;
    }

    GiveExp(partyMember, damage);

    if (enemyState->health <= 0)
    {
        enemyState->health = 0;
        enemyState->alive = false;

        for (size_t i = 0; i < memberState->otCount;)
        {
            OverTimeEffect *effect = &memberState->otEffects[i];

            if (!enemyState[effect->target].alive)
            {
                RemoveEnemyEffect(memberState, &enemyState[effect->target], i);
                continue;
            }
            i++;
        }
    }
}

static void DoAoeDamage(const Ability *ability, MemberState *memberState, EnemyState *enemyState, PartyMember *partyMember, const int targetIdx)
{
    int totalDamage = 0;

    for (size_t i = 0; i < numEnemiesAbil; i++)
    {
        if (!enemyState[i].alive) continue;

        float randomScale = 0.95f + ((float)rand() / RAND_MAX) * 0.1f;
        int damage = (int)(ability->coeff * memberState->combatStats.coeff) * randomScale;
        damage *= memberState->combatStats.damageMulti;

        if (i == targetIdx)
        {
            damage *= 2;
        }

        float critChance = memberState->combatStats.crit;
        float critRoll = ((float)rand() / (float)RAND_MAX) * 100.0f;
        if (critRoll < critChance)
        {
            damage *= 2.0;
        }
        
        totalDamage += damage;
        enemyState[i].health -= damage;

        if (enemyState[i].health <= 0)
        {
            enemyState[i].health = 0;
            enemyState[i].alive = false;
            enemyState[i].greyOutNeeded = true;

            for (size_t i = 0; i < memberState->otCount; i++)
            {
                OverTimeEffect *effect = &memberState->otEffects[i];

                if (!enemyState[effect->target].alive)
                {
                    RemoveEnemyEffect(memberState, &enemyState[effect->target], i);
                    continue;
                }
                i++;
            }
        }
    }

    int leech = (totalDamage * memberState->combatStats.leech) / 100;
    memberState->healingDone += leech;
    partyMember->health.x += leech;

    if (partyMember->health.x > partyMember->health.y)
    {
        partyMember->health.x = partyMember->health.y;
    }

    memberState->damageDone += totalDamage;
    PushDamageMessage(partyMember->classData->name, "all enemies", ability->name, totalDamage, 0XFFFFFF);
    GiveExp(partyMember, totalDamage);
}

static void DoSingleTargetHeal(const Ability *ability, const int targetIdx, const char *healerName, MemberState *state, PartyMember *partyMember)
{
    const float randomScale = 0.95f + ((float)rand() / RAND_MAX) * 0.1f;
    int heal = (int)(ability->coeff * state->combatStats.coeff * randomScale);
    heal *= state->combatStats.damageMulti;

    PartyMember *target = &party[targetIdx];

    float critChance = state->combatStats.crit;
    float critRoll = ((float)rand() / (float)RAND_MAX) * 100.0f;
    if (critRoll < critChance)
    {
        heal *= 2.0;
        PushDamageMessage(healerName, target->classData->name, ability->name, heal, 0Xe4cf00);
    }
    else
    {
        PushHealMessage(healerName, target->classData->name, ability->name, heal, 0x51c000);
    }

    int leech = (heal * state->combatStats.leech) / 100;
    state->healingDone += leech;
    partyMember->health.x += leech;

    if (partyMember->health.x > partyMember->health.y)
    {
        partyMember->health.x = partyMember->health.y;
    }

    state->healingDone += heal;
    target->health.x += heal;

    GiveExp(partyMember, heal);

    if (target->health.x > target->health.y)
    {
        target->health.x = target->health.y;
    }

    DrawHealthBar(targetIdx);
}


static void DoAoeHeal(const Ability *ability, PartyMember *partyMember, MemberState *healerState, const char *healerName)
{
    int totalhealing = 0;

    for (int i = 0; i < 6; i++)
    {
        if (party[i].health.x == 0) continue;

        const float randomScale = 0.95f + ((float)rand() / RAND_MAX) * 0.1f;
        int heal = (int)(ability->coeff * healerState->combatStats.coeff * randomScale);
        heal *= healerState->combatStats.damageMulti;

        float critChance = healerState->combatStats.crit;
        float critRoll = ((float)rand() / (float)RAND_MAX) * 100.0f;
        if (critRoll < critChance)
        {
            heal *= 2.0;
        }

        PartyMember *target = &party[i];
        healerState->healingDone += heal;
        target->health.x += heal;
        totalhealing += heal;

        if (target->health.x > target->health.y)
        {
            target->health.x = target->health.y;
        }

        DrawHealthBar(i);
    }

    int leech = (totalhealing * healerState->combatStats.leech) / 100;
    healerState->healingDone += leech;
    partyMember->health.x += leech;

    if (partyMember->health.x > partyMember->health.y)
    {
        partyMember->health.x = partyMember->health.y;
    }

    PushHealMessage(healerName, "all allies", ability->name, totalhealing, 0x51c000);
}

static int DoDamageTick(const int amount, EnemyState *enemyState, MemberState *state, const int enemyIndex)
{
    const float randomScale = 0.95f + ((float)rand() / RAND_MAX) * 0.1f;
    int damage = (int)(amount * randomScale);
    damage *= state->combatStats.damageMulti;

    float critChance = state->combatStats.crit;
    float critRoll = ((float)rand() / (float)RAND_MAX) * 100.0f;
    if (critRoll < critChance)
    {
        damage *= 2.0;
    }

    state->damageDone += damage;
    enemyState->health -= damage;

    if (enemyState->health <= 0)
    {
        enemyState->health = 0;
        enemyState->alive = false;
        enemyState->greyOutNeeded = true;
    }

    return damage;
}

static int DoHealTick(const int amount, const int targetIdx, MemberState *state)
{
    const float randomScale = 0.95f + ((float)rand() / RAND_MAX) * 0.1f;
    int heal = (int)(amount * randomScale);
    heal *= state->combatStats.damageMulti;

    float critChance = state->combatStats.crit;
    float critRoll = ((float)rand() / (float)RAND_MAX) * 100.0f;
    if (critRoll < critChance)
    {
        heal *= 2.0;
    }

    PartyMember *target = &party[targetIdx];
    state->healingDone += heal;
    target->health.x += heal;

    if (target->health.x > target->health.y)
    {
        target->health.x = target->health.y;
    }

    DrawHealthBar(targetIdx);
    return heal;
}


static void GiveExp(PartyMember *partyMember, const int value)
{
    int xp = 0;
    switch (gameDifficulty)
    {
        case DIFFICULTY_BUTTERCUP:
        {
            xp = value * 3;
        } break;

        case DIFFICULTY_NORMAL:
        {
            xp = value * 2;
        } break;

        case DIFFICULTY_MASOCHIST:
        {
            xp = value;
        } break;

    }

    if (partyMember->level == 10)
    {
        partyMember->xp.x += xp;
    }
    else
    {
        const int xpToLevelUp = partyMember->xp.y - partyMember->xp.x;

        if (xp < xpToLevelUp) // grant xp, nothing more needed
        {
            partyMember->xp.x += xp;
        }
        else if (xp >= xpToLevelUp) // level up
        {
            const int xpOverflow = xp - xpToLevelUp;

            partyMember->health.y *= 1.2;
            partyMember->secondary.y = (partyMember->secondary.y / partyMember->level) * (partyMember->level + 1);
            partyMember->level += 1;
            partyMember->xp.x = xpOverflow;
            partyMember->xp.y *= 1.8;

            const char *memberName = partyMember->classData->name;
            char levelUpMessage[64] = { 0 };
            sprintf_s(levelUpMessage, sizeof(levelUpMessage), "%s is now level %i!", memberName, partyMember->level);
            PushMessage(levelUpMessage, 0Xffa600);
        }
    }
}


static void SetAbilityCostOrGen(PartyMember *member, const int cost)
{
    if (cost == 0) return;

    if (cost > 0)
    {
        member->secondary.x -= cost;
    }
    else
    {
        member->secondary.x += -cost;

        if (member->secondary.x > member->secondary.y)
        {
            member->secondary.x = member->secondary.y;
        }
    }
}


static void HandleDeath(MemberState *memberState, PartyMember *target, const int targetIndex)
{
    PushDeathMessage(target->classData->name);

    target->health.x = 0;
    memberState[targetIndex].alive = false;

    DrawDeadPartyMemberFrame(targetIndex);
    DrawDeadPartyMemberIndicator(targetIndex);
}


void DoEnemyAbility(const Enemy *enemy, EnemyState *enemyState, MemberState *memberState)
{
    int abilityIdx = -1; // Default to an invalid state

    for (int i = 0; i < 3; i++) // decrement cooldowns by one
    {
        if (enemyState->cooldownState[i] > 0)
        {
            enemyState->cooldownState[i] -= 1;
        }
    }

    for (int j = 0; j < 3; j++) // choose next ability
    {
        if (enemyState->cooldownState[j] == 0)
        {
            abilityIdx = j;
            break;
        }
    }

    if (abilityIdx == -1) return;

    // Put ability on cooldown
    enemyState->cooldownState[abilityIdx] = enemy->abilities[abilityIdx].cooldown;

    switch (enemy->abilities[abilityIdx].type)
    {
        case ABILITY_DAMAGE:
        {
            DoEnemyDamageAbility(&enemy->abilities[abilityIdx], enemy->nameSingular, memberState, enemyState, enemy);
        } break;
    }
}


static void DoEnemyDamageAbility(const EnemyAbility *ability, const char *enemyName, MemberState *memberState, EnemyState *state, Enemy *enemy)
{
    const int diffOffset = gameDifficulty * 2;

    switch (ability->subtype)
    {
        case DAMAGE_SINGLE_TARGET:
        case DAMAGE_LEECH:
        case DAMAGE_TANK_BUSTER:
        {
            int damage = rand() % (ability->damage[diffOffset + 1] + 1 - ability->damage[diffOffset]) + ability->damage[diffOffset];

            int targetIndex = 0;

            if (ability->subtype == DAMAGE_TANK_BUSTER)
            {
                if (memberState[0].alive)
                {
                    targetIndex = 0;
                }
                else
                {
                    int targetIndex = rand() % 6;
                    damage *= 2;
                }
            }
            else
            {
                int targetIndex = rand() % 6;
            }

            if (!memberState[targetIndex].alive)
            {
                do
                {
                    targetIndex = (targetIndex + 1) % 6;
                } while (!memberState[targetIndex].alive);
            }

            float avoidChance = memberState[targetIndex].combatStats.avoid;
            float avoidRoll = ((float)rand() / (float)RAND_MAX) * 100.0f;
            if (avoidRoll < avoidChance)
            {
                PushAvoidMessage(enemyName, party[targetIndex].classData->name, 0Xffa600);
                return;
            }

            damage *= memberState[targetIndex].combatStats.damageTaken; // Scale damage by damage taken (any DR applied)

            if (ability->subtype == DAMAGE_LEECH)
            {
                state->health += damage ;

                if (state->health > enemy->health[gameDifficulty])
                {
                    state->health = enemy->health[gameDifficulty];
                }
            }

            PartyMember *target = &party[targetIndex];
            target->health.x -= damage;
            PushDamageMessage(enemyName, target->classData->name, ability->name, damage, 0X9c9c9c);

            if (target->health.x <= 0)
            {
                HandleDeath(memberState, target, targetIndex);
                return;
            }

            DrawHealthBar(targetIndex);
        } break;

        case DAMAGE_AOE:
        {
            int totalDamage = 0;
            for (int i = 0; i < 6; i++)
            {
                if (!memberState[i].alive) continue;

                float avoidChance = memberState[i].combatStats.avoid;
                float avoidRoll = ((float)rand() / (float)RAND_MAX) * 100.0f;
                if (avoidRoll < avoidChance) continue;

                int damage = rand() % (ability->damage[diffOffset + 1] + 1 - ability->damage[diffOffset]) + ability->damage[diffOffset];

                damage *= memberState[i].combatStats.damageTaken;
                totalDamage += damage;

                PartyMember *target = &party[i];
                target->health.x -= damage;

                if (target->health.x <= 0)
                {
                    HandleDeath(memberState, target, i);
                    continue;
                }

                DrawHealthBar(i);
            }

            PushDamageMessage(enemyName, "the party", ability->name, totalDamage, 0X9c9c9c);
        } break;
    }
}


static void AddAllyOverTimeEffect(MemberState *ownerState, MemberState *recieverState, OverTimeEffect effect) 
{
    // Check if we need to grow the array.
    if (ownerState->otCount >= ownerState->otCapacity)
    {
        size_t newCapacity = ownerState->otCapacity * 2;
        OverTimeEffect *newEffects = realloc(ownerState->otEffects, newCapacity * sizeof(OverTimeEffect));
        if (!newEffects) 
        {
            PushMessage("over-time effect realloc failure", 0XFF0000);
            return;
        }
        ownerState->otEffects = newEffects;
        ownerState->otCapacity = newCapacity;
    }

    ownerState->otEffects[ownerState->otCount] = effect;
    ownerState->otCount++;
    recieverState->otRecieved++;
}


static void RemoveAllyEffect(MemberState *ownerState, MemberState *recieverState, size_t i)
{
    for (size_t j = i; j < ownerState->otCount - 1; j++)
    {
        ownerState->otEffects[j] = ownerState->otEffects[j + 1];
    }
    ownerState->otCount--;
    recieverState->otRecieved--;
}


static void AddEnemyOverTimeEffect(MemberState *ownerState, EnemyState *recieverState, OverTimeEffect effect)
{
    if (ownerState->otCount >= ownerState->otCapacity)
    {
        size_t newCapacity = ownerState->otCapacity * 2;
        OverTimeEffect *newEffects = realloc(ownerState->otEffects, newCapacity * sizeof(OverTimeEffect));
        if (!newEffects)
        {
            PushMessage("over-time effect realloc failure", 0XFF0000);
            return;
        }
        ownerState->otEffects = newEffects;
        ownerState->otCapacity = newCapacity;
    }

    ownerState->otEffects[ownerState->otCount] = effect;
    ownerState->otCount++;
    recieverState->otRecieved++;
}

static void RemoveEnemyEffect(MemberState *ownerState, EnemyState *recieverState, size_t i)
{
    for (size_t j = i; j < ownerState->otCount - 1; j++)
    {
        ownerState->otEffects[j] = ownerState->otEffects[j + 1];
    }
    ownerState->otCount--;
    recieverState->otRecieved--;
}


bool TickOverTimeEffects(MemberState *state, MemberState *partyState, EnemyState *enemyState, const int index)
{
    bool updateUILayout = false;

    for (size_t i = 0; i < state->otCount; )
    {
        OverTimeEffect *effect = &state->otEffects[i];

        // Check if the target is dead.
        if (effect->type == ABILITY_DAMAGE && !enemyState[effect->target].alive)
        {
            RemoveEnemyEffect(state, &enemyState[effect->target], i);
            updateUILayout = true;
            continue;
        }
        else if (effect->type != ABILITY_DAMAGE && party[effect->target].health.x == 0)
        {
            RemoveAllyEffect(state, &partyState[effect->target], i);
            updateUILayout = true;
            continue;
        }

        // Process the effect based on type.
        switch (effect->type)
        {
            case ABILITY_HEAL:
            {
                int healTick = DoHealTick(effect->amount, effect->target, state);
                GiveExp(&party[index], effect->amount);
            } break;

            case ABILITY_DAMAGE:
            {
                PartyMember *member = &party[index];
                if (member->classData->specs[member->specIndex].secondaryType == SECONDARY_SPIRIT)
                {
                    member->secondary.x += 2;
                    if (member->secondary.x > member->secondary.y)
                        member->secondary.x = member->secondary.y;
                }
                DrawSecondaryProgressBar(index);
                int dmgTick = DoDamageTick(effect->amount, &enemyState[effect->target], &partyState[index], effect->target);
                GiveExp(member, dmgTick);
            } break;
        }

        // Decrement the duration.
        effect->duration--;

        // Check for expiry.
        if (effect->duration <= 0)
        {
            switch (effect->type)
            {
                case BUFF_DR:
                {
                    partyState[effect->target].combatStats.damageTaken += effect->amount;
                } break;
                
                case BUFF_DAMAGE:
                {
                    partyState[effect->target].combatStats.damageMulti -= effect->amount;
                } break;  
            }

            updateUILayout = true;

            // Remove based on effect type.
            if (effect->type == ABILITY_DAMAGE)
            {
                RemoveEnemyEffect(state, &enemyState[effect->target], i);
            }
            else
            {
                RemoveAllyEffect(state, &partyState[effect->target], i);
            }
            continue; // Don't increment i; next effect has shifted into this slot.
        }
        UpdateAllyStatusIcon(effect);
        i++;
    }

    return updateUILayout;
}


void DrawAllyStatusIcons(MemberState *state)
{
    for (size_t i = 0; i < state->otCount; i++)
    {
        const OverTimeEffect *otEffect = &state->otEffects[i];

        if (otEffect->type == ABILITY_DAMAGE) continue;

        const int sX = 960 + (14 * otEffect->uiLocation.x);
        const int sY = (27 + (14 * otEffect->uiLocation.y) + (67 * otEffect->target));

        DrawColourRectangle(sX, sY, 12, 12, otEffect->borderCol);
        DrawColourRectangle(sX + 1, sY + 1, 10, 10, otEffect->iconCol);

        char durationLeft[2] = { 0 };
        sprintf_s(durationLeft, sizeof(durationLeft), "%i", otEffect->duration);
        DrawString(durationLeft, sX + 3, sY + 2, 0XFFFFFF, 1);
    }
}

void DrawEnemyStatusIcons(MemberState *state, const int enemyIndex)
{
    for (size_t i = 0; i < state->otCount; i++)
    {
        const OverTimeEffect *otEffect = &state->otEffects[i];
        
        if (otEffect->type != ABILITY_DAMAGE) continue;

        int x, y;
        GetEnemyPositionAbil(otEffect->target, &x, &y);
        
        const int sX = x + (14 * otEffect->uiLocation.x) + 4;
        const int sY = y + 158;

        DrawColourRectangle(sX, sY, 12, 12, otEffect->borderCol);
        DrawColourRectangle(sX + 1, sY + 1, 10, 10, otEffect->iconCol);

        char durationLeft[2] = { 0 };
        sprintf_s(durationLeft, sizeof(durationLeft), "%i", otEffect->duration);
        DrawString(durationLeft, sX + 3, sY + 2, 0XFFFFFF, 1);
    }
}

static void UpdateAllyStatusIcon(OverTimeEffect *otEffect)
{
    if (otEffect->type != ABILITY_DAMAGE)
    {
        const int sX = 960 + (14 * otEffect->uiLocation.x);
        const int sY = (27 + (14 * otEffect->uiLocation.y) + (67 * otEffect->target));

        DrawColourRectangle(sX, sY, 12, 12, otEffect->borderCol);
        DrawColourRectangle(sX + 1, sY + 1, 10, 10, otEffect->iconCol);

        char durationLeft[2] = { 0 };
        sprintf_s(durationLeft, sizeof(durationLeft), "%i", otEffect->duration);
        DrawString(durationLeft, sX + 3, sY + 2, 0XFFFFFF, 1);
    }
    else if (otEffect->type == ABILITY_DAMAGE)
    {
        int x, y;
        GetEnemyPositionAbil(otEffect->target, &x, &y);

        const int sX = x + (14 * otEffect->uiLocation.x) + 4;
        const int sY = y + 158;

        DrawColourRectangle(sX, sY, 12, 12, otEffect->borderCol);
        DrawColourRectangle(sX + 1, sY + 1, 10, 10, otEffect->iconCol);

        char durationLeft[2] = { 0 };
        sprintf_s(durationLeft, sizeof(durationLeft), "%i", otEffect->duration);
        DrawString(durationLeft, sX + 3, sY + 2, 0XFFFFFF, 1);
    }
}



void UpdateCombatMeters(MemberState partyState[6], const int cycle)
{
    const int sXLeft = 584;
    const int sXRight = sXLeft + 218;
    const int sY = 436;
    const int maxBarWidth = 206;

    // clear meter areas
    DrawGreyscaleRectangle(sXLeft, sY, maxBarWidth, 84, 0); 
    DrawGreyscaleRectangle(sXRight, sY, maxBarWidth, 84, 0);

    int totalDamage = 0, totalHealing = 0;
    for (int i = 0; i < 6; i++)
    {
        totalDamage += partyState[i].damageDone;
        totalHealing += partyState[i].healingDone;
    }

    // Damage meter -- left column
    MeterEntry damageEntries[6];
    for (int j = 0; j < 6; j++)
    {
        damageEntries[j].state = &partyState[j];
        damageEntries[j].originalIndex = j;
    }

    qsort(damageEntries, 6, sizeof(MeterEntry), CompareDamageEntry);

    const float maxDpc = ((float)damageEntries[0].state->damageDone / (float)cycle);

    char dmgline[60];
    char dmgStr[16];
    char dpcStr[16];
    
    for (int k = 0; k < 6; k++) 
    {
        MemberState *ms = damageEntries[k].state;
        int damage = ms->damageDone;
        if (!damage) continue; // return if 0 damage, as the list is sorted, all subsequent entries will also be 0

        const float dpc = ((float)damage / cycle);
        const float percent = ((float)damage / totalDamage) * 100.0f;
        const int partyIndex = damageEntries[k].originalIndex;
        const char *name = party[partyIndex].classData->name;

        if (damage >= 1000)
        {
            snprintf(dmgStr, sizeof(dmgStr), "%1.1fk", damage / 1000.0f);
        }
        else
        {
            snprintf(dmgStr, sizeof(dmgStr), "%d", damage);
        }

        if (dpc >= 1000)
        {
            snprintf(dpcStr, sizeof(dpcStr), "%1.1fk", dpc / 1000.0f);
        }
        else
        {
            snprintf(dpcStr, sizeof(dpcStr), "%d", (int)dpc);
        }

        snprintf(dmgline, sizeof(dmgline),
            "%d. %-3.3s %s (%s, %1.1f%%)", k + 1, name, dmgStr, dpcStr, percent);

        int scaledWidth = (int)((dpc / maxDpc) * maxBarWidth);
        const uint32_t barColour = DarkenColour(party[partyIndex].classData->colour, 0.7f);
        DrawColourRectangle(sXLeft, sY + (14 * k), scaledWidth, 14, barColour);
        DrawString(dmgline, sXLeft + 4, sY + 3 + (14 * k), 0XFFFFFF, 1);
    }

    // Healing meter -- right column

    MeterEntry healingEntries[6];
    for (int l = 0; l < 6; l++)
    {
        healingEntries[l].state = &partyState[l];
        healingEntries[l].originalIndex = l;
    }

    qsort(healingEntries, 6, sizeof(MeterEntry), CompareHealingEntry);

    float maxHpc = ((float)healingEntries[0].state->healingDone / (float)cycle);

    char healLine[60];
    char healStr[16];
    char hpcStr[16];

    for (int m = 0; m < 6; m++)
    {
        MemberState *ms = healingEntries[m].state;
        int healing = ms->healingDone;
        if (!healing) return;

        float hpc = ((float)healing) / cycle;
        float percent = ((float)healing / totalHealing) * 100.0f;
        int partyIndex = healingEntries[m].originalIndex;
        const char *name = party[partyIndex].classData->name;

        if (healing >= 1000)
        {
            snprintf(healStr, sizeof(healStr), "%1.1fk", healing / 1000.0f);
        }
        else
        {
            snprintf(healStr, sizeof(healStr), "%d", healing);
        }

        if (hpc >= 1000)
        {
            snprintf(hpcStr, sizeof(hpcStr), "%1.1fk", hpc / 1000.0f);
        }
        else
        {
            snprintf(hpcStr, sizeof(hpcStr), "%d", (int)hpc);
        }

        snprintf(healLine, sizeof(healLine), 
            "%d. %-3.3s %s (%s, %1.1f%%)", m + 1, name, healStr, hpcStr, percent);

        int scaledWidth = (int)((hpc / maxHpc) * maxBarWidth);
        const uint32_t healBarColour = DarkenColour(party[partyIndex].classData->colour, 0.7f);
        DrawColourRectangle(sXRight, sY + (14 * m), scaledWidth, 14, healBarColour);
        DrawString(healLine, sXRight + 4, sY + 3 + (14 * m), 0xFFFFFF, 1);
    }
}



static int CompareDamageEntry(const void *a, const void *b)
{
    const MeterEntry *entryA = (const MeterEntry *)a;
    const MeterEntry *entryB = (const MeterEntry *)b;
    return entryB->state->damageDone - entryA->state->damageDone;
}

static int CompareHealingEntry(const void *a, const void *b) 
{
    const MeterEntry *entryA = (const MeterEntry *)a;
    const MeterEntry *entryB = (const MeterEntry *)b;
    return entryB->state->healingDone - entryA->state->healingDone;
}

static int CompareHealTargets(const void *a, const void *b)
{
    const HealTarget *targetA = (const HealTarget *)a;
    const HealTarget *targetB = (const HealTarget *)b;
    return targetA->member->health.x - targetB->member->health.x;
}


bool CanUseAbility(PartyMember *member, MemberState *state, Ability *ability, const int index)
{
    const Spec *spec = &member->classData->specs[member->specIndex];

    bool levelReqMet = member->level > index;
    bool costReqMet = ability->cost <= member->secondary.x;
    bool notOnCD = state->cooldownState[index] == 0;
   
    if (levelReqMet && costReqMet && notOnCD)
    {
        return true;
    }
    else 
    {
        return false;
    }
}


static void PushDamageMessage(const char *attackerName, 
                              const char *targetName, 
                              const char *abilityName, 
                              const int damage,
                              const uint32_t colour)
{
    char dmgMessage[70] = { 0 };
    sprintf_s(dmgMessage, sizeof(dmgMessage),
        "%s hits %s with %s for %i dmg", attackerName, targetName, abilityName, damage);

    PushMessage(dmgMessage, colour);
}

static void PushHealMessage(const char *healerName,
                            const char *targetName,
                            const char *abilityName,
                            const int healing,
                            const uint32_t colour)
{
    char healMessage[70] = { 0 };
    sprintf_s(healMessage, sizeof(healMessage),
        "%s heals %s with %s for %i", healerName, targetName, abilityName, healing);

    PushMessage(healMessage, colour);
}

static void PushAvoidMessage(const char *attackerName,
                             const char *targetName,
                             const uint32_t colour)
{
    char avoidMessage[70] = { 0 };
    sprintf_s(avoidMessage, sizeof(avoidMessage),
        "%s avoids an attack by %s", targetName, attackerName);

    PushMessage(avoidMessage, colour);
}

static void PushDeathMessage(const char *name)
{
    char deathMessage[70] = { 0 };
    sprintf_s(deathMessage, sizeof(deathMessage), "%s has been killed", name);
    PushMessage(deathMessage, 0XFF0000);
}


static void GetEnemyPositionAbil(const int enemyIndex, int *x, int *y)
{
    const int centerX = 222;
    const int singleRowY = 100;  // Y position for single-row layouts.
    const int topRowY = 26;      // Y position for the top row in two-row layouts.
    const int bottomRowY = 190;  // Y position for the bottom row in two-row layouts.
    const int spacing = 162;     // X spacing between enemy sprites.

    if (numEnemiesAbil <= 3)
    {
        // Arrange in one row, centered horizontally.
        *y = singleRowY;
        if (numEnemiesAbil == 1)
        {
            *x = centerX;
        }
        else if (numEnemiesAbil == 2)
        {
            // For two enemies, leave a gap in the center.
            if (enemyIndex == 0)
            {
                *x = centerX - spacing / 2;
            }
            else
            {
                *x = centerX + spacing / 2;
            }
        }
        else if (numEnemiesAbil == 3)
        {
            // For three, position one at the center and one on each side.
            if (enemyIndex == 0)
            {
                *x = centerX - spacing;
            }
            else if (enemyIndex == 1)
            {
                *x = centerX;
            }
            else
            {
                *x = centerX + spacing;
            }
        }
    }
    else
    {
        // For more than 3 enemies, split into two rows.
        int topCount = (numEnemiesAbil + 1) / 2;
        int bottomCount = numEnemiesAbil / 2;
        if (enemyIndex < topCount)
        {
            *y = topRowY;
            int centerIndex = (topCount - 1) / 2;
            *x = centerX + (enemyIndex - centerIndex) * spacing;
            if (topCount % 2 == 0)
            {
                *x -= spacing / 2;
            }
        }
        else
        {
            // Bottom row.
            *y = bottomRowY;
            int rowIndex = enemyIndex - topCount;
            int centerIndex = (bottomCount - 1) / 2;
            *x = centerX + (rowIndex - centerIndex) * spacing;
            if (bottomCount % 2 == 0)
            {
                *x -= spacing / 2;
            }
        }
    }
}
































/// OLD
//static void DoPartyDamageAbility(const Ability *ability, EnemyState *state)
//{
//    float randomScale = 0.95f + ((float)rand() / RAND_MAX) * 0.1f;
//    int damage = (int)(ability->damage * randomScale);
//    state->health -= damage;
//
//    if (state->health <= 0)
//    {
//        state->health = 0;
//        state->alive = false;
//        state->cooldownState[0] = -1;
//        state->cooldownState[1] = -1;
//    }
//}
//
//
//void DoPartyAbility(const PartyMember *partyMember, MemberState *state, EnemyState *enemyState, int abilityIdx)
//{
//    const Ability *ability = &partyMember->classData->specs[partyMember->specIndex].abilities[abilityIdx];
//
//    // put on cooldown
//    state->cooldownState[abilityIdx] = ability->cooldown;
//    
//    switch (ability->type)
//    {
//        case ABILITY_DAMAGE:
//        {
//            DoPartyDamageAbility(ability, enemyState);
//        }
//    }
//
//}
//
//void DoPartyHeal(PartyMember *caster, MemberState *state, int targetIdx, int abilityIdx) 
//{
//    const Ability *ability = &caster->classData->specs[caster->specIndex].abilities[abilityIdx];
//    state->cooldownState[abilityIdx] = ability->cooldown;
//    caster->secondary.x -= ability->cost;
//
//    float randomScale = 0.95f + ((float)rand() / RAND_MAX) * 0.1f;
//    int heal = (int)(ability->damage * randomScale);
//    PartyMember *target = &party[targetIdx];
//    target->health.x += heal;
//
//    if (target->health.x > target->health.y)
//    {
//        target->health.x = target->health.y;
//    }
//
//    DrawHealthBar(targetIdx);
//}



//void DoEnemyAbility(const Enemy *enemy, EnemyState *state)
//{
//	// select first availiable ability not on CD (they are stored in prio order)
//	volatile int abilityIdx = 0;
//	for (volatile int i = 0; i < 3; i++)
//	{
//		if (state->cooldownState[i] == 0)
//		{
//			abilityIdx = i;
//			break;
//		}
//	}
//
//	// put on cooldown
//	state->cooldownState[abilityIdx] = enemy->abilities[abilityIdx].cooldown;
//
//	switch (enemy->abilities[abilityIdx].type)
//	{
//		case ABILITY_DAMAGE:
//		{
//			DoDamageAbility(&enemy->abilities[abilityIdx]);
//		} break;
//	}
//}