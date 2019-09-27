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

extern void (*r4300i[0x40])();
extern void (*special[0x40])();
extern void (*regimm[0x20])();
extern void (*MmuSpecial[0x40])();
extern void (*MmuNormal[0x20])();
extern void (*FpuCommands[0x40])();
extern void CheckInterrupts(void);
extern u8 *rdram;

unsigned long GenerateCRC (unsigned char *data, int size);



/********* These functions need to go somewhere else! ********/
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
u32 JumpTable[0x500]; // Not sure how large a JumpTable to make atm...

/*******  R4K Block Analysis ********

08-04-02 - Initial Version

*************************************/

void R4KDyna_Analysis (u32 dynaPC) {
	u32 codeSize = 0; // Size from start to JR
	u32 myPC = dynaPC;
	u32 code, crc;
	u32 JLoc = 0; // Jump Table Location
	u32 buff[0x100]; // Hoping it is not larger then this...
	u32 Loc = 0;
	//DisassembleRange (dynaPC, dynaPC+0x1000);

// Step #1 - Find a suitable end to the procedure and build a static jump table
	while (1) {
        code = vr32 (myPC);
		// TODO: If a J is found... should be check its range?  If the range is too long
		// then we should stop at the J or if we detect no future code execution is
		// possible after the J
        buff[Loc++] = maskImmediate(code);
		if ((code & 0xFC00003F) == 0x00000008) { // Find a JR (Not necessarily a JR RA)
			myPC += 4;
			break;
		}
		/* NOT NEEDED!!!!
		// Purpose of the Jump Table is to tell the compiler which jumps are local
		if (isBranch(code) || isJump(code)) {
			JumpTable[JLoc++] = myPC+4+(((s16)(code&0xFFFF))<<2);
			Debug (0, "Jump found at %08X to %08X", myPC, JumpTable[JLoc-1]);
		}
		*/
		myPC+= 4;
	}
	codeSize = (myPC - dynaPC);
	Debug (0, "JR Detected at: %08X - CodeSize: %i", myPC, codeSize);

// Step #2 - CRC the Function for HLE purposes...
	crc = GenerateCRC ((u8 *)buff, codeSize);
	Debug (0, "Function CRC: %08X", crc);
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

u32 *R4K_OptimizeBlock (u32 DynaPC) {
	// This is where I left off last!
	return NULL;
}

/*******  R4K CompileBlock ********

08-04-02 - Initial Version

*************************************/

void R4KDyna_CompileBlock (u32 dynaPC) {
	u32 *script;
	// First Pass - Analyze block for Branches, Jumps, and HLE
	R4KDyna_Analysis (dynaPC);

	// Second Pass - Optimization Analysis and Application
//	script = R4KDyna_OptimizeBlock (dynaPC);

	// Third Pass - Recompilation
	//R4KDyna_GenerateCode (script);
	//vr32 (dynaPC);
}

/*******  R4K Dynarec Execute ******

08-04-02 - Initial Version

*************************************/
void R4KDyna_Execute (void) {
	u32 *CodeBlock;
	Debug (0, "Entering Dynarec Loop");
	
	u32 dwRDRAMStartLoc;
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
		dwRDRAMStartLoc = (u32)rdram + (TLBLUT[pc>>12] & 0x1fffffff); // ***Temporary***

		CodeBlock = (u32 *)dwRDRAMStartLoc;
		if (CodeBlock[0] != 0xFEEDBEEF) {
			Debug (0, "Compiling code block at: %08X", pc);
			R4KDyna_CompileBlock (pc);
			return;// No compile function yet :)
		} else {
			Debug (0, "Executing code block at: %08X", pc);
			__asm int 3;
		}
	}

	Debug (0, "Leaving Dynarec Loop");
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
		Debug (0, "t = %08X", t);

				
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
