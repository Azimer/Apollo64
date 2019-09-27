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
 *		??-??-??	Initial Version (Andrew)
 *      06-02-00    Revised MULT and SRAV (Andrew)
 *		06-19-00	Rewrote some of the emulate code
 *
 **************************************************************************/
/**************************************************************************
 *
 *  Notes:
 *	
 *	-MULT and SRAV use 64bit mult/shifts when it is not necessary. This
 *	 causes a slowdown in emulation, but not innacurate emulation.
 *
 **************************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h> // For status window
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <math.h>
#include "WinMain.h"
#include "EmuMain.h"
#include "resource.h"
#pragma warning( disable : 4244 )

#define CPU_ERROR(x,y) Debug(0,"%s %08X",x,y)

#define BREAKPOINT 0xFFFFFFF

#define FAST_INFINITE_LOOP

#ifdef FAST_INFINITE_LOOP
	#define FAST_INFINITE_LOOP_JUMP   extern u32 VsyncTime,instructions; if (target==pc-4 && pr32(pc)==0) instructions = VsyncTime-1;
	#define FAST_INFINITE_LOOP_BRANCH extern u32 VsyncTime,instructions; if (target==pc-4 && pr32(pc)==0 && sop.rt==sop.rs) instructions = VsyncTime-1;
#else
	#define FAST_INFINITE_LOOP_JUMP //
	#define FAST_INFINITE_LOOP_BRANCH
#endif

u32 *myPC;

FILE *instr = NULL;

extern u32 VsyncTime;
extern u32 VsyncInterrupt;
u64 CpuRegs[32];
u64 MmuRegs[32];
R4K_FPU_union FpuRegs;
u32 FpuControl[32];
s64 cpuHi;
s64 cpuLo;
u32 instructions = 0;
volatile u32 pc;
volatile char InterruptNeeded=0;
s_sop sop;

extern void (*r4300i[0x40])();
extern void (*special[0x40])();
extern void (*regimm[0x20])();
extern void (*MmuSpecial[0x40])();
extern void (*MmuNormal[0x20])();
extern void (*FpuCommands[0x40])();
extern u32 valloc; // Needed for opcode fetcher
int inDelay=0; // In a delay slot
		   
void doInstr(void) {
		((u32*)&sop)[0] = opcode = vr32(pc);
		r4300i[sop.op]();
}

extern u8 *pif;
unsigned long GenerateCRC (unsigned char *data, int size);

void ResetCPU (void) {
	void InitMMU();
	memcpy(idmem, RomMemory, 0x1000);
	memset(&CpuRegs, 0, sizeof(CpuRegs));
	memset(&MmuRegs, 0, sizeof(MmuRegs));
	memset(&FpuRegs, 0, sizeof(FpuRegs));
	memset(&FpuControl, 0, sizeof(FpuControl));
	DWORD bootcode = GenerateCRC (idmem+0x40, 0x1000-0x40);
	InitMMU();
	pc = 0xA4000040;
	CpuRegs[0x1D] = (s32)0xA4001FF0;    // SP
	CpuRegs[0x16] = 0x3F;          // S6
	CpuRegs[0x14] = 1;             // S4
	CpuRegs[0x1F] = (s32)0x80000000;
	switch (bootcode) {
		case 0xB9C47DC8: // CIC-NUS-6102
			/* CIC-NUS #2 */
			CpuRegs[22] = (u64)0x000000000000003F;
			Debug (0, "CIC-NUS-6102 Detected...");
		break;

		case 0xEDCE5AD9: // CIC-NUS-6103
			CpuRegs[22] = (u64)0x0000000000000078;
			Debug (0, "CIC-NUS-6103 BootCode Detected...");
		break;

		case 0xB53C588A: // CIC-NUS-6105
			CpuRegs[22] = (u64)0x0000000000000091;
			Debug (0, "CIC-NUS-6105 Detected...");
		break;
		default:
			Debug (1, "Unknown Bootcode... Please e-mail Azimer with the following Information: \r\nCRC: %X, RomTitle: %s", bootcode, rInfo.InternalName);
			cpuIsReset = true;
			cpuIsDone = true;
	}
	
	cpuHi = cpuLo = 0;
	SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) L_STR(IDS_EMU_STARTED));
}

void Emulate(void) {

		while (!cpuIsReset) {
			CheckInterrupts();

			((u32*)&sop)[0] = vr32(pc);

			pc+=4;
			r4300i[sop.op]();

			if (CpuRegs[0] != 0) {
				Debug (0, "CpuRegs[0] != 0, PC = %08X", pc-4);
				cpuIsDone = true;
				cpuIsReset = true;
			}
		}
}

