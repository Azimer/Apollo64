#include <windows.h>
#include <stdio.h>
#include "debug.h"
#include "resource.h"
#include "driver.h"

// Declared in driver.c (Will ALWAYS assume it's LSB Endianess on a DWORD alignment)
extern char *pRDRAM;
extern char *pDMEM;
extern char *pIMEM;

BOOL CALLBACK MemViewProc ( HWND hwndDlg,
							UINT uMsg,  
							WPARAM wParam,
							LPARAM lParam );

void Debugger_Open () {
	DialogBox (g_hInstance, MAKEINTRESOURCE(IDD_MEMVIEW), g_hWnd, MemViewProc);
}