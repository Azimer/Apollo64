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
#include <commctrl.h> // For status window
#include <stdio.h>
#include "resource.h"
#include "WinMain.h"
#include "EmuMain.h"
#include "videodll.h"
#include "inputdll.h"
bool volatile cpuIsDone = false;
bool volatile cpuIsReset = false;

bool cpuContextIsValid=false;
bool cpuIsPaused=false;

struct n64hdr RomHeader;
static char *zero = "";
HANDLE AdaptoidHandle = NULL;

HANDLE volatile handleCpuThread = NULL;
HANDLE volatile handleAudioThread = NULL;
DWORD dwThreadId, dwThrdParam;
DWORD dwAThreadId, dwAThrdParam;
UINT AudioThreadProc(LPVOID);

void OpenROM (char *filename);
void StartCPU (void);
void StopCPU (void);
void ToggleCPU (void);
UINT CpuThreadProc(LPVOID);

// Code remaining from eclipse
void OnOpen(char* filename);
// end code remaining from eclipse

LPEXCEPTION_POINTERS intelException;
u64 nullMemory = 0;

u32 HandleWin32Exception(LPEXCEPTION_POINTERS info) {
	intelException = info;
	extern u32 valloc;
	if (info[0].ExceptionRecord[0].ExceptionCode==EXCEPTION_ACCESS_VIOLATION) {
		Debug(0,L_STR(IDS_VIOLATION_ACCESS),(info[0].ExceptionRecord[0].ExceptionInformation[1])-valloc);
		Debug (0, "pc = %08X", pc-4);
		cpuIsDone = true;
		cpuIsReset = true;
		cpuContextIsValid = false;
		return EXCEPTION_EXECUTE_HANDLER;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

// Everything below this is interfacing
void InitEmu () {
	rInfo.ExternalName = zero;
	rInfo.InternalName = zero;
	rInfo.BootCode = 0;
	rInfo.CRC1 = 0;
	rInfo.CRC2 = 0;
	rInfo.ExCRC1 = 0;
	rInfo.ExCRC2 = 0;
	InitMem ();

	AdaptoidHandle = CreateFile( "\\\\.\\Wish_NA1",  // the name of the Adaptoid driver
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, NULL);

	int nAdaptoids = 0;
	u32 nbytes = 0;

	if (AdaptoidHandle == INVALID_HANDLE_VALUE) {
		AdaptoidHandle = NULL;
		nAdaptoids = 0;
		Debug(0,L_STR(IDS_NO_ADAPTOID));
	} else {
		ReadFile(AdaptoidHandle, &nAdaptoids, 1, &nbytes, NULL);
		Debug(0,L_STR(IDS_ADAPTOIDS_FOUND),nAdaptoids);
	}

}

void OpenROM (char *filename) {
	if (cpuContextIsValid == true)
		StopCPU ();
	OnOpen (filename);
	handleCpuThread = CreateThread (NULL, NULL, (LPTHREAD_START_ROUTINE) CpuThreadProc, &dwThrdParam, CREATE_SUSPENDED, &dwThreadId);
	handleAudioThread = CreateThread (NULL, NULL, (LPTHREAD_START_ROUTINE) AudioThreadProc, &dwAThrdParam, CREATE_SUSPENDED, &dwAThreadId);
	cpuContextIsValid = true;
	StartCPU ();
}

void StartCPU (void) {
	if (cpuContextIsValid == false)
		return;
	cpuIsDone = false;
	cpuIsPaused = false;
	ResumeThread (handleCpuThread);
	ResumeThread (handleAudioThread);
}

void StopCPU (void) {
	int slpcntr=0;
	if (cpuContextIsValid == false)
		return;
	cpuIsDone = true;
	cpuIsReset = true;
	if (cpuIsPaused == true) {
		ToggleCPU ();
	}
	while (handleCpuThread) {
		Sleep (10);
		slpcntr++;
		if (slpcntr > 100) {
			TerminateThread(handleCpuThread,0);
			Debug (0, L_STR(IDS_CPU_KILL));
			handleCpuThread = NULL;
		}
	}
	slpcntr = 0;
	while (handleAudioThread) {
		Sleep (100);
		slpcntr++;
		if (slpcntr > 100) {
			TerminateThread(handleAudioThread,0);
			Debug (0, L_STR(IDS_AUDIO_KILL));
			handleAudioThread = NULL;
		}
	}
	VirtualFree ((void*)(valloc+0x10000000),64*1024*1024,MEM_DECOMMIT);
	cpuIsPaused = false;
	cpuIsReset = false;
	cpuContextIsValid = false;
	handleCpuThread = NULL;
	handleAudioThread = NULL;
	if (slpcntr < 100)
		Debug (0, L_STR(IDS_THREAD_DIE));
	SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) L_STR(IDS_EMU_STOPPED));
}