void opCOP0(void) {
	if (sop.rs == 16) MmuSpecial[sop.func]();
	else MmuNormal[sop.rs]();
}

void opCOP2(void) {
	bool CopUnuseableException(u32 addr, u32 copx);
	CopUnuseableException(pc-4,2);
}

void opNI(void) {
	Debug(0,"%08X: NI called. op%02X func%02X rs%02X",pc-4,sop.op,sop.func,sop.rs);
	CPU_ERROR("NI Called",pc);
	cpuIsDone = cpuIsReset = true;
}

void opSPECIAL ( void ) {
	special[sop.func]();
}

void opREGIMM( void ) {
	regimm[sop.rt]();
}

void opADD(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = (s32)(CpuRegs[sop.rt] + CpuRegs[sop.rs]);
	// TODO: Integer Overflow...
}

void opADDI(void) { // Last Checked 01-14-01
	CpuRegs[sop.rt] = (s32)(CpuRegs[sop.rs] + ((s16)opcode));
	// TODO: Integer Overflow...
}

void opADDIU(void) { // Last Checked 01-14-01
	CpuRegs[sop.rt] = (s32)(CpuRegs[sop.rs] + ((s16)opcode));
}

void opADDU(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = (s32)(CpuRegs[sop.rt] + CpuRegs[sop.rs]);
}

void opAND(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = CpuRegs[sop.rt] & CpuRegs[sop.rs];
}

void opANDI(void) { // Last Checked 01-14-01
	CpuRegs[sop.rt] = CpuRegs[sop.rs] & ((u16)opcode);
}

void opBEQ(void) { // Last Checked 01-14-01
	if (CpuRegs[sop.rt] == CpuRegs[sop.rs])	{
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		FAST_INFINITE_LOOP_BRANCH
		doInstr();
		pc = target;
	}
}

void opBEQL(void) { // Last Checked 01-14-01
	if (CpuRegs[sop.rt] == CpuRegs[sop.rs]) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		FAST_INFINITE_LOOP_BRANCH
		doInstr();
		pc = target;
	} else {
		pc +=4;
	}
}

void opBGEZ(void) { // Last Checked 01-14-01
	if (((s64)CpuRegs[sop.rs]) >= 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		pc = target;
	}
}

void opBGEZAL(void) { // Last Checked 01-14-01
	if (((s64)CpuRegs[sop.rs]) >= 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		pc = target;
	}
	CpuRegs[31] = pc+4;
}

void opBGEZALL(void) { // Last Checked 01-14-01
	if (((s64)CpuRegs[sop.rs]) >= 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		pc = target;
	}else{
		pc+=4;
	}
	CpuRegs[31] = pc+4;
}

void opBGEZL(void) { // Last Checked 01-14-01
	if (((s64)CpuRegs[sop.rs]) >= 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		pc = target;
	}else{
		pc+=4;
	}
}

void opBGTZ(void) { // Last Checked 01-14-01
	if (((s64)CpuRegs[sop.rs]) > 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		pc = target;
	}
}

void opBGTZL(void) {
	if (((s64)CpuRegs[sop.rs]) > 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		pc = target;
	}else{
		pc+=4;
	}
}

void opBLEZ(void) { // Last Checked 01-14-01
	if (((s64)CpuRegs[sop.rs]) <= 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		pc = target;
	}
}

void opBLEZL(void) { // Last Checked 01-14-01
	if (((s64)CpuRegs[sop.rs]) <= 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		pc = target;
	}else{
		pc+=4;
	}
}

void opBLTZ(void) { // Last Checked 01-14-01
	if (((s64)CpuRegs[sop.rs]) < 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		pc = target;
	}
}

void opBLTZAL(void) { // Last Checked 01-14-01
	if (((s64)CpuRegs[sop.rs]) < 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		pc = target;
	}
	CpuRegs[31] = pc+4;
}

void opBLTZALL(void) { // Last Checked 01-14-01
	if (((s64)CpuRegs[sop.rs]) < 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		pc = target;
	}else{
		pc+=4;
	}
	CpuRegs[31] = pc+4;
}

void opBLTZL(void) { // Last Checked 01-14-01
	if (((s64)CpuRegs[sop.rs]) < 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		pc = target;
	}else{
		pc+=4;
	}
}

void opBNE(void) { // Last Checked 12-28-00
	if (CpuRegs[sop.rt] != CpuRegs[sop.rs]) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		pc = target;
	}
}

