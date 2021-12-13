/****************************************************************************
 *                                                                          *
 *               Copyright (C) 2000, Eclipse Productions                    *
 *                                                                          *
 *  Offical Source code of the Apollo Project.  DO NOT DISTRIBUTE!  Any     *
 *  unauthorized distribution of these files in any way, shape, or form     *
 *  is a direct infringement on the copyrights entitled to Eclipse          *
 *  Productions and you will be prosecuted to the fullest extent of         *
 *  the law.  Have a nice day... ^_^                                        *
 *                                                                          *
 ****************************************************************************/
/****************************************************************************
 *  rsp.cpp - Reality Signal Processor Emulation							*
 *																			*
 *  Revision History:														*
 *																			*
 *	Name		Date		Comment											*
 * -------------------------------------------------------------			*
 *	Azimer		01-05-01	Initial Version									*
 *																			*
 ****************************************************************************/
/****************************************************************************
 *																			*
 *  Notes:																	*
 *																			*
 *																			*
 ****************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h> // For status window
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "WinMain.h"
#include "EmuMain.h"
#include "resource.h"

unsigned long GenerateCRC (unsigned char *data, int size);

extern u8* idmem;
extern u8* SP;
extern u8* SP2;
extern u8* rdram;

HWND g_hWnd;

//	pInterruptMask = (int*)Audio_Info.MI_INTR_REG;
BYTE *pRDRAM;
BYTE *pDMEM;
BYTE *pIMEM;


DWORD crclist[0x200];

void BeginRSP () {
	pRDRAM = rdram;
	pIMEM = idmem+0x1000;
	pDMEM = idmem;
	g_hWnd = GhWnd;

	for (int x=0; x < 0x200; x++) {
		if (crclist[x] == 0) {
			crclist[x] = GenerateCRC ((idmem+0x1000), 0x1000);
			Debug (0, "RSP Sound UCode CRC: %08X", crclist[x]);
			return;
		} else {
			if (crclist[x] == GenerateCRC ((idmem+0x1000), 0x1000))
				return;
		}
	}	
}