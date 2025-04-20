#include "class.h"
#include "colour.h"
#include "map.h"
#include "party.h"
#include "renderer.h"
#include "ui.h"
#include <malloc.h>
#include <stdio.h>
#include <string.h>


#define BORDER_COL 0XFFFFFF
#define BACKGROUND_COL 0

#define BORDER_THICKNESS 2
#define INSET (BORDER_THICKNESS * 2)
#define PADDING 16

// Left elements (view & message box)
#define LEFT_ELEMENTS_WIDTH 550
#define VIEW_HEIGHT 350
#define MESSAGE_BOX_HEIGHT 140

// Centre elements (compass, map, arrows & menu)
#define CENTRE_ELEMENTS_X (LEFT_ELEMENTS_WIDTH + (PADDING * 2))
#define CENTRE_ELEMENTS_WIDTH 160
#define METERS_WIDTH 210
#define COMPASS_HEIGHT 30
#define MENU_HEIGHT 122
#define INFOBAR_HEIGHT 30
#define ARROWS_HEIGHT 102

// Right elements (party)
#define RIGHT_ELEMENTS_X (LEFT_ELEMENTS_WIDTH + CENTRE_ELEMENTS_WIDTH + (PADDING * 3))


const unsigned char arrows[6][8] = {
	{ 0x18, 0x3C, 0x7E, 0xFF, 0xFF, 0x3C, 0x3C, 0x3C},  // UP
	{ 0x3C, 0x3C, 0x3C, 0xFF, 0xFF, 0x7E, 0x3C, 0x18},  // DOWN
	{ 0x18, 0x1C, 0xFE, 0xFF, 0xFF, 0xFE, 0x1C, 0x18 }, // RIGHT
	{ 0x18, 0x38, 0x7F, 0xFF, 0xFF, 0x7F, 0x38, 0x18 }, // LEFT
	{ 0x0C, 0x0E, 0xFF, 0xFF, 0xCE, 0xCC, 0xC0, 0xC0 },  // TURN RIGHT
	{ 0x30, 0x70, 0xFF, 0xFF, 0x73, 0x33, 0x03, 0x03 } // TURN LEFT
};


MessageLog msgLog = {
	.count = 0,
	.start = 0,
	.messages = { NULL }
};

static void DrawViewFrame(void)
{
	DrawGreyscaleRectangle(
		PADDING, 
		PADDING, 
		LEFT_ELEMENTS_WIDTH, 
		VIEW_HEIGHT,
		BORDER_COL
	);

	DrawGreyscaleRectangle(
		PADDING + BORDER_THICKNESS, 
		PADDING + BORDER_THICKNESS, 
		LEFT_ELEMENTS_WIDTH - INSET, 
		VIEW_HEIGHT - INSET,
		BACKGROUND_COL
	);
}

static void DrawMessageBox(void)
{
	DrawGreyscaleRectangle(
		PADDING, 
		VIEW_HEIGHT + (PADDING * 2),
		LEFT_ELEMENTS_WIDTH,
		MESSAGE_BOX_HEIGHT,
		BORDER_COL
	);
	
	DrawGreyscaleRectangle(
		PADDING + BORDER_THICKNESS,
		VIEW_HEIGHT + (PADDING * 2) + BORDER_THICKNESS,
		LEFT_ELEMENTS_WIDTH - (BORDER_THICKNESS * 2),
		MESSAGE_BOX_HEIGHT - (BORDER_THICKNESS * 2),
		BACKGROUND_COL
	);
}



void DrawCompass(const int dir)
{
	DrawGreyscaleRectangle(
		CENTRE_ELEMENTS_X, 
		PADDING, 
		CENTRE_ELEMENTS_WIDTH,
		COMPASS_HEIGHT, 
		BORDER_COL
	);
	
	DrawGreyscaleRectangle(
		CENTRE_ELEMENTS_X + BORDER_THICKNESS,
		PADDING + BORDER_THICKNESS, 
		CENTRE_ELEMENTS_WIDTH - INSET,
		COMPASS_HEIGHT - INSET, 
		BACKGROUND_COL);

	const char *directions[4] = { "N", "E", "S", "W" };

	DrawStringCentered(directions[dir], CENTRE_ELEMENTS_X, CENTRE_ELEMENTS_X + CENTRE_ELEMENTS_WIDTH, PADDING + 8, 0XFFFFFF, 2);
	DrawString(directions[(dir + 4 - 1) % 4], CENTRE_ELEMENTS_X + 8, PADDING + 8, 0XFFFFFF, 2);
	DrawString(directions[(dir + 1) % 4], CENTRE_ELEMENTS_X + CENTRE_ELEMENTS_WIDTH - 22, PADDING + 8, 0XFFFFFF, 2);
}