void opBNEL(void) { // Last Checked 01-14-01
	if (CpuRegs[sop.rt] != CpuRegs[sop.rs]) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		pc = target;
	} else{
		pc+=4;
	}
}

void opBREAK(void) {
	Debug(0,"BREAK opcode at %X, A2:%X, VO:%X\n",pc,CpuRegs[6],CpuRegs[2]);
	CPU_ERROR("Break Called",pc);
}

void opCACHE(void) { } // TODO: Do I need to do anything with this instruction? 12-28-00

void opDADD(void) {
	CpuRegs[sop.rd] = CpuRegs[sop.rt] + CpuRegs[sop.rs];
}

void opDADDI(void) {
	CpuRegs[sop.rt] = CpuRegs[sop.rs] + ((s16)opcode);
}

void opDADDIU(void) {
	CpuRegs[sop.rt] = CpuRegs[sop.rs] + ((s16)opcode);
}

void opDADDU(void) {
	CpuRegs[sop.rd] = CpuRegs[sop.rt] + CpuRegs[sop.rs];
}

void opDDIV(void) {
	if (CpuRegs[sop.rt] == 0) {
		cpuLo = cpuHi = 0;
	} else {
		cpuLo = (s64)CpuRegs[sop.rs] / (s64)CpuRegs[sop.rt];
		cpuHi = (s64)CpuRegs[sop.rs] % (s64)CpuRegs[sop.rt];
	}
}

void opDDIVU(void) {
	if (CpuRegs[sop.rt] == 0) {
		cpuLo = cpuHi = 0;
	} else {
		cpuLo = (u64)CpuRegs[sop.rs] / (u64)CpuRegs[sop.rt];
		cpuHi = (u64)CpuRegs[sop.rs] % (u64)CpuRegs[sop.rt];
	}
}

void opDIV(void) {
	if (CpuRegs[sop.rt] == 0) {
		cpuLo = cpuHi = 0;
	} else {
		cpuLo = ((s32)CpuRegs[sop.rs]) / ((s32)CpuRegs[sop.rt]);
		cpuHi = ((s32)CpuRegs[sop.rs]) % ((s32)CpuRegs[sop.rt]);
	}
}

void opDIVU(void) {
	if (CpuRegs[sop.rt] == 0) {
		cpuLo = cpuHi = 0;
	} else {
		cpuLo = ((u32)CpuRegs[sop.rs]) / ((u32)CpuRegs[sop.rt]);
		cpuHi = ((u32)CpuRegs[sop.rs]) % ((u32)CpuRegs[sop.rt]);
	}
}

#define MULT_X86(src1,src2) \
	__asm mov eax, [src1] \
	__asm mov edx, [src2] \
	__asm mul edx 

#define ADD_CARRY_X86(dest1,dest2) \
	__asm add dword ptr [dest1+4],eax \
	__asm adc dword ptr [dest2+4],edx

#define STORE_X86(dest) \
	__asm mov dword ptr [dest],eax \
	__asm mov dword ptr [dest+4],edx


void opDMULT(void) {

	int neg_sign = 0;
	s64 v1 = CpuRegs[sop.rs];
	s64 v2 = CpuRegs[sop.rt];
	if (v1<0) { v1 = -v1; neg_sign = 1; }
	if (v2<0) { v2 = -v2; neg_sign = ! neg_sign; }
	u32 a = (u32)(v1>>32);
	u32 b = (u32)v1;
	u32 c = (u32)(v2>>32);
	u32 d = (u32)v2;
	MULT_X86(b,d)
	STORE_X86(cpuLo)
	MULT_X86(a,c)
	STORE_X86(cpuHi)
	MULT_X86(b,c)
	ADD_CARRY_X86(cpuLo,cpuHi)
	MULT_X86(a,d)
	ADD_CARRY_X86(cpuLo,cpuHi)

	if (neg_sign) { cpuLo = ~cpuLo; cpuHi = ~cpuHi; cpuLo +=1; if (cpuLo == 0) cpuHi+=1; }	
}

void opDMULTU(void) {
	u32 a = (u32)(CpuRegs[sop.rs]>>32);
	u32 b = (u32)CpuRegs[sop.rs];
	u32 c = (u32)(CpuRegs[sop.rt]>>32);
	u32 d = (u32)CpuRegs[sop.rt];
	MULT_X86(b,d)
	STORE_X86(cpuLo)
	MULT_X86(a,c)
	STORE_X86(cpuHi)
	MULT_X86(b,c)
	ADD_CARRY_X86(cpuLo,cpuHi)
	MULT_X86(a,d)
	ADD_CARRY_X86(cpuLo,cpuHi)

}

