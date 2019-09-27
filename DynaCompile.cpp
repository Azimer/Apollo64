#include <windows.h>
#include <stdio.h>
#include "WinMain.h"
#include "EmuMain.h"
#include "CpuMain.h"
#include "DynaMain.h"
#include "DynaCompile.h"
#include "x86ops.h"
// No local links


bool DynaCompileBlock(u32 blockCount) {
	BlockType *block = &BlockInfo[blockCount];
	u32 dynaPC = block->target;
	u32 maxPC  = block->target;
	u32 *op =	(u32 *)(rdram + 
				(TLBLUT[block->target>>12] & 0x1fffffff) + 
				(0xfff & block->target));
	u8 *opstart = (u8 *)op;
	u8 *SaveLoc = x86BlockPtr.ubPtr;
	block->memused = 0;

//	Int3 ();
	while (1) {
		u32 code;
		// HACK -----

		op =	(u32 *)(rdram + 
				(TLBLUT[dynaPC>>0xC] & 0x1fffffff) + 
				(0xfff & dynaPC));
		code = *op;
		if ((code & 0xFFFF0000) == 0xECFF0000) {
			code &= 0xFFFF;
//			if (BlockInfo[code].target != dynaPC)
//				__asm int 3;
			code = BlockInfo[code].RecOp;
		}
		dynaPC += 4;// pc += 4;
//		if (pc != dynaPC)
//			__asm int 3;
		// Compile the opcode now...
		// Set sop to what it should be
		*(u32 *)&sop = *op;
		u32 retVal = CompileOpCode (op, dynaPC);
		if (retVal == 1)
		{ op++; dynaPC += 4; }
		else if (retVal == -1) {
			x86BlockPtr.ubPtr = SaveLoc;
			extern void GenerateTLBException (DWORD addr, DWORD type);
			GenerateTLBException (dynaPC+4, 0);
			Debug (0, "Attempting to do TLB Exception in Delay...");
			return false;
			break;
		}
		op++;
		//pc = dynaPC;
//		r4300i[sop.op]();
		//dynaPC = pc;

		if (isBreakable(code) || (TLBLUT[dynaPC>>12] > 0xFFFFFFF0)) {
			block->memused = (u32)x86BlockPtr.ubPtr - (u32)block->mem;
			block->blocksize = (dynaPC - block->target);
//			break;
			block->crc = CRCRec (opstart, block->blocksize);
			Ret ();
			break;
		}
	
		if (isExceptionable(code)) {
			u8 *JumpByte;
			//Int3 ();
//			break;

			CompConstToVariable (dynaPC, &pc, "pc");
			JeLabel8("Check Exception", 0);
			JumpByte = (x86BlockPtr.ubPtr - 1);
			Ret ();
			SetBranch8b(JumpByte, x86BlockPtr.ptr);
		}


		/* 
		// This is only for local linking... add when ready for it!!
		if (maxPC == dynaPC)
			maxPC += 4;
		if (!isOutBound(*op, &maxPC)) {
			maxPC += 4; // The next opcode is not a local jump...
		}
		if (isBranch(*op)) {
			maxPC = getBranchLocation(*op);
		} else if (isJump) {
			u32 location = getJumpLocation (*op);
			if (location > maxPC)
		}
		*/
	}


//	x86BlockPtr.ubPtr += block->memused;
	return true;
}
/*
  *** Compilation ***
Compile:
	Check if op is a branch or jump
		if BRANCH then update function range (exceptions have an odd range)
		if JUMP then check if jump is local
		if JR and branch range <= pc then break from recompilation
		if JUMP and branch range <= pc then break from recompilation
		if ERET and not in exception handler then break from recompilation (???)
	Compile opcode
	goto Compile until region meets analysis criteria of branch range
  *** End Compilation ***
*/