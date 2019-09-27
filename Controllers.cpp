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
#include "winmain.h"
#include "emumain.h"
#include "plugin.h"
#include "inputdll.h"

// ****************** Interfacing for the Controller plugin ****************************

INPUTDLL inpdll;
CONTROL ContInfo[4];
BUTTONS keys;

static bool LoadFunctions () {
	inpdll.CloseDLL			 = (void (__cdecl*)( void		   )) GetProcAddress(inpdll.hinstLibInput, "CloseDLL");
	inpdll.ControllerCommand = (void (__cdecl*)( int, BYTE *   )) GetProcAddress(inpdll.hinstLibInput, "ControllerCommand");
	inpdll.DllAbout			 = (void (__cdecl*)( HWND          )) GetProcAddress(inpdll.hinstLibInput, "DllAbout");
	inpdll.DllConfig		 = (void (__cdecl*)( HWND          )) GetProcAddress(inpdll.hinstLibInput, "DllConfig");
	inpdll.DllTest			 = (void (__cdecl*)( HWND          )) GetProcAddress(inpdll.hinstLibInput, "DllTest");
	inpdll.GetDllInfo		 = (void (__cdecl*)( PLUGIN_INFO * )) GetProcAddress(inpdll.hinstLibInput, "GetDllInfo");
	inpdll.GetKeys			 = (void (__cdecl*)( int, BUTTONS *)) GetProcAddress(inpdll.hinstLibInput, "GetKeys");
	inpdll.InitiateControllers = (void (__cdecl*)( HWND, CONTROL[])) GetProcAddress(inpdll.hinstLibInput, "InitiateControllers");
	inpdll.ReadController	 = (void (__cdecl*)( int, BYTE *   )) GetProcAddress(inpdll.hinstLibInput, "ReadController");
	inpdll.RomClosed		 = (void (__cdecl*)( void		   )) GetProcAddress(inpdll.hinstLibInput, "RomClosed");
	inpdll.RomOpen			 = (void (__cdecl*)( void		   )) GetProcAddress(inpdll.hinstLibInput, "RomOpen");
	inpdll.WM_KeyDown		 = (void (__cdecl*)( WPARAM, LPARAM)) GetProcAddress(inpdll.hinstLibInput, "WM_KeyDown");
	inpdll.WM_KeyUp			 = (void (__cdecl*)( WPARAM, LPARAM)) GetProcAddress(inpdll.hinstLibInput, "WM_KeyUp");
	return true;
}

void CloseInputPlugin () {
  if(inpdll.CloseDLL)
	  inpdll.CloseDLL();

  if(inpdll.hinstLibInput)
	  FreeLibrary(inpdll.hinstLibInput); 

  ZeroMemory (&inpdll, sizeof(INPUTDLL));
}

BOOL LoadInputPlugin (char *libname) {
	PLUGIN_INFO Plugin_Info;
	ZeroMemory (&Plugin_Info, sizeof(Plugin_Info));

	if (libname == NULL) {
		return FALSE;
	}
	
	if (inpdll.hinstLibInput != NULL) 
		inpdll.Close ();

	inpdll.hinstLibInput = LoadLibrary(libname);

	if (inpdll.hinstLibInput == NULL) {
		Debug (0, "Could not load %s because it couldn't be found!", libname);
		inpdll.Close();
		return FALSE;
	}

	inpdll.GetDllInfo = (void (__cdecl*)(PLUGIN_INFO *)) GetProcAddress(inpdll.hinstLibInput, "GetDllInfo");
	if(!inpdll.GetDllInfo) {
		Debug (0, "%s does not conform to the plugin spec.", libname);
		return FALSE;//This isn't a plugin dll, so don't bother.	
	}
	inpdll.GetDllInfo(&Plugin_Info);

	if(Plugin_Info.Type == PLUGIN_TYPE_CONTROLLER) {
		if(Plugin_Info.Version != PLUGIN_INP_VERSION) {
			Debug (0, "%s is not compatable with Apollo. %X != %X", libname, Plugin_Info.Version, PLUGIN_INP_VERSION);
			inpdll.Close();
			return FALSE;
		}

		if (LoadFunctions () == false) {
			inpdll.Close();
			return FALSE;
		}
	} else {
		//Here we insert Audio code, controller code, etc.
		Debug (0, "%s dll isn't an Input plugin!", libname);
		inpdll.Close();
		return FALSE;
	}
	
	//strcpy(RegSettings.vidDll,libname);
	return TRUE;
}


//#define _PIF_DEBUGGING_ENABLED_
#ifdef _PIF_DEBUGGING_ENABLED_
	#pragma message("WARNING: PIF DEBUGGING IS ENABLED!!!");
#endif

#pragma warning( disable : 4035 )

inline DWORD SWAP_DWORD(DWORD y) {
	__asm { 
		mov eax,y 
		bswap eax 
	}
} 

#pragma warning( default : 4035 )

u8 bspif[64];
int volatile cmdcntr=0;
int volatile pifsend=0;
int volatile pifrecv=0;
int volatile cmd=0;
int volatile port=0;

