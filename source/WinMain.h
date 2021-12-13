#ifndef WINMAIN_DOT_H
#define WINMAIN_DOT_H

#include <windows.h>

#define WINTITLE "Apollo v0.10 Private Beta R1.5"
#define LASTCOMPILED __DATE__

#define CHECK_MENU(id)		CheckMenuItem(GhMenu, id, MF_CHECKED)
#define UNCHECK_MENU(id)	CheckMenuItem(GhMenu, id, MF_UNCHECKED)
#define GET_MENU(id)		GetMenuState(GhMenu,  id, MF_BYCOMMAND)

// Type definitions

#include "common.h"

// Common Structures
typedef struct n64hdr {
  u16  valid; // 00
  u8  is_compressed; // 02
  u8  unknown; //03
  u32 Clockrate; // 04
  u32 Program_Counter; // 08
  u32 Release; // 0C
  u32 CRC1; // 10
  u32 CRC2; // 14
  u64 filler1; // 18
  u8  Image_Name[20]; // 20-33
  u8  uk1[7]; // 34-3a
  //u8  filler2[7];
  u8  Manu_ID; // 3b
  u16  Cart_ID; // 3c
  u8  Country_Code; // 3e
  u8  filler3; //3f
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
	char lastRom[8][100];
	bool enableConsoleWindow;
	bool enableConsoleLogging;
	char vidDll[128];
	char sndDll[128];
	char inpDll[128];
	bool RumblePack;
	int  recentSize;
	bool compressSaveStates;
	bool FullScreen;
	int VSyncHack;
	bool force4keep;
	bool dynamicEnabled;
	bool isPakInstalled;
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
#ifndef _FUCK
extern "C" {
#endif
	void Debug (int logtype, char *string, ...); // also defined in console.cpp
#ifndef _FUCK
}
#endif
//void Debug (int logtype, char *string, ...); // also defined in console.cpp

// From Settings.cpp
void SaveSettings( void ); // Save Registry Settings
void LoadSettings( void ); // Load Registry Settings
int LoadCommandLine(char* commandLine);

void RecentMenus( char *addition );

#endif // WINMAIN_DOT_H