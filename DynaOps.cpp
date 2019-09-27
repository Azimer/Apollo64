#include <windows.h>
#include <stdio.h>
#include "WinMain.h"
#include "EmuMain.h"
#include "CpuMain.h"
#include "DynaCodes.h"

//#define __64BIT_BRANCH__

bool blockFailed = false; // When an unimplemented op is encountered
LinkInfo LinkTable[MAX_LINKS];
u32 LinkCnt = 0;

// Checks to see if the Delay slot will mutate
bool DelayMutates (u32 rs, u32 rt, u32 delay) { // Checks delay slot for dependancy
	u32 rd;
	if (delay == 0)
		return false;
	if ((delay >> 26) == 0) {
		rd = (delay >> 0xB) & 0x1f;
	} else {
		u32 op = (delay >> 26);
		rd = (delay >> 0x10) & 0x1f;
		if ((op > 0x28) && (op <= 0x2F)) // Skips non-mutating ops
			rd = 0;
		if ((op > 0x38) && (op <= 0x3F)) // Skips non-mutating ops
			rd = 0;
	}

	if (rd == 0x0)
		return false;
	
	if (rd == 0)
		return false;
	if (rs == rd)
		return true;
	if (rt == rd)
		return true;
	return false;
}

__inline void R4KDyna_CompileDelay (u32 *delay) { // Done this way for easy debug code removal
	char buff[256];
	u32 temp = *(u32 *)&sop; // save sop
	OpcodeLookup (pc, buff);
	DynaLog ("DelaySlot - %08X	%s", pc, buff);
	R4KDyna_RecompileOpcode (delay);
	*(u32 *)&sop = temp;
}

void CompareRStoRT (u32 *delay) {
	if (DelayMutates (sop.rs, sop.rt, *delay)) {
		MoveVariableToEax(&CpuRegs[sop.rt], "CpuRegs[sop.rt]");
		CompEaxToVariable(&CpuRegs[sop.rs], "CpuRegs[sop.rs]");
		PushFD(); // Save our flags
		R4KDyna_CompileDelay (delay);
		PopFD();
	} else {
		R4KDyna_CompileDelay (delay);
		MoveVariableToEax(&CpuRegs[sop.rt], "CpuRegs[sop.rt]");
		CompEaxToVariable(&CpuRegs[sop.rs], "CpuRegs[sop.rs]");
	}
}

void CompareRStoZero (u32 *delay) {
	if (DelayMutates (sop.rs, 0, *delay)) {
		CompConstToVariable(0, &CpuRegs[sop.rs], "CpuRegs[sop.rs]");
		PushFD(); // Save our flags
		R4KDyna_CompileDelay (delay);
		PopFD();
	} else {
		R4KDyna_CompileDelay (delay);
		CompConstToVariable(0, &CpuRegs[sop.rs], "CpuRegs[sop.rs]");
	}
}

#define targetInRange false//((target >= compStart) && (target < compEnd))

// TODO: We can do a bunch of branch to same address optimizations here...
void R4KDyna_AddLink (u32 pcLoc, u8 *mem) {
	
	if (LinkCnt > MAX_LINKS)
		__asm int 3;
	if (pcLoc < pc) { // We can save ourselves some heartache...
		for (u32 t = 0; t < LinkLocs; t++) {
			if (pcLoc == JumpTable[t].target) {
				int n = (int)JumpTable[t].mem - ((int)mem + 1);
				if ((n > 127) || (n < -128)) { // Then we have a problem... but fixable with a little effort
					x86BlockPtr.ubPtr -= 2;
					if (*(x86BlockPtr.ubPtr) == 0xEB) {// Jmp
						*(x86BlockPtr.ubPtr++) = 0xE9;
					} else {
						*(x86BlockPtr.uwPtr++) = ((*(x86BlockPtr.ubPtr) << 0x8)+0x1000) | 0x0F; // Convert to 32bit
					}
					mem = (u8 *)x86BlockPtr.udwPtr;
					*(x86BlockPtr.udwPtr++) = 0;
					SetBranch32b (mem, JumpTable[t].mem);
				} else {
					SetBranch8b (mem, JumpTable[t].mem);
				}
				break;
			}
		}
	} else {
		// Force 32bit Branch - TODO: Make some sort of approximation?
		x86BlockPtr.ubPtr -= 2;
		if (*(x86BlockPtr.ubPtr) == 0xEB) {// Jmp
			*(x86BlockPtr.ubPtr++) = 0xE9;
		} else {
			*(x86BlockPtr.uwPtr++) = ((*(x86BlockPtr.ubPtr) << 0x8)+0x1000) | 0x0F; // Convert to 32bit
		}
		mem = (u8 *)x86BlockPtr.udwPtr;
		*(x86BlockPtr.udwPtr++) = 0;
		LinkTable[LinkCnt].target = pcLoc;
		LinkTable[LinkCnt].mem = mem;
		LinkCnt++;
	}
}

