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
 *		06-20-00	Initial Version (Andrew)
 *
 **************************************************************************/
/**************************************************************************
 *
 *  Notes:
 *	
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


#define RD() ((opcode & 0xffff) >> 11)
#define RT() ((opcode >> 16) & 0x1f)
#define RS() ((opcode >> 21) & 0x1f)
#define SA() ((opcode >> 6) & 0x1f)
#undef opcode

void Fetch( char* buffer, unsigned int opcode, unsigned int location, char fmt );
HWND DebuggerHwnd = NULL;
u32 DebuggerLocation = 0x80000040;

/*
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

*/

char *r4kreg[0x20] = {
  "R0", "AT", "V0", "V1", "A0", "A1", "A2", "A3", "T0", "T1", "T2",
  "T3", "T4", "T5", "T6", "T7", "S0", "S1", "S2", "S3", "S4", "S5",
  "S6", "S7", "T8", "T9", "K0", "K1", "GP", "SP", "S8", "RA"
};

char *mmuregs[0x20] = {
  "Index", "Random", "EntryLo0", "EntryLo1", "Context", "PageMask", "Wired",
  "RESERVED", "BadVAddr", "Count", "EntryHi", "Compare", "Status", "Cause",
  "ExceptPC", "PRevID", "Config", "LLAddr", "WatchLo", "WatchHi", "XContext", "RESERVED",
  "RESERVED", "RESERVED", "RESERVED", "RESERVED", "PErr", "CacheErr", "TagLo",
  "TagHi", "ErrorEPC", "RESERVED"
};

void COP0Lookup (char *out) {
	switch (sop.rs) {
	case 0x00: sprintf (out, "MFC0	%s, %s", r4kreg[sop.rt], mmuregs[sop.rd]); break;
	case 0x04: sprintf (out, "MTC0	%s, %s", r4kreg[sop.rt], mmuregs[sop.rd]); break;
	case 0x16: // Special
		switch (sop.func) {
			case 0x01: sprintf (out, "TLBR"); break;
			case 0x02: sprintf (out, "TLBWI"); break;
			case 0x06: sprintf (out, "TLBWR"); break;
			case 0x08: sprintf (out, "TLBP"); break;
			case 0x18: sprintf (out, "ERET"); break;
			default: sprintf (out, "UKNWN	%08X", ((u32 *)&sop)[0]);
		}
	default: sprintf (out, "CUKNWN	%08X", ((u32 *)&sop)[0]);
	}
}

void COP1Lookup () {
}

