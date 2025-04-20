#include "disenchant.h"
#include "input.h"
#include "item.h"
#include "loot_state.h"
#include "movement_state.h"
#include "renderer.h"
#include "state.h"
#include "state_manager.h"
#include "ui.h"
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <Windows.h>

typedef enum
{
	LOOT_SELECT_ITEM, // scroll up and down to compare loot
	LOOT_SELECT_ACTION, // select an item and choose between equip, discard, etc
	LOOT_SELECT_EQUIP,
} LootSubState;

typedef struct LootStateData
{
	Item item[6];
	int numItems;
	int itemSelection;
	int menuSelection;
	int equipSelection;

	LootSubState subState;

	bool subStateProcessed;
	bool subStateRendered;
} LootStateData;

const char *mainStatStr[3] = { "Strength", "Agility", "Intelligence" };
const char *itemStatsStr[6] = { "Critical", "Haste", "Mastery", "Resistance", "Avoidance", "Leech" };

static void GenerateLoot(LootStateData *data, ItemGenData *itemGenData, const int numItems);
static void DrawLootIcon(LootStateData *data, const int index, const int x, const int y);
static void DrawLootItemPanel(Item *item);
static void DrawComparisonPanels(Item *newItem, Item *item1, Item *item2);

static void GetPartyIndex(int *index1, int *index2, MainStat statType);
static void RemoveItemFromLoot(LootStateData *data, int indexToRemove);


void UpdateLoot(struct GameState *state, float deltaTime)
{
	LootStateData *data = (LootStateData *)state->data;

	switch (data->subState)
	{
		case LOOT_SELECT_ITEM:
		{
			if (IsKeyJustPressed(VK_UP))
			{
				data->itemSelection = (data->itemSelection + data->numItems - 1) % data->numItems;
				data->subStateRendered = false;
			}

			if (IsKeyJustPressed(VK_DOWN))
			{
				data->itemSelection = (data->itemSelection + 1) % data->numItems;
				data->subStateRendered = false;
			}

			if (IsKeyJustPressed(VK_RETURN))
			{
				data->subState = LOOT_SELECT_ACTION;
				data->subStateRendered = false;
			}
		} break;

		case LOOT_SELECT_ACTION:
		{
			if (IsKeyJustPressed(VK_UP))
			{
				data->menuSelection = (data->menuSelection + 3 - 1) % 3;
				data->subStateRendered = false;
			}

			if (IsKeyJustPressed(VK_DOWN))
			{
				data->menuSelection = (data->menuSelection + 1) % 3;
				data->subStateRendered = false;
			}

			if (IsKeyJustPressed(VK_ESCAPE))
			{
				DrawGreyscaleRectangle(80, 260, 140, 40, 0);
				data->subState = LOOT_SELECT_ITEM;
				data->subStateRendered = false;
			}

			if (IsKeyJustPressed(VK_RETURN))
			{
				switch (data->menuSelection)
				{
					case 0:
					{
						DrawGreyscaleRectangle(80, 260, 140, 40, 0);
						data->subState = LOOT_SELECT_EQUIP;
						data->subStateRendered = false;
					} break;

					case 1:
					{
						Item *item = &data->item[data->itemSelection];
						bool addedJob = AddDisenchantJob(item->ilvl, item->rarity);

						if (addedJob)
						{
							PushMessage("Item has been sent to be disenchanted", 0X00FF00);
							RemoveItemFromLoot(data, data->itemSelection);
							DrawGreyscaleRectangle(18, 18, 546, 346, 0);
							data->menuSelection = 0;
							data->subState = LOOT_SELECT_ITEM;
							data->subStateRendered = false;
						}
						else
						{
							PushMessage("The guild is too busy for any more items right now!", 0XFF0000);
						}
					} break;

					case 2:
					{
						PushMessage("Item has been discarded", 0XDDDDDD);
						RemoveItemFromLoot(data, data->itemSelection);
						DrawGreyscaleRectangle(18, 18, 546, 346, 0);
						data->menuSelection = 0;
						data->subState = LOOT_SELECT_ITEM;
						data->subStateRendered = false;
					} break;
				}
			}
		} break;

		case LOOT_SELECT_EQUIP:
		{
			if (IsKeyJustPressed(VK_RIGHT))
			{
				data->equipSelection = (data->equipSelection + 2 - 1) % 2;
				data->subStateRendered = false;
			}

			if (IsKeyJustPressed(VK_LEFT))
			{
				data->equipSelection = (data->equipSelection + 1) % 2;
				data->subStateRendered = false;
			}
			
			if (IsKeyJustPressed(VK_ESCAPE))
			{
				DrawGreyscaleRectangle(240, 330, 140, 18, 0);
				DrawGreyscaleRectangle(400, 330, 140, 18, 0);
				data->subState = LOOT_SELECT_ACTION;
				data->subStateRendered = false;
			}

			if (IsKeyJustPressed(VK_RETURN))
			{
				Item *item = &data->item[data->itemSelection];
				int partyIndex1, partyIndex2;
				GetPartyIndex(&partyIndex1, &partyIndex2, item->stats.mainStatType);
				int targetPartyIndex = data->equipSelection == 0 ? partyIndex1 : partyIndex2;

				EquipItem(&party[targetPartyIndex], item);
				RemoveItemFromLoot(data, data->itemSelection);

				DrawGreyscaleRectangle(18, 18, 546, 346, 0);
				data->subState = LOOT_SELECT_ITEM;
				data->subStateRendered = false;
			}
		} break;
	}
}

