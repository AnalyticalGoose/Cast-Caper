#include "state.h"

static GameState *currentState;


void SetState(GameState *state)
{
	currentState = state;
}

void UpdateCurrentState(float deltaTime)
{
	if (currentState && currentState->update)
	{
		currentState->update(currentState, deltaTime);
	}
}

void RenderCurrentState(void)
{
	if (currentState && currentState->render)
	{
		currentState->render(currentState);
	}
}

void DestroyCurrentState(void)
{
	if (currentState && currentState->destroy)
	{
		currentState->destroy(currentState);
	}
}