void REGIMMLookup (char *out, u32 addy) {
	switch (sop.rt) {
		case 0x00: sprintf (out, "BLTZ	%s, 0x%08X", r4kreg[sop.rs], addy+4+(((s16 *)&sop)[0]<<2)); break;
		case 0x01: sprintf (out, "BGEZ	%s, 0x%08X", r4kreg[sop.rs], addy+4+(((s16 *)&sop)[0]<<2)); break;
		case 0x02: sprintf (out, "BLTZL	%s, 0x%08X", r4kreg[sop.rs], addy+4+(((s16 *)&sop)[0]<<2)); break;
		case 0x03: sprintf (out, "BGEZL	%s, 0x%08X", r4kreg[sop.rs], addy+4+(((s16 *)&sop)[0]<<2)); break;
		case 0x08: sprintf (out, "TGEI	%s, 0x%08X", r4kreg[sop.rs], (((u16 *)&sop)[0]<<2)); break;
		case 0x09: sprintf (out, "TGEIU	%s, 0x%08X", r4kreg[sop.rs], (((u16 *)&sop)[0]<<2)); break;
		case 0x0A: sprintf (out, "TLTI	%s, 0x%08X", r4kreg[sop.rs], (((u16 *)&sop)[0]<<2)); break;
		case 0x0B: sprintf (out, "TLTIU	%s, 0x%08X", r4kreg[sop.rs], (((u16 *)&sop)[0]<<2)); break;
		case 0x0C: sprintf (out, "TEQI	%s, 0x%08X", r4kreg[sop.rs], (((u16 *)&sop)[0]<<2)); break;
		case 0x0E: sprintf (out, "TNEI	%s, 0x%08X", r4kreg[sop.rs], (((u16 *)&sop)[0]<<2)); break;
		case 0x10: sprintf (out, "BLTZAL	%s, 0x%08X", r4kreg[sop.rs], addy+4+(((s16 *)&sop)[0]<<2)); break;
		case 0x11: if (sop.rs) sprintf (out, "BGEZAL	%s, 0x%08X", r4kreg[sop.rs], addy+4+(((s16 *)&sop)[0]<<2));
				   else sprintf (out, "BAL	0x%08X", addy+4+(((s16 *)&sop)[0]<<2)); break;
		case 0x12: sprintf (out, "BLTZALL	%s, 0x%08X", r4kreg[sop.rs], addy+4+(((s16 *)&sop)[0]<<2)); break;
		case 0x13: sprintf (out, "BGEZALL	%s, 0x%08X", r4kreg[sop.rs], addy+4+(((s16 *)&sop)[0]<<2)); break;
		default:
			sprintf (out, "*********REGIMM	%08X********", sop.rt); break;
	}
	
}
/*
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
*/
void SPECIALLookup (char *out) {
	switch (sop.func) {
		case 0x00: if (((u32*)&sop)[0] == 0) sprintf (out, "NOP");
				   else sprintf (out, "SLL	%s, %s, 0x%X", r4kreg[sop.rd], r4kreg[sop.rt], sop.sa); break;
		case 0x01: sprintf (out, "Unknown	%08X", ((u32 *)&sop)[0]); break;
		case 0x02: sprintf (out, "SRL	%s, %s, 0x%X", r4kreg[sop.rd], r4kreg[sop.rt], sop.sa); break;
		case 0x03: sprintf (out, "SRA	%s, %s, 0x%X", r4kreg[sop.rd], r4kreg[sop.rt], sop.sa); break;
		case 0x04: sprintf (out, "SLLV	%s, %s, %s", r4kreg[sop.rd], r4kreg[sop.rt], r4kreg[sop.rs]); break;
		case 0x06: sprintf (out, "SRLV	%s, %s, %s", r4kreg[sop.rd], r4kreg[sop.rt], r4kreg[sop.rs]); break;
		case 0x07: sprintf (out, "SRAV	%s, %s, %s", r4kreg[sop.rd], r4kreg[sop.rt], r4kreg[sop.rs]); break;
		case 0x08: sprintf (out, "JR	%s", r4kreg[sop.rs]); break;
		case 0x09: if (sop.rd == 31) sprintf (out, "JALR	%s", r4kreg[sop.rs]);
				   else sprintf (out, "JALR	%s, %s", r4kreg[sop.rd], r4kreg[sop.rs]); break;
		case 0x0C: sprintf (out, "SYSCALL	????"); break;
		case 0x0D: sprintf (out, "BREAK	????"); break;
		case 0x0F: sprintf (out, "SYNC	????"); break;
		case 0x10: sprintf (out, "MFHI	%s", r4kreg[sop.rd]); break;
		case 0x11: sprintf (out, "MTHI	%s", r4kreg[sop.rd]); break;
		case 0x12: sprintf (out, "MFLO	%s", r4kreg[sop.rd]); break;
		case 0x13: sprintf (out, "MTLO	%s", r4kreg[sop.rd]); break;
		case 0x18: sprintf (out, "MULT	%s, %s", r4kreg[sop.rs], r4kreg[sop.rt]); break;
		case 0x19: sprintf (out, "MULTU	%s, %s", r4kreg[sop.rs], r4kreg[sop.rt]); break;
		case 0x1A: sprintf (out, "DIV	%s, %s", r4kreg[sop.rs], r4kreg[sop.rt]); break;
		case 0x1B: sprintf (out, "DIVU	%s, %s", r4kreg[sop.rs], r4kreg[sop.rt]); break;
		case 0x1F: sprintf (out, "DDIVU %s, %s", r4kreg[sop.rs], r4kreg[sop.rt]); break;
		case 0x20: sprintf (out, "ADD	%s, %s, %s", r4kreg[sop.rd], r4kreg[sop.rs], r4kreg[sop.rt]); break;
		case 0x21: sprintf (out, "ADDU	%s, %s, %s", r4kreg[sop.rd], r4kreg[sop.rs], r4kreg[sop.rt]); break;
		case 0x22: sprintf (out, "SUB	%s, %s, %s", r4kreg[sop.rd], r4kreg[sop.rs], r4kreg[sop.rt]); break;
		case 0x23: sprintf (out, "SUBU	%s, %s, %s", r4kreg[sop.rd], r4kreg[sop.rs], r4kreg[sop.rt]); break;
		case 0x24: sprintf (out, "AND	%s, %s, %s", r4kreg[sop.rd], r4kreg[sop.rs], r4kreg[sop.rt]); break;
		case 0x25: sprintf (out, "OR	%s, %s, %s", r4kreg[sop.rd], r4kreg[sop.rs], r4kreg[sop.rt]); break;
		case 0x26: sprintf (out, "XOR	%s, %s, %s", r4kreg[sop.rd], r4kreg[sop.rs], r4kreg[sop.rt]); break;
		case 0x2A: sprintf (out, "SLT	%s, %s, %s", r4kreg[sop.rd], r4kreg[sop.rs], r4kreg[sop.rt]); break;
		case 0x2B: sprintf (out, "SLTU	%s, %s, %s", r4kreg[sop.rd], r4kreg[sop.rs], r4kreg[sop.rt]); break;
		default: sprintf (out, "**********SPECIAL	%08X***********", sop.func);
	}
}
/*
A4000898  r4300 00000014
A40004E8  r4300 00000015
A40009D0  r4300 00000024
A400098C  r4300 00000028
A40003F8  r4300 0000002F*/
void OpcodeLookup (DWORD addy, char *out) {
	((u32*)&sop)[0] = vr32(addy);
	switch (sop.op) {
		case 0x00: SPECIALLookup (out); break;
		case 0x01: REGIMMLookup (out, addy); break;
		case 0x02: sprintf (out, "J	%08X", ((addy+4) & 0xf0000000) + (((((u32 *)&sop)[0] << 2) & 0x0fffffff))); break;
		case 0x03: sprintf (out, "JAL	%08X", ((addy+4) & 0xf0000000) + (((((u32 *)&sop)[0] << 2) & 0x0fffffff))); break;
		case 0x04: if (sop.rt) sprintf (out, "BEQ	%s, %s, 0x%08X", r4kreg[sop.rs], r4kreg[sop.rt], addy+4+(((s16 *)&sop)[0]<<2));
				   else if (sop.rs) sprintf (out, "BEQZ	%s, 0x%08X", r4kreg[sop.rs], addy+4+(((s16 *)&sop)[0]<<2)); 
				   else sprintf (out, "B	0x%08X", addy+4+(((s16 *)&sop)[0]<<2)); break;
		case 0x05: if (sop.rt) sprintf (out, "BNE	%s, %s, 0x%08X", r4kreg[sop.rs], r4kreg[sop.rt], addy+4+(((s16 *)&sop)[0]<<2));
				   else sprintf (out, "BNEZ	%s, %08X", r4kreg[sop.rs], addy+4+(((s16 *)&sop)[0]<<2)); break;
		case 0x06: sprintf (out, "BLEZ	%s, %s, 0x%08X", r4kreg[sop.rs], r4kreg[sop.rt], addy+4+(((s16 *)&sop)[0]<<2)); break;
		case 0x07: sprintf (out, "BGTZ	%s, %s, 0x%08X", r4kreg[sop.rs], r4kreg[sop.rt], addy+4+(((s16 *)&sop)[0]<<2)); break;
		case 0x08: sprintf (out, "ADDI	%s, %s, 0x%04X", r4kreg[sop.rt], r4kreg[sop.rs], ((u16 *)&sop)[0]); break;
		case 0x09: sprintf (out, "ADDIU	%s, %s, 0x%04X", r4kreg[sop.rt], r4kreg[sop.rs], ((u16 *)&sop)[0]); break;
		case 0x0A: sprintf (out, "SLTI	%s, %s, 0x%04X", r4kreg[sop.rt], r4kreg[sop.rs], ((u16 *)&sop)[0]); break; 
		case 0x0B: sprintf (out, "SLTIU	%s, %s, 0x%04X", r4kreg[sop.rt], r4kreg[sop.rs], ((u16 *)&sop)[0]); break;
		case 0x0C: sprintf (out, "ANDI	%s, %s, 0x%04X", r4kreg[sop.rt], r4kreg[sop.rs], ((u16 *)&sop)[0]); break;
		case 0x0D: sprintf (out, "ORI	%s, %s, 0x%04X", r4kreg[sop.rt], r4kreg[sop.rs], ((u16 *)&sop)[0]); break;
		case 0x0E: sprintf (out, "XORI	%s, %s, 0x%04X", r4kreg[sop.rt], r4kreg[sop.rs], ((u16 *)&sop)[0]); break;
		case 0x0F: sprintf (out, "LUI	%s, 0x%04X", r4kreg[sop.rt], ((u16 *)&sop)[0]); break;
		case 0x10: COP0Lookup (out); break;
		case 0x11: sprintf (out, "COP1	%08X", sop.rs); break;
		case 0x14: if (sop.rt) sprintf (out, "BEQL	%s, %s, 0x%08X", r4kreg[sop.rs], r4kreg[sop.rt], addy+4+(((s16 *)&sop)[0]<<2));
				   else sprintf (out, "BEQZL	%s, 0x%08X", r4kreg[sop.rs], addy+4+(((s16 *)&sop)[0]<<2)); break;
		case 0x15: if (sop.rt) sprintf (out, "BNEL	%s, %s, 0x%08X", r4kreg[sop.rs], r4kreg[sop.rt], addy+4+(((s16 *)&sop)[0]<<2));
				   else sprintf (out, "BNEZL	%s, 0x%08X", r4kreg[sop.rs], addy+4+(((s16 *)&sop)[0]<<2)); break;
		case 0x16: sprintf (out, "BLEZL	%s, %s, 0x%08X", r4kreg[sop.rs], r4kreg[sop.rt], addy+4+(((s16 *)&sop)[0]<<2)); break;
		case 0x20: sprintf (out, "LB	%s, 0x%04X(%s)", r4kreg[sop.rt], ((u16 *)&sop)[0], r4kreg[sop.rs]); break;
		case 0x21: sprintf (out, "LH	%s, 0x%04X(%s)", r4kreg[sop.rt], ((u16 *)&sop)[0], r4kreg[sop.rs]); break;
		// 22 - LWL
		case 0x23: sprintf (out, "LW	%s, 0x%04X(%s)", r4kreg[sop.rt], ((u16 *)&sop)[0], r4kreg[sop.rs]); break;
		case 0x24: sprintf (out, "LBU	%s, 0x%04X(%s)", r4kreg[sop.rt], ((u16 *)&sop)[0], r4kreg[sop.rs]); break;
		case 0x25: sprintf (out, "LHU	%s, 0x%04X(%s)", r4kreg[sop.rt], ((u16 *)&sop)[0], r4kreg[sop.rs]); break;
		// 26 - LWR
		case 0x27: sprintf (out, "LWU	%s, 0x%04X(%s)", r4kreg[sop.rt], ((u16 *)&sop)[0], r4kreg[sop.rs]); break;
		case 0x28: sprintf (out, "SB	%s, 0x%04X(%s)", r4kreg[sop.rt], ((u16 *)&sop)[0], r4kreg[sop.rs]); break;
		case 0x29: sprintf (out, "SH	%s, 0x%04X(%s)", r4kreg[sop.rt], ((u16 *)&sop)[0], r4kreg[sop.rs]); break;
		// 2A - SWL
		case 0x2B: sprintf (out, "SW	%s, 0x%04X(%s)", r4kreg[sop.rt], ((u16 *)&sop)[0], r4kreg[sop.rs]); break;
		// 2C - SDL
		// 2D - SDR
		// 2E - SWR
		case 0x2F: sprintf (out, "CACHE %X, 0x%04X(%s)", sop.rt, ((u16 *)&sop)[0], r4kreg[sop.rs]); break;
		// 30 - LL
		case 0x31: sprintf (out, "LWC1  COP1[%i], 0x%04X(%s)", sop.rt, ((u16 *)&sop)[0], r4kreg[sop.rs]); break;
		// 32 - LWC2
		// 34 - LLD
		// 35 - LDC1
		// 36 - LDC2
		// 37 - LD
		// 38 - SC
		// 39 - SWC1
		// 3A - SWC2
		// 3C - SCD
		// 3D - SDC1
		// 3E - SDC2
		// 3F - SD
		default:
			sprintf (out, "********r4300 %08X*********", sop.op);
	}
}