void DrawMap(MapData *map)
{
	// Draw the map panel background:
	DrawGreyscaleRectangle(
		CENTRE_ELEMENTS_X,
		COMPASS_HEIGHT + (PADDING - BORDER_THICKNESS),
		CENTRE_ELEMENTS_WIDTH, // keep map square
		CENTRE_ELEMENTS_WIDTH,
		BORDER_COL
	);

	DrawGreyscaleRectangle(
		CENTRE_ELEMENTS_X + BORDER_THICKNESS,
		COMPASS_HEIGHT + PADDING,
		CENTRE_ELEMENTS_WIDTH - INSET,
		CENTRE_ELEMENTS_WIDTH - INSET,
		BACKGROUND_COL
	);

	const int tileCount = 16;
	const int availableSize = CENTRE_ELEMENTS_WIDTH - INSET;
	const int tileSize = (int)availableSize / tileCount;
	const int offset = (availableSize - (tileSize * tileCount)) / 2;
	const int baseX = CENTRE_ELEMENTS_X + BORDER_THICKNESS + offset;
	const int baseY = COMPASS_HEIGHT + PADDING + offset;

	for (int row = 0; row < tileCount; row++) 
	{
		for (int col = 0; col < tileCount; col++) 
		{
			if (map->tiles[row][col] == 1) {
				int tileX = baseX + col * tileSize;
				int tileY = baseY + row * tileSize;
				DrawGreyscaleRectangle(tileX, tileY, tileSize, tileSize, 0xDDDDDD);
			}
			else if (map->decorations[row][col] == 5) {
				int tileX = baseX + col * tileSize;
				int tileY = baseY + row * tileSize;
				DrawColourRectangle(tileX, tileY, tileSize, tileSize, 0x90d36f);
			}
		}
	}

	DrawGreyscaleRectangle(CENTRE_ELEMENTS_X, 210, CENTRE_ELEMENTS_WIDTH, 12, 0);

	DrawStringCentered(
		LoadMapName(mapIndex), 
		CENTRE_ELEMENTS_X, 
		CENTRE_ELEMENTS_X + CENTRE_ELEMENTS_WIDTH, 
		212, 
		BORDER_COL, 
		1
	);
}


static DrawMenu(void)
{
	DrawGreyscaleRectangle(
		CENTRE_ELEMENTS_X,
		COMPASS_HEIGHT + CENTRE_ELEMENTS_WIDTH + (PADDING * 2) - BORDER_THICKNESS + 24,
		CENTRE_ELEMENTS_WIDTH,
		MENU_HEIGHT,
		BORDER_COL
	);

	DrawGreyscaleRectangle(
		CENTRE_ELEMENTS_X + BORDER_THICKNESS,
		COMPASS_HEIGHT + CENTRE_ELEMENTS_WIDTH + (PADDING * 2) + 24,
		CENTRE_ELEMENTS_WIDTH - INSET,
		MENU_HEIGHT - INSET,
		0
	);

	const char *menuStr[6] =
	{
		"[P] Party",
		"[G] Disenchant",
		"[C] Craft",
		"[H] Sneak",
		"[L] Clear Messages",
	};

	const int sX = CENTRE_ELEMENTS_X + BORDER_THICKNESS;

	for (int i = 0; i < 5; i++)
	{
		const int sY = COMPASS_HEIGHT + CENTRE_ELEMENTS_WIDTH + (PADDING * 2) + (24 * i) + 47;
		DrawThickLine(sX, sY, sX + CENTRE_ELEMENTS_WIDTH - (BORDER_THICKNESS * 2), sY, 0XFFFFFF, 2);
	}

	for (int j = 0; j < 5; j++)
	{
		const int sY = COMPASS_HEIGHT + CENTRE_ELEMENTS_WIDTH + (PADDING * 2) + (24 * j) + 24;
		DrawStringCentered(menuStr[j], sX, sX + CENTRE_ELEMENTS_WIDTH - (BORDER_THICKNESS * 2), sY + 8, 0XDDDDDD, 1);
	}


}

