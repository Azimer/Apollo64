#include <windows.h>
#include <stdio.h>
#include "WinMain.h"
#include "EmuMain.h"
#include "CpuMain.h"
#include "DynaMain.h"
#include "DynaCompile.h"
#include "x86ops.h"


extern void (*r4300i[0x40])();
extern void (*special[0x40])();
extern void (*regimm[0x20])();
extern void (*MmuSpecial[0x40])();
extern void (*MmuNormal[0x20])();
void opCOP1 ();
void InterpreteOp (u32 *op, u32 dynaPC);

inline void RS_RT_Comp () {
	MoveVariableToEax((u32 *)&CpuRegs[sop.rt], "CpuRegs[sop.rt]");
	CompEaxToVariable((u32 *)&CpuRegs[sop.rs], "CpuRegs[sop.rs]");
}
inline void RS_RT_Comp64 () {
	MoveVariableToEax((u8 *)&CpuRegs[sop.rt]+4, "CpuRegs[sop.rt]");
	CompEaxToVariable((u8 *)&CpuRegs[sop.rs]+4, "CpuRegs[sop.rs]");
}

inline void DoDelay (u32 dynaPC) {
	dynaPC += 4;
	u32 *op =	(u32 *)(rdram + 
				(TLBLUT[dynaPC>>0xC] & 0x1fffffff) + 
				(0xfff & dynaPC));
	CompileOpCode (op, dynaPC);
}

// 32bit - No Reg Caching - No Local Link... Simple recompilation...
void dynaBNE (u32 dynaPC, u32 *delay) { // 32bit Jump!
	u8 *JumpByte, *JumpByte2;
	u32 target = (*(s16*)&sop);
	target = dynaPC + (target << 2);

	if ((target == (dynaPC-4)) && *delay==0 && sop.rt==sop.rs) { 
		MoveConstToVariable(0, &InterruptTime, "");
	} else {
		RS_RT_Comp ();
		JeLabel8 ("", 0);
		JumpByte = (x86BlockPtr.ubPtr-1);
		CompileOpCode (delay, dynaPC+4); // Compile Delay slot...
		MoveConstToVariable (target, &pc, "");
		SetBranch8b (JumpByte, x86BlockPtr.ptr);
	}
}
//#define FAST_INFINITE_LOOP_BRANCH 
//extern u32 VsyncTime,instructions; 
//if (target==pc-4 && pr32(pc)==0 && sop.rt==sop.rs) { InterruptTime = 0; };

void dynaBEQ (u32 dynaPC, u32 *delay) { // 32bit Jump!
	u8 *JumpByte, *JumpByte2;
	u32 target = (*(s16*)&sop);
	target = dynaPC + (target << 2);

	if ((target == (dynaPC-4)) && *delay==0 && sop.rt==sop.rs) { 
		MoveConstToVariable(0, &InterruptTime, "");
	} else {
		MoveConstToVariable (dynaPC, &pc, ""); // Needed for delay slot - TODO: Is it necessary??
		RS_RT_Comp64 ();
		JneLabel8 ("", 0);
		JumpByte2 = (x86BlockPtr.ubPtr-1);
		RS_RT_Comp ();
		JneLabel8 ("", 0);
		JumpByte = (x86BlockPtr.ubPtr-1);
		u32 *op =	(u32 *)(rdram + 
					(TLBLUT[dynaPC>>0xC] & 0x1fffffff) + 
					(0xfff & dynaPC));
		CompileOpCode (op, dynaPC+4);
		MoveConstToVariable (target, &pc, "");
		SetBranch8b (JumpByte, x86BlockPtr.ptr);
		SetBranch8b (JumpByte2, x86BlockPtr.ptr);
	}
}

int CompileOpCode (u32 *op, u32 dynaPC) {
	*(u32 *)&sop = *op;
	switch (sop.op) {
		case 0x4:
			if (TLBLUT[dynaPC>>12] > 0xFFFFFFF0) // TODO: This is failing in Golden Eye 007...
				return -1;
//			if (dynaPC == 0x150a6cf0) {
//				Debug (0, "Detected 64bit op...");
//				InterpreteOp(op, dynaPC);
//				return 0;
//			}
			dynaBEQ (dynaPC, op+1);
			return 0; // Means to advance the pointer stuff by 1
		break;
//		case 0x5:
//			dynaBNE (dynaPC, op+1);
//			return 0; // Means to advance the pointer stuff by 1
		break;//*/
		default:
			InterpreteOp (op, dynaPC);
	}
	return 0;
}

void InterpreteOp (u32 *op, u32 dynaPC) {
	MoveConstToVariable(dynaPC, &pc, "pc");
	MoveConstToVariable(*op, &sop, "sop");
	switch (sop.op) {
		case 0x00:
			CallFunctionDirect (special[sop.func], "Interpreted Opcode");
		break;
		case 0x01:
			CallFunctionDirect (regimm[sop.rt], "Interpreted Opcode");
		break;
		case 0x10:
			if (sop.rs == 16)
				CallFunctionDirect (MmuSpecial[sop.func], "Interpreted Opcode");
			else
				CallFunctionDirect (MmuNormal[sop.rs], "Interpreted Opcode");
		break;
		case 0x11: // Add direct links to these and it will go like 5 fps faster!
			CallFunctionDirect (opCOP1, "Interpreted Opcode");
		break;
		default:
			CallFunctionDirect(r4300i[sop.op], "Interpreted Opcode");
	}
	
}