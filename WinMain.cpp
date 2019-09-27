const char * BETAINFO = 
"Thank you for helping beta test Apollo\r\
This beta belongs to: nobody                                          \r\
Unauthorized distribution of the beta is grounds for immediate removal\r\
New to R1:\r\
  -Added 4MB Expansion as an option\r\
  -Fixed FPU (Fixes Turok)\r\
  -Added Faster TLB Accessing\r\
  -Banjo Tooie/Jet Force Gemini Copyprotection support\r\
  -Partially working new internal audio code\r\
What's Next in R2:\r\
  -Fixed SaveStates\r\
  -Fixed Registry Settings\r\
  -Mario64 capable Dynarec\r\
  -Rombrowser plugin support\r\
  -Cheats Menu\r\
  -Speed Limiter\r\
";

/**************************************************************
  WinMain.cpp - Main Windows Functions and GUI Stuff

  History:

	Name		Date		Comment
  -------------------------------------------------------------
	Azimer		05-20-00	Initial Version	
 
 **************************************************************/

#include <windows.h>
#include <commctrl.h> // For status window
#include <stdio.h>
#include "afxres.h" // Needed for resource.h for some reason
#include "resource.h"
#include "WinMain.h"
#include "console.h"
#include "EmuMain.h"
#include "videodll.h"
#include "inputdll.h"
#include "audiodll.h"

// Global Variables
HWND GhWnd;	      // hwnd to the main window (G Means Global)
HWND GhWndPlugin = NULL;
HINSTANCE GhInst; // Application Instance
HMENU GhMenu; // Main Menu
RECT rcWindow; // Rectangle of the Main Window
HWND hwndStatus;
int LangUsed = MAKELANGID(LANG_NEUTRAL,SUBLANG_SYS_DEFAULT);
BOOL bFullScreen = FALSE;
BOOL bShouldFS   = FALSE;


struct RomInfoStruct	rInfo;
struct rSettings		RegSettings;

// File Scope Variables
bool isFullScreen = false;
char OpenRomString[] = "Nintendo64 Files (*.n64;*.v64;*.u64;*.bin;*.rom;*.z64)\0*.n64;*.v64;*.u64;*.bin;*.rom;*.z64;*.zip;*.pal;*.usa;*.jap;*.eur\0Apollo Test Files\0*.rdp\0Nintendo OBJ Files\0*.obj\0Apollo SaveState Files\0*.ass\0All Files (*.*)\0*.*\0\0";

// File Scope Prototypes
int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE, LPSTR CmdLine, int nCmdShow);
void StartGUI (HINSTANCE hInst, int nCmdShow);
void ShutDownGUI ();
HWND CreateMainWindow (int nCmdShow);
void SetClientRect(HWND Thwnd, RECT goal);
long FAR PASCAL WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK GenericProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

bool GetRomName(char *OpenRomBuffer);
void RecentMenus( char *addition );
void PrintDebugQueue (); // Defined for the console window

// WinMain - Start of the program.  Starts the GUI and gets things going

int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE, LPSTR CmdLine, int nCmdShow) {
	MSG msg;
	HINSTANCE hMain=NULL;
	//if (LoadCommandLine(CmdLine)) return -1;

	StartGUI (hInstance, nCmdShow); // Creates the main window and sets it all up for the emulator
	/*AllocConsole ();
	HANDLE shit = GetStdHandle(STD_OUTPUT_HANDLE);
	char *buffer = "Testing this shit";
	COORD dwCord;
	DWORD write;
	dwCord.X = 0; dwCord.Y = 0;
	SetConsoleTitle ("Apollo Debug Window");
	WriteConsoleOutputCharacter (shit, buffer, strlen(buffer), dwCord, &write);*/
	_asm align 16
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg); // allow use of keyboard
		DispatchMessage(&msg);  // return control to windows
		//WaitMessage();
	}
	//FreeConsole ();
	ShutDownGUI ();	      // Closes the GUI and check to make sure all free memory was released
	KillMemory();
	return (0);
}

// Sets up the GUI

void StartGUI (HINSTANCE hInst, int nCmdShow) {
	GhInst = hInst;
	LoadSettings ();
	//void InitInternalVideo(void);
	//InitInternalVideo();
	InitEmu ();
	GhWnd = CreateMainWindow (nCmdShow);
	LoadPlugins ();

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

}

