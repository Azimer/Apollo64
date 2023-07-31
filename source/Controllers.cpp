#include <windows.h>
#include <stdio.h>
#include <commctrl.h> // For status window
#include "winmain.h"
#include "emumain.h"
#include "plugin.h"
#include "inputdll.h"
#include "pif2.h"

// ****************** Interfacing for the Controller plugin ****************************

INPUTDLL inpdll;
CONTROL ContInfo[4];
BUTTONS keys;

extern char eeprom[2*1024];
extern char MemPacks[0x8032];
extern bool MPinUse, EEPinUse;

static bool LoadFunctions () {
	inpdll.CloseDLL			 = (void (__cdecl*)( void		   )) GetProcAddress(inpdll.hinstLibInput, "CloseDLL");
	inpdll.ControllerCommand = (void (__cdecl*)( int, BYTE *   )) GetProcAddress(inpdll.hinstLibInput, "ControllerCommand");
	inpdll.DllAbout			 = (void (__cdecl*)( HWND          )) GetProcAddress(inpdll.hinstLibInput, "DllAbout");
	inpdll.DllConfig		 = (void (__cdecl*)( HWND          )) GetProcAddress(inpdll.hinstLibInput, "DllConfig");
	inpdll.DllTest			 = (void (__cdecl*)( HWND          )) GetProcAddress(inpdll.hinstLibInput, "DllTest");
	inpdll.GetDllInfo		 = (void (__cdecl*)( PLUGIN_INFO * )) GetProcAddress(inpdll.hinstLibInput, "GetDllInfo");
	inpdll.GetKeys			 = (void (__cdecl*)( int, BUTTONS *)) GetProcAddress(inpdll.hinstLibInput, "GetKeys");
	inpdll.InitiateControllers = (void (__cdecl*)( HWND, CONTROL[])) GetProcAddress(inpdll.hinstLibInput, "InitiateControllers");
	inpdll.ReadController	 = (void (__cdecl*)( int, BYTE *   )) GetProcAddress(inpdll.hinstLibInput, "ReadController");
	inpdll.RomClosed		 = (void (__cdecl*)( void		   )) GetProcAddress(inpdll.hinstLibInput, "RomClosed");
	inpdll.RomOpen			 = (void (__cdecl*)( void		   )) GetProcAddress(inpdll.hinstLibInput, "RomOpen");
	inpdll.WM_KeyDown		 = (void (__cdecl*)( WPARAM, LPARAM)) GetProcAddress(inpdll.hinstLibInput, "WM_KeyDown");
	inpdll.WM_KeyUp			 = (void (__cdecl*)( WPARAM, LPARAM)) GetProcAddress(inpdll.hinstLibInput, "WM_KeyUp");
	return true;
}

void CloseInputPlugin () {
  if(inpdll.CloseDLL)
	  inpdll.CloseDLL();

  if(inpdll.hinstLibInput)
	  FreeLibrary(inpdll.hinstLibInput); 

  ZeroMemory (&inpdll, sizeof(INPUTDLL));
}

BOOL LoadInputPlugin (char *libname) {
	PLUGIN_INFO Plugin_Info;
	ZeroMemory (&Plugin_Info, sizeof(Plugin_Info));

	if (libname == NULL) {
		return FALSE;
	}
	
	if (inpdll.hinstLibInput != NULL) 
		inpdll.Close ();

	inpdll.hinstLibInput = LoadLibrary(libname);

	if (inpdll.hinstLibInput == NULL) {
		Debug (0, "Could not load %s because it couldn't be found!", libname);
		inpdll.Close();
		return FALSE;
	}

	inpdll.GetDllInfo = (void (__cdecl*)(PLUGIN_INFO *)) GetProcAddress(inpdll.hinstLibInput, "GetDllInfo");
	if(!inpdll.GetDllInfo) {
		Debug (0, "%s does not conform to the plugin spec.", libname);
		return FALSE;//This isn't a plugin dll, so don't bother.	
	}
	inpdll.GetDllInfo(&Plugin_Info);

	if(Plugin_Info.Type == PLUGIN_TYPE_CONTROLLER) {
		if(Plugin_Info.Version > PLUGIN_INP_VERSION) {
			Debug (0, "%s is not compatable with Apollo. %X != %X", libname, Plugin_Info.Version, PLUGIN_INP_VERSION);
			inpdll.Close();
			return FALSE;
		}

		if (LoadFunctions () == false) {
			inpdll.Close();
			return FALSE;
		}
	} else {
		//Here we insert Audio code, controller code, etc.
		Debug (0, "%s dll isn't an Input plugin!", libname);
		inpdll.Close();
		return FALSE;
	}
	
	//strcpy(RegSettings.vidDll,libname);
	return TRUE;
}