#ifdef _PIF_DEBUGGING_ENABLED_
FILE *sifp = fopen ("d:\\pif.txt", "wt");
#endif

void DoPifEmulation (unsigned char *dest, unsigned char *src) {
	int QuickEscape = 0; // Allows me to leave the pif emulation quickly for unsupported stuff :)
	cmdcntr=0;
	pifsend=0;
	pifrecv=0;
	cmd=0;
	port=0;
	int cnt = 0;

	__asm { // Byte Swap the damn thing... make it easy on me...
		push edi;
		mov edi, dword ptr [src];
		mov ecx,0;
pifstart:
		mov eax, [edi];
		add edi, 4;
		bswap eax;
		mov dword ptr [bspif+ecx], eax;
		add ecx, 4;
		cmp ecx, 64;
		jne pifstart;
		pop edi;
	}

#ifdef _PIF_DEBUGGING_ENABLED_
	fprintf (sifp, "Start pif\n");
	for (cnt=0; cnt < 64; cnt+=8) {
		fprintf (sifp, "%02X %02X %02X %02X - %02X %02X %02X %02X\n", 
			bspif[cnt+0], bspif[cnt+1], bspif [cnt+2], bspif[cnt+3],
			bspif[cnt+4], bspif[cnt+5], bspif [cnt+6], bspif[cnt+7]);
	}
	fprintf (sifp, "\n");
#endif

	extern HANDLE AdaptoidHandle;
	if (AdaptoidHandle) {
		DWORD nbytes=0;
		WriteFile(AdaptoidHandle,bspif,64,&nbytes,NULL);
		ReadFile(AdaptoidHandle,bspif,64,&nbytes,NULL);
	} else {
		do {
			while (bspif[cmdcntr] == 0xFF)
				cmdcntr++;
			if (bspif[cmdcntr] == 0xFE)
				break;
			pifsend = bspif[cmdcntr++];
			if (pifsend != 0x00) {
				pifrecv = bspif[cmdcntr];
				cmd  = bspif[cmdcntr+1]; // At least one value is being sent :)
				switch (cmd) {
// *************************************************************************
// Port status (controllers basically)
				case 0x00:
					//if (port == 0) {
					if (ContInfo[port].Present == TRUE) {
						if (pifrecv > 3) {
							bspif[cmdcntr] |= 0x40;
							cmdcntr+= 5; // skip recv/cmd/3 data bytes (TODO: Not perfect emulation-wise)
							break;
						}
						cmdcntr+=2; // skip recv and cmd
						bspif[cmdcntr++] = 0x05;
						bspif[cmdcntr++] = 0x00;
						bspif[cmdcntr++] = 0x02; // TODO: When you get packs done modify this!
					} else {
						bspif[cmdcntr] |= 0x80;
						cmdcntr += 5; // skip recv/cmd/3 data bytes
					}
					break;
// *************************************************************************
// Read Controller
				case 0x01:
					//if (port == 0) {
					if (ContInfo[port].Present == TRUE) {
						inpdll.GetKeys(port, &keys);
						cmdcntr+=2;
						//bspif[cmdcntr++] = 0;
						//bspif[cmdcntr++] = 0;
						//bspif[cmdcntr++] = 0;
						//bspif[cmdcntr++] = 0;
						memcpy (&bspif[cmdcntr], &keys, 4);
						cmdcntr+=4;
					}
					else {
						bspif[cmdcntr] |= 0x80;
						cmdcntr += 6; // skip recv/cmd/4 data bytes
					}					
					break;
// *************************************************************************
// Read Mempack
				case 0x02:
					QuickEscape = 1;
					break;
// *************************************************************************
// Write Mempack
				case 0x03:
					QuickEscape = 1;
					break;
// *************************************************************************
// Read EEPROM
				case 0x04:
					QuickEscape = 1;
					break;
// *************************************************************************
// Write EEPROM
				case 0x05:
					QuickEscape = 1;
					break;
// *************************************************************************
				default:
					;//Debug (0, L_STR(IDS_PIF_UK), cmd);
				}
			}
			port++;
		} while (QuickEscape==0);
	}

#ifdef _PIF_DEBUGGING_ENABLED_
	fprintf (sifp, "End pif\n");
	for (cnt=0; cnt < 64; cnt+=8) {
		fprintf (sifp, "%02X %02X %02X %02X - %02X %02X %02X %02X\n", 
			bspif[cnt+0], bspif[cnt+1], bspif [cnt+2], bspif[cnt+3],
			bspif[cnt+4], bspif[cnt+5], bspif [cnt+6], bspif[cnt+7]);
	}
	fprintf (sifp, "\n");
#endif

	__asm { // Save our pif information
		push edi;
		mov edi, dword ptr [dest];
		mov ecx,0;
pifstart2:
		mov eax, dword ptr [bspif+ecx];
		bswap eax;
		mov [edi], eax;
		add ecx, 4;
		add edi, 4;
		cmp ecx, 64;
		jne pifstart2;
		pop edi;
	}
}

