#ifndef STATE_H
#define STATE_H

typedef struct GameState 
{
	void(*update)(struct GameState *state, float deltaTime);
	void(*render)(struct GameState *state);
	void(*destroy)(struct GameState *state);
	void *data;
} GameState;

#endif // STATE_H