//#define _PIF_DEBUGGING_ENABLED_
#ifdef _PIF_DEBUGGING_ENABLED_
	#pragma message("WARNING: PIF DEBUGGING IS ENABLED!!!")
#endif

#pragma warning( disable : 4035 )

inline DWORD SWAP_DWORD(DWORD y) {
	__asm { 
		mov eax,y 
		bswap eax 
	}
} 

#pragma warning( default : 4035 )

u8 bspif[64];
int volatile cmdcntr=0;
int volatile pifsend=0;
int volatile pifrecv=0;
int volatile cmd=0;
int volatile port=0;


// w00p w00p... Azi AM MAS DISASSEMBLER AND REing DUDE.. KTHX MEMPACKS!!!!
BYTE __osContDataCrc (BYTE *A0) {
  BYTE  Var00F, Var00E;
  WORD Var008;
  short Var004;
  WORD A1=0;

  Var00F = 0;
  for (Var008 = 0; Var008 < 0x21; Var008++) { // 32 bytes
	 Var004 = 7;
	 while (Var004 >= 0) { // 8 bits
		if (Var00F & 0x80)
		  Var00E = 0x85;
		else
		  Var00E = 0;

		Var00F = Var00F << 1;
		if (Var008 == 0x20) {
		  Var00F = Var00F & 0xFF;
		} else { // 108
		  if (A0[0] & (1 << (Var004 & 0x1f)))
			 A1 = 1;
		  else
			 A1 = 0;
		  Var00F = Var00F | A1;
		}// 13C
		Var004 = Var004 + 0xFFFF;
		Var00F = Var00F ^ Var00E;
	 } // end while
	 A0++;
  } // end for
  return (Var00F);
}

#ifdef _PIF_DEBUGGING_ENABLED_
FILE *sifp = fopen ("d:\\pif.txt", "wt");
#endif

extern bool is16kEEP;
u32 eepDetectToggle = 0x6FA0;
void DisassembleRange (u32 Start, u32 End);