void R4KDyna_BEQ(u32 *delay) { // 8-16-02
	u8 *JumpByte, *JumpByte2;
	u32 target = ((s16)opcode);
	target = pc + (target << 2);
#ifdef __64BIT_BRANCH__
	Debug (0, "BEQ Needs 64bit operation!");
	blockFailed = true;
#else
#endif
	// TODO: Could check to see if this is zero
	
	CompareRStoRT (delay);
	
	if (targetInRange) {
		JeLabel8("Local Link", 0);
		R4KDyna_AddLink (target, x86BlockPtr.ubPtr - 1);
	} else {
		JneLabel8("BEQ-NoBranch", 0); // Skip the ret... this is an expensive jump
		JumpByte = (x86BlockPtr.ubPtr - 1);
		MoveConstToVariable (target, &pc, "pc");
		Ret ();
		SetBranch8b(JumpByte, x86BlockPtr.ptr);
	}
	DynaLog ("BEQ-NoBranch:");
	pc+=4;
}

void R4KDyna_BNE(u32 *delay) { // 8-16-02
	u8 *JumpByte, *JumpByte2;
	u32 target = ((s16)opcode);
	target = pc + (target << 2);
#ifdef __64BIT_BRANCH__
	Debug (0, "BNE Needs 64bit operation!");
	blockFailed = true;
#else
#endif
	// TODO: Could check to see if this is zero

	CompareRStoRT (delay);
	
	if (targetInRange) {
		JneLabel8("Local Link", 0);
		R4KDyna_AddLink (target, x86BlockPtr.ubPtr - 1);
	} else {
		JeLabel8("BNE-NoBranch", 0); // Skip the ret... this is an expensive jump
		JumpByte = (x86BlockPtr.ubPtr - 1);
		MoveConstToVariable (target, &pc, "pc");
		Ret ();
		SetBranch8b(JumpByte, x86BlockPtr.ptr);
	}
	DynaLog ("BNE-NoBranch:");
	pc+=4;
}

void R4KDyna_BLEZ(u32 *delay) { // 8-18-02
	u8 *JumpByte, *JumpByte2;
	u32 target = ((s16)opcode);
	target = pc + (target << 2);
#ifdef __64BIT_BRANCH__
	Debug (0, "BLEZ Needs 64bit operation!");
	blockFailed = true;
#else
#endif
	// TODO: Could check to see if this is zero
	
	CompareRStoZero (delay);

	if (targetInRange) {
		JleLabel8("Local Link", 0);
		R4KDyna_AddLink (target, x86BlockPtr.ubPtr - 1);
	} else {
		JgLabel8("BLE-NoBranch", 0); // Skip the ret... this is an expensive jump
		JumpByte = (x86BlockPtr.ubPtr - 1);
		MoveConstToVariable (target, &pc, "pc");
		Ret ();
		SetBranch8b(JumpByte, x86BlockPtr.ptr);
	}
	DynaLog ("BLEZ-NoBranch:");
	pc+=4;
}

void R4KDyna_BGTZ(u32 *delay) { // 8-18-02
	u8 *JumpByte, *JumpByte2;
	u32 target = ((s16)opcode);
	target = pc + (target << 2);
#ifdef __64BIT_BRANCH__
	Debug (0, "BGTZ Needs 64bit operation!");
	blockFailed = true;
#else
#endif
	// TODO: Could check to see if this is zero
	
	CompareRStoZero (delay);

	if (targetInRange) {
		JgLabel8("Local Link", 0);
		R4KDyna_AddLink (target, x86BlockPtr.ubPtr - 1);
	} else {
		JleLabel8("BGTZ-NoBranch", 0); // Skip the ret... this is an expensive jump
		JumpByte = (x86BlockPtr.ubPtr - 1);
		MoveConstToVariable (target, &pc, "pc");
		Ret ();
		SetBranch8b(JumpByte, x86BlockPtr.ptr);
	}
	DynaLog ("BGTZ-NoBranch:");
	pc+=4;
}

