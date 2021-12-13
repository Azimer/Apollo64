/**************************************************************************
 *                                                                        *
 *               Copyright (C) 2000-2001, Eclipse Productions             *
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
 *		11-01-01	Rewrote parts of the file... going to keep up on history now
 *		11-11-01	Removed COP1 Unusable when it shouldn't be checked.
 *					Added LL/SC for Gauntlet Legends
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
#include "CpuMain.h"
#include "resource.h"
#pragma warning( disable : 4244 )

#define CPU_ERROR(x,y) Debug(0,"%s %08X",x,y)

#define BREAKPOINT 0xFFFFFFF

#define FAST_INFINITE_LOOP

#define ENABLE_RDRAM_HACK

#ifdef FAST_INFINITE_LOOP
#define FAST_INFINITE_LOOP_JUMP   extern u32 VsyncTime,instructions; if (target==pc-4 && pr32(pc)==0) { InterruptTime = 0; };
#define FAST_INFINITE_LOOP_BRANCH extern u32 VsyncTime,instructions; if (target==pc-4 && pr32(pc)==0 && sop.rt==sop.rs) { InterruptTime = 0; };
#else
	#define FAST_INFINITE_LOOP_JUMP //
	#define FAST_INFINITE_LOOP_BRANCH
#endif

extern u32 VsyncTime;
extern u32 VsyncInterrupt;
u64 CpuRegs[32];
u32 MmuRegs[32];
R4K_FPU_union FpuRegs;
u32 FpuControl[32];
s64 cpuHi;
s64 cpuLo;
u32 instructions = 0;
u32 SystemType;
u32 pc;
char InterruptNeeded=0;
s_sop sop;

extern void (*r4300i[0x40])();
extern void (*special[0x40])();
extern void (*regimm[0x20])();
extern void (*MmuSpecial[0x40])();
extern void (*MmuNormal[0x20])();
extern void (*FpuCommands[0x40])();
extern DWORD fsize; // Needed for some rom hacks
int inDelay=0; // In a delay slot
		   
void OpcodeLookup (DWORD, char *); // Used for Debugging, Defined in R4KDebugger
//void DynaReset (); // Reset the dynarec...

void doInstr(void) {
	inDelay = 1;
//	if (pc < 0xA0000000)
//	if (TLBLUT[pc>>12] > 0xFFFFFFF0)
//		__asm int 3;
	opcode = vr32(pc);
	*(DWORD*)&sop = opcode;

	r4300i[sop.op]();
	inDelay = 0;
}

unsigned long GenerateCRC (unsigned char *data, int size);

void DisassembleRange (u32 Start, u32 End);
void OpcodeLookup (DWORD addy, char *out);
void ClearEventList ();

void InitRegisters () {
	/*
		CpuRegs[0]=0x0000000000000000;
		CpuRegs[6]=0xFFFFFFFFA4001F0C;
		CpuRegs[7]=0xFFFFFFFFA4001F08;
		CpuRegs[8]=0x00000000000000C0;
		CpuRegs[9]=0x0000000000000000;
		CpuRegs[10]=0x0000000000000040;
		CpuRegs[11]=0xFFFFFFFFA4000040;
		CpuRegs[16]=0x0000000000000000;
		CpuRegs[17]=0x0000000000000000;
		CpuRegs[18]=0x0000000000000000;
		CpuRegs[19]=0x0000000000000000;
		CpuRegs[21]=0x0000000000000000; 
		CpuRegs[26]=0x0000000000000000;
		CpuRegs[27]=0x0000000000000000;
		CpuRegs[28]=0x0000000000000000;
		CpuRegs[29]=0xFFFFFFFFA4001FF0;
		CpuRegs[30]=0x0000000000000000;
*/
		*(DWORD *)&idmem[0x1004] = 0x8DA807FC;
/*
		CpuRegs[5]=0x000000005493FB9A;
		CpuRegs[14]=0xFFFFFFFFC2C20384;
*/		
			*(DWORD *)&idmem[0x1000] = 0x3C0DBFC0;
			*(DWORD *)&idmem[0x1008] = 0x25AD07C0;
			*(DWORD *)&idmem[0x100C] = 0x31080080;
			*(DWORD *)&idmem[0x1010] = 0x5500FFFC;
			*(DWORD *)&idmem[0x1014] = 0x3C0DBFC0;
			*(DWORD *)&idmem[0x1018] = 0x8DA80024;
			*(DWORD *)&idmem[0x101C] = 0x3C0BB000;
			
