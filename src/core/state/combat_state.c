#include "ability.h"
#include "animation.h"
#include "colour.h"
#include "difficulty.h"
#include "enemy.h"
#include "input.h"
#include "loot_state.h"
#include "map.h"
#include "renderer.h"
#include "state.h"
#include "state_manager.h"
#include "movement_state.h"
#include "ui.h"
#include <stdint.h>
#include <stdio.h>
#include <windows.h>


typedef enum
{
	COMBAT_TRANSITION,		// Transition from movement_state (ambush popup)
	COMBAT_SETUP,			// pre-fight setup, create enemies, UI & action rolls
	COMBAT_ALLOCATE_TURN,	// assign next turn and setup the requisite UI
	COMBAT_ENEMY_TURN,		// enemy takes turn automatically
	COMBAT_PARTY_TURN,		// setup UI and handle user action
	COMBAT_END,				// fight over, handle win 
	COMBAT_WIPE,
} CombatSubState;

typedef struct CombatStateData
{
	const Enemy *enemies[6];
	EnemyState enemyState[6];
	MemberState partyState[6];

	float turnCD[12];
	float prevTurnCD[12];

	float globalCycleProgress;
	float prevGlobalCycleProgess;
	int globalCycle;

	CombatSubState subState;
	float animationTime;
	int8_t numEnemies;
	int8_t nextTurn;
	int8_t actionSelection;
	int8_t enemyTargetSelection;
	int allyTargetSelection;

	bool selectingAction;
	bool targetingEnemy;
	bool targetingAlly;

	bool doneTurnTick;
	bool subStateRendered;
	bool subStateProcessed;
} CombatStateData;


// Private function prototypes
static void GenerateEnemies(CombatStateData *data, int enemyCountByType[3]);
static void GenerateAmbushMessage(CombatStateData *data, const int enemyCountByType[3]);
static void DrawEnemy(const int index, const int numEnemies, const Enemy *enemy, const EnemyState *enemyState, const float turnCD);
static void RedrawEnemySprite(const unsigned char (*sprite)[4], const index, const numEnemies);
static void DrawEnemyHealthBar(const int health, const int maxHealth, const int x, const int y);
static void DrawEnemyActionBar(const float turnCD, const int x, const int y);
static void DrawEnemyTargetIndicator(const int target, const int numEnemies);
static void DrawAllyActionBar(const float turnCD, const int index);
static void DrawAllyTargetIndicator(const int allyTargetSelection);
static void RedrawAllAllyStatusIcons(CombatStateData *data);
static void DrawGlobalCycleBar(const float cycleProgress, const int cycleCount);
static void RollTurn(const uint8_t index, CombatStateData *data, float *turnCD);
static void InitNextTurn(CombatStateData *data);
static void GenerateTurnOrder(CombatStateData *data);
static void SetNextTurn(CombatStateData *data);
static void EndTurn(CombatStateData *data);
static void GetEnemyPosition(const int enemyIndex, const int totalEnemies, int *x, int *y);
static void GetNewEnemyTarget(CombatStateData *data);

static void InitMemberState(MemberState *state);
static void InitCombatStats(PartyMember *member, MemberState *state);

static void UpdateStatusEffectsUILayout(CombatStateData *data, const int targetIdx);

static void IncrementGlobalCycle(CombatStateData *data);

static void CheckPartyWipe(CombatStateData *data);


