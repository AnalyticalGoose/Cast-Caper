#include "class.h"
#include "difficulty.h"
#include "input.h"
#include "movement_state.h"
#include "party.h"
#include "renderer.h"
#include "state.h"
#include "state_manager.h"
#include "stats.h"
#include "title_state.h"
#include <windows.h>


typedef enum {
	NEW_GAME_DIFFICULTY,
	NEW_GAME_PARTY,
	NEW_GAME_MEMBER,
} NewGameSubState;


typedef struct NewGameStateData
{
	NewGameSubState subState;
	BOOLEAN subStateRendered;
	int difficultySelection;
	BOOLEAN firstPartyRendered;
	int partyMemberSelection;
	int prevMemberSelection;
	int yAxisMenuSelection;
	//int partySubMenuSelection; // Spec / class selection
} NewGameStateData;


static const unsigned char diffSprites[3][32][4] = {
	{ // Buttercup
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x03, 0xC0, 0x00 },
		{ 0x00, 0x1F, 0xF8, 0x00 },
		{ 0x00, 0x3F, 0xFC, 0x00 },
		{ 0x00, 0xFC, 0x3F, 0x00 },
		{ 0x00, 0xF0, 0x0F, 0x00 },
		{ 0x01, 0xC0, 0x03, 0x80 },
		{ 0x03, 0x9E, 0xF9, 0xC0 },
		{ 0x03, 0xBD, 0x7D, 0xC0 },
		{ 0x03, 0x3E, 0x7C, 0xC0 },
		{ 0x03, 0x3F, 0xFC, 0xC0 },
		{ 0x03, 0x77, 0xEE, 0xC0 },
		{ 0x03, 0x77, 0xEE, 0xC0 },
		{ 0x01, 0x7F, 0xFE, 0x80 },
		{ 0x01, 0x7C, 0x3E, 0x80 },
		{ 0x00, 0x7D, 0xBE, 0x00 },
		{ 0x00, 0x3B, 0xDC, 0x00 },
		{ 0x00, 0x3A, 0x5C, 0x00 },
		{ 0x00, 0x18, 0x18, 0x00 },
		{ 0x00, 0x04, 0x20, 0x00 },
		{ 0x00, 0x1A, 0x58, 0x00 },
		{ 0x00, 0x70, 0x0C, 0x00 },
		{ 0x00, 0x7C, 0x3C, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
	},
	{ // Normal
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x3F, 0xFC, 0x00 },
		{ 0x00, 0x40, 0x02, 0x00 },
		{ 0x00, 0x80, 0x01, 0x00 },
		{ 0x00, 0xB0, 0x0D, 0x00 },
		{ 0x00, 0xBF, 0xFD, 0x00 },
		{ 0x00, 0xBF, 0xFD, 0x00 },
		{ 0x00, 0xBF, 0xFD, 0x00 },
		{ 0x00, 0xA3, 0xC5, 0x00 },
		{ 0x01, 0x7F, 0xFE, 0x80 },
		{ 0x03, 0xF7, 0xEF, 0xC0 },
		{ 0x03, 0xF7, 0xEF, 0xC0 },
		{ 0x03, 0xFF, 0xFF, 0xC0 },
		{ 0x01, 0xFE, 0x7F, 0x80 },
		{ 0x00, 0x7F, 0xFE, 0x00 },
		{ 0x00, 0x78, 0x1E, 0x00 },
		{ 0x00, 0x7F, 0xFE, 0x00 },
		{ 0x00, 0x3F, 0xFC, 0x00 },
		{ 0x00, 0x1F, 0xF8, 0x00 },
		{ 0x00, 0x07, 0xE0, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
	},
	{
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x80, 0x01, 0x00 },
		{ 0x00, 0x80, 0x01, 0x00 },
		{ 0x01, 0x87, 0xE1, 0x80 },
		{ 0x01, 0xDF, 0xFB, 0x80 },
		{ 0x01, 0xEF, 0xF7, 0x80 },
		{ 0x00, 0xDF, 0xFB, 0x00 },
		{ 0x00, 0x7F, 0xFE, 0x00 },
		{ 0x00, 0x3F, 0xFC, 0x00 },
		{ 0x00, 0xFF, 0xFF, 0x00 },
		{ 0x02, 0xCF, 0xF3, 0x40 },
		{ 0x00, 0xE3, 0xC7, 0x00 },
		{ 0x02, 0xE3, 0xC7, 0x40 },
		{ 0x02, 0xFF, 0xFF, 0x40 },
		{ 0x00, 0xDF, 0xFB, 0x00 },
		{ 0x00, 0xCF, 0xF3, 0x00 },
		{ 0x00, 0x43, 0xC2, 0x00 },
		{ 0x00, 0x34, 0x2C, 0x00 },
		{ 0x00, 0x3B, 0xDC, 0x00 },
		{ 0x00, 0x3C, 0x3C, 0x00 },
		{ 0x00, 0x1F, 0xF8, 0x00 },
		{ 0x00, 0x03, 0xC0, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
	}
};

