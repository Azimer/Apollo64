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
 *		08-04-02	Initial Version
 *
 **************************************************************************/
/**************************************************************************
 *
 *  Notes:
 *	
 *
 **************************************************************************/



#include <windows.h>
#include <stdio.h>
#include "WinMain.h"
#include "EmuMain.h"
#include "CpuMain.h"
#include "DynaCodes.h" // Dynarec X86 Codes and Support Function

// Fake ops
// 0x1C - 0x70 - HLE Patch
// 0x1D - 0x74 - Optimization
// 0x1E - 0x78 - Optimization
// 0x1F - 0x7C - Recompiled Block

FILE *dynaLogFile = fopen ("d:\\dynalog.txt", "wt");
u32 compStart, compEnd;  // Compiler Range for determining if Branch/Jump should be local
u8 tempBuff[4*1024*1024]; // Just a 1MB block for now... ;-/
BUFFER_POINTER x86Buff;  
BUFFER_POINTER x86BlockPtr;

/********* These functions need to go somewhere else! ********/

// TODO: I need to detect ERET and COP1 Jumps!
bool isJump (u32 code) { // JAL and JALR are not included since they are special.
	return (
		((code & 0xFC000000) == 0x08000000) // J
		);
/*
	retVal = 
    opSPECIAL,	opREGIMM,	opJ,	opJAL,		opBEQ,	opBNE,	opBLEZ,	opBGTZ, //00-07
    opADDI,		opADDIU,	opSLTI,	opSLTIU,	opANDI,	opORI,	opXORI,	opLUI,	//08-0F
    opCOP0,		opCOP1,		opCOP2,	opNI,		opBEQL,	opBNEL,	opBLEZL,opBGTZL,//10-17
// Mask RegImm with FC1F0000
// RegImm
    opBLTZ,		opBGEZ,		opBLTZL,	opBGEZL,	opNI,	opNI,	opNI,	opNI,
    opTGEI,		opTGEIU,	opTLTI,		opTLTIU,	opTEQI,	opNI,	opTNEI,	opNI,
    opBLTZAL,	opBGEZAL,	opBLTZALL,	opBGEZALL,	opNI,	opNI,	opNI,	opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,	opNI,	opNI,	opNI,*/
}

bool isBranch (u32 code) {
	return (
		((code & 0xFC000000) == 0x10000000) || // BEQ
		((code & 0xFC000000) == 0x14000000) || // BNE
		((code & 0xFC000000) == 0x18000000) || // BLEZ
		((code & 0xFC000000) == 0x1C000000) || // BGTZ
		((code & 0xFC000000) == 0x50000000) || // BEQL
		((code & 0xFC000000) == 0x54000000) || // BNEL
		((code & 0xFC000000) == 0x58000000) || // BLEZL
		((code & 0xFC000000) == 0x5C000000) || // BGTZL
		((code & 0xFC1F0000) == 0x04000000) || // BLTZ
		((code & 0xFC1F0000) == 0x04010000) || // BGEZ
		((code & 0xFC1F0000) == 0x04020000) || // BLTZL
		((code & 0xFC1F0000) == 0x04030000) || // BGEZL
		((code & 0xFC1F0000) == 0x04100000) || // BLTZAL
		((code & 0xFC1F0000) == 0x04110000) || // BGEZAL
		((code & 0xFC1F0000) == 0x04120000) || // BLTZALL
		((code & 0xFC1F0000) == 0x04130000)    // BGEZALL
		);
}

u32 maskImmediate (u32 op) {
	static u32 prev = 0;
	switch (op >> 26) {
		case 0x03:
		case 0x04:
			op &= 0xFC000000;
		break;
		case 0x08: case 0x09: case 0x0D:
			if (prev == 0xF) { // If it's a LUI... then mask... This may not be enough?
				op &= 0xFFFF0000;
			}
		break; // Should these be masked ??
		case 0x0F:
			op &= 0xFFFF0000;
		break;
	}
	prev = (op >> 26);
	return op;
}