void UpdateCombat(struct GameState *state, float deltaTime)
{
	CombatStateData *data = (CombatStateData *)state->data;

	switch (data->subState)
	{
		case COMBAT_TRANSITION: // Transition from movement_state (ambush popup)
		{
			data->animationTime += deltaTime;
		} break;

		case COMBAT_SETUP: // pre-fight setup, create enemies, UI & action rolls
		{
			if (!data->subStateProcessed)
			{
				int enemyCountByType[3] = { 0, 0, 0 };

				GenerateEnemies(data, enemyCountByType);
				GenerateAmbushMessage(data, enemyCountByType);
				PushMessage("Press [ENTER] to begin combat", 0X00FF00);

				GenerateTurnOrder(data);

				data->subStateProcessed = true;
			}
			else // Once enemies have been generated, await user input to start combat
			{
				if (IsKeyJustPressed(VK_RETURN))
				{
					ClearMessageLog();
					data->subStateProcessed = false;
					data->animationTime = 0.016f;
					data->subState = COMBAT_ALLOCATE_TURN;
				}
			}
		} break;

		case COMBAT_ALLOCATE_TURN: // assign next turn and setup the requisite UI
		{
			if (!data->subStateProcessed)
			{
				SetNextTurn(data);
				InitNextTurn(data);

				data->animationTime = 0.0f;
				data->subStateRendered = false;
				data->subStateProcessed = true;
			}

			data->animationTime += deltaTime;
		} break;

		case COMBAT_ENEMY_TURN: // enemy takes turn automatically
		{
			if (!data->subStateProcessed)
			{
				const int nextTurn = data->nextTurn;

				RollTurn(nextTurn, data, &data->turnCD[nextTurn]);
				DoEnemyAbility(data->enemies[nextTurn - 6], (EnemyState *)&data->enemyState[nextTurn - 6], data->partyState);
				CheckPartyWipe(data);
				
				data->animationTime = 0.0f;
				data->subStateProcessed = true;
			}

			data->animationTime += deltaTime;
		} break;
		
		case COMBAT_PARTY_TURN: // setup UI and handle user action
		{
			if (!data->doneTurnTick)
			{
				bool fullRedraw = TickOverTimeEffects(&data->partyState[data->nextTurn], data->partyState, data->enemyState, data->nextTurn);
				UpdateCombatMeters(data->partyState, data->globalCycle);
				DrawXPProgressBar(data->nextTurn);

				if (fullRedraw)
				{
					RedrawAllAllyStatusIcons(data);
				}

				for (size_t i = 0; i < data->numEnemies; i++)
				{
					int x, y;
					GetEnemyPosition(i, data->numEnemies, &x, &y);
					
					if (data->enemyState[i].greyOutNeeded)
					{
						DrawBitMap32x32(data->enemies[i]->sprite, x, y, 0x282828, 4);
						DrawGreyscaleRectangle(x, y + 132, 140, 38, 0);
						GetNewEnemyTarget(data);

						data->enemyState[i].greyOutNeeded = false;
					}
					else if (data->enemyState[i].alive)
					{
						DrawEnemyHealthBar(data->enemyState[i].health, data->enemies[i]->health[gameDifficulty], x, y);
					}
				}

				MemberState *memberState = &data->partyState[data->nextTurn];

				for (size_t i = 0; i < NUM_ABILITES; i++)
				{
					if (party[data->nextTurn].classData->specs[party[data->nextTurn].specIndex].abilities[i].offGcd)
					{
						continue;
					}
					else 
					{
						if (memberState->cooldownState[i] > 0)
						{
							memberState->cooldownState[i] -= 1;
						}
					}
				}

				data->doneTurnTick = true;
			}

			if (!data->subStateProcessed)
			{
				if (data->selectingAction) // Selecting party member ability
				{
					if (IsKeyJustPressed(VK_UP))
					{
						data->actionSelection = (data->actionSelection + party[data->nextTurn].level - 1) % party[data->nextTurn].level;
						data->subStateRendered = false;
					}

					if (IsKeyJustPressed(VK_DOWN))
					{
						data->actionSelection = (data->actionSelection + 1) % party[data->nextTurn].level;
						data->subStateRendered = false;
					}

					if (IsKeyJustPressed(VK_RETURN))
					{
						const PartyMember *member = &party[data->nextTurn];

						if (!CanUseAbility(member,
							&data->partyState[data->nextTurn],
							&member->classData->specs[member->specIndex].abilities[data->actionSelection],
							data->actionSelection))
						{
							return;
						}

						data->selectingAction = false;
						data->subStateRendered = false;

						AbilityType type = party[data->nextTurn].classData->specs[party[data->nextTurn].specIndex].abilities[data->actionSelection].type;

						if (type == ABILITY_DAMAGE || type == ABILITY_DEBUFF)
						{
							data->targetingEnemy = true;
							data->targetingAlly = false;
						}
						else
						{
							data->targetingAlly = true;
							data->targetingEnemy = false;

							// if the previously selected (saved) party member has died, find a new target.
							if (!data->partyState[data->allyTargetSelection].alive)
							{
								const int targetSelection = data->allyTargetSelection;
								do
								{
									data->allyTargetSelection = (data->allyTargetSelection + 1) % 6;
								} while (!data->partyState[data->allyTargetSelection].alive && data->allyTargetSelection != targetSelection);
							}
						}
					}
				}
				else // Selecting target
				{
					if (data->targetingEnemy) // targeting enemy, i.e. a damaging or debuffing ability
					{
						if (IsKeyJustPressed(VK_RETURN))
						{
							//DoOffensivePartyAbility(
							//	&party[data->nextTurn],
							//	&data->partyState[data->nextTurn],
							//	&data->enemyState[data->enemyTargetSelection],
							//	data->actionSelection,
							//	data->enemies[data->enemyTargetSelection]->nameSingular
							//);

							//const char *name = data->enemies[data->enemyTargetSelection];
							const PartyMember *partyMember = &party[data->nextTurn];

							DoOffensivePartyAbility(
								data->partyState,
								partyMember,
								&data->partyState[data->nextTurn],
								data->enemyState,
								data->enemyTargetSelection,
								data->enemies[data->enemyTargetSelection]->nameSingular,
								data->actionSelection
								);

							//

							for (size_t i = 0; i < data->numEnemies; i++)
							{
								int x, y;
								GetEnemyPosition(i, data->numEnemies, &x, &y);

								if (data->enemyState[i].greyOutNeeded)
								{
									DrawGreyscaleRectangle(x, y + 6, 128, 122, 0);
									DrawBitMap32x32(data->enemies[i]->sprite, x, y, 0x282828, 4);
									DrawGreyscaleRectangle(x, y + 132, 140, 38, 0);
									
									if (i == data->allyTargetSelection)
									{
										GetNewEnemyTarget(data);
									}

									data->enemyState[i].greyOutNeeded = false;
								}
								else if (data->enemyState[i].alive)
								{
									DrawEnemyHealthBar(data->enemyState[i].health, data->enemies[i]->health[gameDifficulty], x, y);
								}
							}

							//

							UpdateCombatMeters(data->partyState, data->globalCycle);

							DrawHealthBar(data->nextTurn);
							DrawSecondaryProgressBar(data->nextTurn);
							DrawXPProgressBar(data->nextTurn);

							int x, y;
							const int enemyTarget = data->enemyTargetSelection;
							GetEnemyPosition(enemyTarget, data->numEnemies, &x, &y);
							DrawEnemyHealthBar(data->enemyState[enemyTarget].health, data->enemies[enemyTarget]->health[gameDifficulty], x, y);

							if (party[data->nextTurn].classData->specs[party[data->nextTurn].specIndex].abilities[data->actionSelection].offGcd)
							{
								data->targetingAlly = false;
								data->selectingAction = true;
								DrawActionMenuParty(partyMember, &data->partyState[data->nextTurn], data->nextTurn, -1);
								RedrawEnemySprite(data->enemies[data->enemyTargetSelection]->sprite, data->enemyTargetSelection, data->numEnemies);
								return;
							}

							data->animationTime = 0.0f;
							data->targetingEnemy = false;
							data->actionSelection = 0;
							data->subStateProcessed = true;
							data->subStateRendered = false;
						}

						if (IsKeyJustPressed(VK_RIGHT))
						{
							const int targetSelection = data->enemyTargetSelection;
							RedrawEnemySprite(data->enemies[targetSelection]->sprite, targetSelection, data->numEnemies);
							do
							{
								data->enemyTargetSelection = (data->enemyTargetSelection + 1) % data->numEnemies;
							} while (!data->enemyState[data->enemyTargetSelection].alive && data->enemyTargetSelection != targetSelection);

							data->subStateRendered = false;
						}

						if (IsKeyJustPressed(VK_LEFT))
						{
							const int targetSelection = data->enemyTargetSelection;
							RedrawEnemySprite(data->enemies[targetSelection]->sprite, targetSelection, data->numEnemies);
							do
							{
								data->enemyTargetSelection = (data->enemyTargetSelection + data->numEnemies - 1) % data->numEnemies;
							} while (!data->enemyState[data->enemyTargetSelection].alive && data->enemyTargetSelection != targetSelection);

							data->subStateRendered = false;
						}

						if (IsKeyJustPressed(VK_ESCAPE))
						{
							RedrawEnemySprite(data->enemies[data->enemyTargetSelection]->sprite, data->enemyTargetSelection, data->numEnemies);
							data->selectingAction = true;
							data->targetingEnemy = false;
							data->subStateRendered = false;
						}
					}
					else if (data->targetingAlly)  // targeting ally - i.e. a healing or buffing ability
					{
						if (IsKeyJustPressed(VK_RETURN))
						{
							const PartyMember *partyMember = &party[data->nextTurn];
							const Ability *ability = &partyMember->classData->specs[partyMember->specIndex].abilities[data->actionSelection];

							if (ability->subtype == BUFF_RES && data->partyState[data->allyTargetSelection].alive)
							{
								PushMessage("Target is alive, and cannot be revived!", 0XFF0000);
								return;
							}

							bool fullRedraw = DoDefensivePartyAbility(data->partyState,
																	  partyMember,
																	  &data->partyState[data->nextTurn],
																	  &data->partyState[data->allyTargetSelection],
																	  data->allyTargetSelection,
																	  data->actionSelection
							);

							if (fullRedraw)
							{
								RedrawAllAllyStatusIcons(data);
							}

							UpdateCombatMeters(data->partyState, data->globalCycle);

							DrawSecondaryProgressBar(data->nextTurn);
							DrawXPProgressBar(data->nextTurn);

							bool turn = data->allyTargetSelection == data->nextTurn;
							DrawPartyMemberFrame(data->allyTargetSelection, turn);

							if (party[data->nextTurn].classData->specs[party[data->nextTurn].specIndex].abilities[data->actionSelection].offGcd)
							{
								data->targetingAlly = false;
								data->selectingAction = true;
								DrawActionMenuParty(partyMember, &data->partyState[data->nextTurn], data->nextTurn, -1);
								return;
							}

							data->subStateProcessed = true;
							data->actionSelection = 0;
							data->targetingAlly = false;
							data->animationTime = 0.0f;
						}

						if (IsKeyJustPressed(VK_UP))
						{
							const int targetSelection = data->allyTargetSelection;
							bool turn = targetSelection == data->nextTurn;
							
							if (party[data->nextTurn].classData->specs[party[data->nextTurn].specIndex].abilities[data->actionSelection].subtype == BUFF_RES)
							{
								data->allyTargetSelection = (data->allyTargetSelection + 5) % 6;

								if (data->partyState[targetSelection].alive)
								{
									DrawPartyMemberFrame(targetSelection, turn);
								}
								else
								{
									DrawDeadPartyMemberFrame(targetSelection);
								}
							}
							else
							{
								do
								{
									data->allyTargetSelection = (data->allyTargetSelection + 5) % 6;
								} while (!data->partyState[data->allyTargetSelection].alive && data->allyTargetSelection != targetSelection);

								DrawPartyMemberFrame(targetSelection, turn);
							}

							data->subStateRendered = false;
						}

						if (IsKeyJustPressed(VK_DOWN))
						{
							const int targetSelection = data->allyTargetSelection;
							bool turn = targetSelection == data->nextTurn;

							if (party[data->nextTurn].classData->specs[party[data->nextTurn].specIndex].abilities[data->actionSelection].subtype == BUFF_RES)
							{
								data->allyTargetSelection = (data->allyTargetSelection + 1) % 6;

								if (data->partyState[targetSelection].alive)
								{
									DrawPartyMemberFrame(targetSelection, turn);
								}
								else
								{
									DrawDeadPartyMemberFrame(targetSelection);
								}
							}
							else
							{
								do
								{
									data->allyTargetSelection = (data->allyTargetSelection + 1) % 6;
								} while (!data->partyState[data->allyTargetSelection].alive && data->allyTargetSelection != targetSelection);
								
								DrawPartyMemberFrame(targetSelection, turn);
							}

							data->subStateRendered = false;
						}

						if (IsKeyJustPressed(VK_ESCAPE))
						{
							const int targetSelection = data->allyTargetSelection;
							bool turn = targetSelection == data->nextTurn;
							DrawPartyMemberFrame(targetSelection, turn);
							
							data->selectingAction = true;
							data->targetingAlly = false;
							data->subStateRendered = false;
						}
					}
				}
			}
			else
			{
				data->animationTime += deltaTime;

				if (data->animationTime >= 0.5f)
				{
					EndTurn(data);
				}
			}
		} break;

		case COMBAT_END:
		case COMBAT_WIPE: // fight over, handle win transition
		{
			data->animationTime += deltaTime;
		} break;
	}
}


