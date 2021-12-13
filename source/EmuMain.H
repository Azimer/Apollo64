// Everything below this is from the old CPU/Memory
// TODO: Figure out what NEEDS to be here after the dynarec is installed!
// TODO: Reorganize this once I open the source to phro

#ifndef MIN
#define MIN(a, b)				((a) < (b) ? (a) : (b))
#endif

extern char InterruptNeeded;

//in cpu.cpp
extern QWORD CpuRegs[32];
extern DWORD MmuRegs[32];
//extern DWORD FpuRegs[32];
extern DWORD FpuControl[32];
extern void *FPRDoubleLocation[32];
extern void *FPRFloatLocation[32];
//extern QWORD cpuHi;
//extern QWORD cpuLo;
extern SQWORD cpuHi;
extern SQWORD cpuLo;
extern DWORD pc;
extern DWORD instructions;
void Emulate( void );
void CheckInterrupts(void);
void InitMem( int );
extern BYTE*  idmem;
extern DWORD GameSize;
extern BYTE* RomMemory;

typedef union {
	s32		w[32];
	float	s[32];
	double	d[32];
	s64		l[32];
	s32		W[32][2];
	s64		DW[32];
} R4K_FPU_union;
extern R4K_FPU_union FpuRegs;

typedef struct {
		unsigned int	func:6;
		unsigned int	sa:5;		
		unsigned int	rd:5;
		unsigned int	rt:5;		
		unsigned int	rs:5;		
		unsigned int	op:6;
} s_sop;

extern s_sop sop;

typedef struct {
	DWORD hh,hl;
	DWORD lh,ll;
	BYTE isGlobal;
} s_tlb;

#define SP_INTERRUPT	0x1
#define SI_INTERRUPT	0x2
#define AI_INTERRUPT	0x4
#define VI_INTERRUPT	0x8
#define PI_INTERRUPT	0x10
#define DP_INTERRUPT	0x20
#define COUNT_INTERRUPT 0x80//not really anything in the mips regs.

#define vReadChar	vr8
#define vReadShort	vr16
#define vReadLong	vr32
#define vReadQuad	vr64

#define vStoreChar	vw8
#define vStoreShort vw16
#define vStoreLong	vw32
#define vStoreQuad	vw64

#define ReadChar	pr8
#define ReadShort	pr16
#define ReadLong	pr32
#define ReadQuad	pr64

#define StoreChar	pw8
#define StoreShort	pw16
#define StoreLong	pw32
#define StoreQuad	pw64

void MemoryAllocRom(u32 filesize);
void KillMemory(void);
extern u32 valloc;


u32 VirtualToPhysical(u32 addr, int type);
u32 CookData(u32 addr, u32 data);
u32 ReadData(u32 addr);
//Memory Module Interface

//11 is swapped mem, 8 is unswapped mem
#define SET_INTR(x) MI[8] |= x;
#define CLEAR_INTR(x) MI[8] &= ~(x);

inline void* returnMemPointer(u32 addr) { extern u32 valloc; return (void*)(valloc+(addr&0x1fffffff)); }

inline u8  pr8 (u32 location) { return (( u8*)returnMemPointer(location^3))[0]; }
inline u16 pr16(u32 location) { return ((u16*)returnMemPointer(location^2))[0]; }
inline u32 pr32(u32 location) { return ((u32*)returnMemPointer(location))[0]; }
inline u64 pr64(u32 location) { return ((u32*)returnMemPointer(location))[1] | ((QWORD)((u32*)returnMemPointer(location))[0] << 32); }