/********* These functions need to go somewhere else! ********/


void DisassembleRange (u32 Start, u32 End);
LinkInfo JumpTable[MAX_LOCS]; // Not sure how large a JumpTable to make atm...
LinkInfo BlockInfo[0x1000]; 
u32 DestTable[MAX_LINKS]; // Destination Table used for Interrupt checking and others
u32 LinkLocs = 0;
u32 DestCnt = 0;

void Sort (u32 *DTable, u32 DCnt) {
	bool isSorted = false;
	while (isSorted == false) {
		isSorted = true;
		for (int x = 0; x < (int)(DCnt-1); x++) { // Simple Bubble Sort
			if (DTable[x] > DTable[x+1]) {
				u32 t = DTable[x+1];
				DTable[x+1] = DTable[x];
				DTable[x] = t;
				isSorted = false;
			}
		}
	}
}

/*******  R4K Block Analysis ********

08-04-02 - Initial Version

*************************************/

void R4KDyna_Analysis (u32 dynaPC) {
	u32 codeSize = 0; // Size from start to JR
	u32 myPC = dynaPC;
	u32 code, crc;
	u32 MaxPC = dynaPC; // MaxPC
	u32 buff[MAX_LOCS]; // Hoping it is not larger then this...
	u32 Loc = 0;
	//DisassembleRange (dynaPC, dynaPC+0x1000);

// Step #1 - Find a suitable end to the procedure and build a static jump table
	compStart = myPC;
	DestCnt = 0;
	while (1) {
		if (Loc >= MAX_LOCS)
			__asm int 3;
        code = vr32 (myPC);
		// TODO: If a J is found... should be check its range?  If the range is too long
		// then we should stop at the J or if we detect no future code execution is
		// possible after the J
        buff[Loc++] = maskImmediate(code);
		if ((code & 0xFC00003F) == 0x00000008) { // Find a JR (Not necessarily a JR RA)
			//if (myPC >= MaxPC) { // Make sure it is the FINAL JR in the block
				myPC += 8; // Moves past delay slot
				break;
			//}
		}
		if ((code & 0xFC00003F) == 0x00000009) { // Find a JALR
			//if (myPC >= MaxPC) { // Make sure it is the FINAL block
				myPC += 8; // Moves past delay slot
				break;
			//}
		}
		
		if ((code & 0xFC000000) == 0x08000000) { // Find an unconditional J
			//if (myPC >= MaxPC) {
				myPC += 8;
				break;
			//}
		}
		
		if ((code & 0xFC000000) == 0x0C000000) { // Find an unconditional JAL
			//if (myPC >= MaxPC) {
				myPC += 8;
				break;
			//}
		}
		if ((code & 0xFC000000) == 0x7C000000) { // Find recompiled block
			//DisassembleRange (compStart, myPC+0x1000);
			Debug (0, "This could be a problem! : %08X", myPC);
			Debug (0, "Start = %08X", compStart);
			//__asm int 3;
			myPC -= 4;
			break;
		}
		if (isBranch(code) | isJump(code)) {
			myPC += 8;
			break;
		}
		
		myPC+= 4;


		// Purpose is to keep track where branches are
		if (isBranch(code) | isJump(code)) {
			if (DestCnt >= MAX_LINKS)
				__asm int 3;
			DestTable[DestCnt++] = myPC+(((s16)(code&0xFFFF))<<2);
			if (DestTable[DestCnt-1] > MaxPC) { // Branches should never cross functions
				MaxPC = DestTable[DestCnt-1];
			}
			DynaLog ("Branch found at %08X to %08X", myPC, DestTable[DestCnt-1]);
		}
	}
	Sort (DestTable, DestCnt);
	compEnd = myPC;
	codeSize = (myPC - dynaPC);
	DynaLog ("JR Detected at: %08X - CodeSize: %i", myPC, codeSize);
	DynaLog ("CompStart %08X - CompEnd %08X", compStart, compEnd);

// Step #2 - CRC the Function for HLE purposes...
	crc = GenerateCRC ((u8 *)buff, codeSize);
	DynaLog ("Function CRC: %08X", crc);
}