void R4KDyna_BGEZ(u32 *delay) { // 8-18-02
	u8 *JumpByte, *JumpByte2;
	u32 target = ((s16)opcode);
	target = pc + (target << 2);
#ifdef __64BIT_BRANCH__
	Debug (0, "BGEZ Needs 64bit operation!");
	blockFailed = true;
#else
#endif

	CompareRStoZero (delay);

	if (targetInRange) {
		JgeLabel8("Local Link", 0);
		R4KDyna_AddLink (target, x86BlockPtr.ubPtr - 1);
	} else {
		JlLabel8("BGEZ-NoBranch", 0); // Skip the ret... this is an expensive jump
		JumpByte = (x86BlockPtr.ubPtr - 1);
		MoveConstToVariable (target, &pc, "pc");
		Ret ();
		SetBranch8b(JumpByte, x86BlockPtr.ptr);
	}
	DynaLog ("BGEZ-NoBranch:");
	pc+=4;
}

void R4KDyna_BGEZAL (u32 *delay) {
	u8 *JumpByte, *JumpByte2;
	u32 target = ((s16)opcode);
	target = pc + (target << 2);
	MoveConstToVariable (pc+4, &CpuRegs[31], "RA"); // Link unconditionally
#ifdef __64BIT_BRANCH__
	Debug (0, "BGEZAL Needs 64bit operation!");
	blockFailed = true;
#else
#endif
/*
	if (sop.rs == 0) {
		R4KDyna_CompileDelay (delay);
		JmpLabel8("Local Link", 0);
		R4KDyna_AddLink (target, x86BlockPtr.ubPtr-1);
		DynaLog ("BAL End");
		pc+=4;
		return;
	}
*/
	CompareRStoZero (delay);

	if (targetInRange) {
		JgeLabel8("Local Link", 0);
		R4KDyna_AddLink (target, x86BlockPtr.ubPtr - 1);
	} else {
		JlLabel8("BGEZAL-NoBranch", 0); // Skip the ret... this is an expensive jump
		JumpByte = (x86BlockPtr.ubPtr - 1);
		MoveConstToVariable (target, &pc, "pc");
		Ret ();
		SetBranch8b(JumpByte, x86BlockPtr.ptr);
	}
	DynaLog ("BGEZAL-NoBranch:");
	pc+=4;
}

void R4KDyna_BLTZ(u32 *delay) { // 8-18-02
	u8 *JumpByte, *JumpByte2;
	u32 target = ((s16)opcode);
	target = pc + (target << 2);
#ifdef __64BIT_BRANCH__
	Debug (0, "BLTZ Needs 64bit operation!");
	blockFailed = true;
#else
#endif
	
	CompareRStoZero (delay);

	if (targetInRange) {
		JlLabel8("Local Link", 0);
		R4KDyna_AddLink (target, x86BlockPtr.ubPtr - 1);
	} else {
		JgeLabel8("BLTZ-NoBranch", 0); // Skip the ret... this is an expensive jump
		JumpByte = (x86BlockPtr.ubPtr - 1);
		MoveConstToVariable (target, &pc, "pc");
		Ret ();
		SetBranch8b(JumpByte, x86BlockPtr.ptr);
	}
	DynaLog ("BLTZ-NoBranch:");
	pc+=4;
}

void R4KDyna_BEQL(u32 *delay) {
	u8 *JumpByte, *JumpByte2;
	u32 target = ((s16)opcode);
	target = pc + (target << 2);
#ifdef __64BIT_BRANCH__
	Debug (0, "BEQL Needs 64bit operation!");
	blockFailed = true;
#else
#endif
	// TODO: Could check to see if this is zero
	MoveVariableToEax(&CpuRegs[sop.rt], "CpuRegs[sop.rt]");
	CompEaxToVariable(&CpuRegs[sop.rs], "CpuRegs[sop.rs]");
	JneLabel8("BEQL-NoBranch", 0);
	JumpByte = x86BlockPtr.ubPtr - 1;
	R4KDyna_CompileDelay (delay);
	if (targetInRange) {
		JmpLabel8("Local Link", 0);
		R4KDyna_AddLink (target, x86BlockPtr.ubPtr - 1);
	} else {
		MoveConstToVariable (target, &pc, "pc");
		Ret ();
	}
	DynaLog ("BEQL-NoBranch:");
	SetBranch8b(JumpByte, x86BlockPtr.ptr);
	pc+=4;
}

