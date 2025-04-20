#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

struct GameState;

void SetState(struct GameState *state);
void UpdateCurrentState(float deltaTime);
void RenderCurrentState(void);
void DestroyCurrentState(void);

#endif // STATE_MANAGER_H