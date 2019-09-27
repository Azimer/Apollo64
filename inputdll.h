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