void opDSLL(void) {
	//if (opcode == 0) return;
	CpuRegs[sop.rd] = CpuRegs[sop.rt] << sop.sa;
}

void opDSLLV(void) {
	CpuRegs[sop.rd] = CpuRegs[sop.rt] << (CpuRegs[sop.rs] & 0x3f);
}

void opDSLL32(void) {
	CpuRegs[sop.rd] = CpuRegs[sop.rt] << (sop.sa+32);
}

void opDSRA(void) {
	CpuRegs[sop.rd] = ((s64)CpuRegs[sop.rt]) >> sop.sa;
}

void opDSRAV(void) {
	CpuRegs[sop.rd] = ((s64)CpuRegs[sop.rt]) >> (CpuRegs[sop.rs] & 0x3f);
}

void opDSRA32(void) {
	CpuRegs[sop.rd] = ((s64)CpuRegs[sop.rt]) >> (sop.sa+32);
}

void opDSRL(void) {
	CpuRegs[sop.rd] = CpuRegs[sop.rt] >> sop.sa;
}

void opDSRLV(void) {
	CpuRegs[sop.rd] = CpuRegs[sop.rt] >> (CpuRegs[sop.rs] & 0x3f);
}

void opDSRL32(void) {
	CpuRegs[sop.rd] = CpuRegs[sop.rt] >> (sop.sa+32);
}

void opDSUB(void) {
	CpuRegs[sop.rd] = CpuRegs[sop.rs] - CpuRegs[sop.rt];
}

void opDSUBU(void) {
	CpuRegs[sop.rd] = CpuRegs[sop.rs] - CpuRegs[sop.rt];
}

void opJ(void) { // Last Checked 01-14-01
	u32 target = (pc & 0xf0000000) + ((opcode << 2) & 0x0fffffff);
	FAST_INFINITE_LOOP_JUMP
	doInstr();
	pc = target;
}

void opJAL(void) { // Last Checked 01-14-01
	u32 target = (pc & 0xf0000000) + ((opcode << 2) & 0x0fffffff);
	CpuRegs[31] = pc+4;
	doInstr();
	pc = target;
}

void opJALR(void) { // Last Checked 01-14-01
	u32 target = CpuRegs[sop.rs];
	CpuRegs[sop.rd] = pc+4;
	doInstr();
	pc = target;
}

void opJR(void) { // Last Checked 01-14-01
	u32 target = CpuRegs[sop.rs];
	doInstr();
	pc = target;
}

void opLB(void) {
	CpuRegs[sop.rt] = (s8)(vr8(CpuRegs[sop.rs] + (s16)opcode));
}

void opLBU(void) { // Last Checked 01-14-01
	CpuRegs[sop.rt] = (u8)(vr8(CpuRegs[sop.rs] + (s16)opcode));
}

void opLD(void) {
	CpuRegs[sop.rt] = (vr64(CpuRegs[sop.rs] + (s16)opcode));
}

void opLDC1(void) {
	FpuRegs.l[sop.rt/2] = vr64(CpuRegs[sop.rs] + (s16)opcode);
}

void opLDC2(void) {
	bool CopUnuseableException(u32 addr, u32 copx);
	CopUnuseableException(pc-4,2);
}

void opLDL(void) {
	u32 addr = CpuRegs[sop.rs] + (s16)opcode;
	u32 lwTmp = vr32(addr&0xfffffffc);
	switch (addr&3) {
	case 0: break;
	case 1:  lwTmp  = (lwTmp << 8 ) | ((u32)CpuRegs[sop.rt]&0x00000000000000ff); break;
	case 2:  lwTmp  = (lwTmp << 16) | ((u32)CpuRegs[sop.rt]&0x000000000000ffff); break;
	case 3:  lwTmp  = (lwTmp << 24) | ((u32)CpuRegs[sop.rt]&0x0000000000ffffff); break;
	case 4:  lwTmp  = (lwTmp << 32) | ((u32)CpuRegs[sop.rt]&0x00000000ffffffff); break;
	case 5:  lwTmp  = (lwTmp << 40) | ((u32)CpuRegs[sop.rt]&0x000000ffffffffff); break;
	case 6:  lwTmp  = (lwTmp << 48) | ((u32)CpuRegs[sop.rt]&0x0000ffffffffffff); break;
	default: lwTmp  = (lwTmp << 56) | ((u32)CpuRegs[sop.rt]&0x00ffffffffffffff); break;
	}
	CpuRegs[sop.rt] = (s32)lwTmp;
}