void ToggleCPU (void) { // Toggles the pause state and unpause state
	if (cpuContextIsValid == false)
		return;
	if (cpuIsPaused == true) {
		cpuIsPaused = false;
		ResumeThread (handleCpuThread);
		ResumeThread (handleAudioThread);
		Debug (0, L_STR(IDS_CPU_UNPAUSED));
		SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) L_STR(IDS_EMU_STARTED));
	} else {
		cpuIsPaused = true;
		SuspendThread (handleCpuThread);
		SuspendThread (handleAudioThread);
		Debug (0, L_STR(IDS_CPU_PAUSED));
		SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) L_STR(IDS_EMU_PAUSED));
	}
}
// End of the interfacing
// Implementation functions below

void ResetCPU ();

UINT CpuThreadProc(LPVOID) {/*
	if (gfxdll.Load (RegSettings.vidDll) == FALSE) {
		Debug(0,"Using Internal Functions for video...");
		gfxdll.Load (".");// if it fails to load we still have the defaults...
	}*/
	gfxdll.Init();
	gfxdll.RomOpen();
	inpdll.InitiateControllers (GhWnd, ContInfo);
	inpdll.RomOpen();
	while (cpuIsDone == false) {
		cpuIsReset = false;
		ResetCPU ();
		while (cpuIsReset == false) {
			__try {
				Emulate ();
			}__except ( HandleWin32Exception(GetExceptionInformation()) ) {
					short* hh = (short*)intelException[0].ExceptionRecord[0].ExceptionAddress;
					if (hh[0]==0x008B) {
						intelException[0].ContextRecord[0].Eax = 0;
						intelException[0].ContextRecord[0].Eip += 2;
					} else if (hh[0]==0x0889) {
						intelException[0].ContextRecord[0].Eip += 2;
					} else if (hh[0]==0x0888) {
						intelException[0].ContextRecord[0].Eip += 2;
					}
			}
		}
	}
	cpuIsReset = false;
	cpuIsReset = true;
	gfxdll.RomClosed();
	inpdll.RomClosed();
	/*gfxdll.Close();
	gfxdll.Load(".");*/
	Debug(0,"Thread Exited normally");
	handleCpuThread = NULL;
	ExitThread (0);
	return 0;
}

extern DWORD *DAI;

UINT AudioThreadProc(LPVOID) { // Just until I get the sound plugin specs ;)
	while (cpuIsDone == false) {
//		DAI[1] = 0;
//		DAI[3] = 0;
		Sleep (1000);
	}
	handleAudioThread = NULL;
	ExitThread (0);
	return 0;
}

// Code remaining from eclipse
#define NORMAL_SWAP		0x00
#define WORD_SWAP		0x01
#define DWORD_SWAP		0x02