void ShutDownGUI () {
	StopCPU ();
	SaveSettings ();
	DestroyConsoleWindow ();
	DestroyWindow (hwndStatus);
	DestroyWindow (GhWnd);
	UnLoadPlugins ();
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
	DWORD style = WS_POPUP|WS_SYSMENU|WS_CAPTION|WS_MINIMIZEBOX|WS_CLIPCHILDREN;

	client.right = 640;
	client.bottom = 480;

    int middleX = (GetSystemMetrics(SM_CXSCREEN)/2) - client.right/2;
	int middleY = (GetSystemMetrics(SM_CYSCREEN)/2) - client.bottom/2;

	newhwnd = CreateWindowEx(exStyle,"apollo",WINTITLE,style,middleX,
		middleY,client.right,client.bottom,NULL,NULL, GhInst,NULL);

	
	SetMenu(newhwnd,LoadMenuIndirect(LoadResource(NULL,FindResourceEx(NULL,RT_MENU,MAKEINTRESOURCE(ID_MENU),LangUsed))));

	ShowWindow(newhwnd, nCmdShow);
	UpdateWindow(newhwnd);
	SetFocus(newhwnd);

	GhMenu = GetMenu(newhwnd);
	
	if (RegSettings.RumblePack == false) {
		UNCHECK_MENU (ID_OPTIONS_RUMBLE);
	} else {
		CHECK_MENU (ID_OPTIONS_RUMBLE);
	}
	if (RegSettings.FullScreen == false) {
		bShouldFS = false;
		UNCHECK_MENU (ID_OPTIONS_FULLSCREEN);
	} else {
		bShouldFS = true;
		CHECK_MENU (ID_OPTIONS_FULLSCREEN);
	}
	if (RegSettings.compressSaveStates == false) {
		UNCHECK_MENU (ID_CPU_COMPRESS);
	} else {
		CHECK_MENU (ID_CPU_COMPRESS);
	}
	if (RegSettings.force4keep == false) {
		UNCHECK_MENU (ID_OPTIONS_4KEEP);
	} else {
		CHECK_MENU (ID_OPTIONS_4KEEP);
	}
	if (RegSettings.isPakInstalled == false) {
		UNCHECK_MENU (ID_OPTIONS_8MB);
	} else {
		CHECK_MENU (ID_OPTIONS_8MB);
	}
	if (RegSettings.dynamicEnabled == false)
		UNCHECK_MENU (ID_CPU_DYNAREC_ENABLED);
	else
		CHECK_MENU (ID_CPU_DYNAREC_ENABLED);
	UNCHECK_MENU (ID_CPU_PAUSE);
	UNCHECK_MENU (ID_OPTIONS_VSYNCHACK_1); 
	UNCHECK_MENU (ID_OPTIONS_VSYNCHACK_2);
	UNCHECK_MENU (ID_OPTIONS_VSYNCHACK_3);
	UNCHECK_MENU (ID_OPTIONS_VSYNCHACK_4);
	switch (RegSettings.VSyncHack) {
		case 1:
			CHECK_MENU (ID_OPTIONS_VSYNCHACK_1); 
		break;
		case 2:
			CHECK_MENU (ID_OPTIONS_VSYNCHACK_2);
		break;
		case 3:
			CHECK_MENU (ID_OPTIONS_VSYNCHACK_3);
		break;
		case 4:
			CHECK_MENU (ID_OPTIONS_VSYNCHACK_4);
		break;
	}
	RecentMenus(NULL);

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

bool printreaddata = false;

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
				case ID_HELP_BETAINFO: {
					MessageBox (hWnd, BETAINFO, "Beta Information", MB_OK);
				} break;
				case ID_FILE_CLOSE:
					if (bFullScreen == TRUE) {
						gfxdll.ChangeWindow ();
						bFullScreen = FALSE;
					}
					StopCPU ();
				break;
				case ID_FILE_ROMINFO:
					dprintf (L_STR(IDS_NEED_IMPLEMENT_ROMINFO));
				break;
				case ID_FILE_SCREENSHOT:
					if (gfxdll.CaptureScreen != NULL) {
						char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR];
						char fname[_MAX_FNAME],ext[_MAX_EXT];
						char dllpath[_MAX_PATH];

						GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
						_splitpath(path_buffer,drive,dir,fname,ext);
						strcpy (dllpath, drive);
						strcat (dllpath, dir);
						gfxdll.CaptureScreen(dllpath);
						Debug (0, "Screenshot Taken");
					} else {
						Debug (1, "This graphics DLL doesn't support Screenshots");
					}
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
						RegSettings.dynamicEnabled = false;
						cpuIsReset = true;
						UNCHECK_MENU (ID_CPU_DYNAREC_ENABLED);
					} else {
						RegSettings.dynamicEnabled = true;
						cpuIsReset = true;
						CHECK_MENU (ID_CPU_DYNAREC_ENABLED);
					}
				break;
				case ID_CPU_SAVESTATE:
					cpuSaveState = true;
					cpuIsReset = true;
				break;
				case ID_CPU_LOADSTATE:
					cpuLoadState = true;
					cpuIsReset = true;
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
					SaveSettings ();
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
					SaveSettings ();
				break;
				case ID_HELP_ABOUT:
					DialogBoxIndirect(GhInst,(DLGTEMPLATE*)LoadResource(NULL,FindResourceEx(NULL,RT_DIALOG,MAKEINTRESOURCE(IDD_DLGABOUT),LangUsed)),GhWnd,(DLGPROC)GenericProc);
					//DialogBox(GhInst,MAKEINTRESOURCE(IDD_DLGABOUT),GhWnd,(DLGPROC)GenericProc);
					break;
				case ID_CPU_R4KDEBUGGER:
					//void InvokeOpTester ();
					//InitializeDebugger();
					//InvokeOpTester ();
					break;
				case ID_CPU_COMPRESS: {
					if (GET_MENU(ID_CPU_COMPRESS) == MF_CHECKED) {
						RegSettings.compressSaveStates = false;
						UNCHECK_MENU (ID_CPU_COMPRESS);
					} else {
						RegSettings.compressSaveStates = true;
						CHECK_MENU (ID_CPU_COMPRESS);
					}
					SaveSettings ();
				}
				break;
				case ID_OPTIONS_VIDEO:
					if (cpuIsPaused == false) {
						ToggleCPU ();
						if (gfxdll.DllConfig)
							gfxdll.DllConfig(hWnd);
						ToggleCPU ();
					} else {
						if (gfxdll.DllConfig)
							gfxdll.DllConfig(hWnd);
					}
				break;
				case ID_OPTIONS_INPUT:
					if (cpuIsPaused == false) {
						ToggleCPU ();
						if (inpdll.DllConfig)
							inpdll.DllConfig(hWnd);
						ToggleCPU ();
					} else {
						if (inpdll.DllConfig)
							inpdll.DllConfig(hWnd);
					}
				break;
				case ID_OPTIONS_SOUND:
					if (cpuIsPaused == false) {
						ToggleCPU ();
						if (snddll.DllConfig)
							snddll.DllConfig(hWnd);
						ToggleCPU ();
					} else {
						if (snddll.DllConfig)
							snddll.DllConfig(hWnd);
					}
				break;
				case ID_OPTIONS_CONTROLLER:
					BOOL CALLBACK PluginProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
					DialogBoxIndirect(GhInst,(DLGTEMPLATE*)LoadResource(NULL,FindResourceEx(NULL,RT_DIALOG,MAKEINTRESOURCE(IDD_PLUGINS),LangUsed)),GhWnd,(DLGPROC)PluginProc);
					/*if (!GhWndPlugin) {
						GhWndPlugin = CreateDialogIndirect(GhInst,(DLGTEMPLATE*)LoadResource(NULL,FindResourceEx(NULL,RT_DIALOG,MAKEINTRESOURCE(IDD_PLUGINS),LangUsed)),GhWnd,(DLGPROC)PluginProc);
						ShowWindow(GhWndPlugin, SW_SHOW);
					}*/
					SaveSettings ();
					break;
				case ID_OPTIONS_RUMBLE: {
					if (GET_MENU(ID_OPTIONS_RUMBLE) == MF_CHECKED) {
						RegSettings.RumblePack = false;
						UNCHECK_MENU (ID_OPTIONS_RUMBLE);
					} else {
						RegSettings.RumblePack = true;
						CHECK_MENU (ID_OPTIONS_RUMBLE);
					}
					SaveSettings ();
				}
				break;
	
				case ID_OPTION_FULLSCREEN: {
					if (GET_MENU(ID_OPTIONS_FULLSCREEN) == MF_CHECKED) {
						RegSettings.FullScreen = false;
						UNCHECK_MENU (ID_OPTIONS_FULLSCREEN);
						bShouldFS   = FALSE;
						if (cpuIsDone == false) {
							bFullScreen = bShouldFS;
							gfxdll.ChangeWindow ();
						}
					} else {
						RegSettings.FullScreen = true;
						CHECK_MENU (ID_OPTIONS_FULLSCREEN);
						bShouldFS	= TRUE;
						if (cpuIsDone == false) {
							bFullScreen = bShouldFS;
							gfxdll.ChangeWindow ();
						}
					}
				}
				break;
				case ID_OPTIONS_VSYNCHACK_1:
					CHECK_MENU (ID_OPTIONS_VSYNCHACK_1); UNCHECK_MENU (ID_OPTIONS_VSYNCHACK_2);
					UNCHECK_MENU (ID_OPTIONS_VSYNCHACK_3);    UNCHECK_MENU (ID_OPTIONS_VSYNCHACK_4);
					RegSettings.VSyncHack = 1;
				break;
				case ID_OPTIONS_VSYNCHACK_2:
					UNCHECK_MENU (ID_OPTIONS_VSYNCHACK_1); CHECK_MENU (ID_OPTIONS_VSYNCHACK_2);
					UNCHECK_MENU (ID_OPTIONS_VSYNCHACK_3);    UNCHECK_MENU (ID_OPTIONS_VSYNCHACK_4);
					RegSettings.VSyncHack = 2;
				break;
				case ID_OPTIONS_VSYNCHACK_3:
					UNCHECK_MENU (ID_OPTIONS_VSYNCHACK_1); UNCHECK_MENU (ID_OPTIONS_VSYNCHACK_2);
					CHECK_MENU (ID_OPTIONS_VSYNCHACK_3);    UNCHECK_MENU (ID_OPTIONS_VSYNCHACK_4);
					RegSettings.VSyncHack = 3;
				break;
				case ID_OPTIONS_VSYNCHACK_4:
					UNCHECK_MENU (ID_OPTIONS_VSYNCHACK_1); UNCHECK_MENU (ID_OPTIONS_VSYNCHACK_2);
					UNCHECK_MENU (ID_OPTIONS_VSYNCHACK_3);    CHECK_MENU (ID_OPTIONS_VSYNCHACK_4);
					RegSettings.VSyncHack = 4;
				break;
				case ID_OPTIONS_4KEEP: {
					if (GET_MENU(ID_OPTIONS_4KEEP) == MF_CHECKED) {
						RegSettings.force4keep = false;
						UNCHECK_MENU (ID_OPTIONS_4KEEP);
					} else {
						RegSettings.force4keep = true;
						CHECK_MENU (ID_OPTIONS_4KEEP);
					}
					SaveSettings ();
				} break;
				case ID_OPTIONS_8MB: {
					if (cpuIsDone == false) {
						if (MessageBox (GhWnd, "This will reset your rom image\r\n... Are you sure you want to do this?", WINTITLE, MB_YESNO) == FALSE) {
							break;
						}
					}
					if (GET_MENU(ID_OPTIONS_8MB) == MF_CHECKED) {
						RegSettings.isPakInstalled = false;
						UNCHECK_MENU (ID_OPTIONS_8MB);
					} else {
						RegSettings.isPakInstalled = true;
						CHECK_MENU (ID_OPTIONS_8MB);
					}
					if (cpuIsDone == false) {
						cpuIsPaused = true;
						cpuIsReset = true;
					}
					void ChangeRDRAMSize (int);
					ChangeRDRAMSize (4*1024*1024 << (RegSettings.isPakInstalled ? 1 : 0));
					cpuIsPaused = false;

					SaveSettings ();
				} break;
				default:  // Recent Files 
					if (MenuId >= ID_FILE_RECENT_FILE && MenuId < (ID_FILE_RECENT_FILE + 8)) {
						//char buff[64];
						//exit_n64();
						//GetMenuString(GhMenu, MenuId, buff, 64, MF_BYCOMMAND);
						OpenROM (RegSettings.lastRom[MenuId - ID_FILE_RECENT_FILE]);
					}
					break;
			} // end switch (MenuId)
		}
		break;

		case WM_TIMER:
			PrintDebugQueue ();
			return 0;
		break;

		case WM_SIZE: {
			if (bFullScreen == TRUE)
				break;
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
			if (gfxdll.MoveScreen)
				gfxdll.MoveScreen ((int)(short) LOWORD(lParam), (int)(short) HIWORD(lParam));
			break;

		case WM_PAINT:
			BeginPaint(GhWnd, &ps);
			if (gfxdll.DrawScreen)
				gfxdll.DrawScreen();
			EndPaint(GhWnd, &ps);
			break;

		case WM_KEYDOWN:
			//inpdll.WM_KeyDown (wParam, lParam);
			
			switch (wParam) {
				case 27: // ESC - Stop
					SendMessage (hWnd, WM_COMMAND, ID_FILE_CLOSE, 0);
				break;
				case 49: case 50: case 51: case 52: 
				case 53: case 54: case 55: case 56:// 1-8
					if (RegSettings.recentSize > (int)(wParam-49)) {
						OpenROM (RegSettings.lastRom[wParam-49]);
					}
				break;
				case 112: // F1 - About Dialog Box
					SendMessage (hWnd, WM_COMMAND, ID_HELP_ABOUT, 0);
				break;
				case 113: // F2 - Save State
					SendMessage (hWnd, WM_COMMAND, ID_CPU_SAVESTATE, 0);
				break;
				case 114: // F3 - Clear Debug window
					ClearConsoleWindow ();
				break;
				case 115: // F4 - Load State
					SendMessage (hWnd, WM_COMMAND, ID_CPU_LOADSTATE, 0);
				break;
				case 116: // F5 - Reset
					SendMessage (hWnd, WM_COMMAND, ID_CPU_RESET, 0);
				break;
				case 117: // F6 - Video
					SendMessage (hWnd, WM_COMMAND, ID_OPTIONS_VIDEO, 0);
				break;
				case 118: // F7 - Input
					SendMessage (hWnd, WM_COMMAND, ID_OPTIONS_INPUT, 0);
				break;
				case 119: // F8 - Audio
					SendMessage (hWnd, WM_COMMAND, ID_OPTIONS_SOUND, 0);
				break;
				case 120: // F9 - Pause
					SendMessage (hWnd, WM_COMMAND, ID_CPU_PAUSE, 0);
				break;
				case 122: // F11 - Screenshot
					SendMessage (hWnd, WM_COMMAND, ID_FILE_SCREENSHOT, 0);
				break;
				case 123: // F12 - FullScreen
					SendMessage (hWnd, WM_COMMAND, ID_OPTION_FULLSCREEN, 0);
					//void PrintInterruptStatus ();
					//PrintInterruptStatus ();
				break;
				default:
					//Debug (0, "Key: %i", wParam);
					inpdll.WM_KeyDown (wParam, lParam);
			}
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


void RecentMenus(char *addition)
{
	BOOL matched = FALSE;
	int x = 0;
	int position=0;
	char buff[255];

	if (addition != NULL) { // Then we are just building the table...
		for (x = 0; x < RegSettings.recentSize; x++) {
			if (strcmp(addition, RegSettings.lastRom[x]) == 0) {
				position = x;
				matched = TRUE;
			}
		}

		if (matched == TRUE) {
			strcpy (buff, RegSettings.lastRom[position]); // Save this...
			for (x = position; x > 0; x--) {
				strcpy (RegSettings.lastRom[x], RegSettings.lastRom[x-1]); // Move it all down the list
			}
			strcpy (RegSettings.lastRom[0], buff);
			} else { // New edition to the family
				if (RegSettings.recentSize < 8)
				RegSettings.recentSize++;
			for (x = RegSettings.recentSize-1; x > 0; x--)
				strcpy (RegSettings.lastRom[x], RegSettings.lastRom[x-1]); // Move it all down the list
			strcpy (RegSettings.lastRom[0], addition);
		}
	}

	MENUITEMINFO menuinfo;
	HMENU hRecent = GetSubMenu(GetSubMenu(GhMenu, 0), 2);

	memset(&menuinfo, 0, sizeof(MENUITEMINFO));

	for (x = 0; x < 8; x++) {
		DeleteMenu(hRecent, ID_FILE_RECENT_FILE + x, MF_BYCOMMAND);
	}

	for (x = 0; x < RegSettings.recentSize; x++) {

		menuinfo.cbSize = sizeof(MENUITEMINFO);
		menuinfo.fMask = MIIM_TYPE|MIIM_ID;
		menuinfo.fType = MFT_STRING;
		menuinfo.fState = MFS_ENABLED|MFS_UNCHECKED;

		menuinfo.wID = ID_FILE_RECENT_FILE + x;
		sprintf (buff, "&%i ", x+1);
		strcat (buff, RegSettings.lastRom[x]);

		menuinfo.dwTypeData = buff;
		menuinfo.cch = 4;

		InsertMenuItem(hRecent, ID_FILE_RECENT_FILE + x, FALSE, &menuinfo);
	}
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