/*******  R4K Optimize Block ********

08-05-02 - Initial Version

*************************************/
/* Primary Function of this procedure is to generate a script for the compiler
	Optimization Features:
	Static Linkage			- Not Done
	Instruction Reordering	- Not Done
	Constant Pairing		- Not Done
	Function Inlining		- Not Done
	HLE Functions			- Not Done
*/

u32 *R4KDyna_OptimizeBlock (u32 DynaPC) {
	// TODO: Still need to implement :-)
	return (u32 *)(rdram + (TLBLUT[DynaPC>>12] & 0x1fffffff) + (DynaPC & 0xfff));
}
void R4KDyna_Link () { // This is only called for link ahead ops
	u32 x, t;
	for (x = 0; x < LinkCnt; x++) {
		for (t = 0; t < LinkLocs; t++) {
			if (LinkTable[x].target == JumpTable[t].target) {
				//int n = (int)JumpTable[t].mem - ((int)LinkTable[x].mem + 1);
				SetBranch32b (LinkTable[x].mem, JumpTable[t].mem);
				//SetBranch8b (LinkTable[x].mem, JumpTable[t].mem);
				//if ((n > 127) || (n < -128)) // Then we have a problem... but fixable with a little effort
				//	__asm int 3;
				break;
			}
		}
// Games that do this...
// Perfect Dark, Super Smash Bros, Mario 64, 1080
		if (t == LinkLocs) {// Perfect Dark does this unfortunately! :(
			//DisassembleRange (LinkTable[x].target-0x10, LinkTable[x].target+0x10);
			//Debug (1, "Static Link to Delay Slot not Implemented!");
			//__asm int 3; // Then it must be looking to link to a delay slot
			blockFailed = true;
		}
	}
	LinkCnt = 0;
	LinkLocs = 0;
}

int theCount;
/*
void R4KDyna_temp() {
	InterruptTime -= theCount;
	if ((InterruptTime <= 0)) {
		CheckInterrupts();
	}
}
*/

//char *InterruptExecute = ;
void fucker () {
	Debug (0, "Interrupt Executed: %08X\n", pc);
}
void fucker2 () {
	Debug (0, "------------------- %08X\n", pc);
}
void __declspec ( naked ) HandleInterrupt() {
	__asm {
//		call fucker
		mov eax, dword ptr [pc]; // Load up the PC
		push eax; // Save our pc
		call CheckInterrupts;
		pop eax;  // Load our pc
		cmp eax, dword ptr [pc]; // Compare it to the PC
		je NoChange; // No interrupt happen
		// TODO: Saving would be a good thing here instead of junking the return!
		pop eax; // junk the return
	}
//	Debug (0, "Handling...");
	__asm {
NoChange:
//		call fucker2
		ret; // Return to DynaLoop
	}
}

void R4KDyna_CheckInterrupts(u32 opCount) {
	
	u8 *JumpByte, *JumpByte2;
	// TODO: Later on... we need to make this faster...
	SubConst8ToVariable ((u8)(opCount*RegSettings.VSyncHack), &InterruptTime, "InterruptTime");
	JgLabel8("No Interrupt", 0);
	JumpByte = x86BlockPtr.ubPtr - 1;
		MoveConstToVariable (pc, &pc, "pc"); // Store PC for call to function
		CallFunctionDirect(HandleInterrupt, "HandleInterrupt");
	DynaLog ("No Interrupt");
	SetBranch8b (JumpByte, x86BlockPtr.ptr);
	
}
u32 JalEntry;

