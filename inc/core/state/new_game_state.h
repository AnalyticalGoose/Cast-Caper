#ifndef NEW_GAME_STATE_H
#define NEW_GAME_STATE_H

struct GameState;

void UpdateNewGame(struct GameState *state, float deltaTime);
void RenderNewGame(struct GameState *state);
void DestroyNewGame(struct GameState *state);
struct GameState *CreateNewGameState(int substate);

#endif // NEW_GAME_STATE_H