void RenderCombat(struct GameState *state)
{
	CombatStateData *data = (CombatStateData *)state->data;

	switch (data->subState)
	{
		case COMBAT_TRANSITION: // Transition from movement_state (ambush popup)
		{
			const float totalAnimationTime = 1.0f;

			if (data->animationTime >= totalAnimationTime)
			{
				DrawGreyscaleRectangle(18, 18, 564 - 18, 364 - 18, 0); // clear ambush popup
				data->subState = COMBAT_SETUP;
			}
			else // if ambush popup animation is still playing
			{
				const float flashPeriod = 0.25f;
				const float factor = GetLerpFactor(data->animationTime, totalAnimationTime, flashPeriod);
				const uint32_t colour = InterpolateColour(0X600000, 0XFF1111, factor);

				// Draw flashing !AMBUSH animation
				DrawColourRectangle(280, 100, 24, 100, colour);
				DrawColourRectangle(280, 214, 24, 24, colour);
				DrawStringCentered("AMBUSH", 18, 564, 260, colour, 6);
			}
		} break;

		case COMBAT_SETUP: // pre-fight setup, create enemies, UI & action rolls
		{
			if (!data->subStateRendered)
			{
				for (int i = 0; i < data->numEnemies; i++)
				{
					DrawEnemy(i, data->numEnemies, data->enemies[i], &data->enemyState[i], data->turnCD[i + 6]);
				}
				for (int j = 0; j < 6; j++)
				{
					if (!data->partyState[j].alive) continue;

					DrawAllyActionBar(data->turnCD[j], j);
				}

				DrawCentreCombatUI();
				data->subStateRendered = true;
			}
		} break;

		case COMBAT_ALLOCATE_TURN: // assign next turn and setup the requisite UI
		{
			const float actionBarAnimationTime = 0.5f;

			if (data->subStateProcessed)
			{
				// Smoothly lerp the enemy action bars over 0.5 seconds up to their new value
				if (data->animationTime < actionBarAnimationTime)
				{
					float t = data->animationTime / actionBarAnimationTime;

					for (int i = 0; i < data->numEnemies; i++)
					{
						if (!data->enemyState[i].alive)
						{
							continue; // if enemy is dead, continue.
						}

						int x, y;
						GetEnemyPosition(i, data->numEnemies, &x, &y);

						float interpolatedProgress = ((1.0f - t) * data->prevTurnCD[i + 6] + t * data->turnCD[i + 6]);
						DrawEnemyActionBar(interpolatedProgress, x, y);
					}

					for (int j = 0; j < 6; j++)
					{
						if (!data->partyState[j].alive)
						{
							continue; // if enemy is dead, continue.
						}

						float interpolatedProgress = ((1.0f - t) * data->prevTurnCD[j] + t * data->turnCD[j]);
						DrawAllyActionBar(interpolatedProgress, j);
					}

					float interpolatedProgress = ((1.0f - t) * data->prevGlobalCycleProgess + t * data->globalCycleProgress);
					DrawGlobalCycleBar(interpolatedProgress, data->globalCycle);
				}
				else
				{
					if (data->nextTurn > 5) // If enemy turn next
					{
						DrawActionMenuEnemy();
						data->subState = COMBAT_ENEMY_TURN;
					}
					else if (data->nextTurn > -1) // if ally turn next
					{
						DrawAllyActionBar(0.0f, data->nextTurn);
						DrawPartyMemberFrame(data->nextTurn, TRUE);
						data->selectingAction = true;
						data->subState = COMBAT_PARTY_TURN;
					}
					else if (data->nextTurn == -1)
					{
						data->globalCycleProgress = 100.f;
						data->globalCycle += 1;

						IncrementGlobalCycle(data);

						data->subState = COMBAT_ALLOCATE_TURN;
					}

					data->subStateProcessed = false;
				}
			}
		} break;

		case COMBAT_ENEMY_TURN: // enemy takes turn automatically
		{
			const float enemyAttackAnimationTime = 0.5f;
			const float enemyAttackAnimationPeriod = 0.25f;

			int x, y;
			GetEnemyPosition(data->nextTurn - 6, data->numEnemies, &x, &y);

			if (data->animationTime < enemyAttackAnimationTime) // animate enemy attack and lerp action bar down.
			{
				float t = data->animationTime / enemyAttackAnimationTime;
				float interpolatedProgress = t * data->turnCD[data->nextTurn];
				DrawEnemyActionBar(interpolatedProgress, x, y);

				float factor = GetLerpFactor(data->animationTime, enemyAttackAnimationTime, enemyAttackAnimationPeriod);
				const int shiftAmount = 20;
				int currentX = x + (int)(factor * shiftAmount);

				DrawGreyscaleRectangle(currentX - 3, y + 8, 138, 128, 0);
				DrawBitMap32x32(data->enemies[data->nextTurn - 6]->sprite, currentX, y, 0xFFFFFF, 4);
			}
			else
			{
				DrawGreyscaleRectangle(x - 3, y + 8, 138, 128, 0);
				DrawBitMap32x32(data->enemies[data->nextTurn - 6]->sprite, x, y, 0xFFFFFF, 4);

				data->subStateRendered = true;
				data->subStateProcessed = false;
				data->subState = COMBAT_ALLOCATE_TURN;
			}

		} break;

		case COMBAT_PARTY_TURN: // setup UI and handle user action
		{
			if (!data->subStateRendered)
			{
				if (data->selectingAction)
				{
					bool hasUsableAbility = DrawActionMenuParty(&party[data->nextTurn], &data->partyState[data->nextTurn], data->actionSelection, -1);

					if (!hasUsableAbility)
					{
						const char *memberName = party[data->nextTurn].classData->name;
						char message[64] = { 0 };
						sprintf_s(message, sizeof(message), "%s has no usable abilities - turn passed automatically", memberName);
						PushMessage(message, 0XFF0000);
						EndTurn(data);
					}

					data->subStateRendered = true;
				}
				else
				{
					if (data->targetingEnemy)
					{
						DrawEnemyTargetIndicator(data->enemyTargetSelection, data->numEnemies);
						data->subStateRendered = true;
					}

					else if (data->targetingAlly)
					{
						DrawAllyTargetIndicator(data->allyTargetSelection);
						data->subStateRendered = true;
					}

					else // if not selecting action, targeting an enemy or ally -- then have just attacked. Animate enemy taking damage.
					{
						const float totalEnemyDamagedAnimationTime = 0.25f;
						const float enemyDamagedAnimationPeriod = 0.125f;

						int x, y;
						GetEnemyPosition(data->enemyTargetSelection, data->numEnemies, &x, &y);

						if (data->animationTime < totalEnemyDamagedAnimationTime)
						{
							const float t = data->animationTime / totalEnemyDamagedAnimationTime;

							if (data->enemyState[data->enemyTargetSelection].alive) // if enemy is still alive after damage
							{
								const int shiftAmount = 10;
								float factor = GetLerpFactor(data->animationTime, totalEnemyDamagedAnimationTime, enemyDamagedAnimationPeriod);
								int currentX = x + (int)(factor * shiftAmount);
								DrawGreyscaleRectangle(currentX - 2, y + 8, 138, 128, 0);
								DrawBitMap32x32(data->enemies[data->enemyTargetSelection]->sprite, currentX, y, 0xFF0000, 4);
							}
							else // if enemy is dead, fade out
							{
								const float fadeFactor = 1.0f - t;
								const uint32_t fadedColour = InterpolateColour(0x282828, 0xFFFFFF, fadeFactor);
								DrawGreyscaleRectangle(x - 3, y + 8, 138, 128, 0);
								DrawBitMap32x32(data->enemies[data->enemyTargetSelection]->sprite, x, y, fadedColour, 4);
							}
						}
						else // animation complete
						{
							if (data->enemyState[data->enemyTargetSelection].alive) // if enemy is still alive after damage
							{
								DrawGreyscaleRectangle(x - 3, y + 8, 138, 128, 0);
								RedrawEnemySprite(data->enemies[data->enemyTargetSelection]->sprite, data->enemyTargetSelection, data->numEnemies);
							}
							else // if enemy is dead
							{
								DrawGreyscaleRectangle(x - 3, y + 132, 140, 38, 0);
								GetNewEnemyTarget(data);
							}

							// If combat is not over (as dictated by the above), roll the next turn and restart combat loop
							EndTurn(data);
						}
					}
				}
			}
		} break;

		case COMBAT_END: // fight over, handle win transition
		{
			const float totalAnimationTime = 1.0f;

			if (data->animationTime >= totalAnimationTime)
			{
				DrawGreyscaleRectangle(18, 18, 564 - 18, 364 - 18, 0); // clear popup

				ItemGenData *itemGenData = BuildItemGenDataArray(data->enemies, numEnemiesAbil);

				DestroyCurrentState();
				SetState(CreateLootState(itemGenData, numEnemiesAbil));
			}
			else // if ambush popup animation is still playing
			{
				const float flashPeriod = 0.25f;
				const float factor = GetLerpFactor(data->animationTime, totalAnimationTime, flashPeriod);
				const uint32_t colour = InterpolateColour(0X056000, 0X3eff11, factor);

				// Draw flashing animation
				DrawStringCentered("YOU WIN!", 18, 564, 200, colour, 6);
			}
		} break;

		case COMBAT_WIPE:
		{
			const float totalAnimationTime = 2.0f;

			if (data->animationTime >= totalAnimationTime)
			{
				DrawGreyscaleRectangle(18, 18, 564 - 18, 364 - 18, 0); // clear popup

				for (int i = 0; i < 6; i++)
				{
					party[i].health.x = party[i].health.y;
				}

				ClearFrame(0);
				DestroyCurrentState();
				SetState(CreateMovementState(1));
			}
			else // if ambush popup animation is still playing
			{
				const float flashPeriod = 0.25f;
				const float factor = GetLerpFactor(data->animationTime, totalAnimationTime, flashPeriod);
				const uint32_t colour = InterpolateColour(0X600000, 0XFF1111, factor);

				// Draw flashing animation
				DrawStringCentered("WIPE!", 18, 564, 200, colour, 6);
			}
		} break;
	}
}


