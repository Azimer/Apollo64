#ifndef __RECOMPILERDEBUG_DOT_H__
#define __RECOMPILERDEBUG_DOT_H__

#include <windows.h>
#include "../common.h"

// Local Globals:

extern bool inDebugger;		// Are we inside the Debugger?

extern u64 DebugRegs[32];   // A set of Registers for debugging (to check Reg Cache)


// Local Functions:

void OpenDebuggerWindow (HINSTANCE hApp, HWND hParent);


// Legacy Functions

void OpcodeLookup (DWORD addy, char *out);


#endif