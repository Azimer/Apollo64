#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../common.h"
#include "Recompiler.h"
#include "RecompilerDebug.h"
#include "../resource.h"
#include "../winmain.h"
#include "../emumain.h"
#include "../cpumain.h"

HWND hDlgBox;
bool inDebugger = false;

u64 DebugRegs[32];
u32 x86GPR[8];

void InterpreterStep ();

// ****** Support functions for Display ******
void UpdateRegisters () {
	char buff[20];
	for (int i = 0; i < 32; i++) {
		sprintf (buff, "%08X%08X", (u32)(CpuRegs[i]>>32), CpuRegs[i]&0xFFFFFFFF);
		SetDlgItemText (hDlgBox, IDC_REGR0+i, buff);
	}
	for (i = 0; i < 8; i++) {
		sprintf (buff, "%08X", x86GPR[i]);
		SetDlgItemText (hDlgBox, IDC_REGEAX+i, buff);
	}
}
#define MAXPC 26
DWORD TopIndex, BottomIndex;
DWORD AddressBase;

//#define UpdateScroll(i) SetScrollPos((HWND)lParam, SB_CTL, GetScrollPos(lParam, SB_CTL)+i, TRUE)

void UpdateR4KCode (u32 addy) {
	char buffer[100];

	TopIndex = addy;
	SendMessage(GetDlgItem(hDlgBox,IDC_R4KCODE), LB_RESETCONTENT, 0, (long)buffer);
	for (int i = 0; i < MAXPC; i++) {
		sprintf (buffer, "%08X ", addy+(i*4));
		OpcodeLookup(addy+(i*4), buffer+0x9);
		SendMessage(GetDlgItem(hDlgBox,IDC_R4KCODE), LB_ADDSTRING, 0, (long)buffer);
	}
	BottomIndex = addy+((MAXPC-1)*4);
	if (pc >= TopIndex) {
		if (pc <= BottomIndex) {
			SendMessage(GetDlgItem(hDlgBox,IDC_R4KCODE), LB_SETCURSEL, (pc-TopIndex)/4, 0);
		}
	}
}

void UpdateR4KCodeScrollUp () {
	char buffer[100];

	if (TopIndex == 0xFFFFFFFC)
		return;
	BottomIndex += 4; TopIndex += 4;
	
	SendMessage(GetDlgItem(hDlgBox,IDC_R4KCODE), LB_DELETESTRING, 0, 0);

	sprintf (buffer, "%08X ", BottomIndex);
	OpcodeLookup(BottomIndex, buffer+0x9);
	SendMessage(GetDlgItem(hDlgBox,IDC_R4KCODE), LB_ADDSTRING, 0, (long)buffer);
}

void UpdateR4KCodeScrollDown () {
	char buffer[100];

	if (TopIndex == 0x00000000)
		return;
	BottomIndex -= 4; TopIndex -= 4;
	
	SendMessage(GetDlgItem(hDlgBox,IDC_R4KCODE), LB_DELETESTRING, (MAXPC-1), 0);

	sprintf (buffer, "%08X ", TopIndex);
	OpcodeLookup(TopIndex, buffer+0x9);
	SendMessage(GetDlgItem(hDlgBox,IDC_R4KCODE), LB_ADDSTRING, 0, (long)buffer);
}

void UpdateR4KCodeStep (u32 addy) {
	if (addy >= (BottomIndex - 8)) {
		UpdateR4KCode(addy-((MAXPC/4)*4));
	} else {
		SendMessage(GetDlgItem(hDlgBox,IDC_R4KCODE), LB_SETCURSEL, (pc-TopIndex)/4, 0);
	}
}

void UpdateR4KCodeJump (u32 addy) {
	UpdateR4KCode(addy-((MAXPC/4)*4));
}

BOOL CALLBACK DebugProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) {
	int MenuId,i;

	hDlgBox = hWnd;
	switch(message)
	{
		case WM_INITDIALOG: {
			UpdateRegisters ();
			AddressBase = (pc>>16);
			UpdateR4KCode(pc-((MAXPC/2)*4));
			SetScrollRange (GetDlgItem(hWnd, IDC_R4KSCROLL), SB_CTL, 0x8000, 0x80FF, FALSE);
			SetScrollPos (GetDlgItem(hWnd, IDC_R4KSCROLL), SB_CTL, 0x8000, TRUE);
			ShowWindow (hWnd, SW_SHOW);

			return TRUE;
		}
		break;

		case WM_VSCROLL:
			if ((HWND)lParam == GetDlgItem (hWnd, IDC_R4KSCROLL)) {
				switch (LOWORD(wParam)) {
					case SB_LINEUP:
						UpdateR4KCodeScrollDown();
					break;
					case SB_LINEDOWN:
						UpdateR4KCodeScrollUp();
					break;
					case SB_PAGEUP:
						UpdateR4KCode (TopIndex-(MAXPC*4));
					break;
					case SB_PAGEDOWN:
						UpdateR4KCode (TopIndex+(MAXPC*4));
					break;
					case SB_THUMBPOSITION:
						SetScrollPos ((HWND)lParam, SB_CTL, HIWORD(wParam), TRUE);
					case SB_THUMBTRACK:
						TopIndex = TopIndex & 0xFFFF;
						TopIndex |= (HIWORD(wParam) << 0x10);
						UpdateR4KCode (TopIndex);
					break;
				}
			} else {
				;
			}
			return FALSE;
		break;

		case WM_COMMAND:
			MenuId = LOWORD(wParam);
			switch (MenuId) {
				case IDOK:
					EndDialog(hWnd,0);
					return TRUE;
				break;
				case IDC_STEP: {
					if (RegSettings.dynamicEnabled == true) {
						;//RecompilerStep ();
					} else {
						InterpreterStep ();
					}
					UpdateRegisters ();
				}
				case IDC_GOTO: {
					char buff[13];
					int loc;
					GetDlgItemText (hWnd, IDC_GOTOLOC, buff, 12);
					if ((loc = strlen(buff)) == 8) {
						loc = strtoul(buff, NULL, 0x10);
						loc &= 0xFFFFFFFC;
						AddressBase = (loc >> 0x10) & 0xFF00;
						UpdateR4KCode(loc);
						SetScrollRange (GetDlgItem(hWnd, IDC_R4KSCROLL), SB_CTL, AddressBase, (AddressBase | 0xFF), FALSE);
						SetScrollPos (GetDlgItem(hWnd, IDC_R4KSCROLL), SB_CTL, (loc >> 0x10), TRUE);
					}
				}
			}
		break;
	}
	return FALSE;
}

void OpenDebuggerWindow (HINSTANCE hApp, HWND hParent) {
	CreateDialog (hApp, MAKEINTRESOURCE(IDD_RECOMPDEBUG), NULL, DebugProc);
}

void InterpreterStep () {
		u32 lastPC;
		InterruptTime -= RegSettings.VSyncHack;

		if ((InterruptTime <= 0))
			CheckInterrupts();
		extern u8 *rdram;
		if (TLBLUT[pc>>12] > 0xFFFFFFF0) {
			QuickTLBExcept ();
		}
		((u32*)&sop)[0] = ((u32 *)returnMemPointer(TLBLUT[pc>>12]+(pc & 0xfff)))[0];
		pc+=4;
		lastPC = pc;
		r4300i[sop.op]();
		if ( lastPC == pc ) {
			UpdateR4KCodeStep (pc);
		} else {
			UpdateR4KCodeJump (pc);
		}

		if (CpuRegs[0] != 0) {
			CpuRegs[0] = 0;			
		}
}