void DestroyCombat(struct GameState *state)
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


struct GameState *CreateCombatState()
{
	//const int8_t numEnemies = 6;
	boss = false;
	
	MapData *mapData = LoadMap(mapIndex);
	const int8_t numEnemies = rand() % (mapData->enemyRate.y + 1 - mapData->enemyRate.x) + mapData->enemyRate.x;

	numEnemiesAbil = numEnemies;

	GameState *state = malloc(sizeof(GameState));
	if (!state)
	{
		return NULL;
	}

	CombatStateData *data = calloc(1, sizeof(CombatStateData) + numEnemies * sizeof(const Enemy *));
	if (!data)
	{
		free(state);
		return NULL;
	}

	data->globalCycle = 1;
	data->globalCycleProgress = 100.0f;
	data->subState = COMBAT_TRANSITION;
	data->subStateRendered = false;
	data->subStateProcessed = false;
	data->doneTurnTick = false;
	data->numEnemies = numEnemies;
	data->animationTime = 0.0f;

	for (int i = 0; i < 6; i++)
	{
		MemberState *state = &data->partyState[i];
		InitMemberState(state);

		if (party[i].health.x == 0)
		{
			state->alive = false;
		}

		InitCombatStats(&party[i], state);
	}

	state->data = data;
	state->update = UpdateCombat;
	state->render = RenderCombat;
	state->destroy = DestroyCombat;

