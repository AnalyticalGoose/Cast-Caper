#include "ability.h"
#include "animation.h"
#include "colour.h"
#include "difficulty.h"
#include "input.h"
#include "party.h"
#include "renderer.h"
#include "state_manager.h"
#include "movement_state.h"
#include "title_state.h"
#include "state.h"
#include "ui.h"
#include <stdint.h>
#include <stdio.h>
#include <windows.h>


typedef enum
{
	BOSS_TRANSITION,
	BOSS_SETUP,
	BOSS_ALLOCATE_TURN,
	BOSS_TURN,
	BOSS_PARTY_TURN,
	BOSS_PHASE_TRANSITION,
	BOSS_END,
	BOSS_WIPE,
} BossSubState;

typedef enum
{
	PHASE_CHICKEN,
	PHASE_DINO,
	PHASE_DEATH,
} BossPhase;


typedef struct BossStateData
{
	MemberState partyState[6];
	Enemy *boss;
	EnemyState bossState;

	float turnCD[7];
	float prevTurnCD[7];

	float globalCycleProgress;
	float prevGlobalCycleProgess;
	int globalCycle;
	BossPhase bossPhase;

	BossSubState subState;
	float animationTime;
	int nextTurn;
	int actionSelection;
	int allyTargetSelection;

	bool selectingAction;
	bool targetingEnemy;
	bool targetingAlly;

	bool doneTurnTick;
	bool subStateRendered;
	bool subStateProcessed;
} BossStateData;


static void BossInitMemberState(MemberState *state);
static void BossInitCombatStats(PartyMember *member, MemberState *state);
static void BossGenerateTurnOrder(BossStateData *data);
static void BossRollTurn(const int index, BossStateData *data, float *turnCD);
static void BossSetNextTurn(BossStateData *data);
static void BossInitNextTurn(BossStateData *data);
static void GenerateBoss(BossStateData *data, const int bossIndex);
static void DrawBoss(const Enemy *boss, const EnemyState *bossState, const float turnCD);
static void RedrawBossSprite(const Enemy *boss);
static void DrawBossHealthBar(const int health, const int maxHealth);
static void DrawBossActionBar(const float turnCD);
static void BossDrawAllyActionBar(const float turnCD, const int index);
static void BossDrawGlobalCycleBar(const float cycleProgress, const int cycleCount);
static void BossIncrementGlobalCycle(BossStateData *data);
static void BossRedrawAllAllyStatusIcons(BossStateData *data);
static void UpdateStatusEffectsUILayout(BossStateData *data, const int targetIdx);
static void BossEndTurn(BossStateData *data);
static void BossDrawEnemyTargetIndicator();
static void BossDrawAllyTargetIndicator(const int allyTargetSelection);
static void HandleKillBoss(BossStateData *data);
static void CheckPartyWipe(BossStateData *data);


