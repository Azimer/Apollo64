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
#include "audiodll.h"
#include "resource.h"


void SignalDeadRSP(DWORD *data);
void SignalDeadRDP(void);
int activeDPplugin = 0;
int activeAIplugin = 0;

extern "C" {
	void rsp_run();
	char *pRDRAM;
	char *pDMEM;
	char *pIMEM;
}


//----------------------------------------------------------------------------------------------

unsigned long GenerateCRC (unsigned char *data, int size);

extern u8* idmem;
extern u8* SP;
extern u8* SP2;
extern u8* rdram;

HWND g_hWnd;

//	pInterruptMask = (int*)Audio_Info.MI_INTR_REG;

DWORD crclist[0x200];
/*
void AudioHLE () {
	u32 Boot  = ((u32*)idmem)[0xFC8/4], BootLen = ((u32*)idmem)[0xFCC/4];
	u32 UC    = ((u32*)idmem)[0xFD0/4], UCLen   = ((u32*)idmem)[0xFD4/4];
	u32 UCData= ((u32*)idmem)[0xFD8/4], UDataLen= ((u32*)idmem)[0xFDC/4];
	u32 List  = ((u32*)idmem)[0xFF0/4], ListLen = ((u32*)idmem)[0xFF4/4];
	u32 *HLEPtr= (u32 *)(rdram+List);
	u32 t9, k0;
	//FILE *dfile = fopen ("d:\\audio.txt", "wt");
	
	for (u32 x=0; x < 0x200; x++) {
		if (crclist[x] == 0) {
			crclist[x] = GenerateCRC ((idmem+0x1080), 0xe12); //e14
			if (crclist[x] != 0)
				Debug (0, "RSP Audio UCode CRC: %08X", crclist[x]);
			break;
		} else {
			if (crclist[x] == GenerateCRC ((rdram+UC), 0xe14))
				break;
		}
	} /*
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
/*}
*/

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
void RspDump ();
void DisassembleRange (u32 Start, u32 End);
u32 dlists=0, alists = 0;
//Purpose: called when bit 0 is set in the SP_STATUS. dispatch to correct HLE emulation function.
void SignalRSP(DWORD *data) {
	static int hack=0;
//	char buff[256];
//	u32 crc, crc2;
	switch (((u32*)idmem)[0xFC0/4]) {
		case 0x1: // Graphics Task
			//dprintf ("+");
			//if (dlists < 6) {
/*                0x04000fd0: ucode
                0x04000fd4: ucode length
                0x04000fd8: ucode data
                0x04000fdc: ucode data length*/
/*			if (dlists == 0) {
				crc = GenerateCRC (rdram + *(DWORD *)(idmem + 0xfd8), 0x120);
				switch (crc) {
					case 0x8FC89DAA:
						Debug (0, "GBI AutoDetected to be Custom - (Perfect Dark)");
					break;
					case 0xEFC8835B:
						Debug (0, "GBI AutoDetected to be Custom - (Golden Eye 007)");
					break;
					case 0xE5518116: // Diddy Kong Racing!
						Debug (0, "GBI AutoDetected to be Custom - (Diddy King Racing)");
					break;
					case 0x7C53D1EC:
						Debug (0, "GBI AutoDetected to be Microcode 1 - (BlastCorps)");
					break;
					break;
					case 0x4B4F89F5:
						Debug (0, "GBI AutoDetected to be Microcode 1 - (Intro by TRSI + LFC)");
					break;
					case 0x94706A47:
						Debug (0, "GBI AutoDetected to be Microcode 1 - (Demos)");
					break;
					case 0xBDF1319D:
						Debug (0, "GBI AutoDetected to be Custom - (Rouge Squadron)");
					break;
					case 0xBBEF0F84:
						Debug (0, "GBI AutoDetected to be Microcode 0 - (WaveRace USA, and ???)");
					break;
					case 0x24C2EF91: // Mortal Kombat Trilogy - Might be a spriteonly GBI
						Debug (0, "GBI AutoDetected to be Microcode 1 - (MKT)");
					break;
					case 0x6370661B:
						Debug (0, "GBI AutoDetected to be Microcode 1 - (Mario64, Crusin' USA)");
					break;
					case 0x9268F83F:
						crc = GenerateCRC (rdram + *(DWORD *)(idmem + 0xfd0), 0x120);
						if (crc == 0x4B2A7FED) {
							Debug (0, "GBI Autodetect to be Microcode 3 Variant - (Conker's Bad Fur Day)");
						} else {
							Debug (0, "GBI Autodetect to be Microcode 3 - (Zeldas, popular)");
						}
					break;
					default:
						if (*(DWORD *)(0x2B0 + rdram + *(DWORD *)(idmem + 0xfd8)) == 0x52535020) {
							Debug (0, "GBI Autodetect to be Microcode 2 - %08X", crc);
							
							//A339760E - Robotron
							//A339760E - Banjo Kazooie
							//59F11356 - 1080
							//A339760E - WaveRace SE
							//27D68C16 - Mario Kart 64
							//A339760E - WCW/NWO World Tour
							//C90F421D - SF Rush
							//A339760E - Rampage
							//7BABBDB2 - NHL Breakaway
							//59F11356 - Virtual Pool
							
						} else {
							Debug (0, "GBI Autodetect failed - %08X", crc);
							//FILE *dfile;
							//int i;
							//u8 *dm = rdram + *(DWORD *)(idmem + 0xfd8);
							//u32 len = *(DWORD *)(idmem + 0xfdc);
							//char str[0x10];
							//dfile = fopen ("c:\\dump.txt", "wt");
							//for (i=0; i < len; i++) {
							//	if ((i & 0xF) == 0x0)
							//		fprintf (dfile, "%04X: ", i);
							//	fprintf (dfile, "%02X ", dm[i^3]);
							//	str[i&0xf] = dm[i^3];
							//	if ((i & 0xF) == 0xF)
							//		fprintf (dfile, "\n", str);
							//}
							//fclose (dfile);
							Debug (0, "Unknown GBI: %08X - %08X", crc, *(DWORD *)(rdram + *(DWORD *)(idmem + 0xfd8)));
						}
				}
			}
*/
			dlists++;
			//sprintf (buff, "Dlists: %i  Alists: %i", dlists, alists);
			//SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) buff);
			//memcpy (&idmem[0x1080], &rdram[((u32*)idmem)[0xFD0/4]], 0xF80);
			//memcpy (&idmem[0x0], &rdram[((u32*)idmem)[0xFD8/4]], ((u32*)idmem)[0xFDC/4]);
			//RspDump ();
			//__asm int 3;
			gfxdll.ProcessDList();
			SignalDeadRSP(data);
			SignalDeadRDP();
			//instructions += 0x10000;
		break;
		case 0x2: // Audio Task
			//dprintf ("-");
			//if (alists < 6) {
				//Debug (0, "SND UCode CRC: %08X", GenerateCRC (rdram + *(DWORD *)(idmem + 0xfd8)+0x50, 0x120)); // *(DWORD *)(idmem+0xfdc)
			//}
			alists++;
			//sprintf (buff, "Dlists: %i  Alists: %i", dlists, alists);
			//SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) buff);
			//Debug (0, "BootCode: %08X Length: %08X", ((u32*)idmem)[0xFC8/4], ((u32*)idmem)[0xFCC/4]);
			//Debug (0, "UCode   : %08X Length: %08X", ((u32*)idmem)[0xFD0/4], ((u32*)idmem)[0xFD4/4]);
			//Debug (0, "UC Data : %08X Length: %08X", ((u32*)idmem)[0xFD8/4], ((u32*)idmem)[0xFDC/4]);
			//Debug (0, "List Dat: %08X Length: %08X", ((u32*)idmem)[0xFF0/4], ((u32*)idmem)[0xFF4/4]);
			//Debug (0, "AL1: %08X  AL2: %08X", ((u32*)rdram)[((u32*)idmem)[0xFF0/4]/4], ((u32*)rdram)[(((u32*)idmem)[0xFF0/4]+4)/4]);
			//AudioHLE ();
			//cpuIsDone = true;
			//cpuIsReset = true;
			/*if (hack < 80)
				hack++;
			if (hack == 79) {
				SignalDeadRDP ();
				hack = 1000;
			}*/
			snddll.ProcessAList();
			pRDRAM = (char *)rdram;
			pDMEM = (char *)idmem;
			pIMEM = (char *)idmem + 0x1000;
			//rsp_run ();
			SignalDeadRSP(data);
		break;
		case 0x4: // Texture Decoder (guess I need special graphics plugin for this)
			//pRDRAM = (char *)rdram;
			//pDMEM = (char *)idmem;
			//pIMEM = (char *)idmem + 0x1000;
			void zlist_uncompress();
			zlist_uncompress();
			//rsp_run ();
			SignalDeadRSP(data);
			Debug(0,"Texture Decoder");
		break;
		default:
			Debug(0,"%08X: RSP/??? signaled to start. DataType is %d", pc, ((u32*)idmem)[0xFC0/4]);
			pRDRAM = (char *)rdram;
			pDMEM = (char *)idmem;
			pIMEM = (char *)idmem + 0x1000;
			//rsp_run();
			//BeginRSP ();
			//RspDump ();
			//DisassembleRange (0xA4000000, 0xA4002000);
			SignalDeadRSP(data);
	}
}

