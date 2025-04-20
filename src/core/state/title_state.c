#include "input.h"
#include "state.h"
#include "renderer.h"

#include "new_game_state.h"
#include "state_manager.h"

#include <windows.h>


typedef enum 
{
	TITLE_SPLASH,
	TITLE_MENU,
	TITLE_OPTIONS,
} TitleSubState;


typedef struct TitleStateData
{
	TitleSubState subState;
	BOOLEAN subStateRendered;
	int menuSelection;
	int optionsSelection;
} TitleStateData;


void UpdateTitle(GameState *state, float deltaTime)
{
	TitleStateData *data = (TitleStateData *)state->data;

	switch (data->subState)
	{
		case TITLE_SPLASH:
		{
			if (IsKeyJustPressed(VK_RETURN))
			{
				ClearFrame(0);
				data->subState = TITLE_MENU;
				data->menuSelection = 0;
				data->subStateRendered = FALSE;
			}
		} break;

		case TITLE_MENU:
		{
			if (IsKeyJustPressed(VK_UP))
			{
				data->menuSelection = (data->menuSelection + 2 - 1) % 2;
				data->subStateRendered = FALSE;
			}
			if (IsKeyJustPressed(VK_DOWN))
			{
				data->menuSelection = (data->menuSelection + 1) % 2;
				data->subStateRendered = FALSE;
			}
			if (IsKeyJustPressed(VK_RETURN))
			{
				switch (data->menuSelection)
				{
					case 0: // New Game
					{
						ClearFrame(0);
						DestroyCurrentState();
						SetState(CreateNewGameState(0));

					} break;

					case 1: // Quit
					{
						PostQuitMessage(0);
					} break;
				}
			} break;
		}

		case TITLE_OPTIONS: // UNUSED
		{
			if (IsKeyJustPressed(VK_UP))
			{
				data->optionsSelection = (data->optionsSelection + 3 - 1) % 3;
				data->subStateRendered = FALSE;
			}

			if (IsKeyJustPressed(VK_DOWN))
			{
				data->optionsSelection = (data->optionsSelection + 1) % 3;
				data->subStateRendered = FALSE;
			}

			if (IsKeyJustPressed(VK_RETURN))
			{
				switch (data->optionsSelection)
				{
					case 0: // Controls
					{

					} break;

					case 1: // Audio
					{

					} break;

					case 2: // Back
					{
						ClearFrame(0);
						data->subState = TITLE_MENU;
						data->subStateRendered = FALSE;
					}
				}
			}

			if (IsKeyJustPressed(VK_ESCAPE))
			{
				ClearFrame(0);
				data->subState = TITLE_MENU;
				data->subStateRendered = FALSE;
			}
		} break;
	}
}


void RenderTitle(GameState *state)
{
	TitleStateData *data = (TitleStateData *)state->data;

	switch (data->subState)
	{
		case TITLE_SPLASH:
		{
			if (!data->subStateRendered)
			{
				DrawStringCentered("Cast & Caper:", 4, 1020, 104, 0x272727, 5);
				DrawStringCentered("Cast & Caper:", 2, 1020, 102, 0x6d6d6d, 5);
				DrawStringCentered("Cast & Caper:", 0, 1020, 100, 0xFFFFFF, 5);
				DrawStringCentered("Cuthbert's Castle", 4, 1020, 174, 0x272727, 4);
				DrawStringCentered("Cuthbert's Castle", 2, 1020, 172, 0x6d6d6d, 4);
				DrawStringCentered("Cuthbert's Castle", 0, 1020, 170, 0xFFFFFF, 4);

				DrawStringCentered("Press [ENTER] to start", 0, 1020, 360, 0XFFFFFF, 2);

				data->subStateRendered = TRUE;
			}
		} break;
		
		case TITLE_MENU:
		{
			if (!data->subStateRendered)
			{
				DrawStringCentered("Cast & Caper:", 4, 1020, 104, 0x272727, 5);
				DrawStringCentered("Cast & Caper:", 2, 1020, 102, 0x6d6d6d, 5);
				DrawStringCentered("Cast & Caper:", 0, 1020, 100, 0xFFFFFF, 5);
				DrawStringCentered("Cuthbert's Castle", 4, 1020, 174, 0x272727, 4);
				DrawStringCentered("Cuthbert's Castle", 2, 1020, 172, 0x6d6d6d, 4);
				DrawStringCentered("Cuthbert's Castle", 0, 1020, 170, 0xFFFFFF, 4);

				DrawStringCentered("New Game", 0, 1020, 280 - 25, data->menuSelection == 0 ? 0xFF0000 : 0xFFFFFF, 2);
				DrawStringCentered("Exit", 0, 1020, 280, data->menuSelection == 1 ? 0xFF0000 : 0xFFFFFF, 2);
				//DrawStringCentered("Options", 0, 1020, 280 + 25, data->menuSelection == 2 ? 0xFF0000 : 0xFFFFFF, 2);
				//DrawStringCentered("Exit", 0, 1020, 280 + 50, data->menuSelection == 3 ? 0xFF0000 : 0xFFFFFF, 2);

				DrawString("[ENTER] Continue", 860, 520, 0XFFFFFF, 1);

				data->subStateRendered = TRUE;
			}
		} break;

		case TITLE_OPTIONS:
		{
			if (!data->subStateRendered)
			{
				DrawStringCentered("Controls", 0, 1020, 280 - 25, data->optionsSelection == 0 ? 0xFF0000 : 0xFFFFFF, 2);
				DrawStringCentered("Audio", 0, 1020, 280, data->optionsSelection == 1 ? 0xFF0000 : 0xFFFFFF, 2);
				DrawStringCentered("Back", 0, 1020, 280 + 25, data->optionsSelection == 2 ? 0xFF0000 : 0xFFFFFF, 2);

				DrawString("[ENTER] Continue", 860, 520, 0XFFFFFF, 1);
				DrawString("[ESC] Back", 30, 520, 0XFFFFFF, 1);

				data->subStateRendered = TRUE;
			}
		}
	}
}


void DestroyTitle(GameState *state)
{
	if (state)
	{
		if (state->data)
			free(state->data);
		free(state);
	}
}


GameState *CreateTitleState(int subState)
{
	GameState *state = malloc(sizeof(GameState));
	if (!state)
	{
		return NULL;
	}

	TitleStateData *data = malloc(sizeof(TitleStateData));
	if (!data)
	{
		free(state);
		return NULL;
	}

	data->subState = subState;
	data->subStateRendered = FALSE;
	data->menuSelection = 0;

	state->data = data;
	state->update = UpdateTitle;
	state->render = RenderTitle;
	state->destroy = DestroyTitle;
	
	return state;
}