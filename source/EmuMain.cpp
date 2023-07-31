#include <windows.h>
#include <commctrl.h> // For status window
#include <stdio.h>
#include "resource.h"
#include "WinMain.h"
#include "EmuMain.h"
#include "videodll.h"
#include "inputdll.h"
#include "audiodll.h"

void SaveChipOpen ();
void SaveChipClose ();
void SaveState ();
void LoadState ();

bool volatile cpuIsDone = true;
bool volatile cpuIsReset = false;
bool volatile cpuSaveState = false;
bool volatile cpuLoadState = false;

bool cpuContextIsValid=false;
bool cpuIsPaused=false;

extern BOOL bFullScreen;
extern BOOL bShouldFS;

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
bool OnOpen(char* filename);
// end code remaining from eclipse

LPEXCEPTION_POINTERS intelException;
u64 nullMemory = 0;

void DisassembleRange (u32 Start, u32 End);
bool IgnoreErrors = false;
extern u32 VsyncTime;
extern u32 romsize;

u32 ChangeProtectionLevel = PAGE_READONLY;
u32 garbage;

u32 HandleWin32Exception(LPEXCEPTION_POINTERS info) {
	intelException = info;
	extern u32 valloc;
	static u64 nextLW;

	//Debug (1, "Exception = %08X", info[0].ExceptionRecord[0].ExceptionCode);

	if (info[0].ExceptionRecord[0].ExceptionCode==EXCEPTION_ACCESS_VIOLATION) {
		void PrintInterruptStatus ();
			
		if (((info[0].ExceptionRecord[0].ExceptionInformation[1])-valloc) >= 0x10000000) {
			if (((info[0].ExceptionRecord[0].ExceptionInformation[1])-valloc) < 0x1FD00000)
			Debug (0, "UnAuthorized ROM Access: %08X using opcode: %X", pc, sop.op);
			if (sop.op == 0x2b) { // SW
				u32 addr = (u32)(CpuRegs[sop.rs] + (s32)(s16)opcode);
				nextLW = CpuRegs[sop.rt];
				dprintf ("EXCEPTION - Writing %08X from location ", nextLW);
				Debug (0, "%08X", addr);
				ChangeProtectionLevel = PAGE_NOACCESS;
				return EXCEPTION_EXECUTE_HANDLER;
			}
			if (sop.op == 0x23) { // LW
				u32 addr = (u32)(CpuRegs[sop.rs] + (s32)(s16)opcode);
				dprintf ("EXCEPTION - Reading %08X from location ", nextLW);
				Debug (0, "%08X", addr);
				CpuRegs[sop.rt] = nextLW;
				ChangeProtectionLevel = PAGE_READONLY;
				return EXCEPTION_EXECUTE_HANDLER;
			}

			pc-=4;
			

			Debug (0, "Write to ROM seemed to cause a problem: %08X", pc);

			ChangeProtectionLevel = PAGE_READONLY;
			return EXCEPTION_EXECUTE_HANDLER;
			//Debug (1, "This ROM is dead.");
			//for (;;);
			
		} else if (((info[0].ExceptionRecord[0].ExceptionInformation[1])-valloc) == 0x00800000) {
			Debug (0, "Exception Trapped Expansion Pak Detection");
			return EXCEPTION_EXECUTE_HANDLER;
		} else if (((info[0].ExceptionRecord[0].ExceptionInformation[1])-valloc) == 0x008FFFFC) {
			Debug (0, "Exception Trapped Expansion Pak Detection");
			return EXCEPTION_EXECUTE_HANDLER;
		} else if (((info[0].ExceptionRecord[0].ExceptionInformation[1])-valloc) == 0x00400000) {
			Debug (0, "Exception Trapped 4MB Expansion Pak Detection 000");
			return EXCEPTION_EXECUTE_HANDLER;
		} else if (((info[0].ExceptionRecord[0].ExceptionInformation[1])-valloc) == 0x004FFFFC) {
			Debug (0, "Exception Trapped 4MB Expansion Pak Detection FFF");
			return EXCEPTION_EXECUTE_HANDLER;
		} else if (((info[0].ExceptionRecord[0].ExceptionInformation[1])-valloc) > 0x00400000) {
			if (((info[0].ExceptionRecord[0].ExceptionInformation[1])-valloc) < 0x00800000)
			return EXCEPTION_EXECUTE_HANDLER;
		}
/*
		if (IgnoreErrors == false) {
			PrintInterruptStatus ();
			if (MessageBox (GhWnd, "Apollo is having problems with this ROM Image.  It is VERY UNLIKELY this rom will function. You may choose to continue or abort peacefully.\r\nDo you wish to continue?", WINTITLE, MB_YESNO | MB_ICONEXCLAMATION) == IDNO) {
				cpuIsDone = true;
				cpuIsReset = true;
				cpuContextIsValid = false;
				if (bFullScreen == TRUE) {
					gfxdll.ChangeWindow ();
					bFullScreen = FALSE;
				}
			} else {
				IgnoreErrors = true;
			}
		}*/
		
			Debug(0,L_STR(IDS_VIOLATION_ACCESS),(info[0].ExceptionRecord[0].ExceptionInformation[1])-valloc);
			Debug (0, "pc = %08X", pc-4);
			extern char *r4kreg[0x20];
			for (int x = 0; x < 0x20; x++) {
				Debug (0, "%s = %08X", r4kreg[x], CpuRegs[x]);
			}
/*
		DisassembleRange (0xE0096580-0x2000, 0xE0096580+0x2000);
		*/
		//Debug (0, "instructions : %i / vsynctime : %i", instructions, VsyncTime);
		//cpuIsDone = true;
		//cpuIsReset = true;
		//cpuContextIsValid = false;
		//DisassembleRange (0x800D0000, 0x800D2000);
		return EXCEPTION_EXECUTE_HANDLER;
	} else if (info[0].ExceptionRecord[0].ExceptionCode==0xc0001337) {
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
	if (RegSettings.isPakInstalled == true) {
		InitMem (8*1024*1024);
	} else {
		InitMem (4*1024*1024);
	}

	AdaptoidHandle = CreateFile( "\\\\.\\Wish_NA1",  // the name of the Adaptoid driver
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, NULL);

	int nAdaptoids = 0;
	u32 nbytes = 0;

	if (AdaptoidHandle == INVALID_HANDLE_VALUE) {
		AdaptoidHandle = NULL;
		nAdaptoids = 0;
		Debug(0, "No adaptoids found");
	} else {
		ReadFile(AdaptoidHandle, &nAdaptoids, 1, &nbytes, NULL);
		Debug(0, "%i adaptoid(s) found" ,nAdaptoids);
	}

}
void ChangeRDRAMSize (int memsize);

void OpenROM (char *filename) {
	if (cpuContextIsValid == true)
		StopCPU ();
	ChangeRDRAMSize (4*1024*1024 << (RegSettings.isPakInstalled ? 1 : 0));
	if (OnOpen (filename) == true) {
		RecentMenus (filename);
		SaveSettings ();
		handleCpuThread = CreateThread (NULL, NULL, (LPTHREAD_START_ROUTINE) CpuThreadProc, &dwThrdParam, CREATE_SUSPENDED, &dwThreadId);
		handleAudioThread = CreateThread (NULL, NULL, (LPTHREAD_START_ROUTINE) AudioThreadProc, &dwAThrdParam, CREATE_SUSPENDED, &dwAThreadId);
		cpuContextIsValid = true;
		IgnoreErrors = false;
		SaveChipOpen ();
		StartCPU ();
		Sleep (100);
		if (bShouldFS == TRUE) {
			Debug (0, "Going Full Screen");
			bFullScreen = TRUE;
			gfxdll.ChangeWindow();
		}
	} else {
		Debug (1, "Unable to open %s", filename);
	}
}
extern u32 dlists, alists;
void StartCPU (void) {
	if (cpuContextIsValid == false)
		return;
	cpuIsDone = false;
	cpuIsPaused = false;
	dlists=0; alists = 0;
	ResumeThread (handleAudioThread);
	Sleep(25);
	ResumeThread (handleCpuThread);
}
//extern u32 eepDetectToggle;
void StopCPU (void) {
	int slpcntr=0;
	//Debug (0, "EEPRom %08X", eepDetectToggle-1);
	if (cpuContextIsValid == false)
		return;
	if (cpuIsPaused == true) {
		ToggleCPU ();
	}
	cpuIsDone = true;
	cpuIsReset = true;
	while (handleCpuThread) {
		Sleep (100);
		slpcntr++;
		if (slpcntr > 10) {
			TerminateThread(handleCpuThread,0);
			Debug (0, L_STR(IDS_CPU_KILL));
			handleCpuThread = NULL;
		}
	}
	slpcntr = 0;
	while (handleAudioThread) {
		PostThreadMessage (dwAThreadId, WM_QUIT, 0, 0);
 		Sleep (100);
		slpcntr++;
		if (slpcntr > 10) {
			TerminateThread(handleAudioThread,0);
			Debug (0, L_STR(IDS_AUDIO_KILL));
			handleAudioThread = NULL;
		}
	}
	SaveChipClose ();
	VirtualFree ((void*)(valloc+0x10000000),64*1024*1024,MEM_DECOMMIT);
	cpuIsPaused = false;
	cpuIsReset = false;
	cpuContextIsValid = false;
	handleCpuThread = NULL;
	handleAudioThread = NULL;
/*	if (slpcntr < 100)
		Debug (0, L_STR(IDS_THREAD_DIE));*/
	SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) L_STR(IDS_EMU_STOPPED));
}

