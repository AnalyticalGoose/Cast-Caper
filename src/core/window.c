#include "framebuffer.h"
#include "input.h"
#include "renderer.h"
#include "state_manager.h"
#include "title_state.h"
#include <time.h>
#include <windows.h>



static HDC hdc;

static int renderWidth = FRAME_WIDTH * FRAME_SCALE;
static int renderHeight = FRAME_HEIGHT * FRAME_SCALE;
static int xOffset = 0;
static int yOffset = 0;


static void ResizeDIBSection(HWND hWnd, int width, int height)
{
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);

	HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
	FillRect(hdc, &clientRect, blackBrush);

	float scaleX = (float)width / FRAME_WIDTH;
	float scaleY = (float)height / FRAME_HEIGHT;
	float scale = (scaleX < scaleY) ? scaleX : scaleY;

	renderWidth = (int)(FRAME_WIDTH * scale);
	renderHeight = (int)(FRAME_HEIGHT * scale);

	xOffset = (width - renderWidth) / 2;
	yOffset = (height - renderHeight) / 2;
}


void UpdateClientWindow()
{
	StretchDIBits(
		hdc,
		xOffset, yOffset, renderWidth, renderHeight,
		0, 0, FRAME_WIDTH, FRAME_HEIGHT,
		&frameBuffer,
		&bitmapInfo,
		DIB_RGB_COLORS,
		SRCCOPY
	);
	
	return;
}


static LRESULT CALLBACK
WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	//LRESULT result = 0;

	switch (uMsg)
	{
		case WM_KEYDOWN:
		{
			if (wParam < 256)
			{
				HandleKeyDown((int)wParam);
			}
		} break;

		case WM_KEYUP:
		{
			if (wParam < 256)
			{
				HandleKeyUp((int)wParam);
			}
		} break;

		case WM_SIZE:
		{
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);
			ResizeDIBSection(hWnd, width, height);
		} break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_CLOSE:
			PostQuitMessage(0);
			break;

		case WM_ACTIVATEAPP:
			OutputDebugStringA("WN_ACTIVATEAPP\n");
			break;

		default:
			//OutputDebugStringA("default\n");
			break;
		}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


BOOLEAN InitWindow(HINSTANCE hInstance, int nShowCmd)
{
	WNDCLASS windowClass = { 0 };	

	windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = L"Window";
	
	if (!RegisterClass(&windowClass)) return FALSE;

	HWND windowHandle =
		CreateWindowEx(
			0,
			windowClass.lpszClassName,
			L"DCJAM 2025",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, 
			CW_USEDEFAULT,
			FRAME_WIDTH * FRAME_SCALE,
			FRAME_HEIGHT * FRAME_SCALE,
			0,
			0,
			hInstance,
			0);

	if (!windowHandle) return FALSE;

	hdc = GetDC(windowHandle);

	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
	bitmapInfo.bmiHeader.biWidth = FRAME_WIDTH;
	bitmapInfo.bmiHeader.biHeight = -FRAME_HEIGHT;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;

	SetState(CreateTitleState(0));
	//SetState(CreateMovementState(0));
	
	srand((unsigned int)time(NULL)); // seed the random generator

	ShowWindow(windowHandle, SW_SHOWMAXIMIZED);
	return TRUE;
}