#define V_VSYNC
#define SITIME 1000
#define PITIME 250
//#define AITIME 510
//#define AITIME 350000
#define AITIME 750000
//#define AITIME 930000


//#define SITIME (500 * incrementer)
//#define PITIME (data * 4 + 25) //((((float)(data+1) * 8.9) + 50)*incrementer)
//#define PITIME ((((float)(data+1) * 8.9) + 50)*incrementer)
//#define SITIME (100 * incrementer)
//#define RSPTIME 5//(1000 * CF)
//#define AITIME 5//(1000 * CF)

/**************************************************************************
 *                                                                        *
 *               Copyright (C) 2000, Eclipse Productions                  *
 *                                                                        *
 *  Offical Source code of the Apollo Project.  DO NOT DISTRIBUTE!  Any   *
 *  unauthorized distribution of these files in any way, shape, or form   *
 *  is a direct infringement on the copyrights entitled to Eclipse        *
 *  Productions and will you will be prosecuted to the fullest extent of  *
 *  the law.  Have a nice day... ^_^                                      *
 *                                                                        *
 *************************************************************************/

/**************************************************************************
 *
 *  Revision History:
 *		Date:		Comment:
 *	-----------------------------------------------------------------------
 *      05-16-00    Initial Version (Andrew)
 *      05-30-00    Memory Rewrite (Andrew)
 *      06-02-00    Inline of Functions (Andrew)
 *      06-20-00    Addition of AI,SP, and DP registers (Andrew)
 *		11-01-01	Readded the scanline hack.  ExciteBike wants it
 *
 **************************************************************************/
