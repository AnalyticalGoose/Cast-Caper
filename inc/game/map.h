#ifndef MAP_H
#define MAP_H

#include "enemy.h"
#include <stdbool.h>
#include <stdint.h>
#include <xy.h>


typedef enum {
	TRIGGER_NULL,
	TRIGGER_ENABLE_ENCOUNTERS,
	TRIGGER_DISABLE_ENCOUNTERS,
	TRIGGER_NEXT_MAP,
	TRIGGER_PREV_MAP,
	TRIGGER_BOSS,
} Trigger;


typedef struct {
	struct xy doorPos;
	struct xy movePos;
	Trigger trigger;
} Door;


typedef struct {
	char tiles[16][16];
	char decorations[16][16];
	Door doors[10];
	struct xy encounterRate;
	const Enemy *enemies[3];
	struct xy enemyRate;
	struct xy respawn;
} MapData;


Door *FindDoorAtPosition(MapData *mapData, struct xy pos);
MapData *LoadMap(int index);
char *LoadMapName(int index);


#endif MAP_H