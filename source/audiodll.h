#ifndef __AUDIODLL_DOT_H__
#define __AUDIODLL_DOT_H__

#include "plugin.h"

BOOL LoadAudioPlugin (char *libname);
void CloseAudioPlugin ();
void InitSNDPlugin ();

typedef struct {
	HINSTANCE hinstLibAudio; // hInstance to the Video Plugin Library
// True to the specs functions
	void (__cdecl* CloseDLL		  )( void );
	void (__cdecl* DllAbout		  )( HWND hParent );
	void (__cdecl* DllConfig	  )( HWND hParent );
	void (__cdecl* DllTest		  )( HWND hParent );
	void (__cdecl* ProcessAList	  )( void );
	void (__cdecl* RomClosed	  )( void );
	BOOL (__cdecl* InitiateAudio  )( AUDIO_INFO Audio_Info );
	void (__cdecl* GetDllInfo	  )( PLUGIN_INFO * PluginInfo );
	void (__cdecl* AiDacrateChanged)( int SystemType );
	void (__cdecl* AiLenChanged	  )( void );
	DWORD (__cdecl* AiReadLength  )( void );
	void (__cdecl* AiUpdate		  )( BOOL Wait );
	void (__cdecl* AiCallBack  )( void );

// Support Functions...
	inline BOOL Load (char* libname) {return LoadAudioPlugin(libname);} // Loads a plugin with the specified name
	inline void Init () {InitSNDPlugin ();}
	inline void Close () {CloseAudioPlugin();} // Closes a previously loaded plugin
} AUDIODLL;

extern AUDIODLL snddll;

#endif //__AUDIODLL_DOT_H__