void R4KDyna_BNEL(u32 *delay) { // 8-16-02
	u8 *JumpByte, *JumpByte2;
	u32 target = ((s16)opcode);
	target = pc + (target << 2);
#ifdef __64BIT_BRANCH__
	Debug (0, "BNEL Needs 64bit operation!");
	blockFailed = true;
#else
#endif
	// TODO: Could check to see if this is zero
	MoveVariableToEax(&CpuRegs[sop.rt], "CpuRegs[sop.rt]");
	CompEaxToVariable(&CpuRegs[sop.rs], "CpuRegs[sop.rs]");
	JeLabel8("BNEL-NoBranch", 0);
	JumpByte = x86BlockPtr.ubPtr - 1;
	R4KDyna_CompileDelay (delay);
	if (targetInRange) {
		JmpLabel8("Local Link", 0);
		R4KDyna_AddLink (target, x86BlockPtr.ubPtr - 1);
	} else {
		MoveConstToVariable (target, &pc, "pc");
		Ret ();
	}
	DynaLog ("BNEL-NoBranch:");
	SetBranch8b(JumpByte, x86BlockPtr.ptr);
	pc+=4;
}

void CheckPC () {
	Debug (0, "pc from ERET = %08X", pc);
}

void R4KDyna_ERET () { // TODO: Stop interpreting this
	//MoveConstToVariable (pc, &pc, "pc"); // TODO: Remove...
	MoveConstToVariable(*(u32 *)&sop, &sop, "GlobalOpcode");
	MoveConstToVariable (pc, &pc, "pc");
	CallFunctionDirect(r4300i[sop.op], "Interpreted Opcode");
//	CallFunctionDirect(CheckPC, "Checking ERET Return Value");
	Ret ();
}

void R4KDyna_JR (u32 *delay) {
	if (DelayMutates (sop.rs, 0, *delay)) {
		__asm int 3; // Don't handle this yet...
	}
	R4KDyna_CompileDelay (delay); 
	// TODO: Could check to see if this is zero
	MoveVariableToEax (&CpuRegs[sop.rs], "CpuRegs[sop.rs]");
	MoveEaxToVariable (&pc, "pc");
	Ret (); // End of Block
	pc+=4;
}

void R4KDyna_J (u32 *delay) {
	u32 target = (pc & 0xf0000000) + ((opcode << 2) & 0x0fffffff);
	R4KDyna_CompileDelay (delay);
	if (targetInRange) {
		JmpLabel8("Local Link", 0);
		R4KDyna_AddLink (target, x86BlockPtr.ubPtr - 1);
	} else {
		MoveConstToVariable (target, &pc, "pc");
		Ret ();
	}
	pc+=4;
}

void R4KDyna_JAL (u32 *delay) {
	u32 target = (pc & 0xf0000000) + ((opcode << 2) & 0x0fffffff);
	R4KDyna_CompileDelay (delay);
	MoveConstToVariable (pc+4, &CpuRegs[31], "RA"); // Link unconditionally
	MoveConstToVariable (target, &pc, "pc");
	Ret ();
	// TODO: Limit a recompile if possible...  May eventually make all ops do this anyway...
	//Debug (0, "%08X: JAL Link: %08X", pc+4, target);
	//JalEntry = 0x7C000000 + (x86BlockPtr.ubPtr - x86Buff.ubPtr);
	pc+=4;
}