void R4KDyna_GenerateCode (u32 *src) {
	u32 *end = (u32 *)((u32)src+(compEnd-compStart));
	u32 *start = src;
	u32 Cntr = 0, opCount = 0;
	char buff[256];
	DynaLog ("Dumping R4K Code to disasm.txt...");
	//DisassembleRange (compStart, compEnd);
	pc = compStart;
	JalEntry = 0;
	
	for (; start < end; start++) {
		if (Cntr < DestCnt) {
			if (pc == DestTable[Cntr]) { // Jump Location
				Cntr++;
				SubConst8ToVariable ((u8)(opCount*RegSettings.VSyncHack), &InterruptTime, "InterruptTime");
				while (pc == DestTable[Cntr]) // Skip Duplicate Labels
					Cntr++;
				opCount = 0;
				// Add Interrupt Stuff here
				DynaLog ("*** Local Link ***");
			}
		}
		OpcodeLookup (pc, buff);
		DynaLog ("%08X	%s", pc, buff);
		JumpTable[LinkLocs].target = pc;
		JumpTable[LinkLocs].mem = x86BlockPtr.ubPtr;
		LinkLocs++;
		// *** Everything above this will attach itself to end of previous opcode ***
		if (isBranch(*start) || isJump(*start)) { // Check interrupts at each branch
			opCount++; // Hack ;)
			DynaLog ("*** Jump ***");
			R4KDyna_CheckInterrupts(opCount);
			opCount = 0;
		}
/*
		if (pc == 0x80203558) {
			Int3 ();
		}*/
		opCount++;
		pc += 4; // PC is incremented before Execution of op in interpreter
		{
			u32 *saveStart = start;
			u8  *savePtr = x86BlockPtr.ubPtr;
			start = R4KDyna_RecompileOpcode (start);
			if (LinkCnt != 0)
				__asm int 3;
			//*saveStart = 0x7C000000 + (savePtr - x86Buff.ubPtr);
			//JalEntry = 0;
			//Debug (0, "Placed JAL Marker at %08X", pc-4);
		}
		//start = R4KDyna_RecompileOpcode (start);
		
		//DynaLog ("Code: %08X", *start);
	}
	MoveConstToVariable (pc, &pc, "pc");
	Ret();
	pc = compEnd; // One past delay slot
	R4KDyna_Link ();
}

/*******  R4K CompileBlock ********

08-04-02 - Initial Version

*************************************/

int blockCount = 0; // Allows me to incrementally Dynarec blocks... Easier debugging

void R4KDyna_CompileBlock (u32 dynaPC) {
	u32 *script;
	// First Pass - Analyze block for Branches, Jumps, and HLE
	R4KDyna_Analysis (dynaPC);

	// Second Pass - Optimization Analysis and Application
	script = R4KDyna_OptimizeBlock (dynaPC);

	// Third Pass - Recompilation
	R4KDyna_GenerateCode (script);
	//vr32 (dynaPC);

	if (blockFailed == true) {
		Debug (0, "Block Recompilation Failure... going Interpretive");
		Debug (0, "Block Count: %i", blockCount);
	}
}

/*******  R4K Dynarec Execute ******

08-04-02 - Initial Version

*************************************/
	bool lastBuild = true;