static void DrawInfoBar(void)
{
	DrawGreyscaleRectangle(
		CENTRE_ELEMENTS_X,
		VIEW_HEIGHT + (PADDING * 2),
		CENTRE_ELEMENTS_WIDTH,
		INFOBAR_HEIGHT,
		BORDER_COL
	);

	DrawGreyscaleRectangle(
		CENTRE_ELEMENTS_X + BORDER_THICKNESS,
		VIEW_HEIGHT + (PADDING * 2) + BORDER_THICKNESS,
		CENTRE_ELEMENTS_WIDTH - INSET,
		INFOBAR_HEIGHT - INSET,
		0
	);
}


static void DrawArrows(void)
{
	DrawGreyscaleRectangle(
		CENTRE_ELEMENTS_X, 
		VIEW_HEIGHT + INFOBAR_HEIGHT + 40, // + (PADDING * 2.5) = 40
		CENTRE_ELEMENTS_WIDTH,
		ARROWS_HEIGHT,
		BORDER_COL
	);

	DrawGreyscaleRectangle(
		CENTRE_ELEMENTS_X + BORDER_THICKNESS,
		VIEW_HEIGHT + INFOBAR_HEIGHT + 40 + BORDER_THICKNESS, // // + (PADDING * 2.5) = 40
		CENTRE_ELEMENTS_WIDTH - INSET,
		ARROWS_HEIGHT - INSET,
		0
	);

	DrawBitMap(arrows[0], CENTRE_ELEMENTS_X + 63, VIEW_HEIGHT + (PADDING * 2) + 51, 0XDDDDDD, 4);  // UP
	DrawBitMap(arrows[1], CENTRE_ELEMENTS_X + 63, VIEW_HEIGHT + (PADDING * 2) + 95, 0XDDDDDD, 4);  // DOWN
	DrawBitMap(arrows[2], CENTRE_ELEMENTS_X + 14, VIEW_HEIGHT + (PADDING * 2) + 95, 0XDDDDDD, 4);  // LEFT
	DrawBitMap(arrows[3], CENTRE_ELEMENTS_X + 112, VIEW_HEIGHT + (PADDING * 2) + 95, 0XDDDDDD, 4); // RIGHT
	DrawBitMap(arrows[4], CENTRE_ELEMENTS_X + 14, VIEW_HEIGHT + (PADDING * 2) + 51, 0XDDDDDD, 4);  // TURN RIGHT
	DrawBitMap(arrows[5], CENTRE_ELEMENTS_X + 112, VIEW_HEIGHT + (PADDING * 2) + 51, 0XDDDDDD, 4); // TURN LEFT
}

void DrawArrow(const int i, const uint32_t col)
{
	switch (i)
	{
		case 0:
		{
			DrawBitMap(arrows[0], CENTRE_ELEMENTS_X + 63, VIEW_HEIGHT + (PADDING * 2) + 51, col, 4);  // UP
		} break;
		case 1:
		{
			DrawBitMap(arrows[1], CENTRE_ELEMENTS_X + 63, VIEW_HEIGHT + (PADDING * 2) + 95, col, 4);  // DOWN
		} break;
		case 2:
		{
			DrawBitMap(arrows[2], CENTRE_ELEMENTS_X + 14, VIEW_HEIGHT + (PADDING * 2) + 95, col, 4);  // LEFT
		} break;
		case 3:
		{
			DrawBitMap(arrows[3], CENTRE_ELEMENTS_X + 112, VIEW_HEIGHT + (PADDING * 2) + 95, col, 4); // RIGHT
		} break;
		case 4:
		{
			DrawBitMap(arrows[4], CENTRE_ELEMENTS_X + 14, VIEW_HEIGHT + (PADDING * 2) + 51, col, 4);  // TURN RIGHT
		} break;
		case 5:
		{
			DrawBitMap(arrows[5], CENTRE_ELEMENTS_X + 112, VIEW_HEIGHT + (PADDING * 2) + 51, col, 4); // TURN LEFT
		} break;

	}
}


