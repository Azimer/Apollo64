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
#include "resource.h"

VIDEODLL dlgPlugin;
extern VIDEODLL internalGfxDll;
extern bool cpuContextIsValid;
char dlgPluginName[260];
char Directory[260];

void GetPluginDir(void) {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT];
	GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
	_splitpath(path_buffer,drive,dir,fname,ext);
	strcpy(Directory,drive);
	strcat(Directory,dir);
	strcat(Directory,"*.dll");
}

void QueryVideoPlugins(HWND hWnd, int combobox) {
	u32 index = 0;
	WIN32_FIND_DATA finder;
	VIDEODLL query;
	PLUGIN_INFO Plugin_Info;
	PLUGIN_INFO Plugin_Info_Current;
	GetPluginDir();
	gfxdll.GetDllInfo(&Plugin_Info_Current);
	internalGfxDll.GetDllInfo(&Plugin_Info);
	SendMessage(GetDlgItem(hWnd,combobox),CB_RESETCONTENT, 0,0);
	index = SendMessage(GetDlgItem(hWnd,combobox),CB_ADDSTRING, 0,(LPARAM)Plugin_Info.Name);
	if (!strcmp(Plugin_Info.Name,Plugin_Info_Current.Name)) {
		SendMessage(GetDlgItem(hWnd,combobox),CB_SETCURSEL, (WPARAM)index, (LPARAM)0);
	}
	HANDLE findHandle = FindFirstFile(Directory,&finder);
	if (findHandle) {
		do {
			if (strcmp(finder.cFileName,RegSettings.vidDll))
				query.hinstLibVideo = LoadLibrary(finder.cFileName);
			else query.hinstLibVideo = gfxdll.hinstLibVideo;
			if (query.hinstLibVideo) {
				query.GetDllInfo = (void (__cdecl*)(PLUGIN_INFO *)) GetProcAddress(query.hinstLibVideo, "GetDllInfo");
				if (query.GetDllInfo) {
					query.GetDllInfo(&Plugin_Info);
					if (Plugin_Info.Version==0x0102 && Plugin_Info.Type==PLUGIN_TYPE_GFX) {
						//Debug(0,"%s found as a Plugin",Plugin_Info.Name);
						index = SendMessage(GetDlgItem(hWnd,combobox),CB_ADDSTRING, 0,(LPARAM)Plugin_Info.Name);
						if (!strcmp(Plugin_Info.Name,Plugin_Info_Current.Name)) {
							SendMessage(GetDlgItem(hWnd,combobox),CB_SETCURSEL, (WPARAM)index, (LPARAM)0);
						}
					} else {
						char temp[10];
						sprintf (temp, "%x", Plugin_Info.Version); 
						Debug (0, "%s Not Gfx Plugin!",temp); 
					}
				}
				if (strcmp(finder.cFileName,RegSettings.vidDll))
					FreeLibrary(query.hinstLibVideo); 
			}
		} while (FindNextFile(findHandle,&finder)!=0);
		FindClose(findHandle);
	}
}
/*//*
void QueryPlugins (HWND hWnd) {
}


BOOL CALLBACK PluginProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int MenuId;

	switch (message) {
		case WM_INITDIALOG: {
			QueryPlugins (hWnd);
			break;
		}
		case WM_COMMAND: {
			MenuId = LOWORD(wParam);
			switch (MenuId) {
				case IDOK:
				case IDCANCEL:
				case ID_APPLY_ALL:
				case ID_PLUGIN_VID_CONFIG:
				case ID_PLUGIN_VID_TEST:
				case ID_PLUGIN_VID_ABOUT:
				case ID_PLUGIN_VID_APPLY:
				case ID_PLUGIN_AUD_CONFIG:
				case ID_PLUGIN_AUD_TEST:
				case ID_PLUGIN_AUD_ABOUT:
				case ID_PLUGIN_AUD_APPLY:
				case ID_PLUGIN_CON_CONFIG:
				case ID_PLUGIN_CON_TEST:
				case ID_PLUGIN_CON_ABOUT:
				case ID_PLUGIN_CON_APPLY:
					break;
			}
		}
	}
	return FALSE;
}*/