bool CheckROM (FILE *romfp, int &swapped) {
	memset(&RomHeader, 0, sizeof(RomHeader));

	fread(&RomHeader.valid, 2, 1, romfp);
	fread(&RomHeader.is_compressed, 1, 1, romfp);
	fread(&RomHeader.unknown, 1, 1, romfp);

	if (RomHeader.valid == 0x8037) {
		swapped = NORMAL_SWAP;
	} else if (RomHeader.valid == 0x3780) {
		swapped = WORD_SWAP;
	} else {
		WORD temp;

		temp = RomHeader.valid;
		RomHeader.valid = (RomHeader.is_compressed << 8) | RomHeader.unknown;

		RomHeader.is_compressed = temp & 0xff;
		RomHeader.unknown = (temp >> 8) & 0xff;

		/* more needs to be done here or somewhere else
		   header information-wise */
		
		if (RomHeader.valid == 0x3780) {
			swapped = DWORD_SWAP;
		} else {
			MessageBox(GhWnd,L_STR(IDS_INVALID_ROM),WINTITLE,MB_OK);
			fclose(romfp);
			return false;
		}		
	}
	return true;
}

#pragma warning( disable : 4035 )

inline DWORD SWAP_DWORD(DWORD y) {
	__asm { 
		mov eax,y 
		bswap eax 
	}
} 

#pragma warning( default : 4035 )

void ReadInROM (FILE *romfp, int swapped, DWORD filesize) {
	char buffer[0x18];
	DWORD i;
	int j = 0, k = 0;

	if (swapped == DWORD_SWAP)
		k = (filesize / 10) / 4;
	else
		k = (filesize / 10) / 32;

	int percent = 0;

	if (swapped == NORMAL_SWAP) {
		for (i = 0; i < filesize; i += 32, j++) {	
			fread(&RomMemory[i], 32, 1, romfp);

			_asm {
				mov edx, dword ptr RomMemory
				add edx, i

				movq mm0, qword ptr [edx]
				movq mm2, qword ptr [edx+8]
				movq mm4, qword ptr [edx+16]
				movq mm6, qword ptr [edx+24]

				movq mm1, mm0
				psrlw mm0, 8
				psllw mm1, 8
				por mm0, mm1

				movq mm3, mm2
				psrlw mm2, 8
				psllw mm3, 8
				por mm2, mm3

				movq mm5, mm4
				psrlw mm4, 8
				psllw mm5, 8
				por mm4, mm5

				movq mm7, mm6
				psrlw mm6, 8
				psllw mm7, 8
				por mm6, mm7

				movq qword ptr [edx], mm0
				movq qword ptr [edx+8], mm2
				movq qword ptr [edx+16], mm4
				movq qword ptr [edx+24], mm6
			}
			if (j==k) {
				j=0;
				percent += 10;
				sprintf(buffer,L_STR(IDS_LOADED_NS), percent);				
				SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) buffer);
			}
		}
		_asm emms
	} else if (swapped == DWORD_SWAP) {
		for (i = 0; i < filesize; i += 4, j++) {	
			fread(&RomMemory[i], 4, 1, romfp);
			_asm {
				mov edx, dword ptr RomMemory
				add edx, i

				mov ebx, dword ptr [edx]
				bswap ebx
				mov dword ptr [edx], ebx
			}
			if (j==k) {
				j=0;
				percent += 10;
				sprintf(buffer,L_STR(IDS_LOADED_DS), percent);				
				SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) buffer);
			}
		}
	} else if (swapped == WORD_SWAP) {
		for (i = 0; i < filesize; i += 32, j++) {	
			fread(&RomMemory[i], 32, 1, romfp);
			if (j==k) {
				j=0;
				percent += 10;
				sprintf(buffer,L_STR(IDS_LOADED_WS), percent);				
				SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) buffer);
			}
		}
	}
}

extern u8* pif;