static void DrawPartyMember(int i, int xOffset, int partyMemberSelection, int prevMemberSelction, int yAxisMenu) {
	const Class *classData = party[i].classData;
	const char *className = classData->name;
	int classColour = classData->colour;
	const char *specName = classData->specs[party[i].specIndex].name;
	Role role = classData->specs[party[i].specIndex].role;

	DrawGreyscaleRectangle(xOffset - 20, 94, 140, 200, 0);

	if (partyMemberSelection == i && yAxisMenu != 3)
	{
		DrawColourRectangle(xOffset, 94, 100, 100, 0xFF0000);

		if (yAxisMenu == 0)
		{
			DrawString("<", xOffset - 20, 138, 0xFFFFFF, 2);
			DrawString(">", xOffset + 103, 138, 0xFFFFFF, 2);
		}
		else 
		{
			DrawString("<", xOffset - 7, 210 + (20 * yAxisMenu), 0xFFFFFF, 1);
			DrawString(">", xOffset + 99, 210 + (20 * yAxisMenu), 0xFFFFFF, 1);
		}

		if (prevMemberSelction != -1)
		{
			int prevXOffset = 110 + (140 * prevMemberSelction);
			DrawPartyMember(prevMemberSelction, prevXOffset, partyMemberSelection, -1, yAxisMenu);
			DrawGreyscaleRectangle(prevXOffset - 20, 138, 16, 16, 0);
			DrawGreyscaleRectangle(prevXOffset + 103, 138, 16, 16, 0);
		}
	}

	else
	{
		DrawColourRectangle(xOffset, 94, 100, 100, 0xFFFFFF);
	}

	DrawStringCentered(roleString[role], xOffset, xOffset + 100, 210, roleColour[role], 1);
	DrawStringCentered(className, xOffset, xOffset + 100, 230, classColour, 1);
	DrawStringCentered(specName, xOffset, xOffset + 100, 250, classColour, 1);

	DrawColourRectangle(xOffset + 2, 96, 96, 96, roleBackground[role]);
	DrawSprite(xOffset + 2, 96, 96, 96, classData->sprite, classData->specs[party[i].specIndex].palette);
}


