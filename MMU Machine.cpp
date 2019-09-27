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
#include "CpuMain.h"
#include "resource.h"
#include "videodll.h"
#include "audiodll.h"
#pragma warning( disable : 4244 )

// These values work for MACE
// 625000
// 1


u32 VsyncTiming = 450000;//789000;//777809;
int NeedCount = 0;
//#define VsyncTiming 550000
//#define incrementer 3 // 777809 <-- LaC
//450000 - 2
//#define VsyncTiming 450000
//#define incrementer 2
//625000 - 4
// PAL - 937500
// NTSC - 781250

void GetNextInterrupt ();

u32 VsyncTime = VsyncTiming;
int InterruptTime;
u32 VsyncInterrupt = VsyncTime;
u32 CountInterrupt=-1;
u32 PiInterrupt=-1;
s_tlb tlb[0x40];
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
#define ADDR_LOAD		4
#define ADDR_STORE		5
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

int LLBit;

void InitMMU() {
	u32 i;
	for (i=0; i < 32; i++) {
		tlb[i].hh = tlb[i].ll = tlb[i].lh = 0;
		tlb[i].hl = 0x00000000;
		tlb[i].isGlobal = 0;
	}
	tlb[0].ll = tlb[0].lh = 3; // Needed for Blast Corps
	instructions = 0xA000000;   // Needed for WaveRace SE
	VsyncTiming = 425000;
	InterruptTime = VsyncTiming;
	VsyncTime = VsyncInterrupt = (VsyncTiming + instructions);

	MMU_SR     = 0x34000000;
	MMU_PRID   = 0x00000b00;
	MMU_CONFIG = 0x0006E463;
	MMU_RANDOM = 0x1F;
	LLBit = 0;
}