//Purpose: called when a valid start and end is written to DP registers. Normally is not used
//  because most of the SP ucodes internally use the RDP, so the R4K usually doesn't use this.
void SignalRDP(void) {
	extern u8* SP;
	extern u8* DPC;
	extern u8* MI;
	//InterruptNeeded |= SI_INTERRUPT;
	//SET_INTR(SI_INTERRUPT);
	//SignalDeadRSP (&((u32*)SP)[4]);
	SignalDeadRDP ();
}

void CheckRDP() {
	extern u8* MI;
    //if ((MI_INTR_REG_R & MI_INTR_SP) != 0) Trigger_RSPBreak();
    if ((((u32*)MI)[2] & DP_INTERRUPT) != 0)  {
		SignalDeadRDP();
	}
}

void SignalDeadRSP(DWORD *data) {
	extern u8* SP;
	extern u8* SP2;
	extern u8* MI;
	//((u32*)SP)[4] |= 0x303;
	//*data |= 0x303;
	*data |= 0x203;
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
	//DPC_STATUS_REG(m_MemoryData) = 0x801;
	((u32*)DPC)[2] = ((u32*)DPC)[1];
	((u32*)DPC)[3] = 0x801;
	SET_INTR(DP_INTERRUPT);/* */
	InterruptNeeded |= DP_INTERRUPT;
}