void DisassembleRange (u32 Start, u32 End) {
	FILE *disasm = fopen ("d:\\disasm.txt", "wt");
	char buffer[0x100];
	char tbuff[50];
	u32 pcx = Start;
	u32 tmp;
	while (pcx != End) {
		OpcodeLookup(pcx, buffer);
		/*tmp = strlen (buffer);
		memset (tbuff, 32, 35-tmp);
		tbuff[35-tmp] = 0;
		strcat (buffer, tbuff);
*/
		fprintf (disasm, "%08X  %s			%08X\n", pcx, buffer, vr32(pcx));
		pcx += 4;
	}
	fclose (disasm);
}


bool InitializeDebugger(void) {
	if (DebuggerHwnd!=0) return false;
	DebuggerHwnd = CreateDialog(GhInst,MAKEINTRESOURCE(IDD_DEBUGME),
				GhWnd,(DLGPROC)Debugger);
	ShowWindow(DebuggerHwnd,SW_SHOW);
	return (DebuggerHwnd !=0);
}

#define TYPER4K		0
#define TYPEMMU		1
#define TYPEFPU		2
#define TYPERSP		3
#define TYPEMI		4
#define TYPEPI		5
#define TYPESP		6
#define TYPEDP		7
#define TYPEAI		8
#define TYPERAM		9
#define TYPERI		10
#define TYPEVI		11
#define TYPESI		12

