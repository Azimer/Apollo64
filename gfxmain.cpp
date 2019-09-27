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

#include <windows.h>
#include <ddraw.h>
#include <commctrl.h> // For status window
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "WinMain.h"
#include "EmuMain.h"
#include "videodll.h"
#include "resource.h"

void internalVidProcessDList(void);
void internalVidMoveScreen(int a, int b);
void internalVidDllAbout(HWND hWnd);
void internalVidDllClose(void);
void internalVidChangeWindow(BOOL fullscreen);
void internalVidDllConfig(HWND hWnd);
void internalVidDllTest(HWND hWnd);
void internalVidDrawScreen(void);
void internalVidUpdateScreen(void);
BOOL internalVidInitiateGFX(GFX_INFO);
void internalVidGetDllInfo(PLUGIN_INFO* a);
void internalVidRomChange(void);
void DummyVoidVoid(void);

int bpp;

GUID					Driver;

LPDIRECTDRAW7			DDWin2 = NULL;
LPDIRECTDRAWCLIPPER		DDClip = NULL;

LPDIRECTDRAWSURFACE7	cfbBuffer = NULL; 
LPDIRECTDRAWSURFACE7	PrimarySurface = NULL;
int						e_Width = 320, e_Height = 240;

int yDest, xDest;

void ComputeFPS(void);
int InitResolution(int X, int Y);

VIDEODLL internalGfxDll;

/**************************************************************************************/
//CFB section of the internal video
/**************************************************************************************/

void InitInternalVideo(void) {
	gfxdll.hinstLibVideo	=	NULL;//GhInst;
	gfxdll.CloseDLL			=	internalVidDllClose;
	gfxdll.ChangeWindow		=	internalVidChangeWindow;
	gfxdll.DllAbout			=	internalVidDllAbout;
	gfxdll.DllConfig		=	internalVidDllConfig;
	gfxdll.DllTest			=	internalVidDllTest;
	gfxdll.DrawScreen		=	internalVidDrawScreen;
	gfxdll.GetDllInfo		=	internalVidGetDllInfo;
	gfxdll.InitiateGFX		=	internalVidInitiateGFX;
	gfxdll.MoveScreen		=	internalVidMoveScreen;
	gfxdll.ProcessDList		=	DummyVoidVoid;
	gfxdll.ProcessRDPList	=	DummyVoidVoid; // -=Suppose=- to be gone with v1.1 of the dll specs...
	gfxdll.RomOpen			=	internalVidRomChange;
	gfxdll.RomClosed		=	internalVidRomChange;
	gfxdll.UpdateScreen		=	internalVidUpdateScreen;
	gfxdll.ViStatusChanged	=	DummyVoidVoid;
	gfxdll.ViWidthChanged	=	DummyVoidVoid;
	memcpy (&internalGfxDll, &gfxdll, sizeof(VIDEODLL));
}

void internalVidMoveScreen(int a, int b) {
	GetClientRect(GhWnd, &rcWindow);
	ClientToScreen(GhWnd, (LPPOINT)&rcWindow);
	ClientToScreen(GhWnd, (LPPOINT)&rcWindow+1);
	rcWindow.bottom -= 20;
}

void internalVidDllAbout(HWND hWnd) {
	MessageBox(hWnd,"Internal DLL. Made by Eclipse Productions","The Apollo Project",MB_OK);
}

void internalVidDllConfig(HWND hWnd) {
	MessageBox(hWnd,"No configuration necessary.","The Apollo Project",MB_OK);
}

void internalVidDllTest(HWND hWnd) {
	MessageBox(hWnd,"Test Passed!","The Apollo Project",MB_OK);
}

void internalVidDllClose(void) {
	if (DDWin2) {
		DDWin2->RestoreDisplayMode();
		DDWin2->SetCooperativeLevel(GhWnd, DDSCL_NORMAL);
	}

	if (cfbBuffer) cfbBuffer->Release();

	if (PrimarySurface) PrimarySurface->Release();

	if (DDWin2) DDWin2->Release();

	DDWin2 = NULL; PrimarySurface = NULL; cfbBuffer = NULL;
}