void UpdateBoss(struct GameState *state, float deltaTime)
{
	BossStateData *data = (BossStateData *)state->data;

	switch (data->subState)
	{
		case BOSS_TRANSITION:
		{
			data->animationTime += deltaTime;
		} break;

		case BOSS_SETUP:
		{
			if (!data->subStateProcessed)
			{
				GenerateBoss(data, 0);

				PushMessage("You have been ambushed by Cuthbert, keeper of the golden egg!", 0xFF1111);
				PushMessage("Press [ENTER] to face your final challenge.", 0X00FF00);

				BossGenerateTurnOrder(data);

				data->subStateProcessed = true;
			}
			else // Once enemies have been generated, await user input to start combat
			{
				if (IsKeyJustPressed(VK_RETURN))
				{
					ClearMessageLog();
					data->subStateProcessed = false;
					data->animationTime = 0.016f;
					data->subState = BOSS_ALLOCATE_TURN;
				}
			}
		} break;

		case BOSS_ALLOCATE_TURN:
		{
			if (!data->subStateProcessed)
			{
				BossSetNextTurn(data);
				BossInitNextTurn(data);

				data->animationTime = 0.0f;
				data->subStateRendered = false;
				data->subStateProcessed = true;
			}

			data->animationTime += deltaTime;
		} break;

		case BOSS_TURN:
		{
			if (!data->subStateProcessed)
			{
				const int nextTurn = data->nextTurn;

				BossRollTurn(nextTurn, data, &data->turnCD[nextTurn]);
				DoEnemyAbility(data->boss, &data->bossState, data->partyState);
				CheckPartyWipe(data);

				data->animationTime = 0.0f;
				data->subStateProcessed = true;
			}

			data->animationTime += deltaTime;
		} break;

		case BOSS_PARTY_TURN:
		{
			if (!data->doneTurnTick)
			{
				bool fullRedraw = TickOverTimeEffects(&data->partyState[data->nextTurn], data->partyState, &data->bossState, data->nextTurn);
				UpdateCombatMeters(data->partyState, data->globalCycle);
				DrawXPProgressBar(data->nextTurn);

				if (fullRedraw)
				{
					BossRedrawAllAllyStatusIcons(data);
				}

				if (data->bossState.greyOutNeeded)
				{
					HandleKillBoss(data);
					return;
				}
				else if (data->bossState.alive)
				{
					DrawBossHealthBar(data->bossState.health, data->boss->health[gameDifficulty]);
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
				else // selecting target
				{
					if (data->targetingEnemy)
					{
						if (IsKeyJustPressed(VK_RETURN))
						{
							const PartyMember *partyMember = &party[data->nextTurn];

							DoOffensivePartyAbility(
								data->partyState,
								partyMember,
								&data->partyState[data->nextTurn],
								&data->bossState,
								0,
								"Cuthbert",
								data->actionSelection
							);

							if (data->bossState.greyOutNeeded)
							{
								HandleKillBoss(data);

								//DrawBitMap32x32(data->boss->sprite, 222, 90, 0x282828, 4);
								//DrawGreyscaleRectangle(78, 226, 426, 38, 0);

								//PushMessage("Handle win condition", 0xFF0000);
								//data->subState = BOSS_END;
								return;
							}
							else if (data->bossState.alive)
							{
								DrawBossHealthBar(data->bossState.health, data->boss->health[gameDifficulty]);
							}

							UpdateCombatMeters(data->partyState, data->globalCycle);

							DrawHealthBar(data->nextTurn);
							DrawSecondaryProgressBar(data->nextTurn);
							DrawXPProgressBar(data->nextTurn);

							if (party[data->nextTurn].classData->specs[party[data->nextTurn].specIndex].abilities[data->actionSelection].offGcd)
							{
								data->targetingAlly = false;
								data->selectingAction = true;
								DrawActionMenuParty(partyMember, &data->partyState[data->nextTurn], data->nextTurn, -1);
								RedrawBossSprite(data->boss);
								return;
							}

							data->animationTime = 0.0f;
							data->targetingEnemy = false;
							data->actionSelection = 0;
							data->subStateProcessed = true;
							data->subStateRendered = false;
						}

						if (IsKeyJustPressed(VK_ESCAPE))
						{
							RedrawBossSprite(data->boss);
							data->selectingAction = true;
							data->targetingEnemy = false;
							data->subStateRendered = false;
						}
					}
					else if (data->targetingAlly)
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
								BossRedrawAllAllyStatusIcons(data);
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
					BossEndTurn(data);
				}
			}

		} break;

		case BOSS_PHASE_TRANSITION:
		{
			if (!data->subStateProcessed)
			{
				switch (data->bossPhase)
				{
					case PHASE_DINO:
					{
						ClearMessageLog();
						PushMessage("Cuthbert, is shaking.... He's devolving!", 0XFF0000);
					} break;
					case PHASE_DEATH:
					{
						ClearMessageLog();
						PushMessage("It's not over... he's transforming again!", 0XFF0000);
					} break;
				}
				data->subStateProcessed = true;
			}

			data->animationTime += deltaTime;
		} break;

		case BOSS_END:
		{
			if (data->animationTime >= 5.0f)
			{
				if (IsKeyJustPressed('C'))
				{
					DestroyCurrentState();
					SetState(CreateMovementState(0));
					return;
				}
				if (IsKeyJustPressed('M'))
				{
					ClearFrame(0);
					DestroyCurrentState();
					SetState(CreateTitleState(0));
					return;
				}
			}
			else
			{
				data->animationTime += deltaTime;
			}
		} break;

		case BOSS_WIPE:
		{
			data->animationTime += deltaTime;
		}
	}
}

void RenderBoss(struct GameState *state)
{
	BossStateData *data = (BossStateData *)state->data;

	switch (data->subState)
	{
		case BOSS_TRANSITION:
		{
			const float totalAnimationTime = 0.1f;
			//const float totalAnimationTime = 3.0f;

			if (data->animationTime >= totalAnimationTime)
			{
				DrawGreyscaleRectangle(18, 18, 564 - 18, 364 - 18, 0); // clear popup
				data->subState = BOSS_SETUP;
			}
			else
			{
				const float flashPeriod = 0.25f;
				const float factor = GetLerpFactor(data->animationTime, totalAnimationTime, flashPeriod);
				const uint32_t colour = InterpolateColour(0X600000, 0XFF1111, factor);

				// Draw flashing !AMBUSH animation
				DrawColourRectangle(280, 60, 24, 100, colour);
				DrawColourRectangle(280, 174, 24, 24, colour);
				DrawStringCentered("BOSS", 18, 564, 220, colour, 6);
				DrawStringCentered("INCOMING", 18, 564, 280, colour, 6);
			}
		} break;

		case BOSS_SETUP:
		{
			if (!data->subStateRendered)
			{
				for (int i = 0; i < 6; i++)
				{
					BossDrawAllyActionBar(data->turnCD[i], i);
				}

				DrawBoss(data->boss, &data->bossState, data->turnCD[6]);
				DrawCentreCombatUI();
				data->subStateRendered = true;
			}

		} break;

		case BOSS_ALLOCATE_TURN:
		{
			const float actionBarAnimationTime = 0.5f;

			if (data->subStateProcessed)
			{
				if (data->animationTime < actionBarAnimationTime)
				{
					float t = data->animationTime / actionBarAnimationTime;
					float interpolatedBossProgress = ((1.0f - t) * data->prevTurnCD[6] + t * data->turnCD[6]);
					DrawBossActionBar(interpolatedBossProgress);

					for (int i = 0; i < 6; i++)
					{
						if (!data->partyState[i].alive)
						{
							continue; // if enemy is dead, continue.
						}

						float interpolatedAllyProgress = ((1.0f - t) * data->prevTurnCD[i] + t * data->turnCD[i]);
						BossDrawAllyActionBar(interpolatedAllyProgress, i);
					}

					float interpolatedGlobalProgress = ((1.0f - t) * data->prevGlobalCycleProgess + t * data->globalCycleProgress);
					BossDrawGlobalCycleBar(interpolatedGlobalProgress, data->globalCycle);
				}
				else
				{
					if (data->nextTurn == 6) // boss turn
					{
						DrawActionMenuEnemy();
						data->subState = BOSS_TURN;
					}
					else if (data->nextTurn > -1) // ally turn
					{
						BossDrawAllyActionBar(0.0f, data->nextTurn);
						DrawPartyMemberFrame(data->nextTurn, TRUE);
						data->selectingAction = true;
						data->subState = BOSS_PARTY_TURN;
					}
					else if (data->nextTurn == -1)
					{
						data->globalCycleProgress = 100.0f;
						data->globalCycle += 1;

						BossIncrementGlobalCycle(data);

						data->subState = BOSS_ALLOCATE_TURN;
					}

					data->subStateProcessed = false;
				}
			}

		} break;

		case BOSS_TURN:
		{
			const float bossAttackAnimationTime = 0.5f;
			const float bossAttackAnimationPeriod = 0.25f;

			const int x = 222;
			const int y = 90;

			if (data->animationTime < bossAttackAnimationTime)
			{
				float t = data->animationTime / bossAttackAnimationTime;
				float interpolatedProgress = t * data->turnCD[data->nextTurn];
				DrawBossActionBar(interpolatedProgress);

				float factor = GetLerpFactor(data->animationTime, bossAttackAnimationTime, bossAttackAnimationPeriod);
				const int shiftAmount = 20;
				int currentX = x + (int)(factor * shiftAmount);

				//DrawGreyscaleRectangle(currentX, y + 6, 128, 122, 0);
				DrawGreyscaleRectangle(200, 84, 178, 140, 0);
				DrawBitMap32x32(data->boss->sprite, currentX, y, 0xFFFFFF, 4);
			}
			else
			{
				RedrawBossSprite(data->boss);

				data->subStateRendered = true;
				data->subStateProcessed = false;
				data->subState = BOSS_ALLOCATE_TURN;
			}

		} break;

		case BOSS_PARTY_TURN:
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
						BossEndTurn(data);
					}

					data->subStateRendered = true;
				}
				else
				{
					if (data->targetingEnemy)
					{
						BossDrawEnemyTargetIndicator();
						data->subStateRendered = true;
					}
					else if (data->targetingAlly)
					{
						BossDrawAllyTargetIndicator(data->allyTargetSelection);
						data->subStateRendered = true;
					}
					else // if not selecting action, targeting an enemy or ally -- then have just attacked. Animate enemy taking damage.
					{
						const float totalBossDamagedAnimationTime = 0.25f;
						const float bossDamagedAnimationPeriod = 0.125f;
						const int x = 222;
						const int y = 90;

						if (data->animationTime < totalBossDamagedAnimationTime)
						{
							const float t = data->animationTime / totalBossDamagedAnimationTime;

							if (data->bossState.alive)
							{
								const int shiftAmount = 10;
								float factor = GetLerpFactor(data->animationTime, totalBossDamagedAnimationTime, bossDamagedAnimationPeriod);
								int currentX = x + (int)(factor * shiftAmount);
								DrawGreyscaleRectangle(200, 84, 178, 140, 0);
								DrawBitMap32x32(data->boss->sprite, currentX, y, 0xFF0000, 4);
							}
							else // if enemy is dead, fade out
							{
								const float fadeFactor = 1.0f - t;
								const uint32_t fadedColour = InterpolateColour(0x282828, 0xFFFFFF, fadeFactor);
								DrawGreyscaleRectangle(200, 84, 178, 140, 0);
								DrawBitMap32x32(data->boss->sprite, x, y, fadedColour, 4);
							}
						}
						else
						{
							if (data->bossState.alive)
							{
								RedrawBossSprite(data->boss);
							}
							else
							{
								DrawBossHealthBar(data->bossState.health, data->boss->health[gameDifficulty]);
								HandleKillBoss(data);
								return;
							}

							// If combat is not over (as dictated by the above), roll the next turn and restart combat loop
							BossEndTurn(data);
						}
					}
				}
			}
		} break;

		case BOSS_PHASE_TRANSITION:
		{
			if (!data->subStateRendered)
			{
				const float bossPlasingAnimationTime = 5.0f;
				const float bossPhasingAnimationPeriod = 0.25f;

				const int x = 222;
				const int y = 90;

				if (data->animationTime < bossPlasingAnimationTime)
				{
					float t = data->animationTime / bossPlasingAnimationTime;
					float interpolatedProgress = t * data->turnCD[data->nextTurn];
					DrawBossActionBar(interpolatedProgress);

					float factor = GetLerpFactor(data->animationTime, bossPlasingAnimationTime, bossPhasingAnimationPeriod);
					const int shiftAmount = 20;
					int currentX = x + (int)(factor * shiftAmount);

					DrawGreyscaleRectangle(200, 84, 178, 140, 0);
					DrawBitMap32x32(data->boss->sprite, currentX, y, 0x282828, 4);
				}
				else
				{
					GenerateBoss(data, (int)data->bossPhase);
					RedrawBossSprite(data->boss);
					DrawBossHealthBar(data->bossState.health, data->boss->health[gameDifficulty]);

					switch (data->bossPhase)
					{
						case PHASE_DINO:
						{
							PushMessage("Oh no! Cuthbert has transformed into a Poultrasaurus Rex!", 0XFF0000);
						} break;
						case PHASE_DEATH:
						{
							PushMessage("Cuthbert has transcended into his final form.", 0XFF0000);
							PushMessage("The true guardian of the Golden Egg... Death himself.", 0XFF0000);
							PushMessage(" ", 0);
							PushMessage("Prepare yourself for one final fight heroes!", 0XFF0000);
						} break;
					}

					data->subStateRendered = true;
					data->subStateProcessed = false;
					data->subState = BOSS_ALLOCATE_TURN;
				}
			}
		} break;

		case BOSS_END:
		{
			if (!data->subStateRendered)
			{
				const float bossDeathAnimationTime = 5.0f;
				const float bossDeathAnimationPeriod = 0.25f;

				const int x = 222;
				const int y = 90;

				if (data->animationTime < bossDeathAnimationTime)
				{
					float t = data->animationTime / bossDeathAnimationTime;
					float interpolatedProgress = t * data->turnCD[data->nextTurn];
					DrawBossActionBar(interpolatedProgress);

					float factor = GetLerpFactor(data->animationTime, bossDeathAnimationTime, bossDeathAnimationPeriod);
					const int shiftAmount = 20;
					int currentX = x + (int)(factor * shiftAmount);

					DrawGreyscaleRectangle(200, 84, 178, 140, 0);
					DrawBitMap32x32(data->boss->sprite, currentX, y, 0x282828, 4);
				}
				else
				{
					ClearMessageLog();
					PushMessage("Cuthbert has been defeated at last.", 0XFFFFFF);
					PushMessage("Congratulations Heroes, take your rest now.", 0XFFFFF);
					PushMessage(" ", 0);
					PushMessage("Press [C] to continue playing", 0XFFFFFF);
					PushMessage("Press [M] to quit to main menu", 0XFFFFFF);
				}
			}
		} break;

		case BOSS_WIPE:
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