#define IncreaseMaxPif2 300
int MaxPif2Cmds = 300;
/*
unsigned __int64 * Pif2Reply[4];

char * GetPif2FileName(void) {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT];
	static char IniFileName[_MAX_PATH];

	GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
	_splitpath( path_buffer, drive, dir, fname, ext );
	sprintf(IniFileName,"%s%s%s",drive,dir,"pif2.dat");
	return IniFileName;
}

BOOLEAN pif2valid = FALSE;

void LoadPIF2 () {
	FILE *pif2db = fopen (GetPif2FileName(), "rt");
//	unsigned __int64 p1, p2, r1, r2;
	char buff[255];
	int cnt = 0;
	
	Pif2Reply[0] = (unsigned __int64 *)malloc((MaxPif2Cmds + 1) * sizeof(__int64));
	Pif2Reply[1] = (unsigned __int64 *)malloc((MaxPif2Cmds + 1) * sizeof(__int64));
	Pif2Reply[2] = (unsigned __int64 *)malloc((MaxPif2Cmds + 1) * sizeof(__int64));
	Pif2Reply[3] = (unsigned __int64 *)malloc((MaxPif2Cmds + 1) * sizeof(__int64));

	if (Pif2Reply[0] == NULL || Pif2Reply[1] == NULL ||
		Pif2Reply[2] == NULL || Pif2Reply[3] == NULL) 
	{
		Debug (1, "Error with allocating memory for PIF2");
		ExitThread(0);
	}
	if (pif2db == NULL) {
		Pif2Reply[0][0] = -1; Pif2Reply[1][0] = -1; 
		Pif2Reply[2][0] = -1; Pif2Reply[3][0] = -1;
		return;
	}
	
	while (feof (pif2db) == 0) {
		if (cnt == MaxPif2Cmds) {
			MaxPif2Cmds += IncreaseMaxPif2;
			Pif2Reply[0] = (unsigned __int64 *)realloc(Pif2Reply[0],(MaxPif2Cmds + 1) * sizeof(__int64));
			Pif2Reply[1] = (unsigned __int64 *)realloc(Pif2Reply[1],(MaxPif2Cmds + 1) * sizeof(__int64));
			Pif2Reply[2] = (unsigned __int64 *)realloc(Pif2Reply[2],(MaxPif2Cmds + 1) * sizeof(__int64));
			Pif2Reply[3] = (unsigned __int64 *)realloc(Pif2Reply[3],(MaxPif2Cmds + 1) * sizeof(__int64));
			if (Pif2Reply[0] == NULL || Pif2Reply[1] == NULL ||
				Pif2Reply[2] == NULL || Pif2Reply[3] == NULL) 
			{
				Debug (1, "Error with allocating memory for PIF2");
				ExitThread(0);
			}
		}
		fgets (buff, 255, pif2db);
		if (buff[0] != ';') {
			if (buff[0] != '\n') {
				sscanf (buff, "0x%08X%08X, 0x%08X%08X, 0x%08X%08X, 0x%08X%08X", 
				((DWORD *)&Pif2Reply[0][cnt])+1, &Pif2Reply[0][cnt], 
				((DWORD *)&Pif2Reply[1][cnt])+1, &Pif2Reply[1][cnt],
				((DWORD *)&Pif2Reply[2][cnt])+1, &Pif2Reply[2][cnt], 
				((DWORD *)&Pif2Reply[3][cnt])+1, &Pif2Reply[3][cnt]);
				cnt++;
			}
		}
	}

	Pif2Reply[0][cnt] = Pif2Reply[1][cnt] = Pif2Reply[2][cnt] = Pif2Reply[3][cnt] = -1;
	
	pif2valid = TRUE;

	fclose (pif2db);
}
*/
void ReadPif2 () {
		int cnt = 0;
		char buff[256];
/*		if (pif2valid == FALSE)
			LoadPIF2 ();*/
		while (Pif2Reply[cnt][0] != -1) {
			if (Pif2Reply[cnt][0] == *(unsigned __int64 *)&bspif[48]) {
				if (Pif2Reply[cnt][1] == *(unsigned __int64 *)&bspif[56]) {
					bspif[46] = bspif[47] = 0x00;
					/*Debug (0,  "P1=%08X%08X P2=%08X%08X",   *(DWORD *)&bspif[52],
																*(DWORD *)&bspif[48],
																*(DWORD *)&bspif[60],
																*(DWORD *)&bspif[56]);*/
					*(unsigned __int64 *)&bspif[48] = Pif2Reply[cnt][2];
					*(unsigned __int64 *)&bspif[56] = Pif2Reply[cnt][3];
					/*Debug (0,  "R1=%08X%08X R2=%08X%08X\r\n",   *(DWORD *)&bspif[52],
																*(DWORD *)&bspif[48],
																*(DWORD *)&bspif[60],
																*(DWORD *)&bspif[56]);*/
					cnt = -1;
					break;
				}
			}
			cnt++;
		}
		if (cnt != -1) {
			char buff2[256];
			int count;

			sprintf (buff, "Copyright sequence not found in LUT.  Game will no longer function. :(\r\n\r\nInfo:\r\nP1=%08X%08X P2=%08X%08X\r\n", 
				*(DWORD *)&bspif[52],
				*(DWORD *)&bspif[48],
				*(DWORD *)&bspif[60],
				*(DWORD *)&bspif[56]);
			for (count = 48; count < 64; count++) {
				if (count % 4 == 0) { 
					strcat(buff,count == 48?"0x":", 0x");
				}
				sprintf(buff2,"%02X",bspif[count]);
				strcat(buff,buff2);
			}
			Debug (1, buff);
		}
}