void internalVidChangeWindow(BOOL fullscreen) {
	//TODO isFullscreen = fullscreen;
}

void internalVidRomChange(void) {
	DDBLTFX ddbltfx;
	ddbltfx.dwSize = sizeof(DDBLTFX);
	ddbltfx.dwFillColor = 0;
	
	if (cfbBuffer) cfbBuffer->Blt(&rcWindow, NULL, NULL, DDBLT_COLORFILL|DDBLT_WAIT, &ddbltfx);
	if (PrimarySurface) PrimarySurface->Blt(&rcWindow, NULL, NULL, DDBLT_COLORFILL|DDBLT_WAIT, &ddbltfx);
}

void internalVidGetDllInfo(PLUGIN_INFO* a) {
	strcpy(a[0].Name,"Apollo Internal Video Functions");
	a[0].MemoryBswaped = TRUE;
	a[0].NormalMemory = FALSE;
	a[0].Type = PLUGIN_TYPE_GFX;
	a[0].Version = 0x0102;
}

BOOL internalVidInitiateGFX(GFX_INFO a) {
	//since this fake DLL is internal to Apollo, I cheat and use vars unknown to the spec.
	if (InitResolution (320, 240) != 0) {
		Debug (0, L_STR(IDS_DX_DEAD));
		return FALSE;
	}
	return TRUE;
}

void internalVidDrawScreen(void) {
	RECT dest = {0, 0, xDest, yDest };
	if (PrimarySurface)	PrimarySurface->Blt(&rcWindow, cfbBuffer, &dest, DDBLT_WAIT, NULL);
}

