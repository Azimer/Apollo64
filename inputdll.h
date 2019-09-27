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

#ifndef __INPUTDLL_DOT_H__
#define __INPUTDLL_DOT_H__

#include "plugin.h"

BOOL LoadInputPlugin (char *libname);
void CloseInputPlugin ();
void InitINPPlugin ();

typedef struct {
	HINSTANCE hinstLibInput; // hInstance to the INPUT Plugin Library
// True to the specs functions
	void (__cdecl* CloseDLL				)( void );
	void (__cdecl* ControllerCommand	)( int Control, BYTE * Command );
	void (__cdecl* DllAbout				)( HWND hParent );
	void (__cdecl* DllConfig			)( HWND hParent );
	void (__cdecl* DllTest				)( HWND hParent );
	void (__cdecl* GetDllInfo			)( PLUGIN_INFO * PluginInfo );
	void (__cdecl* GetKeys				)( int Control, BUTTONS * Keys );
	void (__cdecl* InitiateControllers	)( HWND hMainWindow, CONTROL Controls[4] );
	void (__cdecl* ReadController		)( int Control, BYTE * Command );
	void (__cdecl* RomClosed			)( void	);
	void (__cdecl* RomOpen				)( void	);
	void (__cdecl* WM_KeyDown			)( WPARAM wParam, LPARAM lParam );
	void (__cdecl* WM_KeyUp				)( WPARAM wParam, LPARAM lParam );

// Support Functions...
	inline BOOL Load (char* libname) {return LoadInputPlugin(libname);} // Loads a plugin with the specified name
	inline void Close () {CloseInputPlugin();} // Closes a previously loaded plugin
} INPUTDLL;

extern INPUTDLL inpdll;
extern CONTROL ContInfo[4];

#endif //__INPUTDLL_DOT_H__