void R4KDyna_JALR (u32 *delay) {
	u32 target = (pc & 0xf0000000) + ((opcode << 2) & 0x0fffffff);
	if (DelayMutates (sop.rs, 0, *delay)) {
		__asm int 3; // Don't handle this yet...
	}
	R4KDyna_CompileDelay (delay);
	MoveConstToVariable (pc+4, &CpuRegs[sop.rd], "Link"); // Link unconditionally
	MoveVariableToEax (&CpuRegs[sop.rs], "CpuRegs[sop.rs]");
	MoveEaxToVariable (&pc, "pc");
	Ret (); // End of Block
	// TODO: Limit a recompile if possible...  May eventually make all ops do this anyway...
	//DynaLog ("Forcing JAL Link return...");
	//JalEntry = 0x7C000000 + (x86BlockPtr.ubPtr - x86Buff.ubPtr);
	pc+=4;
}

// TODO: Should I load the PC incase of an exception?
void R4KDyna_InterpreteOp () { // Needed info is in sop...
	MoveConstToVariable(*(u32 *)&sop, &sop, "GlobalOpcode");
	CallFunctionDirect(r4300i[sop.op], "Interpreted Opcode");
}


void test_BEQ (u32 *delay) {
	u32 target = ((s16)opcode);
	u8 *JumpByte;
	target = pc + (target << 2);

	MoveVariableToEax(&CpuRegs[sop.rt], "CpuRegs[sop.rt]");
	CompEaxToVariable(&CpuRegs[sop.rs], "CpuRegs[sop.rs]");
	PushFD();

	*(u32 *)&sop = *delay;
	MoveConstToVariable(*(u32 *)&sop, &sop, "GlobalOpcode");
	CallFunctionDirect(r4300i[sop.op], "Interpreted Opcode");

	MoveConstToVariable (pc+4, &pc, "pc");
	PopFD();
	JneLabel8("", 0);
	JumpByte = x86BlockPtr.ubPtr - 1;	
	MoveConstToVariable (target, &pc, "pc");
// NoBranch
	SetBranch8b(JumpByte, x86BlockPtr.ptr);
}

void test_BEQ2 (u32 *delay) {
	u32 target = ((s16)opcode);
	target = pc + (target << 2);
	u32 rt = CpuRegs[sop.rt], rs = CpuRegs[sop.rs];
	__asm {
		mov eax, dword ptr [rt];
		cmp eax, dword ptr [rs];
		pushfd;
	}
	*(u32 *)&sop = *delay;
	r4300i[sop.op](); // Execute delay
	__asm {
		mov eax, dword ptr [pc];
		add eax, 4;
		popfd;
		jne NoJump
		mov eax, dword ptr [target];
NoJump:
		mov dword ptr [pc], eax;
	}
}

