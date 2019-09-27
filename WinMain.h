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

#ifndef WINMAIN_DOT_H
#define WINMAIN_DOT_H

#include <windows.h>

#define WINTITLE "Apollo v0.01d Public Beta"
#define LASTCOMPILED __DATE__

#define CHECK_MENU(id)		CheckMenuItem(GhMenu, id, MF_CHECKED)
#define UNCHECK_MENU(id)	CheckMenuItem(GhMenu, id, MF_UNCHECKED)
#define GET_MENU(id)		GetMenuState(GhMenu,  id, MF_BYCOMMAND)

// Type definitions

#define	u8	unsigned char
#define s8	signed char
#define u16	unsigned short
#define s16	signed short
#define u32	unsigned long
#define s32	signed long
#define u64	unsigned __int64
#define s64	signed __int64

#define BYTE   unsigned char
#define SBYTE  signed char
#define WORD   unsigned short
#define SWORD  signed short
#define DWORD  unsigned long
#define SDWORD signed long
#define QWORD  unsigned __int64
#define SQWORD signed __int64

// Common Structures
typedef struct n64hdr {
  u16  valid;
  u8  is_compressed;
  u8  unknown;
  u32 Clockrate;
  u32 Program_Counter;
  u32 Release;
  u32 CRC1;
  u32 CRC2;
  u64 filler1;
  u8  Image_Name[20];
  u8  uk1[8];
  u8  filler2[7];
  u8  Manu_ID;
  u16  Cart_ID;
  u8  Country_Code;
  u8  filler3;
  u32 BOOTCODE;
} Header;

struct RomInfoStruct {// Holds the information displayed when RomInfo is called
	char *ExternalName;
	char *InternalName;
	int  BootCode;
	int  CRC1;
	int  CRC2;
	int  ExCRC1;
	int  ExCRC2;
};

//Registry save settings...
struct rSettings {
	u32 Version; // Version of the settings (more then enough space)
	int recentIndex;
	char lastDir[127];
	char lastRom[8][63];
	bool enableConsoleWindow;
	bool enableConsoleLogging;
	char vidDll[128];
	char sndDll[128];
	char inpDll[128];
	//int rdpMode;
	//char ControllerConfig[4][21];//one more than needed
	//char JoyConfig[4][21]; // Gamedevice configs
	//int  GameDevices[4];   // Which gamedevices belong to what player
	//int  CurrentJoy[4];
	//bool UseJoyForAnalog[4];
	//bool RumblePack[4];
	//bool MemoryPack[4];
	//bool Disabled[4];
};


// Global Variables
// TODO: Seperate all common variables from GUI common variables
extern HWND GhWnd;	      // hwnd to the main window (G Means Global)
extern HWND GhWndPlugin;
extern HINSTANCE GhInst; // Application Instance
extern HMENU GhMenu; // Main Menu
extern RECT rcWindow; // Rectangle of the Main Window
extern HWND hwndStatus;
extern bool isFullScreen; // Window is currently in full screen

extern Header RomHeader;
extern struct rSettings RegSettings;
extern struct RomInfoStruct rInfo;

// Global Functions
// TODO: Seperate all common functions from GUI common functions
void SetClientRect(HWND Thwnd, RECT goal);
extern char L_STR_buffer[512];
char* L_STR(int); //returns a string resource for use.

// From console.cpp
void dprintf (char* string, ...); // defined in console.cpp
void Debug (int logtype, char *string, ...); // also defined in console.cpp

// From Settings.cpp
void SaveSettings( void ); // Save Registry Settings
void LoadSettings( void ); // Load Registry Settings
int LoadCommandLine(char* commandLine);

#endif // WINMAIN_DOT_H