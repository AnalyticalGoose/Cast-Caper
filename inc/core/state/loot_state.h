#ifndef LOOT_STATE_H
#define LOOT_STATE_H

#include "enemy.h"

struct GameState;

void UpdateLoot(struct GameState *state, float deltaTime);
void RenderLoot(struct GameState *state);
void DestroyLoot(struct GameState *state);
struct GameState *CreateLootState(ItemGenData *itemGenData, const int numItems);

#endif // !LOOT_STATE_H