/**************************************************************************
 *
 *  Notes:
 *	
 *	-Like the new memory system? Fast as fuck compared to the old. :-)
 *	-Memory is now inlined, for speed purposes. Now the only slow memory routine
 *   is CookData. This one needs some speedups and we got it all set. MSVC makes
 *   some real crap code for that function...
 *
 **************************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <time.h>
#include "resource.h"
#include "WinMain.h"
#include "EmuMain.h"
#include "videodll.h"
#include "audiodll.h"
#include "cpumain.h"

u32 LastAudio;

#pragma warning( disable : 4244 ) // this disables a pointless warning i hate
#pragma warning( disable : 4035 ) // this disables a pointless warning i hate

#define MEM_ERROR(x,y) Debug(0,"%s %08X",x,y)

#define RSP_DLIST_EVENT		0x1
#define RSP_EVENT			0x2
#define SI_DMA_EVENT		0x3
#define PI_DMA_EVENT		0x4
#define AI_DMA_EVENT		0x5

void ScheduleEvent (u32 EventType, u32 Time, u32 size, void *extra);
void DoPifEmulation (u8* dest, u8* src); // TODO: Waiting for new memory
unsigned long GameSize;

u32 valloc = 0;
u8* rdram = NULL;
u8* ramRegs0 = NULL;
u8* ramRegs4 = NULL;
u8* ramRegs8 = NULL;
u8* RomMemory = NULL;
u8* idmem = NULL;
u8* SP = NULL;
u8* SP2 = NULL;
u8* DPC = NULL;
u8* DPS = NULL;
u8* MI = NULL;
u8* VI = NULL;
u8* AI = NULL;
u8* PI = NULL;
u8* RI = NULL;
u8* SI = NULL;
u8* pif = NULL;
u8* cartDom2 = NULL;
u8* cartDom1 = NULL;
u8* cartDom3 = NULL;
u8* cartDom4 = NULL;
DWORD *DVI; // Used for VI Hack
DWORD *DAI; // Used for AI temp fix
DWORD fsize=0;

extern char FlashRAM[0x20000]; // 1MBit in size
extern bool SRinUse; // SaveChips.cpp
extern bool FRinUse;

char FlashRAM_Buffer[128];
u32 FlashRAM_Offset = 0;

int rdramsize = 4*1024*1024;

void InitMem ( int memsize ) {
	if ((valloc = (u32)VirtualAlloc(0,512*1024*1024,MEM_RESERVE,PAGE_READWRITE))!=NULL) {
		Debug (0, "%08X used as memory base.", valloc);
	} else {
		Debug (1, "Page could not be allocated. Sorry, but don't try to run anything. It won't work");
		return;
	}
	rdramsize = memsize;
	rdram	= (u8*)VirtualAlloc((void*)valloc,				memsize+1,  MEM_COMMIT,PAGE_READWRITE);
	idmem	= (u8*)VirtualAlloc((void*)(valloc+0x04000000),	0x2000,		MEM_COMMIT,PAGE_READWRITE);
	SP		= (u8*)VirtualAlloc((void*)(valloc+0x04040000),	0x20,		MEM_COMMIT,PAGE_READWRITE);
	SP2		= (u8*)VirtualAlloc((void*)(valloc+0x04080000),	0x08,		MEM_COMMIT,PAGE_READWRITE);
	DPC		= (u8*)VirtualAlloc((void*)(valloc+0x04100000),	0x10,		MEM_COMMIT,PAGE_READWRITE);
	DPS		= (u8*)VirtualAlloc((void*)(valloc+0x04200000),	0x10,		MEM_COMMIT,PAGE_READWRITE);
	MI		= (u8*)VirtualAlloc((void*)(valloc+0x04300000),	0x10,		MEM_COMMIT,PAGE_READWRITE);
	VI		= (u8*)VirtualAlloc((void*)(valloc+0x04400000),	0x38,		MEM_COMMIT,PAGE_READWRITE);
	AI		= (u8*)VirtualAlloc((void*)(valloc+0x04500000),	0x18,		MEM_COMMIT,PAGE_READWRITE);
	PI		= (u8*)VirtualAlloc((void*)(valloc+0x04600000),	0x34,		MEM_COMMIT,PAGE_READWRITE);
	RI		= (u8*)VirtualAlloc((void*)(valloc+0x04700000),	0x20,		MEM_COMMIT,PAGE_READWRITE);
	SI		= (u8*)VirtualAlloc((void*)(valloc+0x04800000),	0x1C,		MEM_COMMIT,PAGE_READWRITE);
	ramRegs0= (u8*)VirtualAlloc((void*)(valloc+0x03F00000),	0x30,		MEM_COMMIT,PAGE_READWRITE);
	ramRegs4= (u8*)VirtualAlloc((void*)(valloc+0x03F04000),	0x30,		MEM_COMMIT,PAGE_READWRITE);
	ramRegs8= (u8*)VirtualAlloc((void*)(valloc+0x03F80000),	0x30,		MEM_COMMIT,PAGE_READWRITE);
	cartDom2= (u8*)VirtualAlloc((void*)(valloc+0x05000000),	0x10000,	MEM_COMMIT,PAGE_READWRITE);
	cartDom1= (u8*)VirtualAlloc((void*)(valloc+0x06000000),	0x10000,	MEM_COMMIT,PAGE_READWRITE);
	cartDom3= (u8*)VirtualAlloc((void*)(valloc+0x08000000),	0x20000,	MEM_COMMIT,PAGE_READWRITE);
	RomMemory=(u8*)VirtualAlloc((void*)(valloc+0x10000000),	0x10000,	MEM_COMMIT,PAGE_READWRITE); // Just to make J's DLL work
	cartDom4= (u8*)VirtualAlloc((void*)(valloc+0x1FD00000),	0x10000,	MEM_COMMIT,PAGE_READWRITE);
	pif		= (u8*)VirtualAlloc((void*)(valloc+0x1FC00000),	0x800,		MEM_COMMIT,PAGE_READWRITE);

//  Added for VI Hack and AI fix
	DVI = (u32 *)VI;
	DAI = (u32 *)AI;

	((u32*)MI)[1] = 0x01010101;
	((u32*)DPC)[3] = 0;
}

u32 romsize;

void ChangeRDRAMSize (int memsize) {
	if (memsize != rdramsize) {
		VirtualFree((void*)valloc,				rdramsize+1, MEM_DECOMMIT);
		rdram	= (u8*)VirtualAlloc((void*)valloc,				memsize+1,  MEM_COMMIT,PAGE_READWRITE);
		rdramsize = memsize;
	}
}

void MemoryAllocRom(u32 filesize) {
	filesize = ((filesize + 0x1fffff)/0x200000)*0x400000;//bring it up to a even 2MB.
	romsize = filesize;
	VirtualFree((void*)(valloc+0x10000000),	64*1024*1024,MEM_DECOMMIT);
	RomMemory= (u8*)VirtualAlloc((void*)(valloc+0x10000000),	filesize,	MEM_COMMIT,PAGE_READWRITE);
	Debug (0, "Allocated %X bytes for ROM Image", filesize);
	fsize = filesize;
}

void KillMemory(void) {
	VirtualFree((void*)valloc,				rdramsize+1, MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x04000000),	0x2000,		 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x04040000),	0x20,		 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x04080000),	0x08,		 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x04100000),	0x10,		 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x04200000),	0x10,		 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x04300000),	0x10,		 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x04400000),	0x38,		 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x04500000),	0x18,		 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x04600000),	0x34,		 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x04700000),	0x20,		 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x04800000),	0x1C,		 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x1FC00000),	0x800,		 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x03F00000),	0x30,		 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x03F04000),	0x30,		 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x03F80000),	0x30,		 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x05000000),	1024*1024,	 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x06000000),	1024*1024,	 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x08000000),	0x20000,	 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x10000000),	64*1024*1024,MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x1FD00000),	1024*1024,MEM_DECOMMIT);
}

 //Used for VI Hack..

extern u32 VsyncTime;
extern int InterruptTime;
#define MMU_COUNT MmuRegs[9]
u32 ReadData (u32 addr) {
	u32 retVal;
	switch (addr & 0x00ffffff) {
		case 0x00400010: // Current Scanline
			retVal = ((u32*)returnMemPointer(addr))[0];
			retVal+=0x80;
			if (retVal > 512)
				retVal = 0;
			DVI[4] = retVal;
			//Debug (0, "Scanline count: %i", retVal);
			return retVal;
		break;
		case 0x00500000: {
			//Debug (0, "Read AI_DRAM_ADDR_REG = %08X", *(u32 *)(AI+0x00));
		} break;
		case 0x00500004: {
			instructions = (VsyncTime - InterruptTime);
			MMU_COUNT = instructions;
			u32 retVal = snddll.AiReadLength ();
			//((u32*)returnMemPointer(addr))[0] = retVal;
			//Debug (0, "Read AI_LEN_REG = %08X", *(u32 *)(AI+0x04));
			return retVal;
			//return ((u32*)returnMemPointer(addr))[0] / 2;
		}
/*
		case 0x00500008: {
			Debug (0, "Read AI_CONTROL_REG = %08X", *(u32 *)(AI+0x08));
		} break;
		case 0x0050000C: {
			Debug (0, "Read AI_STATUS_REG = %08X", *(u32 *)(AI+0x0C));
		} break;
		case 0x00500010: {
			Debug (0, "Read AI_DACRATE_REG = %08X", *(u32 *)(AI+0x10));
		} break;
		case 0x00500014: {
			Debug (0, "Read AI_BITRATE_REG = %08X", *(u32 *)(AI+0x14));
		} break;
*/
		default:
			return ((u32*)returnMemPointer(addr))[0];
	}
	return ((u32*)returnMemPointer(addr))[0];
}

