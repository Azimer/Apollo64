#include "../common.h"
#include "Recompiler.h"
#include "RecompilerDebug.h"
#include "X86Regs.h"
#include "X86Ops.h"
#include "../EmuMain.h"


/*
	Recompiler files:
	Recompiler.cpp		- Includes the main recompiler loop
	RecompilerOps.cpp	- Includes opcode for recompiler
	RecompilerRegs.cpp	- Includes Register stuff for recompiler
	RecompilerOpt.cpp	- Includes optimization stuff
	RecompilerDebug.cpp - Includes all debugging tools

  */

u32 CheckBlock () {
	return 0;
}

u32 CreateBlock () {
	// 0x80322800 - osRecv
	void ScanHLEFunction ();
	ScanHLEFunction();
	//AnalyzeBlock(pc);
	return 0;
}

void ExecuteBlock (u32 Block) {
}

void RecompilerMain () {
	u32 retVal;
//	while (cpuIsReset == false) {
		retVal = CheckBlock (); // Check Block to see if it is already compiled.
		if (retVal == 0) {
			retVal = CreateBlock (); // Create a new Block
		}
		ExecuteBlock (retVal); // Execute Block
		// Do stuff here...
//	}
}

/*

  JAL - This function will look for HLE functions and analyze the function from start
        to JR RA.
  JALR - Same as above
  JR (non RA) - This function will compile until it reaches unreachable code.
  J - Same as above
  EXECEPTION happens - The function will compile untile it reaches unreachable code
  in a special buffer set aside just for the exception handler.
*/