void R4KDyna_Execute (void) {
	u32 *CodeBlock;
	u32 lastPC;
	u8 *startPtr;
	int x;
	DynaLog ("Entering Dynarec Loop");
	
	u32 dwRDRAMStartLoc;
	if ((pc == 0x80000000) || (pc == 0x80000004)) { // CIC-NUS-6102, 6103, 6105, 6106 Start Here
		x86Buff.ptr = tempBuff; // TODO: This is only temporary until I get recompilation going well
		x86BlockPtr.ptr = tempBuff;
		LinkCnt = 0; LinkLocs = 0; DestCnt = 0;
		Debug (0, "DynaRec Reset!");
	}
	if (x86BlockPtr.ptr == NULL)
		__asm int 3;

	while (cpuIsReset == false) { // Keep going until we need to break
		// Check for compiled block
		// TLBLut - 0xFFFFFFFF = TLB Miss Exception
		//        - 0xFFFFFFFE = TLB Invalid Exception
		//        - 0xFFFFFFFD = ROM I/O Unavailable (for dynamically loaded ROM)
		//        - 0xFFFFFFFC = Protected Memory Region (Recompiled Code)
		dwRDRAMStartLoc = TLBLUT[pc>>12];
		if (dwRDRAMStartLoc > 0xFFFFFFF0) { // This will need to be changed for ProtectMem
			__asm int 3;  // I can't handle exceptions yet... I need to research a LOT more!
		}
		// TODO: Need to make this access rdram directly once new TLB is complete...
		dwRDRAMStartLoc = (u32)rdram + (TLBLUT[pc>>12] & 0x1fffffff) + (0xfff & pc); // ***Temporary***

		CodeBlock = (u32 *)dwRDRAMStartLoc;
		u32 s;
		// TODO: We should make every rdram location in the code section
		// take on a 0x7C------ prefixed value.
//		x86BlockPtr.ptr = tempBuff;
// Pong dies at 1b8
//		0x1b0 is ok
//		0x1b1 is the bad block
#define BSTOP 0x731
		lastPC = pc;
		for (x = 0; x < blockCount; x++) {
			if (pc == BlockInfo[x].target) {
				break;
			}
		}

//		x86BlockPtr.ubPtr = x86Buff.ubPtr;
		if (1) {
//		if (x == blockCount) { 
//		if ((CodeBlock[0] & 0xFF000000) != 0x7C000000) {
//			if (blockCount == BSTOP) {
//				Debug (0, "***Stopping Recompiler Here***");
//				Debug (0, "PC = %08X", pc);
//				blockCount++;
//			}
			
//			if (pc == BAD_PC)
//				blockFailed = true;
//			if (pc == 0x802005e8)
//				__asm int 3;


			if (blockFailed == false) {
				x86BlockPtr.ubPtr = x86Buff.ubPtr;
				startPtr = x86BlockPtr.ubPtr;
				InterruptTime -= RegSettings.VSyncHack;
				if ((InterruptTime <= 0))
					CheckInterrupts();
				((u32*)&sop)[0] = ((u32 *)returnMemPointer(TLBLUT[pc>>12]+(pc & 0xfff)))[0];
				dwRDRAMStartLoc = (u32)(returnMemPointer(TLBLUT[pc>>12]+(pc & 0xfff)));
				pc += 4;
				MoveConstToVariable (pc, &pc, "pc");


				u32 *R4KDyna_RecompileOpcode (u32 *);
				R4KDyna_RecompileOpcode ((u32 *)dwRDRAMStartLoc);
				Ret (); // Return to DynaLoop
				__asm {
					mov eax, dword ptr [startPtr];
					call eax;
				}
/*
//			if (blockCount < BSTOP) { // 0x2a
				u32 savePC = pc;
				DynaLog ("Compiling code block at: %08X", pc);
				startPtr = x86BlockPtr.ubPtr;

				BlockInfo[blockCount].mem = startPtr;
				BlockInfo[blockCount].target = pc;
				R4KDyna_CompileBlock (pc);
				if (blockFailed == true) {
					//__asm int 3;
					blockCount = 0x0;//0x7FFFFFFF;
					pc = savePC; // No furthur execution possible
				} else {
					Debug (0, "Build: %08X..%08X", compStart, compEnd);
					// TODO: Mark block as compiled
					//CodeBlock[0] = 0x7C000000 + (startPtr - x86Buff.ubPtr); // Mark Block as compiled
					blockCount++;
					// TODO: I still need to save the x86BlockPtr somewhere
					__asm {
						mov eax, dword ptr [startPtr];
						call eax;
					}
				}*/
			} else { // Interprete the Block
				do {
					InterruptTime -= RegSettings.VSyncHack;

					if ((InterruptTime <= 0))
						CheckInterrupts();
					if (TLBLUT[pc>>12] > 0xFFFFFFF0) {
						((u32*)&sop)[0] = s = vr32(pc); // Should Generate an Exception
						__asm int 3;
					} else {
						((u32*)&sop)[0] = s = ((u32 *)returnMemPointer(TLBLUT[pc>>12]+(pc & 0xfff)))[0];
					}
					pc+=4;

					r4300i[sop.op]();

					if (CpuRegs[0] != 0) {
						void r4300i_HandleErrorRegister(void);
						r4300i_HandleErrorRegister();	
					}

				} while ((s & 0xFC00003F) != 0x00000008); // Stop at JR
			}
			//break;// Do not execute compiled function yet :)
		} else {
			//Debug (0, "Block: %08X", pc);
			//startPtr = (CodeBlock[0] - 0x7C000000) + x86Buff.ubPtr;
			startPtr = BlockInfo[x].mem;
			//if ((CodeBlock[0] - 0x7C000000) > 2 * 1024 * 1024)
			//	__asm int 3;
			//CodeBlock[0] = 0x7C000000 + (startPtr - x86Buff.ubPtr); // Mark Block as compiled
			lastBuild = false;
			__asm {
				mov eax, dword ptr [startPtr];
				call eax;
			}
			//Debug (0, "Exiting code block at: %08X", pc);
		}
	}

	DynaLog ("Leaving Dynarec Loop");
}