void ToggleCPU (void) { // Toggles the pause state and unpause state
	if (cpuContextIsValid == false)
		return;
	if (cpuIsPaused == true) {
		cpuIsPaused = false;
		UNCHECK_MENU (ID_CPU_PAUSE);
		ResumeThread (handleCpuThread);
		//ResumeThread (handleAudioThread);
		//Debug (0, L_STR(IDS_CPU_UNPAUSED));
		//SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) L_STR(IDS_EMU_STARTED));
	} else {
		cpuIsPaused = true;
		//cpuIsReset = true;
		CHECK_MENU (ID_CPU_PAUSE);
		SuspendThread (handleCpuThread);
		//SuspendThread (handleAudioThread);
		//Debug (0, L_STR(IDS_CPU_PAUSED));
		//SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) L_STR(IDS_EMU_PAUSED));
	}
}
// End of the interfacing
// Implementation functions below

void ResetCPU ();
void CPUEntry ();

UINT CpuThreadProc(LPVOID) {
	/*if ((gfxdll.Init == NULL) || (inpdll.InitiateControllers == NULL)) {
		Debug (1, "Can't do emulation without plugins installed properly!!!");
		ExitThread (-1);
		return -1;
	}*/
//	gfxdll.Init();
	gfxdll.RomOpen();
	Debug (0, "Graphics Installed");
	inpdll.RomOpen();
	Debug (0, "Input Installed");
	cpuLoadState = cpuSaveState = false;
	ChangeProtectionLevel = PAGE_READONLY;
	DWORD old = ChangeProtectionLevel;
	bool cpuSelect = RegSettings.dynamicEnabled;
//  Remember to reenable this or else some roms are going to hate me...
//    VirtualProtect((void *)(valloc+0x10000000), romsize, ChangeProtectionLevel, &old);

	while (cpuIsDone == false) {
		cpuIsReset = false;
		ResetCPU ();
		while (cpuIsReset == false) {
			/*__try {
				if (ChangeProtectionLevel != old) {
					VirtualProtect((void *)(valloc+0x10000000), romsize, ChangeProtectionLevel, &old); // Ok.. next read is bad
					old = ChangeProtectionLevel;
				}*/
				Emulate ();
			/*}__except ( HandleWin32Exception(GetExceptionInformation()) ) {
					short* hh = (short*)intelException[0].ExceptionRecord[0].ExceptionAddress;
					if (hh[0]==0x008B) {
						intelException[0].ContextRecord[0].Eax = 0;
						intelException[0].ContextRecord[0].Eip += 2;
					} else if (hh[0]==0x0889) {
						VirtualProtect((void *)(valloc+0x10000000), romsize, PAGE_NOACCESS, &old); // Ok.. next read is bad
						intelException[0].ContextRecord[0].Eip += 2;
					} else if (hh[0]==0x0888) {
						intelException[0].ContextRecord[0].Eip += 2;
					}
			}//*/
			// Save/Load States here
				if (cpuIsPaused == true) {
					cpuIsReset = false;
					SuspendThread (handleCpuThread);
				}
				if (cpuSaveState == true) {
					cpuSaveState = false;
					cpuIsReset = false;
					SaveState (); // SaveChips
				}
				if (cpuLoadState == true) {
					cpuLoadState = false;
					cpuIsReset = false;
					gfxdll.RomClosed();
					inpdll.RomClosed();
					LoadState (); // SaveChips
					gfxdll.RomOpen();
					Debug (0, "Graphics Installed");
					inpdll.RomOpen();
					Debug (0, "Input Installed");
					gfxdll.ViStatusChanged ();
					gfxdll.ViWidthChanged ();
					cpuLoadState = cpuSaveState = false;

					ChangeProtectionLevel = PAGE_READONLY;
				}
				if (cpuSelect != RegSettings.dynamicEnabled) {
					cpuSelect = RegSettings.dynamicEnabled;
					cpuIsReset = false;
				}
		}
	}
	/*extern int OpcodeFreq[];
	cpuIsReset = false;
	for (int x=0; x < 0x40; x++) {
		if (OpcodeFreq[x] != 0)
			Debug (0, "Opcode %02X Executed %i times", x, OpcodeFreq[x]);
	}*/
	cpuIsReset = true;
	gfxdll.RomClosed();
	inpdll.RomClosed();
	Debug(0,"Cpu Thread Shutdown");
	handleCpuThread = NULL;
	ExitThread (0);
	return 0;
}

