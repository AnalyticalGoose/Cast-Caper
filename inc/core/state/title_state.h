#ifndef TITLE_STATE_H
#define TITLE_STATE_H

struct GameState;

void UpdateTitle(struct GameState *state, float deltaTime);
void RenderTitle(struct GameState *state);
void DestroyTitle(struct GameState *state);
struct GameState *CreateTitleState(int substate);

#endif // TITLE_STATE_H