/*		
			CpuRegs[1]=0x0000000000000000;
			CpuRegs[2]=0xFFFFFFFFF58B0FBF;
			CpuRegs[3]=0xFFFFFFFFF58B0FBF;
			CpuRegs[4]=0x0000000000000FBF;
			CpuRegs[12]=0xFFFFFFFF9651F81E;
			CpuRegs[13]=0x000000002D42AAC5;
			CpuRegs[15]=0x0000000056584D60;
			CpuRegs[22]=0x0000000000000091; 
			CpuRegs[25]=0xFFFFFFFFCDCE565F;
			
			CpuRegs[20]=0x0000000000000001;
			CpuRegs[23]=0x0000000000000000;
			CpuRegs[24]=0x0000000000000003;*/

	CpuRegs[0x00] = 0;
	CpuRegs[0x01] = 0;
	CpuRegs[0x02] = 0xffffffffd1731be9;
	CpuRegs[0x03] = 0xffffffffd1731be9;
	CpuRegs[0x04] = 0x01be9;
	CpuRegs[0x05] = 0xfffffffff45231e5;
	CpuRegs[0x06] = 0xffffffffa4001f0c;
	CpuRegs[0x07] = 0xffffffffa4001f08;
	CpuRegs[0x08] = 0x070;
	CpuRegs[0x09] = 0;
	CpuRegs[0x0a] = 0x040;
	CpuRegs[0x0b] = 0xffffffffa4000040;
	CpuRegs[0x0c] = 0xffffffffd1330bc3;
	CpuRegs[0x0d] = 0xffffffffd1330bc3;
	CpuRegs[0x0e] = 0x025613a26;
	CpuRegs[0x0f] = 0x02ea04317;
	CpuRegs[0x10] = 0;
	CpuRegs[0x11] = 0;
	CpuRegs[0x12] = 0;
	CpuRegs[0x13] = 0;
	CpuRegs[0x14] = (u64)SystemType;
	
	CpuRegs[0x15] = 0;
	CpuRegs[0x17] = 0x06;
	CpuRegs[0x18] = 0;
	CpuRegs[0x19] = 0xffffffffd73f2993;
	CpuRegs[0x1a] = 0;
	CpuRegs[0x1b] = 0;
	CpuRegs[0x1c] = 0;
	CpuRegs[0x1d] = 0xffffffffa4001ff0;
	CpuRegs[0x1e] = 0;
	CpuRegs[0x1f] = 0xffffffffa4001550;
	
}

void ResetCPU (void) {
	char Buffer[255];
	DWORD bootcode;

#ifdef ENABLE_RDRAM_HACK
	u32 hack;
#endif
	void InitMMU();
	memcpy(idmem, RomMemory, 0x1000);
	memset(&CpuRegs, 0, sizeof(CpuRegs));
	memset(&MmuRegs, 0, sizeof(MmuRegs));
	memset(&FpuRegs, 0, sizeof(FpuRegs));
	memset(&FpuControl, 0, sizeof(FpuControl));
	bootcode = GenerateCRC (idmem+0x40, 0x1000-0x40);
	InitMMU();
	cpuHi = cpuLo = 0;
	pc = 0xA4000040;
	//pc = 0xBFC00000;
	sprintf (Buffer, "Now playing: %s", rInfo.InternalName);
	SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) Buffer);
	ClearEventList (); // Clears the Interrupt Processing...
	TLBLUT.ResetTLB(); // Clear TLB Table
	//DynaReset ();

	switch (RomHeader.Country_Code)	{
		// NTSC
		case 0x45: case 0x4A:
			SystemType = 1;
		break;

		// PAL
		case 0x44: case 0x50: case 0x55: case 0x59:	case 0x58:
			SystemType = 0;
		break;

		default: 
			SystemType = 1;
		break;
	}
	Debug (0, "CC: %X", RomHeader.Country_Code);
#ifdef ENABLE_RDRAM_HACK
	hack = 0x318;
#endif
	switch (bootcode) {
		case 0xB9C47DC8: // CIC-NUS-6102
			/* CIC-NUS #2 */
			CpuRegs[22] = (u64)0x000000000000003F;
			Debug (0, "CIC-NUS-6102 Detected...");
		break;

		case 0xEDCE5AD9: // CIC-NUS-6103
			CpuRegs[22] = (u64)0x0000000000000078;
			Debug (0, "CIC-NUS-6103 Detected...");
		break;

		case 0xB53C588A: // CIC-NUS-6105
			CpuRegs[22] = (u64)0x0000000000000091;
#ifdef ENABLE_RDRAM_HACK
			hack = 0x3F0;
#endif
			Debug (0, "CIC-NUS-6105 Detected...");
		break;
		case 0x57BB4128: // Yoshi Story???
		case 0x6D8ED9C:  // F-Zero X
			// CIC-NUS-6106
			CpuRegs[22] = (u64)0x0000000000000085;
			Debug (0, "CIC-NUS-6106 Detected...");
		break;
		default:
			Debug (1, "Unknown Bootcode... Please e-mail Azimer with the following Information: \r\nCRC: %X, RomTitle: %s", bootcode, rInfo.InternalName);
			cpuIsReset = true;
			cpuIsDone = true;
	}
	InitRegisters ();
	
	//DisassembleRange (0xA4000040, 0xA4001000); // Disassemble the Bootcode
#ifdef ENABLE_RDRAM_HACK
		while ((pc > 0xA4000000)) {
			//instructions += incrementer;
			InterruptTime -= incrementer;
			((u32*)&sop)[0] = vr32(pc);
			pc+=4;
			r4300i[sop.op]();
		}
	if (RegSettings.isPakInstalled == true) {
		pw32 (hack      , 0x00800000); // Memory Size Hack
	} else {
		pw32 (hack      , 0x00400000); // Memory Size Hack
	}
	u64 crc = (u64)RomHeader.CRC1 << 32 | RomHeader.CRC2;
	Debug (0, "CRC: %08X%08X CC: %02X", RomHeader.CRC1, RomHeader.CRC2, RomHeader.Country_Code);
	pw32 (0xA02FE1C0, 0xAD170014); // DK64 Hack...
	pw32 (0xA02FB1F4, 0xAD090010); // Banjo Tooie Hack
	pw32 (0xA5000508, 0x05080508); // F-Zero X Hack for N64DD - Thanks F|RES and LaC!
	// Golden Eye 007 (USA) (Need to add JAP and EURO versions...)
	if ((crc == 0xdcbc50d109fd1aa3) && (RomHeader.Country_Code == 0x45)) {
		//TLBLUT.mapmem (0x7f000000,0x10034b30,0x1000000); // TODO: I need to fix this so it works again...
	}
	
