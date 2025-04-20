#ifndef DISENCHANT_H
#define DISENCHANT_H

#include <stdbool.h>

typedef struct {
    float progress;
    float requiredTime;
    int rarity;
    int ilvl;
    bool active;
} Job;

typedef struct {
    Job jobs[10];
    float speedMultiplier;
    float valueMultiplier;
    int craftIlvl;
    int craftRarity;
    struct {
        int normal, magic, rare, epic;
    } resources;
    int maxJobs;

    int jobSlotUpgradeLvl;
    int jobSpeedUpgradeLvl;
    int jobValueUpgradeLvl;
    int ilvlUpgradeLvl;
    int rarityUpgradeLvl;
} DisenchantState;

extern bool needDisenchantUIRedraw;

void InitDisenchant(void);
bool AddDisenchantJob(const int ilvl, const int rarity);
void UpdateDisenchant(float deltaTime);
bool TryUpgrade(DisenchantState *state, const int upgradeType);

DisenchantState *GetDisenchantState(void);

#endif