void opLDR(void) {
	u32 addr = CpuRegs[sop.rs] + (s16)opcode;
	u32 lwTmp = vr32(addr&0xfffffffc);
	switch (addr&3) {
	case 7:  break;
	case 6:  lwTmp  = (lwTmp >> 8 ) | ((u32)CpuRegs[sop.rt]&0xff00000000000000); break;
	case 5:  lwTmp  = (lwTmp >> 16) | ((u32)CpuRegs[sop.rt]&0xffff000000000000); break;
	case 4:  lwTmp  = (lwTmp >> 24) | ((u32)CpuRegs[sop.rt]&0xffffff0000000000); break;
	case 3:  lwTmp  = (lwTmp >> 32) | ((u32)CpuRegs[sop.rt]&0xffffffff00000000); break;
	case 2:  lwTmp  = (lwTmp >> 40) | ((u32)CpuRegs[sop.rt]&0xffffffffff000000); break;
	case 1:  lwTmp  = (lwTmp >> 48) | ((u32)CpuRegs[sop.rt]&0xffffffffffff0000); break;
	default: lwTmp  = (lwTmp >> 56) | ((u32)CpuRegs[sop.rt]&0xffffffffffffff00); break;
	}
	CpuRegs[sop.rt] = (s32)lwTmp;
}

void opLH(void) { // Last Checked 01-14-01
	CpuRegs[sop.rt] = (s16)(vr16(CpuRegs[sop.rs] + (s16)opcode));
	// TODO: Address Error Exception
}

void opLHU(void) {
	CpuRegs[sop.rt] = (s64)(u16)(vr16(CpuRegs[sop.rs] + (s16)opcode));
}


void opLL(void) {
	CPU_ERROR("LL Called",pc);
}

void opLLD(void) {
	CPU_ERROR("LLD Called",pc);
}

void opLUI(void) { // Last Checked 01-14-01
	CpuRegs[sop.rt] = (s32)((u16)opcode << 16);
}

void opLW(void) { // Last Checked 01-14-01
	CpuRegs[sop.rt] = (s32)(vr32(CpuRegs[sop.rs] + (s16)opcode));
	// TODO: Address Error Exception...
}

void opLWC1(void) {
	FpuRegs.w[sop.rt] = vr32(CpuRegs[sop.rs] + (s16)opcode);
}

void opLWC2(void) {
	bool CopUnuseableException(u32 addr, u32 copx);
	CopUnuseableException(pc-4,2);
}

void opLWL(void) { // Last Checked 12-28-00
	u32 addr = CpuRegs[sop.rs] + (s16)opcode;
	u32 lwTmp = vr32(addr&0xfffffffc);
	switch (addr&3) {
	case 0:  break;
	case 1:  lwTmp  = (lwTmp << 8 ) | ((u32)CpuRegs[sop.rt]&0x000000ff); break;
	case 2:  lwTmp  = (lwTmp << 16) | ((u32)CpuRegs[sop.rt]&0x0000ffff); break;
	default: lwTmp  = (lwTmp << 24) | ((u32)CpuRegs[sop.rt]&0x00ffffff); break;
	}
	CpuRegs[sop.rt] = (s32)lwTmp;
	//Debug (0, "LWL: Address: %08X Data: %08X", addr, lwTmp);
}

void opLWR(void) { // Last Checked 12-28-00
	u32 addr = CpuRegs[sop.rs] + (s16)opcode;
	u32 lwTmp = vr32(addr&0xfffffffc);
	switch (addr&3) {
	case 3:  break;
	case 2:  lwTmp  = (lwTmp >> 8 ) | ((u32)CpuRegs[sop.rt]&0xff000000); break;
	case 1:  lwTmp  = (lwTmp >> 16) | ((u32)CpuRegs[sop.rt]&0xffff0000); break;
	default: lwTmp  = (lwTmp >> 24) | ((u32)CpuRegs[sop.rt]&0xffffff00); break;
	}
	CpuRegs[sop.rt] = (s32)lwTmp;
	//Debug (0, "LWR: Address: %08X Data: %08X", addr, lwTmp);
}

void opLWU(void) {
	CpuRegs[sop.rt] = (u32)(vr32(CpuRegs[sop.rs] + (s16)opcode));//unrisky
}

void opMFHI(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = cpuHi;
}

void opMFLO(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = cpuLo;
}
// NOTES: Changed sop.rs to sop.rt
void opMTHI(void) { // Last Checked 01-14-01
	cpuHi = CpuRegs[sop.rs];
}
// NOTES: Changed sop.rs to sop.rt
void opMTLO(void) { // Last Checked 01-14-01
	cpuLo = CpuRegs[sop.rs];
}

