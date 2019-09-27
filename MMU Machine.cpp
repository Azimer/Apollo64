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
 *		03-29-00	Initial Version (Andrew)
 *      05-16-00    Added InitMMU, added defines to clean up code (Andrew)
 *      06-29-00    Added more true exception code after giving up on Sunman. (Andrew)
 *
 **************************************************************************/
/**************************************************************************
 *
 *  Notes:
 *	
 *  -TLB translator needs to have exception handling put in. It is very
 *   primitive and does not run GoldenEye because of the lack thereof.
 *   It does run Mario and Starfox though. :) -KAH
 *  -When doing TlbException don't forget to differentiate 64bit vs 32bit -KAH
 *
 **************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include "WinMain.h"
#include "EmuMain.h"
#include "resource.h"
#include "videodll.h"
#pragma warning( disable : 4244 )

u32 VsyncTime = 625000;
u32 VsyncInterrupt=625000;
u32 CountInterrupt=-1;
u32 AiInterrupt=-1;
s_tlb tlb[32];
extern int inDelay;

#define CPU_ERROR(x,y) Debug(0,"%s %08X",x,y)

#define MMU_INDEX		MmuRegs[0]
#define MMU_RANDOM		MmuRegs[1]
#define MMU_ENTRYLO0	MmuRegs[2]
#define MMU_ENTRYLO1	MmuRegs[3]
#define MMU_CONTEXT		MmuRegs[4]
#define MMU_PAGEMASK	MmuRegs[5]
#define MMU_WIRED		MmuRegs[6]
#define MMU_BADVADDR	MmuRegs[8]
#define MMU_COUNT		MmuRegs[9]
#define MMU_ENTRYHI		MmuRegs[10]
#define MMU_COMPARE		MmuRegs[11]
#define MMU_SR			MmuRegs[12]
#define MMU_CAUSE		MmuRegs[13]
#define MMU_EPC			MmuRegs[14]
#define MMU_PRID		MmuRegs[15]
#define MMU_CONFIG		MmuRegs[16]
#define MMU_LLADDR		MmuRegs[17]
#define MMU_WATCHLO		MmuRegs[18]
#define MMU_WATCHHI		MmuRegs[19]
#define MMU_XCONTEXT	MmuRegs[20]
#define MMU_ECC			MmuRegs[26]
#define MMU_CACHEERR	MmuRegs[27]
#define MMU_TAGLO		MmuRegs[28]
#define MMU_TAGHI		MmuRegs[29]
#define MMU_ERROREPC	MmuRegs[30]

//Exceptions
#define INTERRUPT		0
#define TLB_MOD			1
#define TLB_LOAD		2
#define TLB_STORE		3
//4 5 6 7 are not going to be handled
#define SYSCALL			8
#define BREAK			9
//10 is not going to be handled
#define COP_UNUSEABLE	11
#define MATH_OVERFLOW	12
#define TRAP_EX			13
//14 is not going to be handled
#define FPU_EX			15
//and the remaining is not going to be handled

void InitMMU() {
	u32 i;
	for (i=0; i < 32; i++) {
		tlb[i].hh = tlb[i].hl = tlb[i].ll = tlb[i].lh = 0;
		tlb[i].isGlobal = 0;
	}
	instructions = 0;
	VsyncTime = VsyncInterrupt = 625000;
	MMU_SR     = 0x34000000;
	MMU_PRID   = 0x00000b00;
	MMU_CONFIG = 0x0006E463;
	MMU_RANDOM = 0x2F;
}

void opMFC0(void) { // Fixed sop.rt==1 to sop.rd==1
	extern u8* MI;
	MMU_COUNT = instructions;
	if (sop.rd==1) { MmuRegs[sop.rd] = (instructions % (47-((MMU_WIRED) & 0x3F))) + 47;}
	if (sop.rd==13 && MI[8]==0) { MMU_CAUSE &= 0xfffffbff;}//remove pin if there's no reason for it.
	CpuRegs[sop.rt] = MmuRegs[sop.rd];
}

void opMTC0(void) {
	if (sop.rd==8 || sop.rd==15) return;
	if (sop.rd==13) { MMU_CAUSE = (((u32)CpuRegs[sop.rt]) & 0x300) | (MMU_CAUSE & ~(0x300)); return;}
	if (sop.rd==11) {
		CountInterrupt=CpuRegs[sop.rt]-1;
		if (CountInterrupt > instructions)
			VsyncTime = MIN(MIN(CountInterrupt,AiInterrupt),VsyncInterrupt);
		else 
			CountInterrupt = -1;
		MMU_CAUSE &= 0xffff7fff;
	}
	MmuRegs[sop.rd] = (u32)CpuRegs[sop.rt];
}
void opERET(void){
	if (MMU_SR & 0x4)	{ pc = MMU_ERROREPC;	MMU_SR &= ~0x4;}
	else				{ pc = MMU_EPC;			MMU_SR &= ~0x2;}
	//MMU_SR |= 0x1; // STATUS_IE
}

void opTLBP(void){
	MMU_INDEX=0x80000000;
	for (int i=0; i < 32; i++) {
		if((tlb[i].hl&0xfffff000) == (MMU_ENTRYHI&0xfffff000)) {//assumes it is always global
			MMU_INDEX=i;
			return;
		}
	}
}

void opTLBR(void){
	int j = MMU_INDEX & 0x1f;
	MMU_PAGEMASK = tlb[j].hh;
	MMU_ENTRYHI = tlb[j].hl & (~tlb[j].hh);
	MMU_ENTRYLO0 = tlb[j].lh | tlb[j].isGlobal;
	MMU_ENTRYLO1 = tlb[j].ll | tlb[j].isGlobal;
}

void opTLBWI(void){
	int j = MMU_INDEX & 0x1f;
	tlb[j].hh = MMU_PAGEMASK;
	tlb[j].hl = MMU_ENTRYHI & (~MMU_PAGEMASK);
	tlb[j].lh = MMU_ENTRYLO0 & 0xFFFFFFFE;
	tlb[j].ll = MMU_ENTRYLO1 & 0xFFFFFFFE;
	tlb[j].isGlobal = (u8)(MMU_ENTRYLO0 & MMU_ENTRYLO1 & 0x1);
}

void opTLBWR(void){
	CPU_ERROR(L_STR(IDS_TLBWR_CALLED),opcode);
}

void opNI();

void (*MmuSpecial[0x40])() = {
    opNI,    opTLBR,  opTLBWI, opNI,    opNI,    opNI,    opTLBWR, opNI,
    opTLBP,  opNI,    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,
    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,
    opERET,  opNI,    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,
    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,
    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,
    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,
    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,    opNI
};

void (*MmuNormal[0x20])() = {
    opMFC0,	opNI,	opNI,	opNI,	opMTC0,	opNI,	opNI,	opNI,
    opNI,	opNI,	opNI,	opNI,	opNI,	opNI,	opNI,	opNI,
    opNI,	opNI,	opNI,	opNI,	opNI,	opNI,	opNI,	opNI,
    opNI,	opNI,	opNI,	opNI,	opNI,	opNI,	opNI,	opNI
};


//------------------------------------------------------------------------
//Interrupt/Exception Processing Section

void GenerateException(DWORD addr, DWORD type) {

	pc = 0x180;
	MMU_CAUSE = (MMU_CAUSE & 0x3000ff00) | (type << 2);
	if (!(MMU_SR & 2)) {
		MMU_EPC = addr;
		if (type==TLB_LOAD || type==TLB_STORE) pc = 0x80;//80 vs 0, porp to 64bit vs 32bit
	}
	MMU_SR |= 2;
	if (inDelay) MMU_CAUSE |= (1 << 31);
	if (MMU_SR & (1 << 22)) pc += 0xbfc00200;
	else pc += 0x80000000;
}

bool CopUnuseableException(u32 addr, u32 copx) {
	Debug (0, "COP%i is Unusable...", copx);
	MMU_CAUSE &= ~(3 << 28);
	MMU_CAUSE |= (copx << 28);
	GenerateException(addr,COP_UNUSEABLE);
	return true;
}

bool InterruptException(u32 addr, u32 pins) {
	//pins: 7 = COUNT, 2 = MI
	MMU_CAUSE |= 1 << (8+pins);
	if (!(MMU_SR & 1)) return false;//interrupts are disabled.
	if (!(MMU_SR & (1 << (8+pins)))) return false; //specific interrupt is disabled.
	GenerateException(addr,INTERRUPT);
	return true;
}
extern u8 *VI;
void ComputeFPS(void);
void HandleInterrupt(int type) {
	if (type & COUNT_INTERRUPT) {
		InterruptException(pc,7);
		Debug(0,L_STR(IDS_COUNT));
		Debug (0, "Instructions: %08X", instructions);
	}
	extern u8* MI;
	((u32*)MI)[2] |= (type & 0x3f);
	if (type & VI_INTERRUPT) {
		ComputeFPS();
		gfxdll.UpdateScreen();
	}
	if (!(((u32*)MI)[3] & (type & 0x3f))) return;//the interrupt(s) in question are not allowed yet.
	InterruptException(pc,2);
}

void CheckInterrupts() {
	if (++instructions == VsyncTime) {

		if (pc==0x800001C8) {
			CPU_ERROR(L_STR(IDS_CRC_FAILED),0);
			pc = 0x800001D0;
		}
		     if (VsyncTime==VsyncInterrupt) { VsyncInterrupt += 625000; InterruptNeeded |= VI_INTERRUPT;    }
		else if (VsyncTime==CountInterrupt) { CountInterrupt = -1;      InterruptNeeded |= COUNT_INTERRUPT; }
		else if (VsyncTime==AiInterrupt   ) { AiInterrupt    = -1;      SignalAiDone();                     }
		else {
			CPU_ERROR(L_STR(IDS_DELAY_DEAD),VsyncTime);
		}
		VsyncTime = MIN(MIN(CountInterrupt,AiInterrupt),VsyncInterrupt);
	}
	//if ((MMU_SR & 0x1) == 0) return;
	//if ((MMU_SR & 0x6) != 0) return;
	if (InterruptNeeded) { HandleInterrupt(InterruptNeeded); InterruptNeeded = 0; }

}

/*
  This function needs some serious work. This has no load exceptions, no store 
  exceptions, no tlb miss exceptions, and no tlb invalid exceptions...Shame on me. :)
*/
u32 VirtualToPhysical(u32 addr) {
	u32 oldAddr = addr;
	if ((addr & 0xC0000000)==0xC0000000) ;
	else if (addr & 0x80000000) { 
		return addr;
	}
	for (int i=0; i < 32; i++) {
		int page = ((tlb[i].hh >> 13) + 1) * 4096;
		int mask = ~(tlb[i].hh | 0x1fff);
		int maskLow = (tlb[i].hh | 0x1fff);
		if ((addr & mask) == (tlb[i].hl & mask)) {
			//match
			if (addr & page) {//odd page
				if (tlb[i].ll & 0x2) addr = 0x80000000 | ((tlb[i].ll << 6) & (mask >> 1)) | (addr & (maskLow >> 1));
			} else {//even page
				if (tlb[i].lh & 0x2) addr = 0x80000000 | ((tlb[i].lh << 6) & (mask >> 1)) | (addr & (maskLow >> 1));
			}
		}
	}
	return addr;
}
