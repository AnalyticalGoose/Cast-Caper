#include "party.h"
#include "class.h"

PartyMember party[6] = {
    {
        .classData = &classes[0],
        .specIndex = 0,
        .health = { 400, 400 },
        .secondary = { 0, 10 },
        .level = 1,
        .xp = { 0, 100 },
    },
    {
        .classData = &classes[1],
        .specIndex = 1,
        .health = { 200, 200 },
        .secondary = { 10, 10 },
        .level = 1,
        .xp = { 0, 100 },
    },
    {
        .classData = &classes[2], 
        .specIndex = 2 ,
        .health = { 200, 200 },
        .secondary = { 0, 5 },
        .level = 1,
        .xp = { 0, 100 },
    },
    {
        .classData = &classes[3], 
        .specIndex = 1,
        .health = { 200, 200 },
        .secondary = { 0, 10 },
        .level = 1,
        .xp = { 0, 100 },
    },
    {
        .classData = &classes[4], 
        .specIndex = 0,
        .health = { 200, 200 },
        .secondary = { 0, 10 },
        .level = 1,
        .xp = { 0, 100 },
    },
    {
        .classData = &classes[5], 
        .specIndex = 1,
        .health = { 200, 200 },
        .secondary = { 10, 10 },
        .level = 1,
        .xp = { 0, 100 },
    },
};

int mapIndex = 0;
bool encountersEnabled = false;
bool inSafezone = true;
struct xy pos = { 3, 2 };
int direction = 0;




void EquipItem(PartyMember *member, Item *item)
{
    switch (item->slot)
    {
        case SLOT_HEAD:
        {
            member->gear.items[0] = *item;
        } break;

        case SLOT_SHOULDER:
        {
            member->gear.items[1] = *item;
        } break;

        case SLOT_CHEST:
        {
            member->gear.items[2] = *item;
        } break;

        case SLOT_HANDS:
        {
            member->gear.items[3] = *item;
        } break;

        case SLOT_LEGS:
        {
            member->gear.items[4] = *item;
        } break;
        
        case SLOT_FEET:
        {
            member->gear.items[5] = *item;
        } break;

        case SLOT_NECK:
        {
            member->gear.items[6] = *item;
        } break;

        case SLOT_RING:
        {
            member->gear.items[7] = *item;
        } break;

        case SLOT_WEAPON:
        {
            member->gear.items[8] = *item;
        } break;
    }

    RecalculateMemberStats(member);
}


void RecalculateMemberStats(PartyMember *member)
{
    int main = 0, crit = 0, mastery = 0, haste = 0, res = 0, avoid = 0, leech = 0;

    for (int i = 0; i < 9; i++) {
        ItemStats itemStats = member->gear.items[i].stats;
        main += itemStats.mainStat;
        crit += itemStats.crit;
        mastery += itemStats.mastery;
        haste += itemStats.haste;
        res += itemStats.res;
        avoid += itemStats.avoid;
        leech += itemStats.leech;
    }

    member->stats.mainStat = main;
    member->stats.crit = crit;
    member->stats.mastery = mastery;
    member->stats.haste = haste;
    member->stats.res = res;
    member->stats.avoid = avoid;
    member->stats.leech = leech;
}


void GenerateAndEquipItemSet(PartyMember *member,
                             const int ilvl,
                             const Rarity rarity,
                             const char *base)
{
    const MainStat mainStat = member->classData->specs[member->specIndex].mainStat;

    for (int i = 0; i < 9; i++)
    {
        Item item = GenerateItem(mainStat, (Slot)i, ilvl, rarity, base);
        EquipItem(member, &item);
    }
}