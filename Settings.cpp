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
  Settings.cpp - Registry Settings and INI/Config stuff

  History:

	Name		Date		Comment
  -------------------------------------------------------------
	Azimer		05-27-00	Initial Version	
	Phrodide	06-29-00	Added Comand Line Parsing
	Azimer		01-01-01	Rewrote some of the file
 
 **************************************************************/

#include "resource.h"
#include "WinMain.h"
#include <d3d.h>

#define RegVersion 0x0002 // Version 1 (Version 0 was Eclipse)

unsigned char DefaultControllerConfig[21] = { 0x1C, 0x36, 0x1e, 0x2c, 0xc8, 0xd0, 0xcb, 0xcd, 0x10, 0x11, 0x1f, 0x20, 0x2d, 0x12, 0x33, 0x34, 0xc8, 0xd0, 0xcb, 0xcd, 0x00 };

void LoadSettings( void ) {
	HKEY settings;
	unsigned long size = 0;
	unsigned long result;
	unsigned long resultsize = 4;
	memset(&RegSettings,0,sizeof(struct rSettings));
	RegSettings.enableConsoleWindow = false;
	RegSettings.enableConsoleLogging = false;
	strcpy (RegSettings.vidDll, ".");
	// Load up the defaults
	//RegSettings.rdpMode = 0;// TODO: In eclipse it was D3D_HAL... will this still apply to us?
	//RegSettings.Disabled[1] = RegSettings.Disabled[2] = RegSettings.Disabled[3] = 1;
	//memcpy(&RegSettings.ControllerConfig[0], &DefaultControllerConfig, 21);
	RegCreateKeyEx(HKEY_LOCAL_MACHINE,"Software\\Apollo\\Data",0,0,REG_OPTION_NON_VOLATILE,
					KEY_ALL_ACCESS,NULL,&settings,&result);
	RegQueryValueEx(settings,"regSize",NULL,NULL,(unsigned char*)&size,&resultsize);
	if (size==0) {
		//First time running on this computer...display discalimer.
		if(MessageBox(0,L_STR(IDS_DISCLAIMER),WINTITLE,MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2)==IDNO) {
			PostQuitMessage(0);
			return;
		}
		size = sizeof(struct rSettings);
		resultsize = 4;
		RegSetValueEx(settings,"regSize",NULL,REG_DWORD,(unsigned char*)&size,resultsize);
		RegSetValueEx(settings,"registry",NULL,REG_BINARY,(unsigned char*)&RegSettings,size);
	}else if (size != sizeof(struct rSettings)) {
		//Old style data...load just the most we can...or mebbe the defaults?
		RegQueryValueEx(settings,"registry",NULL,NULL,(unsigned char*)&RegSettings,&size);
	} else {
		RegQueryValueEx(settings,"registry",NULL,NULL,(unsigned char*)&RegSettings,&size);
	}
	RegCloseKey(settings);
	if (RegVersion != RegSettings.Version)
		if (MessageBox (GhWnd, L_STR(IDS_SETTING_OLD), "Registry Settings are old/been modified", MB_YESNO | MB_ICONWARNING) == IDYES)
			return;
	strcpy (RegSettings.inpDll, "input.dll");
/*
	for (int numcont = 0; numcont < 4; numcont++) {
		for (i=0; i < 21; i++) {
			N64KeyToPcKey[numcont][i] = RegSettings.ControllerConfig[numcont][i];
			N64KeyToJoy[numcont][i] = RegSettings.JoyConfig[numcont][i];
		}
		MempackEnabled[numcont] = RegSettings.MemoryPack[numcont];
		RumblepackEnabled[numcont] = RegSettings.RumblePack[numcont];
		UseJoyForAnalog[numcont] = RegSettings.UseJoyForAnalog[numcont];
		JoyIsDisabled[numcont] = RegSettings.Disabled[numcont];
		CurrentJoy[numcont] = RegSettings.CurrentJoy[numcont];
	}
*/
}

void SaveSettings( void ) {
	int resultsize = 4;
	int size = sizeof(struct rSettings);
	HKEY settings;
	unsigned long result;
/*
	for (int numcont = 0; numcont < 4; numcont++) {
		for (i=0; i < 21; i++) {
			RegSettings.ControllerConfig[numcont][i]=N64KeyToPcKey[numcont][i];
			RegSettings.JoyConfig[numcont][i]=N64KeyToJoy[numcont][i];
		}
		RegSettings.MemoryPack[numcont] = MempackEnabled[numcont];
		RegSettings.RumblePack[numcont] = RumblepackEnabled[numcont];
		RegSettings.UseJoyForAnalog[numcont] = UseJoyForAnalog[numcont];
		RegSettings.Disabled[numcont] = JoyIsDisabled[numcont];
		RegSettings.CurrentJoy[numcont] = CurrentJoy[numcont];
	}
*/
	RegSettings.Version = RegVersion;
	RegCreateKeyEx(HKEY_LOCAL_MACHINE,"Software\\Apollo\\Data",0,0,REG_OPTION_NON_VOLATILE,
					KEY_ALL_ACCESS,NULL,&settings,&result);
	RegSetValueEx(settings,"regSize",NULL,REG_DWORD,(unsigned char*)&size,resultsize);
	RegSetValueEx(settings,"registry",NULL,REG_BINARY,(unsigned char*)&RegSettings,size);
	RegCloseKey(settings);
}
/* Disabled Language Support...
int LoadCommandLine(char* commandLine) {
	extern int LangUsed;
	int i=0;
	while(commandLine[i]!=0) {
		if (commandLine[i]=='-'){
			if (!strncmp("-english",commandLine+i,sizeof("-english"))) {
				LangUsed = MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US);
			} else if (!strncmp("-swedish",commandLine+i,sizeof("-swedish"))) {
				LangUsed = MAKELANGID(LANG_SWEDISH,SUBLANG_DEFAULT);
			} else if (!strncmp("-spanish",commandLine+i,sizeof("-spanish"))) {
				LangUsed = MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_MODERN);
			} else if (!strncmp("-portuguese",commandLine+i,sizeof("-portuguese"))) {
				LangUsed = MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE);
			} else if (!strncmp("-port",commandLine+i,sizeof("-port"))) {
				LangUsed = MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE);
			} else if (!strncmp("-danish",commandLine+i,sizeof("-danish"))) {
				LangUsed = MAKELANGID(LANG_DANISH, SUBLANG_DEFAULT);
			} else if (!strncmp("-french",commandLine+i,sizeof("-french"))) {
				LangUsed = MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH);
			} else if (!strncmp("-dutch",commandLine+i,sizeof("-dutch"))) {
				LangUsed = MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH);
			}
		}
		i++;
	}
	return 0;
}*/