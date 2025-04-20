#include "item.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Windows.h>


#define NUM_GEAR_TYPES (sizeof(baseGearStats) / sizeof(baseGearStats[0]))
#define NUM_QUALITIES (sizeof(qualityPrefix)/sizeof(qualityPrefix[0]))
#define NUM_SECONDARIES (sizeof(secondaries)/sizeof(secondaries[0]))
#define NUM_MAGIC_SUFFIX (sizeof(secondarySuffixMagic)/sizeof(secondarySuffixMagic[0]))
#define NUM_RARE_SUFFIX (sizeof(secondarySuffixRare)/sizeof(secondarySuffixRare[0]))
#define NUM_EPIC_SUFFIX (sizeof(secondarySuffixEpic)/sizeof(secondarySuffixEpic[0]))
#define NUM_SUFFIX_LISTS (sizeof(secondariesSuffixLists)/sizeof(secondariesSuffixLists[0]))


static void GenerateSuffixIndices(int *out, Rarity rarity);
static int BuildSuffixKey(int suffixes[], int count);
static int CompareInts(const void *a, const void *b);
static const char *lookupSuffix(Rarity rarity, int key);


//const uint32_t rarityColours[4] = { 0XFFFFFF, 0x4de302, 0x013ee1, 0x9700ff };
const uint32_t rarityColours[4] = { 0XFFFFFF, 0x4de302, 0x013ee1, 0xaa50ec };

GearStat baseGearStats[] = {
    {2, {1, 2}},  // Helmet
    {3, {2, 3}},  // Shoulder
    {4, {2, 4}},  // Chest
    {2, {1, 2}},  // Gloves
    {3, {2, 3}},  // Legs
    {2, {1, 2}},  // Boots
    {1, {2, 4}},  // Amulet
    {1, {2, 3}},  // Ring
    {5, {2, 6}},  // Weapon
};

const char *itemBases[9][3] = {
    {"Headguard ", "Cowl ", "Crown "},          // Helm
    {"Pauldrons ", "Mantle ", "Epaulets "},     // Shoulder
    {"Breastplate ", "Tunic ", "Robetop "},     // Chest
    {"Gauntlets ", "Handgrips ", "Mitts "},     // Gloves
    {"Legplates ", "Leggings ", "Robebottoms "},// Legs
    {"Sabatons ", "Footwraps ", "Slippers "},   // Boots
    {"Chain ", "Choker ", "Rosary "},           // Amulet
    {"Duster ", "Signet ", "Band "},            // Ring
    {"Battleaxe ", "Crossbow ", "Staff "}        // Weapon
};

const char *qualityPrefix[] = {
    "Inferior ",      // 1-20%
    "Poor ",          // 21-40%
    "Acceptable ",    // 41-60%
    "Fine ",          // 61-90%
    "Exceptional ",   // 91-99%
    "Perfect "        // 100%
};

const char *secondaries[] = {
    "Critical Strike",  // 0
    "Haste",            // 1
    "Mastery",          // 2
    "Resistance",       // 3
    "Avoidance",        // 4
    "Leech"             // 5
};


// Magic quality (1 secondary)
SecondarySuffix secondarySuffixMagic[] = {
    {0, "of Penetration"},  // Crit
    {1, "of Speed"},        // Haste
    {2, "of Potency"},      // Mastery
    {3, "of Warding"},      // Resistance
    {4, "of Dodging"},      // Avoidance
    {5, "of Regeneration"}  // Leech
};

// Rare quality (2 secondaries)
SecondarySuffix secondarySuffixRare[] = {
    {1, "of Agility"},      // Crit + Haste
    {2, "of Precision"},    // Crit + Mastery
    {3, "of Fortification"},// Crit + Resistance
    {4, "of Evasion"},      // Crit + Avoidance
    {5, "of Vampirism"},    // Crit + Leech
    {12, "of Ferocity"},    // Haste + Mastery
    {13, "of Resilience"},  // Haste + Resistance
    {14, "of Nimbleness"},  // Haste + Avoidance
    {15, "of Draining"},    // Haste + Leech
    {23, "of Resolve"},     // Mastery + Resistance
    {24, "of Deflection"},  // Mastery + Avoidance
    {25, "of Bloodlust"},   // Mastery + Leech
    {34, "of Safeguarding"},// Resistance + Avoidance
    {35, "of Retaliation"}, // Resistance + Leech
    {45, "of Survival"}     // Avoidance + Leech
};

