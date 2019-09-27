/*
    Apollo N64 Emulator (c) Eclipse Productions
    Copyright (C) 2001 Azimer (azimer@emulation64.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <windows.h>
#include <stdio.h>
#include "WinMain.h"
#include "EmuMain.h"
#include "console.h"
#include "afxres.h"
#include "resource.h"

/*												console.cpp
	Implements the Console Window =)

	Author			Date		Comment
	-----------		---------	----------------------------
	Azimer			12-04-99	Initial Version
*/

HWND DShWnd, DhWnd;
bool isHidden = false;


LRESULT CALLBACK ConsoleProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message) 
	{
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			// TODO: Add any drawing code here...
			RECT rt;
			GetClientRect(hWnd, &rt);
			// Draw here kthx
			EndPaint(hWnd, &ps);
			break;
		case WM_CREATE: {
			RECT rt;
			HFONT hSysFont;
			LOGFONT SysFont;
			
			GetClientRect(DShWnd, &rt);

			DhWnd = CreateWindowEx (WS_EX_CLIENTEDGE, "EDIT", 
			"", 
			ES_AUTOVSCROLL | WS_CHILD | ES_MULTILINE | ES_READONLY | WS_VSCROLL | WS_EX_CLIENTEDGE, 
			rt.left, 
			rt.top, 
			rt.left+320, 
			rt.top+240, 
			hWnd, 
			NULL, 
			GhInst, 
			0);

			SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &SysFont, 0);
			hSysFont = CreateFontIndirect(&SysFont);
			SendMessage(DhWnd, WM_SETFONT, (WPARAM) hSysFont, 0);

			ShowWindow(DhWnd, SW_SHOW);
		}
		break;
		case WM_CLOSE:
			ToggleConsoleWindow ();
			break;
		case WM_DESTROY:
			DShWnd = NULL;
			DestroyWindow(DhWnd);
			DhWnd = NULL;
			break;
		case WM_SIZE: {
			RECT rt;
		    //HDC hdc = GetDC(DhWnd);
			GetClientRect(DShWnd, &rt);
			SetWindowPos (DhWnd, 
				NULL, 
				rt.top, 
				rt.left, 
				rt.right-rt.left, 
				rt.bottom-rt.top, 
				SWP_NOOWNERZORDER | SWP_NOZORDER);
		}
		break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
   return 0;
}

void ToggleConsoleWindow () {
	if (isHidden == false) {
		ShowWindow (DShWnd, SW_HIDE);
		isHidden = true;
		SetFocus (GhWnd);
	} else {
		ShowWindow (DShWnd, SW_SHOW);
		isHidden = false;
		SetFocus (GhWnd);
	}
}

void ClearConsoleWindow () {
	SetWindowText (DhWnd, "");
}

void DestroyConsoleWindow () {
	DestroyWindow (DShWnd);
	DShWnd = NULL;
}
void CreateConsoleWindow (bool shouldHide) {
	RECT rt;
	isHidden = shouldHide;
	if (!DShWnd) {
		DShWnd = CreateWindowEx(0,
								"CONSOLE", 
								"Console Window", 
								WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT, 
								0, 
								CW_USEDEFAULT, 
								0, 
								NULL,//GhWnd, 
								NULL, 
								GhInst, 
								NULL);

	}
	GetClientRect(GhWnd, &rt);
	SetWindowPos(DShWnd, NULL, rt.right+8, rt.top, 320, 240+20, SWP_NOOWNERZORDER | SWP_NOZORDER);

	if (shouldHide == false)
		ShowWindow(DShWnd, SW_SHOW);
	else
		ShowWindow (DShWnd, SW_HIDE);
	SetFocus (GhWnd);
}

void Debug (int logtype, char *string, ...) {
	if (cpuIsReset == true)
		return;
	char buffer[300];
	va_list argptr;
	va_start(argptr, string);
	vsprintf(buffer, string, argptr);
	va_end(argptr);
	if (logtype & 0x1)
		MessageBox (GhWnd, buffer, WINTITLE, MB_OK);
#ifdef _DEBUG // only do logfiles in debug build
	switch (logtype) { // TODO: Meaningful logfiles
		case 0x2: // MessageBox and LogFile #1
		case 0x4: // MessageBox and LogFile #2
		case 0x6: // MessageBox and LogFile #3
		default: // Just the console window
			break;
	}
#endif
	strcat (buffer, "\r\n");
	dprintf (buffer);
}

void dprintf (char* string, ...){
	char buffer[300];
	va_list argptr;
	va_start(argptr, string);
	vsprintf(buffer, string, argptr);
	va_end(argptr);

	SetFocus (DhWnd);
	int ndx = GetWindowTextLength (DhWnd);
	//SendMessage (DhWnd, EM_SETSEL, 0, 1000);//(LPARAM)ndx);
	SendMessage (DhWnd, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
	SendMessage (DhWnd, EM_REPLACESEL, 0, (LPARAM) ((LPSTR) buffer));
	SendMessage (DhWnd, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
	if (RegSettings.enableConsoleLogging == true) {
		// We are logging here!
	}
	SetFocus (GhWnd);
}

ATOM RegisterConsoleClass (HINSTANCE hInstance) {
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)ConsoleProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(NULL, IDI_WINLOGO);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= "CONSOLE";
	wcex.hIconSm		= LoadIcon(NULL, IDI_WINLOGO);

	return RegisterClassEx(&wcex);
}