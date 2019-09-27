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
#pragma warning( disable : 4244 ) // this disables a pointless warning i hate
#pragma warning( disable : 4035 ) // this disables a pointless warning i hate

#define MEM_ERROR(x,y) Debug(0,"%s %08X",x,y)

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


void InitMem ( void ) {
	if ((valloc = (u32)VirtualAlloc(0,512*1024*1024,MEM_RESERVE,PAGE_READWRITE))!=NULL) {
		dprintf(L_STR(IDS_MEM_LOAD),valloc);
	} else {
		dprintf(L_STR(IDS_MEM_LOAD_FAIL));
		return;
	}
	rdram	= (u8*)VirtualAlloc((void*)valloc,				8*1024*1024,MEM_COMMIT,PAGE_READWRITE);
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
	pif		= (u8*)VirtualAlloc((void*)(valloc+0x1FC00000),	0x800,		MEM_COMMIT,PAGE_READWRITE);
	ramRegs0= (u8*)VirtualAlloc((void*)(valloc+0x03F00000),	0x30,		MEM_COMMIT,PAGE_READWRITE);
	ramRegs4= (u8*)VirtualAlloc((void*)(valloc+0x03F04000),	0x30,		MEM_COMMIT,PAGE_READWRITE);
	ramRegs8= (u8*)VirtualAlloc((void*)(valloc+0x03F80000),	0x30,		MEM_COMMIT,PAGE_READWRITE);
	cartDom2= (u8*)VirtualAlloc((void*)(valloc+0x05000000),	0x10000,	MEM_COMMIT,PAGE_READWRITE);
	cartDom1= (u8*)VirtualAlloc((void*)(valloc+0x06000000),	0x10000,	MEM_COMMIT,PAGE_READWRITE);
	cartDom3= (u8*)VirtualAlloc((void*)(valloc+0x08000000),	0x40000,	MEM_COMMIT,PAGE_READWRITE);
	cartDom4= (u8*)VirtualAlloc((void*)(valloc+0x1FD00000),	0x10000,	MEM_COMMIT,PAGE_READWRITE);

//  Added for VI Hack and AI fix
	DVI = (u32 *)VI;
	DAI = (u32 *)AI;

	((u32*)MI)[1] = 0x01010101;
	((u32*)DPC)[3] = 0;
	/*
	FILE *sram = fopen ("zelda.ram", "rb");
	if (sram != NULL) {
		fread (cartDom3, 1, 0x10000, sram);
		__asm {
			mov eax, dword ptr [cartDom3];
			mov ecx, 0;
srambswap:
			mov edx, [eax+ecx*4];
			bswap edx;
			mov [eax+ecx*4], edx;
			inc ecx;
			cmp ecx, 04000h;
			jnz srambswap;

		}
		fclose (sram);
	}
	*/
}

void MemoryAllocRom(u32 filesize) {
	filesize = ((filesize + 0x1fffff)/0x200000)*0x400000;//bring it up to a even 2MB.
	VirtualFree((void*)(valloc+0x10000000),	64*1024*1024,MEM_DECOMMIT);
	RomMemory= (u8*)VirtualAlloc((void*)(valloc+0x10000000),	filesize,	MEM_COMMIT,PAGE_READWRITE);
	Debug (0, "Allocated %X bytes for ROM Image", filesize);
	fsize = filesize;
}

void KillMemory(void) {
	VirtualFree((void*)valloc,				8*1024*1024, MEM_DECOMMIT);
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
	VirtualFree((void*)(valloc+0x08000000),	1024*1024*4,	 MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x10000000),	64*1024*1024,MEM_DECOMMIT);
	VirtualFree((void*)(valloc+0x1FD00000),	1024*1024,MEM_DECOMMIT);
}

 //Used for VI Hack..
u32 ReadHandler (u32 addr) {
	u32 retVal = ((u32*)returnMemPointer(addr))[0];
	retVal++;
	if (retVal > 512)
		retVal = 0;
	DVI[4] = retVal;
	return retVal;
}

