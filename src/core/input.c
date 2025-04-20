#include "input.h"
#include "memory.h"


static bool currentKeys[256] = { 0 };
static bool previousKeys[256] = { 0 };

void UpdateInput()
{
	memcpy(previousKeys, currentKeys, sizeof(currentKeys));
}

void HandleKeyDown(int VKCode)
{
	if (VKCode >= 0 && VKCode < 256)
		currentKeys[VKCode] = true;
}

void HandleKeyUp(int VKCode)
{
	if (VKCode >= 0 && VKCode < 256)
		currentKeys[VKCode] = false;
}

bool IsKeyPressed(int VKCode)
{
	return (VKCode >= 0 && VKCode < 256) ? currentKeys[VKCode] : false;
}

bool IsKeyJustPressed(int VKCode)
{
	if (VKCode < 0 || VKCode >= 256) return false;
	return !previousKeys[VKCode] && currentKeys[VKCode];
}