void opMULT(void) { // Last Checked 01-14-01 : Not needed for 1080
	//cpuLo = Int32x32To64((u32)CpuRegs[sop.rs],(u32)CpuRegs[sop.rt]);
	//cpuHi = (s32)(((s32*)&cpuLo)[1]);
	//cpuLo = (s32)(((s32*)&cpuLo)[0]);
	cpuLo = (s64)(s32)CpuRegs[sop.rs] * (s64)(s32)CpuRegs[sop.rt];
	cpuHi = (s32)(cpuLo >> 32);
	cpuLo = (s32)cpuLo;
}

void opMULTU(void) { // Last Checked 01-14-01
	/*
	cpuLo = UInt32x32To64((u32)CpuRegs[sop.rs],(u32)CpuRegs[sop.rt]);
	cpuHi = (s32)(((s32*)&cpuLo)[1]);
	cpuLo = (s32)(((s32*)&cpuLo)[0]);
	*/
    //u64 result;

	cpuLo = (u64)(u32)CpuRegs[sop.rs] * (u64)(u32)CpuRegs[sop.rt];
	cpuHi = (s32)(cpuLo >> 32);
	cpuLo = (s32)cpuLo;
}

void opNOR(void) {
	CpuRegs[sop.rd] = ~(CpuRegs[sop.rs] | CpuRegs[sop.rt]);
}

void opOR(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = CpuRegs[sop.rs] | CpuRegs[sop.rt];
}

void opORI(void) { // Last Checked 01-14-01
	CpuRegs[sop.rt] = CpuRegs[sop.rs] | ((u16)opcode);
}

void opSB(void) { // Last Checked 01-14-01
	vw8(CpuRegs[sop.rs] + (s16)opcode,(u8)CpuRegs[sop.rt]);
	// TODO: Address Error Exception
}

void opSC(void) {
	CPU_ERROR("SC Called",pc);
}

void opSCD(void) {
	CPU_ERROR("SCD Called",pc);
}

void opSD(void) {
	vw64(CpuRegs[sop.rs] + (s16)opcode,CpuRegs[sop.rt]);
}

void opSDC1(void) {
	vw64(CpuRegs[sop.rs] + (s16)opcode,FpuRegs.l[sop.rt/2]);
}

void opSDC2(void) {
	bool CopUnuseableException(u32 addr, u32 copx);
	CopUnuseableException(pc-4,2);
}

void opSDL(void) {
	u32 addr = CpuRegs[sop.rs] + (s16)opcode;
	u64 lwTmp = vr64(addr&0xfffffff8);
	switch (addr&7) {
	case 0: lwTmp = CpuRegs[sop.rt]; break;
	case 1: lwTmp  = ((u64)CpuRegs[sop.rt] >> 8) | (lwTmp&0xff00000000000000); break;
	case 2: lwTmp  = ((u64)CpuRegs[sop.rt] >> 16) | (lwTmp&0xffff000000000000); break;
	case 3: lwTmp  = ((u64)CpuRegs[sop.rt] >> 24) | (lwTmp&0xffffff0000000000); break;
	case 4: lwTmp  = ((u64)CpuRegs[sop.rt] >> 32) | (lwTmp&0xffffffff00000000); break;
	case 5: lwTmp  = ((u64)CpuRegs[sop.rt] >> 40) | (lwTmp&0xffffffffff000000); break;
	case 6: lwTmp  = ((u64)CpuRegs[sop.rt] >> 48) | (lwTmp&0xffffffffffff0000); break;
	default: lwTmp  = ((u64)CpuRegs[sop.rt] >> 56) | (lwTmp&0xffffffffffffff00); break;
	}
	vw64(addr&0xfffffff8,lwTmp);
}

void opSDR(void) {
	u32 addr = CpuRegs[sop.rs] + (s16)opcode;
	u64 lwTmp = vr64(addr&0xfffffffc);
	switch (addr&3) {
	case 7: lwTmp = CpuRegs[sop.rt]; break;
	case 6: lwTmp  = ((u64)CpuRegs[sop.rt] << 8) | (lwTmp&0x00000000000000ff); break;
	case 5: lwTmp  = ((u64)CpuRegs[sop.rt] << 16) | (lwTmp&0x000000000000ffff); break;
	case 4: lwTmp  = ((u64)CpuRegs[sop.rt] << 24) | (lwTmp&0x0000000000ffffff); break;
	case 3: lwTmp  = ((u64)CpuRegs[sop.rt] << 32) | (lwTmp&0x00000000ffffffff); break;
	case 2: lwTmp  = ((u64)CpuRegs[sop.rt] << 40) | (lwTmp&0x000000ffffffffff); break;
	case 1: lwTmp  = ((u64)CpuRegs[sop.rt] << 48) | (lwTmp&0x0000ffffffffffff); break;
	default: lwTmp  = ((u64)CpuRegs[sop.rt] << 56) | (lwTmp&0x00ffffffffffffff); break;
	}
	vw64(addr&0xfffffffc,lwTmp);
}

