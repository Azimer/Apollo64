/*
    Apollo N64 Emulator (c) Eclipse Productions
    Copyright (C) 2001 Azimer (azimer@emulation64.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

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
  This field tells us what an opcodes arguments are.
  f = treat next var as fpu register.
  d = rd
  t = rt
  s = rs
  a = sa
  e = rs in [el] format
  i = immediate (WORD)(opcode)
  u = func (first 6 bits)
  b = compute branch
  j = compute j
  o = display entire opcode (for opni's)
*/
/*
char *CpuRegNames[0x20] = {
  "ZR", "AT", "V0", "V1", "A0", "A1", "A2", "A3", "T0", "T1", "T2",
  "T3", "T4", "T5", "T6", "T7", "S0", "S1", "S2", "S3", "S4", "S5",
  "S6", "S7", "T8", "T9", "K0", "K1", "GP", "SP", "S8", "RA"
};

char *MmuRegNames[0x20] = {
  "Index", "Random", "EntryLo0", "EntryLo1", "Context", "PageMask", "Wired",
  "RESERVED", "BadVAddr", "Count", "EntryHi", "Compare", "Status", "Cause",
  "ExceptPC", "PRevID", "Config", "LLAddr", "WatchLo", "WatchHi", "XContext", "RESERVED",
  "RESERVED", "RESERVED", "RESERVED", "RESERVED", "PErr", "CacheErr", "TagLo",
  "TagHi", "ErrorEPC", "RESERVED"
};

char* fpu_dbg_str[0x40] = {
	"ADD.",		"SUB.",		"MUL.",		"DIV.",		"SRT.",		"ABS.",		"MOV.",		"NEG.",
	"RND.L.",	"TRC.L.",	"CEL.L.",	"FLR.L.",	"RND.W.",	"TRC.W.",	"CEL.W.",	"FLR.W.",
	"NI",		"NI",		"NI",		"NI",		"NI",		"NI",		"NI",		"NI",
	"NI",		"NI",		"NI",		"NI",		"NI",		"NI",		"NI",		"NI",
	"CVT.S.",	"CVT.D.",	"NI",		"NI",		"CVT.W.",	"CVT.L.",	"NI",		"NI",
	"NI",		"NI",		"NI",		"NI",		"NI",		"NI",		"NI",		"NI",
	"C.F.",		"C.UN.",	"C.EQ.",	"C.UEQ.",	"C.OLT.",	"C.ULT.",	"C.OLE.",	"C.ULE.",
	"C.SF.",	"C.NGLE.",	"C.SEQ.",	"C.NGL.",	"C.LT.",	"C.NGE.",	"C.LE.",	"C.NGT."
};

char *FpuRegNames[0x20] = {
  "F00","F01","F02","F03","F04","F05","F06","F07","F08","F09","F10","F11",
  "F12","F13","F14","F15","F16","F17","F18","F19","F20","F21","F22","F23",
  "F24","F25","F26","F27","F28","F29","F30","F31",
};

char *r4300i_debug[0x40] = {
    "o",	"o",	"j",	"j",	"tsb",	"tsb",	"sb",	"sb",  
    "tsi",	"tsi",	"tsi",	"tsi",	"tsi",	"tsi",	"tsi",	"ti",
    "o",	"o",	"o",	"o",	"tsb",	"tsb",	"sb",	"sb",  
    "tsi",	"tsi",	"tsi",	"tsi",	"o",	"o",	"o",	"o",  
	"tsi",	"tsi",	"tsi",	"tsi",	"tsi",	"tsi",	"tsi",	"tsi",  
    "tsi",	"tsi",	"tsi",	"tsi",	"tsi",	"tsi",	"tsi",	"tsi",
    "tsi",	"ftsi",	"tsi",	"o",	"tsi",	"ftsi",	"tsi",	"tsi", 
    "tsi",	"ftsi",	"tsi",	"o",	"tsi",	"ftsi",	"tsi",	"tsi" 
};

char *special_debug[0x40] = {
    "dta",	"o",	"dta",	"dta",	"dts",	"o",	"dts",	"dts",  
    "s",	"ds",	"o",	"o",	"o",	"o",	"o",	"o",  
    "d",	"s",	"d",	"s",	"dts",	"o",	"dts",	"dts",  
    "st",	"st",	"st",	"st",	"st",	"st",	"st",	"st",
    "dst",	"dst",	"dst",	"dst",	"dst",	"dst",	"dst",	"dst",  
    "o",	"o",	"dst",	"dst",	"dst",	"dst",	"dst",	"dst",  
    "st",	"st",	"st",	"st",	"st",	"o",	"st",	"o",  
    "dta",	"o",	"dta",	"dta",	"dta",	"o",	"dta",	"dta"
};

char *regimm_debug[0x20] = {
    "sb",	"sb",	"sb",	"sb",	"o",	"o",	"o",	"o",
    "si",	"si",	"si",	"si",	"si",	"o",	"si",	"o",
    "sb",	"sb",	"sb",	"sb",	"o",	"o",	"o",	"o",
    "o",	"o",	"o",	"o",	"o",	"o",	"o",	"o"
};

char *fpu_debug[0x40] = {
    "fafdft",	"fsfdft",	"fafdft",	"fafdft",	"fafd",	"fafd",	"fafd",	"fafd",
    "o",	"fafd",	"o",	"o",	"o",	"fafd",	"o",	"o",
    "o",	"o",	"o",	"o",	"o",	"o",	"o",	"o",
    "o",	"o",	"o",	"o",	"o",	"o",	"o",	"o",
    "fafd",	"fafd",	"o",	"o",	"fafd",	"fafd",	"o",	"o",
    "o",	"o",	"o",	"o",	"o",	"o",	"o",	"o",
    "o",	"o",	"o",	"o",	"o",	"o",	"o",	"o",
    "o",	"o",	"o",	"o",	"o",	"o",	"o",	"o"
};

char* r4300i_str[0x40] = {
    "SPECIAL",	"REGIMM",	"J",	"JAL",	"BEQ",	"BNE",	"BLEZ",	"BGTZ",  
    "ADDI",		"ADDIU",	"SLTI",	"SLTIU","ANDI",	"ORI",	"XORI",	"LUI",
    "COP0",		"COP1",		"COP2",	"NI",	"BEQL",	"BNEL",	"BLEZL","BGTZL",  
    "DADDI",	"DADDIU",	"LDL",	"LDR",	"NI",	"NI",	"NI",	"NI",  
	"LB",		"LH",		"LWL",	"LW",	"LBU",	"LHU",	"LWR",	"LWU",  
    "SB",		"SH",		"SWL",	"SW",	"SDL",	"SDR",	"SWR",	"CACHE",
    "LL",		"LWC1",		"LWC2",	"NI",	"LLD",	"LDC1",	"LDC2",	"LD", 
    "SC",		"SWC1",		"SWC2",	"NI",	"SCD",	"SDC1",	"SDC2",	"SD" 
};

char* special_str[0x40] = {
    "SLL",	"NI",	"SRL",	"SRA",	"SLLV",		"NI",		"SRLV",		"SRAV",  
    "JR",	"JALR",	"NI",	"NI",	"SYSCALL",	"BREAK",	"NI",		"SYNC",  
    "MFHI",	"MTHI",	"MFLO",	"MTLO",	"DSLLV",	"NI",		"DSRLV",	"DSRAV",  
    "MULT",	"MULTU","DIV",	"DIVU",	"DMULT",	"DMULTU",	"DDIV",		"DDIVU",
    "ADD",	"ADDU",	"SUB",	"SUBU",	"AND",		"OR",		"XOR",		"NOR",  
    "NI",	"NI",	"SLT",	"SLTU",	"DADD",		"DADDU",	"DSUB",		"DSUBU",  
    "TGE",	"TGEU",	"TLT",	"TLTU",	"TEQ",		"NI",		"TNE",		"NI",  
    "DSLL",	"NI",	"DSRL",	"DSRA",	"DSLL32",	"NI",		"DSRL32",	"DSRA32"
};

char* regimm_str[0x20] = {
    "BLTZ",		"BGEZ",		"BLTZL",	"BGEZL",	"NI",	"NI",	"NI",	"NI",
    "TGEI",		"TGEIU",	"TLTI",		"TLTIU",	"TEQI",	"NI",	"TNEI",	"NI",
    "BLTZAL",	"BGEZAL",	"BLTZALL",	"BGEZALL",	"NI",	"NI",	"NI",	"NI",
    "NI",		"NI",		"NI",		"NI",		"NI",	"NI",	"NI",	"NI"
};

char* fpu_str[0x40] = {
	"fpADD",	"fpSUB",	"fpMUL",	"fpDIV",	"fpSRT",	"fpABS",	"fpMOV",	"fpNEG",
	"fpRNDl",	"fpTRCl",	"fpCELl",	"fpFLRl",	"fpRNDw",	"fpTRCw",	"fpCELw",	"fpFLRw",
	"opNI",		"opNI",		"opNI",		"opNI",		"opNI",		"opNI",		"opNI",		"opNI",
	"opNI",		"opNI",		"opNI",		"opNI",		"opNI",		"opNI",		"opNI",		"opNI",
	"fpCVTs",	"fpCVTd",	"opNI",		"opNI",		"fpCVTw",	"fpCVTl",	"opNI",		"opNI",
	"opNI",		"opNI",		"opNI",		"opNI",		"opNI",		"opNI",		"opNI",		"opNI",
	"fpC.F",	"fpC.UN",	"fpC.EQ",	"fpC.UEQ",	"fpC.OLT",	"fpC.ULT",	"fpC.OLE",	"fpC.ULE",
	"fpC.SF",	"fpC.NGLE",	"fpC.SEQ",	"fpC.NGL",	"fpC.LT",	"fpC.NGE",	"fpC.LE",	"fpC.NGT"
};

void DebugAssembleInstruction(char *buffer, int location, int opcode){
	sprintf(buffer,"%08X [%08X] ",location,opcode);
	char temp[80];
	int j=0;
	switch ((opcode >> 26)) {
	case 0x0:
		if (opcode==0) strcat(buffer,"NOP");
		else {
			strcat(buffer,special_str[(opcode & 0x3f)]);
			strcat(buffer, " ");
			do {
				Fetch(buffer, opcode, location, special_debug[(opcode & 0x3f)][j++]);
				if (special_debug[(opcode & 0x3f)][j] != 0) strcat(buffer,", ");
			} while (special_debug[(opcode & 0x3f)][j] != 0);
		}
		break;
	case 0x1:
		strcat(buffer,regimm_str[RT()]);
		strcat(buffer, " ");
		do {
			Fetch(buffer, opcode, location, regimm_debug[RT()][j++]);
			if (regimm_debug[RT()][j] != 0) strcat(buffer,", ");
		} while (regimm_debug[RT()][j] != 0);
		break;
	case 0x10:
		switch (RS()){
		case 0x0:
			sprintf(temp,"MFC0 %s, %s",CpuRegNames[RT()],MmuRegNames[RD()]);
			strcat(buffer,temp);
			break;
		case 0x4:
			sprintf(temp,"MTC0 %s, %s",MmuRegNames[RD()],CpuRegNames[RT()]);
			strcat(buffer,temp);
			break;
		case 16:
			switch(opcode & 0x3f) {
			case 0x1:
				sprintf(temp,"TLBR");
				strcat(buffer,temp);
				break;
			case 0x2:
				sprintf(temp,"TLBWI");
				strcat(buffer,temp);
				break;
			case 0x6:
				sprintf(temp,"TLBWR");
				strcat(buffer,temp);
				break;
			case 0x8:
				sprintf(temp,"TLBP");
				strcat(buffer,temp);
				break;
			case 0x18:
				sprintf(temp,"ERET");
				strcat(buffer,temp);
				break;
			default:
				break;
			}
		default:
			break;
		}
		break;
	case 0x11:
		switch (RS()){
		case 0x0:
			sprintf(temp,"MFC1 %s, %s",CpuRegNames[RT()],FpuRegNames[RD()]);
			strcat(buffer,temp);
			break;
		case 0x2:
			sprintf(temp,"CFC1 %s, FC%d",CpuRegNames[RT()],RD());
			strcat(buffer,temp);
			break;
		case 0x4:
			sprintf(temp,"MTC1 %s, %s",FpuRegNames[RD()],CpuRegNames[RT()]);
			strcat(buffer,temp);
			break;
		case 0x6:
			sprintf(temp,"CTC1 FC%d, %s",RD(),CpuRegNames[RT()]);
			strcat(buffer,temp);
			break;
		case 0x8:
			switch ((opcode >> 16) & 0x3){
			case 0:
				strcat(buffer,"BC1F ");
				Fetch(buffer, opcode, location, 'b');
				break;
			case 1:
				strcat(buffer,"BC1T ");
				Fetch(buffer, opcode, location, 'b');
				break;
			case 2:
				strcat(buffer,"BC1FL ");
				Fetch(buffer, opcode, location, 'b');
				break;
			case 3:
				strcat(buffer,"BC1TL ");
				Fetch(buffer, opcode, location, 'b');
				break;
			default:
				break;
			}
			break;
		case 16:
			strcat(buffer,fpu_dbg_str[opcode & 0x3f]);
			strcat(buffer,"S");
			break;
		case 17:
			strcat(buffer,fpu_dbg_str[opcode & 0x3f]);
			strcat(buffer,"D");
			break;
		case 20:
			strcat(buffer,fpu_dbg_str[opcode & 0x3f]);
			strcat(buffer,"W");
			break;
		case 21:
			strcat(buffer,fpu_dbg_str[opcode & 0x3f]);
			strcat(buffer,"L");
			break;
		default:
			break;
		}
		if (RS() >=16){
			strcat(buffer, " ");
			do {
				Fetch(buffer, opcode, location, fpu_debug[opcode & 0x3f][j++]);
				if (fpu_debug[opcode & 0x3f][j] != 0 && fpu_debug[opcode & 0x3f][j-1] != 'f') strcat(buffer,", ");
			} while (fpu_debug[opcode & 0x3f][j] != 0);
		}
		break;
	default:
		strcat(buffer,r4300i_str[(opcode >> 26)]);
		strcat(buffer, " ");
		do {
			Fetch(buffer, opcode, location, r4300i_debug[(opcode >> 26)][j++]);
			if (r4300i_debug[(opcode >> 26)][j] != 0 && r4300i_debug[(opcode >> 26)][j-1] != 'f') strcat(buffer,", ");
		} while (r4300i_debug[(opcode >> 26)][j] != 0);
		break;
	}
}

bool DbgTreatFpu = false;

void Fetch( char* buffer, unsigned int opcode, unsigned int location, char fmt ) {
	char temp[0x10];
	switch (fmt) {
	case 'f':
		DbgTreatFpu = true;
		return; break;
	case 'd':
		if (DbgTreatFpu) strcat( buffer, FpuRegNames[RD()]);
		else strcat( buffer, CpuRegNames[RD()]);
		break;
	case 't':
		if (DbgTreatFpu) strcat( buffer, FpuRegNames[RT()]);
		else strcat( buffer, CpuRegNames[RT()]);
		break;
	case 's':
		if (DbgTreatFpu) strcat( buffer, FpuRegNames[RS()]);
		else strcat( buffer, CpuRegNames[RS()]);
		break;
	case 'e':
		sprintf(temp, "[%d]", RS() & 0x0f);
		strcat( buffer, temp );
		break;
	case 'a':
		if (DbgTreatFpu) strcat( buffer, FpuRegNames[SA()]);
		else {
			sprintf(temp, "%02X", (WORD)SA());
			strcat( buffer, temp );
		}
		break;
	case 'i':
		sprintf(temp, "%04X", (WORD)opcode);
		strcat( buffer, temp );
		break;
	case 'u':
		sprintf(temp, "%02X", (BYTE)opcode & 0x3f);
		strcat( buffer, temp );
		break;
	case 'b':
		sprintf(temp, "%04X", location +4+ (((SWORD)opcode) << 2));		
		strcat( buffer, temp );
		break;
	case 'j':
		sprintf(temp, "%04X", (location & 0xf0000000) + ((opcode << 2) & 0x0fffffff) );		
		strcat( buffer, temp );
		break;
	default://o
		sprintf(temp, "%X, %08X",opcode >> 26, opcode);
		strcat( buffer, temp);
		break;
	}
}
*/
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
  "ZR", "AT", "V0", "V1", "A0", "A1", "A2", "A3", "T0", "T1", "T2",
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
				lvItem.iSubItem = 0;
				lvItem.pszText = R4KRegNames[x];
				SendMessage(GetDlgItem(hWnd,IDC_DEBUG_REGS),LVM_INSERTITEM, 0, (LPARAM)&lvItem);
				lvItem.pszText = "0xFFFFFFFFA4001550";
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
void UpdateCPU (HWND hWnd, int CPUType) {
	if (CPUType == R4KCPU) {
	} else {
	}
}




BOOL CALLBACK Debugger( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) {
	int MenuId;//,i;
	TCITEM index;
	LVCOLUMN lvColumn;
	//char buffer[0x20];

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
			iCurrentCPU = 0;
			iCurrentREG = 0;
			UpdateRegisters (hWnd, iCurrentREG);
			UpdateCPU		(hWnd, iCurrentCPU);
			return TRUE;
		}
		break;

		case WM_COMMAND:
			MenuId = LOWORD(wParam);
			if (MenuId == IDOK) {
				EndDialog(DebuggerHwnd,0);
				DebuggerHwnd = 0;
				return TRUE;
			}
		break;

		case WM_NOTIFY: {
			int idCtrl = wParam;
			NMHDR *nmHeader = (NMHDR *)lParam;
			switch (idCtrl) {
			case IDC_DEBUG_REGSEL:
				if ((nmHeader->code == TCN_SELCHANGE)&&(nmHeader->idFrom == IDC_DEBUG_REGSEL))
					UpdateRegisters (hWnd, iCurrentREG = SendMessage(GetDlgItem(hWnd,IDC_DEBUG_REGSEL),TCM_GETCURSEL,0,0));
			break;
			}
		break;
		}
	}
	return FALSE;
}
