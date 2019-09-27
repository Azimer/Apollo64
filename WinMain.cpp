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

/**************************************************************
  WinMain.cpp - Main Windows Functions and GUI Stuff

  History:

	Name		Date		Comment
  -------------------------------------------------------------
	Azimer		05-20-00	Initial Version	
 
 **************************************************************/

#include <windows.h>
#include <commctrl.h> // For status window
#include "afxres.h" // Needed for resource.h for some reason
#include "resource.h"
#include "WinMain.h"
#include "console.h"
#include "EmuMain.h"
#include "videodll.h"
#include "inputdll.h"

// Global Variables
HWND GhWnd;	      // hwnd to the main window (G Means Global)
HWND GhWndPlugin = NULL;
HINSTANCE GhInst; // Application Instance
HMENU GhMenu; // Main Menu
RECT rcWindow; // Rectangle of the Main Window
HWND hwndStatus;
int LangUsed = MAKELANGID(LANG_NEUTRAL,SUBLANG_SYS_DEFAULT);

struct RomInfoStruct	rInfo;
struct rSettings		RegSettings;

// File Scope Variables
bool isFullScreen = false;
char OpenRomString[] = "Nintendo64 Files (*.n64; *.bin)\0*.n64;*.v64;*.u64;*.bin;*.rom\0Apollo Test Files\0*.rdp\0Nintendo OBJ Files\0*.obj\0Apollo SaveState Files\0*.ec1\0All Files (*.*)\0*.*\0\0";

// File Scope Prototypes
int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE, LPSTR CmdLine, int nCmdShow);
void StartGUI (HINSTANCE hInst, int nCmdShow);
void ShutDownGUI ();
HWND CreateMainWindow (int nCmdShow);
void SetClientRect(HWND Thwnd, RECT goal);
long FAR PASCAL WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK GenericProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

bool GetRomName(char *OpenRomBuffer);

void (__cdecl* RunGUI)( void );

// WinMain - Start of the program.  Starts the GUI and gets things going

int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE, LPSTR CmdLine, int nCmdShow) {
	MSG msg;
	HINSTANCE hMain=NULL;
	//if (LoadCommandLine(CmdLine)) return -1;

	StartGUI (hInstance, nCmdShow); // Creates the main window and sets it all up for the emulator
	_asm align 16
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg); // allow use of keyboard
		DispatchMessage(&msg);  // return control to windows
		//WaitMessage();
	}
	ShutDownGUI ();	      // Closes the GUI and check to make sure all free memory was released
	KillMemory();
	return (0);
}

// Sets up the GUI

void StartGUI (HINSTANCE hInst, int nCmdShow) {
	GhInst = hInst;
	LoadSettings ();
	void InitInternalVideo(void);
	InitInternalVideo();
	/*
	if (gfxdll.Load (RegSettings.vidDll) == FALSE) {
		Debug (1, "Unable to load %s, falling back to Internal Video Plugin", RegSettings.vidDll);
	}/*
	if (snddll.Load (RegSettings.sndDll) == FALSE) {
		Debug (1, "Unable to load %s, falling back to Internal Sound Plugin", RegSettings.sndDll);
	}*/
	if (inpdll.Load (RegSettings.inpDll) == FALSE) {
		Debug (1, "Unable to load %s, falling back to Internal Input Plugin", RegSettings.inpDll);
	}
	GhWnd = CreateMainWindow (nCmdShow);
	// Set the default states
	if (RegSettings.enableConsoleWindow == true)
		CHECK_MENU (ID_CONSOLE_TOGGLE);
	else
		UNCHECK_MENU (ID_CONSOLE_TOGGLE);

	if (RegSettings.enableConsoleLogging == true)
		CHECK_MENU (ID_CONSOLE_LOG);
	else
		UNCHECK_MENU (ID_CONSOLE_LOG);
	RegisterConsoleClass(hInst);
	CreateConsoleWindow (!RegSettings.enableConsoleWindow); // false signifies to show the console window
	dprintf (L_STR(IDS_CONSOLE_READY));

	InitEmu ();
}

void ShutDownGUI () {
	StopCPU ();
	gfxdll.Close();
	//snddll.Close();
	inpdll.Close();
	SaveSettings ();
	DestroyConsoleWindow ();
	DestroyWindow (hwndStatus);
	DestroyWindow (GhWnd);
}