void opSH(void) { // Last Checked 01-14-01
	vw16(CpuRegs[sop.rs] + (s16)opcode,(u16)CpuRegs[sop.rt]);
}

void opSLL(void) { // Last Checked 01-14-01
	if (opcode==0) return;
	CpuRegs[sop.rd] = (s32)(CpuRegs[sop.rt] << sop.sa);
}

void opSLLV(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = (s32)(CpuRegs[sop.rt] << (CpuRegs[sop.rs] & 0x1f));
}

void opSLT(void) { // Last Checked 01-14-01
	if (((s64)CpuRegs[sop.rs]) < ((s64)CpuRegs[sop.rt])) {
		CpuRegs[sop.rd] = 1;
	} else {
		CpuRegs[sop.rd] = 0;
	}
}

void opSLTI(void) { // Last Checked 01-14-01
	s64 test = (s16)opcode;
	if (((s64)CpuRegs[sop.rs]) < test) {
		CpuRegs[sop.rt] = 1;
	} else {
		CpuRegs[sop.rt] = 0;
	}
}

void opSLTIU(void) {
	u64 test = ((s16)opcode);
	if (CpuRegs[sop.rs] < test) {
		CpuRegs[sop.rt] = 1;
	} else {
		CpuRegs[sop.rt] = 0;
	}
}

void opSLTU(void) { // Last Checked 01-14-01
	if (CpuRegs[sop.rs] < CpuRegs[sop.rt]) {
		CpuRegs[sop.rd] = 1;
	} else {
		CpuRegs[sop.rd] = 0;
	}
}

void opSRA(void) {
	CpuRegs[sop.rd] = (s32)(((s32)CpuRegs[sop.rt]) >> sop.sa);
}

void opSRAV(void) {
	CpuRegs[sop.rd] = (s32)(((s32)CpuRegs[sop.rt]) >> (CpuRegs[sop.rs] & 0x1f));
}

void opSRL(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = (s32)(((u32)CpuRegs[sop.rt]) >> sop.sa);
}

void opSRLV(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = (s32)(((u32)CpuRegs[sop.rt]) >> (CpuRegs[sop.rs] & 0x1f));
}

void opSUB(void) {
	CpuRegs[sop.rd] = (s32)(CpuRegs[sop.rs] - CpuRegs[sop.rt]);
}

void opSUBU(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = (s32)(((s32)CpuRegs[sop.rs]) - ((s32)CpuRegs[sop.rt]));
}

void opSW(void) { // Last Checked 01-14-01
	vw32(CpuRegs[sop.rs] + (s16)opcode,(u32)CpuRegs[sop.rt]);
}

void opSWC1(void) {
	vw32(CpuRegs[sop.rs] + (s16)opcode,FpuRegs.w[sop.rt]);
}

void opSWC2(void) {
	bool CopUnuseableException(u32 addr, u32 copx);
	CopUnuseableException(pc-4,2);
}

void opSWL(void) { // Last Checked 12-28-00
	u32 addr = CpuRegs[sop.rs] + (s16)opcode;
	u32 lwTmp = vr32(addr&0xfffffffc);
	switch (addr&3) {
	case 0:  lwTmp  = CpuRegs[sop.rt]; break;
	case 1:  lwTmp  = ((u32)CpuRegs[sop.rt] >> 8 ) | (lwTmp&0xff000000); break;
	case 2:  lwTmp  = ((u32)CpuRegs[sop.rt] >> 16) | (lwTmp&0xffff0000); break;
	default: lwTmp  = ((u32)CpuRegs[sop.rt] >> 24) | (lwTmp&0xffffff00); break;
	}
	vw32(addr&0xfffffffc,lwTmp);
	//Debug (0, "SWL: Address: %08X Data: %08X", addr, lwTmp);
}

