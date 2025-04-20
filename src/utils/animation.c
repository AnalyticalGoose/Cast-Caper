#include <math.h>

float GetLerpFactor(float animationTime, float totalanimationTime, float period)
{
    if (animationTime >= totalanimationTime)
    {
        return 1.0f;
    }
    else
    {
        float phase = fmodf(animationTime, period) / period;
        if (phase < 0.5f)
        {
            return phase * 2.0f; // ramp up
        }
        else
        {
            return (1.0f - phase) * 2.0f; // ramp down
        }
    }
}