void RenderLoot(struct GameState *state)
{
	LootStateData *data = (LootStateData *)state->data;

	switch (data->subState)
	{
		case LOOT_SELECT_ITEM:
		{
			if (!data->subStateRendered)
			{
				for (size_t i = 0; i < data->numItems; i++)
				{
					DrawLootIcon(data, i, 24, (36 * i) + 100);
				}

				Item *newItem = &data->item[data->itemSelection];

				int partyIndex1, partyIndex2;
				GetPartyIndex(&partyIndex1, &partyIndex2, newItem->stats.mainStatType);

				DrawGreyscaleRectangle(284, 36, 216, 64, 0);
				DrawSprite(280, 36, 64, 64, &party[partyIndex1].classData->sprite, &party[partyIndex1].classData->specs[party[partyIndex1].specIndex].palette);
				DrawSprite(440, 36, 64, 64, &party[partyIndex2].classData->sprite, &party[partyIndex2].classData->specs[party[partyIndex2].specIndex].palette);

				Item *oldItem1 = &party[partyIndex1].gear.items[(int)data->item[data->itemSelection].slot];
				Item *oldItem2 = &party[partyIndex2].gear.items[(int)data->item[data->itemSelection].slot];

				DrawLootItemPanel(newItem);
				DrawComparisonPanels(newItem, oldItem1, oldItem2);

				DrawString("[ENTER] Select", 91, 260, 0xe4bb00, 1);

				data->subStateRendered = true;
			}
		} break;

		case LOOT_SELECT_ACTION:
		{
			if (!data->subStateRendered)
			{
				DrawColourRectangle(80, 260, 140, 18, data->menuSelection == 0 ? 0XFFFFFF : 0X780000);
				DrawColourRectangle(81, 261, 138, 16, 0XBD0000);
				DrawStringCentered("Equip", 80, 220, 265, 0XFFFFFF, 1);

				DrawColourRectangle(80, 280, 140, 18, data->menuSelection == 1 ? 0XFFFFFF : 0X780000);
				DrawColourRectangle(81, 281, 138, 16, 0XBD0000);
				DrawStringCentered("Send to guild", 80, 220, 285, 0XFFFFFF, 1);

				DrawColourRectangle(80, 300, 140, 18, data->menuSelection == 2 ? 0XFFFFFF : 0X780000);
				DrawColourRectangle(81, 301, 138, 16, 0XBD0000);
				DrawStringCentered("Discard", 80, 220, 305, 0XFFFFFF, 1);

				data->subStateRendered = true;
			}
		} break;

		case LOOT_SELECT_EQUIP:
		{
			if (!data->subStateRendered)
			{
				DrawColourRectangle(240, 330, 140, 18, data->equipSelection == 0 ? 0XFFFFFF : 0X780000);
				DrawColourRectangle(241, 331, 138, 16, 0XBD0000);
				DrawStringCentered("Equip", 240, 380, 335, 0XFFFFFF, 1);

				DrawColourRectangle(400, 330, 140, 18, data->equipSelection == 1 ? 0XFFFFFF : 0X780000);
				DrawColourRectangle(401, 331, 138, 16, 0XBD0000);
				DrawStringCentered("Equip", 400, 540, 335, 0XFFFFFF, 1);

				data->subStateRendered = true;
			}
		}
	}
}

void DestroyLoot(struct GameState *state)
{
	if (state)
	{
		if (state->data)
		{
			free(state->data);
		}
		free(state);
	}
}

struct GameState *CreateLootState(ItemGenData *itemGenData, const int numItems)
{
	GameState *state = malloc(sizeof(GameState));
	if (!state)
	{
		return NULL;
	}

	LootStateData *data = malloc(sizeof(LootStateData));
	if (!data)
	{
		free(state);
		return NULL;
	}

	data->numItems = numItems;
	data->itemSelection = 0;
	data->menuSelection = 0;
	data->equipSelection = 0;
	data->subState = LOOT_SELECT_ITEM;
	data->subStateProcessed = false;
	data->subStateRendered = false;

