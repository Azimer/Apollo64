#include <windows.h>
#include <commctrl.h> // For status window
#include <stdio.h>
#include "resource.h"
#include "WinMain.h"
#include "EmuMain.h"
#include "zlib.h"

bool FRinUse  = false;
bool EEPinUse = false;
bool MPinUse  = false;
bool SRinUse  = false;

bool is16kEEP = false;

char savepath[255];

char eeprom[2*1024]; // 2K is the largest
char FlashRAM[0x20000]; // 1MBit in size
char MemPacks[0x8032];

/* Externs needed for save states*/
extern u8* rdram;
extern u8* ramRegs0;
extern u8* ramRegs4;
extern u8* ramRegs8;
extern u8* idmem;
extern u8* SP;
extern u8* SP2;
extern u8* DPC;
extern u8* DPS;
extern u8* MI;
extern u8* VI;
extern u8* AI;
extern u8* PI;
extern u8* RI;
extern u8* SI;
extern u8* pif;

extern u32 VsyncTime;
extern u32 VsyncInterrupt;
extern u32 CountInterrupt;
extern int InterruptTime;
extern s_tlb tlb[32];

void SaveEvents (FILE *fp);
void LoadEvents (FILE *fp);

void MakeSavePath () {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT];

	GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
	_splitpath(path_buffer,drive,dir,fname,ext);
	strcpy (savepath, drive);
	strcat (savepath, dir);
	strcat (savepath, "Save\\");
}
#define SAVE_VERSION 0x103 // odd == compressed, even == uncompressed
u32 ssver = SAVE_VERSION;
u8 *data;

extern u32 PiInterrupt;
extern int NeedCount;