void internalVidUpdateScreen(void) {
	extern u8* VI;


	int width = ((u32*)VI)[2];
	
	//ComputeFPS();


	if (width >= 320) {
		DDSURFACEDESC2 ddsd;
		RECT dest;
		inline void* returnMemPointer(u32 addr);
		
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		
		if (DD_OK == cfbBuffer->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR|DDLOCK_WAIT,NULL)) {
			
			unsigned char* PcAddr = (u8*)returnMemPointer(((u32*)VI)[1]);
			LPVOID lpIncBuffer = ddsd.lpSurface;
			int lPitch = ddsd.lPitch;
			xDest = width;
			yDest = (width*3)/4;
			
			if (bpp == 16) {
				u8 and1[8] = {0x1F, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x1F, 0x00};
				u8 and2[8] = {0xC0, 0xFF, 0xC0, 0xFF, 0xC0, 0xFF, 0xC0, 0xFF};
				__asm {
					mov eax, dword ptr [lpIncBuffer]	; byte pointer (PC)
					mov edx, dword ptr [PcAddr]			; byte pointer (n64)
					mov ebx, dword ptr [lPitch]			; pitch dumbass
					mov edi, dword ptr [yDest]			; y counter

					movq mm7, qword ptr [and1]	; bottom
					movq mm6, qword ptr [and2]	; top/middle

					mov esi, 16
					movd mm4, esi			; optimization

					push ebp
					mov ebp, 8

_MainLoop16_:

					mov esi, dword ptr [xDest]	; x counter
					mov ecx, eax			; reset video latch
					shr esi, 2	

_InternalX16_:
					movq mm0, qword ptr [edx]	; pcaddr pointer

					; byte swap words
					movq mm1, mm0	; copy mm0
					psrld mm0, mm4	; shift all bytes right 
					pslld mm1, mm4	; shift all bytes left
					por mm0, mm1	; or them together again

					; -----------------------------
					; convert mm0 (4 pixels) to 565
					
					movq mm1, mm0	; lower 5 bits copy
					psrlw mm1, 1	; shift out alpha

					pand mm0, mm6	; top bits $FFC0
					pand mm1, mm7	; and $1f
					por mm0, mm1	; finish up or(s)

					; ----------------
					; blit this fucker

					movq qword ptr [ecx], mm0

					add edx, ebp	; we just did 4*2 (n64)
					add ecx, ebp	; we just did 4*2 (video)
					dec esi
					jnz short _InternalX16_

					add eax, ebx		; add to video pitch
					dec edi
					jnz short _MainLoop16_

					emms
					pop ebp
				}
			} else if (bpp==15) {
				__asm {
					mov eax, dword ptr lpIncBuffer		; byte pointer (PC)
					mov edx, dword ptr PcAddr			; byte pointer (n64)
					mov ebx, dword ptr lPitch			; pitch dumbass
					mov edi, dword ptr yDest			; y counter
					
					push ebp
					mov ebp, 8
_MainLoop15_:
					mov esi, dword ptr xDest	; x counter
					mov ecx, eax			; reset video latch
					shr esi, 2		
					
_InternalX15_:
					movq mm0, qword ptr [edx]	; pcaddr pointer
					
					; byte swap words
					movq mm1, mm0	; copy mm1
					psrld mm0, 16	; shift all bytes right 
					pslld mm1, 16	; shift all bytes left
					por mm0, mm1	; or them together again
					
					; -----------------------------
					; convert mm0 (4 pixels) to 555
					
					psrlw mm0, 1	; shift out alpha
					
					; ----------------
					; blit this fucker

					movq qword ptr [ecx], mm0
					
					add ecx, ebp	; we just did 4*2 (video)
					add edx, ebp	; we just did 4*2 (n64)
					dec esi
					
					jnz short _InternalX15_
					
					add eax, ebx		; add to video pitch
					dec edi
					jnz short _MainLoop15_
					
					emms
					pop ebp
				}
			} else if (bpp==32) {
				u8 and1[8] = {0xF8, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x00};
				u8 and2[8] = {0x00, 0xF8, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00};
				u8 and3[8] = {0x00, 0x00, 0xF8, 0x00, 0x00, 0x00, 0xF8, 0x00};
				__asm {
					mov eax, dword ptr [lpIncBuffer]	; byte pointer (PC)
					mov edx, dword ptr [PcAddr]			; byte pointer (n64)
					mov ebx, dword ptr [lPitch]			; pitch dumbass
					mov edi, dword ptr [yDest]			; y counter

					movq mm7, qword ptr [and1]	; blue
					movq mm6, qword ptr [and2]	; green
					movq mm5, qword ptr [and3]	; red

					mov esi, 32
					movd mm4, esi			; optimization

					push ebp
					mov ebp, 4

_MainLoop24_:

					mov esi, dword ptr [xDest]	; x counter
					mov ecx, eax			; reset video latch
					shr esi, 1	

_InternalX24_:
					movq mm0, qword ptr [edx]	; pcaddr pointer

					; byte swap words
					movq mm1, mm0	; copy mm0
					psllq mm0, mm4	; shift all bytes right 
					psrlq mm0, 48	; shift all bytes right 
					psllq mm1, mm4	; shift all bytes left
					por mm0, mm1	; or them together again

					; -----------------------------
					; convert mm0 (2 pixels) to 888
					
					movq mm1, mm0	; mm1 is green
					movq mm2, mm0	; mm2 is red
					pslld mm0, 2	; blue is done
					pslld mm1, 5	; green is done
					pslld mm2, 8	; red is done

					pand mm0, mm7	; keep just blue
					pand mm1, mm6	; keep just green
					pand mm2, mm5	; keep just red
					por mm0, mm1	; finish up or(s)
					por mm0, mm2	; finish up or(s)

					; ----------------
					; blit this fucker

					movq qword ptr [ecx], mm0

					add edx, 4	; we just did 2 pixels (n64)
					add ecx, 6	; we just did 2 pixels (video)
					dec esi
					jnz short _InternalX24_

					add eax, ebx		; add to video pitch
					dec edi
					jnz short _MainLoop24_

					emms
					pop ebp
				}

			} else if (bpp==32) {
				u8 and1[8] = {0xF8, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x00};
				u8 and2[8] = {0x00, 0xF8, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00};
				u8 and3[8] = {0x00, 0x00, 0xF8, 0x00, 0x00, 0x00, 0xF8, 0x00};
				__asm {
					mov eax, dword ptr [lpIncBuffer]	; byte pointer (PC)
					mov edx, dword ptr [PcAddr]			; byte pointer (n64)
					mov ebx, dword ptr [lPitch]			; pitch dumbass
					mov edi, dword ptr [yDest]			; y counter

					movq mm7, qword ptr [and1]	; blue
					movq mm6, qword ptr [and2]	; green
					movq mm5, qword ptr [and3]	; red

					mov esi, 32
					movd mm4, esi			; optimization

					push ebp
					mov ebp, 4

_MainLoop32_:

					mov esi, dword ptr [xDest]	; x counter
					mov ecx, eax			; reset video latch
					shr esi, 1	

_InternalX32_:
					movq mm0, qword ptr [edx]	; pcaddr pointer

					; byte swap words
					movq mm1, mm0	; copy mm0
					psllq mm0, mm4	; shift all bytes right 
					psrlq mm0, 48	; shift all bytes right 
					psllq mm1, mm4	; shift all bytes left
					por mm0, mm1	; or them together again

					; -----------------------------
					; convert mm0 (2 pixels) to 888
					
					movq mm1, mm0	; mm1 is green
					movq mm2, mm0	; mm2 is red
					pslld mm0, 2	; blue is done
					pslld mm1, 5	; green is done
					pslld mm2, 8	; red is done

					pand mm0, mm7	; keep just blue
					pand mm1, mm6	; keep just green
					pand mm2, mm5	; keep just red
					por mm0, mm1	; finish up or(s)
					por mm0, mm2	; finish up or(s)

					; ----------------
					; blit this fucker

					movq qword ptr [ecx], mm0

					add edx, 4	; we just did 2 pixels (n64)
					add ecx, 8	; we just did 2 pixels (video)
					dec esi
					jnz short _InternalX32_

					add eax, ebx		; add to video pitch
					dec edi
					jnz short _MainLoop32_

					emms
					pop ebp
				}
			}
			
			cfbBuffer->Unlock(NULL);
			
			dest.top = dest.left = 0;
			dest.right = xDest;
			dest.bottom = yDest;
			
			PrimarySurface->Blt(&rcWindow, cfbBuffer, &dest, DDBLT_WAIT, NULL);
		}
	}
}

