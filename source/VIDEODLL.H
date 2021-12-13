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
	void (__cdecl* ChangeWindow	  )( void );
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
	void (__cdecl* CaptureScreen  )( char * Directory );
// Support Functions...
	inline BOOL Load (char* libname) {return LoadVideoPlugin(libname);} // Loads a plugin with the specified name
	inline void Init () {InitGFXPlugin ();}
	inline void Close () {CloseVideoPlugin();} // Closes a previously loaded plugin
} VIDEODLL;

extern VIDEODLL gfxdll;

#endif //__VIDEODLL_DOT_H__