/*
CookData: This function is the hardware handler for the N64.
  It takes the data, and uses it in a mimicing fashion, and returns
  data in such a way the read functions to not have to be smart.

  This function is far from complete.
*/
u32 CookData(u32 addr, u32 data) {
	u32 retVal;
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
		if (!(retVal & 1)) SignalRSP(&retVal);
		return retVal; break;
	case 0x0004001C://SP
//		SPsema=0;
		return 0; break;
	case 0x00100004://DPC END
		SignalRDP();
		break;
	case 0x0010000C://DPC STATUS
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
		return retVal; break;
	case 0x00400000:// VI_STATUS
		gfxdll.ViStatusChanged ();
		return data; break;
	case 0x00400008:// VI_WIDTH
		gfxdll.ViWidthChanged ();
		return data; break;
	case 0x00400010://VI
		CLEAR_INTR(VI_INTERRUPT); 
		return data; break;
	case 0x00500004://AI LEN
		if (data) {//If there is no data, don't start playing...(duh)
			((u32*)AI)[1] = data;//gotta do this so the audio routines work.
			if (((u32*)AI)[3] & 0x40000000) ((u32*)AI)[3] |= 0x80000001;//set AI_FULL if sound is playing
			((u32*)AI)[3] |= 0x40000000;//set AI_BUSY
			SignalAI();//Start sound now...
		}
		break;
	case 0x0050000C://AI
		CLEAR_INTR(AI_INTERRUPT);
		return ((u32*)AI)[3]; break;
/*
	case 0x00600000:
		Debug (0, "%08X: DRAM Address: %08X", pc-4, data);
		return data;
	case 0x00600004: 
		Debug (0, "%08X: CART Address: %08X", pc-4, data);
		return data;
*/
	case 0x00600010://PI
		if (data & 0x2) { CLEAR_INTR(PI_INTERRUPT); }
		return 0; break;
	case 0x00600008://PI
		//Note: Incorrect emulation of all writes to register 2, case 8. A write here causes a RDRAM->ROM DMA.
		//Debug (0, "%08X PI Transfer: (RDRAM)%08X -> (ROM)%08X with size %u", pc-4, ((DWORD*)PI)[0], ((DWORD*)PI)[1], data+1);
		if (((DWORD*)PI)[1] < 0x10000000)
			memcpy(returnMemPointer(((DWORD*)PI)[1]),returnMemPointer(((DWORD*)PI)[0]),data+1);
		//MEM_ERROR("RDRAM->ROM",data);
		InterruptNeeded |= PI_INTERRUPT;
		SET_INTR(PI_INTERRUPT);
		return data; break;
	case 0x0060000C://PI
		//if (((DWORD*)PI)[1] < 0x10000000)
		//Debug (0, "%08X PI Transfer: (ROM)%08X -> (RDRAM)%08X with size %u", pc-4, ((DWORD*)PI)[1], ((DWORD*)PI)[0], data+1);
		if ((((DWORD*)PI)[1]+data) < (0x10000000+fsize)) {
			memcpy(returnMemPointer(((DWORD*)PI)[0]),returnMemPointer(((DWORD*)PI)[1]),data+1);
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
	case 0x00800004://SI PIF2DRAM
		DoPifEmulation ((u8*)returnMemPointer(ReadLong(0x04800000)), (u8*)returnMemPointer(0x1FC007C0));
		InterruptNeeded |= SI_INTERRUPT;
		SET_INTR(SI_INTERRUPT);
		pw32(0x04800018, 0x00001000);//SI_STATUS_REG = 0x00001000;
		return data; break;
	case 0x00800010://SI DRAM2PIF
		memcpy ((u8*)returnMemPointer(0x1FC007C0), (u8*)returnMemPointer(ReadLong(0x04800000)), 64);
		InterruptNeeded |= SI_INTERRUPT;
		SET_INTR(SI_INTERRUPT);
		pw32(0x04800018, 0x00001000);//SI_STATUS_REG = 0x00001000;
		return data; break;
	case 0x00800018://SI STATUS REG
		CLEAR_INTR(SI_INTERRUPT); 
		return 0; break;
	default: break;
	}
	return data;
}