/**************************************************************************
 *                                                                        *
 *               Copyright (C) 2000-2002, Eclipse Productions             *
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
 *		02-15-03	Initial Version
 *		02-21-03	Golden Eye capable Caching interpreter complete
 *
 **************************************************************************/
/**************************************************************************
 *
 *  Notes:
 *		-Golden Eye thrashes the TLB.  This causes the rdram to constantly
 *		be refreshed with data, even when it is not necessary.  A couple
 *		workarounds is to use a hack to prevent TLB Miss exceptions, and
 *		possibly a method of telling what part of TLB is mapping to rom
 *		automatically...  Discussion on this topic is needed.
 *		- Still need to test Conker and Perfect Dark.  I ought to perfect the
 *		selfmod detection code before I continue.
 *	
 *
 **************************************************************************/
/**************************************************************************
 *
 *	Build History:
 *		Build 1 : Caching Interpretive without compiled jumps
 *	
 *
 **************************************************************************/



#include <windows.h>
#include <stdio.h>
#include "WinMain.h"
#include "EmuMain.h"
#include "CpuMain.h"
#include "DynaMain.h"

// Insert possible user settings that can change the functional of the recompiler
/*
	Buffer Size
	Self-Mod Methods
	32bit/64bit jumps (or automatic)
*/
#define MAX_BLOCKS 50000

FILE *dynaLogFile = fopen ("d:\\dynalog.txt", "wt");
u32 compStart, compEnd;  // Compiler Range for determining if Branch/Jump should be local
u8 tempBuff[4*1024*1024]; // Just a 1MB block for now... ;-/
BUFFER_POINTER x86Buff;  
BUFFER_POINTER x86BlockPtr;
u32 blockCount, blockMax;
BlockType BlockInfo[MAX_BLOCKS];
int blockFound;

u8 *savebuff;