#define R4KCPU		0
#define RSPCPU		1

char *lpszREGTYPES[13] = {"R4K", "MMU", "FPU", "RSP", "MI", "PI", "SP", 
						  "DP", "AI", "RAM", "RI", "VI", "SI"};

char *R4KRegNames[0x20] = {
  "R0", "AT", "V0", "V1", "A0", "A1", "A2", "A3", "T0", "T1", "T2",
  "T3", "T4", "T5", "T6", "T7", "S0", "S1", "S2", "S3", "S4", "S5",
  "S6", "S7", "T8", "T9", "K0", "K1", "GP", "SP", "S8", "RA"
};


static int iCurrentCPU = 0;
static int iCurrentREG = 0;


void UpdateRegisters (HWND hWnd, int REGType) {
	LVCOLUMN lvColumn;
	char buffer [20];
	int x;
	LVITEM lvItem;

	SendMessage(GetDlgItem(hWnd,IDC_DEBUG_REGS),LVM_DELETEALLITEMS, 0, 0);
	ZeroMemory (&lvColumn, sizeof(LVCOLUMN));
	ZeroMemory (&lvItem, sizeof(LVITEM));
	lvItem.mask = LVIF_TEXT;
	switch (REGType) {
		case TYPER4K:
			strcpy (buffer, "r4k Regs");
			for (x=0x1F; x >= 0; x--) {
				char buff[0x12];
				lvItem.iSubItem = 0;
				lvItem.pszText = r4kreg[x];
				SendMessage(GetDlgItem(hWnd,IDC_DEBUG_REGS),LVM_INSERTITEM, 0, (LPARAM)&lvItem);
				sprintf (buff, "%08X",  (CpuRegs[x]>>32));
				sprintf (buff+8, " %08X",  CpuRegs[x]);
				lvItem.pszText = buff;
				lvItem.iSubItem = 1;
				SendMessage(GetDlgItem(hWnd,IDC_DEBUG_REGS),LVM_SETITEM, 0, (LPARAM)&lvItem);
			}
		break;
		case TYPEMMU:
			strcpy (buffer, "MMU Registers");
		break;
		case TYPEFPU:
			strcpy (buffer, "FPU Registers");
		break;
		case TYPERSP:
			strcpy (buffer, "SP Registers");
		break;
		case TYPEMI:
			strcpy (buffer, "MI Registers");
		break;
		case TYPEPI:
			strcpy (buffer, "PI Registers");
		break;
		case TYPESP:
			strcpy (buffer, "SP Registers");
		break;
		case TYPEDP:
			strcpy (buffer, "DP Registers");
		break;
		case TYPEAI:
			strcpy (buffer, "AI Registers");
		break;
		case TYPERAM:
			strcpy (buffer, "RDRAM Registers");
		break;
		case TYPERI:
			strcpy (buffer, "RI Registers");
		break;
		case TYPEVI:
			strcpy (buffer, "VI Registers");
		break;
		case TYPESI:
			strcpy (buffer, "SI Registers");
		break;
		default:
			return;
	}
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = buffer;
	SendMessage(GetDlgItem(hWnd,IDC_DEBUG_REGS),LVM_SETCOLUMN, 0, (LPARAM)&lvColumn);
	SendMessage(GetDlgItem(hWnd,IDC_DEBUG_REGS),LVM_SETCOLUMNWIDTH, 0, MAKELPARAM(LVSCW_AUTOSIZE_USEHEADER,0));
	SendMessage(GetDlgItem(hWnd,IDC_DEBUG_REGS),LVM_SETCOLUMNWIDTH, 1, MAKELPARAM(LVSCW_AUTOSIZE_USEHEADER,0));
}

