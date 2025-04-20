#include "audio.h"
#include "boss_state.h"
#include "combat_state.h"
#include "disenchant.h"
#include "input.h"
#include "item.h"
#include "map.h"
#include "party.h"
#include "renderer.h"
#include "state.h"
#include "state_manager.h"
#include "ui.h"
#include "view.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

/*
 Movement state encompasses movement and all menu's and such that can be accessed while
 the player is moving around the map, which included the disenchanting idle game and crafting.
 This preserves all of the state data for these elements that need to exists in parallel -
 and saves me having to store that data in a smarter manner.

 This was also written last, so it's complete ass....
*/

typedef enum
{
	MOVEMENT,
	PARTY_SCREEN,
	DISENCHANT_SCREEN,
	CRAFTING_SCREEN,
	CRAFTING_ITEM,
	CRAFTING_EQUIP,
} MovementSubState;

typedef struct MovementStateData
{
	MapData *mapData;
	Item craftingItem;
	MovementSubState subState;
	float progressTimer;
	bool fullRendered;
	bool subStateRendered;
	int nextEncounter;
	int partyMemberSelected;
	int disenchantMenuSelected;
	int craftingMenuSelected;
	int craftingItemMenuSelected;
	int craftingEquipMenuSelected;

	int craftingItemIlvl;
	int craftingItemRarity;
} MovementStateData;


static void DrawPartyArrow(xy oldPos, xy newPos);
static void DrawPartyFrames(const int memberSelected);
static void DrawCharacterInfo(MovementStateData *data);

static void DrawDisenchantUI(DisenchantState *state);
static void UpdateDisenchantUI(DisenchantState *state);
static void DrawDisenchantScreen(DisenchantState *state);
static void UpdateDisenchantSceen(DisenchantState *state);
static void DrawDisenchantButtons(DisenchantState *state, MovementStateData *data);

static void DrawCraftingScreen(MovementStateData *data);
static void DrawCraftingButtons(MovementStateData *data, DisenchantState *state);
static void TryCraft(const int slotIndex, DisenchantState *state, MovementStateData *data);
static void CraftDrawLootItemPanel(Item *item);
static void CraftDrawComparisonPanels(Item *newItem, Item *item1, Item *item2);
static void CraftGetPartyIndex(int *index1, int *index2, MainStat statType);


static void Move(int moveDirection, MapData *mapData, int *nextEncounter, BOOLEAN *subStateRendered, MovementStateData *data)
{
	struct xy newPos = { 0 };

	switch (moveDirection)
	{
		// Move backward
		case 0:
		{
			newPos.x = pos.x + increment[direction].x;
			newPos.y = pos.y + increment[direction].y;
		} break;

		// Move forward
		case 1:
		{
			newPos.x = pos.x - increment[direction].x;
			newPos.y = pos.y - increment[direction].y;
		} break;

		// Move left
		case 2:
		{
			newPos.x = pos.x + left[direction].x;
			newPos.y = pos.y + left[direction].y;
		} break;

		// Move right
		case 3:
		{
			newPos.x = pos.x + right[direction].x;
			newPos.y = pos.y + right[direction].y;
		} break;
	}

	// if newPos is walkable
	if (!mapData->tiles[newPos.y][newPos.x])
	{
		DrawPartyArrow(pos, newPos);

		pos = newPos;  // update position

		if (encountersEnabled)
		{
			(*nextEncounter)--;

			wchar_t wmessage[64];
			swprintf_s(wmessage, 64, L"Next Encounter: %d\n", *nextEncounter);
			OutputDebugStringW(wmessage);

		}

		*subStateRendered = FALSE;  // signal a redraw
	}

	else if (mapData->decorations[newPos.y][newPos.x])
	{
		if (mapData->decorations[newPos.y][newPos.x] < 5) // is a door
		{
			wchar_t wmessage[64];
			swprintf_s(wmessage, 64, L"Door: y = %d, x = %d\n", newPos.y, newPos.x);
			OutputDebugStringW(wmessage);

			Door *door = FindDoorAtPosition(mapData, newPos);
			if (door != NULL)
			{

				switch (door->trigger)
				{
					case TRIGGER_NULL:
					{
						return;
					} break;

					case TRIGGER_DISABLE_ENCOUNTERS:
					{
						DrawPartyArrow(pos, door->movePos);
						pos = door->movePos;
						*subStateRendered = false;

						const char *msg = encountersEnabled == false ? "Entered a safezone - cloak of shadows has been removed." : "Entered a safezone";
						PushMessage(msg, 0XFFFFFF);
						PushMessage("The health and mana of your party have been restored to full!", 0X00FF00);
						encountersEnabled = false;
						inSafezone = true;
						*nextEncounter = -1;

						for (int i = 0; i < 6; i++)
						{
							PartyMember *member = &party[i];
							member->health.x = member->health.y;

							if (member->classData->specs[member->specIndex].secondaryType == SECONDARY_MANA)
							{
								member->secondary.x = member->secondary.y;
							}
							else
							{
								member->secondary.x = 0;
							}

							// Clear dead indicator
							const int y = (67 * i) + 16 + 11;
							const int x = (550 + 160 + (16 * 3)) + 2 + 64;
							DrawGreyscaleRectangle(x, y, 128, 49, 0);

							DrawPartyFrame();
						}

						return;
					} break;

					case TRIGGER_ENABLE_ENCOUNTERS:
					{
						DrawPartyArrow(pos, door->movePos);
						pos = door->movePos;
						*subStateRendered = false;

						encountersEnabled = true;
						inSafezone = false;
						*nextEncounter = rand() % (mapData->encounterRate.y + 1 - mapData->encounterRate.x) + mapData->encounterRate.x;
						PushMessage("Exited a safezone", 0XFFFFFF);

						return;
					} break;

					case TRIGGER_NEXT_MAP:
					{
						mapIndex++;
						data->mapData = LoadMap(mapIndex);
						data->nextEncounter = rand() % (data->mapData->encounterRate.y + 1 - data->mapData->encounterRate.x) + data->mapData->encounterRate.x;
						DrawMap(data->mapData);

						pos = door->movePos;
						DrawPartyArrow(pos, pos);
						*subStateRendered = false;
						return;
					} break;

					case TRIGGER_PREV_MAP:
					{
						mapIndex--;
						data->mapData = LoadMap(mapIndex);
						data->nextEncounter = rand() % (data->mapData->encounterRate.y + 1 - data->mapData->encounterRate.x) + data->mapData->encounterRate.x;
						DrawMap(data->mapData);

						pos = door->movePos;
						DrawPartyArrow(pos, pos);
						*subStateRendered = false;
						return;
					} break;

					case TRIGGER_BOSS:
					{
						DestroyCurrentState();
						SetState(CreateBossState());
					}
				}

				DrawPartyArrow(pos, door->movePos);
				pos = door->movePos;
				*subStateRendered = false;
			}
		}
	}
}