void UpdateNewGame(GameState *state, float deltaTime)
{
	NewGameStateData *data = (NewGameStateData *)state->data;

	switch (data->subState) {
		case NEW_GAME_DIFFICULTY:
		{
			if (IsKeyJustPressed(VK_LEFT))
			{
				data->difficultySelection = (data->difficultySelection + 3 - 1) % 3;
				data->subStateRendered = FALSE;
			}

			if (IsKeyJustPressed(VK_RIGHT))
			{
				data->difficultySelection = (data->difficultySelection + 1) % 3;
				data->subStateRendered = FALSE;
			}

			if (IsKeyJustPressed(VK_RETURN))
			{
				gameDifficulty = data->difficultySelection;

				//ClearFrame(0);
				//data->subState = NEW_GAME_PARTY;
				//data->subStateRendered = FALSE;
				
				ClearFrame(0);
				DestroyCurrentState();
				SetState(CreateMovementState(2));
			}

			if (IsKeyJustPressed(VK_ESCAPE))
			{
				ClearFrame(0);
				DestroyCurrentState();
				SetState(CreateTitleState(1));
			}
		} break;

		case NEW_GAME_PARTY: // UNUSED / CUT CONTENT
		{
			if (IsKeyJustPressed(VK_ESCAPE))
			{
				ClearFrame(0);
				data->subState = NEW_GAME_DIFFICULTY;
				data->subStateRendered = FALSE;
				data->firstPartyRendered = FALSE;
			}

			if (IsKeyJustPressed(VK_LEFT))
			{
				if (data->yAxisMenuSelection == 3) return;
				data->prevMemberSelection = data->partyMemberSelection;
				data->partyMemberSelection = (data->partyMemberSelection + 6 - 1) % 6;
				data->subStateRendered = FALSE;
			}

			if (IsKeyJustPressed(VK_RIGHT))
			{
				if (data->yAxisMenuSelection == 3) return;
				data->prevMemberSelection = data->partyMemberSelection;
				data->partyMemberSelection = (data->partyMemberSelection + 1) % 6;
				data->subStateRendered = FALSE;
			}

			if (IsKeyJustPressed(VK_DOWN) || IsKeyJustPressed(VK_UP))
			{
				data->yAxisMenuSelection = data->yAxisMenuSelection == 0 ? 3 : 0;
				data->subStateRendered = false;
			}

			if (IsKeyJustPressed(VK_RETURN))
			{
				if (data->yAxisMenuSelection == 0)
				{
					data->yAxisMenuSelection = 1;
					data->subState = NEW_GAME_MEMBER;
					int i = data->partyMemberSelection;
					DrawPartyMember(i, 110 + (140 * i), i, i, data->yAxisMenuSelection);
				}
				else if (data->yAxisMenuSelection == 3)
				{
					SetPartyVars();
					ClearFrame(0);
					DestroyCurrentState();
					SetState(CreateMovementState(0));
				}
			}
		} break;

		case NEW_GAME_MEMBER: // // UNUSED / CUT CONTENT
		{
			if (IsKeyJustPressed(VK_UP))
			{
				data->yAxisMenuSelection = data->yAxisMenuSelection == 2 ? 1 : 2;
				int i = data->partyMemberSelection;
				DrawPartyMember(i, 110 + (140 * i), i, i, data->yAxisMenuSelection);
			}

			if (IsKeyJustPressed(VK_DOWN))
			{
				data->yAxisMenuSelection = data->yAxisMenuSelection == 1 ? 2 : 1;
				int i = data->partyMemberSelection;
				DrawPartyMember(i, 110 + (140 * i), i, i, data->yAxisMenuSelection);
			}

			if (IsKeyJustPressed(VK_ESCAPE))
			{
				data->subState = NEW_GAME_PARTY;
				data->subStateRendered = FALSE;
				data->yAxisMenuSelection = 0;
			}

			if (IsKeyJustPressed(VK_LEFT))
			{
				if (data->yAxisMenuSelection == 1) // class
				{
					int classIndex = party[data->partyMemberSelection].classData->index;
					party[data->partyMemberSelection].classData = &classes[(classIndex + 6 - 1) % 6];
					
					int i = data->partyMemberSelection;
					DrawPartyMember(i, 110 + (140 * i), i, i, data->yAxisMenuSelection);
				}
				if (data->yAxisMenuSelection == 2) //  spec
				{
					int specIndex = party[data->partyMemberSelection].specIndex;
					party[data->partyMemberSelection].specIndex = (specIndex + 3 - 1) % 3;

					int i = data->partyMemberSelection;
					DrawPartyMember(i, 110 + (140 * i), i, i, data->yAxisMenuSelection);
				}
			}

			if (IsKeyJustPressed(VK_RIGHT))
			{
				if (data->yAxisMenuSelection == 1) // class
				{
					int classIndex = party[data->partyMemberSelection].classData->index;
					party[data->partyMemberSelection].classData = &classes[(classIndex + 1) % 6];

					int i = data->partyMemberSelection;
					DrawPartyMember(i, 110 + (140 * i), i, i, data->yAxisMenuSelection);
				}
				if (data->yAxisMenuSelection == 2) //  spec
				{
					int specIndex = party[data->partyMemberSelection].specIndex;
					party[data->partyMemberSelection].specIndex = (specIndex + 1) % 3;

					int i = data->partyMemberSelection;
					DrawPartyMember(i, 110 + (140 * i), i, i, data->yAxisMenuSelection);
				}
			}

		} break;
	}
}