// Temporary Repository of Information and Ideas

/* - This is going to be a bitch for TLB Miss Exceptions... but I will deal with it later!

  LW - Nonconstant

  mov tmpreg, R4KReg (or regcache)
  add tmpreg, offset
  mov tmpreg2, tmpreg
  and tmpreg2, 0FFFh
  shr tmpreg, 12
  mov tmpreg3, dword ptr [TLBLut + tmpreg];
  mov R4KReg, dword ptr [tmpreg2+tmpreg3];

  mov tmpreg, address
*/




	/*
	__try {
		u32 t = 0;
		__asm {
			mov eax, 0FFFFFFFFh;
			mov eax, dword ptr [eax];
			mov edx, 01234h;
			mov dword ptr [t], edx;
		}
		DynaLog ("t = %08X", t);

				
	} __except ( HandleException(GetExceptionInformation()) ) {
	}
	*/
	/*
	__except ( HandleWin32Exception(GetExceptionInformation()) ) {
		short* hh = (short*)intelException[0].ExceptionRecord[0].ExceptionAddress;
		if (hh[0]==0x008B) {
			intelException[0].ContextRecord[0].Eax = 0;
			intelException[0].ContextRecord[0].Eip += 2;
		} else if (hh[0]==0x0889) {
			VirtualProtect((void *)(valloc+0x10000000), romsize, PAGE_NOACCESS, &old); // Ok.. next read is bad
			intelException[0].ContextRecord[0].Eip += 2;
		} else if (hh[0]==0x0888) {
			intelException[0].ContextRecord[0].Eip += 2;
		}
	}//*/

/*
extern LPEXCEPTION_POINTERS intelException;

u32 HandleException(LPEXCEPTION_POINTERS info) {
	static int t = 0;
	intelException = info;

	//Debug (1, "Exception = %08X", info[0].ExceptionRecord[0].ExceptionCode);

	short* hh = (short*)info[0].ExceptionRecord[0].ExceptionAddress;

	if (info[0].ExceptionRecord[0].ExceptionCode==EXCEPTION_ACCESS_VIOLATION) {
		if (t == 1) {
			Debug (0, "Fuck!");
			return EXCEPTION_EXECUTE_HANDLER;
		}
		t = 1;
		Debug (0, "Access Violation!!! - %X", hh[0]);
		info[0].ContextRecord[0].Eip += 2;
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}
*/
