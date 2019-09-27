#ifndef __PLUGIN_DOT_H__
#define __PLUGIN_DOT_H__

/* Note: BOOL, BYTE, WORD, DWORD, TRUE, FALSE are defined in windows.h */

#define PLUGIN_TYPE_RSP				1
#define PLUGIN_TYPE_GFX				2
#define PLUGIN_TYPE_AUDIO			3
#define PLUGIN_TYPE_CONTROLLER		4

#define SYSTEM_NTSC					0
#define SYSTEM_PAL					1
#define SYSTEM_MPAL					2

#define PLUGIN_GFX_VERSION			0x0103
#define PLUGIN_INP_VERSION			0x0100
#define PLUGIN_SND_VERSION			0x0101


/***** Structures *****/
typedef struct {
	WORD Version;        /* Should be set to 1 */
	WORD Type;           /* Set to PLUGIN_TYPE_GFX */
	char Name[100];      /* Name of the DLL */

	/* If DLL supports memory these memory options then set them to TRUE or FALSE
	   if it does not support it */
	BOOL NormalMemory;    /* a normal BYTE array */ 
	BOOL MemoryBswaped;  /* a normal BYTE array where the memory has been pre
	                          bswap on a dword (32 bits) boundry */
} PLUGIN_INFO;

typedef struct {
	HWND hWnd;			/* Render window */
	HWND hStatusBar;    /* if render window does not have a status bar then this is NULL */

	BOOL MemoryBswaped;    // If this is set to TRUE, then the memory has been pre
	                       //   bswap on a dword (32 bits) boundry 
						   //	eg. the first 8 bytes are stored like this:
	                       //        4 3 2 1   8 7 6 5

	BYTE * HEADER;	// This is the rom header (first 40h bytes of the rom
					// This will be in the same memory format as the rest of the memory.
	BYTE * RDRAM;
	BYTE * DMEM;
	BYTE * IMEM;

	DWORD * MI_INTR_REG;

	DWORD * DPC_START_REG;
	DWORD * DPC_END_REG;
	DWORD * DPC_CURRENT_REG;
	DWORD * DPC_STATUS_REG;
	DWORD * DPC_CLOCK_REG;
	DWORD * DPC_BUFBUSY_REG;
	DWORD * DPC_PIPEBUSY_REG;
	DWORD * DPC_TMEM_REG;

	DWORD * VI_STATUS_REG;
	DWORD * VI_ORIGIN_REG;
	DWORD * VI_WIDTH_REG;
	DWORD * VI_INTR_REG;
	DWORD * VI_V_CURRENT_LINE_REG;
	DWORD * VI_TIMING_REG;
	DWORD * VI_V_SYNC_REG;
	DWORD * VI_H_SYNC_REG;
	DWORD * VI_LEAP_REG;
	DWORD * VI_H_START_REG;
	DWORD * VI_V_START_REG;
	DWORD * VI_V_BURST_REG;
	DWORD * VI_X_SCALE_REG;
	DWORD * VI_Y_SCALE_REG;

	void (*CheckInterrupts)( void );
} GFX_INFO;

/***** Structures *****/
typedef struct {
	BOOL Present;
	BOOL RawData;
	int  Plugin;
} CONTROL;

typedef union {
	DWORD Value;
	struct {
		unsigned R_DPAD       : 1;
		unsigned L_DPAD       : 1;
		unsigned D_DPAD       : 1;
		unsigned U_DPAD       : 1;
		unsigned START_BUTTON : 1;
		unsigned Z_TRIG       : 1;
		unsigned B_BUTTON     : 1;
		unsigned A_BUTTON     : 1;

		unsigned R_CBUTTON    : 1;
		unsigned L_CBUTTON    : 1;
		unsigned D_CBUTTON    : 1;
		unsigned U_CBUTTON    : 1;
		unsigned R_TRIG       : 1;
		unsigned L_TRIG       : 1;
		unsigned Reserved1    : 1;
		unsigned Reserved2    : 1;

		signed   Y_AXIS       : 8;

		signed   X_AXIS       : 8;
	};
} BUTTONS;

typedef struct {
	HWND hwnd;
	HINSTANCE hinst;

	BOOL MemoryBswaped;    // If this is set to TRUE, then the memory has been pre
	                       //   bswap on a dword (32 bits) boundry 
						   //	eg. the first 8 bytes are stored like this:
	                       //        4 3 2 1   8 7 6 5
	BYTE * HEADER;	// This is the rom header (first 40h bytes of the rom
					// This will be in the same memory format as the rest of the memory.
	BYTE * RDRAM;
	BYTE * DMEM;
	BYTE * IMEM;

	DWORD * MI_INTR_REG;

	DWORD * AI_DRAM_ADDR_REG;
	DWORD * AI_LEN_REG;
	DWORD * AI_CONTROL_REG;
	DWORD * AI_STATUS_REG;
	DWORD * AI_DACRATE_REG;
	DWORD * AI_BITRATE_REG;

	void (*CheckInterrupts)( void );
} AUDIO_INFO;

void LoadPlugins ();
void UnLoadPlugins ();

#endif // __PLUGIN_DOT_H__