extern DWORD *DAI;

UINT AudioThreadProc(LPVOID) { // Just until I get the sound plugin specs ;)
	snddll.Init();
	while (cpuIsDone == false) {
		if (snddll.AiUpdate) {
			snddll.AiUpdate(TRUE);
		} else {
			handleAudioThread = NULL;
			Debug (0, "Bad/No Audio plugin");
			ExitThread (0);
		}
	}
	snddll.RomClosed();
	Debug (0, "Audio Thread Shutdown");
	handleAudioThread = NULL;
	ExitThread (0);
	return 0;
}

FILE *zipopen (const char *fname, const char *flags);
int zipclose (FILE *fp);
int zipseek (FILE *fp, long offset, int flags);
size_t zipread (void *data, size_t size, size_t count, FILE *fp);
long ziptell (FILE *fp);

bool isCompressed = false;
int fileclose (FILE *fp) {
	if (isCompressed == false) {
		return fclose (fp);
	} else {
		return zipclose (fp);
	}
}
size_t fileread (void *data, size_t size, size_t count, FILE *fp) {
	if (isCompressed == false) {
		return fread (data, size, count, fp);
	} else {
		return zipread (data, size, count, fp);
	}
}

int fileseek (FILE *fp, long offset, int flags) {
	if (isCompressed == false) {
		return fseek (fp, offset, flags);
	} else {
		return zipseek (fp, offset, flags);
	}
}

