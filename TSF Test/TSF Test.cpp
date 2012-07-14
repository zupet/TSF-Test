// TSF Test.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "TSF Test.h"

#include "t_tsf.h"


/*---------------------------------------------------------------------------*/ 
/*                                                                           */ 
/*---------------------------------------------------------------------------*/ 

LRESULT CALLBACK		WndProc(HWND, UINT, WPARAM, LPARAM);

TTsfMgr					g_tsfThreadMgr;
TTsfTextStore*			g_tsfTextStore;

/*---------------------------------------------------------------------------*/ 
/*                                                                           */ 
/*---------------------------------------------------------------------------*/ 
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	TCHAR szTitle[100];					
	TCHAR szWindowClass[100];			

	LoadString(hInstance, IDS_APP_TITLE, szTitle, 100);
	LoadString(hInstance, IDC_TSFTEST, szWindowClass, 100);

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_TSFTEST);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);

	HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 500, 600, NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	g_tsfThreadMgr.InitTsfThreadMgr();
	g_tsfTextStore = g_tsfThreadMgr.CreateTsfTextStore(hWnd);

	MSG msg = { 0, };
	while(msg.message != WM_QUIT)
    {
		if(g_tsfThreadMgr.GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
    }

	g_tsfThreadMgr.DestoryTsfTextStore(g_tsfTextStore);
	g_tsfThreadMgr.ClearTextServiceMgr();

	return (int) msg.wParam;
}

/*---------------------------------------------------------------------------*/ 
/*                                                                           */ 
/*---------------------------------------------------------------------------*/ 
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_SETFOCUS:
		g_tsfThreadMgr.SetFocus();
		break;

	case WM_KEYDOWN:
		if(wParam == VK_LEFT)
		{
			g_tsfTextStore->MoveLeft(GetAsyncKeyState(VK_SHIFT) & 0x80000000 ? true : false);
		}
		else if(wParam == VK_RIGHT)
		{
			g_tsfTextStore->MoveRight(GetAsyncKeyState(VK_SHIFT) & 0x80000000 ? true : false);
		}
		break;

	case WM_CHAR:
		if(iswcntrl((wchar_t)wParam) == 0)
		{
			g_tsfTextStore->AddText((wchar_t*)&wParam, 1);
		}
		else if(wParam == '\b')
		{
			g_tsfTextStore->DeleteText();
		}
		else if(wParam == '\r')
		{
			g_tsfTextStore->ClearText();
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		g_tsfTextStore->ShowText(hdc);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

/*---------------------------------------------------------------------------*/ 