void DestroyBoss(struct GameState *state)
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

struct GameState *CreateBossState()
{
	numEnemiesAbil = 1;
	boss = true;

	GameState *state = malloc(sizeof(GameState));
	if (!state)
	{
		return NULL;
	}

	BossStateData *data = calloc(1, sizeof(BossStateData) + sizeof(const Enemy *));
	if (!data)
	{
		free(state);
		return NULL;
	}

	data->globalCycle = 1;
	data->globalCycleProgress = 100.0f;
	data->subState = BOSS_TRANSITION;
	data->subStateRendered = false;
	data->subStateProcessed = false;
	data->doneTurnTick = false;
	data->animationTime = 0.0f;

	for (int i = 0; i < 6; i++)
	{
		MemberState *state = &data->partyState[i];
		BossInitMemberState(state);
		BossInitCombatStats(&party[i], state);
	}

	state->data = data;
	state->update = UpdateBoss;
	state->render = RenderBoss;
	state->destroy = DestroyBoss;

}


static void BossInitMemberState(MemberState *state)
{
	state->alive = true;
	state->damageDone = 0;
	state->healingDone = 0;
	state->otCount = 0;
	state->otRecieved = 0;
	state->otCapacity = 4;
	state->otEffects = malloc(state->otCapacity * sizeof(OverTimeEffect));
}


