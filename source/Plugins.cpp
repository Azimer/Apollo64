/**************************************************************************
 *                                                                        *
 *               Copyright (C) 2000, Eclipse Productions                  *
 *                                                                        *
 *  Offical Source code of the Apollo Project.  DO NOT DISTRIBUTE!  Any   *
 *  unauthorized distribution of these files in any way, shape, or form   *
 *  is a direct infringement on the copyrights entitled to Eclipse        *
 *  Productions and you will be prosecuted to the fullest extent of       *
 *  the law.  Have a nice day... ^_^                                      *
 *                                                                        *
 *************************************************************************/
/**************************************************************************
 *
 *  Revision History:
 *		Date:		Comment:
 *	-----------------------------------------------------------------------
 *		07-02-00	Initial Version (Andrew)
 *
 **************************************************************************/
/**************************************************************************
 *
 *  Notes:
 *	
 *
 **************************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <ddraw.h>
#include <commctrl.h> // For status window
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <math.h>
#include "WinMain.h"
#include "EmuMain.h"
#include "videodll.h"
#include "inputdll.h"
#include "audiodll.h"
#include "resource.h"

#define PATHSIZE 260
#define FILENAMESIZE 30

#define MAX_VIDEO 20
#define MAX_SOUND 20
#define MAX_INPUT 20

char dllpath[PATHSIZE];
char dllsearch[PATHSIZE];

int idxGFX = -1;
int idxSND = -1;
int idxINP = -1;

int maxGFX = 0;
int maxSND = 0;
int maxINP = 0;

char videoPlugins[MAX_VIDEO][FILENAMESIZE];
char soundPlugins[MAX_SOUND][FILENAMESIZE];
char inputPlugins[MAX_INPUT][FILENAMESIZE];

void CreateSearchMask () {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT];

	GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
	_splitpath(path_buffer,drive,dir,fname,ext);
	strcpy (dllpath, drive);
	strcat (dllpath, dir);
	strcat (dllpath, "Plugin\\"); // TR64, PJ64, 1964 compatible
	strcpy (dllsearch, dllpath);
	strcat (dllsearch, "*.dll");
	Debug (0, "Looking for plugins as: %s", dllsearch);
}


bool CheckFilename (char *filename) {
	WIN32_FIND_DATA finder;
	HANDLE findHandle = FindFirstFile(dllsearch, &finder);
	bool retVal = false;

	if (findHandle != INVALID_HANDLE_VALUE) {
		if (strcmp (filename, finder.cFileName) == 0)
			retVal = true;
		else
			while (FindNextFile(findHandle, &finder)) {
				if (strcmp (filename, finder.cFileName) == 0)
					retVal = true;
			}
		FindClose (findHandle);
	}
	return retVal;
}


void AddPluginToDialog (char *filename, HWND hWnd) {
	HMODULE hInstLib = NULL;
	PLUGIN_INFO Plugin;
	char buffer[260];

	strcpy (buffer, dllpath);
	strcat (buffer, filename);

	hInstLib = LoadLibrary (buffer);
	void (__cdecl* GetDllInfo)( PLUGIN_INFO * PluginInfo );

	if (hInstLib) {
		GetDllInfo = (void (__cdecl*)(PLUGIN_INFO *)) GetProcAddress(hInstLib, "GetDllInfo");
		if (GetDllInfo) {
			GetDllInfo (&Plugin);
			switch (Plugin.Type) {
				case PLUGIN_TYPE_RSP : // Type not supported...
					break;
				case PLUGIN_TYPE_GFX : // Graphics
					if (((Plugin.Version == PLUGIN_GFX_VERSION) || (Plugin.Version == 0x0102))&& (maxGFX < MAX_VIDEO)) {
						Debug (0, "%s is a Graphics Plugin...", filename);
						SendMessage(GetDlgItem(hWnd, IDC_VIDEO_LIST), CB_INSERTSTRING, maxGFX, (LPARAM)Plugin.Name);
						if (!strcmp(filename, RegSettings.vidDll)) {
							SendMessage (GetDlgItem (hWnd, IDC_VIDEO_LIST), CB_SETCURSEL, (WPARAM)maxGFX, (LPARAM)0);
							idxGFX = maxGFX;
						}
						strcpy (videoPlugins[maxGFX], filename);
						maxGFX++;
					} else {
						Debug (0, "%s was Rejected Graphics Version %i", filename, Plugin.Version);
					}
					break;
				case PLUGIN_TYPE_AUDIO : // Audio
					if ((Plugin.Version == PLUGIN_SND_VERSION) && (maxSND < MAX_SOUND)) {
						Debug (0, "%s is an Audio Plugin...", filename);
						SendMessage(GetDlgItem(hWnd, IDC_AUDIO_LIST), CB_INSERTSTRING, maxSND, (LPARAM)Plugin.Name);
						if (!strcmp(filename, RegSettings.sndDll)) {
							SendMessage (GetDlgItem (hWnd, IDC_AUDIO_LIST), CB_SETCURSEL, (WPARAM)maxSND, (LPARAM)0);
							idxSND = maxSND;
						}
						strcpy (soundPlugins[maxSND], filename);
						maxSND++;
					} else {
						Debug (0, "%s was Rejected Sound Version %i", filename, Plugin.Version);
					}
					break;
				case PLUGIN_TYPE_CONTROLLER : // Input
					if ((Plugin.Version == PLUGIN_INP_VERSION) && (maxINP < MAX_INPUT)) { // We will accept all inputs right now
						Debug (0, "%s is an Input Plugin...", filename);
						SendMessage(GetDlgItem(hWnd, IDC_CONTROLLER_LIST), CB_INSERTSTRING, maxINP, (LPARAM)Plugin.Name);
						if (!strcmp(filename, RegSettings.inpDll)) {
							SendMessage (GetDlgItem (hWnd, IDC_CONTROLLER_LIST), CB_SETCURSEL, (WPARAM)maxINP, (LPARAM)0);
							idxINP = maxINP;
						}
						strcpy (inputPlugins[maxINP], filename);
						maxINP++;
					} else {
						Debug (0, "%s was Rejected Input Version %i", filename, Plugin.Version);
					}
					break;
				default:
					Debug (0, "%s is Unknown type %i...", filename, Plugin.Type);
			}
		}
		FreeLibrary (hInstLib);
	}
}

void QueryPlugins (HWND hWnd) {
	char fuckyou[256];
	
	WIN32_FIND_DATA finder;

	SendMessage(GetDlgItem(hWnd,IDC_CONTROLLER_LIST), CB_RESETCONTENT, 0, 0);
	SendMessage(GetDlgItem(hWnd,IDC_AUDIO_LIST), CB_RESETCONTENT, 0, 0);
	SendMessage(GetDlgItem(hWnd,IDC_VIDEO_LIST), CB_RESETCONTENT, 0, 0);
	maxINP = maxSND = maxGFX = 0;
	
	HANDLE findHandle = FindFirstFile(dllsearch, &finder);
	if (findHandle != INVALID_HANDLE_VALUE) {
		strcpy (fuckyou, finder.cFileName);
		AddPluginToDialog (fuckyou, hWnd);
		while (FindNextFile(findHandle, &finder)) {
			strcpy (fuckyou, finder.cFileName);
			AddPluginToDialog (fuckyou, hWnd);
		}
		FindClose (findHandle);
	}

	if (idxGFX == -1) {
		EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_VID_CONFIG), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_VID_TEST), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_VID_ABOUT), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_VID_APPLY), FALSE);
	}
	if (idxSND == -1) {
		EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_AUD_CONFIG), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_AUD_TEST), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_AUD_ABOUT), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_AUD_APPLY), FALSE);
	}
	if (idxINP == -1) {
		EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_CON_CONFIG), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_CON_TEST), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_CON_ABOUT), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_CON_APPLY), FALSE);
	}
}	

BOOL CALLBACK PluginProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void LoadPlugins () {
	bool LaunchConfig = false;
	char buffer[260];
	
	CreateSearchMask ();

	if (CheckFilename (RegSettings.vidDll) == true) {
		strcpy (buffer, dllpath);
		strcat (buffer, RegSettings.vidDll); 
		gfxdll.Load (buffer);
		gfxdll.Init();
		Debug (0, "Graphics plugin %s INITIALIZED!", RegSettings.vidDll);
	} else {
		strcpy (RegSettings.vidDll, "");
		Debug (1, "Failed to load Graphics plugin...");
		LaunchConfig = true;
	}

	if (CheckFilename (RegSettings.sndDll) == true) {
		strcpy (buffer, dllpath);
		strcat (buffer, RegSettings.sndDll); 
		snddll.Load (buffer);
		snddll.Init ();
		Debug (0, "Audio DLL %s initialized!", RegSettings.sndDll);
	} else {
		strcpy (RegSettings.sndDll, "");
		Debug (1, "Failed to load Audio plugin...");
		LaunchConfig = true;
	}

	if (CheckFilename (RegSettings.inpDll) == true) {
		strcpy (buffer, dllpath);
		strcat (buffer, RegSettings.inpDll); 
		inpdll.Load (buffer);
		inpdll.InitiateControllers (GhWnd, ContInfo);
		Debug (0, "Input plugin %s INITIALIZED!", RegSettings.inpDll);
	} else {
		strcpy (RegSettings.inpDll, "");
		Debug (1, "Failed to load Input plugin...");
		LaunchConfig = true;
	}

	if (LaunchConfig)
		DialogBox(GhInst,MAKEINTRESOURCE(IDD_PLUGINS),GhWnd,(DLGPROC)PluginProc);
}

void UnLoadPlugins () {
	gfxdll.Close();
	snddll.Close();
	inpdll.Close();
}


BOOL CALLBACK PluginProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int MenuId;
	int index;
	char buffer[240];

	switch (message) {
		case WM_INITDIALOG: {
			idxGFX = -1;
			idxSND = -1;
			idxINP = -1;
			QueryPlugins (hWnd);
			break;
		}
		case WM_COMMAND: {
			MenuId = LOWORD(wParam);
			switch (MenuId) {
				case IDC_CONTROLLER_LIST: {
					int index = 0;
					if (HIWORD (wParam) == CBN_SELCHANGE) {
						index = SendMessage ((HWND)lParam, CB_GETCURSEL, 0, 0);
						if (index != idxINP) {
							EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_CON_CONFIG), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_CON_TEST), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_CON_ABOUT), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_CON_APPLY),  TRUE);
						} else {
							EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_CON_CONFIG), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_CON_TEST), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_CON_ABOUT), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_CON_APPLY),  FALSE);
						}
					}
				} break;
				case IDC_AUDIO_LIST: {
					int index = 0;
					if (HIWORD (wParam) == CBN_SELCHANGE) {
						index = SendMessage ((HWND)lParam, CB_GETCURSEL, 0, 0);
						if (index != idxSND) {
							EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_AUD_CONFIG), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_AUD_TEST), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_AUD_ABOUT), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_AUD_APPLY),  TRUE);
						} else {
							EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_AUD_CONFIG), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_AUD_TEST), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_AUD_ABOUT), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_AUD_APPLY),  FALSE);
						}
					}
				} break;
				case IDC_VIDEO_LIST: {
					int index = 0;
					if (HIWORD (wParam) == CBN_SELCHANGE) {
						index = SendMessage ((HWND)lParam, CB_GETCURSEL, 0, 0);
						if (index != idxGFX) {
							EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_VID_CONFIG), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_VID_TEST), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_VID_ABOUT), FALSE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_VID_APPLY),  TRUE);
						} else {
							EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_VID_CONFIG), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_VID_TEST), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_VID_ABOUT), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_VID_APPLY),  FALSE);
						}
					}
				} break;
				case IDCANCEL:
					EndDialog(hWnd,0);
				case IDOK:
				case ID_APPLY_ALL:
				case ID_PLUGIN_VID_APPLY:
					index = SendMessage (GetDlgItem(hWnd, IDC_VIDEO_LIST), CB_GETCURSEL, 0, 0);
					if ((index != CB_ERR) && (index != idxGFX)) {
						EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_VID_CONFIG), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_VID_TEST), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_VID_ABOUT), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_VID_APPLY), FALSE);
						idxGFX = index;
						gfxdll.Close ();
						strcpy (buffer, dllpath);
						strcat (buffer, videoPlugins[index]);
						gfxdll.Load (buffer);
						gfxdll.Init();
						strcpy (RegSettings.vidDll, videoPlugins[index]);
					}
					if (MenuId == ID_PLUGIN_VID_APPLY) break;
				case ID_PLUGIN_AUD_APPLY:
					index = SendMessage (GetDlgItem(hWnd, IDC_AUDIO_LIST), CB_GETCURSEL, 0, 0);
					if ((index != CB_ERR) && (index != idxSND)) {
						EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_AUD_CONFIG), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_AUD_TEST), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_AUD_ABOUT), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_AUD_APPLY), FALSE);
						idxSND = index;
						snddll.Close ();
						strcpy (buffer, dllpath);
						strcat (buffer, soundPlugins[index]);
						snddll.Load (buffer);
						snddll.Init();
						strcpy (RegSettings.sndDll, soundPlugins[index]);
					}
					if (MenuId == ID_PLUGIN_AUD_APPLY) break;
				case ID_PLUGIN_CON_APPLY:
					index = SendMessage (GetDlgItem(hWnd, IDC_CONTROLLER_LIST), CB_GETCURSEL, 0, 0);
					if ((index != CB_ERR) && (index != idxINP)) {
						EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_CON_CONFIG), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_CON_TEST), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_CON_ABOUT), TRUE); EnableWindow (GetDlgItem(hWnd, ID_PLUGIN_CON_APPLY), FALSE);
						idxINP = index;
						inpdll.Close ();
						strcpy (buffer, dllpath);
						strcat (buffer, inputPlugins[index]);
						inpdll.Load (buffer);
						inpdll.InitiateControllers (GhWnd, ContInfo);
						strcpy (RegSettings.inpDll, inputPlugins[index]);
					}
					if (MenuId == IDOK) {
						GhWndPlugin = NULL;
						EndDialog(hWnd, 0);
					}
				break;

				case ID_PLUGIN_VID_CONFIG:
					if (gfxdll.DllConfig)
						gfxdll.DllConfig (GhWnd);
				break;
				case ID_PLUGIN_VID_TEST:
					if (gfxdll.DllTest)
						gfxdll.DllTest (GhWnd);
				break;
				case ID_PLUGIN_VID_ABOUT:
					if (gfxdll.DllAbout)
						gfxdll.DllAbout (GhWnd);
				break;

				case ID_PLUGIN_AUD_CONFIG:
					if (snddll.DllConfig)
						snddll.DllConfig (GhWnd);
				break;
				case ID_PLUGIN_AUD_TEST:
					if (snddll.DllTest)
						snddll.DllTest (GhWnd);
				break;
				case ID_PLUGIN_AUD_ABOUT:
					if (snddll.DllAbout)
						snddll.DllAbout (GhWnd);
				break;

				case ID_PLUGIN_CON_CONFIG:
					if (inpdll.DllConfig)
						inpdll.DllConfig (GhWnd);
				break;
				case ID_PLUGIN_CON_TEST:
					if (inpdll.DllTest)
						inpdll.DllTest (GhWnd);
				break;
				case ID_PLUGIN_CON_ABOUT:
					if (inpdll.DllAbout)
						inpdll.DllAbout (GhWnd);
				break;
			}
		}
	}
	return FALSE;
}
