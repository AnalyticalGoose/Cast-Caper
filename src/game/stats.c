#include "party.h"


void SetSecondary(int index)
{
	const int baseSecondaries[NUM_SECONDARY] = { 10, 100, 10, 10 };
	const int level = party[index].level;

	party[index].secondary.y = baseSecondaries[party[index].classData->specs[party[index].specIndex].secondaryType] * level;
	party[index].secondary.x = party[index].secondary.y;
}


void GiveExp(int index, int exp)
{

}


void SetPartyVars()
{
	for (int i = 0; i < 6; i++)
	{
		SetSecondary(i);
	};
}