static void BossInitCombatStats(PartyMember *member, MemberState *state)
{
	const int scalingFactor = 3;

	const float baseCrit = member->classData->specs[member->specIndex].baseStats.crit;
	const float baseHaste = member->classData->specs[member->specIndex].baseStats.haste;
	const float baseMastery = member->classData->specs[member->specIndex].baseStats.mastery;

	state->combatStats.coeff = member->stats.mainStat * scalingFactor;
	state->combatStats.crit = baseCrit + ((float)member->stats.crit / member->level);
	state->combatStats.haste = baseHaste + ((float)member->stats.haste / member->level);
	state->combatStats.damageMulti = 1.0f + ((baseMastery + (float)member->stats.mastery / member->level) / 100);
	state->combatStats.leech = (float)member->stats.leech / (member->level * 20);
	state->combatStats.avoid = (float)member->stats.avoid / (member->level * 5);
	state->combatStats.damageTaken = 1.0f - ((float)member->stats.res / (member->level * 5) / 100);
}


static void BossGenerateTurnOrder(BossStateData *data)
{
	for (int i = 0; i < 7; i++)
	{
		BossRollTurn(i, data, &data->turnCD[i]);
	}
	
	BossSetNextTurn(data);
	BossInitNextTurn(data);
}


static void BossRollTurn(const int index, BossStateData *data, float *turnCD)
{
	float nextAction;

	if (index == 6) // is boss
	{
		float randomScale = 0.75f + ((float)rand() / RAND_MAX) * 0.5f;
		const Enemy *enemy = data->boss;
		nextAction = 100 - (enemy->haste * randomScale);

		*turnCD = nextAction;
	}
	else
	{
		float randomScale = 0.9f + ((float)rand() / RAND_MAX) * 0.2f;
		nextAction = 100 - (data->partyState[index].combatStats.haste * randomScale);
		*turnCD = nextAction;
	}
}