/****************************************************************************************/
//RDP section of internal video
/****************************************************************************************/

u32 cmd0,cmd1;
u32 rdpAddr;

extern bool (*rdp[0x100])();


void internalVidProcessDList(void) {
	return;//not yet
//	rdpAddr = pr32(0x04000FF0);//HLE worthy
//	while (1) {
//		cmd0 = pr32(rdpAddr);
//		cmd1 = pr32(rdpAddr+4);
//		rdpAddr+=8;
//		if (!rdp[cmd0>>24]()) return;//any fail causes a return.
//	}
}

bool rdpDummy(void) {
	return true;//always pass.
}
/*
bool (*rdp[0x100])() = {
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, 
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, 
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, 
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, 
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, 
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, 
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, 
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, 
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, 
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, 
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, 
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, 
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, 
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, 
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, 
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, 
	rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy, rdpDummy
}
*/

void DummyVoidVoid () {
}

/*****************************************************************************************/
//Start of DirectX.cpp
/*****************************************************************************************/

int last_time = 0, frames = 0;

void ComputeFPS(void) {
	if (isFullScreen == false) {
		char buffer[16];
		double temp1, temp2;
		frames++;
		int current_time = timeGetTime();

		if (current_time - last_time >= 1000) {
			temp1 = frames;
			temp2 = current_time - last_time - 1000;
			temp2 /= 1000.f;
			if (temp2 < 1) temp1 -= temp1 * temp2;
		
			sprintf(buffer,"%2.2f FPS", temp1);
			SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM)(LPSTR) buffer);

			last_time = timeGetTime();
			frames = 0;
		}
	}
}