void DynaReset () {
	// Should allocate its own memory region...
	x86Buff.ptr = tempBuff; // TODO: This is only temporary until I get recompilation going well
	x86BlockPtr.ptr = tempBuff;
	blockCount = 0;
	if (savebuff == NULL) {
		savebuff = (u8*)malloc (8*1024*1024);
		memset (savebuff, 0, 8*1024*1024);
	}
	Debug (0, "DynaRec Reset!");
}
void GenerateTLBException (DWORD addr, DWORD type);
void R4KDyna_Execute (void) {
	// Local Variables

	u32 dwRDRAMStartLoc;
	u8 *startPtr;
	// Reset the Dynarec...
	if (x86BlockPtr.ptr == NULL) {
		x86Buff.ptr = tempBuff; // TODO: This is only temporary until I get recompilation going well
		x86BlockPtr.ptr = tempBuff;
		blockCount = 0;
	}

	//CodeBlock = (u32 *)dwRDRAMStartLoc;
	// TODO: Get rid of the extra GenerateCRC...  Unnecessary when already computed
	while (!cpuIsReset) {
		//dwRDRAMStartLoc = (u32)rdram + (TLBLUT[pc>>12] & 0x1fffffff) + (0xfff & pc); // ***Temporary***
/*
		while (((pc & 0xFF000000) != 0x15000000)) {
		InterruptTime -= RegSettings.VSyncHack;

		if ((InterruptTime <= 0))
			CheckInterrupts();
		extern u8 *rdram;
		if (TLBLUT[pc>>12] > 0xFFFFFFF0) {
			QuickTLBExcept ();
			//((u32*)&sop)[0] = vr32(pc); // Should Generate an Exception
//			__asm int 3;
		}// else {
			((u32*)&sop)[0] = ((u32 *)returnMemPointer(TLBLUT[pc>>12]+(pc & 0xfff)))[0];
		pc+=4;
		r4300i[sop.op]();
		if (CpuRegs[0] != 0) {
			CpuRegs[0] = 0;
		}
		}
*/
		while (TLBLUT[pc>>12] > 0xFFFFFFF0) {
			QuickTLBExcept();
		}
		dwRDRAMStartLoc =	(u32)(rdram + 
				(TLBLUT[pc>>12] & 0x1fffffff) + 
				(0xfff & pc));
		//dwRDRAMStartLoc = (u32)((pc & 0x1fffffff) + rdram);
		blockFound = -1;
		
		if ((*(u32 *)(savebuff+(dwRDRAMStartLoc-(u32)rdram)) & 0xFFFF0000) == 0xECFF0000) {
			u32 t = *(DWORD *)(savebuff+(dwRDRAMStartLoc-(DWORD)rdram));
			blockFound = t & 0xFFFF;
			if (blockFound < (int)blockCount) {
				// Check CRC of this block now ;)
				if (BlockInfo[blockFound].RecOp != *(u32 *)(dwRDRAMStartLoc)) {
			//		Debug (0, "Block failed MEM check... at pc = %08X", pc);
					blockFound = -1;
				} else {
					if (CRCRec ((u8 *)dwRDRAMStartLoc, BlockInfo[blockFound].blocksize) != BlockInfo[blockFound].crc) {
						Debug (0, "Block failed CRC check... at pc = %08X", pc);
						blockFound = -1;
					}
				}
			} else {
				blockFound = -1;
			}
		}//*/
		
		//x86BlockPtr.ptr = x86Buff.ptr;
		//blockCount = 0;
		/*for (s32 x = blockCount-1; x >= 0; x--) {
			if (pc == BlockInfo[x].target) {
				if (GenerateCRC ((u8 *)dwRDRAMStartLoc, BlockInfo[x].blocksize) == BlockInfo[x].crc) {
					blockFound = x;
					break;
				}
			}
		}//*/
		if (blockFound < 0) { // Block Not Found...
			if (blockMax == (MAX_BLOCKS-1)) { // Do some cleanup!!!

			}
			// Compile Block here...
			BlockInfo[blockCount].target = pc; // TODO: The real target should be an already TLB'd address
			BlockInfo[blockCount].mem = x86BlockPtr.ubPtr;
			//...
			if (DynaCompileBlock(blockCount) == true) {
				blockFound = blockCount;
				blockCount++;
				//Debug (0, "Compiling Block from %08X..%08X\n", pc, pc + BlockInfo[blockFound].blocksize);
				//Debug (0, "Placed stamp at: %08X", ((u32)dwRDRAMStartLoc - (u32)rdram) | 0x80000000);
				BlockInfo[blockFound].RecOp = *(u32 *)(dwRDRAMStartLoc);
				*(u32 *)(savebuff+(dwRDRAMStartLoc-(u32)rdram)) = 0xECFF0000 | blockFound;
			} else {
				continue;
			}
		} else {
			//Debug (0, "Compiled Block Found at %08X\n", pc);
		}
		startPtr = BlockInfo[blockFound].mem;
		__asm {
			mov eax, dword ptr [startPtr];
			call eax;
		}
		
		InterruptTime -= (RegSettings.VSyncHack * ((BlockInfo[blockFound].blocksize/4)));

		if ((InterruptTime <= 0))
			CheckInterrupts();
		if ((u32)x86BlockPtr.ptr > (u32)(tempBuff+4*1024*1024-32767)) {
			x86BlockPtr.ptr = x86Buff.ptr = tempBuff; // Clear code cache...
			blockCount = 0;
			memset (savebuff, 0, 8*1024*1024);
			Debug (0, "Cleared Code Cache...");
		}
		if (blockCount > MAX_BLOCKS) {
			x86BlockPtr.ptr = x86Buff.ptr = tempBuff;
			blockCount = 0;
			Debug (0, "Cleared Code Cache...");
			memset (savebuff, 0, 8*1024*1024);
		}
	}
}
extern int inDelay;
void RecOp (void) { // used mostly for delay slots I think...
	u32 cmd = *(u32 *)&sop;
	if ((cmd & 0xFFFF0000) != 0xECFF0000)
		__asm int 3;
	cmd &= 0xFFFF;
//	if ((BlockInfo[cmd].target != pc) && (BlockInfo[cmd].target != pc-4))
//		__asm int 3;
	//if (inDelay != 1)
	//	__asm int 3;
	*(u32 *)&sop = BlockInfo[cmd].RecOp;
	r4300i[sop.op]();
	//Debug (0, "Forced to use Recovery op at %08X", pc-4);
}


unsigned long CRCRec (unsigned char *data, int size) {


	register unsigned long crc;
	int c = 0;

	crc = 0xFFFFFFFF;
	u8 temp[4];
/*	__asm {
		mov eax, dword ptr [c];
		mov esi, dword ptr [data];
		add esi, eax;

		cmp eax

		movq mm0, qword ptr [esi];
		movq mm2, qword ptr [esi+8];
		movq mm4, qword ptr [esi+16];
		movq mm6, qword ptr [esi+24];

	}*/
	while(c<size) {
		u32 t = *(u32 *)(data+c);
		//crc = (crc >> 8) | (crc << 24);
		crc ^= t;
		c+=4;
	}
	return( crc^0xFFFFFFFF );
}


/* 
				movq mm0, qword ptr [edx]
				movq mm2, qword ptr [edx+8]
				movq mm4, qword ptr [edx+16]
				movq mm6, qword ptr [edx+24]

				movq mm1, mm0
				psrlw mm0, 8
				psllw mm1, 8
				por mm0, mm1

  
	
	  Ideas

Loop:

  Check if current pc has already been compiled
  Validate the memory region using simple rdram overwriting technique (???)
  If memory check fails, then recompile region and invalidate invald region
  if memory check succeeds, then execute block
  goto loop

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
  execute block
  goto Loop
*/