// Creates the Main Application Window (Code reused)

HWND CreateMainWindow (int nCmdShow) {
	HWND newhwnd;
	RECT client;
	WNDCLASS wc;

	// Create the Window class and the Window ...
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GhInst;
	wc.hIcon = LoadIcon(GhInst, MAKEINTRESOURCE(IDI_APOLLO));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (struct HBRUSH__ *)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;//MAKEINTRESOURCE(ID_MENU);
	wc.lpszClassName = "apollo";
	RegisterClass(&wc);
	DWORD exStyle = 0;
	DWORD style = WS_POPUP|WS_SYSMENU|WS_CAPTION|WS_MINIMIZEBOX;

	client.right = 320;
	client.bottom = 240;

    int middleX = (GetSystemMetrics(SM_CXSCREEN)/2) - client.right/2;
	int middleY = (GetSystemMetrics(SM_CYSCREEN)/2) - client.bottom/2;

	newhwnd = CreateWindowEx(exStyle,"apollo",WINTITLE,style,middleX,
		middleY,client.right,client.bottom,NULL,NULL, GhInst,NULL);

	
	SetMenu(newhwnd,LoadMenuIndirect(LoadResource(NULL,FindResourceEx(NULL,RT_MENU,MAKEINTRESOURCE(ID_MENU),LangUsed))));

	ShowWindow(newhwnd, nCmdShow);
	UpdateWindow(newhwnd);
	SetFocus(newhwnd);
	

	GhMenu = GetMenu(newhwnd);
	
	hwndStatus = CreateStatusWindow(WS_VISIBLE|WS_CHILD, L_STR(IDS_READY), newhwnd, IDC_STATUS);

	SetClientRect(newhwnd, client);	

	return(newhwnd);
}

void SetClientRect(HWND Thwnd, RECT goal)
{
	RECT client;
	int width[3];
	int xscr = GetSystemMetrics(SM_CXSCREEN);

	GetClientRect(Thwnd, &client);

    int middleX = (GetSystemMetrics(SM_CXSCREEN)/2) - goal.right/2;
	int middleY = (GetSystemMetrics(SM_CYSCREEN)/2) - goal.bottom/2;

	if (isFullScreen == true) {
		SetWindowPos(Thwnd, HWND_TOPMOST, 0, 0, goal.right, goal.bottom, SWP_SHOWWINDOW);
		SetWindowLong(Thwnd, GWL_STYLE, WS_VISIBLE);
		SetMenu(Thwnd, NULL);
		ShowWindow (hwndStatus, SW_HIDE);
	} else {	
		SetWindowPos(Thwnd, NULL, middleX, middleY, goal.right+6, goal.bottom+64, SWP_SHOWWINDOW);
		SetWindowPos(hwndStatus, NULL, goal.left, goal.bottom-20, goal.right, goal.bottom, SWP_SHOWWINDOW);
		SetWindowLong(Thwnd, GWL_STYLE, WS_CAPTION|WS_BORDER|WS_VISIBLE|WS_SYSMENU|WS_MINIMIZEBOX);
		SetMenu(Thwnd, GhMenu);
		width[0] = (int)((goal.right) / 1.3f);
		width[1] = -1;
		SendMessage(hwndStatus, SB_SETPARTS, (WPARAM) 2, (LPARAM) (LPINT) width);
		ShowWindow (hwndStatus, SW_SHOW);
	}

	PostMessage(GhWnd, WM_MOVE, 0, 0);
}

char L_STR_buffer[512] = "";
u16* StringResource = NULL;;

char* L_STR(int id) {
	int i;int base = 0;

	if (!StringResource){//Resources not loaded, so load.
		HRSRC hrsrc1 = FindResourceEx(NULL,RT_STRING,MAKEINTRESOURCE(id),LangUsed);
		StringResource = (u16*)LoadResource(NULL,hrsrc1);
		if (!StringResource) return "";
	}
	for (i=1; i!=id; i++) { if (!StringResource[base+1]) i--; base += StringResource[base+1] + 1; }

	for (i=0; i < StringResource[1+base]; i++) L_STR_buffer[i] = (char)StringResource[2+i+base];

	L_STR_buffer[i] = 0;
	return L_STR_buffer;
}

