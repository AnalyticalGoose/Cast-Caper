#ifndef MOVEMENT_STATE_H
#define MOVEMENT_STATE_H

extern bool redrawDisenchantWindow;

struct GameState;

void UpdateMovement(struct GameState *state, float deltaTime);
void RenderMovement(struct GameState *state);
void DestroyMovement(struct GameState *state);
struct GameState *CreateMovementState(int substate);

#endif // MOVEMENT_STATE_H