u32 *R4KDyna_RecompileOpcode (u32 *op) {
	*(DWORD*)&sop = *op;
//	R4KDyna_InterpreteOp ();
//	return op;

	if ((pc-4) == BAD_PC) {
		MoveConstToVariable (pc-4, &pc, "pc");
		Ret ();
		return op;
	}
/*
	if (*op == 0x00000000) // Don't compile NOOPs
		return op;
*/
	switch (sop.op) {
	case 0x00: /* R4300I_SPECIAL */
		switch (sop.func) {
		case 0x08: R4KDyna_InterpreteOp (); /*R4KDyna_JR  (++op);*/ break;//r4300i_Compiler_JumpReg(FALSE); break;
		case 0x09: R4KDyna_InterpreteOp (); /*R4KDyna_JALR(++op);*/ break;//r4300i_Compiler_JumpReg(TRUE); break;
		default:
			R4KDyna_InterpreteOp ();
		}
		break;

	case 0x01: /* R4300I_REGIMMEDIATE */
		switch (sop.rt) {
		case 0x00: R4KDyna_InterpreteOp (); /*R4KDyna_BLTZ   (++op);*/ break;
		case 0x01: R4KDyna_InterpreteOp (); /*R4KDyna_BGEZ   (++op);*/  break;
		case 0x02: blockFailed = true; break;//r4300i_Compiler_Branch(&r4300i_Compiler_BLTZ, FALSE); break;
		case 0x03: blockFailed = true; break;//r4300i_Compiler_Branch(&r4300i_Compiler_BGEZ, FALSE); break;
		case 0x10: blockFailed = true; break;//r4300i_Compiler_Branch(&r4300i_Compiler_BLTZ, TRUE); break;
		case 0x11: R4KDyna_InterpreteOp (); /*R4KDyna_BGEZAL (++op);*/ break;
		case 0x12: blockFailed = true; break;//Recompiler.Log("REGIMM_BLTZALL"); break;
		case 0x13: blockFailed = true; break;//Recompiler.Log("REGIMM_BGEZALL"); break;

		default:
			R4KDyna_InterpreteOp ();
		}
		break;

	case 0x02: R4KDyna_InterpreteOp (); /*R4KDyna_J   (++op);*/ break;//r4300i_Compiler_Jump(FALSE); break;
	case 0x03: R4KDyna_InterpreteOp (); /*R4KDyna_JAL (++op);*/ break;//r4300i_Compiler_Jump(TRUE); break;
	case 0x04: {
				/*if (pc == 0x80202058) {
					R4KDyna_BEQ (++op); // Execute the op...
					blockFailed = true;
					MoveConstToVariable (pc, &pc, "pc");
					Ret ();
					break;
				}*/
		if (sop.op != 0x04) __asm int 3;
	MoveConstToVariable(*(u32 *)&sop, &sop, "GlobalOpcode");
				void opBEQ();
//	CallFunctionDirect(opBEQ, "Interpreted Opcode");
//		R4KDyna_InterpreteOp (); 
				//R4KDyna_BEQ (++op);
				test_BEQ (++op);
				//opBEQ();
				MoveConstToVariable (pc, &pc, "pc");
				Ret();
		//blockFailed = true;
			   } break;//r4300i_Compiler_Branch(&r4300i_Compiler_BEQ, FALSE); break;
	case 0x05: R4KDyna_InterpreteOp (); /*R4KDyna_BNE (++op);*/ break;
	case 0x06: R4KDyna_InterpreteOp (); /*R4KDyna_BLEZ(++op);*/ break;//r4300i_Compiler_Branch(&r4300i_Compiler_BLEZ, FALSE); break;
	case 0x07: R4KDyna_InterpreteOp (); /*R4KDyna_BGTZ(++op);*/ break;//r4300i_Compiler_Branch(&r4300i_Compiler_BGTZ, FALSE); break;

	case 0x10: /* R4300I_COP0 */
		switch (sop.rs) {
		case 0x10:
			switch (sop.func) {
			case 0x18: /* COP0_ERET */
				R4KDyna_ERET ();
				//blockFailed = true;
				//r4300i_Compiler_ERET();				
				break;

			default:
				R4KDyna_InterpreteOp ();		
			}
			break;

		default:
			R4KDyna_InterpreteOp ();
		}
		break;

	case 0x11: /* R4300I_COP1 */
		switch (sop.rs) {
		case 0x08: /* COP1_BRANCH */
			switch (sop.rt & 3){
			case 0x00: blockFailed = true; break;//Recompiler.Log("COP1_BC1F"); break;
			case 0x01: blockFailed = true; break;//Recompiler.Log("COP1_BC1T"); break;
			case 0x02: blockFailed = true; break;//Recompiler.Log("COP1_BC1FL"); break;
			case 0x03: blockFailed = true; break;//Recompiler.Log("COP1_BC1TL"); break;
				
			default:
				R4KDyna_InterpreteOp ();
			}
			break;

		default:
			R4KDyna_InterpreteOp ();
		}
		break;
// Likely Ops
	case 0x14: R4KDyna_InterpreteOp (); /*R4KDyna_BEQL (++op);*/ break;
	case 0x15: R4KDyna_InterpreteOp (); /*R4KDyna_BNEL (++op);*/ break;
	case 0x16: blockFailed = true; break;//r4300i_Compiler_Branch(&r4300i_Compiler_BLEZ, FALSE); break;
	case 0x17: blockFailed = true; break;//r4300i_Compiler_Branch(&r4300i_Compiler_BGTZ, FALSE); break;

	default:
		R4KDyna_InterpreteOp ();
	}
	if (blockFailed == true) {
		u8 *JumpByte;
		R4KDyna_InterpreteOp ();
		/*DynaLog ("---------- Failure :-/ -----------");
		MoveConstToVariable (pc, &pc, "pc");
		R4KDyna_InterpreteOp ();
		CompConstToVariable (pc, &pc, "pc");
		JeLabel8("NoChange", 0);
		JumpByte = x86BlockPtr.ubPtr - 1;
		Ret (); // Return to DynaLoop
		SetBranch8b (JumpByte, x86BlockPtr.ubPtr);*/
		blockFailed = false;
	}
	return op;
}