	GenerateLoot(data, itemGenData, numItems);
	free(itemGenData);

	needDisenchantUIRedraw = true;

	state->data = data;
	state->update = UpdateLoot;
	state->render = RenderLoot;
	state->destroy = DestroyLoot;

	return state;
}


static void GenerateLoot(LootStateData *data, ItemGenData *itemGenData, const int numItems)
{
	for (size_t i = 0; i < numItems; i++)
	{
		ItemGenData *itemData = &itemGenData[i];
		Item item = GenerateItem(itemData->mainStat, itemData->slot, itemData->ilvl, itemData->rarity, itemData->baseName);
		data->item[i] = item;
	}
}

static void DrawLootIcon(LootStateData *data, const int index, const int x, const int y)
{
	const unsigned char testBitmap[8] = { 0X18, 0x66, 0xDB, 0x7E, 0x7E, 0x5A, 0x24, 0X18 };
	const uint32_t rarityCol = rarityColours[(int)data->item[index].rarity];

	DrawColourRectangle(x, y, 32, 32, data->itemSelection == index ? 0xFF0000 : rarityCol);
	DrawGreyscaleRectangle(x + 2, y + 2, 28, 28, 0);
	DrawBitMap(testBitmap, x + 4, y + 4, rarityCol, 3);
}

static void DrawLootItemPanel(Item *item)
{
	char statStr[50] = { 0 };
	const int itemStats[6] = { item->stats.crit, item->stats.haste, item->stats.mastery, item->stats.res, item->stats.avoid, item->stats.leech };

	// clear space
	DrawGreyscaleRectangle(80, 100, 140, 140, 0);

	// draw border
	DrawGreyscaleRectangle(80, 100, 140, 102 + (10 * (int)item->rarity), 0XDDDDDD);
	DrawGreyscaleRectangle(82, 102, 136, 98 + (10 * (int)item->rarity), 0);

	// item name
	DrawStringWrapped(item->name, 86, 106, 132, 0, rarityColours[(int)item->rarity], 1);

	// ilvl
	sprintf_s(statStr, sizeof(statStr), "Item Level: %i", item->ilvl);
	DrawString(statStr, 86, 156, 0xe4bb00, 1);

	// main stat
	sprintf_s(statStr, sizeof(statStr), "*%i %s", item->stats.mainStat, mainStatStr[(int)item->stats.mainStatType]);
	DrawString(statStr, 86, 176, 0xDDDDDD, 1);

	// other stats
	int count = 0;
	for (int i = 0; i < 6; i++)
	{
		if (itemStats[i] != 0)
		{
			sprintf_s(statStr, sizeof(statStr), "*%i %s", itemStats[i], itemStatsStr[i]);
			DrawString(statStr, 86, 196 + (10 * count), 0x4de302, 1);
			count++;
		}
	}
}

