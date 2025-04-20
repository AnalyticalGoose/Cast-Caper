#include "audio.h"
#include "input.h"
#include "disenchant.h"
#include "state_manager.h"
#include "window.h"
#include <windows.h>


//#include <item.h>
#include <party.h>


__stdcall wWinMain(_In_ HINSTANCE hInstance,
            _In_opt_ HINSTANCE hPrevInstance,
            _In_ LPWSTR lpCmdLine,
            _In_ int nShowCmd)
{
    if (!InitWindow(hInstance, nShowCmd))
    {
        return -1;
    }

    //if (!InitAudio())
    //{
    //    return -1;
    //}

    //if (!StartAudioThread())
    //{
    //    return -1;
    //}

    for (int i = 0; i < 6; i++)
    {
        GenerateAndEquipItemSet(&party[i], 10, 1, "Starter ");
    }

    InitDisenchant();

    MSG msg = { 0 };
    BOOLEAN running = TRUE;

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    // Fixed timestep (for 60fps, dt = 1/60)
    const double deltaTime = 1.0 / 60.0;
    double accumulator = 0.0;
    LARGE_INTEGER previousTime;
    QueryPerformanceCounter(&previousTime);

    while (running)
    {
        // Process Windows messages.
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = FALSE;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        double frameTime = (double)(currentTime.QuadPart - previousTime.QuadPart) / frequency.QuadPart;
        previousTime = currentTime;
        accumulator += frameTime;

        // Update simulation in fixed increments.
        while (accumulator >= deltaTime)
        {
            UpdateCurrentState((float)deltaTime);
            UpdateDisenchant((float)deltaTime);
            RenderCurrentState();
            UpdateClientWindow();
            UpdateInput();
            
            accumulator -= deltaTime;
        }

        //// Optionally display the frame time for debugging.
        //double elapsedMs = frameTime * 1000.0;
        //wchar_t time[256] = { 0 };
        //swprintf_s(time, 256, L"Frame time: %.6f ms\n", elapsedMs);
        //OutputDebugStringW(time);

        Sleep(1);
    }

    return (int)msg.wParam;
}