u32 bpoints[0x20];
u32 topaddy;
const u32 MAXLIST = 0x26;

void UpdateCPU (HWND hWnd, int CPUType, DWORD addy) {
	char buffer[0x100];
	long tabstop = 70;

	SendMessage(GetDlgItem(hWnd,IDC_DEBUG_DISASM), LB_SETTABSTOPS, 1, (long)&tabstop);

	topaddy = addy; // Address on the top of the list

	//bpoints[0] =addy+0x30;

	SendMessage(GetDlgItem(hWnd,IDC_DEBUG_DISASM), LB_RESETCONTENT, 0, 0);

	if (CPUType == R4KCPU) {
		for (int x = 0; x < MAXLIST; x++) {
			sprintf (buffer, "%08X ", addy+(x*4));
			OpcodeLookup(addy+(x*4), buffer+0x9);
			SendMessage(GetDlgItem(hWnd,IDC_DEBUG_DISASM), LB_INSERTSTRING, x, (long)buffer);
			SetDlgItemText (hWnd, IDC_DEBUG_ITEML1+x, "");
			SetDlgItemText (hWnd, IDC_DEBUG_ITEMR1+x, "");
			for (int z = 0; z < 0x10; z++) {
				if (bpoints[z] == addy+(x*4)) {
					SetDlgItemText (hWnd, IDC_DEBUG_ITEMR1+x, "<B");
					break;
				}
			}
		}
	} else {
		for (int x = 0; x < MAXLIST; x++) {
			sprintf (buffer, "", addy+(x*4));
			SendMessage(GetDlgItem(hWnd,IDC_DEBUG_DISASM), LB_INSERTSTRING, x, (long)buffer);
			SetDlgItemText (hWnd, IDC_DEBUG_ITEML1+x, "");
			SetDlgItemText (hWnd, IDC_DEBUG_ITEMR1+x, "");
		}
	}
}