long filetell (FILE *fp) {
	if (isCompressed == false) {
		return ftell (fp);
	} else {
		return ziptell (fp);
	}
}

// Code remaining from eclipse
#define NORMAL_SWAP		0x00
#define WORD_SWAP		0x01
#define DWORD_SWAP		0x02

bool CheckROM (FILE *romfp, int &swapped) {
	memset(&RomHeader, 0, sizeof(RomHeader));

	fileread(&RomHeader.valid, 2, 1, romfp);
	fileread(&RomHeader.is_compressed, 1, 1, romfp);
	fileread(&RomHeader.unknown, 1, 1, romfp);

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
			fileclose(romfp);
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
			fileread (&RomMemory[i], 32, 1, romfp);

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
				sprintf(buffer, "Loading Rom: %i%%", percent);				
				SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) buffer);
			}
		}
		_asm emms
	} else if (swapped == DWORD_SWAP) {
		for (i = 0; i < filesize; i += 4, j++) {	
			fileread (&RomMemory[i], 4, 1, romfp);
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
				sprintf(buffer, "Loading Rom: %i%%", percent);				
				SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) buffer);
			}
		}
	} else if (swapped == WORD_SWAP) {
		for (i = 0; i < filesize; i += 32, j++) {	
			fileread (&RomMemory[i], 32, 1, romfp);
			if (j==k) {
				j=0;
				percent += 10;
				sprintf(buffer, "Loading Rom: %i%%", percent);				
				SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) buffer);
			}
		}
	}
}