void DrawPartyMemberFrame(const int i, bool turn)
{
	const Class *classData = party[i].classData;
	Role role = classData->specs[party[i].specIndex].role;
	int backgroundColour = roleBackground[role];
	int borderColour = turn ? 0XFF0000 : 0XFFFFFF;

	const int height = 60;
	const int width = 60;
	const int padding = 7;

	int y = PADDING + ((height + padding) * i);

	DrawColourRectangle(
		RIGHT_ELEMENTS_X,
		y,
		width,
		height,
		borderColour
	);

	DrawColourRectangle(
		RIGHT_ELEMENTS_X + BORDER_THICKNESS,
		y + BORDER_THICKNESS,
		width - INSET,
		height - INSET,
		backgroundColour
	);

	DrawSprite(
		RIGHT_ELEMENTS_X + BORDER_THICKNESS,
		y + BORDER_THICKNESS,
		width - INSET,
		width - INSET,
		classData->sprite,
		classData->specs[party[i].specIndex].palette
	);
}


void DrawDeadPartyMemberFrame(const int i)
{
	const Class *classData = party[i].classData;

	const int height = 60;
	const int width = 60;
	const int padding = 7;
	int y = PADDING + ((height + padding) * i);

	uint32_t greyscalePalette[7];
	ConvertPaletteToGreyscale(classData->specs[party[i].specIndex].palette, greyscalePalette);

	DrawGreyscaleRectangle(
		RIGHT_ELEMENTS_X,
		y,
		width,
		height,
		0XFFFFFF
	);

	DrawGreyscaleRectangle(
		RIGHT_ELEMENTS_X + BORDER_THICKNESS,
		y + BORDER_THICKNESS,
		width - INSET,
		height - INSET,
		0X515151
	);

	DrawSprite(
		RIGHT_ELEMENTS_X + BORDER_THICKNESS,
		y + BORDER_THICKNESS,
		width - INSET,
		width - INSET,
		classData->sprite,
		greyscalePalette
	);
}


void DrawSecondaryProgressBar(int partyIndex)
{
	const uint32_t secondaryColours[NUM_SECONDARY] = { 0X0055ff, 0Xff8000, 0xb71b1b, 0xffe800, 0xb130ff };

	const int y = PADDING + ((60 + 7) * partyIndex);
	const int progressBarX = RIGHT_ELEMENTS_X + BORDER_THICKNESS + 60 + 4;
	const int progressBarY = y + 13;
	const int progressBarHeight = 12;

	const PartyMember *member = &party[partyIndex];
	const SecondaryType type = member->classData->specs[member->specIndex].secondaryType;

	const uint32_t barColour = secondaryColours[type];
	const uint32_t borderColour = DarkenColour(barColour, 0.5f);

	float secondaryPercentage = ((float)party[partyIndex].secondary.x / party[partyIndex].secondary.y) * 100;
	char secondaryBuf[20] = { 0 };
	sprintf_s(secondaryBuf, sizeof(secondaryBuf), "%i\\%i", party[partyIndex].secondary.x, party[partyIndex].secondary.y);
	DrawProgressBar(progressBarX, 
					progressBarY + progressBarHeight + 1, 128, 
					progressBarHeight, 
					secondaryPercentage, 
					barColour,
					borderColour
	);
	DrawStringCentered(secondaryBuf, progressBarX, progressBarX + 128, progressBarY + progressBarHeight + 3, 0, 1);
}


