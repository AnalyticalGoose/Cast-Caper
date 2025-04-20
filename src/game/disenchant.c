#include "disenchant.h"
#include <string.h>

static DisenchantState disenchantState;  
bool needDisenchantUIRedraw = true;

void InitDisenchant(void)
{
    memset(&disenchantState, 0, sizeof disenchantState);
    disenchantState.maxJobs = 1;
    disenchantState.speedMultiplier = 1.0f;
    disenchantState.valueMultiplier = 1;
    disenchantState.craftIlvl = 4;
    disenchantState.craftRarity = 0;

    disenchantState.jobSlotUpgradeLvl = 1;
    disenchantState.jobSpeedUpgradeLvl = 1;
    disenchantState.jobValueUpgradeLvl = 1;
    disenchantState.ilvlUpgradeLvl = 1;
    disenchantState.rarityUpgradeLvl = 1;

    disenchantState.resources.normal = 1000;
    disenchantState.resources.magic = 1000;
    disenchantState.resources.rare = 1000;
    disenchantState.resources.epic = 1000;
}

DisenchantState *GetDisenchantState(void)
{
    return &disenchantState;
}

static float ComputeRequiredTime(const int ilvl, const int rarity, const float speedMulti) 
{
    return (ilvl * 6.0f * (rarity + 1) / speedMulti);
}

bool AddDisenchantJob(const int ilvl, const int rarity)
{
    DisenchantState *state = GetDisenchantState();

    for (size_t i = 0; i < state->maxJobs; i++)
    {
        Job *job = &state->jobs[i];
        if (!job->active)
        {
            job->ilvl = ilvl;
            job->rarity = rarity;
            job->requiredTime = ComputeRequiredTime(ilvl, rarity, state->speedMultiplier);
            job->progress = 0.0f;
            job->active = true;
            
            needDisenchantUIRedraw = true;
            return true;
        }
    }
    return false;
}

void UpdateDisenchant(float deltaTime)
{
    DisenchantState *state = GetDisenchantState();

    for (int i = 0; i < state->maxJobs; i++)
    {
        Job *job = &state->jobs[i];
        
        if (!job->active) continue;
        
        job->progress += deltaTime;
        
        if (job->progress >= job->requiredTime)
        {
            const int reward = (int)(job->ilvl / 4) * state->valueMultiplier;
            
            switch (job->rarity)
            {
                case 0: 
                {
                    state->resources.normal += reward;
                } break;

                case 1:
                {
                    state->resources.magic += reward;
                } break;

                case 2:
                {
                    state->resources.rare += reward;
                } break;

                case 3: 
                {
                    state->resources.epic += reward;
                } break;
            }

            job->active = false;
            needDisenchantUIRedraw = true;
        }
    }
}


bool TryUpgrade(DisenchantState *state, const int upgradeType)
{
    const int slotBaseCost[10] = { 0, 4, 8, 16, 24, 36, 50, 100, 150, 250 };
    int *resPool[4] = { &state->resources.normal, &state->resources.magic, &state->resources.rare, &state->resources.epic };

    switch (upgradeType)
    {
        case 0:
        {
            int rawIdx = (state->jobSlotUpgradeLvl - 1) / 2;
            int poolIndex = rawIdx < 3 ? rawIdx : 3;

            if (*resPool[poolIndex] >= slotBaseCost[state->jobSlotUpgradeLvl])
            {
                *resPool[poolIndex] -= slotBaseCost[state->jobSlotUpgradeLvl];

                state->jobSlotUpgradeLvl += 1;
                state->maxJobs += 1;
                return true;
            }
            else
            {
                return false;
            }
        } break;

        case 1:
        {
            int rawIdx = (state->jobSpeedUpgradeLvl - 1) / 2;
            int poolIndex = rawIdx < 3 ? rawIdx : 3;

            if (*resPool[poolIndex] >= slotBaseCost[state->jobSpeedUpgradeLvl])
            {
                *resPool[poolIndex] -= slotBaseCost[state->jobSpeedUpgradeLvl];

                state->jobSpeedUpgradeLvl += 1;
                state->speedMultiplier += 1.0f;
                return true;
            }
            else
            {
                return false;
            }
        } break;

        case 2:
        {
            int rawIdx = (state->jobValueUpgradeLvl - 1) / 2;
            int poolIndex = rawIdx < 3 ? rawIdx : 3;

            if (*resPool[poolIndex] >= slotBaseCost[state->jobValueUpgradeLvl])
            {
                *resPool[poolIndex] -= slotBaseCost[state->jobValueUpgradeLvl];

                state->jobValueUpgradeLvl += 1;
                state->valueMultiplier += 1.0f;
                return true;
            }
            else
            {
                return false;
            }
        } break;

        case 3:
        {
            int rawIdx = (state->ilvlUpgradeLvl - 1) / 2;
            int poolIndex = rawIdx < 3 ? rawIdx : 3;

            if (*resPool[poolIndex] >= slotBaseCost[state->ilvlUpgradeLvl])
            {
                *resPool[poolIndex] -= slotBaseCost[state->ilvlUpgradeLvl];

                state->ilvlUpgradeLvl += 1;
                state->craftIlvl += 2;
                return true;
            }
            else
            {
                return false;
            }
        } break;

        case 4:
        {
            if (*resPool[state->rarityUpgradeLvl] >= slotBaseCost[state->rarityUpgradeLvl * 2])
            {
                *resPool[state->rarityUpgradeLvl] -= slotBaseCost[state->rarityUpgradeLvl * 2];

                state->rarityUpgradeLvl += 1;
                state->craftRarity += 1;
                return true;
            }
            else
            {
                return false;
            }
        } break;
    }
}