/*
CookData: This function is the hardware handler for the N64.
  It takes the data, and uses it in a mimicing fashion, and returns
  data in such a way the read functions to not have to be smart.

  This function is far from complete.
*/
#define FLASH_NOOP  0
#define FLASH_ERASE 1
#define FLASH_WRITE 2
#define FLASH_STATUS 3
u32 FlashRAM_Mode = FLASH_NOOP;


u32 CookData(u32 addr, u32 data) {
	u32 retVal;
	if ((addr&0x1f000000) > 0x05000000) {
		if ((addr & 0x1F000000) > 0x08020000)
			return data;
		if ((addr & 0x1fff0000) == 0x8000000)
			return data;
		if (FRinUse == false) {
			((u32*)cartDom3)[0] = 0x00000000;
			((u32*)cartDom3)[1] = 0x00000000;
			Debug (0, "FlashRAM Detected!");
			FRinUse = true;
			SRinUse = false; // This save type dominates
			FlashRAM_Mode = FLASH_NOOP;
		}
		//Debug (0, "Write to %08X - %08X", addr, data);
		switch (data >> 24) {
			case 0x4b: // Set Erase Block Mode (128 Byte - aligned )
				FlashRAM_Mode = FLASH_ERASE;
				FlashRAM_Offset = (data & 0xFFFF) * 128;
			break;

			case 0x78: // Second part of the Erase Block
				((u32*)cartDom3)[0] = 0x11118008;
				((u32*)cartDom3)[1] = 0x00c20000;
			break;

			// Init a FlashWrite
			case 0xb4:
				FlashRAM_Mode = FLASH_WRITE;
			break;

			// Sets the Block for the FlashWrite (128 Byte - aligned )
			case 0xa5:
				FlashRAM_Offset = (data & 0xFFFF) * 128;
				((u32*)cartDom3)[0] = 0x11118004;
				((u32*)cartDom3)[1] = 0x00c20000;
			break;

			case 0xD2: { // Flash RAM Execute
				switch (FlashRAM_Mode) {
					case FLASH_NOOP:
						break;
					case FLASH_ERASE:
						memset(&FlashRAM[FlashRAM_Offset], 0xFF, 128);
					break;
					case FLASH_WRITE:
						//Debug (0, "FlashRAM Write: %08X", FlashRAM_Offset);
						memcpy(&FlashRAM[FlashRAM_Offset], &FlashRAM_Buffer[0], 128);
					break;
				}
			}
			break;
			case 0xE1: { // Set FlashRAM Status
				FlashRAM_Mode = FLASH_STATUS;
				((u32*)cartDom3)[0] = 0x11118001;
				((u32*)cartDom3)[1] = 0x00c20000;
			}
			break;
			case 0xF0: // Set FlashRAM Read
				FlashRAM_Mode = FLASH_NOOP;
				FlashRAM_Offset = 0;
				((u32*)cartDom3)[0] = 0x11118004;
				((u32*)cartDom3)[1] = 0xf0000000;
			break;
			default:
				FRinUse = false;
				return data;
		}
		
		return data;
	}
	
	switch (addr & 0x00ffffff) {
//	case 0x00080000: break;//SP2
//	case 0x00200000: break;//DPS
//	case 0x00700000: break;//RI
	case 0x00040008://SP DRAM->IDMEM
		if (data+1) memcpy(&idmem[((u32*)SP)[0] & 0x1fff],returnMemPointer(((u32*)SP)[1]),(data+1)&0x1fff);
		return data; break;
	case 0x0004000C://SP IDMEM->DRAM
		if (data+1) memcpy(returnMemPointer(((u32*)SP)[1]),&idmem[((u32*)SP)[0] & 0x1fff],(data+1)&0x1fff);
		return data; break;
	case 0x00040010://SP
		retVal = ((u32*)SP)[4];
		if (data & 0x1) retVal &= ~0x1;//clear and set halt
		if (data & 0x2) retVal |= 0x1;
		if (data & 0x4) retVal &= ~0x2;//clear broken
		if (data & 0x8) CLEAR_INTR(SP_INTERRUPT);
		if (data & 0x10) SET_INTR(SP_INTERRUPT);//clear and set interrupt
		if (data & 0x20) retVal &= ~0x20;//clear and set sstep
		if (data & 0x40) retVal |= 0x20;
		if (data & 0x80) retVal &= ~0x40;//clear and set interrupt on break
		if (data & 0x100) retVal |= 0x40;
		if (data & 0x200) retVal &= ~0x80;//clear and set sig0
		if (data & 0x400) retVal |= 0x80;
		if (data & 0x800) retVal &= ~0x100;//clear and set sig1
		if (data & 0x1000) retVal |= 0x100;
		if (data & 0x2000)retVal &= ~0x200;//clear and set sig2 
		if (data & 0x4000) retVal |= 0x200;
		if (data & 0x8000) retVal &= ~0x400;//clear and set sig3
		if (data & 0x10000) retVal |= 0x400;
		if (data & 0x20000) retVal &= ~0x800;//clear and set sig4
		if (data & 0x40000) retVal |= 0x800;
		if (data & 0x80000) retVal &= ~0x1000;//clear and set sig5
		if (data & 0x100000) retVal |= 0x1000;
		if (data & 0x200000) retVal &= ~0x2000;//clear and set sig6
		if (data & 0x400000) retVal |= 0x2000;
		if (data & 0x800000) retVal &= ~0x4000;//clear and set sig7
		if (data & 0x1000000) retVal |= 0x4000;
		if (!(retVal & 1))
			SignalRSP (&retVal);
			//ScheduleEvent (RSP_EVENT, RSPTIME, &((u32*)SP)[4]);
		return retVal; break;
	case 0x0004001C://SP
//		SPsema=0;
		return 0; break;
	case 0x00100000:
		//Debug (0, "Written to the RDP_START_REG");
	break;
	case 0x00100004://DPC END
		Debug (0, "Written to the DPC_END_REG");
		SignalRDP();
	break;
	case 0x00100008:
		Debug (0, "Written to the DPC_CURRENT_REG");
	break;
	case 0x0010000C://DPC STATUS
		//Debug (0, "Written to the DPC_STATUS_REG: %08X", data);
		retVal = ((u32*)DPC)[3];
		if (data & 0x1) retVal &= ~0x1;
		if (data & 0x2) retVal |= 0x1;
		if (data & 0x4) retVal &= ~0x2;
		if (data & 0x8) retVal |= 0x2;
		if (data & 0x10) retVal &= ~0x4;
		if (data & 0x20) retVal |= 0x4;
		if (data & 0x40) ((u32*)DPC)[7] = 0;
		if (data & 0x80) ((u32*)DPC)[6] = 0;
		if (data & 0x100) ((u32*)DPC)[5] = 0;
		if (data & 0x200) ((u32*)DPC)[4] = 0;
		return retVal;/* | 0x680;*/ break;
	case 0x00100010:
		Debug (0, "Written to the DPC_CLOCK_REG");
	break;
	case 0x00100014:
		Debug (0, "Written to the DPC_BUFBUSY_REG");
	break;
	case 0x00100018:
		Debug (0, "Written to the DPC_PIPEBUSY_REG");
	break;
	case 0x0010001C:
		Debug (0, "Written to the DPC_TMEM_REG");
	break;
	case 0x00300000://MI
		retVal = ((DWORD*)MI)[0];
		if (data & 0x80) retVal &= ~0x80;
		if (data & 0x100) retVal |= 0x80;
		if (data & 0x200) retVal &= ~0x100;
		if (data & 0x400) retVal |= 0x100;
		if (data & 0x800) CLEAR_INTR(DP_INTERRUPT);
		if (data & 0x1000) retVal &=~0x200;
		if (data & 0x2000) retVal |=0x200;
		return retVal; break;
	case 0x00300008://MI
		Debug (0, "This shouldn't happen: %08X", data);
		return ((DWORD*)MI)[2];
	break;
	case 0x0030000C://MI
		//Any write to this register causing an interrupt previously masked to come through
		//will cause an interrupt.
		retVal = ((DWORD*)MI)[3];
		if (data & 0x2) retVal |= 0x1;
		if (data & 0x8) retVal |= 0x2;
		if (data & 0x20) retVal |= 0x4;
		if (data & 0x80) retVal |= 0x8;
		if (data & 0x200) retVal |= 0x10;
		if (data & 0x800) retVal |= 0x20;
		if (data & 0x1) retVal &= ~0x1;
		if (data & 0x4) retVal &= ~0x2;
		if (data & 0x10) retVal &= ~0x4;
		if (data & 0x40) retVal &= ~0x8;
		if (data & 0x100) retVal &= ~0x10;
		if (data & 0x400) retVal &= ~0x20;
		((DWORD*)MI)[3] = retVal;
		if ((((u32*)MI)[3] & (((u32*)MI)[2]))) {
			ScheduleEvent (0x6, 200, 0, NULL); // Delay the interrupt a bit
			//InterruptNeeded |= ((u32*)MI)[2];
			//CheckInterrupts(); // Enabling this causes flashes in Mario Party 2
		}
		//Debug (0, "MI[0xC] = %X", retVal);
		return retVal; break;
	case 0x00400000:// VI_STATUS
		*(u32 *)(VI+0x0) = data;
		gfxdll.ViStatusChanged ();
		return data; break;
	case 0x00400004:// Origin Changed
		//Debug (0, "Origin Changed: %08X", data);
	break;
	case 0x00400008:// VI_WIDTH
		//Debug (0, "Width Changed: %08X", data);
		*(u32 *)(VI+0x8) = data;
		gfxdll.ViWidthChanged ();
		return data; break;
	case 0x00400010://VI
		CLEAR_INTR(VI_INTERRUPT);
		//Debug (0, "VI Clear");
		return DVI[4]/*data*/; break;
	case 0x00400018: {// VSYNC
/*
[21:45] <zilmar> 			VI_INTR_TIME = (VI_V_SYNC_REG + 1) * 1500;
[21:45] <zilmar> 			if ((VI_V_SYNC_REG % 1) != 0) {
[21:45] <zilmar> 				VI_INTR_TIME -= 38;
[21:45] <zilmar> 			}*/		
#ifdef V_VSYNC
		extern u32 VsyncTiming;
		static u32 oldtiming=0;
		//if (VsyncTiming == 625000)
		//	oldtiming = 0;
		if (oldtiming != data) {
			oldtiming = data;
			VsyncTiming = (data + 1) * 1500;
			if ((data % 0x1) != 0)
				VsyncTiming -= 38;
//			if (RegSettings.VSyncHack > 0)
//				VsyncTiming /= RegSettings.VSyncHack;
			Debug (0, "VsyncTiming is now %i", VsyncTiming);
			//if (pr32 (0x8007CF30) == 0x504C0004) {
			//	if (pr32 (0x8007CF38) == 0x1000FFFF) { // HACK: Yoshi Story
			//		Debug (0, "Yoshi Story Hack inplace");
				//	pw32 (0x8007CF30, 0x00000000); // Yoshi Story Hack
				//	pw32 (0x8007CF38, 0x00000000); // Yoshi Story Hack
			//	}
			//}
		}
#endif
		return data; break; }
	case 0x00400030: { // Needed to Fix Zelda OOT 1.2
		if (data == 0)
			data = *(u32 *)(VI+0x30);
		return data;
	} break;
	case 0x00400034: { // Needed to Fix Zelda OOT 1.2
		if (data == 0)
			data = *(u32 *)(VI+0x34);
		return data;
	} break;

	case 0x00500000: { // W
		//Debug (0, "Write AI_DRAM_ADDR_REG = %08X", data);
	} break;
	case 0x00500004: { // R/W
		//Debug (0, "Write AI_LEN_REG = %08X - %i", data, data);
		/*if ((*(u32 *)(AI+0xC) & 1) == 0) // AI DMA Disabled
			return data;*/
		if (data) {//If there is no data, don't start playing...(duh)
			((u32*)AI)[1] = data;
			instructions = (VsyncTime - InterruptTime);
			MMU_COUNT = instructions;
			ScheduleEvent (AI_DMA_EVENT, AITIME, 0, NULL);
			snddll.AiLenChanged();
			((DWORD *)AI)[3] = 0xC0000000;
			//dprintf ("$");
			void GetNextInterrupt ();
			GetNextInterrupt ();
			//snddll.AiUpdate(FALSE);
		}
		return data;
	} break;
	case 0x00500008: { // W
		Debug (0, "Write AI_CONTROL_REG = %08X", data);
	} break;
	case 0x0050000C: { // R/W
		CLEAR_INTR(AI_INTERRUPT);
		//Debug (0, "Write AI_STATUS_REG = %08X", data);
		return *(u32 *)(AI+0xC);
	} break;

	case 0x00500010: { // W
		extern u32 SystemType;
		*(u32 *)(AI+0x10) = data;
		if (snddll.AiDacrateChanged)
			snddll.AiDacrateChanged(!SystemType);
		if (SystemType == 1)
			Debug (0, "Write AI_DACRATE_REG = %i - %f", data, 48681812.0/(float)(data+1));
		else
			Debug (0, "Write AI_DACRATE_REG = %i - %f", data, 49656530.0/(float)(data+1));
		return data;
	} break;
	case 0x00500014: { // W
		Debug (0, "Write AI_BITRATE_REG = %i", data+1);
	} break;

/*
  0x0450 0000 to 0x0450 0003  AI_DRAM_ADDR_REG
  0x0450 0004 to 0x0450 0007  AI_LEN_REG
  0x0450 0008 to 0x0450 000B  AI_CONTROL_REG
  0x0450 000C to 0x0450 000F  AI_STATUS_REG
  0x0450 0010 to 0x0450 0013  AI_DACRATE_REG
  0x0450 0014 to 0x0450 0017  AI_BITRATE_REG*/
/*
	case 0x00600000:
		Debug (0, "%08X: DRAM Address: %08X", pc-4, data);
		return data;
	case 0x00600004: 
		Debug (0, "%08X: CART Address: %08X", pc-4, data);
		return data;
*/
	case 0x00600010://PI
		if (data & 0x3) { CLEAR_INTR(PI_INTERRUPT); }
		return ((u32 *)PI)[0x4]; break;

	case 0x00600008: {//PI
		u32 src = ((DWORD*)PI)[0];
		u32 dst = ((DWORD*)PI)[1];
			
		//Debug (0, "%08X PI Transfer: (RDRAM)%08X -> (RROM)%08X with size %X", pc-4, ((DWORD*)PI)[0], ((DWORD*)PI)[1], data+1);
		if ((dst >= 0x06000000) && (dst < 0x10000000)) {
			u32 address = ((DWORD*)PI)[1] & 0x1FFFF;
			if (FRinUse) {
				//Debug (0, "%08X PI Transfer: (RDRAM)%08X -> (FlashRAM Buffer)%08X with size %X", pc-4, ((DWORD*)PI)[0], ((DWORD*)PI)[1], data+1);
				for (int x = 0; x <= data; x++) {
					FlashRAM_Buffer[x^3] = *(u8 *)returnMemPointer((src+x)^3);
				}
			} else {
				for (int x = 0; x <= data; x++) {
					FlashRAM[(address+x)^3] = *(u8 *)returnMemPointer((src+x)^3);
				}
				if (SRinUse == false) {
					SRinUse = true;
					Debug (0, "SRAM Save Type Detected!");
				}
			}
		}
		((u32 *)PI)[0x4] |= 0x3;
		ScheduleEvent (PI_DMA_EVENT, 0, 0, NULL);
		//InterruptNeeded |= PI_INTERRUPT;
		//SET_INTR(PI_INTERRUPT);
		return data; break;
	}
	case 0x0060000C: { //PI 
		u32 dst = ((DWORD*)PI)[0];
		u32 src = ((DWORD*)PI)[1];
		static u32 DMAINFO[3];
		//src &= 0x1fffffff;
//		CF04DC30
		//Debug (0, "%08X PI Transfer: (ROM)%08X -> (RDRAM)%08X with size %i", pc-4, ((DWORD*)PI)[1], ((DWORD*)PI)[0], data+1);
		if ((src+data) < (0x10000000+fsize)) {
			if ((src >= 0x08000000) && (src < 0x10000000)) {
				u32 address = ((DWORD*)PI)[1] & 0x1FFFF;
				if (FlashRAM_Mode == FLASH_STATUS) {
					//Debug (0, "%08X PI Transfer: (FlashRAM Status)%08X -> (RDRAM)%08X with size %X", pc-4, ((DWORD*)PI)[1], ((DWORD*)PI)[0], data+1);
					for (int x = 0; x <= data; x++) {
						*(u8 *)returnMemPointer((dst+x)^3) = *(u8 *)returnMemPointer((src+x)^3);
						if (x < 8)
							dprintf ("%02X", *(u8 *)returnMemPointer((src+x)^3));
					}
				} else {
					if (SRinUse != true) {
						address *= 2;
					}
					//Debug (0, "%08X PI Transfer: (FlashRAM)%08X -> (RDRAM)%08X with size %X", pc-4, address, ((DWORD*)PI)[0], data+1);
					for (int x = 0; x <= data; x++) {
						*(u8 *)returnMemPointer((dst+x)^3) = FlashRAM[(address+x)^3];
						//if (x < 8)
						//	dprintf ("%02X", *(u8 *)returnMemPointer((src+x)^3));
					}
				}
				//dprintf ("\r\n");
//0F04DC30
				((u32 *)PI)[0x4] |= 0x3;
				ScheduleEvent (PI_DMA_EVENT, 0, 0, NULL);
			} else if (src < 0x08000000) {
				memset (returnMemPointer(dst), 0x00, data+1);
				Debug (0, "%08X BADx PI Transfer: (ROM)%08X -> (RDRAM)%08X with size %X", pc-4, ((DWORD*)PI)[1], ((DWORD*)PI)[0], data+1);
				((u32 *)PI)[4] &= ~0x3;
				InterruptNeeded |= PI_INTERRUPT;
				SET_INTR(PI_INTERRUPT);
				return data;
			} else {
				DMAINFO[0] = dst;
				DMAINFO[1] = src;
				DMAINFO[2] = data;
				((u32 *)PI)[0x4] |= 0x3;
				ScheduleEvent (PI_DMA_EVENT, PITIME, sizeof(u32)*3, DMAINFO);
				//Debug (2, "PI Scheduled");
			}
			//InterruptNeeded |= PI_INTERRUPT;
			//SET_INTR(PI_INTERRUPT);
			return data;
		} else {
			Debug (0, "%08X BAD PI Transfer: (ROM)%08X -> (RDRAM)%08X with size %X", pc-4, ((DWORD*)PI)[1], ((DWORD*)PI)[0], data+1);
			//cpuIsDone = true;
			//cpuIsReset = true;
			memset (returnMemPointer(dst), 0x00, data+1);
			((u32 *)PI)[0x4] |= 0x1;
			ScheduleEvent (PI_DMA_EVENT, PITIME, 0, NULL);
			//InterruptNeeded |= PI_INTERRUPT;
			//SET_INTR(PI_INTERRUPT);
			return data;
		}
		break;
	} //*/
/*
	case 0x00600008://PI
		//Note: Incorrect emulation of all writes to register 2, case 8. A write here causes a RDRAM->ROM DMA.
		//Debug (0, "%08X PI Transfer: (RDRAM)%08X -> (ROM)%08X with size %u", pc-4, ((DWORD*)PI)[0], ((DWORD*)PI)[1], data+1);
		if ((((DWORD*)PI)[1] >= 0x08000000) && (((DWORD*)PI)[1] < 0x10000000)) {
			u32 address = ((DWORD*)PI)[1] & 0x1FFFF;
			if (FRinUse) {
				//Debug (0, "FlashRAM Write DMA: %08X", ((DWORD*)PI)[1]);
				memcpy(&FlashRAM_Buffer[0], returnMemPointer(((DWORD*)PI)[0]), data+1);
			} else {
				//Debug (0, "SRAM Write DMA: %08X", ((DWORD*)PI)[1]);
				memcpy(&FlashRAM[address], returnMemPointer(((DWORD*)PI)[0]), data+1);
				if (SRinUse == false) {
					SRinUse = true;
					Debug (0, "SRAM Save Type Detected!");
				}
			}
		}
		Debug (0, "%08X PI Transfer: (RDRAM)%08X -> (ROM)%08X with size %X", pc-4, ((DWORD*)PI)[0], ((DWORD*)PI)[1], data+1);
		//MEM_ERROR("RDRAM->ROM",data);
		InterruptNeeded |= PI_INTERRUPT;
		SET_INTR(PI_INTERRUPT);
		return data; break;
	case 0x0060000C://PI
		//if (((DWORD*)PI)[1] < 0x10000000)
		//Debug (0, "%08X PI Transfer: (ROM)%08X -> (RDRAM)%08X with size %u", pc-4, ((DWORD*)PI)[1], ((DWORD*)PI)[0], data+1);
		if ((((DWORD*)PI)[1]+data) < (0x10000000+fsize)) {
			if ((((DWORD*)PI)[1] >= 0x08000000) && (((DWORD*)PI)[1] < 0x10000000)) {
				u32 address = ((DWORD*)PI)[1] & 0x1FFFF;
				if (FlashRAM_Mode == FLASH_STATUS) {
					memcpy(returnMemPointer(((DWORD*)PI)[0]),returnMemPointer(((DWORD*)PI)[1]),data+1);
					//Debug (0, "Flash RAM Status DMA: %08X", ((DWORD*)PI)[1]);
				} else {
					if (SRinUse == true) {
						//Debug (0, "SRAM RAM Read DMA: %08X", ((DWORD*)PI)[1]);
					} else {
						address *= 2;
						//Debug (0, "Flash RAM Read DMA: %08X", ((DWORD*)PI)[1]);
					}
					memcpy(returnMemPointer(((DWORD*)PI)[0]),&FlashRAM[address],data+1);
				}
			} else {
				int exdst = (((DWORD*)PI)[0] & 3);
				int exsrc = (((DWORD*)PI)[1] & 3);
				if ((exdst || exsrc)) {
					for (int x = 0; x <= data; x++) {
						*(u8 *)returnMemPointer((((DWORD*)PI)[0]+x)^3) = *(u8 *)returnMemPointer((((DWORD*)PI)[1]+x)^3);
					}
				} else {
					memcpy(returnMemPointer(((DWORD*)PI)[0]),returnMemPointer(((DWORD*)PI)[1]),data+1);
				}
//				memcpy(returnMemPointer(((DWORD*)PI)[0]),returnMemPointer(((DWORD*)PI)[1]),data+1);
			}
			//Debug (0, "%08X PI Transfer: (ROM)%08X -> (RDRAM)%08X with size %X", pc-4, ((DWORD*)PI)[1], ((DWORD*)PI)[0], data+1);
			InterruptNeeded |= PI_INTERRUPT;
			SET_INTR(PI_INTERRUPT);
			return data;
		} else {
			Debug (0, "%08X BAD PI Transfer: (ROM)%08X -> (RDRAM)%08X with size %X", pc-4, ((DWORD*)PI)[1], ((DWORD*)PI)[0], data+1);
			cpuIsDone = true;
			cpuIsReset = true;
			return data;
		}
		break;
//*/
extern u32 PiInterrupt, CountInterrupt, VsyncInterrupt;
extern u32 VsyncTime;

	case 0x00800004://SI PIF2DRAM
		DoPifEmulation ((u8*)returnMemPointer(ReadLong(0x04800000)), (u8*)returnMemPointer(0x1FC007C0));
		//InterruptNeeded |= SI_INTERRUPT;
		//SET_INTR(SI_INTERRUPT);
		//pw32(0x04800018, 0x00001000);//SI_STATUS_REG = 0x00001000;
		((u32 *)SI)[6] = 0x1001;
		ScheduleEvent (SI_DMA_EVENT, SITIME, 0, NULL);
		//PiInterrupt = instructions + 64;
		//VsyncTime = MIN(MIN(CountInterrupt,PiInterrupt),VsyncInterrupt);
		//Debug (0, "P2D: %08X, %08X", ReadLong(0x04800000), data);
		return data; break;
	case 0x00800010://SI DRAM2PIF
		memcpy ((u8*)returnMemPointer(0x1FC007C0), (u8*)returnMemPointer(ReadLong(0x04800000)), 64);
		//InterruptNeeded |= SI_INTERRUPT;
		//SET_INTR(SI_INTERRUPT);
		//pw32(0x04800018, 0x00001000);//SI_STATUS_REG = 0x00001000;
		((u32 *)SI)[6] = 0x1001;
		ScheduleEvent (SI_DMA_EVENT, SITIME, 0, NULL);
		//PiInterrupt = instructions + 64;
		//VsyncTime = MIN(MIN(CountInterrupt,PiInterrupt),VsyncInterrupt);
		//Debug (0, "D2P: %08X, %08X", ReadLong(0x04800000), data);
		return data; break;
	case 0x00800018://SI STATUS REG
		CLEAR_INTR(SI_INTERRUPT); 
		return (((u32 *)SI)[6] &= ~0x1000); break;
	default: break;
	}
	return data;
}