static void BossSetNextTurn(BossStateData *data)
{
	int lowestIndex = 0;
	for (int i = 0; i < 6; i++)
	{
		if (data->partyState[i].alive)
		{
			lowestIndex = i;
			break;
		}
	}

	for (int j = 0; j < 7; j++)
	{
		if (j == 6 && !data->bossState.alive)
		{
			continue;
		}
		if (j < 6 && !data->partyState[j].alive)
		{
			continue;
		}
		if (data->turnCD[j] < data->turnCD[lowestIndex])
		{
			lowestIndex = j;
		}
	}

	if (data->turnCD[lowestIndex] > data->globalCycleProgress)
	{
		data->nextTurn = -1;
	}
	else
	{
		data->nextTurn = lowestIndex;
	}
}

static void BossInitNextTurn(BossStateData *data)
{
	memcpy(data->prevTurnCD, data->turnCD, 7 * sizeof(float));
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

	for (int i = 0; i < 7; i++)
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


static void GenerateBoss(BossStateData *data, const int bossIndex)
{
	data->boss = &enemies[bossIndex];
	data->bossState.alive = true;
	data->bossState.health = enemies[bossIndex].health[gameDifficulty];
	data->bossState.greyOutNeeded = false;
}


static void DrawBoss(const Enemy *boss, const EnemyState *bossState, const float turnCD)
{
	DrawBitMap32x32(boss->sprite, 222, 90, 0XFFFFFF, 4);
	DrawBossHealthBar(bossState->health, boss->health[gameDifficulty]);
	DrawBossActionBar(turnCD);
}

static void RedrawBossSprite(const Enemy *boss)
{
	DrawGreyscaleRectangle(200, 84, 178, 140, 0);
	DrawBitMap32x32(boss->sprite, 222, 90, 0XFFFFFF, 4);
}

static void DrawBossHealthBar(const int health, const int maxHealth)
{
	const int sX = 224;
	const int sY = 236;

	const float healthPercent = (health / (float)maxHealth) * 100;
	char healthBuffer[8] = { 0 };
	sprintf_s(healthBuffer, sizeof(healthBuffer), "%i", health);
	DrawProgressBar(78, sY - 10, 426, 24, healthPercent, 0xFF0000, 0x600000);
	DrawStringCentered(healthBuffer, sX, sX + 128, sY - 5, 0xFFFFFF, 2);
}

static void DrawBossActionBar(const float turnCD)
{
	const int sX = 224 + 4;
	const int sY = 236 + 14 + 2;

	const float progress = 100 - turnCD;
	DrawProgressBar(78, sY, 426, 5, progress, 0x12D500, 0x064600);
}

static void BossDrawAllyActionBar(const float turnCD, const int index)
{
	const int sX = 824;
	const int sY = (67 * index) + 71;
	const float progress = 100 - turnCD;
	DrawProgressBar(sX, sY, 128, 5, progress, 0x12D500, 0x064600);
}

static void BossDrawGlobalCycleBar(const float cycleProgress, const int cycleCount)
{
	const int sX = 558;
	const int sY = 18;
	const float progress = 100 - cycleProgress;
	DrawVerticalProgressBar(sX, sY, 6, 346, progress, 0xc0a600, 0x524700);
}

static void BossIncrementGlobalCycle(BossStateData *data)
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

static void BossRedrawAllAllyStatusIcons(BossStateData *data)
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

static void UpdateStatusEffectsUILayout(BossStateData *data, const int targetIdx)
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

static void BossEndTurn(BossStateData *data)
{
	if (data->subState == BOSS_END) return;

	DrawPartyMemberFrame(data->nextTurn, false);
	const int nextTurnIdx = data->nextTurn;
	BossRollTurn(nextTurnIdx, data, &data->turnCD[nextTurnIdx]);

	data->subState = BOSS_ALLOCATE_TURN;
	data->subStateProcessed = false;
	data->subStateRendered = false;
	data->doneTurnTick = false;
}

static void BossDrawEnemyTargetIndicator()
{
	DrawString("/", 264, 150, 0XFF0000, 6);
}

static void BossDrawAllyTargetIndicator(const int allyTargetSelection)
{
	const int y = 16 + ((60 + 7) * allyTargetSelection);
	DrawString("+", 768, y + 11, 0X1AAD00, 5);
}


static void HandleKillBoss(BossStateData *data)
{
	switch (data->bossPhase)
	{
		case PHASE_CHICKEN:
		case PHASE_DINO:
		{
			data->bossPhase++;
			data->animationTime = 0.0f;
			data->subState = BOSS_PHASE_TRANSITION;
			data->subStateProcessed = false;
		} break;

		case PHASE_DEATH:
		{
			data->subState = BOSS_END;
			data->animationTime = 0.0f;
			data->subStateProcessed = false;
			data->subStateRendered = false;
		} break;
	}
}


static void CheckPartyWipe(BossStateData *data)
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
		data->subState = BOSS_WIPE;
		data->subStateProcessed = false;
		data->subStateRendered = false;

		return;
	}
}