void opMFC0(void) { // Fixed sop.rt==1 to sop.rd==1
	extern u8* MI;
	instructions = (VsyncTime - InterruptTime);
	MMU_COUNT = instructions;
	//if (sop.rd==1) { MmuRegs[sop.rd] = (instructions % (47-((MMU_WIRED) & 0x3F))) + 47;}
	if (sop.rd==1) { MMU_RANDOM = instructions & 0x1F; if (MMU_RANDOM < MMU_WIRED) MMU_RANDOM = 0x1F; }
	if (sop.rd==13 && MI[8]==0) { MMU_CAUSE &= 0xfffffbff;}//remove pin if there's no reason for it.
	//if (sop.rd==14) Debug (0, "%08X: MFC0: EPC = %08X", pc, MMU_EPC);
	//Debug (0, "COUNT");
	CpuRegs[sop.rt] = (s32)MmuRegs[sop.rd];
}
/*
	case 12: //Status
		if ((CP0[Opcode.rd] ^ GPR[Opcode.rt].UW[0]) != 0) {
			CP0[Opcode.rd] = GPR[Opcode.rt].UW[0];
			SetFpuLocations();
		} else {
			CP0[Opcode.rd] = GPR[Opcode.rt].UW[0];
		}
		if ((CP0[Opcode.rd] & 0x18) != 0) { 
			DisplayError("Left kernel mode ??");
		}
		CheckInterrupts();
		break;		
*/
void SetFpuLocations ();
void opMTC0(void) {
	if (sop.rd==8 || sop.rd==15) return;
	if (sop.rd==13) { MMU_CAUSE = (((u32)CpuRegs[sop.rt]) & 0x300) | (MMU_CAUSE & ~(0x300)); return;}
	if (sop.rd==12) { // Status
		if (((MmuRegs[sop.rd] ^ (u32)CpuRegs[sop.rt])) != 0) {
			MmuRegs[sop.rd] = (u32)CpuRegs[sop.rt];
			SetFpuLocations (); // Setup the registers
		} else {
			//Debug (0, "Setting Status: %08X -> %08X", MmuRegs[sop.rd], (u32)CpuRegs[sop.rt]);
			MmuRegs[sop.rd] = (u32)CpuRegs[sop.rt];
		}

		//if ((MmuRegs[sop.rd] & 0x18) != 0)
		//	Debug (0, "Left Kernel Mode - This shouldn't happen?");
		return;
	}
	//if (sop.rd==14) Debug (0, "%08X: MTC0: EPC = %08X", pc, (u32)CpuRegs[sop.rt]);
	if (sop.rd==11) {
		CountInterrupt=CpuRegs[sop.rt]-1;
		//if (CountInterrupt > instructions) {
		NeedCount = 1;
		//Debug (0, "COUNT for %08X - %08X", instructions, CountInterrupt);
		//dprintf ("-");
//**************************************************************
		if (CountInterrupt == -1) {
			NeedCount = 0;
			return;
		}

		GetNextInterrupt ();
//			VsyncTime = MIN(MIN(CountInterrupt,PiInterrupt),VsyncInterrupt);
//**************************************************************
		//} else  {
		//	CountInterrupt = -1;
		//}
		MMU_CAUSE &= 0xffff7fff;
		//Debug (0, "Waiting for COUNT: %08X", CountInterrupt);
	}
	MmuRegs[sop.rd] = (u32)CpuRegs[sop.rt];
	//if (sop.rd == 14)
	//	Debug (0, "%08X: EPC = %08X", pc, MMU_EPC);
}
extern u8 *MI;
void opERET(void){
//	Debug (0, "ERET: PC = %08X  EPC = %08X", pc, MMU_EPC);
	if (MMU_SR & 0x4)	{ pc = MMU_ERROREPC;	MMU_SR &= ~0x4;}
	else				{ pc = MMU_EPC;			MMU_SR &= ~0x2;}
	if (pc == 0)
		__asm int 3;
	MMU_SR &= ~0x6;
	MMU_SR |= 0x1; // STATUS_IE
	LLBit = 0;
//	if ((((u32*)MI)[3] & (((u32*)MI)[2]))) {
//	else if ((((u32*)MI)[3] & (((u32*)MI)[2])) & 0x3F) { HandleInterrupt (0); }
	if (InterruptNeeded)
		CheckInterrupts();
//	}
}
void opTLBP(void);
void opTLBR(void);
void opTLBWI(void);
void opTLBWR(void);
#ifndef USE_NEW_TLB
void opTLBP(void){
	//Debug (0, "TLB Probe");
	MMU_INDEX|=0xFFFFFFFF80000000;
	for (int i=0; i < 32; i++) {
		u32 mask = ~(tlb[i].hh | 0x1fff);//& 0xffffe000;
		if((tlb[i].hl&mask) == (MMU_ENTRYHI&mask)) {//assumes it is always global
			MMU_INDEX=i;
			return;
		}
	}
}

void opTLBR(void){
	//Debug (0, "TLB Read");
	int j = MMU_INDEX & 0x1f;
	MMU_PAGEMASK = tlb[j].hh;
	MMU_ENTRYHI = tlb[j].hl & (~tlb[j].hh);
	MMU_ENTRYLO0 = tlb[j].lh | tlb[j].isGlobal;
	MMU_ENTRYLO1 = tlb[j].ll | tlb[j].isGlobal;
}

void opTLBWI(void){
	u32 j = MMU_INDEX & 0x1f;

/*	if (MMU_ENTRYHI != 0x80000000) {
		Debug (0, "TLB Write Index @ %08X", pc-4);
		Debug (0, "- INDEX    = %08X", MMU_INDEX);
		Debug (0, "- PAGEMASK = %08X", MMU_PAGEMASK);
		Debug (0, "- ENTRYHI  = %08X", MMU_ENTRYHI);
		Debug (0, "- ENTRYLO0 = %08X", MMU_ENTRYLO0);
		Debug (0, "- ENTRYLO1 = %08X", MMU_ENTRYLO1);
		Debug (0, "- G = %08X", (MMU_ENTRYLO0 & MMU_ENTRYLO1 & 0x1));
	}*/
	tlb[j].hh = MMU_PAGEMASK;
	tlb[j].hl = MMU_ENTRYHI & (~MMU_PAGEMASK);
	tlb[j].lh = MMU_ENTRYLO0 & 0xFFFFFFFE;
	tlb[j].ll = MMU_ENTRYLO1 & 0xFFFFFFFE;
	tlb[j].isGlobal = (u8)(MMU_ENTRYLO0 & MMU_ENTRYLO1 & 0x1);
}