void opSWR(void) { // Last Checked 12-28-00
	u32 addr = CpuRegs[sop.rs] + (s16)opcode;
	u32 lwTmp = vr32(addr&0xfffffffc);
	switch (addr&3) {
	case 3:  lwTmp  = CpuRegs[sop.rt]; break;
	case 2:  lwTmp  = ((u32)CpuRegs[sop.rt] <<  8) | (lwTmp&0x000000ff); break;
	case 1:  lwTmp  = ((u32)CpuRegs[sop.rt] << 16) | (lwTmp&0x0000ffff); break;
	default: lwTmp  = ((u32)CpuRegs[sop.rt] << 24) | (lwTmp&0x00ffffff); break;
	}
	vw32(addr&0xfffffffc,lwTmp);
	//Debug (0, "SWR: Address: %08X Data: %08X", addr, lwTmp);
}

void opSYNC(void) { }

void opSYSCALL(void) {
	CPU_ERROR("Syscall Called",pc);
}

void opTEQ(void){
	CPU_ERROR("TEQ Called",pc);
}

void opTEQI(void){
	CPU_ERROR("TEQI Called",pc);
}

void opTGE(void) {
	CPU_ERROR("TGE Called",pc);
}

void opTGEI(void){
	CPU_ERROR("TGEI Called",pc);
}

void opTGEIU(void){
	CPU_ERROR("TGEIU Called",pc);
}

void opTGEU(void) {
	CPU_ERROR("TGEU Called",pc);
}

void opTLT(void){
	CPU_ERROR("TLT Called",pc);
}

void opTLTI(void) {
	CPU_ERROR("TLTI Called",pc);
}

void opTLTIU(void) {
	CPU_ERROR("TLTIU Called",pc);
}

void opTLTU(void) {
	CPU_ERROR("TLTU Called",pc);
}

void opTNE(void) {
	CPU_ERROR("TNE Called",pc);
}

void opTNEI(void) {
	CPU_ERROR("TNEI Called",pc);
}

void opXOR(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = CpuRegs[sop.rs] ^ CpuRegs[sop.rt];
}

void opXORI(void) { // Last Checked 01-14-01
	CpuRegs[sop.rt] = CpuRegs[sop.rs] ^ ((u16)opcode);
}

void opCOP1(void);

void (*r4300i[0x40])() = {
    opSPECIAL,	opREGIMM,	opJ,	opJAL,		opBEQ,	opBNE,	opBLEZ,	opBGTZ,  
    opADDI,		opADDIU,	opSLTI,	opSLTIU,	opANDI,	opORI,	opXORI,	opLUI,
    opCOP0,		opCOP1,		opCOP2,	opNI,		opBEQL,	opBNEL,	opBLEZL,opBGTZL,  
    opDADDI,	opDADDIU,	opLDL,	opLDR,		opNI,	opNI,	opNI,	opNI,  
    opLB,		opLH,		opLWL,	opLW,		opLBU,	opLHU,	opLWR,	opLWU,  
    opSB,		opSH,		opSWL,	opSW,		opSDL,	opSDR,	opSWR,	opCACHE,
    opLL,		opLWC1,		opLWC2,	opNI,		opLLD,	opLDC1, opLDC2,	opLD, 
    opSC,		opSWC1,		opSWC2,	opNI,		opSCD,	opSDC1, opSDC2,	opSD 
};

void (*special[0x40])() = {
    opSLL,	opNI,		opSRL,	opSRA,	opSLLV,		opNI,		opSRLV,		opSRAV,  
    opJR,	opJALR,		opNI,	opNI,	opSYSCALL,	opBREAK,	opNI,		opSYNC,  
    opMFHI,	opMTHI,		opMFLO,	opMTLO,	opDSLLV,	opNI,		opDSRLV,	opDSRAV,  
    opMULT,	opMULTU,	opDIV,	opDIVU,	opDMULT,	opDMULTU,	opDDIV,		opDDIVU,
    opADD,	opADDU,		opSUB,	opSUBU,	opAND,		opOR,		opXOR,		opNOR,  
    opNI,	opNI,		opSLT,	opSLTU,	opDADD,		opDADDU,	opDSUB,		opDSUBU,  
    opTGE,	opTGEU,		opTLT,	opTLTU,	opTEQ,		opNI,		opTNE,		opNI,  
    opDSLL,	opNI,		opDSRL,	opDSRA,	opDSLL32,	opNI,		opDSRL32,	opDSRA32  
};

void (*regimm[0x20])() = {
    opBLTZ,		opBGEZ,		opBLTZL,	opBGEZL,	opNI,	opNI,	opNI,	opNI,
    opTGEI,		opTGEIU,	opTLTI,		opTLTIU,	opTEQI,	opNI,	opTNEI,	opNI,
    opBLTZAL,	opBGEZAL,	opBLTZALL,	opBGEZALL,	opNI,	opNI,	opNI,	opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,	opNI,	opNI,	opNI,
};