	return state;
}

static void InitMemberState(MemberState *state) 
{
	state->alive = true;
	state->damageDone = 0;
	state->healingDone = 0;
	state->otCount = 0;
	state->otRecieved = 0;
	state->otCapacity = 4;
	state->otEffects = malloc(state->otCapacity * sizeof(OverTimeEffect));
	if (state->otEffects == NULL) 
	{
		exit(EXIT_FAILURE);
	}
}

static void InitCombatStats(PartyMember *member, MemberState *state)
{
	const int scalingFactor = 3;

	const float baseCrit = member->classData->specs[member->specIndex].baseStats.crit;
	const float baseHaste = member->classData->specs[member->specIndex].baseStats.haste;
	const float baseMastery = member->classData->specs[member->specIndex].baseStats.mastery;

	state->combatStats.coeff = member->stats.mainStat * scalingFactor;
	state->combatStats.crit = baseCrit + ((float)member->stats.crit / member->level);
	state->combatStats.haste = baseHaste + ((float)member->stats.haste / member->level);
	state->combatStats.damageMulti = 1.0f + ((baseMastery + (float)member->stats.mastery / member->level) / 100);
	state->combatStats.leech = (float)member->stats.leech / (member->level * 5);
	state->combatStats.avoid = (float)member->stats.avoid / (member->level * 3);
	state->combatStats.damageTaken = 1.0f - ((float)member->stats.res / (member->level * 3) / 100);
}