void SaveState () {
	FILE *test;
	int x;
	char rom[255];
	u32 datalen=0;
	strcpy (rom, savepath);
	strcat (rom, rInfo.InternalName);
	strcat (rom, ".ass");
	test = fopen (rom, "wb");
	Debug (0, rom);
	if (test) {
		instructions = (VsyncTime - InterruptTime);

		if (RegSettings.compressSaveStates == false) {
			ssver = 0x0102;
			fwrite (&ssver, 1, sizeof(u32), test);
			for (x = 0; x < 8*1024*1024; x += 1024) {
				fwrite (&rdram[x], 1, 1024, test);
			}
		} else {
			ssver = SAVE_VERSION;
			fwrite (&ssver, 1, sizeof(u32), test);
		}
		Debug (0, "Saving State: %X", ssver);
		fwrite (&idmem[0],    1, 0x2000, test);
		fwrite (&SP[0],       1, 0x20  , test);
		fwrite (&SP2[0],      1, 0x08  , test);
		fwrite (&DPC[0],      1, 0x10  , test);
		fwrite (&DPS[0],      1, 0x10  , test);
		fwrite (&MI[0],       1, 0x10  , test);
		fwrite (&VI[0],       1, 0x38  , test);
		fwrite (&AI[0],       1, 0x18  , test);
		fwrite (&PI[0],       1, 0x34  , test);
		fwrite (&RI[0],       1, 0x20  , test);
		fwrite (&SI[0],       1, 0x1C  , test);
		fwrite (&pif[0],      1, 0x800 , test);
		fwrite (&ramRegs0[0], 1, 0x30  , test);
		fwrite (&ramRegs4[0], 1, 0x30  , test);
		fwrite (&ramRegs8[0], 1, 0x30  , test);

		fwrite (&VsyncTime, 1, sizeof(u32), test);
		fwrite (&VsyncInterrupt, 1, sizeof(u32), test);
		fwrite (&CpuRegs[0], 32, sizeof(u64), test);
		fwrite (&MmuRegs[0], 32, sizeof(u64), test);
		fwrite (&FpuRegs.w[0], 1, sizeof (R4K_FPU_union), test);
		fwrite (&FpuControl[0], 32, sizeof(u32), test);
		fwrite (&cpuHi, 1, sizeof (s64), test);
		fwrite (&cpuLo, 1, sizeof (s64), test);
		fwrite (&instructions, 1, sizeof (u32), test);
		fwrite ((void *)&pc, 1, sizeof (u32), test);
		fwrite ((void *)&InterruptNeeded, 1, sizeof (char), test);
		fwrite (&tlb[0], 32, sizeof(s_tlb), test);
		fwrite (&CountInterrupt, 1, sizeof(u32), test);
		SaveEvents (test);
		if (RegSettings.compressSaveStates == true) {
			data = (u8 *)malloc (8*1024*1024+12);
			datalen = (8*1024*1024+12);
			SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR)("Compressing RDRAM... please wait..."));
			int retVal = compress (data, &datalen, rdram, 8*1024*1024);
			fwrite (data, 1, datalen, test);
			free (data);
			SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR)("Save State Completed"));
			//Debug (0, "retVal = %i, Datalen = %i", retVal, datalen);
		}
		fclose (test);
	} else {
		Debug (1, "Failed to Save State");
	}
}
// TODO: Modify savestates...
void LoadState () {
	FILE *test;
	char rom[255];
	int x;
	u32 datalen, destlen;
	strcpy (rom, savepath);
	strcat (rom, rInfo.InternalName);
	strcat (rom, ".ass");
	test = fopen (rom, "rb");
	fseek (test, 0, SEEK_END);
	datalen = ftell (test);
	fseek (test, 0, SEEK_SET);
	if (test) {
		fread (&ssver, 1, sizeof(u32), test);
		Debug (0, "Loading State: %X", ssver);
		if (ssver != SAVE_VERSION) {
			if ((ssver & 0x1) == 0x0) { // Uncompressed so load rdram
				for (x = 0; x < 8*1024*1024; x += 1024) {
					fread (&rdram[x], 1, 1024, test);
				}
			}
		}
		/*for (x = 0; x < 8*1024*1024; x += 1024) {
			fread (&rdram[x], 1, 1024, test);
		}*/
		fread (&idmem[0],    1, 0x2000, test);
		fread (&SP[0],       1, 0x20  , test);
		fread (&SP2[0],      1, 0x08  , test);
		fread (&DPC[0],      1, 0x10  , test);
		fread (&DPS[0],      1, 0x10  , test);
		fread (&MI[0],       1, 0x10  , test);
		fread (&VI[0],       1, 0x38  , test);
		fread (&AI[0],       1, 0x18  , test);
		fread (&PI[0],       1, 0x34  , test);
		fread (&RI[0],       1, 0x20  , test);
		fread (&SI[0],       1, 0x1C  , test);
		fread (&pif[0],      1, 0x800 , test);
		fread (&ramRegs0[0], 1, 0x30  , test);
		fread (&ramRegs4[0], 1, 0x30  , test);
		fread (&ramRegs8[0], 1, 0x30  , test);

		fread (&VsyncTime, 1, sizeof(u32), test);
		fread (&VsyncInterrupt, 1, sizeof(u32), test);
		fread (&CpuRegs[0], 32, sizeof(u64), test);
		fread (&MmuRegs[0], 32, sizeof(u64), test);
		fread (&FpuRegs.w[0], 1, sizeof (R4K_FPU_union), test);
		fread (&FpuControl[0], 32, sizeof(u32), test);
		fread (&cpuHi, 1, sizeof (s64), test);
		fread (&cpuLo, 1, sizeof (s64), test);
		fread (&instructions, 1, sizeof (u32), test);
		fread ((void *)&pc, 1, sizeof (u32), test);
		fread ((void *)&InterruptNeeded, 1, sizeof (char), test);
		fread (&tlb[0], 32, sizeof(s_tlb), test);
		if (ssver > 0x101) { // EventList was added
			fwrite (&CountInterrupt, 1, sizeof(u32), test);
			LoadEvents (test);
		} else { // Attempt to salvage something...
			if (((u32 *)PI)[0x4] & 0x1) {
				PiInterrupt = instructions + 10;
				InterruptNeeded |= PI_INTERRUPT;
				((u32 *)PI)[4] &= ~0x3;
				Debug (0, "PI Needed out of load");
			}
			if (((u32 *)SI)[6] & 0x1) {
				PiInterrupt = instructions + 10;
				InterruptNeeded |= SI_INTERRUPT;
				Debug (0, "SI Needed out of load");
				((u32 *)SI)[6] &= ~0x1; // Clear the DMA Busy Flag
			}
		}
		if ((ssver & 0x1) == 0) { // uncompressed rom
			fclose (test);
			InterruptTime = (VsyncTime - instructions);
			return;
		}
		datalen = datalen - ftell (test);		
		data = (u8 *)malloc (datalen);
		datalen = fread (data, 1, datalen, test);
		destlen = 8*1024*1024;
		//Debug (0, "destlen = %i, datalen = %i", destlen, datalen);
		int retVal = uncompress (rdram, &destlen, data, datalen);
		free (data);
		//Debug (0, "retVal = %i, destlen = %i, datalen = %i", retVal, destlen, datalen);
		fclose (test);
		InterruptTime = (VsyncTime - instructions);
	} else {
		Debug (1, "Failed to Load State");
	}
}
/*
void LoadOldState100 () {
	FILE *test;
	char rom[255];
	int x;
	strcpy (rom, savepath);
	strcat (rom, rInfo.InternalName);
	strcat (rom, ".ass");
	test = fopen (rom, "rb");
	if (test) {
		Debug (0, "Loading Old State");
		fread (&ssver, 1, sizeof(u32), test);
		if (ssver != 0x100) {
			Debug (1, "Save State is a different version");
			fclose (test);
			return;
		}
		for (x = 0; x < 8*1024*1024; x += 1024) {
			fread (&rdram[x], 1, 1024, test);
		}
		fread (&idmem[0],    1, 0x2000, test);
		fread (&SP[0],       1, 0x20  , test);
		fread (&SP2[0],      1, 0x08  , test);
		fread (&DPC[0],      1, 0x10  , test);
		fread (&DPS[0],      1, 0x10  , test);
		fread (&MI[0],       1, 0x10  , test);
		fread (&VI[0],       1, 0x38  , test);
		fread (&AI[0],       1, 0x18  , test);
		fread (&PI[0],       1, 0x34  , test);
		fread (&RI[0],       1, 0x20  , test);
		fread (&SI[0],       1, 0x1C  , test);
		fread (&pif[0],      1, 0x800 , test);
		fread (&ramRegs0[0], 1, 0x30  , test);
		fread (&ramRegs4[0], 1, 0x30  , test);
		fread (&ramRegs8[0], 1, 0x30  , test);

		fread (&VsyncTime, 1, sizeof(u32), test);
		fread (&VsyncInterrupt, 1, sizeof(u32), test);
		fread (&CpuRegs[0], 32, sizeof(u64), test);
		fread (&MmuRegs[0], 32, sizeof(u64), test);
		fread (&FpuRegs.w[0], 1, sizeof (R4K_FPU_union), test);
		fread (&FpuControl[0], 32, sizeof(u32), test);
		fread (&cpuHi, 1, sizeof (s64), test);
		fread (&cpuLo, 1, sizeof (s64), test);
		fread (&instructions, 1, sizeof (u32), test);
		fread ((void *)&pc, 1, sizeof (u32), test);
		fread ((void *)&InterruptNeeded, 1, sizeof (char), test);
		fread (&tlb[0], 32, sizeof(s_tlb), test);
		fclose (test);
	} else {
		Debug (1, "Failed to Load State");
	}
}

*/
// Done when a rom is open

