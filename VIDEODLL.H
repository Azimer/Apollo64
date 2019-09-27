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

#ifndef __VIDEODLL_DOT_H__
#define __VIDEODLL_DOT_H__

#include "plugin.h"

BOOL LoadVideoPlugin (char *libname);
void CloseVideoPlugin ();
void InitGFXPlugin ();

typedef struct {
	HINSTANCE hinstLibVideo; // hInstance to the Video Plugin Library
// True to the specs functions
	void (__cdecl* CloseDLL		  )( void );
	void (__cdecl* ChangeWindow	  )( BOOL fullscreen );
	void (__cdecl* DllAbout		  )( HWND hParent );
	void (__cdecl* DllConfig	  )( HWND hParent );
	void (__cdecl* DllTest		  )( HWND hParent );
	void (__cdecl* DrawScreen	  )( void );
	void (__cdecl* GetDllInfo	  )( PLUGIN_INFO * PluginInfo );
	BOOL (__cdecl* InitiateGFX	  )( GFX_INFO Gfx_Info );
	void (__cdecl* MoveScreen	  )( int xpos, int ypos );
	void (__cdecl* ProcessDList	  )( void );
	void (__cdecl* ProcessRDPList )( void );
	void (__cdecl* RomClosed	  )( void );
	void (__cdecl* RomOpen		  )( void );
	void (__cdecl* UpdateScreen	  )( void );
	void (__cdecl* ViStatusChanged)( void );
	void (__cdecl* ViWidthChanged )( void );
// Support Functions...
	inline BOOL Load (char* libname) {return LoadVideoPlugin(libname);} // Loads a plugin with the specified name
	inline void Init () {InitGFXPlugin ();}
	inline void Close () {CloseVideoPlugin();} // Closes a previously loaded plugin
} VIDEODLL;

extern VIDEODLL gfxdll;

#endif //__VIDEODLL_DOT_H__


