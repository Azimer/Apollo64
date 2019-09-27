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
 *		06-20-00	Began rcp.cpp file (Andrew)
 *
 **************************************************************************/
/**************************************************************************
 *
 *  Notes:
 *	
 *	-this file encompasses both the rsp and rcp. A dispatcher, I guess.
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
#include "videodll.h"
#include "resource.h"

void SignalDeadRSP(DWORD *data);
void SignalDeadRDP(void);
int activeDPplugin = 0;
int activeAIplugin = 0;


//----------------------------------------------------------------------------------------------

unsigned long GenerateCRC (unsigned char *data, int size);

extern u8* idmem;
extern u8* SP;
extern u8* SP2;
extern u8* rdram;

HWND g_hWnd;

//	pInterruptMask = (int*)Audio_Info.MI_INTR_REG;
extern "C" {
	BYTE *pRDRAM;
	BYTE *pDMEM;
	BYTE *pIMEM;
	void rsp_run();
}


DWORD crclist[0x200];

void AudioHLE () {
	u32 Boot  = ((u32*)idmem)[0xFC8/4], BootLen = ((u32*)idmem)[0xFCC/4];
	u32 UC    = ((u32*)idmem)[0xFD0/4], UCLen   = ((u32*)idmem)[0xFD4/4];
	u32 UCData= ((u32*)idmem)[0xFD8/4], UDataLen= ((u32*)idmem)[0xFDC/4];
	u32 List  = ((u32*)idmem)[0xFF0/4], ListLen = ((u32*)idmem)[0xFF4/4];
	u32 *HLEPtr= (u32 *)(rdram+List);
	//u32 t9, k0;
	//FILE *dfile = fopen ("d:\\audio.txt", "wt");
	/*
	for (u32 x=0; x < 0x200; x++) {
		if (crclist[x] == 0) {
			crclist[x] = GenerateCRC ((rdram+UC), UCLen);
			Debug (0, "RSP Audio UCode CRC: %08X", crclist[x]);
			break;
		} else {
			if (crclist[x] == GenerateCRC ((rdram+UC), UCLen))
				break;
		}
	}*/ /*
	for (x=0; x < (ListLen/4); x+=2) {
		//fprintf (dfile, "%04X: t9 = %08X, k0 = %08X\n", x*8, HLEPtr[x], HLEPtr[x+1]);
		if ((HLEPtr[x] >> 24) == 0x1) {
			cpuIsDone = true;
			cpuIsReset = true;
		}
	}
*//*
	for (x=0; x < 0x4; x++) {
		fprintf (dfile, "%08X", (u32 *)(rdram+UCData+(x*4)));
	}
*/
	//fclose (dfile);
}

void BeginRSP () {
	pRDRAM = rdram;
	pIMEM = idmem+0x1000;
	pDMEM = idmem;
	g_hWnd = GhWnd;

	//rsp_run(); <-- Removed...
}

//----------------------------------------------------------------------------------------------
/*
Initialisation:
        If you know the following you can directly start HLE out of it.
        HLE: High Level Emulation - first hacked out by Epsilon and
RealityMan.
        (Authors of UltraHLE.)
        The DMEM is filled from 0x04000fc0 - 0x04000fff with the following:
                0x04000fc0: task
                            0x00000001 ... Graphics Task
                            0x00000002 ... Audio Task
                0x04000fc4: flags?
                            0x00000002 ... DP wait
                0x04000fc8: bootcode
                0x04000fcc: bootcode length
                0x04000fd0: ucode
                0x04000fd4: ucode length
                0x04000fd8: ucode data
                0x04000fdc: ucode data length
                0x04000fe0: dram stack (for matrices)
                0x04000fe4: dram stack length
                0x04000fe8: ?
                0x04000fec: ?
                0x04000ff0: display list
                0x04000ff4: display list length
                0x04000ff8: ?
                0x04000ffc: ?
*/
void ComputeFPS(void);
//Purpose: called when bit 0 is set in the SP_STATUS. dispatch to correct HLE emulation function.
void SignalRSP(DWORD *data) {
	if (((u32*)idmem)[0xFC0/4]==1) {
		gfxdll.ProcessDList();
		SignalDeadRSP(data);
		SignalDeadRDP();
	} else if (((u32*)idmem)[0xFC0/4]==2) {
		//BeginRSP ();
		//Debug (0, "BootCode: %08X Length: %08X", ((u32*)idmem)[0xFC8/4], ((u32*)idmem)[0xFCC/4]);
		//Debug (0, "UCode   : %08X Length: %08X", ((u32*)idmem)[0xFD0/4], ((u32*)idmem)[0xFD4/4]);
		//Debug (0, "UC Data : %08X Length: %08X", ((u32*)idmem)[0xFD8/4], ((u32*)idmem)[0xFDC/4]);
		//Debug (0, "List Dat: %08X Length: %08X", ((u32*)idmem)[0xFF0/4], ((u32*)idmem)[0xFF4/4]);
		//Debug (0, "AL1: %08X  AL2: %08X", ((u32*)rdram)[((u32*)idmem)[0xFF0/4]/4], ((u32*)rdram)[(((u32*)idmem)[0xFF0/4]+4)/4]);
		AudioHLE ();
		SignalDeadRSP(data);
	} else {
		BeginRSP ();
		Debug(0,L_STR(IDS_RSP_UK),((u32*)idmem)[0xFC0/4]);
		SignalDeadRSP(data);
	}
}

//Purpose: called when a valid start and end is written to DP registers. Normally is not used
//  because most of the SP ucodes internally use the RDP, so the R4K usually doesn't use this.
void SignalRDP(void) {
	extern u8* DPC;
	Debug(0,L_STR(IDS_DP_RUN),((u32*)DPC)[0]);
}

void CheckRDP() {
	extern u8* MI;
    //if ((MI_INTR_REG_R & MI_INTR_SP) != 0) Trigger_RSPBreak();
    if ((((u32*)MI)[2] & DP_INTERRUPT) != 0) 
		SignalDeadRDP();
}

void SignalDeadRSP(DWORD *data) {
	extern u8* SP;
	extern u8* SP2;
	extern u8* MI;
	//((u32*)SP)[4] |= 0x303;
	*data |= 0x303;
	//if (((u32*)SP)[4] & 0x40) { 
	if (*data & 0x40) {
		SET_INTR(SP_INTERRUPT); 
		InterruptNeeded |=SP_INTERRUPT;
	} 
	((u32*)SP2)[0] = 0x1000;
}

void SignalDeadRDP(void) {
	extern u8* DPC;
	extern u8* MI;
	((u32*)DPC)[2] = ((u32*)DPC)[1];
	((u32*)DPC)[3] |= 0x81;/*
	SET_INTR(DP_INTERRUPT); */
	InterruptNeeded |= DP_INTERRUPT;
}