inline u8  vr8 (u32 location) { if ((location ^ 0x80000000) & 0xC0000000) location = VirtualToPhysical(location, 0); return ((u8*)returnMemPointer(location^3))[0]; }
inline u16 vr16(u32 location) { if ((location ^ 0x80000000) & 0xC0000000) location = VirtualToPhysical(location, 0); return ((u16*)returnMemPointer(location^2))[0]; }
//inline u32 vr32(u32 location) { if ((location ^ 0x80000000) & 0xC0000000) location = VirtualToPhysical(location, 0); return ((u32*)returnMemPointer(location))[0];}
inline u32 vr32(u32 location) { if ((location ^ 0x80000000) & 0xC0000000) location = VirtualToPhysical(location, 0); if (location & 0x04000000) return ReadData(location); else return ((u32*)returnMemPointer(location))[0];}
inline u64 vr64(u32 location) { if ((location ^ 0x80000000) & 0xC0000000) location = VirtualToPhysical(location, 0); return ((u32*)returnMemPointer(location))[1] | ((QWORD)((u32*)returnMemPointer(location))[0] << 32);}
//*/
/*
inline u8  vr8 (u32 location) { return ((u8*)returnMemPointer(location^3))[0]; }
inline u16 vr16(u32 location) { return ((u16*)returnMemPointer(location^2))[0]; }
inline u32 vr32(u32 location) { return ((u32*)returnMemPointer(location))[0];}
//inline u32 vr32(u32 location) { if ((location ^ 0x80000000) & 0xC0000000) location = VirtualToPhysical(location, 0); if (location & 0x04000000) return ReadData(location); else return ((u32*)returnMemPointer(location))[0];}
inline u64 vr64(u32 location) { return ((u32*)returnMemPointer(location))[1] | ((QWORD)((u32*)returnMemPointer(location))[0] << 32);}
*/
inline void pw8 (u32 location, u8  data) { ((u8*)returnMemPointer(location^3))[0] = data; }
inline void pw16(u32 location, u16 data) { ((u16*)returnMemPointer(location^2))[0] = data; }
inline void pw32(u32 location, u32 data) { if (location & 0x0C000000) data = CookData(location,data); ((u32*)returnMemPointer(location))[0] = data; }
inline void pw64(u32 location, u64 data) { ((u32*)returnMemPointer(location))[1] = (u32)data; ((u32*)returnMemPointer(location))[0] = (u32)(data >> 32); }

inline void vw8 (u32 location, u8  data) { if ((location ^ 0x80000000) & 0xC0000000) location = VirtualToPhysical(location, 1); ((u8*)returnMemPointer(location^3))[0] = data; }
inline void vw16(u32 location, u16 data) { if ((location ^ 0x80000000) & 0xC0000000) location = VirtualToPhysical(location, 1); ((u16*)returnMemPointer(location^2))[0] = data; }
inline void vw32(u32 location, u32 data) { if ((location ^ 0x80000000) & 0xC0000000) location = VirtualToPhysical(location, 1); if (location & 0x0C000000) data = CookData(location,data); ((u32*)returnMemPointer(location))[0] = data; }
inline void vw64(u32 location, u64 data) { if ((location ^ 0x80000000) & 0xC0000000) location = VirtualToPhysical(location, 1); ((u32*)returnMemPointer(location))[1] = (u32)data; ((u32*)returnMemPointer(location))[0] = (u32)(data >> 32); }
//*/
/*
inline void vw8 (u32 location, u8  data) { ((u8*)returnMemPointer(location^3))[0] = data; }
inline void vw16(u32 location, u16 data) { ((u16*)returnMemPointer(location^2))[0] = data; }
inline void vw32(u32 location, u32 data) { if (location & 0x0C000000) data = CookData(location,data); ((u32*)returnMemPointer(location))[0] = data; }
inline void vw64(u32 location, u64 data) { ((u32*)returnMemPointer(location))[1] = (u32)data; ((u32*)returnMemPointer(location))[0] = (u32)(data >> 32); }
*/
// End of eclipse code

// CPU Externs
#define opcode (((u32*)&sop)[0])
extern bool volatile cpuIsDone;
extern bool volatile cpuIsReset;
extern bool volatile cpuSaveState;
extern bool volatile cpuLoadState;
extern bool cpuIsPaused;
void OpenROM (char *filename);
void StopCPU (void);
void ToggleCPU (void);
void InitEmu ();

//from rcp.cpp
void SignalRSP(DWORD *data);
void SignalRDP(void);

//from R4K Debugger.cpp
extern HWND DebuggerHwnd;
BOOL CALLBACK Debugger( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
bool InitializeDebugger(void);

//from Audio.cpp
void SignalAI(void);
void SignalAiDone(void);