void SaveChipOpen () {
	FILE *test;
	char rom[255], tmp[255];
	if (savepath[0] == 0)
		MakeSavePath ();
	strcpy (rom, savepath);
	strcat (rom, rInfo.InternalName);


	is16kEEP = FRinUse = EEPinUse = MPinUse = SRinUse = false;
// ************ EEPROM **************
	strcpy (tmp, rom);
	strcat (tmp, ".eep");
	test = fopen (tmp, "rb");
	if (test) {
		fseek (test, 0, SEEK_END);
		if (ftell(test) > 512) {
			is16kEEP = true;
			fseek (test, 0, SEEK_SET);
			fread (eeprom, 1, 2048, test);
			Debug (0, "Loaded 16Kbit EEPROM Save File");
		} else {
			is16kEEP = false;
			if (RegSettings.force4keep == true)
				is16kEEP = true;
			Debug (0, "Loaded 4Kbit EEPROM Save File");
			fseek (test, 0, SEEK_SET);
			fread (eeprom, 1, 512, test);
		}
		EEPinUse = true;
		fclose (test);
	} else {
		if (RegSettings.force4keep == true)
			is16kEEP = true;
	}
// *********** MEMPACKS *************
	strcpy (tmp, rom);
	strcat (tmp, ".mpk");
	test = fopen (tmp, "rb");
	if (test == NULL) {
		strcpy (tmp, savepath);
		strcat (tmp, "default.mpk");
		test = fopen (tmp, "rb");
	}
	if (test) {
		MPinUse = true;
		fread (&MemPacks[0], 1, 0x8032, test);
		Debug (0, "Loaded MEMPACK Save File");
		fclose (test);
	}
// *********** SRAM *************
	strcpy (tmp, rom);
	strcat (tmp, ".sra");
	test = fopen (tmp, "rb");
	if (test) {
		SRinUse = true;
		fread (&FlashRAM[0], 1, 0x8000, test);
		/*__asm {
			lea eax, dword ptr [FlashRAM];
			mov ecx, 0;
srambswap:
			mov edx, [eax+ecx*4];
			bswap edx;
			mov [eax+ecx*4], edx;
			inc ecx;
			cmp ecx, 04000h;
			jnz srambswap;
		}*/
		Debug (0, "Loaded SRAM Save File");
		fclose (test);
	}
// *********** Flash ROM *************
	strcpy (tmp, rom);
	strcat (tmp, ".fla");
	test = fopen (tmp, "rb");
	if (test) {
		FRinUse = true;
		SRinUse = false;
		fread (&FlashRAM[0], 1, 0x20000, test);
		/*	__asm {
				lea eax, dword ptr [FlashRAM];
				mov ecx, 0;
	flrambswap:
				mov edx, [eax+ecx*4];
				bswap edx;
				mov [eax+ecx*4], edx;
				inc ecx;
				cmp ecx, 08000h;
				jnz flrambswap;
			}*/
		Debug (0, "Loaded FlashRAM Save File");
		fclose (test);
	}
}