// Windows Procedure...  Last Fucntion in this file since it's going to
// be so large

long FAR PASCAL WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) {
	int MenuId;
	BOOL bCpu = FALSE;
	PAINTSTRUCT ps;

extern bool volatile cpuIsDone;
extern bool volatile cpuIsReset;
extern bool cpuIsPaused;
void OpenROM (char *filename);
void StopCPU (void);
void ToggleCPU (void);

	switch(message) {
		case WM_COMMAND: {
		MenuId = LOWORD(wParam);
			switch(MenuId) {
				case ID_FILE_OPEN: {
					char filename[275];
					if (cpuIsPaused == false)
						ToggleCPU ();
					if (GetRomName (filename)) {
						OpenROM (filename);
					} else {
						dprintf (L_STR(IDS_LOAD_CANCEL));
						if (cpuIsPaused == true)
							ToggleCPU ();
					}
				}
				break;
				case ID_FILE_CLOSE:
					StopCPU ();
				break;
				case ID_FILE_ROMINFO:
					dprintf (L_STR(IDS_NEED_IMPLEMENT_ROMINFO));
				break;
				case ID_FILE_SCREENSHOT:
					dprintf (L_STR(IDS_NEED_IMPLEMENT_SCREENSHOT));
				break;
				case ID_FILE_EXIT:
					PostQuitMessage(0);
					break;
				case ID_CPU_PAUSE:
					ToggleCPU ();
				break;
				case ID_CPU_RESET:
					cpuIsReset = true;
				break;
				case ID_CPU_DYNAREC_ROMTLB:
					if (GET_MENU(ID_CPU_DYNAREC_ROMTLB) == MF_CHECKED) {
//						cpuDynarecTlb = false;
						UNCHECK_MENU (ID_CPU_DYNAREC_ROMTLB);
					} else {
//						cpuDynarecTlb = true;
						CHECK_MENU (ID_CPU_DYNAREC_ROMTLB);
					}
				break;
				case ID_CPU_DYNAREC_ROMMOD:
					if (GET_MENU(ID_CPU_DYNAREC_ROMMOD) == MF_CHECKED) {
//						cpuDynarecMod = false;
						UNCHECK_MENU (ID_CPU_DYNAREC_ROMMOD);
					} else {
//						cpuDynarecMod = true;
						CHECK_MENU (ID_CPU_DYNAREC_ROMMOD);
					}
				break;
				case ID_CPU_DYNAREC_ENABLED:
					if (GET_MENU(ID_CPU_DYNAREC_ENABLED) == MF_CHECKED) {
//						dynamicEnabled = false;
						UNCHECK_MENU (ID_CPU_DYNAREC_ENABLED);
					} else {
//						dynamicEnabled = true;
						CHECK_MENU (ID_CPU_DYNAREC_ENABLED);
					}
				break;
				case ID_CONSOLE_TOGGLE:
					if (GET_MENU(ID_CONSOLE_TOGGLE) == MF_CHECKED) {
						RegSettings.enableConsoleWindow = false;
						UNCHECK_MENU (ID_CONSOLE_TOGGLE);
					} else {
						RegSettings.enableConsoleWindow = true;
						CHECK_MENU (ID_CONSOLE_TOGGLE);
					}
					ToggleConsoleWindow ();
				break;
				case ID_CONSOLE_CLEAR: 
					ClearConsoleWindow ();
				break;
				case ID_CONSOLE_LOG:
					if (GET_MENU(ID_CONSOLE_LOG) == MF_CHECKED){
						RegSettings.enableConsoleLogging = false;
						UNCHECK_MENU (ID_CONSOLE_LOG);
					} else {
						RegSettings.enableConsoleLogging = true;
						CHECK_MENU (ID_CONSOLE_LOG);
					}
				break;
				case ID_HELP_ABOUT:
					DialogBoxIndirect(GhInst,(DLGTEMPLATE*)LoadResource(NULL,FindResourceEx(NULL,RT_DIALOG,MAKEINTRESOURCE(IDD_DLGABOUT),LangUsed)),GhWnd,(DLGPROC)GenericProc);
					//DialogBox(GhInst,MAKEINTRESOURCE(IDD_DLGABOUT),GhWnd,(DLGPROC)GenericProc);
					break;
				case ID_CPU_R4KDEBUGGER:
					//void InvokeOpTester ();
					InitializeDebugger();
					//InvokeOpTester ();
					break;
				case ID_OPTIONS_CONTROLLER:
					BOOL CALLBACK PluginProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
					if (!GhWndPlugin) {
						GhWndPlugin = CreateDialogIndirect(GhInst,(DLGTEMPLATE*)LoadResource(NULL,FindResourceEx(NULL,RT_DIALOG,MAKEINTRESOURCE(IDD_PLUGINS),LangUsed)),GhWnd,(DLGPROC)PluginProc);
						ShowWindow(GhWndPlugin,SW_SHOW);
					}
					break;
	
				default: /* // Recent Files 
					if (MenuId >= ID_FILE_RECENT_FILE && MenuId < (ID_FILE_RECENT_FILE + 8)) {
						char buff[64];
						//exit_n64();
						GetMenuString(GhMenu, MenuId, buff, 64, MF_BYCOMMAND);
						MessageBox (GhWnd, buff, "Recent File", MB_OK);
						//OnOpen(buff);
					} */
					break;
			} // end switch (MenuId)
		}
		break;

		case WM_SIZE: {
			RECT goal;
			int width[2];
			GetClientRect (GhWnd, &goal);
			SetWindowPos(hwndStatus, NULL, goal.left, goal.bottom-20, goal.right, goal.bottom, SWP_SHOWWINDOW);
			width[0] = (int)((goal.right) / 1.3f);
			width[1] = -1;
			SendMessage(hwndStatus, SB_SETPARTS, (WPARAM) 2, (LPARAM) (LPINT) width);
			ShowWindow (hwndStatus, SW_SHOW);
			InvalidateRect (hwndStatus, &goal, TRUE);
			}
			break;

		case WM_MOVE:
			gfxdll.MoveScreen ((int)(short) LOWORD(lParam), (int)(short) HIWORD(lParam));
			break;

		case WM_PAINT:
			BeginPaint(GhWnd, &ps);
			gfxdll.DrawScreen();
			EndPaint(GhWnd, &ps);
			break;

		case WM_KEYDOWN:
			inpdll.WM_KeyDown (wParam, lParam);
			break;

		case WM_KEYUP:
			inpdll.WM_KeyUp (wParam, lParam);
			break;

		case WM_DESTROY:
			//exit_n64();
			StopCPU ();
			PostQuitMessage(0);
			break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

// Generic function to control dialogs that don't really need a complex dialog proc

BOOL CALLBACK GenericProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int MenuId;

	switch(message)
	{
		case WM_INITDIALOG: {
			char buffer[100];
			strcpy (buffer, WINTITLE);
			strcat (buffer, L_STR(IDS_TITLE_SUFFIX));
			SetDlgItemText (hWnd, IDC_LASTCOMP, LASTCOMPILED);
			SetDlgItemText (hWnd, IDC_WINTITLE, buffer);
			break;
		}
		case WM_COMMAND:
			MenuId = LOWORD(wParam);
			if ((MenuId == IDCANCEL)||(MenuId == IDOK)) {
				EndDialog(hWnd,0);
				return TRUE;
			}
	}
	return FALSE;
}


bool GetRomName(char *OpenRomBuffer) {
	char buffer[275] = "";
	OPENFILENAME ofn;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = GhWnd;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = OpenRomString;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buffer;
	ofn.nMaxFile = 275;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = RegSettings.lastDir;
	ofn.lpstrTitle = L_STR(IDS_OPEN_TITLE);
	ofn.Flags = OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = NULL;
	ofn.lCustData = 0;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;

	//bool CpuPause = exit_n64(); // TODO: Remember to pause the CPU

	if (GetOpenFileName(&ofn)==FALSE) {
		// user hit cancel resume
		//if (CpuPause) resume_n64(FALSE); Remember to unpause the CPU
		return false;
	}

	ZeroMemory(RegSettings.lastDir,sizeof(RegSettings.lastDir));
	for (int i=strlen(buffer); i!=0; i--) {
		if (buffer[i]=='\\'){
			strncpy(RegSettings.lastDir,buffer,i);
		}
	}
	strcpy(OpenRomBuffer,buffer);
	return true;
}

