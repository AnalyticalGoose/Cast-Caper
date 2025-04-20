#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

void UpdateInput();
void HandleKeyDown(int VKCode);
void HandleKeyUp(int VKCode);
bool IsKeyPressed(int VKCode);
bool IsKeyJustPressed(int VKCode);

#endif // INPUT_H