void OnOpen(char* filename) {
	if (filename==0) return;
	int swapped;
	FILE *romfp;
	FILE *pifrom = NULL;
	DWORD i = 0;
	DWORD filesize = 0;
	char *filename2;

	filename2 = zero;
	for (i=strlen(filename); i !=0;i--) {
		if (filename[i]=='\\') {
			filename2 = filename+i+1;
			break;
		}
	}
	
	romfp = fopen(filename,"rb");
	if (romfp==NULL) {
		Debug (1,L_STR(IDS_OPEN_FAILED),filename);
		return;
	}

	if (CheckROM (romfp, swapped) == false)
		return;

	fseek(romfp, 0, SEEK_END);
	filesize = ftell(romfp);
	GameSize = filesize;

	SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) L_STR(IDS_MEM_ALLOC));
	
	MemoryAllocRom(filesize);

	if (RomMemory == NULL) {
		MessageBox(GhWnd, L_STR(IDS_MEM_ALLOC_ERROR), WINTITLE, MB_OK);
		PostQuitMessage(0);
		return;
	}

	memset(&RomMemory[0], 0, filesize);
	
	fseek(romfp, 0, SEEK_SET); // paranoia?
	
	SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) L_STR(IDS_LOADING));

	ReadInROM (romfp, swapped, filesize);

	SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) L_STR(IDS_LOADED_100));
	fclose(romfp);
	
	memcpy(&RomHeader, &RomMemory[0], sizeof(RomHeader));
	RomHeader.Program_Counter = SWAP_DWORD(RomHeader.Program_Counter);
	RomHeader.CRC1 = SWAP_DWORD(RomHeader.CRC1);
	RomHeader.CRC2 = SWAP_DWORD(RomHeader.CRC2);

	/* take a short break to fill the info struct */
	if (rInfo.ExternalName[0]!=0) delete rInfo.ExternalName;
	rInfo.ExternalName = new char[strlen(filename)+1];
	strcpy(rInfo.ExternalName,filename);

	if (rInfo.InternalName[0]!=0) delete rInfo.InternalName;
	rInfo.InternalName = new char[21];
	strcpy(rInfo.InternalName,(char*)RomHeader.Image_Name);

	rInfo.BootCode = RomHeader.Program_Counter;
	rInfo.CRC1 = RomHeader.CRC1;
	rInfo.CRC2 = RomHeader.CRC2;

	for (i=0; i < GameSize/4; i++) {
		((u32*)RomMemory)[i] = SWAP_DWORD(((u32*)RomMemory)[i]);
	}
	memcpy(idmem, RomMemory, 0x1000);
	
	pc = 0;
	memset(&CpuRegs, 0, sizeof(CpuRegs));
	memset(&MmuRegs, 0, sizeof(MmuRegs));
	memset(&FpuRegs.w[0], 0, sizeof(FpuRegs));
	memset(&FpuControl, 0, sizeof(FpuControl));
	
	int dwThrdParam = 0;
	unsigned long dwThreadId = 0;
	Debug (0,L_STR(IDS_EMU_BEGIN),RomHeader.Country_Code,filename);
	if (RomHeader.Country_Code=='D' || RomHeader.Country_Code=='P') {
		//PALtiming=true; TODO: hmmm
	}

	if (strcmp(rInfo.InternalName, "") == 0)
		strcpy(rInfo.InternalName, filename2);

	//ChangeMainText(rInfo.InternalName); TODO: hmmm
	Debug (0, "RomTitle: %s", rInfo.InternalName);
}

// End Code remaining from eclipse

// end of implementation functions
unsigned long crcTable[256];
static bool tableIsGenerated = false;

void crcgen() {
	unsigned long	crc, poly;
	int	i, j;

	poly = 0xEDB88320L;
	for (i=0; i<256; i++) {
		crc = i;
		for (j=8; j>0; j--) {
			if (crc&1) {
				crc = (crc >> 1) ^ poly;
			} else {
				crc >>= 1;
			}
		}
		crcTable[i] = crc;
	}
}

unsigned long GenerateCRC (unsigned char *data, int size) {

	if (tableIsGenerated == false)
		crcgen ();

	register unsigned long crc;
	int c = 0;

	crc = 0xFFFFFFFF;
	while(c<size) {
		crc = ((crc>>8) & 0x00FFFFFF) ^ crcTable[ (crc^data[c++]) & 0xFF ];
	}
	return( crc^0xFFFFFFFF );
}