void DoPifEmulation (unsigned char *dest, unsigned char *src) {
	int QuickEscape = 0; // Allows me to leave the pif emulation quickly for unsupported stuff :)
	u8 cmdcntr=0;
	u8 pifsend=0;
	u8 pifrecv=0;
	u8 cmd=0;
	u8 port=0;
	int cnt = 0;

	//Debug (0, "SI");

	__asm { // Byte Swap the damn thing... make it easy on me...
		push edi;
		mov edi, dword ptr [src];
		mov ecx,0;
pifstart:
		mov eax, [edi];
		add edi, 4;
		bswap eax;
		mov dword ptr [bspif+ecx], eax;
		add ecx, 4;
		cmp ecx, 64;
		jne pifstart;
		pop edi;
	}

#ifdef _PIF_DEBUGGING_ENABLED_
	fprintf (sifp, "Start pif\n");
	for (cnt=0; cnt < 64; cnt+=8) {
		fprintf (sifp, "%02X %02X %02X %02X - %02X %02X %02X %02X\n", 
			bspif[cnt+0], bspif[cnt+1], bspif [cnt+2], bspif[cnt+3],
			bspif[cnt+4], bspif[cnt+5], bspif [cnt+6], bspif[cnt+7]);
	}
	fprintf (sifp, "\n");
#endif
	extern HANDLE AdaptoidHandle;
	if (AdaptoidHandle) {
		DWORD nbytes=0;
		WriteFile(AdaptoidHandle,bspif,64,&nbytes,NULL);
		ReadFile(AdaptoidHandle,bspif,64,&nbytes,NULL);
	} else {
		if (bspif[63] == 0x2) {
			int cnt;
			ReadPif2 ();
			Debug (0, "PIF2");
		}
		do {
			if (bspif[63] != 0x1)
				break;
			while (bspif[cmdcntr] == 0xFF)
				cmdcntr++;
			if (bspif[cmdcntr] == 0xFE)
				break;
			if (cmdcntr >= 64)
				break;
			pifsend = bspif[cmdcntr];
			if (pifsend != 0x00) {
				pifrecv = bspif[cmdcntr+1];
				if (pifrecv >= 0xFE)
					break;
				cmd  = bspif[cmdcntr+2]; // At least one value is being sent :)
				switch (cmd) {
// *************************************************************************
// Port status (controllers basically)
				case 0xFF:
				case 0x00:
					if (port < 4) {
						if (ContInfo[port].Present == TRUE) {
							if (pifrecv > 3) {
								if (port == 0)
									Debug (0, "Controller Error");
								bspif[cmdcntr+1] |= 0x40;
								bspif[cmdcntr+3] = 0xFF;
								bspif[cmdcntr+4] = 0xFF;
								bspif[cmdcntr+5] = 0xFF; // TODO: When you get packs done modify this!
								break;
							}
							/*
							if (port == 0)
								Debug (0, "Controller Found");*/
							bspif[cmdcntr+3] = 0x05;
							bspif[cmdcntr+4] = 0x00;
							bspif[cmdcntr+5] = 0x01; // TODO: When you get packs done modify this!
							//Debug (0, "Controller Detect");
						} else {
							if (port == 0)
								Debug (0, "Controller Missing");
							bspif[cmdcntr+1] |= 0x80;
							bspif[cmdcntr+3] = 0xFF;
							bspif[cmdcntr+4] = 0xFF;
							bspif[cmdcntr+5] = 0xFF; // TODO: When you get packs done modify this!
						}
					} else { // EEPROM - TODO: Perhaps an INI with saveram type
						/*if (pc == 0x800D350C) { // WaveRace SE Hack
							if (is16kEEP == false) {
								is16kEEP = true;
								Debug (0, "WaveRace SE Hack enabled");
								Debug (0, "EEPROM will not be saved.");
							}
						}*/

						//is16kEEP = false;
						//is16kEEP = true;
							
						if ((is16kEEP == true)) {
							bspif[cmdcntr+3] = 0x00; // 512 = 0x00 - 2K = 0x00
							bspif[cmdcntr+4] = 0xC0; // 512 = 0x80 - 2K = 0xC0
							bspif[cmdcntr+5] = 0x00;
						} else {
							bspif[cmdcntr+3] = 0x00; // 512 = 0x00 - 2K = 0x00
							bspif[cmdcntr+4] = 0x80; // 512 = 0x80 - 2K = 0xC0
							bspif[cmdcntr+5] = 0x00;
						}
						//Debug (0, "EEPRom probe %08X", instructions);

						
						/*if (eepDetectToggle == 0) {
							Debug (0, "EEPRom probe");
							bspif[cmdcntr+3] = (u8)(eepDetectToggle >> 16);
							bspif[cmdcntr+4] = (u8)(eepDetectToggle >> 8);
							bspif[cmdcntr+5] = (u8)(eepDetectToggle);
							eepDetectToggle++;
						//}*/
					}
					break;
// *************************************************************************
// Read Controller
				case 0x01:
					//if (port == 0) {
					if (ContInfo[port].Present == TRUE) {
						inpdll.GetKeys(port, &keys);
						memcpy (&bspif[cmdcntr+3], &keys, 4);
					}
					else {
						bspif[cmdcntr+1] |= 0x80;
					}					
					break;
// *************************************************************************
// Read Mempack
				case 0x02: {
					u16 offset;

					if (port > 0)
						return;

					offset = ((u16)bspif[cmdcntr+3] << 8) | ((u16)bspif[cmdcntr+4] & 0xE0);

					if (RegSettings.RumblePack == true) {
						memset (&bspif[cmdcntr+5], 0x80, 32);
					} else {
						//Debug (0, "Controller Read From Offset: %08X", offset);
						if (offset < 0x8001) {
							memcpy(&bspif[cmdcntr+5], &MemPacks[offset], 32);
						} else {
							//Debug (0, "Controller Pack Read Offset out of range: %08X", offset);
							//memset (&bspif[cmdcntr+5], 0x00, 32);
						}
					}
					//Debug (0, "Controller Pack Read Offset out of range: %08X", offset);

					bspif[cmdcntr+37] = __osContDataCrc(&bspif[cmdcntr+5]);
					//if (*src != 0xFE)
					//	src++; // Sometimes there is a fucked up extra byte which screws me over >=/  I AM NOT THE PIF DAMNIT!!!
					//if (RegSettings.RumblePack)
/*
					for (int x = 0; x < 64; x++) {
						fprintf (tempfile, "%02X", bspif[x]);
						if ((x & 0x3)==0x3)
							fprintf (tempfile, "\n");
					}*/
	//if (offset > 0x400)*/
				}
				break;
// *************************************************************************
// Write Mempack
				case 0x03: {
					u16 offset;

					if (port > 0)
						return;

					offset = ((u16)bspif[cmdcntr+3] << 8) | ((u16)bspif[cmdcntr+4] & 0xE0);
					if (RegSettings.RumblePack == false) {
						if (offset < 0x8001) {
							//Debug (0, "Controller Write to Offset: %08X", offset);
							memcpy(&MemPacks[offset], &bspif[cmdcntr+5], 32);
							if (MPinUse == false) {
								MPinUse = true;
								Debug (0, "Detected MemPacks");
							}
						} else {
							Debug (0, "Controller Pack Write Offset out of range: %08X", offset);
						}
					} else {
						if ((offset == 0xC000) && (bspif[cmdcntr+5])) {
							SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) "***Rumble***");
						} else {
							SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) "");
						}
					}
					bspif[cmdcntr+37] = __osContDataCrc(&bspif[cmdcntr+5]);
//					QuickEscape = 1;
				}
				break;
// *************************************************************************
// Read EEPROM
				case 0x04: {
					if (port < 4) {
						bspif[cmdcntr+1] |= 0xC0; // Error it all out ;)
						break;
					}
					int address = bspif[cmdcntr+3] << 3;
					if (address < 2048)
						memcpy (&bspif[cmdcntr+4], &eeprom[address], 8);
					if (EEPinUse == false) {
						EEPinUse = true;
						Debug (0, "Detected EEPRom");
					}
					break;
				}
// *************************************************************************
// Write EEPROM
				case 0x05: {
					if (port < 4) {
						bspif[cmdcntr+1] |= 0xC0; // Error it all out ;)
						break;
					}
					int address = bspif[cmdcntr+3] << 3;
					if (address < 2048) {
						memcpy (&eeprom[address], &bspif[cmdcntr+4], 8);
					}

					if (EEPinUse == false) {
						EEPinUse = true;
						Debug (0, "Detected EEPRom");
					}
					cmdcntr--;
					break;
				}
// *************************************************************************
				default:
					QuickEscape = 1;
					for (cnt=0; cnt < 64; cnt+=8) {
						Debug (0, "%02X %02X %02X %02X - %02X %02X %02X %02X\n", 
								bspif[cnt+0], bspif[cnt+1], bspif [cnt+2], bspif[cnt+3],
								bspif[cnt+4], bspif[cnt+5], bspif [cnt+6], bspif[cnt+7]);
						}
					;//Debug (0, L_STR(IDS_PIF_UK), cmd);
//					bspif[49] = 0x40; bspif[63] = 0x00;
				}
				cmdcntr += bspif[cmdcntr] + (bspif[cmdcntr+1] & 0x3f) + 2;
			} else {
				cmdcntr++;
			}
			port++;
		} while (QuickEscape==0);
	}