/* the purpose here is to handle resolution changes only this also
   can involve recreating the status bar */

int InitResolution(int X, int Y) {
	e_Width = X;
	e_Height = Y;

	int InitDD();
	return (InitDD());
}

//Needs to be rewritten....  Sorry
int InitSurfaces(int X, int Y) {
	DDSURFACEDESC2 ddsd;
	DDBLTFX ddbltfx;
	int width[2];
	RECT client;

	if (hwndStatus) DestroyWindow(hwndStatus);
	hwndStatus = NULL;

	/* resize the window now, it will take care
	   of all the bullshit that needs to be done */

	client.left = client.top = 0;
	client.right = X;
	client.bottom = Y;

	DDWin2->RestoreDisplayMode();
	DDWin2->SetCooperativeLevel(GhWnd, DDSCL_NORMAL);
	SetClientRect(GhWnd, client);

	width[0] = (int)(X / 1.3f);
	width[1] = -1;
	hwndStatus = CreateStatusWindow(WS_VISIBLE|WS_CHILD, L_STR(IDS_READY), GhWnd, IDC_STATUS);
	SendMessage(hwndStatus, SB_SETPARTS, (WPARAM) 2, (LPARAM) (LPINT) width);

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	if FAILED(DDWin2->CreateSurface(&ddsd, &PrimarySurface, NULL))
		return 1;
	
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

	ddsd.dwWidth = 640;
	ddsd.dwHeight = 480;

	if (DDWin2->CreateSurface(&ddsd, &cfbBuffer, NULL) != DD_OK ) 
		return 2;

	if FAILED(DDWin2->CreateClipper(0, &DDClip, NULL)) return 4;
	if FAILED(DDClip->SetHWnd(0, GhWnd)) return 5;
	if FAILED(PrimarySurface->SetClipper(DDClip)) return 6;
	DDClip->Release();
	DDClip = NULL;

	ddbltfx.dwSize = sizeof(DDBLTFX);
	ddbltfx.dwFillColor = 0;
	
	PrimarySurface->Blt(&rcWindow, NULL, NULL, DDBLT_COLORFILL|DDBLT_WAIT, &ddbltfx);

	return 0;
}

/* this is the main direct draw procedure it handles driver changes
   and initialization of Direct Draw */

int InitDD()
{
	DDPIXELFORMAT format;

	memset(&format,0, sizeof(format));
	format.dwSize = sizeof(format);

	if (DDWin2) DDWin2->SetCooperativeLevel(GhWnd, DDSCL_NORMAL);

	internalVidDllClose();

	DirectDrawCreateEx(&Driver, (VOID**)&DDWin2, IID_IDirectDraw7, NULL);

	/* if we are screwed return */
	if (!DDWin2) return 1;
	DDWin2->SetCooperativeLevel(GhWnd, DDSCL_NORMAL);

	/* creates all surfaces */
	if (InitSurfaces(e_Width, e_Height)) return 2;
	
	if (cfbBuffer->GetPixelFormat(&format) == DD_OK) {
		bpp = format.dwRGBBitCount;
		if (format.dwGBitMask == 0x03E0) bpp--;

		if (bpp != 16 && bpp != 15 && bpp != 24 && bpp != 32) { MessageBox(GhWnd,L_STR(IDS_COLOR_INVALID), WINTITLE, MB_OK); return 2; }

	} else return 5;

	return 0;
}