// Epic quality (3 secondaries)
SecondarySuffix secondarySuffixEpic[] = {
    {12, "of Devastation"},         // Crit, Haste, Mastery
    {13, "of Alacrity"},            // Crit, Haste, Resistance
    {14, "of Swift Defiance"},      // Crit, Haste, Avoidance
    {15, "of Voracity"},            // Crit, Haste, Leech
    {23, "of Dominance"},           // Crit, Mastery, Resistance
    {24, "of Prowess"},             // Crit, Mastery, Avoidance
    {25, "of Carnage"},             // Crit, Mastery, Leech
    {34, "of Guardianship"},        // Crit, Resistance, Avoidance
    {35, "of Bloodfort"},           // Crit, Resistance, Leech
    {45, "of Shadows"},             // Crit, Avoidance, Leech
    {123, "of Unyielding Fury"},    // Haste, Mastery, Resistance
    {124, "of Relentlessness"},     // Haste, Mastery, Avoidance
    {125, "of Bloodthirst"},        // Haste, Mastery, Leech
    {134, "of Steadfastness"},      // Haste, Resistance, Avoidance
    {135, "of Vampiric Might"},     // Haste, Resistance, Leech
    {145, "of Ghostly Might"},      // Haste, Avoidance, Leech
    {234, "of Indomitability"},     // Mastery, Resistance, Avoidance
    {235, "of Vengeance"},          // Mastery, Resistance, Leech
    {245, "of Endurance"},          // Mastery, Avoidance, Leech
    {345, "of Immortality"}         // Resistance, Avoidance, Leech
};

SecondarySuffixList secondariesSuffixLists[] = {
    {1, secondarySuffixMagic, NUM_MAGIC_SUFFIX},
    {2, secondarySuffixRare, NUM_RARE_SUFFIX},
    {3, secondarySuffixEpic, NUM_EPIC_SUFFIX}
};

Item GenerateItem(const MainStat mainStat, 
                  const Slot slot, 
                  const int ilvl, 
                  const Rarity rarity, 
                  const char *base)
{   
    const int quality = rand() % 100;

    const GearStat slotWeightings = baseGearStats[(int)slot];
    
    const int mainStatWeight = slotWeightings.mainStat;
    const int mainStatVal = (mainStatWeight + (int)rarity) * ilvl;

    const int secondaryMinWeight = slotWeightings.secondaryStats[0];
    const int secondaryMaxWeight = slotWeightings.secondaryStats[1];
    const float secondaryBound = secondaryMinWeight + ((quality - 1.0f) / 99) * (secondaryMaxWeight - secondaryMinWeight);
    int secondaryVal = (int)floor(secondaryBound * ilvl);
    
    const int qualityPrefixWeights[6] = { 20, 20, 20, 30, 9, 1 };
    char *prefixStr = { NULL };
    int qualityCopy = quality;
    for (int j = 0; j < 6; j++)
    {
        if (qualityCopy < qualityPrefixWeights[j])
        {
            prefixStr = qualityPrefix[j];
            break;
        }
        else
        {
            qualityCopy -= qualityPrefixWeights[j];
        }
    }

    const char *slotStr = itemBases[slot][mainStat];
    
    int suffixes[3] = { -1, -1, -1 };
    GenerateSuffixIndices(suffixes, rarity);

    int secondaries[6] = { 0, 0, 0, 0, 0, 0 };
    for (int i = 0; i < (int)rarity; i++)
    {
        int index = suffixes[i];
        if (index < 0 || index > 5) continue;
        secondaries[index] += secondaryVal;
        secondaryVal = (int)floor(secondaryVal * 0.8);
    }

    int suffixKey = BuildSuffixKey(suffixes, (int)rarity);
    char *suffixStr = lookupSuffix((int)rarity, suffixKey);

    ItemStats itemStats =
    {
        .mainStatType = mainStat,
        .mainStat = mainStatVal,
        .crit = secondaries[0],
        .haste = secondaries[1],
        .mastery = secondaries[2],
        .res = secondaries[3],
        .avoid = secondaries[4],
        .leech = secondaries[5],
    };

    Item item =
    {
        .slot = slot,
        .rarity = rarity,
        .ilvl = ilvl,
        .stats = itemStats,
        .name = { "NULL" },
    };

    if (rarity > 0)
    {
        snprintf(item.name, sizeof(item.name), "%s%s%s%s", prefixStr, base, slotStr, suffixStr);
    }
    else
    {
        snprintf(item.name, sizeof(item.name), "%s%s%s", prefixStr, base, slotStr);
    }

    return item;
}