#endif
//	void InitCoreSync ();
//	InitCoreSync ();
}

void r4300i_HandleErrorRegister(void) {
	char buffer[255];
	Debug (0, "CpuRegs[0] != 0, PC = %08X", pc-4);
	CpuRegs[0] = 0;
	OpcodeLookup(pc-4, buffer);
	Debug (0, buffer);
}

#define MAXPCSIZE 0x7F

u32 lastPC[MAXPCSIZE+1];
u32 lastPCPtr = 0;

void Emulate(void) {

	if (RegSettings.dynamicEnabled == true) {
		//RegSettings.dynamicEnabled = false;
		void R4KDyna_Execute (void);
		R4KDyna_Execute (); // Just run once so we can figure stuff out...
		return;


		Debug (1, "The dynarec isn't enabled... Going interpretive...");
		//extern void r4300iCompiler_Execute(void);
		//r4300iCompiler_Execute ();
		return;
	}

	while (!cpuIsReset) {

		InterruptTime -= RegSettings.VSyncHack;

		if ((InterruptTime <= 0))
			CheckInterrupts();
		extern u8 *rdram;
		if (TLBLUT[pc>>12] > 0xFFFFFFF0) {
			QuickTLBExcept ();
		}
		//lastPC[lastPCPtr++] = pc;
		//lastPCPtr &= MAXPCSIZE;
		((u32*)&sop)[0] = ((u32 *)returnMemPointer(TLBLUT[pc>>12]+(pc & 0xfff)))[0];
		pc+=4;
		r4300i[sop.op]();

		if (CpuRegs[0] != 0) {
			r4300i_HandleErrorRegister();			
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
	Debug(0,"%08X: NI called. %08X",pc-4,*(u32 *)&sop);
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
	CpuRegs[sop.rd] = (s64)((s32)CpuRegs[sop.rt] + (s32)CpuRegs[sop.rs]);
	// TODO: Integer Overflow...
}

void opADDI(void) { // Last Checked 01-14-01
	CpuRegs[sop.rt] = (s64)((s32)CpuRegs[sop.rs] + (s32)((s16)opcode));
	// TODO: Integer Overflow...
}

void opADDIU(void) { // Last Checked 01-14-01
	CpuRegs[sop.rt] = (s64)((s32)CpuRegs[sop.rs] + (s32)((s16)opcode));
}

void opADDU(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = (s64)((s32)CpuRegs[sop.rt] + (s32)CpuRegs[sop.rs]);
}

void opAND(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = CpuRegs[sop.rt] & CpuRegs[sop.rs];
}

void opANDI(void) { // Last Checked 01-14-01
	CpuRegs[sop.rt] = CpuRegs[sop.rs] & (u64)((u16)opcode);
}

void Test64bit (u64 x, u64 y) { // *** Temporary Testing ***
	static u32 last64bit = 0;
	char buffer[100];
	if (last64bit == pc)
		return;
	if ( ( (u32)x == (u32)y ) !=
		 ( (u64)x == (u64)y ) ) {
		//Debug (0, "Need 64bit at %08X", pc);
		last64bit = pc;
		//OpcodeLookup(pc-4, buffer);
		//Debug (0, "%08x: %s", pc-4, buffer);
		//OpcodeLookup(pc, buffer);
		//Debug (0, "%08x: %s", pc, buffer);
	}
}

void opBEQ(void) { // Last Checked 01-14-01
	Test64bit (CpuRegs[sop.rt], CpuRegs[sop.rs]);
	if (CpuRegs[sop.rt] == CpuRegs[sop.rs])	{
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		FAST_INFINITE_LOOP_BRANCH
		doInstr();
		pc = target;
	}
}

void opBEQL(void) { // Last Checked 01-14-01
	Test64bit (CpuRegs[sop.rt], CpuRegs[sop.rs]);
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
	Test64bit (0, CpuRegs[sop.rs]);
	if (((s64)CpuRegs[sop.rs]) >= 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		FAST_INFINITE_LOOP_BRANCH
		doInstr();
		pc = target;
	}
}

void opBGEZAL(void) { // Last Checked 01-14-01
	Test64bit (0, CpuRegs[sop.rs]);
	CpuRegs[31] = pc+4;
	if (((s64)CpuRegs[sop.rs]) >= 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		CpuRegs[31] = pc+4;
		pc = target;
	}
}

void opBGEZALL(void) { // Last Checked 01-14-01
	Test64bit (0, CpuRegs[sop.rs]);
	CpuRegs[31] = pc+4;
	if (((s64)CpuRegs[sop.rs]) >= 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		CpuRegs[31] = pc+4;
		pc = target;
	}else{
		pc+=4;
	}
}

void opBGEZL(void) { // Last Checked 01-14-01
	Test64bit (0, CpuRegs[sop.rs]);
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
	Test64bit (0, CpuRegs[sop.rs]);
	if (((s64)CpuRegs[sop.rs]) > 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		FAST_INFINITE_LOOP_BRANCH
		doInstr();
		pc = target;
	}
}

void opBGTZL(void) {
	Test64bit (0, CpuRegs[sop.rs]);
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
	Test64bit (0, CpuRegs[sop.rs]);
	if (((s64)CpuRegs[sop.rs]) <= 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		FAST_INFINITE_LOOP_BRANCH
		doInstr();
		pc = target;
	}
}

void opBLEZL(void) { // Last Checked 01-14-01
	Test64bit (0, CpuRegs[sop.rs]);
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
	Test64bit (0, CpuRegs[sop.rs]);
	if (((s64)CpuRegs[sop.rs]) < 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		FAST_INFINITE_LOOP_BRANCH
		doInstr();
		pc = target;
	}
}

void opBLTZAL(void) { // Last Checked 01-14-01
	Test64bit (0, CpuRegs[sop.rs]);
	CpuRegs[31] = pc+4;
	if (((s64)CpuRegs[sop.rs]) < 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		pc = target;
	}
}

void opBLTZALL(void) { // Last Checked 01-14-01
	Test64bit (0, CpuRegs[sop.rs]);
	CpuRegs[31] = pc+4;
	if (((s64)CpuRegs[sop.rs]) < 0) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		doInstr();
		pc = target;
	}else{
		pc+=4;
	}
}

void opBLTZL(void) { // Last Checked 01-14-01
	Test64bit (0, CpuRegs[sop.rs]);
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
	Test64bit (CpuRegs[sop.rt], CpuRegs[sop.rs]);
	if (CpuRegs[sop.rt] != CpuRegs[sop.rs]) {
		u32 target = ((s16)opcode);
		target = pc + (target << 2);
		FAST_INFINITE_LOOP_BRANCH
		doInstr();
		pc = target;
	}
}

		// speed hack, like in Banjo Game
		// 802483F4:	BNEL	v0,r0, 802483F4,   where r0 = 0
		// 802483f*:	ADDIU	v0,v0, FFFFFFFC    v0=v0-4
		//
		//-----------------------------------------------------------------
		//| BNEL      | Branch on Not Equal Likley                        |
		//|-----------|---------------------------------------------------|
		//|010101 (21)|   rs    |   rt    |            offset             |
		//------6----------5---------5-------------------16----------------
		//
		//
		//-----------------------------------------------------------------
		//| ADDIU     | ADD Immediate Unsigned word                       |
		//|-----------|---------------------------------------------------|
		//|001001 (9) |   rs    |   rt    |          immediate            |
		//------6----------5---------5-------------------16----------------
		//
/*
#define FAST_INFINITE_LOOP_JUMP   extern u32 VsyncTime,instructions; 
if (target==pc-4 && pr32(pc)==0) instructions = VsyncTime-1;
#define FAST_INFINITE_LOOP_BRANCH extern u32 VsyncTime,instructions; 
if (target==pc-4 && pr32(pc)==0 && sop.rt==sop.rs) instructions = VsyncTime-1;
*/
//800CDBC0: BNEL	V0, V1, 0x800CDBC0
//800CDBC4: ADDIU	V0, V0, 0x0004

void opBNEL(void) { // Last Checked 01-14-01
	Test64bit (CpuRegs[sop.rt], CpuRegs[sop.rs]);
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
	//Debug(0,"BREAK opcode at %X, A2:%X, VO:%X\n",pc,CpuRegs[6],CpuRegs[2]);
	//CPU_ERROR("Break Called",pc);
	//void BreakException ();
	//BreakException ();
}

void opCACHE(void) { } // TODO: Do I need to do anything with this instruction? 12-28-00

void opDADD(void) {
	CpuRegs[sop.rd] = CpuRegs[sop.rt] + CpuRegs[sop.rs];
}

void opDADDI(void) {
	CpuRegs[sop.rt] = CpuRegs[sop.rs] + (s64)((s16)opcode);
	// Need OverFlow
}

void opDADDIU(void) {
	CpuRegs[sop.rt] = CpuRegs[sop.rs] + (s64)((s16)opcode);
}

void opDADDU(void) {
	CpuRegs[sop.rd] = CpuRegs[sop.rt] + CpuRegs[sop.rs];
}

void opDDIV(void) {
	if (CpuRegs[sop.rt] == 0) {
		Debug (0, "DDIV by Zero... this should cause an exception: %08X", pc-4);
		cpuLo = cpuHi = 0;
	} else {
		cpuLo = (s64)CpuRegs[sop.rs] / (s64)CpuRegs[sop.rt];
		cpuHi = (s64)CpuRegs[sop.rs] % (s64)CpuRegs[sop.rt];
	}
}

void opDDIVU(void) {
	if (CpuRegs[sop.rt] == 0) {
		Debug (0, "DDIVU by Zero... this should cause an exception: %08X", pc-4);
		cpuLo = cpuHi = 0;
	} else {
		cpuLo = (u64)CpuRegs[sop.rs] / (u64)CpuRegs[sop.rt];
		cpuHi = (u64)CpuRegs[sop.rs] % (u64)CpuRegs[sop.rt];
	}
}

void opDIV(void) {
	if (CpuRegs[sop.rt] == 0) {
		Debug (0, "DIV by Zero... this should cause an exception: %08X", pc-4);
		cpuLo = cpuHi = 0;
	} else {
		cpuLo = ((s32)CpuRegs[sop.rs]) / ((s32)CpuRegs[sop.rt]);
		cpuHi = ((s32)CpuRegs[sop.rs]) % ((s32)CpuRegs[sop.rt]);
	}
}

void opDIVU(void) {
	if (CpuRegs[sop.rt] == 0) {
		Debug (0, "DIVU by Zero... this should cause an exception: %08X", pc-4);
		cpuLo = cpuHi = 0;
	} else {
		cpuLo = ((u32)CpuRegs[sop.rs]) / ((u32)CpuRegs[sop.rt]);
		cpuHi = ((u32)CpuRegs[sop.rs]) % ((u32)CpuRegs[sop.rt]);
	}
}

void opDMULT(void) {
     unsigned __int64 hh,hl,lh,ll,b;
     __int64 t1,t2;

     t1 = CpuRegs[sop.rs];
     t2 = CpuRegs[sop.rt];

     hh = ((__int64)(t1 >> 32)  & 0x0ffffffff)  * ((__int64)(t2 >> 32)  &   0x0ffffffff);
     hl =  (__int64)(t1         & 0x0ffffffff)  * ((__int64)(t2 >> 32)  &   0x0ffffffff);
     lh = ((__int64)(t1 >> 32)  & 0x0ffffffff)  *  (__int64)(t2         &   0x0ffffffff);
     ll = ((__int64)(t1         & 0x0ffffffff)  *  (__int64)(t2         &   0x0ffffffff));

     cpuLo = ((hl + lh) << 32) + ll;

     b=(((hl + lh) + (ll >> 32)) & 0x0100000000)>>32;

     cpuHi = (unsigned __int64)hh + ((signed __int64)(unsigned __int32)(hl
>> 32) + (signed __int64)(unsigned __int32)(lh >> 32) + b);
}

void opDMULTU(void) {
     unsigned __int64 hh,hl,lh,ll,b;
     __int64 t1,t2;
     int sgn = 0;

     t1 = CpuRegs[sop.rs];
     t2 = CpuRegs[sop.rt];
     if (t1 < 0) {sgn ^= 1; t1 = -t1;}
     if (t2 < 0) {sgn ^= 1; t2 = -t2;}

     hh = ((__int64)(t1 >> 32) & 0x0ffffffff)   * ((__int64)(t2 >> 32)  &   0x0ffffffff);
     hl = (__int64)(t1 & 0x0ffffffff)           * ((__int64)(t2 >> 32)  &   0x0ffffffff);
     lh = ((__int64)(t1 >> 32) & 0x0ffffffff)   *  (__int64)(t2         &   0x0ffffffff);
     ll = ((__int64)(t1 & 0x0ffffffff)          *  (__int64)(t2         &   0x0ffffffff));

     cpuLo = ((hl + lh) << 32) + ll;

     b=(((hl + lh) + (ll >> 32)) & 0x0100000000)>>32;

     cpuHi = (unsigned __int64)hh + ((signed __int64)(unsigned __int32)(hl  >> 32) + (signed __int64)(unsigned __int32)(lh >> 32) + b);

     b = (cpuLo >= 0) ? 1 : 0;

     if (sgn != 0)
     {
          cpuLo = -cpuLo;
          cpuHi = -cpuHi + b;
     }
}
//*/
void opDSLL(void) {
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
/*	if (0x8009F1D8 == target) // Zelda Hack
		return;*/
	pc = target;
//	void RecompilerMain ();
//	RecompilerMain ();
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
	CpuRegs[sop.rt] = (s64)(vr64(CpuRegs[sop.rs] + (s16)opcode));
}

void opLDC1(void) { // 10-30-01 - COP1 Unusable isn't shouldn't happen
	bool CopUnuseableException(u32 addr, u32 copx);
	if (!(MmuRegs[12] & 0x20000000)) {
		pc-=4;
		CopUnuseableException(pc,1);
		return;
	}
#ifdef USE_OLD_FPU
	FpuRegs.l[sop.rt/2] = vr64(CpuRegs[sop.rs] + (s16)opcode);
#else
	*(unsigned __int64 *)FPRDoubleLocation[sop.rt] = vr64(CpuRegs[sop.rs] + (s16)opcode);
#endif
}

void opLDC2(void) {
	bool CopUnuseableException(u32 addr, u32 copx);
	CopUnuseableException(pc-4,2);
}

void opLDL(void) { // Fixed: 10-29-01 (addr&3 not addr&7 and addr&0xffffffffc)
	u32 addr = CpuRegs[sop.rs] + (s16)opcode;
	u64 lwTmp = vr64(addr&0xfffffff8); // Was v32
	switch (addr&7) {
	case 0: break;
	case 1:  lwTmp  = (lwTmp << 8 ) | ((u64)CpuRegs[sop.rt]&0x00000000000000ff); break;
	case 2:  lwTmp  = (lwTmp << 16) | ((u64)CpuRegs[sop.rt]&0x000000000000ffff); break;
	case 3:  lwTmp  = (lwTmp << 24) | ((u64)CpuRegs[sop.rt]&0x0000000000ffffff); break;
	case 4:  lwTmp  = (lwTmp << 32) | ((u64)CpuRegs[sop.rt]&0x00000000ffffffff); break;
	case 5:  lwTmp  = (lwTmp << 40) | ((u64)CpuRegs[sop.rt]&0x000000ffffffffff); break;
	case 6:  lwTmp  = (lwTmp << 48) | ((u64)CpuRegs[sop.rt]&0x0000ffffffffffff); break;
	default: lwTmp  = (lwTmp << 56) | ((u64)CpuRegs[sop.rt]&0x00ffffffffffffff); break;
	}
	CpuRegs[sop.rt] = (s64)lwTmp;
}

void opLDR(void) { // Fixed: 10-29-01 (addr&3 not addr&7 and addr&0xffffffffc)
	u32 addr = CpuRegs[sop.rs] + (s32)(s16)opcode;
	u64 lwTmp = vr64(addr&0xfffffff8); // vr32
	switch (addr&7) {
	case 7:  break;
	case 6:  lwTmp  = (lwTmp >> 8 ) | ((u64)CpuRegs[sop.rt]&0xff00000000000000); break;
	case 5:  lwTmp  = (lwTmp >> 16) | ((u64)CpuRegs[sop.rt]&0xffff000000000000); break;
	case 4:  lwTmp  = (lwTmp >> 24) | ((u64)CpuRegs[sop.rt]&0xffffff0000000000); break;
	case 3:  lwTmp  = (lwTmp >> 32) | ((u64)CpuRegs[sop.rt]&0xffffffff00000000); break;
	case 2:  lwTmp  = (lwTmp >> 40) | ((u64)CpuRegs[sop.rt]&0xffffffffff000000); break;
	case 1:  lwTmp  = (lwTmp >> 48) | ((u64)CpuRegs[sop.rt]&0xffffffffffff0000); break;
	default: lwTmp  = (lwTmp >> 56) | ((u64)CpuRegs[sop.rt]&0xffffffffffffff00); break;
	}
	CpuRegs[sop.rt] = (s64)lwTmp;
}

void opLH(void) { // Last Checked 01-14-01
	CpuRegs[sop.rt] = (s64)(s16)(vr16(CpuRegs[sop.rs] + (s32)(s16)opcode));
	// TODO: Address Error Exception
}

void opLHU(void) {
	CpuRegs[sop.rt] = (u64)(u16)(vr16(CpuRegs[sop.rs] + (s32)(s16)opcode));
}

extern int LLBit;
u32 LastLL;

void opLL(void) { // This opcode is needed for Guantlet Legends
	u32 addr = VirtualToPhysical((CpuRegs[sop.rs] + (s32)(s16)opcode), 0);
	addr &= 0x1fffffff;
	//Debug (0, "LL Called: %08X -> %08X",pc-4, (CpuRegs[sop.rs] + (s32)(s16)opcode));
	CpuRegs[sop.rt] = (s64)*(s32*)(valloc+addr);
	LastLL = MmuRegs[17] = addr;
	LLBit = 1;
	// TODO: Otherstuff
}

void opLLD(void) { // I don't believe this is implemented on the r4300
	CPU_ERROR("LLD Called",pc);
}

void opLUI(void) { // Last Checked 01-14-01
	CpuRegs[sop.rt] = (s64)(s32)((s16)opcode << 16);
}
bool romwrite = false;
u32 nextread = 0;
void opLW(void) { // Last Checked 01-14-01
	/*u32 addr = (CpuRegs[sop.rs] + (s32)(s16)opcode);
	if ((addr >= 0xB0000000) && (addr < 0xBFFFFFFF)) {
		if (romwrite == true) {
			Debug (0, "%08X: %08X", addr, CpuRegs[sop.rt]);
			//CpuRegs [sop.rt] = nextread;
			romwrite = false;
		}
	}*/
	CpuRegs[sop.rt] = (s64)(s32)(vr32(CpuRegs[sop.rs] + (s32)(s16)opcode));
	/*extern u32 romsize;
	
/*	if ((addr & 0x00FFFFFF) > 0x2fb1f0) {
		if ((addr & 0x00FFFFFF) < 0x2fb5f0) {
			Debug (0, "Reading Reg: %08X - %08X", addr, CpuRegs[sop.rt]);
		}
	}
	extern u32 romsize;
	if ((addr > 0xB0000000+romsize)) {
		Debug(0, "Reading Reg: %08X - %08X", addr, CpuRegs[sop.rt]);
	}
	// TODO: Address Error Exception...*/
}

void opLWC1(void) { // 10-30-01 - COP1 Unusable isn't shouldn't happen
	bool CopUnuseableException(u32 addr, u32 copx);
	if (!(MmuRegs[12] & 0x20000000)) {
		pc-=4;
		CopUnuseableException(pc,1);
		return;
	}
#ifdef USE_OLD_FPU
	FpuRegs.w[sop.rt] = vr32(CpuRegs[sop.rs] + (s16)opcode);
#else
	*(DWORD *)FPRFloatLocation[sop.rt] = vr32(CpuRegs[sop.rs] + (s16)opcode);
#endif
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
	CpuRegs[sop.rt] = (s64)(s32)lwTmp;
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
	CpuRegs[sop.rt] = (s64)(s32)lwTmp;
}

void opLWU(void) {
	CpuRegs[sop.rt] = (u64)(u32)(vr32(CpuRegs[sop.rs] + (s32)(s16)opcode));//unrisky
}

void opMFHI(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = cpuHi;
}

void opMFLO(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = cpuLo;
}
void opMTHI(void) { // Last Checked 01-14-01
	cpuHi = CpuRegs[sop.rs];
}
void opMTLO(void) { // Last Checked 01-14-01
	cpuLo = CpuRegs[sop.rs];
}

void opMULT(void) { // Last Checked 01-14-01
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
	CpuRegs[sop.rt] = CpuRegs[sop.rs] | (u64)((u16)opcode);
}

void opSB(void) { // Last Checked 01-14-01
	vw8(CpuRegs[sop.rs] + (s16)opcode,(u8)CpuRegs[sop.rt]);
	// TODO: Address Error Exception
}

void opSC(void) { // Required by Guantlet Legends
	//Debug (0, "SC Called: %08X <- %08X",pc-4, (CpuRegs[sop.rs] + (s32)(s16)opcode));
	if (LastLL != MmuRegs[17])
		LLBit = 0;
	if (LLBit == 1) {
		vw32 (CpuRegs[sop.rs] + (s16)opcode, (u32)CpuRegs[sop.rt]);
	}
	CpuRegs[sop.rt] = LLBit;
}

void opSCD(void) {
	CPU_ERROR("SCD Called",pc);
}

void opSD(void) {
	vw64(CpuRegs[sop.rs] + (s16)opcode,CpuRegs[sop.rt]);
}

void opSDC1(void) { // 10-30-01 - COP1 Unusable isn't shouldn't happen
	bool CopUnuseableException(u32 addr, u32 copx);
	if (!(MmuRegs[12] & 0x20000000)) {
		pc-=4;
		CopUnuseableException(pc,1);
		return;
	}
#ifdef USE_OLD_FPU
	vw64(CpuRegs[sop.rs] + (s16)opcode,FpuRegs.l[sop.rt/2]);
#else
	vw64(CpuRegs[sop.rs] + (s16)opcode, *(__int64 *)FPRDoubleLocation[sop.rt]);
#endif
}

void opSDC2(void) {
	bool CopUnuseableException(u32 addr, u32 copx);
	CopUnuseableException(pc-4,2);
}

void opSDL(void) { // Fixed: 10-29-01 (addr&3 not addr&7 and addr&0xffffffffc)
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

void opSDR(void) { // Fixed: 10-29-01 (addr&3 not addr&7 and addr&0xffffffffc)
	u32 addr = CpuRegs[sop.rs] + (s16)opcode;
	u64 lwTmp = vr64(addr&0xfffffff8);
	switch (addr&7) {
	case 7: lwTmp = CpuRegs[sop.rt]; break;
	case 6: lwTmp  = ((u64)CpuRegs[sop.rt] << 8) | (lwTmp&0x00000000000000ff); break;
	case 5: lwTmp  = ((u64)CpuRegs[sop.rt] << 16) | (lwTmp&0x000000000000ffff); break;
	case 4: lwTmp  = ((u64)CpuRegs[sop.rt] << 24) | (lwTmp&0x0000000000ffffff); break;
	case 3: lwTmp  = ((u64)CpuRegs[sop.rt] << 32) | (lwTmp&0x00000000ffffffff); break;
	case 2: lwTmp  = ((u64)CpuRegs[sop.rt] << 40) | (lwTmp&0x000000ffffffffff); break;
	case 1: lwTmp  = ((u64)CpuRegs[sop.rt] << 48) | (lwTmp&0x0000ffffffffffff); break;
	default: lwTmp  = ((u64)CpuRegs[sop.rt] << 56) | (lwTmp&0x00ffffffffffffff); break;
	}
	vw64(addr&0xfffffff8,lwTmp);
}

void opSH(void) { // Last Checked 01-14-01
//	u32 addy = (CpuRegs[sop.rs] + (s16)opcode);
//	if (pc == 0x80001844) {
//		static FILE *dfile = fopen ("c:\\debug.txt", "wt");
//		fprintf (dfile, "Value: %08X\n", CpuRegs[sop.rs]);
//	}
//	if (CpuRegs[sop.rs] == 0x4005)
//		Debug (0, "PC = %08X", pc);
//	if (addy & 1) {
//		void AddressErrorException (bool store);
//		AddressErrorException (true);
//	} else {
		vw16(CpuRegs[sop.rs] + (s16)opcode,(u16)CpuRegs[sop.rt]);
//	}
}

void opSLL(void) { // Last Checked 01-14-01
	if (opcode==0) return;
	CpuRegs[sop.rd] = (s64)(s32)(CpuRegs[sop.rt] << sop.sa);
}

void opSLLV(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = (s64)(s32)(CpuRegs[sop.rt] << (CpuRegs[sop.rs] & 0x1f));
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
	//if (sop.rt == 0) return; // TODO: Commented this out 10-29-01
	if (((s64)CpuRegs[sop.rs]) < (s64)test) {
		CpuRegs[sop.rt] = 1;
	} else {
		CpuRegs[sop.rt] = 0;
	}
}

void opSLTIU(void) {
	s64 test = ((s16)opcode);
	if ((u64)CpuRegs[sop.rs] < (u64)test) {
		CpuRegs[sop.rt] = 1;
	} else {
		CpuRegs[sop.rt] = 0;
	}
}

void opSLTU(void) { // Last Checked 01-14-01
	if ((u64)CpuRegs[sop.rs] < (u64)CpuRegs[sop.rt]) {
		CpuRegs[sop.rd] = 1;
	} else {
		CpuRegs[sop.rd] = 0;
	}
}

void opSRA(void) {
	CpuRegs[sop.rd] = (s64)(s32)(((s32)CpuRegs[sop.rt]) >> sop.sa);
}

void opSRAV(void) {
	CpuRegs[sop.rd] = (s64)(s32)(((s32)CpuRegs[sop.rt]) >> (CpuRegs[sop.rs] & 0x1f));
}

void opSRL(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = (s64)(s32)(((u32)CpuRegs[sop.rt]) >> sop.sa);
}

void opSRLV(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = (s64)(s32)(((u32)CpuRegs[sop.rt]) >> (CpuRegs[sop.rs] & 0x1f));
}

void opSUB(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = (s64)(s32)((s32)CpuRegs[sop.rs] - (s32)CpuRegs[sop.rt]);
}

void opSUBU(void) { // Last Checked 01-14-01
	CpuRegs[sop.rd] = (s64)(s32)(((s32)CpuRegs[sop.rs]) - ((s32)CpuRegs[sop.rt]));
}

void opSW(void) { // Last Checked 01-14-01
	/*u32 addr = CpuRegs[sop.rs] + (s16)opcode;
	if ((addr >= 0xB0000000) && (addr < 0xBFFFFFFF)) {
		romwrite = true;
		//nextread = (u32)CpuRegs[sop.rt];
		Debug (0, "Romwrite to : %08X", addr);
	}// else {*/
		vw32(CpuRegs[sop.rs] + (s16)opcode,(u32)CpuRegs[sop.rt]);
	//}
}

void opSWC1(void) { // 10-30-01 - COP1 Unusable isn't shouldn't happen
	bool CopUnuseableException(u32 addr, u32 copx);
	if (!(MmuRegs[12] & 0x20000000)) {
		pc-=4;
		CopUnuseableException(pc,1);
		return;
	}
#ifdef USE_OLD_FPU
	vw32(CpuRegs[sop.rs] + (s16)opcode,FpuRegs.w[sop.rt]);
#else
	vw32(CpuRegs[sop.rs] + (s16)opcode, *(DWORD *)FPRFloatLocation[sop.rt]);
#endif
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
}

void opSYNC(void) { }

void opSYSCALL(void) {
	//CPU_ERROR("Syscall Called",pc);
	void SyscallException ();
	SyscallException ();
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
	CpuRegs[sop.rt] = CpuRegs[sop.rs] ^ (u64)((u16)opcode);
}

void opCOP1(void);

//void RecOp (void);

void (*r4300i[0x40])() = {
    opSPECIAL,	opREGIMM,	opJ,	opJAL,		opBEQ,	opBNE,	opBLEZ,	opBGTZ, //00-07
    opADDI,		opADDIU,	opSLTI,	opSLTIU,	opANDI,	opORI,	opXORI,	opLUI,	//08-0F
    opCOP0,		opCOP1,		opCOP2,	opNI,		opBEQL,	opBNEL,	opBLEZL,opBGTZL,//10-17
    opDADDI,	opDADDIU,	opLDL,	opLDR,		opNI,	opNI,	opNI,	opNI,   //18-1F
    opLB,		opLH,		opLWL,	opLW,		opLBU,	opLHU,	opLWR,	opLWU,  //20-27
    opSB,		opSH,		opSWL,	opSW,		opSDL,	opSDR,	opSWR,	opCACHE,//28-2F
    opLL,		opLWC1,		opLWC2,	opNI,		opLLD,	opLDC1, opLDC2,	opLD,   //30-37
    opSC,		opSWC1,		opSWC2,	opNI,		opSCD,	opSDC1, opSDC2,	opSD    //38-3F
};

void (*special[0x40])() = {
    opSLL,	opNI,		opSRL,	opSRA,	opSLLV,		opNI,		opSRLV,		opSRAV,  // 00-07
    opJR,	opJALR,		opNI,	opNI,	opSYSCALL,	opBREAK,	opNI,		opSYNC,  // 08-0F
    opMFHI,	opMTHI,		opMFLO,	opMTLO,	opDSLLV,	opNI,		opDSRLV,	opDSRAV, // 10-17
    opMULT,	opMULTU,	opDIV,	opDIVU,	opDMULT,	opDMULTU,	opDDIV,		opDDIVU, // 18-1F
    opADD,	opADDU,		opSUB,	opSUBU,	opAND,		opOR,		opXOR,		opNOR,   // 20-27
    opNI,	opNI,		opSLT,	opSLTU,	opDADD,		opDADDU,	opDSUB,		opDSUBU, // 28-2F
    opTGE,	opTGEU,		opTLT,	opTLTU,	opTEQ,		opNI,		opTNE,		opNI,    // 30-37
    opDSLL,	opNI,		opDSRL,	opDSRA,	opDSLL32,	opNI,		opDSRL32,	opDSRA32 // 38-3F
};

void (*regimm[0x20])() = {
    opBLTZ,		opBGEZ,		opBLTZL,	opBGEZL,	opNI,	opNI,	opNI,	opNI,
    opTGEI,		opTGEIU,	opTLTI,		opTLTIU,	opTEQI,	opNI,	opTNEI,	opNI,
    opBLTZAL,	opBGEZAL,	opBLTZALL,	opBGEZALL,	opNI,	opNI,	opNI,	opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,	opNI,	opNI,	opNI,
};