// Done when a rom is closed
void SaveChipClose () {
	FILE *test;
	char rom[255], tmp[255];
	strcpy (rom, savepath);
	strcat (rom, rInfo.InternalName);

// ************ EEPROM **************
	if (EEPinUse == true) {
		strcpy (tmp, rom);
		strcat (tmp, ".eep");
		test = fopen (tmp, "wb");
		if (test) {
			EEPinUse = false;
			if (is16kEEP == true) {
				fwrite (eeprom, 1, 2048, test);
			} else {
				fwrite (eeprom, 1, 512, test);
			}
			Debug (0, "Saved EEPROM File");
			fclose (test);
		}
	}

// ************ MemPacks **************
	if (MPinUse == true) {
		strcpy (tmp, rom);
		strcat (tmp, ".mpk");
		test = fopen (tmp, "wb");
		if (test == NULL) {
			strcpy (tmp, savepath);
			strcat (tmp, "default.mpk");
			test = fopen (tmp, "wb");
		}
		if (test) {
			MPinUse = false;
			fwrite (MemPacks, 1, 0x8032, test);
			Debug (0, "Saved MemPack File");
			fclose (test);
		}
	}

// *********** SRAM *************
	if (SRinUse == true) {
		strcpy (tmp, rom);
		strcat (tmp, ".sra");
		test = fopen (tmp, "wb");
		if (test) {
			SRinUse = false;
			/*__asm {
				lea eax, dword ptr [FlashRAM];
				mov ecx, 0;
	srambswap:
				mov edx, [eax+ecx*4];
				bswap edx;
				mov [eax+ecx*4], edx;
				inc ecx;
				cmp ecx, 02000h;
				jnz srambswap;
			}*/
			fwrite (&FlashRAM[0], 1, 0x8000, test);
			Debug (0, "Saved SRAM File: %s", tmp);
			fclose (test);
		}
	}
// *********** FlashRAM *************
	if (FRinUse == true) {
		strcpy (tmp, rom);
		strcat (tmp, ".fla");
		test = fopen (tmp, "wb");
		if (test) {
			FRinUse = false;
			/*__asm {
				lea eax, dword ptr [FlashRAM];
				mov ecx, 0;
	flrambswap:
				mov edx, [eax+ecx*4];
				bswap edx;
				mov [eax+ecx*4], edx;
				inc ecx;
				cmp ecx, 08000h;
				jnz flrambswap;
			}*/
			fwrite (&FlashRAM[0], 1, 0x20000, test);
			Debug (0, "Saved FlashRAM File: %s", tmp);
			fclose (test);
		}
	}
}