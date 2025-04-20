#ifndef COMBAT_STATE_H
#define COMBAT_STATE_H

struct GameState;

void UpdateCombat(struct GameState *state, float deltaTime);
void RenderCombat(struct GameState *state);
void DestroyCombat(struct GameState *state);
struct GameState *CreateCombatState();

#endif // !COMBAT_STATE_H