void opTLBWR(void){
	u32 j = MMU_RANDOM & 0x1f;
	/*	Debug (0, "TLB Write Random @ %08X", pc-4);
		Debug (0, "- RANDOM    = %08X", MMU_RANDOM);
		Debug (0, "- PAGEMASK = %08X", MMU_PAGEMASK);
		Debug (0, "- ENTRYHI  = %08X", MMU_ENTRYHI);
		Debug (0, "- ENTRYLO0 = %08X", MMU_ENTRYLO0);
		Debug (0, "- ENTRYLO1 = %08X", MMU_ENTRYLO1);
		Debug (0, "- G = %08X", (MMU_ENTRYLO0 & MMU_ENTRYLO1 & 0x1));*/
	tlb[j].hh = MMU_PAGEMASK;
	tlb[j].hl = MMU_ENTRYHI & (~MMU_PAGEMASK);
	tlb[j].lh = MMU_ENTRYLO0 & 0xFFFFFFFE;
	tlb[j].ll = MMU_ENTRYLO1 & 0xFFFFFFFE;
	tlb[j].isGlobal = (u8)(MMU_ENTRYLO0 & MMU_ENTRYLO1 & 0x1);
}
#endif

void opNI();

void (*MmuSpecial[0x40])() = {
    opNI,    opTLBR,  opTLBWI, opNI,    opNI,    opNI,    opTLBWR, opNI, //0x00-0x07
    opTLBP,  opNI,    opNI,    opNI,    opNI,    opNI,    opNI,    opNI, //0x08-0x0F
    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,    opNI,    opNI, //0x10-0x17
    opERET,  opNI,    opNI,    opNI,    opNI,    opNI,    opNI,    opNI, //0x18
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

bool tlbinvalid = false; // Lets us know that the TLB is invalid so we don't go to the wrong addy

// Non-Interrupt Exceptions are Non-Maskable and take priority!

void GenerateException(DWORD addr, DWORD type) {

	//Debug (0, "Exception: %08X", addr);
	pc = 0x180;
	LLBit = 0;
	//Debug (0, "Exception: %08X", addr);
	MMU_CAUSE = (MMU_CAUSE & 0x3000ff00) | (type << 2);
	MMU_EPC = addr;
	
	if (type==TLB_LOAD || type==TLB_STORE) {
		if (tlbinvalid == false)
			pc = 0x0; //80 vs 0, porp to 64bit vs 32bit
		tlbinvalid = false;
		//Debug (0, "TLB Miss occurred: %X", pc);
	}

	MMU_SR |= 2;
	MMU_SR &= 0xFFFFFFFE;
	if (inDelay) {
		//Debug (0, "In Delay Slot");
		MMU_EPC -= 4;
		MMU_CAUSE |= (1 << 31);
	}
	if (MMU_SR & (1 << 22)) pc += 0xbfc00200;
	else pc += 0x80000000;
}

void SyscallException () {
	//Debug (0, "Syscall Exception");
	pc -= 4;
	GenerateException (pc, SYSCALL);
}

void BreakException () {
	Debug (0, "Break Exception");
	pc -= 4;
	GenerateException (pc, BREAK);
}

void AddressErrorException (bool store) {
	pc -= 4;
	if (store)
		GenerateException (pc, ADDR_STORE);
}

bool CopUnuseableException(u32 addr, u32 copx) {
	//Debug (0, "COP%i is Unusable...", copx);
	MMU_CAUSE &= ~(3 << 28);
	MMU_CAUSE |= (copx << 28);
	GenerateException(addr,COP_UNUSEABLE);
	return true;
}

bool InterruptException(u32 addr, u32 pins) {
	//pins: 7 = COUNT, 2 = MI
	MMU_CAUSE |= 1 << (8+pins);
	if (!(MMU_SR & 1)) { return false; }//interrupts are disabled.
	if (!(MMU_SR & (1 << (8+pins)))) return false; //specific interrupt is disabled.
	GenerateException(addr,INTERRUPT);
	return true;
}
extern u8 *VI;
void ComputeFPS(void);
void HandleInterrupt(int type) {
	if (type & COUNT_INTERRUPT) {
		if (NeedCount == 0) {
			dprintf ("*");
			return;
		}
		NeedCount = 0;
		InterruptException(pc,7);
	}
	extern u8* MI;
	((u32*)MI)[2] |= (type & 0x3f);
	if (!(((u32*)MI)[3] & (((u32*)MI)[2] & 0x3f))) {
		return;//the interrupt(s) in question are not allowed yet.
	}
	InterruptException(pc,2);
}

/*

[17:21] <Azimer> Are interrupts disabled when an exception occurs?
[17:23] <zilmar> yes 
[17:23] <zilmar> it changes the lower bit to 0
[17:23] <zilmar> and eret sets it back to 1
[17:23] <zilmar> and when that bit is off .. then  interrupts do not occur
[17:24] <zilmar> void TestInterrupts ( void ) {
[17:24] <zilmar> 	if ((MI_INTR_MASK_REG.UW & MI_INTR_REG.UW) != 0) {
[17:24] <zilmar> 		CAUSE_REGISTER.UW |= CAUSE_IP2;
[17:24] <zilmar> 	} else  {
[17:24] <zilmar> 		CAUSE_REGISTER.UW &= ~CAUSE_IP2;
[17:24] <zilmar> 	}
[17:24] <zilmar> 	if (( STATUS_REGISTER.UW & STATUS_IE   ) == 0 ) { return; }
[17:24] <zilmar> 	if (( STATUS_REGISTER.UW & STATUS_EXL  ) != 0 ) { return; }
[17:24] <zilmar> 	if (( STATUS_REGISTER.UW & STATUS_ERL  ) != 0 ) { return; }
[17:24] <zilmar> 	if (( STATUS_REGISTER.UW & CAUSE_REGISTER.UW & 0xFF00) != 0) {
[17:24] <zilmar> 		DoException(EXC_INT,0);
[17:24] <zilmar> 	}
[17:24] <zilmar> }

*/

extern u32 *DVI;
extern u8 *DPC;
u32 ExecuteEvent ();
void DisassembleRange (u32, u32);

u32 t;

u32 *synchack = &t;

void GetNextInterrupt () {
	u32 t0 = VsyncInterrupt - instructions - 1;
	u32 t1 = PiInterrupt - instructions - 1;

	if (t0 >= t1)  {
		u32 t2 = CountInterrupt - instructions - 1;
		if (t2 >= t1) {
			VsyncTime = PiInterrupt;
			InterruptTime = t1;
		} else {
			VsyncTime = CountInterrupt;
			InterruptTime = t2;
		}
	} else {
		u32 t2 = CountInterrupt - instructions - 1;
		if (t2 >= t0) {
			VsyncTime = VsyncInterrupt;
			InterruptTime = t0;
		} else {
			VsyncTime = CountInterrupt;
			InterruptTime = t2;
		}
	}
}

void CheckInterrupts() {
	/*Debug (2, "pc = %08X", pc);*/
	//if (pc == 0x80201068) {
		//DisassembleRange (pc-0x1000, pc+0x1000);
	//	__asm int 3;
	//}//*/
	if (InterruptTime <= 0) {
		instructions = (VsyncTime - InterruptTime);
		MMU_COUNT = instructions;
		if (pc==0x800001C8) {
			CPU_ERROR(L_STR(IDS_CRC_FAILED),0);
			pc = 0x800001D0;
		}
		     if (VsyncTime==VsyncInterrupt) { VsyncInterrupt += VsyncTiming; InterruptNeeded |= VI_INTERRUPT;    }
		else if (VsyncTime==PiInterrupt   ) { PiInterrupt    = -1;      InterruptNeeded |= ExecuteEvent (); /*Debug (0, "SI Interrupt! %08X", instructions);*/}
		else if (VsyncTime==CountInterrupt) { CountInterrupt = -1;      InterruptNeeded |= COUNT_INTERRUPT; }
		else {
			__asm int 3;
			//CPU_ERROR(L_STR(IDS_DELAY_DEAD),VsyncTime);
		}
		if (InterruptNeeded & VI_INTERRUPT) {
			//if (RegSettings.enableConsoleWindow) {
				extern u32 dlists, alists;
				extern u32 ailens;
//				char buff[255];
				//sprintf (buff, "DL/s: %i  AL/s: %i  AI/s: %i", dlists, alists, ailens);
				//sprintf (buff, "pc: 0x%08X  Dlists: %i  Alists: %i", pc, dlists, alists);

				//SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) buff);
			//}
			ComputeFPS();
			gfxdll.UpdateScreen();
		}

		GetNextInterrupt ();
		//if (VsyncTime == 0xFFFFFFFF)
		//	VsyncTime = 0;
	}
	if ((MMU_SR & 0x1) == 0) return;
	if ((MMU_SR & 0x6) != 0) return;
	extern u8 *MI;
	if (InterruptNeeded) { HandleInterrupt(InterruptNeeded); InterruptNeeded = 0; }
	else if ((((u32*)MI)[3] & (((u32*)MI)[2])) & 0x3F) { HandleInterrupt (0); }
}


#ifndef USE_NEW_TLB
u32 VirtualToPhysical(u32 addr, int type) {
	u32 oldAddr = addr;
	bool match = false;

//	__asm int 3;
/*
	if ((addr & 0xC0000000)==0xC0000000) ;
	else if (addr & 0x80000000) { 
		return addr;
	}
*/
	for (int i=31; i >= 0; i--) {
		int page = ((tlb[i].hh >> 13) + 1) * 4096;
		int mask = ~(tlb[i].hh | 0x1fff);
		int maskLow = (tlb[i].hh | 0x1fff);

		if ((addr & mask) == (tlb[i].hl & mask)) {
			//match
			match = true;
				if (addr & page) {//odd page
					if (tlb[i].ll & 0x2) 
						addr = 0x80000000 | ((tlb[i].ll << 6) & (mask >> 1)) | (addr & (maskLow >> 1));
					else {
						match = false;
						tlbinvalid = true;
						//Debug (0, "%08X: TLB Invalid Exception %08X", pc, addr); 
					}
				} else {//even page
					if (tlb[i].lh & 0x2) {
						addr = 0x80000000 | ((tlb[i].lh << 6) & (mask >> 1)) | (addr & (maskLow >> 1));
					} else {
						match = false;
						tlbinvalid = true;
						//Debug (0, "%08X: TLB Invalid Exception %08X", pc, addr); 
					}
				}
			break;
		}
	}
	
	
	if (match == false) { // TLB Hacks galore!!!
		//if (type)
		//	Debug(0,"TLB Store Miss at %08X, pc:%08X, exception!",addr,pc-4);
		//else
		//	Debug(0,"TLB Load Miss at %08X, pc:%08X, exception!",addr,pc-4);

		/*for (int i = MMU_WIRED; i < 32; i++) {
			if (tlb[i].hl == 0x80000000)
				break;
			if ((tlb[i].ll & tlb[i].lh & 2)==0)
				break;
		}

		if (i < 32) {
			MMU_RANDOM = i;
		} else*/ {
			MMU_RANDOM = instructions & 0x1F;
			if (MMU_RANDOM < MMU_WIRED)
				MMU_RANDOM = 0x1F;
		}

		//Debug (0, "MMU_RANDOM = %i", MMU_RANDOM);

		MMU_CONTEXT &= 0xFF800000;
		MMU_CONTEXT |= (addr >> 13) << 4;
		MMU_ENTRYHI &= 0x00001FFF;
		MMU_ENTRYHI = (addr & 0xFFFFE000);
		MMU_BADVADDR = addr;

		if (pc != addr)
			pc -= 4;

		//if (inDelay == 1)
		//	Debug (0, "TLB Exception in Delay Slot!");
		if (type)
			GenerateException(pc, TLB_STORE);
		else
			GenerateException(pc, TLB_LOAD);

		//MMU_INDEX = 0;
		//Emulate ();
		RaiseException (0xc0001337, 0,0, NULL); // TLB Exception
		return 0;
	}
	return addr;
}
#endif