void DrawXPProgressBar(int partyIndex)
{
	const int y = PADDING + ((60 + 7) * partyIndex);
	const int progressBarX = RIGHT_ELEMENTS_X + BORDER_THICKNESS + 60 + 4;
	const int progressBarY = y + 13;
	const int progressBarHeight = 12;

	char xpBuf[20] = { 0 };
	if (party[partyIndex].level == 10)
	{
		sprintf_s(xpBuf, sizeof(xpBuf), "%i", party[partyIndex].xp.x);
		DrawProgressBar(progressBarX, progressBarY + ((progressBarHeight + 2) * 2), 128, progressBarHeight, 100, 0Xffa600, 0Xbb7a00);
		DrawStringCentered(xpBuf, progressBarX, progressBarX + 128, progressBarY + ((progressBarHeight + 2) * 2) + 2, 0, 1);
	}
	else
	{
		float xpPercentage = ((float)party[partyIndex].xp.x / party[partyIndex].xp.y) * 100;
		sprintf_s(xpBuf, sizeof(xpBuf), "%i\\%i", party[partyIndex].xp.x, party[partyIndex].xp.y);
		DrawProgressBar(progressBarX, progressBarY + ((progressBarHeight + 2) * 2), 128, progressBarHeight, xpPercentage, 0Xffa600, 0Xbb7a00);
		DrawStringCentered(xpBuf, progressBarX, progressBarX + 128, progressBarY + ((progressBarHeight + 2) * 2) + 2, 0, 1);
	}
}


void DrawPartyFrame(void)
{
	const int height = 60;
	const int width = 60;
	const int padding = 7;

	for (int i = 0; i < 6; i++)
	{
		DrawPartyMemberFrame(i, false);

		if (party[i].health.x == 0)
		{
			DrawDeadPartyMemberFrame(i);
			DrawDeadPartyMemberIndicator(i);
			continue;
		}

		int y = PADDING + ((height + padding) * i);

		// Progress bars
		const int progressBarX = RIGHT_ELEMENTS_X + BORDER_THICKNESS + width + 4;
		const int progressBarY = y + 13;
		const int progressBarHeight = 12;

		// Title Text
		char roleBitmap[2] = { 0 };
		int roleColour = 0;

		switch (party[i].classData->specs[party[i].specIndex].role)
		{
			case ROLE_TANK:
			{
				roleBitmap[0] = '#';
				roleColour = 0X3242DF;
			} break;
			case ROLE_HEALER:
			{
				roleBitmap[0] = '+';
				roleColour = 0X3AD41E;
			} break;
			case ROLE_DPS:
			{
				roleBitmap[0] = '/';
				roleColour = 0XD72020;
			} break;
			case ROLE_SUPPORT:
			{
				roleBitmap[0] = '^';
				roleColour = 0XF0D02F;
			} break;
		}

		DrawString(roleBitmap, progressBarX, progressBarY - progressBarHeight, roleColour, 1);
		DrawString(party[i].classData->name, progressBarX + 12, progressBarY - progressBarHeight, 0XFFFFFF, 1);

		// Health
		//float healthPercentage = ((float)party[i].health.x / party[i].health.y) * 100;
		//char healthBuf[20] = { 0 };
		//sprintf_s(healthBuf, sizeof(healthBuf), "%i\\%i", party[i].health.x, party[i].health.y);
		//DrawProgressBar(progressBarX, progressBarY - 2, 128, progressBarHeight, healthPercentage, 0Xca0000, 0X600000);
		//DrawStringCentered(healthBuf, progressBarX, progressBarX + 128, progressBarY, 0, 1);
		DrawHealthBar(i);
		
		// Secondary Resource
		//float secondaryPercentage = ((float)party[i].secondary.x / party[i].secondary.y) * 100;
		//char secondaryBuf[20] = { 0 };
		//sprintf_s(secondaryBuf, sizeof(secondaryBuf), "%i\\%i", party[i].secondary.x, party[i].secondary.y);
		//DrawProgressBar(progressBarX, progressBarY + progressBarHeight + 2, 128, progressBarHeight, secondaryPercentage, 0X1a29c9, 0X122289);
		//DrawStringCentered(secondaryBuf, progressBarX, progressBarX + 128, progressBarY + progressBarHeight + 4, 0, 1);
		DrawSecondaryProgressBar(i);

		// XP
		DrawXPProgressBar(i);
	}
}