BOOL CALLBACK PluginProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int MenuId;
	WIN32_FIND_DATA finder;
	VIDEODLL query;
	PLUGIN_INFO Plugin_Info;

	switch(message)
	{
		case WM_INITDIALOG: {
			memcpy(&dlgPlugin,&gfxdll,sizeof(VIDEODLL));
			//put plugin find functions here.
			QueryVideoPlugins(hWnd, IDC_VIDEO_LIST);
			break;
		}
		case WM_COMMAND:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				SendMessage(GetDlgItem(hWnd,ID_PLUGIN_VID_APPLY),BM_SETSTYLE, (WPARAM)BS_PUSHBUTTON, (LPARAM)0);
				SendMessage(GetDlgItem(hWnd,ID_APPLY_ALL),BM_SETSTYLE, (WPARAM)BS_PUSHBUTTON, (LPARAM)0);
				HWND curSel = (HWND)lParam;
				u32 index = SendMessage(curSel,CB_GETCURSEL,0,0);
				if (!index) {
					//using Internal Functions
					strcpy(dlgPluginName,".");
					memcpy(&dlgPlugin,&internalGfxDll,sizeof(VIDEODLL));
				} else {
					HANDLE findHandle = FindFirstFile(Directory,&finder);
					if (findHandle) {
						do {
							if (strcmp(finder.cFileName,RegSettings.vidDll))
								query.hinstLibVideo = LoadLibrary(finder.cFileName);
							else query.hinstLibVideo = gfxdll.hinstLibVideo;
							if (query.hinstLibVideo) {
								query.GetDllInfo = (void (__cdecl*)(PLUGIN_INFO *)) GetProcAddress(query.hinstLibVideo, "GetDllInfo");
								if (query.GetDllInfo) {
									query.GetDllInfo(&Plugin_Info);
									if (Plugin_Info.Version==0x0102 && Plugin_Info.Type==PLUGIN_TYPE_GFX) {
										if (!(--index)) {
											query.DllAbout  = (void (__cdecl*)(HWND)) GetProcAddress(query.hinstLibVideo, "DllAbout");
											query.DllTest   = (void (__cdecl*)(HWND)) GetProcAddress(query.hinstLibVideo, "DllTest");
											query.DllConfig = (void (__cdecl*)(HWND)) GetProcAddress(query.hinstLibVideo, "DllConfig");
											strcpy(dlgPluginName,finder.cFileName);
											memcpy(&dlgPlugin,&query,sizeof(VIDEODLL));
										}
									}
								}
//								if (strcmp(finder.cFileName,RegSettings.vidDll))
//									FreeLibrary(query.hinstLibVideo); 
							}
						} while (FindNextFile(findHandle,&finder)!=0);
						FindClose(findHandle);
					}
				}
				break;
			}
			MenuId = LOWORD(wParam);
			if (MenuId == IDOK) {
				if (memcmp(&dlgPlugin,&gfxdll,sizeof(VIDEODLL))) {
					if (!cpuIsPaused && cpuContextIsValid) {
						//pause
						ToggleCPU();
						//load plugin
						gfxdll.Load(dlgPluginName);
						gfxdll.Init();
						gfxdll.RomOpen();
						//unpause
						ToggleCPU();
					} else {
						//load plugin
						gfxdll.Load(dlgPluginName);
						gfxdll.Init();
					}
				}
				//save settings
				//strcpy (RegSettings.vidDll, dlgPluginName);
				HANDLE findHandle = FindFirstFile(Directory,&finder);
				if (findHandle) {
					do {
						if (strcmp(finder.cFileName,RegSettings.vidDll))
							query.hinstLibVideo = LoadLibrary(finder.cFileName);
						else query.hinstLibVideo = gfxdll.hinstLibVideo;
						if (query.hinstLibVideo) {
							if (strcmp(finder.cFileName,RegSettings.vidDll))
								FreeLibrary(query.hinstLibVideo); 
						}
					} while (FindNextFile(findHandle,&finder)!=0);
					FindClose(findHandle);
				}
				EndDialog(hWnd,0);
				GhWndPlugin = NULL;
				return TRUE;
			} else if (MenuId == IDCANCEL) {
				HANDLE findHandle = FindFirstFile(Directory,&finder);
				if (findHandle) {
					do {
						if (strcmp(finder.cFileName,RegSettings.vidDll))
							query.hinstLibVideo = LoadLibrary(finder.cFileName);
						else query.hinstLibVideo = gfxdll.hinstLibVideo;
						if (query.hinstLibVideo) {
							if (strcmp(finder.cFileName,RegSettings.vidDll))
								FreeLibrary(query.hinstLibVideo); 
						}
					} while (FindNextFile(findHandle,&finder)!=0);
					FindClose(findHandle);
				}
				EndDialog(hWnd,0);
				GhWndPlugin = NULL;
				return TRUE;
			} else if (MenuId == ID_APPLY_ALL) {
				if (memcmp(&dlgPlugin,&gfxdll,sizeof(VIDEODLL))) {
					if (!cpuIsPaused && cpuContextIsValid) {
						//pause
						ToggleCPU();
						//load plugin
						gfxdll.Load(dlgPluginName);
						gfxdll.Init();
						gfxdll.RomOpen();
						//unpause
						ToggleCPU();
					} else {
						//load plugin
						gfxdll.Load(dlgPluginName);
						gfxdll.Init();
					}
				}
				//save settings, grey out other 3 apply buttons.
			} else if (MenuId == ID_PLUGIN_VID_CONFIG) {
				if (dlgPlugin.DllConfig) dlgPlugin.DllConfig(hWnd);
			} else if (MenuId == ID_PLUGIN_VID_TEST) {
				if (dlgPlugin.DllTest) dlgPlugin.DllTest(hWnd);
			} else if (MenuId == ID_PLUGIN_VID_ABOUT) {
				if (dlgPlugin.DllAbout) dlgPlugin.DllAbout(hWnd);
			} else if (MenuId == ID_PLUGIN_VID_APPLY) {
				if (memcmp(&dlgPlugin,&gfxdll,sizeof(VIDEODLL))) {
					if (!cpuIsPaused && cpuContextIsValid) {
						//pause
						ToggleCPU();
						//load plugin
						gfxdll.Load(dlgPluginName);
						gfxdll.Init();
						gfxdll.RomOpen();
						//unpause
						ToggleCPU();
					} else {
						//load plugin
						gfxdll.Load(dlgPluginName);
						gfxdll.Init();
					}
				}
				//save settings, grey button
			} else if (MenuId == ID_PLUGIN_AUD_CONFIG) {
			} else if (MenuId == ID_PLUGIN_AUD_TEST) {
			} else if (MenuId == ID_PLUGIN_AUD_ABOUT) {
			} else if (MenuId == ID_PLUGIN_AUD_APPLY) {
				//save settings, grey button
			} else if (MenuId == ID_PLUGIN_CON_CONFIG) {
			} else if (MenuId == ID_PLUGIN_CON_TEST) {
			} else if (MenuId == ID_PLUGIN_CON_ABOUT) {
			} else if (MenuId == ID_PLUGIN_CON_APPLY) {
				//save settings, grey button
			}
	}
	return FALSE;
}