static void DrawComparisonPanels(Item *newItem, Item *item1, Item *item2)
{
	char statStr[50] = { 0 };
	const int newItemStats[6] = { newItem->stats.crit, newItem->stats.haste, newItem->stats.mastery, newItem->stats.res, newItem->stats.avoid, newItem->stats.leech };
	const int itemStats1[6] = { item1->stats.crit, item1->stats.haste, item1->stats.mastery, item1->stats.res, item1->stats.avoid, item1->stats.leech };
	const int itemStats2[6] = { item2->stats.crit, item2->stats.haste, item2->stats.mastery, item2->stats.res, item2->stats.avoid, item2->stats.leech };

	// base panels
	DrawGreyscaleRectangle(240, 100, 140, 230, 0);
	DrawGreyscaleRectangle(240, 100, 140, 194 + (10 * (int)item1->rarity), 0XDDDDDD);
	DrawGreyscaleRectangle(242, 102, 136, 190 + (10 * (int)item1->rarity), 0);
	DrawGreyscaleRectangle(400, 100, 140, 232, 0);
	DrawGreyscaleRectangle(400, 100, 140, 194 + (10 * (int)item2->rarity), 0XDDDDDD);
	DrawGreyscaleRectangle(402, 102, 136, 190 + (10 * (int)item2->rarity), 0);

	// item name
	DrawStringWrapped(item1->name, 246, 106, 132, 0, rarityColours[(int)item1->rarity], 1);
	DrawStringWrapped(item2->name, 406, 106, 132, 0, rarityColours[(int)item2->rarity], 1);

	// ilvl
	sprintf_s(statStr, sizeof(statStr), "Item Level: %i", item1->ilvl);
	DrawString(statStr, 246, 156, 0xe4bb00, 1);
	sprintf_s(statStr, sizeof(statStr), "Item Level: %i", item2->ilvl);
	DrawString(statStr, 406, 156, 0xe4bb00, 1);

	// main stat
	sprintf_s(statStr, sizeof(statStr), "*%i %s", item1->stats.mainStat, mainStatStr[(int)item1->stats.mainStatType]);
	DrawString(statStr, 246, 176, 0xDDDDDD, 1);
	sprintf_s(statStr, sizeof(statStr), "*%i %s", item1->stats.mainStat, mainStatStr[(int)item2->stats.mainStatType]);
	DrawString(statStr, 406, 176, 0xDDDDDD, 1);

	// other stats
	int count1 = 0;
	int count2 = 0;
	for (int i = 0; i < 6; i++)
	{
		if (itemStats1[i] != 0)
		{
			sprintf_s(statStr, sizeof(statStr), "*%i %s", itemStats1[i], itemStatsStr[i]);
			DrawString(statStr, 246, 196 + (10 * count1), 0x4de302, 1);
			count1++;
		}
		if (itemStats2[i] != 0)
		{
			sprintf_s(statStr, sizeof(statStr), "*%i %s", itemStats2[i], itemStatsStr[i]);
			DrawString(statStr, 406, 196 + (10 * count2), 0x4de302, 1);
			count2++;
		}
	}

	DrawString("If replaced:", 246, 206 + (10 * count1), 0xe4bb00, 1);
	DrawString("If replaced:", 406, 206 + (10 * count2), 0xe4bb00, 1);

	int newMain = newItem->stats.mainStat;
	int oldMain1 = item1->stats.mainStat;
	int diff1 = newMain - oldMain1;
	if (!(diff1 == 0 || (newMain == 0 && oldMain1 == 0))) 
	{
		uint32_t color = (diff1 > 0) ? 0x00FF00 : 0xFF0000;
		sprintf_s(statStr, sizeof(statStr), diff1 > 0 ? "*%i %s" : "%i %s", diff1, mainStatStr[(int)newItem->stats.mainStatType]);
		DrawString(statStr, 246, 220 + (10 * count1), color, 1);
		count1++;
	}

	int oldMain2 = item2->stats.mainStat;
	int diff2 = newMain - oldMain2;
	if (!(diff2 == 0 || (newMain == 0 && oldMain1 == 0)))
	{
		uint32_t color = (diff2 > 0) ? 0x00FF00 : 0xFF0000;
		sprintf_s(statStr, sizeof(statStr), diff2 > 0 ? "*%i %s" : "%i %s", diff2, mainStatStr[(int)newItem->stats.mainStatType]);
		DrawString(statStr, 406, 220 + (10 * count2), color, 1);
		count2++;
	}

	int count3 = 0;
	int count4 = 0;
	for (int j = 0; j < 6; j++)
	{
		int diff1 = newItemStats[j] - itemStats1[j];
		if (!(diff1 == 0 || (newMain == 0 && oldMain1 == 0)))
		{
			uint32_t color = (diff1 > 0) ? 0x00FF00 : 0xFF0000;
			sprintf_s(statStr, sizeof(statStr), diff1 > 0 ? "*%i %s" : "%i %s", diff1, itemStatsStr[j]);
			DrawString(statStr, 246, 220 + (10 * (count1 + count3)), color, 1);
			count3++;
		}

		int diff2 = newItemStats[j] - itemStats2[j];
		if (!(diff2 == 0 || (newMain == 0 && oldMain2 == 0)))
		{
			uint32_t color = (diff2 > 0) ? 0x00FF00 : 0xFF0000;
			sprintf_s(statStr, sizeof(statStr), diff2 > 0 ? "*%i %s" : "%i %s", diff2, itemStatsStr[j]);
			DrawString(statStr, 406, 220 + (10 * (count2 + count4)), color, 1);
			count4++;
		}
	}

}

static void GetPartyIndex(int *index1, int *index2, MainStat statType)
{
	switch (statType)
	{
		case MAINSTAT_STR:
		{
			*index1 = 0;
			*index2 = 3;
		} break;

		case MAINSTAT_AGI:
		{
			*index1 = 4;
			*index2 = 5;
		} break;

		case MAINSTAT_INT:
		{
			*index1 = 1;
			*index2 = 2;
		} break;
	}
}

static void RemoveItemFromLoot(LootStateData *data, int indexToRemove)
{
	for (int i = indexToRemove; i < data->numItems - 1; i++)
	{
		data->item[i] = data->item[i + 1];
	}
	data->numItems--;

	if (data->numItems == 0)
	{
		ClearFrame(0);
		ClearMessageLog();
		DestroyCurrentState();
		SetState(CreateMovementState(0));
	}

	if (data->itemSelection >= data->numItems)
	{
		data->itemSelection = data->numItems - 1;
	}
}