void RenderNewGame(GameState *state)
{
	NewGameStateData *data = (NewGameStateData *)state->data;

	switch (data->subState)
	{
		case NEW_GAME_DIFFICULTY:
		{
			if (!data->subStateRendered)
			{
				DrawStringCentered("CHOOSE YOUR DIFFICULTY:", 0, 1020, 40, 0XFFFFFF, 2);
				DrawString("[ENTER] Start Game", 860, 520, 0XFFFFFF, 1);
				DrawString("[ESC] Back", 30, 520, 0XFFFFFF, 1);

				DrawColourRectangle(261, 76, 150, 150, data->difficultySelection == 0 ? 0xFF0000 : 0xFFFFFF);
				DrawColourRectangle(265, 80, 142, 142, 0);
				DrawBitMap32x32(diffSprites[0], 256, 74, data->difficultySelection == 0 ? 0xFF0000 : 0xFFFFFF, 5);

				DrawString("BUTTERCUP", 266, 240, data->difficultySelection == 0 ? 0xFF0000 : 0xFFFFFF, 2);

				if (data->difficultySelection == 0)
				{
					DrawStringWrapped(
						"You are a delicate little flower, who becomes incontinent at the slightest confrontation.",
						265, 280, 154, true, 0xFFFFFF, 1
					);
					DrawStringWrapped(
						"Encounters are minimally tuned and require little knowledge or skill to beat.",
						265, 340, 154, true, 0xFFFFFF, 1
					);
				}
				else 
				{
					DrawGreyscaleRectangle(261, 278, 160, 160, 0);
				}

				DrawColourRectangle(436, 76, 150, 150, data->difficultySelection == 1 ? 0xFF0000 : 0xFFFFFF);
				DrawColourRectangle(440, 80, 142, 142, 0);
				DrawBitMap32x32(diffSprites[1], 430, 74, data->difficultySelection == 1 ? 0xFF0000 : 0xFFFFFF, 5);
				DrawStringCentered("NORMAL", 0, 1020, 240, data->difficultySelection == 1 ? 0xFF0000 : 0xFFFFFF, 2);

				if (data->difficultySelection == 1)
				{
					DrawStringWrapped(
						"You are familiar with crawlers and experienced with party RPG mechanics.",
						438, 280, 154, true, 0xFFFFFF, 1
					);

					DrawStringWrapped(
						"Play the game as it was intended. Be rewarded for your successes & punished for your mistakes.",
						438, 340, 154, true, 0xFFFFFF, 1
					);
				}
				else
				{
					DrawGreyscaleRectangle(434, 278, 160, 160, 0);
				}

				DrawColourRectangle(610, 76, 150, 150, data->difficultySelection == 2 ? 0xFF0000 : 0xFFFFFF);
				DrawColourRectangle(614, 80, 142, 142, 0);
				DrawBitMap32x32(diffSprites[2], 604, 74, data->difficultySelection == 2 ? 0xFF0000 : 0xFFFFFF, 5);
				DrawString("MASOCHIST", 616, 240, data->difficultySelection == 2 ? 0xFF0000 : 0xFFFFFF, 2);

				if (data->difficultySelection == 2)
				{
					DrawStringWrapped(
						"You are a glutton for punishment & take pleasure in wiping repeatedly for little gain.",
						612, 280, 154, true, 0xFFFFFF, 1
					);

					DrawStringWrapped(
						"DPS & healing checks can be hard, requiring optimised gear, and ability execution.",
						612, 340, 154, true, 0xFFFFFF, 1
					);
				}
				else
				{
					DrawGreyscaleRectangle(608, 278, 160, 160, 0);
				}

				data->subStateRendered = TRUE;
			}
		} break;

		case NEW_GAME_PARTY:
		{
			if (!data->firstPartyRendered)
			{
				DrawStringCentered("CHOOSE YOUR PARTY:", 0, 1020, 40, 0XFFFFFF, 2);
				DrawString("Role", 20, 210, 0XFFFFFF, 1);
				DrawString("Class", 20, 230, 0XFFFFFF, 1);
				DrawString("Spec", 20, 250, 0XFFFFFF, 1);

				DrawColourRectangle(420, 470, 180, 34, data->yAxisMenuSelection == 3 ? 0xFF0000 : 0xFFFFFF);
				DrawGreyscaleRectangle(422, 472, 176, 30, 0);
				DrawStringCentered("START GAME", 0, 1020, 480, data->yAxisMenuSelection == 3 ? 0xFF0000 : 0xFFFFFF, 2);
				
				DrawString("[ENTER] Continue", 860, 520, 0XFFFFFF, 1);
				DrawString("[ESC] Back", 30, 520, 0XFFFFFF, 1);

				for (int i = 0; i < 6; i++)
				{
					DrawPartyMember(i, 110 + (140 * i), data->partyMemberSelection, -1, data->yAxisMenuSelection);
				}
				data->subStateRendered = TRUE;
				data->firstPartyRendered = TRUE;
			}

			if (!data->subStateRendered)
			{
				int i = data->partyMemberSelection;
				DrawPartyMember(i, 110 + (140 * i), i, data->prevMemberSelection, data->yAxisMenuSelection);
				
				DrawColourRectangle(420, 470, 180, 34, data->yAxisMenuSelection == 3 ? 0xFF0000 : 0xFFFFFF);
				DrawGreyscaleRectangle(422, 472, 176, 30, 0);
				DrawStringCentered("START GAME", 0, 1020, 480, data->yAxisMenuSelection == 3 ? 0xFF0000 : 0xFFFFFF, 2);
				
				data->subStateRendered = TRUE;
			}
		}
	}
}


void DestroyNewGame(GameState *state)
{
	if (state)
	{
		if (state->data)
			free(state->data);
		free(state);
	}
};


GameState *CreateNewGameState(int substate) 
{
	GameState *state = malloc(sizeof(GameState));
	if (!state)
	{
		return NULL;
	}

	NewGameStateData *data = malloc(sizeof(NewGameStateData));
	if (!data)
	{
		free(state);
		return NULL;
	}

	data->subState = substate;
	data->subStateRendered = FALSE;
	data->difficultySelection = 1;
	data->firstPartyRendered = FALSE;
	data->partyMemberSelection = 0;
	data->prevMemberSelection = -1;
	data->yAxisMenuSelection = 0;

	state->data = data;
	state->update = UpdateNewGame;
	state->render = RenderNewGame;
	state->destroy = DestroyNewGame;

	return state;
}