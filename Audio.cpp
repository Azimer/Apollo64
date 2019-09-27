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
 *		06-24-00	Initial Version (Andrew)
 *
 **************************************************************************/
/**************************************************************************
 *
 *  Notes:
 *	
 *	-Designed to have the Audio interrupt fire at about the same time it
 *	 would as if we were going at 60FPS. Also correctly emulates everything
 *	 except for the LEN register, which I will have to verify. -KAH
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
#include "audiodll.h"


// ****************** Interfacing for the Controller plugin ****************************

AUDIODLL snddll;
void SignalAiDone(void);

static bool LoadFunctions () {
	snddll.CloseDLL			 = (void (__cdecl*)( void		   )) GetProcAddress(snddll.hinstLibAudio, "CloseDLL");
	snddll.DllAbout			 = (void (__cdecl*)( HWND          )) GetProcAddress(snddll.hinstLibAudio, "DllAbout");
	snddll.DllConfig		 = (void (__cdecl*)( HWND          )) GetProcAddress(snddll.hinstLibAudio, "DllConfig");
	snddll.DllTest			 = (void (__cdecl*)( HWND          )) GetProcAddress(snddll.hinstLibAudio, "DllTest");
	snddll.ProcessAList		 = (void (__cdecl*)( void		   )) GetProcAddress(snddll.hinstLibAudio, "ProcessAList");
	snddll.RomClosed		 = (void (__cdecl*)( void		   )) GetProcAddress(snddll.hinstLibAudio, "RomClosed");
	snddll.InitiateAudio	 = (BOOL (__cdecl*)( AUDIO_INFO	   )) GetProcAddress(snddll.hinstLibAudio, "InitiateAudio");
	snddll.GetDllInfo		 = (void (__cdecl*)( PLUGIN_INFO * )) GetProcAddress(snddll.hinstLibAudio, "GetDllInfo");
	snddll.AiDacrateChanged	 = (void (__cdecl*)( int		   )) GetProcAddress(snddll.hinstLibAudio, "AiDacrateChanged");
	snddll.AiLenChanged		 = (void (__cdecl*)( void		   )) GetProcAddress(snddll.hinstLibAudio, "AiLenChanged");
	snddll.AiReadLength		 = (DWORD (__cdecl*)( void		   )) GetProcAddress(snddll.hinstLibAudio, "AiReadLength");
	snddll.AiUpdate			 = (void (__cdecl*)( BOOL		   )) GetProcAddress(snddll.hinstLibAudio, "AiUpdate");

	return true;
}

void CloseAudioPlugin () {
  if(snddll.CloseDLL)
	  snddll.CloseDLL();

  if(snddll.hinstLibAudio)
	  FreeLibrary(snddll.hinstLibAudio); 

  ZeroMemory (&snddll, sizeof(AUDIODLL));
}

BOOL LoadAudioPlugin (char *libname) {
	PLUGIN_INFO Plugin_Info;
	ZeroMemory (&Plugin_Info, sizeof(Plugin_Info));

	if (libname == NULL) {
		return FALSE;
	}
	
	if (snddll.hinstLibAudio != NULL) 
		snddll.Close ();

	snddll.hinstLibAudio = LoadLibrary(libname);

	if (snddll.hinstLibAudio == NULL) {
		Debug (0, "Could not load %s because it couldn't be found!", libname);
		snddll.Close();
		return FALSE;
	}

	snddll.GetDllInfo = (void (__cdecl*)(PLUGIN_INFO *)) GetProcAddress(snddll.hinstLibAudio, "GetDllInfo");
	if(!snddll.GetDllInfo) {
		Debug (0, "%s does not conform to the plugin spec.", libname);
		return FALSE;//This isn't a plugin dll, so don't bother.	
	}
	snddll.GetDllInfo(&Plugin_Info);

	if(Plugin_Info.Type == PLUGIN_TYPE_AUDIO) {
		if(Plugin_Info.Version != PLUGIN_SND_VERSION) {
			Debug (0, "%s is not compatable with Apollo. %X != %X", libname, Plugin_Info.Version, PLUGIN_INP_VERSION);
			snddll.Close();
			return FALSE;
		}

		if (LoadFunctions () == false) {
			snddll.Close();
			return FALSE;
		}
	} else {
		//Here we insert Audio code, controller code, etc.
		Debug (0, "%s dll isn't an Audio plugin!", libname);
		snddll.Close();
		return FALSE;
	}
	
	return TRUE;
}

AUDIO_INFO SndInfo; // make it stay just in case the evil plugin doesn't store a local copy

extern u8* rdram;
extern u8* idmem;
extern u8* MI;
extern u8* AI;


u32 DummyReg;

void InitSNDPlugin () {

	u32 FakeReg;

	ZeroMemory (&SndInfo, sizeof(AUDIO_INFO));


	SndInfo.hwnd					= GhWnd;
	SndInfo.hinst					= GhInst;
	SndInfo.MemoryBswaped			= TRUE;

	SndInfo.HEADER					= RomMemory;
	SndInfo.RDRAM					= rdram;
	SndInfo.DMEM					= idmem;
	SndInfo.IMEM					= (idmem+0x1000);

	SndInfo.AI_DRAM_ADDR_REG	= (u32 *)(AI+0x00);
	SndInfo.AI_LEN_REG			= (u32 *)(AI+0x04);
	SndInfo.AI_CONTROL_REG		= (u32 *)(AI+0x08);
	SndInfo.AI_STATUS_REG		= (u32 *)(AI+0x0C);
	SndInfo.AI_DACRATE_REG		= (u32 *)(AI+0x10);
	SndInfo.AI_BITRATE_REG		= (u32 *)(AI+0x14);
	SndInfo.MI_INTR_REG			= (u32 *)(MI+0x08);

	SndInfo.CheckInterrupts		= SignalAiDone;

	if (snddll.InitiateAudio (SndInfo) == FALSE) {
		CloseAudioPlugin ();
		Debug (0, "Plugin Initialization Failed.");
	}
}

void ScheduleEvent (u32, u32, u32, void *);

#define AI_DMA_EVENT		0x5

void SignalAiDone(void) {
	//*SndInfo.AI_STATUS_REG |= 0x80000001;
	//ScheduleEvent (AI_DMA_EVENT, 200, 0, NULL);
	//if ((((u32*)MI)[3] & (((u32*)MI)[2])) & AI_INTERRUPT)
	//((u32*)MI)[3] |= AI_INTERRUPT;
	//((u32*)MI)[2] |= AI_INTERRUPT;
	InterruptNeeded |= AI_INTERRUPT;
	//Debug (0, "Audio Interrupt");
}

// ******************** END AUDIO STUFF **********************
/*
u32 AI_second_delay = 0;


//#define ENABLEWAVE // Enabled wave logging

#ifdef ENABLEWAVE
bool Logging = false;

FILE* audio = NULL;

bool MemoryBswaped = true;

#define FREQ		44100
#define BPS			16
#define SEGMENTS	4
#define LOCK_SIZE	(FREQ / 60) // Always 8bit mono (For the time being...)

static WAVEFORMATEX wfx;

struct WAVEFILEFORMAT {
	char riff[4];
	DWORD filesize;
	char wavefmt[8];
	DWORD fmtlength;
	WAVEFORMAT wfx;
	WORD bps;
	char data[4];
	DWORD datasize;
} wave;

void Soundmemcpy(void * dest, const void * src, size_t count);

void CloseWave () {
	wave.filesize = ftell (audio);
	fseek (audio, 0, SEEK_SET);
	fwrite (&wave, 1, sizeof(WAVEFILEFORMAT), audio);
	fclose (audio);
}

void LogWave (void *sndptr, DWORD sndlen, u32 freq) {
	unsigned char buffer[262144];
	if (audio == NULL) {
		audio = fopen("C:\\audio.wav","wb");
		if (audio == NULL) {
			Logging = false;
			return;
		}
		ZeroMemory (&wfx, sizeof(WAVEFORMATEX)); 
		wfx.wFormatTag = WAVE_FORMAT_PCM;
		wfx.nChannels = 2; 
		wfx.nSamplesPerSec = freq;//22050;     
		wfx.wBitsPerSample = 16; 
		wfx.nBlockAlign = wfx.wBitsPerSample / 8 * wfx.nChannels;
		wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
		atexit (CloseWave);
		strcpy ((char *)wave.riff,		"RIFF	 ");
		strcpy ((char *)wave.wavefmt,	"WAVEfmt ");
		strcpy ((char *)wave.data,		"data	 ");
		memcpy (&wave.wfx, &wfx, sizeof(WAVEFORMAT));
		wave.fmtlength = sizeof(WAVEFORMAT)+2;
		wave.bps = wfx.wBitsPerSample;
		wave.filesize = 0;
		wave.datasize = 0;
		fwrite (&wave, 1, sizeof(WAVEFILEFORMAT), audio);
		//MessageBox (NULL, "This should only come up once", "AUDIO", MB_OK);
		Debug (0, "Logging Audio...");
	}
	Soundmemcpy (buffer, sndptr, sndlen);
	fwrite(buffer, 1, sndlen, audio);
	wave.datasize += sndlen;
}
#endif
void SignalAI(void) {
	extern u8* AI;
	extern u32 AiInterrupt;
	extern u32 CountInterrupt, VsyncInterrupt, VsyncTime;
	u32 dacRate = 49152000/(((u32*)AI)[4]+1); //NTSC conversion
	float seconds = ((float)((u32*)AI)[1]/dacRate)/2;//div2 because of it being stereo
	if (((u32*)AI)[5]==15) seconds /=2;
	//Debug(0,"status%08X len%X rate%i freq%u seconds%2.2f, counter%X",((u32*)AI)[3],((u32*)AI)[1],((u32*)AI)[5]+1,dacRate,seconds,(u32)(seconds*625000*60));
#ifdef ENABLEWAVE
	if (Logging == true)
		LogWave (returnMemPointer(((u32*)AI)[0]), ((u32*)AI)[1], dacRate);
#endif	
	//fwrite(returnMemPointer(((u32*)AI)[0]),((u32*)AI)[1],1,audio);
	//fclose(audio);
	if (((u32*)AI)[3] & 1) {
		AI_second_delay = (u32)(seconds*625000*60);
	}
	AiInterrupt = instructions + (u32)(seconds*625000*60);
	VsyncTime = MIN(MIN(CountInterrupt,AiInterrupt),VsyncInterrupt);
}

void SignalAiDone(void) {
	extern u8* AI;
	extern u32 AiInterrupt;
	if (((u32*)AI)[3] & 1) {//second sound in queue...unset full status, set interrupt, and redo AiInterrupt
		((u32*)AI)[3] = 0x40000000;
		InterruptNeeded |= AI_INTERRUPT;
		AiInterrupt = AI_second_delay + instructions;
	} else {//only one sound in queue...unset busy
		((u32*)AI)[3] = 0;
		AiInterrupt = -1;
		((u32*)AI)[1] = 0;
		((u32*)AI)[0] = 0;
	}
}


#ifdef ENABLEWAVE
void Soundmemcpy(void * dest, const void * src, size_t count) {
	if (MemoryBswaped) {
		_asm {
			mov edi, dest
			mov ecx, src
			mov edx, 0
			push ebx
			push edi
		memcpyloop1:
			mov ax, word ptr [ecx + edx]
			mov bx, word ptr [ecx + edx + 2]
			mov  word ptr [edi + edx + 2],ax
			mov  word ptr [edi + edx],bx
			add edx, 4
			mov ax, word ptr [ecx + edx]
			mov bx, word ptr [ecx + edx + 2]
			mov  word ptr [edi + edx + 2],ax
			mov  word ptr [edi + edx],bx
			add edx, 4
			cmp edx, count
			jb memcpyloop1
			pop edi
			pop ebx
		}
	} else {
		_asm {
			mov edi, dest
			mov ecx, src
			mov edx, 0		
		memcpyloop2:
			mov ax, word ptr [ecx + edx]
			xchg ah,al
			mov  word ptr [edi + edx],ax
			add edx, 2
			mov ax, word ptr [ecx + edx]
			xchg ah,al
			mov  word ptr [edi + edx],ax
			add edx, 2
			mov ax, word ptr [ecx + edx]
			xchg ah,al
			mov  word ptr [edi + edx],ax
			add edx, 2
			mov ax, word ptr [ecx + edx]
			xchg ah,al
			mov  word ptr [edi + edx],ax
			add edx, 2
			cmp edx, count
			jb memcpyloop2
		}
	}
}
#endif
*/