static void Turn(int turn_direction)
{
	switch (turn_direction)
	{
		case 0:
		{
			--direction;
			if (direction < 0) direction = 3;
		} break;

		case 1:
		{
			direction++;
			if (direction > 3) direction = 0;
		} break;
	}
	DrawPartyArrow(pos, pos);
	DrawCompass(direction);
}


void UpdateMovement(GameState *state, float deltaTime)
{
	MovementStateData *data = (MovementStateData *)state->data;

	if (data->nextEncounter == 0)
	{
		data->nextEncounter = rand() % (data->mapData->encounterRate.y + 1 - data->mapData->encounterRate.x) + data->mapData->encounterRate.x;

		PushMessage("You have been ambushed! Prepare to fight;", 0XFF0000);

		DestroyCurrentState();
		SetState(CreateCombatState());
	}

	if (IsKeyJustPressed('P'))
	{
		if (data->subState != PARTY_SCREEN)
		{
			DrawGreyscaleRectangle(18, 18, 546, 346, 0);
			data->subState = PARTY_SCREEN;
			data->subStateRendered = false;
		}
		else
		{
			DrawGreyscaleRectangle(18, 18, 546, 346, 0);
			data->subState = MOVEMENT;
			data->subStateRendered = false;

		}
	}

	if (IsKeyJustPressed('G'))
	{
		if (data->subState != DISENCHANT_SCREEN)
		{
			DisenchantState *state = GetDisenchantState();
			
			DrawDisenchantScreen(state);
			DrawDisenchantButtons(state, data);

			data->subState = DISENCHANT_SCREEN;
			data->subStateRendered = false;
		}
		else
		{
			DrawGreyscaleRectangle(18, 18, 546, 346, 0);
			data->subState = MOVEMENT;
			data->subStateRendered = false;
		}
	}

	if (IsKeyJustPressed('C'))
	{
		if (data->subState != CRAFTING_SCREEN)
		{
			data->subState = CRAFTING_SCREEN;
			data->subStateRendered = false;
		}
		else
		{
			DrawGreyscaleRectangle(18, 18, 546, 346, 0);
			data->subState = MOVEMENT;
			data->subStateRendered = false;
		}
	}

	switch (data->subState)
	{
		case MOVEMENT:
		{
			if (IsKeyJustPressed('W') || IsKeyJustPressed(VK_NUMPAD8))
			{
				Move(0, data->mapData, &data->nextEncounter, &data->subStateRendered, data);
			}
			if (IsKeyJustPressed('S') || IsKeyJustPressed(VK_NUMPAD2))
			{
				Move(1, data->mapData, &data->nextEncounter, &data->subStateRendered, data);
			}
			if (IsKeyJustPressed('A') || IsKeyJustPressed(VK_NUMPAD4))
			{
				Move(2, data->mapData, &data->nextEncounter, &data->subStateRendered, data);
			}
			if (IsKeyJustPressed('D') || IsKeyJustPressed(VK_NUMPAD6))
			{
				Move(3, data->mapData, &data->nextEncounter, &data->subStateRendered, data);
			}
			if (IsKeyJustPressed('Q') || IsKeyJustPressed(VK_NUMPAD7))
			{
				Turn(0);
				data->subStateRendered = FALSE;
			}
			if (IsKeyJustPressed('E') || IsKeyJustPressed(VK_NUMPAD9))
			{
				Turn(1);
				data->subStateRendered = FALSE;
			}

			if (IsKeyJustPressed('H'))
			{
				if (!inSafezone)
				{
					if (encountersEnabled)
					{
						encountersEnabled = false;
						PushMessage("Rogue has cloaked the party in shadow, you can not be ambushed.", 0XFFFFFF);
					}
					else
					{
						encountersEnabled = true;
						PushMessage("Rogue has revealed the party, you can now be ambushed again.", 0XFFFFFF);
					}
				}
				else
				{
					PushMessage("You cannot be cloaked in a safezone.", 0XFF0000);
				}
			}

			if (IsKeyJustPressed('L'))
			{
				ClearMessageLog();
			}
		} break;

		case PARTY_SCREEN:
		{
			if (IsKeyJustPressed(VK_UP))
			{
				data->partyMemberSelected = (data->partyMemberSelected + 6 - 1) % 6;
				data->subStateRendered = false;
			}

			if (IsKeyJustPressed(VK_DOWN))
			{
				data->partyMemberSelected = (data->partyMemberSelected + 1) % 6;
				data->subStateRendered = false;
			}

			if (IsKeyJustPressed(VK_ESCAPE))
			{
				DrawGreyscaleRectangle(18, 18, 546, 346, 0);
				data->subState = MOVEMENT;
				data->subStateRendered = false;
			}
		} break;

		case DISENCHANT_SCREEN:
		{
			if (IsKeyJustPressed(VK_UP))
			{
				data->disenchantMenuSelected = (data->disenchantMenuSelected + 5 - 1) % 5;
				DrawDisenchantButtons(GetDisenchantState(), data);
			}

			if (IsKeyJustPressed(VK_DOWN))
			{
				data->disenchantMenuSelected = (data->disenchantMenuSelected + 1) % 5;
				DrawDisenchantButtons(GetDisenchantState(), data);
			}

			if (IsKeyJustPressed(VK_ESCAPE))
			{
				DrawGreyscaleRectangle(18, 18, 546, 346, 0);
				data->subState = MOVEMENT;
				data->subStateRendered = false;
			}

			if (IsKeyJustPressed(VK_RETURN))
			{
				DisenchantState *state = GetDisenchantState();

				switch (data->disenchantMenuSelected)
				{
					case 0:
					{
						if (state->jobSlotUpgradeLvl == 10)
						{
							PushMessage("Job slots are fully upgraded!", 0XFF0000);
							return;
						}

						bool upgradeSuccess = TryUpgrade(GetDisenchantState(), 0);
						if (upgradeSuccess)
						{
							needDisenchantUIRedraw = true;
						}
						else
						{
							PushMessage("Insufficient resources for this upgrade.", 0XFF0000);
						}
					} break;

					case 1:
					{
						if (state->jobSpeedUpgradeLvl == 10)
						{
							PushMessage("Job speed is fully upgraded!", 0XFF0000);
							return;
						}

						bool upgradeSuccess = TryUpgrade(GetDisenchantState(), 1);
						if (upgradeSuccess)
						{
							needDisenchantUIRedraw = true;
						}
						else
						{
							PushMessage("Insufficient resources for this upgrade.", 0XFF0000);
						}
					} break;

					case 2:
					{
						if (state->jobValueUpgradeLvl == 10)
						{
							PushMessage("Job value is fully upgraded!", 0XFF0000);
							return;
						}

						bool upgradeSuccess = TryUpgrade(GetDisenchantState(), 2);
						if (upgradeSuccess)
						{
							needDisenchantUIRedraw = true;
						}
						else
						{
							PushMessage("Insufficient resources for this upgrade.", 0XFF0000);
						}
					} break;

					case 3:
					{
						if (state->ilvlUpgradeLvl == 10)
						{
							PushMessage("Crafting ILvl is fully upgraded!", 0XFF0000);
							return;
						}

						bool upgradeSuccess = TryUpgrade(GetDisenchantState(), 3);
						if (upgradeSuccess)
						{
							needDisenchantUIRedraw = true;
						}
						else
						{
							PushMessage("Insufficient resources for this upgrade.", 0XFF0000);
						}
					} break;

					case 4:
					{
						if (state->rarityUpgradeLvl == 4)
						{
							PushMessage("Crafting rarity is fully upgraded!", 0XFF0000);
							return;
						}

						bool upgradeSuccess = TryUpgrade(GetDisenchantState(), 4);
						if (upgradeSuccess)
						{
							needDisenchantUIRedraw = true;
						}
						else
						{
							PushMessage("Insufficient resources for this upgrade.", 0XFF0000);
						}
					} break;
				}
			}

			if (data->progressTimer >= 0.1f)
			{
				data->subStateRendered = false;
				data->progressTimer = 0.0f;
			}

			data->progressTimer += deltaTime;
		} break;

		case CRAFTING_SCREEN:
		{
			if (IsKeyJustPressed(VK_RETURN))
			{
				TryCraft(data->craftingMenuSelected, GetDisenchantState(), data);
			}
		} break;

		case CRAFTING_ITEM:
		{
			if (IsKeyJustPressed(VK_UP))
			{
				data->craftingItemMenuSelected = (data->craftingItemMenuSelected + 3 - 1) % 3;
				data->subStateRendered = false;
			}

			if (IsKeyJustPressed(VK_DOWN))
			{
				data->craftingItemMenuSelected = (data->craftingItemMenuSelected + 1) % 3;
				data->subStateRendered = false;
			}

			if (IsKeyJustPressed(VK_RETURN))
			{
				switch (data->craftingItemMenuSelected)
				{
					case 0:
					{
						DrawGreyscaleRectangle(80, 280, 140, 60, 0);
						data->subState = CRAFTING_EQUIP;
						data->subStateRendered = false;
					} break;

					case 1:
					{
						bool success = AddDisenchantJob(data->craftingItemIlvl, data->craftingItemRarity);
						if (success)
						{
							PushMessage("Item has been sent to be disenchanted", 0X00FF00);
							data->subState = CRAFTING_SCREEN;
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

						data->subState = CRAFTING_SCREEN;
						data->subStateRendered = false;
					} break;
				}
			}
		} break;

		case CRAFTING_EQUIP:
		{
			if (IsKeyJustPressed(VK_LEFT))
			{
				data->craftingEquipMenuSelected = (data->craftingEquipMenuSelected + 2 - 1) % 2;
				data->subStateRendered = false;
			}

			if (IsKeyJustPressed(VK_RIGHT))
			{
				data->craftingEquipMenuSelected = (data->craftingEquipMenuSelected + 1) % 2;
				data->subStateRendered = false;
			}

			if (IsKeyJustPressed(VK_RETURN))
			{
				int partyIndex1, partyIndex2;
				CraftGetPartyIndex(&partyIndex1, &partyIndex2, data->craftingItem.stats.mainStatType);
				int targetPartyIndex = data->craftingEquipMenuSelected == 0 ? partyIndex1 : partyIndex2;

				EquipItem(&party[targetPartyIndex], &data->craftingItem);

				data->subState = CRAFTING_SCREEN;
				data->subStateRendered = false;
			}

			if (IsKeyJustPressed(VK_ESCAPE))
			{
				DrawGreyscaleRectangle(240, 330, 140, 18, 0);
				DrawGreyscaleRectangle(400, 330, 140, 18, 0);
				data->subState = CRAFTING_ITEM;
				data->subStateRendered = false;
			}
		} break;
	}
}

void RenderMovement(GameState *state)
{
	MovementStateData *data = (MovementStateData *)state->data;
	
	if (!data->fullRendered)
	{
		DrawFullUI(direction);

		DrawView(data->mapData->tiles, data->mapData->decorations, pos, direction);
		data->fullRendered = TRUE;
		data->subStateRendered = TRUE;
	}

	if (needDisenchantUIRedraw)
	{
		DisenchantState *dState = GetDisenchantState();

		UpdateDisenchantUI(dState);

		if (data->subState == DISENCHANT_SCREEN)
		{
			DrawDisenchantScreen(dState);
			DrawDisenchantButtons(dState, data);
		}
	}

	switch (data->subState)
	{
		case MOVEMENT:
		{
			if (IsKeyPressed('W') || IsKeyPressed(VK_NUMPAD8))
			{
				DrawArrow(0, 0x69A969);
			}
			else
			{
				DrawArrow(0, 0xDDDDDD);
			}

			if (IsKeyPressed('S') || IsKeyPressed(VK_NUMPAD2))
			{
				DrawArrow(1, 0x69A969);
			}
			else
			{
				DrawArrow(1, 0xDDDDDD);
			}

			if (IsKeyPressed('A') || IsKeyPressed(VK_NUMPAD4))
			{
				DrawArrow(2, 0x69A969);
			}
			else
			{
				DrawArrow(2, 0xDDDDDD);
			}

			if (IsKeyPressed('D') || IsKeyPressed(VK_NUMPAD6))
			{
				DrawArrow(3, 0x69A969);
			}
			else
			{
				DrawArrow(3, 0xDDDDDD);
			}

			if (IsKeyPressed('Q') || IsKeyPressed(VK_NUMPAD7))
			{
				DrawArrow(4, 0x69A969);
			}
			else
			{
				DrawArrow(4, 0xDDDDDD);
			}

			if (IsKeyPressed('E') || IsKeyPressed(VK_NUMPAD9))
			{
				DrawArrow(5, 0x69A969);
			}
			else
			{
				DrawArrow(5, 0xDDDDDD);
			}

			if (!data->subStateRendered)
			{
				DrawView(data->mapData->tiles, data->mapData->decorations, pos, direction);
				data->subStateRendered = true;
			}
		} break;

		case PARTY_SCREEN:
		{
			if (!data->subStateRendered)
			{
				DrawGreyscaleRectangle(18, 18, 546, 346, 0);
				DrawPartyFrames(data->partyMemberSelected);
				DrawCharacterInfo(data);
				data->subStateRendered = true;
			}
		} break;

		case DISENCHANT_SCREEN:
		{
			if (!data->subStateRendered)
			{
				UpdateDisenchantSceen(GetDisenchantState());
				data->subStateRendered = true;
			}
		} break;

		case CRAFTING_SCREEN:
		{
			if (!data->subStateRendered)
			{
				DrawCraftingScreen(data);
				DrawCraftingButtons(data, GetDisenchantState());
				
				data->subStateRendered = true;
			}
		} break;

		case CRAFTING_ITEM:
		{
			if (!data->subStateRendered)
			{
				if (!data->subStateRendered)
				{
					DrawColourRectangle(80, 280, 140, 18, data->craftingItemMenuSelected == 0 ? 0XFFFFFF : 0X780000);
					DrawColourRectangle(81, 281, 138, 16, 0XBD0000);
					DrawStringCentered("Equip", 80, 220, 285, 0XFFFFFF, 1);

					DrawColourRectangle(80, 300, 140, 18, data->craftingItemMenuSelected == 1 ? 0XFFFFFF : 0X780000);
					DrawColourRectangle(81, 301, 138, 16, 0XBD0000);
					DrawStringCentered("Disenchant", 80, 220, 305, 0XFFFFFF, 1);

					DrawColourRectangle(80, 320, 140, 18, data->craftingItemMenuSelected == 2 ? 0XFFFFFF : 0X780000);
					DrawColourRectangle(81, 321, 138, 16, 0XBD0000);
					DrawStringCentered("Discard", 80, 220, 325, 0XFFFFFF, 1);

					data->subStateRendered = true;
				}
			}
		} break;

		case CRAFTING_EQUIP:
		{
			if (!data->subStateRendered)
			{
				DrawColourRectangle(240, 330, 140, 18, data->craftingEquipMenuSelected == 0 ? 0XFFFFFF : 0X780000);
				DrawColourRectangle(241, 331, 138, 16, 0XBD0000);
				DrawStringCentered("Equip", 240, 380, 335, 0XFFFFFF, 1);

				DrawColourRectangle(400, 330, 140, 18, data->craftingEquipMenuSelected == 1 ? 0XFFFFFF : 0X780000);
				DrawColourRectangle(401, 331, 138, 16, 0XBD0000);
				DrawStringCentered("Equip", 400, 540, 335, 0XFFFFFF, 1);

				data->subStateRendered = true;
			}
		}
	}
}

void DestroyMovement(GameState *state)
{
	if (state)
	{
		if (state->data)
			free(state->data);
		free(state);
	}
}

GameState *CreateMovementState(int subState)
{
	GameState *state = malloc(sizeof(GameState));
	if (!state)
	{
		return NULL;
	}

	MovementStateData *data = malloc(sizeof(MovementStateData));
	if (!data)
	{
		free(state);
		return NULL;
	}

	data->mapData = LoadMap(mapIndex);

	if (subState == 1)
	{
		pos = data->mapData->respawn;
		inSafezone = true;
		encountersEnabled = false;
		data->subState = MOVEMENT;

		PushMessage("You have respawned back in a safezone", 0X00FF00);
	}
	else if (subState == 2)
	{
		PushMessage("Your party stumbles into the basement of Cuthbert's Castle.", 0XFF0000);
		PushMessage(" ", 0);
		PushMessage("Rumours of Cuthbert's twisted creations, and", 0XFF0000);
		PushMessage("his prized golden egg have circulated for years.", 0XFF0000);
		PushMessage(" ", 0);
		PushMessage("Perhaps you will ascend the keep, and end his reign of tryanny?", 0XFF0000);

		data->subState = MOVEMENT;
	}
	else
	{
		data->subState = MOVEMENT;
	}

	data->progressTimer = 0.0f;
	data->fullRendered = FALSE;
	data->subStateRendered = FALSE;

	DrawMap(data->mapData);
	DrawPartyArrow(pos, pos);
	DrawDisenchantUI(GetDisenchantState());

	data->nextEncounter = rand() % (data->mapData->encounterRate.y + 1 - data->mapData->encounterRate.x) + data->mapData->encounterRate.x;;
	data->partyMemberSelected = 0;
	data->disenchantMenuSelected = 0;
	data->craftingMenuSelected = 0;
	data->craftingItemMenuSelected = 0;
	data->craftingEquipMenuSelected = 0;

	state->data = data;
	state->update = UpdateMovement;
	state->render = RenderMovement;
	state->destroy = DestroyMovement;

	return state;
}

static void DrawPartyArrow(xy oldPos, xy newPos)
{
	const unsigned char movementArrows[4][8] = {
	{ 0x00, 0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18, 0x00},  // N
	{ 0x00, 0x10, 0x30, 0x7E, 0x7E, 0x30, 0x10, 0x00 }, // E
	{ 0x00, 0x18, 0x18, 0x18, 0x7E, 0x3C, 0x18, 0x00},  // S
	{ 0x00, 0x08, 0x0C, 0x7E, 0x7E, 0x0C, 0x08, 0x00 }, // W
	};

	const int sX = 590;
	const int sY = 52;

	if (!inSafezone)
	{
		DrawGreyscaleRectangle(sX + 1 + (9 * oldPos.x), sY + (9 * oldPos.y), 7, 8, 0);
	}
	else
	{
		DrawColourRectangle(sX + 1 + (9 * oldPos.x), sY + (9 * oldPos.y), 7, 8, 0x90D36F);
	}

	DrawBitMap(movementArrows[direction], sX + (9 * newPos.x), sY + (9 * newPos.y), 0xff0000, 1);
}

static void DrawPartyFrames(const int memberSelected)
{
	for (int i = 0; i < 6; i++)
	{
		const Class *classData = party[i].classData;
		Role role = classData->specs[party[i].specIndex].role;
		int backgroundColour = roleBackground[role];
		int borderColour = memberSelected == i ? 0XFF0000 : 0XFFFFFF;

		const int height = 48;
		const int width = 48;
		const int padding = 4;

		int y = 36 + (height + padding) * i;

		DrawColourRectangle(22, y, width, height, borderColour);
		DrawColourRectangle(22 + 2, y + 2, width - 4, height - 4, backgroundColour);
		DrawSprite(22 + 2, y + 2, width - 4, width - 4, classData->sprite, classData->specs[party[i].specIndex].palette);
	}
}

static void DrawCharacterInfo(MovementStateData *data)
{
	const unsigned char testBitmap[8] = { 0X18, 0x66, 0xDB, 0x7E, 0x7E, 0x5A, 0x24, 0X18 };
	const char *mainStatStr[3] = { "Strength", "Agility", "Intelligence" };
	const char *itemStatsStr[6] = { "Critical", "Haste", "Mastery", "Resistance", "Avoidance", "Leech" };

	char statStr[50] = { 0 };

	PartyMember *member = &party[data->partyMemberSelected];
	TotalStats *stats = &member->stats;
	EquippedItems *gear = &member->gear;

	sprintf_s(statStr, sizeof(statStr), "%s stats:", member->classData->name);
	DrawString(statStr, 400, 40, 0xFFFFFF, 1);

	sprintf_s(statStr, sizeof(statStr), "Level: %i", member->level);
	DrawString(statStr, 400, 56, 0xe4bb00, 1);
	

	float ilvl = 0;
	for (int i = 0; i < 9; i++)
	{
		Item *item = &gear->items[i];
		uint32_t rarityCol = rarityColours[(int)item->rarity];

		const int sY = 36 + (34 + 1) * i;
		DrawColourRectangle(80, sY, 300, 32, rarityCol);
		DrawGreyscaleRectangle(81, sY + 1, 298, 30, 0);
		DrawBitMap(testBitmap, 84, sY + 4, rarityCol, 3);

		sprintf_s(statStr, sizeof(statStr), "Item Level: %i", item->ilvl);
		DrawString(statStr, 124, sY + 6, 0xe4bb00, 1);
		sprintf_s(statStr, sizeof(statStr), "Main Stat: %i", item->stats.mainStat);
		DrawString(statStr, 254, sY + 6, 0xDDDDDD, 1);

		sprintf_s(statStr, sizeof(statStr), "C:%i", item->stats.crit);
		DrawString(statStr, 124, sY + 18, item->stats.crit > 0 ? 0x45b945 : 0xDDDDDD, 1);
		sprintf_s(statStr, sizeof(statStr), "H:%i", item->stats.haste);
		DrawString(statStr, 166, sY + 18, item->stats.haste > 0 ? 0x45b945 : 0xDDDDDD, 1);
		sprintf_s(statStr, sizeof(statStr), "M:%i", item->stats.mastery);
		DrawString(statStr, 208, sY + 18, item->stats.mastery > 0 ? 0x45b945 : 0xDDDDDD, 1);
		sprintf_s(statStr, sizeof(statStr), "R:%i", item->stats.res);
		DrawString(statStr, 250, sY + 18, item->stats.res > 0 ? 0x45b945 : 0xDDDDDD, 1);
		sprintf_s(statStr, sizeof(statStr), "A:%i", item->stats.avoid);
		DrawString(statStr, 292, sY + 18, item->stats.avoid > 0 ? 0x45b945 : 0xDDDDDD, 1);
		sprintf_s(statStr, sizeof(statStr), "L:%i", item->stats.leech);
		DrawString(statStr, 334, sY + 18, item->stats.leech > 0 ? 0x45b945 : 0xDDDDDD, 1);

		ilvl += item->ilvl;
	}
	ilvl /= 9;

	sprintf_s(statStr, sizeof(statStr), "Item Level: %.2f", ilvl);
	DrawString(statStr, 400, 70, 0xe4bb00, 1);
	sprintf_s(statStr, sizeof(statStr), "%s: %i", mainStatStr[(int)gear->items[0].stats.mainStatType], stats->mainStat);
	DrawString(statStr, 400, 90, 0xDDDDDD, 1);

	// raw values
	sprintf_s(statStr, sizeof(statStr), "Critical: %i", stats->crit);
	DrawString(statStr, 400, 104, 0xDDDDDD, 1);
	sprintf_s(statStr, sizeof(statStr), "Haste: %i", stats->haste);
	DrawString(statStr, 400, 118, 0xDDDDDD, 1);
	sprintf_s(statStr, sizeof(statStr), "Mastery: %i", stats->mastery);
	DrawString(statStr, 400, 132, 0xDDDDDD, 1);
	sprintf_s(statStr, sizeof(statStr), "Resistance: %i", stats->res);
	DrawString(statStr, 400, 146, 0xDDDDDD, 1);
	sprintf_s(statStr, sizeof(statStr), "Avoidance: %i", stats->avoid);
	DrawString(statStr, 400, 160, 0xDDDDDD, 1);
	sprintf_s(statStr, sizeof(statStr), "Leech: %i", stats->leech);
	DrawString(statStr, 400, 174, 0xDDDDDD, 1);

	// normalised values to percentages
	float baseCrit = member->classData->specs[member->specIndex].baseStats.crit;
	sprintf_s(statStr, sizeof(statStr), "Crit Chance: %.2f%%", baseCrit + (float)stats->crit / member->level);
	DrawString(statStr, 400, 200, 0xDDDDDD, 1);
	float baseHaste = member->classData->specs[member->specIndex].baseStats.haste;
	sprintf_s(statStr, sizeof(statStr), "Speed: %.2f%%", baseHaste + (float)stats->haste / member->level);
	DrawString(statStr, 400, 214, 0xDDDDDD, 1);
	float baseMastery = member->classData->specs[member->specIndex].baseStats.mastery;
	sprintf_s(statStr, sizeof(statStr), "Mastery: %.2f%%", baseMastery + (float)stats->mastery / member->level);
	DrawString(statStr, 400, 228, 0xDDDDDD, 1);
	sprintf_s(statStr, sizeof(statStr), "Resistance: %.2f%%", (float)stats->res / (member->level * 5));
	DrawString(statStr, 400, 242, 0xDDDDDD, 1);
	sprintf_s(statStr, sizeof(statStr), "Avoidance: %.2f%%", (float)stats->avoid / (member->level * 5));
	DrawString(statStr, 400, 256, 0xDDDDDD, 1);
	sprintf_s(statStr, sizeof(statStr), "Leech: %.2f%%", (float)stats->leech / (member->level * 20));
	DrawString(statStr, 400, 270, 0xDDDDDD, 1);
}

static void DrawDisenchantUI(DisenchantState *state)
{
	DrawGreyscaleRectangle(757, 420, 194, 102, 0XFFFFFF);
	DrawGreyscaleRectangle(759, 422, 190, 98, 0);
	DrawStringCentered("Disenchant & Craft", 757, 951, 428, 0XDDDDDD, 1);
	
	DrawString(" Slots:", 836, 446, 0XDDDDDD, 1);
	DrawString(" Speed:", 836, 460, 0XDDDDDD, 1);
	DrawString(" Value:", 836, 474, 0XDDDDDD, 1);
	DrawString("  Ilvl:", 836, 488, 0XDDDDDD, 1);
	DrawString("Rarity:", 836, 502, 0XDDDDDD, 1);

	const unsigned char gem[8] = { 0X00, 0x3C, 0x4E, 0xBF, 0x8F, 0x4E, 0x2C, 0X18 };
	const int resources[4] = { state->resources.normal, state->resources.magic, state->resources.rare, state->resources.epic };
	char buf[8] = { 0 };

	for (int i = 0; i < 4; i++)
	{
		DrawBitMap(gem, 766, 498 - (19 * i), rarityColours[i], 2);
		sprintf_s(buf, sizeof(buf), "%i", resources[i]);
		DrawString(buf, 792, 503 - (19 * i), 0XDDDDDD, 1);
	}
}

static void UpdateDisenchantUI(DisenchantState *state)
{
	const int resources[4] = { state->resources.normal, state->resources.magic, state->resources.rare, state->resources.epic };
	char buf[8] = { 0 };

	DrawGreyscaleRectangle(788, 440, 36, 74, 0);
	DrawGreyscaleRectangle(890, 440, 50, 74, 0);

	for (int i = 0; i < 4; i++)
	{
		sprintf_s(buf, sizeof(buf), "%i", resources[i]);
		DrawString(buf, 792, 503 - (19 * i), 0XDDDDDD, 1);
	}

	int jobCount = 0;
	for (size_t j = 0; j < state->maxJobs; j++)
	{
		if (state->jobs[j].active)
		{
			jobCount++;
		}
	}

	sprintf_s(buf, sizeof(buf), "%i \\ %i", jobCount, state->maxJobs);
	DrawString(buf, 898, 446, 0XDDDDDD, 1);
	
	sprintf_s(buf, sizeof(buf), "%ix", (int)state->speedMultiplier);
	DrawString(buf, 898, 460, 0XDDDDDD, 1);

	sprintf_s(buf, sizeof(buf), "%ix", (int)state->valueMultiplier);
	DrawString(buf, 898, 474, 0XDDDDDD, 1);

	sprintf_s(buf, sizeof(buf), "%i", state->craftIlvl);
	DrawString(buf, 898, 488, 0XDDDDDD, 1);

	sprintf_s(buf, sizeof(buf), "%i \\ %i", state->craftRarity + 1, 4);
	DrawString(buf, 898, 502, 0XDDDDDD, 1);

	needDisenchantUIRedraw = false;
}

static void DrawDisenchantScreen(DisenchantState *state)
{
	DrawGreyscaleRectangle(18, 18, 546, 346, 0); // clear

	const unsigned char gem[8] = { 0X00, 0x3C, 0x4E, 0xBF, 0x8F, 0x4E, 0x2C, 0X18 };
	
	char buf[8] = { 0 };
	for (int i = 0; i < state->maxJobs; i++)
	{
		if (!state->jobs[i].active)
		{
			DrawGreyscaleRectangle(240, 44 + (30 * i), 250, 20, 0X545454);
			DrawStringCentered("Inactive", 240, 490, 50 + (30 * i), 0XFFFFFF, 1);
			continue;
		}

		int progress = (int)(state->jobs[i].progress / state->jobs[i].requiredTime) * 100;
		DrawProgressBar(240, 44 + (30 * i), 250, 20, progress, 0Xffa600, 0Xbb7a00);

		sprintf_s(buf, sizeof(buf), "%i", (int)(state->jobs[i].ilvl / 4) * (int)state->valueMultiplier);
		DrawString(buf, 194, 52 + (30 * i), 0XFFFFFF, 1);
		DrawBitMap(gem, 216, 45 + (30 * i), rarityColours[(int)state->jobs[i].rarity], 2);
	}
}

static void UpdateDisenchantSceen(DisenchantState *state)
{
	DrawGreyscaleRectangle(498, 40, 50, 310, 0);

	char buf[8] = { 0 };
	for (int i = 0; i < state->maxJobs; i++)
	{
		if (!state->jobs[i].active) continue;

		float progress = (state->jobs[i].progress / state->jobs[i].requiredTime) * 100;
		DrawProgressBar(240, 44 + (30 * i), 250, 20, progress, 0Xffa600, 0Xbb7a00);
		
		sprintf_s(buf, sizeof(buf), "%i%%", (int)progress);
		DrawStringCentered(buf, 240, 490, 50 + (30 * i), 0XFFFFFF, 1);

		sprintf_s(buf, sizeof(buf), "%.1fs", state->jobs[i].requiredTime - state->jobs[i].progress);
		DrawString(buf, 500, 50 + (30 * i), 0XFFFFFF, 1);
	}
}

static void DrawDisenchantButtons(DisenchantState *state, MovementStateData *data)
{
	const unsigned char gem[8] = { 0X00, 0x3C, 0x4E, 0xBF, 0x8F, 0x4E, 0x2C, 0X18 };
	static const int slotBaseCost[10] = { 0, 4, 8, 16, 24, 36, 50, 100, 150, 250 };
	char buf[8] = { 0 };

	int rawIdx = 0;

	if (state->jobSlotUpgradeLvl < 10)
	{
		rawIdx = (state->jobSlotUpgradeLvl - 1) / 2;
		DrawColourRectangle(40, 44, 140, 40, data->disenchantMenuSelected == 0 ? 0XFFFFFF : 0X780000);
		DrawColourRectangle(41, 45, 138, 38, 0XBD0000);
		DrawBitMap(gem, 114, 49, rarityColours[rawIdx == 4 ? 3 : rawIdx], 2);
		sprintf_s(buf, sizeof(buf), "%i", slotBaseCost[state->jobSlotUpgradeLvl]);
		DrawString(buf, 86, 55, rarityColours[rawIdx == 4 ? 3 : rawIdx], 1);
		DrawStringCentered("Add New Slot", 40, 180, 71, 0XFFFFFF, 1);
	}
	else
	{
		DrawColourRectangle(40, 44, 140, 40, data->disenchantMenuSelected == 0 ? 0XFFFFFF : 0X780000);
		DrawColourRectangle(41, 45, 138, 38, 0XBD0000);
		DrawStringCentered("Max Upgraded", 40, 180, 61, 0XFFFFFF, 1);
	}

	if (state->jobSpeedUpgradeLvl < 10)
	{
		rawIdx = (state->jobSpeedUpgradeLvl - 1) / 2;
		DrawColourRectangle(40, 104, 140, 40, data->disenchantMenuSelected == 1 ? 0XFFFFFF : 0X780000);
		DrawColourRectangle(41, 105, 138, 38, 0XBD0000);
		DrawBitMap(gem, 114, 109, rarityColours[rawIdx == 4 ? 3 : rawIdx], 2);
		sprintf_s(buf, sizeof(buf), "%i", slotBaseCost[state->jobSpeedUpgradeLvl]);
		DrawString(buf, 86, 115, rarityColours[rawIdx == 4 ? 3 : rawIdx], 1);
		DrawStringCentered("Disenchant Speed", 40, 180, 131, 0XFFFFFF, 1);
	}
	else
	{
		DrawColourRectangle(40, 104, 140, 40, data->disenchantMenuSelected == 1 ? 0XFFFFFF : 0X780000);
		DrawColourRectangle(41, 105, 138, 38, 0XBD0000);
		DrawStringCentered("Max Upgraded", 40, 180, 121, 0XFFFFFF, 1);
	}

	if (state->jobValueUpgradeLvl < 10)
	{
		rawIdx = (state->jobValueUpgradeLvl - 1) / 2;
		DrawColourRectangle(40, 164, 140, 40, data->disenchantMenuSelected == 2 ? 0XFFFFFF : 0X780000);
		DrawColourRectangle(41, 165, 138, 38, 0XBD0000);
		DrawBitMap(gem, 114, 169, rarityColours[rawIdx == 4 ? 3 : rawIdx], 2);
		sprintf_s(buf, sizeof(buf), "%i", slotBaseCost[state->jobValueUpgradeLvl]);
		DrawString(buf, 86, 175, rarityColours[rawIdx == 4 ? 3 : rawIdx], 1);
		DrawStringCentered("Disenchant Value", 40, 180, 191, 0XFFFFFF, 1);
	}
	else
	{
		DrawColourRectangle(40, 164, 140, 40, data->disenchantMenuSelected == 2 ? 0XFFFFFF : 0X780000);
		DrawColourRectangle(41, 165, 138, 38, 0XBD0000);
		DrawStringCentered("Max Upgraded", 40, 180, 181, 0XFFFFFF, 1);
	}

	if (state->ilvlUpgradeLvl < 10)
	{
		rawIdx = (state->ilvlUpgradeLvl - 1) / 2;
		DrawColourRectangle(40, 224, 140, 40, data->disenchantMenuSelected == 3 ? 0XFFFFFF : 0X780000);
		DrawColourRectangle(41, 225, 138, 38, 0XBD0000);
		DrawBitMap(gem, 114, 229, rarityColours[rawIdx == 4 ? 3 : rawIdx], 2);
		sprintf_s(buf, sizeof(buf), "%i", slotBaseCost[state->ilvlUpgradeLvl]);
		DrawString(buf, 86, 235, rarityColours[rawIdx == 4 ? 3 : rawIdx], 1);
		DrawStringCentered("Crafting ilvl", 40, 180, 251, 0XFFFFFF, 1);
	}
	else
	{
		DrawColourRectangle(40, 224, 140, 40, data->disenchantMenuSelected == 3 ? 0XFFFFFF : 0X780000);
		DrawColourRectangle(41, 225, 138, 38, 0XBD0000);
		DrawStringCentered("Max Upgraded", 40, 180, 241, 0XFFFFFF, 1);
	}

	if (state->rarityUpgradeLvl < 4)
	{
		DrawColourRectangle(40, 284, 140, 40, data->disenchantMenuSelected == 4 ? 0XFFFFFF : 0X780000);
		DrawColourRectangle(41, 285, 138, 38, 0XBD0000);
		DrawBitMap(gem, 114, 289, rarityColours[state->rarityUpgradeLvl], 2);
		sprintf_s(buf, sizeof(buf), "%i", slotBaseCost[state->rarityUpgradeLvl * 2]);
		DrawString(buf, 86, 295, rarityColours[state->rarityUpgradeLvl], 1);
		DrawStringCentered("Crafting rarity", 40, 180, 311, 0XFFFFFF, 1);
	}
	else
	{
		DrawColourRectangle(40, 284, 140, 40, data->disenchantMenuSelected == 4 ? 0XFFFFFF : 0X780000);
		DrawColourRectangle(41, 285, 138, 38, 0XBD0000);
		DrawStringCentered("Max Upgraded", 40, 180, 301, 0XFFFFFF, 1);
	}
}




static void DrawCraftingScreen(MovementStateData *data)
{
	DrawGreyscaleRectangle(18, 18, 546, 346, 0); // clear
}

static void DrawCraftingButtons(MovementStateData *data, DisenchantState *state)
{
	const unsigned char gem[8] = { 0X00, 0x3C, 0x4E, 0xBF, 0x8F, 0x4E, 0x2C, 0X18 };
	static const int slotBaseCost[10] = { 4, 8, 16, 24, 36, 50, 100, 150, 200, 250 };
	char buf[8] = { 0 };

	DrawColourRectangle(40, 44, 140, 40, data->craftingMenuSelected == 0 ? 0XFFFFFF : 0X780000);
	DrawColourRectangle(41, 45, 138, 38, 0XBD0000);
	DrawBitMap(gem, 114, 49, rarityColours[state->rarityUpgradeLvl - 1], 2);
	sprintf_s(buf, sizeof(buf), "%i", slotBaseCost[state->ilvlUpgradeLvl - 1]);
	DrawString(buf, 86, 55, rarityColours[state->rarityUpgradeLvl -1], 1);
	DrawStringCentered("Random Slot", 40, 180, 71, 0XFFFFFF, 1);
}

static void TryCraft(const int slotIndex, DisenchantState *state, MovementStateData *data)
{
	static const int slotBaseCost[10] = { 4, 8, 16, 24, 36, 50, 100, 150, 200, 250 };
	int *resPool[4] = { &state->resources.normal, &state->resources.magic, &state->resources.rare, &state->resources.epic };

	if (*resPool[state->craftRarity] >= slotBaseCost[state->ilvlUpgradeLvl - 1])
	{
		*resPool[state->craftRarity] -= slotBaseCost[state->ilvlUpgradeLvl - 1];

		DrawGreyscaleRectangle(18, 18, 546, 346, 0); // clear

		Slot slot = (Slot)(rand() % 9);
		MainStat mainStat = (MainStat)(rand() % 3);
		Item item = GenerateItem(mainStat, slot, state->craftIlvl, (Rarity)state->craftRarity, "Crafted ");

		data->craftingItem = item;
		data->craftingItemIlvl = item.ilvl;
		data->craftingItemRarity = (int)item.rarity;

		int partyIndex1, partyIndex2;
		CraftGetPartyIndex(&partyIndex1, &partyIndex2, item.stats.mainStatType);

		Item *oldItem1 = &party[partyIndex1].gear.items[(int)slot];
		Item *oldItem2 = &party[partyIndex2].gear.items[(int)slot];

		DrawGreyscaleRectangle(284, 36, 216, 64, 0);
		DrawSprite(280, 36, 64, 64, &party[partyIndex1].classData->sprite, &party[partyIndex1].classData->specs[party[partyIndex1].specIndex].palette);
		DrawSprite(440, 36, 64, 64, &party[partyIndex2].classData->sprite, &party[partyIndex2].classData->specs[party[partyIndex2].specIndex].palette);

		CraftDrawLootItemPanel(&item);
		CraftDrawComparisonPanels(&item, oldItem1, oldItem2);

		needDisenchantUIRedraw = true;
		data->subState = CRAFTING_ITEM;
		data->subStateRendered = false;
	}
	else
	{
		PushMessage("Insufficient resources for this upgrade.", 0xff0000);
	}
}

static void CraftDrawLootItemPanel(Item *item)
{
	const char *mainStatStr[3] = { "Strength", "Agility", "Intelligence" };
	const char *itemStatsStr[6] = { "Critical", "Haste", "Mastery", "Resistance", "Avoidance", "Leech" };

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

static void CraftDrawComparisonPanels(Item *newItem, Item *item1, Item *item2)
{
	const char *mainStatStr[3] = { "Strength", "Agility", "Intelligence" };
	const char *itemStatsStr[6] = { "Critical", "Haste", "Mastery", "Resistance", "Avoidance", "Leech" };

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


static void CraftGetPartyIndex(int *index1, int *index2, MainStat statType)
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


static void CraftDrawIndividualItemButtons(MovementStateData *data)
{

}