// Called at the start of combat, randomly generates enemies
static void GenerateEnemies(CombatStateData *data, int enemyCountByType[3])
{
	const size_t enemySlots = 6;
	MapData *mapData = LoadMap(mapIndex);

	for (size_t i = 0; i < enemySlots; i++)
	{
		if (i < (size_t)data->numEnemies)
		{
			int randomType = rand() % 3;
			data->enemies[i] = mapData->enemies[randomType];
			//data->enemies[i] = &enemies[randomType];
			data->enemyState[i].alive = true;
			//data->enemyState[i].health = enemies[randomType].health[gameDifficulty];
			data->enemyState[i].health = mapData->enemies[randomType]->health[gameDifficulty];
			enemyCountByType[randomType]++;

		}
		else
		{
			data->enemies[i] = NULL;
		}
	}
}


static void GenerateAmbushMessage(CombatStateData *data, const int enemyCountByType[3])
{
	MapData *mapData = LoadMap(mapIndex);

	int typesPresent = 0;
	for (int j = 0; j < 3; j++)
	{
		if (enemyCountByType[j] > 0)
		{
			typesPresent++;
		}
	}

	char ambushMessage[72] = { 0 };
	int pos = 0;         // current write position in the message buffer
	int processed = 0;   // number of enemy types processed

	for (int t = 0; t < 3; t++)
	{
		if (enemyCountByType[t] > 0)
		{
			// Insert a separator if this isn't the first type.
			if (processed > 0)
			{
				if (processed == typesPresent - 1)
				{
					// For the last type, add a separator with "and".
					pos += sprintf_s(ambushMessage + pos, sizeof(ambushMessage) - pos, ", and ");
				}
				else
				{
					pos += sprintf_s(ambushMessage + pos, sizeof(ambushMessage) - pos, ", ");
				}
			}
			// Append enemy name; choose singular or plural depending on the count.
			if (enemyCountByType[t] > 1)
			{
				pos += sprintf_s(ambushMessage + pos, sizeof(ambushMessage) - pos, "%d %s",
					enemyCountByType[t], mapData->enemies[t]->nameplural);
			}
			else
			{
				pos += sprintf_s(ambushMessage + pos, sizeof(ambushMessage) - pos, "%s %s",
					mapData->enemies[t]->singularPrefix, mapData->enemies[t]->nameSingular);
			}
			processed++;
		}
	}
	PushMessage(ambushMessage, 0xFF1111);
}