#ifdef _PIF_DEBUGGING_ENABLED_
	fprintf (sifp, "End pif\n");
	for (cnt=0; cnt < 64; cnt+=8) {
		fprintf (sifp, "%02X %02X %02X %02X - %02X %02X %02X %02X\n", 
			bspif[cnt+0], bspif[cnt+1], bspif [cnt+2], bspif[cnt+3],
			bspif[cnt+4], bspif[cnt+5], bspif [cnt+6], bspif[cnt+7]);
	}
	fprintf (sifp, "\n");
#endif

//bspif[63] = 0;

	__asm { // Save our pif information
		push edi;
		mov edi, dword ptr [dest];
		mov ecx,0;
pifstart2:
		mov eax, dword ptr [bspif+ecx];
		bswap eax;
		mov [edi], eax;
		add ecx, 4;
		add edi, 4;
		cmp ecx, 64;
		jne pifstart2;
		pop edi;
	}
}

/*
bool MempackEnabled[4] = {false, false, false, false};
bool RumblepackEnabled[4] = {false, false, false, false};
bool JoyIsDisabled[4] = {false, true, true, true}; // Emulated Joy is disabled
unsigned char EEPROMDATA[2048];
unsigned char MEMPACKDATA[0x8000];
extern bool usesEEPROM;
extern bool usesMEMPACK;
extern DWORD ContStats[4];
//extern HANDLE AdaptoidHandle;
//extern int nAdaptoids;
extern byte CopyOfPif[64];


void DoPifEmulation (unsigned char *dest, unsigned char *src) {
	unsigned char numsend; // Number of bytes to send the pif
	unsigned char numget;  // Number of bytes get in return
	unsigned char command; // Command #
	unsigned char *srcsave = src;
	//int AdaptoidPos=0;
	int channel=0;  // Current channel the pif is looking at

	while (1) { // While end command list signal not gotten
		while (*src == 0xFF)
			src++;
		if (*src == 0xFE)
			break;
		numsend = *src++;
		if (numsend) { // it's not zero # of things to send then do nothing
			numget  = *src; // WARN: What if this is zero?
			command = src[1];   // Notice I didn't increment so it makes controllers easier
			switch (command) {
				case 0x00: // Controller status
					if (channel < 4) {
						if (JoyIsDisabled[channel]) {
							*src |= 0x80; // Not connected
							src += 5; // skip numget, command, and 3 return values
						} else {
							src[2] = 0x05;
							src[3] = 0x00;
							if (RumblepackEnabled[channel] || MempackEnabled[channel])
								src[4] = 1;
							else
								src[4] = 2;
							src += 5;
						}
					} else {
						src[2] = 0x02; // 0x00
						src[3] = 0x00; // 0x80
						src[4] = 0x00;
						src += 5;
					}
				break;
				case 0x01: // Controller read
					if (JoyIsDisabled[channel]) {
						*src |= 0x80; // Not connected
						src += 6; // skip numget, command, and 4 return values
					} else {
						unsigned char *tmp;
						src += 2;
						tmp = (unsigned char *)&ContStats[channel];
						__asm {
							mov eax, dword ptr [tmp];
							mov ecx, [eax];
							bswap ecx;
							mov eax, dword ptr [src];
							mov [eax], ecx;
						}
						src += 4;
					}
				break;
				case 0x02: { // Read MEMPACK
					src += 2;
					int address;
					address = *src++ << 8; // High Byte
					address |= (*src & 0xE0); // Low Byte
					//unsigned byte addresscrc = (*src & 0x1F); // Address CRC (unused)
					src++;
					if ((address < 0x7F81) && MempackEnabled[channel]){
						memcpy (src, &MEMPACKDATA[address], 32);
					} else {
						if (RumblepackEnabled[channel])
							memset (src, 0x80, 32);
						else
							memset (src, 0x00, 32);
					}

					src[32] = __osContDataCrc(src);
					src += 33; // 32 bytes + CRC
					if (*src != 0xFE)
						src++; // Sometimes there is a fucked up extra byte which screws me over >=/  I AM NOT THE PIF DAMNIT!!!
				}
				break;
				case 0x03: // Write MEMPACK
					src += 2;
					int address;
					address = *src++ << 8; // High Byte
					address |= (*src & 0xE0); // Low Byte
					//unsigned byte addresscrc = (*src & 0x1F); // Address CRC (unused)
					src++;
					if ((address < 0x7F81) && MempackEnabled[channel]){
						memcpy (&MEMPACKDATA[address], src, 32);
						usesMEMPACK = true;
					} else {
						if (address == 0xC000) {
							if (*src)
								SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) "***Rumble***");
						//	else
						//		SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPSTR) "");
						}
					}

					src[32] = __osContDataCrc(src);
					src += 33; // 32 bytes + CRC
					if (*src != 0xFE)
						src++; // Sometimes there is a fucked up extra byte which screws me over >=/  I AM NOT THE PIF DAMNIT!!!
					break;
				case 0x04: { // EEPROM Read
					if (channel < 4) {
						*src |= 0xC0; // Error it all out ;)
						*src += (2 + numsend + numget);
						break;
					}
					src += 2;
					int address = *src++ << 3;
					if (address < 512)
						memcpy (src, &EEPROMDATA[address], 8);
					src += 8;
				}
				break;
				case 0x05:{ // EEPROM Write
					if (channel < 4) {
						*src |= 0xC0; // Error it all out ;)
						*src += (2 + numsend + numget);
						break;
					}
					src += 2;
					int address = *src++ << 3;
					if (address < 512) {
						memcpy (&EEPROMDATA[address], src, 8);
						usesEEPROM = true;
					}
					src += 8;
				}
				break;
				default:{
					if (command == 0xFF)
						break; // Mario64 likes FF0103FF for some odd reason... FF is not valid
					Log(7, "Unknown PIF Command... please report this!!!");
				}
			} // end switch
		} // end if
		channel++; // this channel is done...
	} // end while
	memcpy (dest, srcsave, 64); // Copy all of it over...
}
*/