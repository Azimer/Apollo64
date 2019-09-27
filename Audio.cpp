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

u32 AI_second_delay = 0;


//#define ENABLEWAVE // Enabled wave logging

#ifdef ENABLEWAVE
bool Logging = true;

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