static void GenerateSuffixIndices(int *out, Rarity rarity)
{
    int filled = 0;
    while (filled < (int)rarity)
    {
        int val = rand() % 6;
        bool exists = false;

        for (int i = 0; i < filled; i++)
        {
            if (out[i] == val)
            {
                exists = true;
                break;
            }
        }
        if (!exists)
        {
            out[filled++] = val;
        }
    }
}

static int BuildSuffixKey(int suffixes[], int count)
{
    int temp[3] = { NULL };
    int tempCount = 0;
    for (int i = 0; i < count; i++) 
    {
        if (suffixes[i] != 0) { // Skip any zero entries.
            temp[tempCount++] = suffixes[i];
        }
    }

    if (tempCount == 0) return 0; // If all entries are zero, key is 0.

    qsort(temp, tempCount, sizeof(int), CompareInts);

    int key = 0;
    for (int i = 0; i < tempCount; i++) 
    {
        key = key * 10 + temp[i];
    }
    return key;
}

// Comparison function for qsort: compare two ints.
static int CompareInts(const void *a, const void *b)
{
    int int_a = *(const int *)a;
    int int_b = *(const int *)b;
    return (int_a - int_b);
}

static const char *lookupSuffix(Rarity rarity, int key) 
{
    for (int i = 0; i < NUM_SUFFIX_LISTS; i++) 
    {
        if (secondariesSuffixLists[i].rarity == rarity) 
        {
            for (int j = 0; j < secondariesSuffixLists[i].count; j++) 
            {
                if (secondariesSuffixLists[i].suffixes[j].key == key) 
                {
                    return secondariesSuffixLists[i].suffixes[j].suffix;
                }
            }
        }
    }
    return NULL; // not found
}


ItemGenData *BuildItemGenDataArray(const Enemy **enemies, int numEnemies)
{
    ItemGenData *lootData = malloc(numEnemies * sizeof(ItemGenData));
    if (!lootData) return NULL;

    for (size_t i = 0; i < numEnemies; i++)
    {
        const Enemy *enemy = enemies[i]; // Not &enemies[i]
        lootData[i].baseName = enemy->loot.baseName;
        lootData[i].ilvl = rand() % (enemy->loot.ilvl[1] + 1 - enemy->loot.ilvl[0]) + enemy->loot.ilvl[0];
        lootData[i].slot = (Slot)(rand() % 9);
        lootData[i].mainStat = (MainStat)(rand() % 3);

        int rarityRoll = rand() % 100;
        int rarity = 0;
        for (int j = 0; j < 4; j++)
        {
            if (rarityRoll < enemy->loot.rarity[j])
            {
                rarity = j;
                break;
            }
            else
            {
                rarityRoll -= enemy->loot.rarity[j];
            }
        }

        lootData[i].rarity = (Rarity)(rarity);
    }

    return lootData;
}