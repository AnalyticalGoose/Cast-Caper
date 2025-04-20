#ifndef BOSS_STATE_H
#define BOSS_STATE_H

struct GameState;

void UpdateBoss(struct GameState *state, float deltaTime);
void RenderBoss(struct GameState *state);
void DestroyBoss(struct GameState *state);
struct GameState *CreateBossState();

#endif // BOSS_H