static uint32_t GetHealthBarColour(float healthPercentage) {
	if (healthPercentage >= 75.0f) 
	{
		// Map 75-100%: yellow at 75% to green at 100%
		float t = (healthPercentage - 75.0f) / 25.0f;
		return InterpolateColour(0xf2cd12, 0x0dd100, t);
	}
	else if (healthPercentage >= 50.0f) 
	{
		// Map 50-75%: orange at 50% to yellow at 75%
		float t = (healthPercentage - 50.0f) / 25.0f;
		return InterpolateColour(0xe45d05, 0xf2cd12, t);
	}
	else 
	{
		// Map 0-50%: red at 0% to orange at 50%
		float t = healthPercentage / 50.0f;
		return InterpolateColour(0xd70000, 0xe45d05, t);
	}
}



void DrawHealthBar(int index)
{
	const int progressBarY = (67 * index) + PADDING + 13;
	const int progressBarX = RIGHT_ELEMENTS_X + BORDER_THICKNESS + 64;

	float healthPercentage = ((float)party[index].health.x / party[index].health.y) * 100;
	const uint32_t barColour = GetHealthBarColour(healthPercentage);
	const uint32_t borderColour = DarkenColour(barColour, 0.5f);

	char healthBuf[50] = { 0 };
	sprintf_s(healthBuf, sizeof(healthBuf), "%i\\%i", party[index].health.x, party[index].health.y);
	DrawProgressBar(progressBarX, progressBarY - 2, 128, 12, healthPercentage, barColour, borderColour);
	DrawStringCentered(healthBuf, progressBarX, progressBarX + 128, progressBarY, 0, 1);
}

void DrawDeadPartyMemberIndicator(const int index)
{
	const int y = (67 * index) + 16 + 11;
	const int x = (550 + 160 + (16 * 3)) + 2 + 64;
	DrawGreyscaleRectangle(x, y, 128, 49, 0XDDDDDD);
	DrawGreyscaleRectangle(x + 1, y + 1, 126, 47, 0X1A1A1A);
	DrawStringCentered("DEAD", x, x + 128, y + 18, 0XFFFFFF, 2);

	DrawGreyscaleRectangle(x + 129, y, 50, 50, 0);
}


static void DrawMessages(void)
{
	const int messageBoxPadding = 8;
	const MessageLog *log = &msgLog;

	for (int i = 0; i < log->count; i++)
	{
		int index = (log->start + i) % MAX_MESSAGES;
		if (log->messages[index] != NULL)
		{
			DrawGreyscaleRectangle(
				PADDING + BORDER_THICKNESS + messageBoxPadding,
				VIEW_HEIGHT + (PADDING * 2) + messageBoxPadding + BORDER_THICKNESS + (14 * i),
				LEFT_ELEMENTS_WIDTH - messageBoxPadding - (BORDER_THICKNESS * 2),
				8,
				0
			);

			DrawString(
				log->messages[index]->message,
				PADDING + BORDER_THICKNESS + messageBoxPadding,
				VIEW_HEIGHT + (PADDING * 2) + messageBoxPadding + BORDER_THICKNESS + (14 * i),
				log->messages[index]->colour,
				1
			);
		}
	}
}