static void DrawEnemy(const int index, const int numEnemies, const Enemy *enemy, const EnemyState *enemyState, const float turnCD)
{
	int x, y;
	GetEnemyPosition(index, numEnemies, &x, &y);
	DrawBitMap32x32(enemy->sprite, x, y, 0XFFFFFF, 4);
	DrawEnemyHealthBar(enemyState->health, enemy->health[gameDifficulty], x, y);
	DrawEnemyActionBar(turnCD, x, y);
}

static void RedrawEnemySprite(const unsigned char (*sprite)[4], const index, const numEnemies)
{
	int x, y;
	GetEnemyPosition(index, numEnemies, &x, &y);
	DrawGreyscaleRectangle(x, y + 6, 128, 122, 0);
	DrawBitMap32x32(sprite, x, y, 0XFFFFFF, 4);
}

static void DrawEnemyHealthBar(const int health, const int maxHealth, const int x, const int y)
{
	const int sX = x + 4;
	const int sY = y + 136;
	const float healthPercent = (health / (float)maxHealth) * 100;

	char healthBuffer[8] = { 0 };
	sprintf_s(healthBuffer, sizeof(healthBuffer), "%i", health);
	DrawProgressBar(sX, sY, 128, 14, healthPercent, 0xFF0000, 0x600000);
	DrawStringCentered(healthBuffer, sX, sX + 128, sY + 4, 0xFFFFFF, 1);
}

static void DrawEnemyActionBar(const float turnCD, const int x, const int y)
{
	const int sX = x + 4;
	const int sY = y + 136 + 14 + 2;
	const float progress = 100 - turnCD;
	DrawProgressBar(sX, sY, 128, 5, progress, 0x12D500, 0x064600);
}

static void DrawEnemyTargetIndicator(const int target, const int numEnemies)
{
	int x, y;
	GetEnemyPosition(target, numEnemies, &x, &y);
	DrawString("/", x + 50, y + 50, 0XFF0000, 6);
}

static void DrawAllyActionBar(const float turnCD, const int index)
{
	const int sX = 824;
	const int sY = (67 * index) + 71;
	const float progress = 100 - turnCD;
	DrawProgressBar(sX, sY, 128, 5, progress, 0x12D500, 0x064600);
}

static void DrawAllyTargetIndicator(const int allyTargetSelection)
{
	const int y = 16 + ((60 + 7) * allyTargetSelection);
	DrawString("+", 768, y + 11, 0X1AAD00, 5);
}

static void RedrawAllAllyStatusIcons(CombatStateData *data)
{
	for (int i = 0; i < 6; i++)
	{
		// Update ui layout in data, and clear space for redraw
		UpdateStatusEffectsUILayout(data, i);
		const int y = (67 * i) + 27;
		DrawGreyscaleRectangle(958, y, 56, 50, 0);
	}

	for (int j = 0; j < 6; j++)
	{
		DrawAllyStatusIcons(&data->partyState[j]);
	}
}

static void DrawGlobalCycleBar(const float cycleProgress, const int cycleCount)
{
	const int sX = 558;
	const int sY = 18;
	const float progress = 100 - cycleProgress;
	DrawVerticalProgressBar(sX, sY, 6, 346, progress, 0xc0a600, 0x524700);
}


static void RollTurn(const uint8_t index, CombatStateData *data, float *turnCD)
{
	if (data->turnCD[index] == -1.0f)
	{
		return;
	}

	float nextAction;

	if (index > 5) // enemy
	{
		float randomScale = 0.75f + ((float)rand() / RAND_MAX) * 0.5f;

		const Enemy *enemy = data->enemies[index - 6];
		nextAction = 100 - (enemy->haste * randomScale);

		*turnCD = nextAction;
	}
	else // party member
	{
		//if (!data->partyState[index].alive)
		//{
		//	data->turnCD[index] = -1.0f;
		//	return;
		//}

		float randomScale = 0.9f + ((float)rand() / RAND_MAX) * 0.2f;

		nextAction = 100 - (data->partyState[index].combatStats.haste * randomScale);
		*turnCD = nextAction;
	}
}

static void InitNextTurn(CombatStateData *data)
{
	int totalEntries = 6 + data->numEnemies;
	memcpy(data->prevTurnCD, data->turnCD, totalEntries * sizeof(float));
	data->prevGlobalCycleProgess = data->globalCycleProgress;

	// Decrement entries
	float lowestVal = data->turnCD[data->nextTurn];

	if (data->globalCycleProgress < lowestVal)
	{
		lowestVal = data->globalCycleProgress;
	}
	else
	{
		data->globalCycleProgress -= lowestVal;
	}

	for (int i = 0; i < totalEntries; i++)
	{
		if (data->turnCD[i] == -1.0f)
		{
			continue;
		}
		else
		{
			data->turnCD[i] -= lowestVal;
		}
	}
}

static void GenerateTurnOrder(CombatStateData *data)
{
	for (int8_t i = 0; i < 6 + data->numEnemies; i++)
	{
		RollTurn(i, data, &data->turnCD[i]);
	}

	SetNextTurn(data);
	InitNextTurn(data);
}