extern u8* pif;

bool OnOpen(char* filename) {
	if (filename==0) return false;
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

/*
	pifrom = fopen ("d:\\emu\\n64\\pj643\\pifntsc.raw", "rb");
	if (pifrom!=NULL) {
		Debug (0, "Loaded pifrom from external source");
		u32 temp;
		for (int cntr=0; cntr < 496; cntr++) {
			fread (&temp, 4, 1, pifrom);
			*((DWORD *)pif+cntr) = SWAP_DWORD(temp);
		}
	}
*/

	if (stricmp(&filename[strlen(filename) - 4], ".zip") == 0) {
		romfp = zipopen (filename, "rb");
		isCompressed = true;
	} else {
		romfp = fopen(filename,"rb");
		isCompressed = false;
	}

	if (romfp==NULL) {
		return false;
	}

	if (CheckROM (romfp, swapped) == false)
		return false;

	fileseek(romfp, 0, SEEK_END);
	filesize = filetell(romfp);
	GameSize = filesize;

	SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) L_STR(IDS_MEM_ALLOC));
	
	MemoryAllocRom(filesize);

	if (RomMemory == NULL) {
		MessageBox(GhWnd, L_STR(IDS_MEM_ALLOC_ERROR), WINTITLE, MB_OK);
		PostQuitMessage(0);
		return false;
	}

	memset(&RomMemory[0], 0, filesize);
	
	fileseek(romfp, 0, SEEK_SET); // paranoia?
	
	SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) L_STR(IDS_LOADING));

	ReadInROM (romfp, swapped, filesize);

	SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)"Rom Loaded...");
	fileclose(romfp);
	
	memcpy(&RomHeader, &RomMemory[0], sizeof(RomHeader));
	//__asm int 3;
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
	void ResetFPU ();
	ResetFPU ();

	int dwThrdParam = 0;
	unsigned long dwThreadId = 0;
	Debug (0,L_STR(IDS_EMU_BEGIN),RomHeader.Country_Code,filename);
	if (RomHeader.Country_Code=='D' || RomHeader.Country_Code=='P') {
		//PALtiming=true; TODO: hmmm
	}

	if (strcmp(rInfo.InternalName, "") == 0)
		strcpy(rInfo.InternalName, filename2);

	//ChangeMainText(rInfo.InternalName); TODO: hmmm
	i = strlen(rInfo.InternalName)-1;
	while ((i > 0)) { // Trim this down for save stuff...
		if (rInfo.InternalName[i] == 32) {
			rInfo.InternalName[i] = '\0';
		} else if ((rInfo.InternalName[i] == 255)) {
			rInfo.InternalName[i] = '\0';
		} else {
			break;
		}
		i--;
	}
	return true;
}

// End Code remaining from eclipse

// end of implementation functions
unsigned long crcTable[256];
bool tableIsGenerated = false;

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