static void DrawCombatMeters(void)
{
	DrawGreyscaleRectangle(
		CENTRE_ELEMENTS_X + METERS_WIDTH,
		VIEW_HEIGHT + INFOBAR_HEIGHT + 40,
		8,
		ARROWS_HEIGHT,
		0
	);

	DrawColourRectangle(
		CENTRE_ELEMENTS_X,
		VIEW_HEIGHT + INFOBAR_HEIGHT + 40, // + (PADDING * 2.5) = 40
		METERS_WIDTH,
		ARROWS_HEIGHT,
		0XDD0000
	);

	DrawString("Damage Done", CENTRE_ELEMENTS_X + 6, VIEW_HEIGHT + INFOBAR_HEIGHT + 44, 0XFFFFFF, 1);

	DrawGreyscaleRectangle(
		CENTRE_ELEMENTS_X + BORDER_THICKNESS,
		VIEW_HEIGHT + INFOBAR_HEIGHT + 40 + BORDER_THICKNESS + 14,
		METERS_WIDTH - INSET,
		ARROWS_HEIGHT - INSET - 14,
		0
	);

	DrawColourRectangle(
		CENTRE_ELEMENTS_X + 8 + METERS_WIDTH,
		VIEW_HEIGHT + INFOBAR_HEIGHT + 40, // + (PADDING * 2.5) = 40
		METERS_WIDTH,
		ARROWS_HEIGHT,
		0X159000
	);

	DrawString("Healing Done", CENTRE_ELEMENTS_X + 8 + METERS_WIDTH + 6, VIEW_HEIGHT + INFOBAR_HEIGHT + 44, 0XFFFFFF, 1);

	DrawGreyscaleRectangle(
		CENTRE_ELEMENTS_X + BORDER_THICKNESS + 8 + METERS_WIDTH,
		VIEW_HEIGHT + INFOBAR_HEIGHT + 40 + BORDER_THICKNESS + 14, // // + (PADDING * 2.5) = 40
		METERS_WIDTH - INSET,
		ARROWS_HEIGHT - INSET - 14,
		0
	);


	//const int barHeight = 14;
	//const int sx = CENTRE_ELEMENTS_X + BORDER_THICKNESS + CENTRE_ELEMENTS_WIDTH + PADDING;
	//const int y = VIEW_HEIGHT + INFOBAR_HEIGHT + 40 + BORDER_THICKNESS + 14;
	//for (int i = 0; i < 6; i++)
	//{
	//	int sy = y + (barHeight * i);
	//	DrawColourRectangle(sx, sy, CENTRE_ELEMENTS_WIDTH - INSET, barHeight, party[i].classData->colour);
	//}
}


void DrawCentreCombatUI(void)
{
	DrawGreyscaleRectangle(CENTRE_ELEMENTS_X, PADDING, CENTRE_ELEMENTS_WIDTH, 516, 0);

	DrawGreyscaleRectangle(CENTRE_ELEMENTS_X, PADDING, CENTRE_ELEMENTS_WIDTH, VIEW_HEIGHT + 45, 0X777777);
	DrawGreyscaleRectangle(CENTRE_ELEMENTS_X + BORDER_THICKNESS, PADDING + 14, CENTRE_ELEMENTS_WIDTH - (BORDER_THICKNESS * 2), VIEW_HEIGHT + 29, 0);
	DrawString("Action Menu", CENTRE_ELEMENTS_X + 6, PADDING + 3, 0XFFFFFF, 1);

	DrawCombatMeters();
}


void DrawActionMenuEnemy(void)
{
	DrawGreyscaleRectangle(CENTRE_ELEMENTS_X + BORDER_THICKNESS, PADDING + 14, CENTRE_ELEMENTS_WIDTH - (BORDER_THICKNESS * 2), VIEW_HEIGHT + 29, 0X333333);
	DrawStringCentered("Enemy turn", CENTRE_ELEMENTS_X, CENTRE_ELEMENTS_X + CENTRE_ELEMENTS_WIDTH, 200, 0XFFFFFF, 1);
	DrawStringCentered("Please wait", CENTRE_ELEMENTS_X, CENTRE_ELEMENTS_X + CENTRE_ELEMENTS_WIDTH, 214, 0XFFFFFF, 1);
}

static void DrawAbilityInfo(Ability *ability)
{
	const AbilitySubtype abilType = ability->subtype;
	const int abilCost = ability->cost;
	const int abilCooldown = ability->cooldown;

	const char *costLabel = (abilCost < 0) ? "GAIN" : "COST";
	const char *gcdLabel = (ability->offGcd) ? "INSTANT" : "TURN";

	const int sX = CENTRE_ELEMENTS_X + BORDER_THICKNESS + 4;

	char infoStr[128];
	snprintf(infoStr, sizeof(infoStr), "%s:%d CD:%d", costLabel, abilCost, abilCooldown);
	DrawString(infoStr, sX, 298, 0xDDDDDD, 1);
	snprintf(infoStr, sizeof(infoStr), "TYPE:%s", gcdLabel);
	DrawString(infoStr, sX, 312, 0xDDDDDD, 1);
	DrawStringWrapped(ability->description, sX, 328, CENTRE_ELEMENTS_WIDTH - 6, 0, 0xDDDDDD, 1);
}