//IDC_DEBUG_SCRBAR


BOOL CALLBACK Debugger( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) {
	int MenuId,i;
	TCITEM index;
	LVCOLUMN lvColumn;
	char buffer[0x20];
	static DWORD daddy;

	switch(message)
	{
		case WM_INITDIALOG: {
			index.pszText = "R4K";
			index.cchTextMax = 3;
			index.mask = TCIF_TEXT;
			SendMessage(GetDlgItem(hWnd,IDC_DEBUG_CPUSEL),TCM_INSERTITEM,0,(LPARAM)&index);
			index.pszText = "RSP";
			SendMessage(GetDlgItem(hWnd,IDC_DEBUG_CPUSEL),TCM_INSERTITEM,1,(LPARAM)&index);
			SendMessage(GetDlgItem(hWnd,IDC_DEBUG_CPUSEL),TCM_SETITEMSIZE,0,(LPARAM)0x40);
			for (int x = 0; x < 13; x++) {
				index.cchTextMax = 2;
				index.mask = TCIF_TEXT;
				index.pszText = lpszREGTYPES[x];
				SendMessage(GetDlgItem(hWnd,IDC_DEBUG_REGSEL),TCM_INSERTITEM,x,(LPARAM)&index);
			}
			SendMessage(GetDlgItem(hWnd,IDC_DEBUG_REGSEL),TCM_SETITEMSIZE,0,(LPARAM)0x30);
			lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;
			lvColumn.cx = 0x80;
			lvColumn.pszText = "Register";
			lvColumn.cchTextMax = 16;
			SendMessage(GetDlgItem(hWnd,IDC_DEBUG_REGS),LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);
			lvColumn.pszText = "Register Value";
			lvColumn.cx = 0x100;
			SendMessage(GetDlgItem(hWnd,IDC_DEBUG_REGS),LVM_INSERTCOLUMN, 1, (LPARAM)&lvColumn);
			SendMessage(GetDlgItem(hWnd,IDC_DEBUG_REGS),LVM_SETEXTENDEDLISTVIEWSTYLE, 
				LVS_EX_FULLROWSELECT|LVS_EX_ONECLICKACTIVATE, 
				LVS_EX_FULLROWSELECT|LVS_EX_ONECLICKACTIVATE);
			iCurrentCPU = R4KCPU;
			iCurrentREG = 0;
			UpdateRegisters (hWnd, iCurrentREG);
			memset (bpoints, -1, 0x20*4);
			daddy = pc;
			UpdateCPU		(hWnd, iCurrentCPU, daddy);
			return TRUE;
		}
		break;

		case WM_COMMAND:
			MenuId = LOWORD(wParam);
			switch (MenuId) {
				case IDOK:
					EndDialog(DebuggerHwnd,0);
					DebuggerHwnd = 0;
					return TRUE;
				break;
				case IDC_DEBUG_DISASM: {
					if (HIWORD(wParam) == LBN_SELCHANGE) {
						char lpBuff[0x40];
						int selection=0;
						selection = SendMessage(GetDlgItem(hWnd,IDC_DEBUG_DISASM), LB_GETCURSEL, 0, 0);
						sprintf (lpBuff, "0x%08X", topaddy+selection*4);
						SetDlgItemText (hWnd, IDC_DEBUG_COPYBOX, lpBuff);
					} else if (HIWORD(wParam) == LBN_DBLCLK) {
						char lpBuff[0x40];
						int selection=0;
						selection = SendMessage(GetDlgItem(hWnd,IDC_DEBUG_DISASM), LB_GETCURSEL, 0, 0);
						for (int x=0; x<0x20; x++)
							if (bpoints[x] == topaddy+selection*4) {
								bpoints[x] = 0xFFFFFFFF;
								x = 0x30;
								break;
							}
						if (x != 0x30)
							for (x=0; x<0x20; x++)
								if (bpoints[x] == 0xFFFFFFFF) {
									bpoints[x] = topaddy+selection*4;
									break;
								}
						UpdateCPU (hWnd, iCurrentCPU, topaddy);
					}
				}
				break;
			}
		break;

		case WM_NOTIFY: {
			int idCtrl = wParam;
			NMHDR *nmHeader = (NMHDR *)lParam;
			switch (nmHeader->idFrom){//idCtrl) {
			case IDC_DEBUG_REGSEL:
				if ((nmHeader->code == TCN_SELCHANGE)&&(nmHeader->idFrom == IDC_DEBUG_REGSEL))
					UpdateRegisters (hWnd, iCurrentREG = SendMessage(GetDlgItem(hWnd,IDC_DEBUG_REGSEL),TCM_GETCURSEL,0,0));
			break;
			case IDC_DEBUG_CPUSEL:
				if ((nmHeader->code == TCN_SELCHANGE)&&(nmHeader->idFrom == IDC_DEBUG_CPUSEL))
					UpdateCPU (hWnd, iCurrentCPU= SendMessage(GetDlgItem(hWnd,IDC_DEBUG_CPUSEL),TCM_GETCURSEL,0,0), pc);
			break;
			}
		break;
		}
	}
	return FALSE;
}