static void SetNextTurn(CombatStateData *data)
{
	const int totalEntries = 6 + data->numEnemies;
	int lowestIndex = 0;

	// Ensure starting member isn't dead, which would break the following searches.
	for (int j = 0; j < 6; j++)
	{
		if (data->partyState[j].alive)
		{
			lowestIndex = j;
			break;
		}
	}

	// Find next turn (lowest value by index).
	for (int k = 1; k < totalEntries; k++)
	{
		if (data->turnCD[k] == -1.0f)
		{
			continue;
		}
		if (k >= 6 && !data->enemyState[k - 6].alive)
		{
			continue;
		}
		if (k < 6 && !data->partyState[k].alive)
		{
			continue;
		}
		if (data->turnCD[k] < data->turnCD[lowestIndex])
		{
			lowestIndex = k;
		}
	}

	if (data->turnCD[lowestIndex] > data->globalCycleProgress)
	{
		data->nextTurn = -1;
	}
	else
	{
		data->nextTurn = (int8_t)lowestIndex;
	}
}


static void EndTurn(CombatStateData *data)
{
	if (data->subState == COMBAT_END) return;

	DrawPartyMemberFrame(data->nextTurn, false);
	const int nextTurnIdx = data->nextTurn;
	RollTurn(nextTurnIdx, data, &data->turnCD[nextTurnIdx]);

	data->subState = COMBAT_ALLOCATE_TURN;
	data->subStateProcessed = false;
	data->subStateRendered = false;
	data->doneTurnTick = false;
}


static void GetEnemyPosition(const int enemyIndex, const int totalEnemies, int *x, int *y)
{
	const int centerX = 222;
	const int singleRowY = 100;  // Y position for single-row layouts.
	const int topRowY = 26;      // Y position for the top row in two-row layouts.
	const int bottomRowY = 190;  // Y position for the bottom row in two-row layouts.
	const int spacing = 162;     // X spacing between enemy sprites.

	if (totalEnemies <= 3)
	{
		// Arrange in one row, centered horizontally.
		*y = singleRowY;
		if (totalEnemies == 1)
		{
			*x = centerX;
		}
		else if (totalEnemies == 2)
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
		else if (totalEnemies == 3)
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
		int topCount = (totalEnemies + 1) / 2;
		int bottomCount = totalEnemies / 2;
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

static void GetNewEnemyTarget(CombatStateData *data)
{
	bool newTargetFound = false;

	for (int i = 0; i < data->numEnemies; i++)
	{
		if (data->enemyState[i].alive)
		{
			data->enemyTargetSelection = i;
			newTargetFound = true;
			break;
		}
	}

	if (!newTargetFound)
	{
		data->animationTime = 0.0f;
		ClearMessageLog();
		PushMessage("You have won the battle! Your enemies have dropped some loot.", 0X00FF00);
		data->subState = COMBAT_END;
		return;
	}
}


static void UpdateStatusEffectsUILayout(CombatStateData *data, const int targetIdx)
{
	OverTimeEffect *targetEffects[12] = { NULL };
	int count = 0;

	for (int i = 0; i < 6; i++)
	{
		MemberState *memberState = &data->partyState[i];
		for (int j = 0; j < memberState->otCount; j++)
		{
			if (memberState->otEffects[j].target == targetIdx)
				targetEffects[count++] = &memberState->otEffects[j];
		}
	}

	int enemyEffectCount = 0, allyEffectCount = 0;


	for (size_t k = 0; k < count; k++)
	{
		if (targetEffects[k]->type == ABILITY_DAMAGE)
		{
			targetEffects[k]->uiLocation.x = enemyEffectCount;
			targetEffects[k]->uiLocation.y = 0;
			enemyEffectCount++;
		}
		else
		{
			targetEffects[k]->uiLocation.x = allyEffectCount % 4;
			targetEffects[k]->uiLocation.y = allyEffectCount / 4;
			allyEffectCount++;
		}
	}

	data->partyState[targetIdx].otRecieved = count;
}


static void IncrementGlobalCycle(CombatStateData *data)
{
	for (int i = 0; i < 6; i++)
	{
		if (!data->partyState[i].alive) continue;

		MemberState *memberState = &data->partyState[i];
		PartyMember *member = &party[i];

		for (size_t i = 0; i < NUM_ABILITES; i++)
		{
			if (!member->classData->specs[member->specIndex].abilities[i].offGcd)
			{
				continue;
			}
			else
			{
				if (memberState->cooldownState[i] > 0)
				{
					memberState->cooldownState[i] -= 1;
				}
			}
		}
		
		const SecondaryType secondary = member->classData->specs[member->specIndex].secondaryType;

		if (secondary == SECONDARY_MANA)
		{
			member->secondary.x += (int)member->secondary.y / 10;

			if (member->secondary.x > member->secondary.y)
			{
				member->secondary.x = member->secondary.y;
			}

			DrawSecondaryProgressBar(i);
		}
		else if (secondary == SECONDARY_ENERGY)
		{
			member->secondary.x += (int)member->secondary.y / 4;

			if (member->secondary.x > member->secondary.y)
			{
				member->secondary.x = member->secondary.y;
			}

			DrawSecondaryProgressBar(i);
		}

	}

	UpdateCombatMeters(data->partyState, data->globalCycle);
}


static void CheckPartyWipe(CombatStateData *data)
{
	int deadCount = 0;
	for (int i = 0; i < 6; i++)
	{
		if (party[i].health.x == 0)
		{
			deadCount++;
			continue;
		}
		else
		{
			return;
		}
	}
	if (deadCount == 6)
	{
		PushMessage("You have wiped!", 0xFFFFFF);

		data->animationTime = 0.0f;
		data->subState = COMBAT_WIPE;
		data->subStateProcessed = false;
		data->subStateRendered = false;

		return;
	}
}