bool DrawActionMenuParty(PartyMember *member, MemberState *state, int selectedIdx, int prevIdx)
{
	const Spec *spec = &member->classData->specs[member->specIndex];
	
	DrawGreyscaleRectangle(CENTRE_ELEMENTS_X + BORDER_THICKNESS, PADDING + 14, CENTRE_ELEMENTS_WIDTH - (BORDER_THICKNESS * 2), VIEW_HEIGHT + 29, 0);

	bool hasUsableAbility = false;
	for (int i = 0; i < 10; i++)
	{
		bool levelReqMet = member->level > i;
		bool costReqMet = spec->abilities[i].cost <= member->secondary.x;
		bool notOnCD = state->cooldownState[i] == 0;		
		bool allReqsMet = false;

		const char *abilityName = NULL;
		uint32_t colBorder;

		if (levelReqMet && costReqMet && notOnCD)
		{
			allReqsMet = true;
			hasUsableAbility = true;
			abilityName = spec->abilities[i].name;
			colBorder = i == selectedIdx ? 0XFFFFFF : 0X780000;
		}
		else
		{
			if (!levelReqMet)
			{
				abilityName = "LOCKED";
			}
			else if (!costReqMet)
			{
				abilityName = "NEED RESOURCE";
			}
			else if (!notOnCD)
			{
				abilityName = "ON COOLDOWN";
			}

			colBorder = i == selectedIdx ? 0XFFFFFF : 0X505050;
		}

		uint32_t colBackground = allReqsMet ? 0XBD0000 : 0X393939;
		uint32_t colString = allReqsMet ? 0XFFFFFF : 0;

		DrawColourRectangle(CENTRE_ELEMENTS_X + 8, ((18 + 8) * i) + (PADDING + 14) + 9, CENTRE_ELEMENTS_WIDTH - 16, 18, colBorder);
		DrawColourRectangle(CENTRE_ELEMENTS_X + 9, ((18 + 8) * i) + (PADDING + 14) + 10, CENTRE_ELEMENTS_WIDTH - 18, 16, colBackground);
		DrawStringCentered(abilityName,
						   CENTRE_ELEMENTS_X + 8, 
						   CENTRE_ELEMENTS_X + CENTRE_ELEMENTS_WIDTH - 16, 
						   ((18 + 8) * i) + (PADDING + 14) + 14, 
						   colString,
						   1
		);
	}

	DrawAbilityInfo(&spec->abilities[selectedIdx]);

	return hasUsableAbility;
}


void PushMessage(const char *msg, const int colour)
{
	int index;
	MessageLog *log = &msgLog;

	if (log->count < MAX_MESSAGES)
	{
		index = (log->start + log->count) % MAX_MESSAGES;
		log->count++;
	}
	else
	{
		index = log->start;
		log->start = (log->start + 1) % MAX_MESSAGES;
	}

	if (log->messages[index])
	{
		free(log->messages[index]->message);
	}
	else
	{
		// Allocate new Message struct if it doesn't exist
		log->messages[index] = malloc(sizeof(Message));
		if (!log->messages[index]) return; // Handle allocation failure
	}

	log->messages[index]->message = _strdup(msg);
	log->messages[index]->colour = colour;

	DrawMessages();
}


void ClearMessageLog()
{
	MessageLog *log = &msgLog;

	for (int i = 0; i < MAX_MESSAGES; i++) 
	{
		if (log->messages[i] != NULL) 
		{
			free(log->messages[i]->message);
			free(log->messages[i]);
			log->messages[i] = NULL;
		}
	}
	log->count = 0;
	log->start = 0;

	DrawMessageBox();
}


void DrawFullUI(const int dir) 
{
	DrawViewFrame();
	DrawMessageBox();
	
	DrawCompass(dir);
	DrawMenu();
	//DrawInfoBar();
	DrawArrows();
	DrawPartyFrame();

	DrawMessages();
}
