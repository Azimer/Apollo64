#include <windows.h>
#include "winmain.h"
#include "emumain.h"

extern u8* rdram;
extern u8* ramRegs0;
extern u8* ramRegs4;
extern u8* ramRegs8;
extern u8* idmem;
extern u8* SP;
extern u8* SP2;
extern u8* DPC;
extern u8* DPS;
extern u8* MI;
extern u8* VI;
extern u8* AI;
extern u8* PI;
extern u8* RI;
extern u8* SI;
extern u8* pif;

extern s_tlb tlb[32];

#define MAXEVENTS 10

extern u32 PiInterrupt, CountInterrupt, VsyncInterrupt;
extern u32 VsyncTime;


typedef struct {
	u32 Time;
	u32 EventType;
	u32 extra[8];
} EventList;

extern EventList EList [MAXEVENTS];
extern u32 ListCnt;

void ScheduleNextEvent ();

u32 storeptr = 0;
u8 *InterpSave, *DynaSave;


void ResetStore () {
	storeptr = 0;
}

void Store (void *src, int cnt, int size, u8 *buff) {
	memcpy (buff+storeptr, src, cnt*size);
	storeptr += (cnt*size);
}

void Restore (void *dst, int cnt, int size, u8 *buff) {
	memcpy (dst, buff+storeptr, cnt*size);
	storeptr += (cnt*size);
}

void StoreEvents (u8 *c) {
	Store (&PiInterrupt, 1, sizeof(u32), c);
	Store (EList, MAXEVENTS, sizeof(EventList), c);
}

void RestoreEvents (u8 *c) {
	Restore (&PiInterrupt, 1, sizeof(u32), c);
	Restore (EList, MAXEVENTS, sizeof(EventList), c);
	ListCnt = 0;
	for (int x = 0; x < MAXEVENTS; x++) {
		if (EList[x].EventType != 0)
			ListCnt++;
	}
	ScheduleNextEvent ();
}

void SaveCore (int core) {
	u8 *c;
	int x;
	ResetStore ();
	if (core) {
		c = DynaSave;
	} else {
		c = InterpSave;
	}

		for (x = 0; x < 8*1024*1024; x += 1024) {
			Store (&rdram[x], 1, 1024, c);
		}
		Store (&idmem[0],    1, 0x2000, c);
		Store (&SP[0],       1, 0x20  , c);
		Store (&SP2[0],      1, 0x08  , c);
		Store (&DPC[0],      1, 0x10  , c);
		Store (&DPS[0],      1, 0x10  , c);
		Store (&MI[0],       1, 0x10  , c);
		Store (&VI[0],       1, 0x38  , c);
		Store (&AI[0],       1, 0x18  , c);
		Store (&PI[0],       1, 0x34  , c);
		Store (&RI[0],       1, 0x20  , c);
		Store (&SI[0],       1, 0x1C  , c);
		Store (&pif[0],      1, 0x800 , c);
		Store (&ramRegs0[0], 1, 0x30  , c);
		Store (&ramRegs4[0], 1, 0x30  , c);
		Store (&ramRegs8[0], 1, 0x30  , c);

		Store (&VsyncTime, 1, sizeof(u32), c);
		Store (&VsyncInterrupt, 1, sizeof(u32), c);
		Store (&CpuRegs[0], 32, sizeof(u64), c);
		Store (&MmuRegs[0], 32, sizeof(u64), c);
		Store (&FpuRegs.w[0], 1, sizeof (R4K_FPU_union), c);
		Store (&FpuControl[0], 32, sizeof(u32), c);
		Store (&cpuHi, 1, sizeof (s64), c);
		Store (&cpuLo, 1, sizeof (s64), c);
		Store (&instructions, 1, sizeof (u32), c);
		Store ((void *)&pc, 1, sizeof (u32), c);
		Store ((void *)&InterruptNeeded, 1, sizeof (char), c);
		Store (&tlb[0], 32, sizeof(s_tlb), c);
		Store (&CountInterrupt, 1, sizeof(u32), c);
		StoreEvents (c);
		//Debug (0, "Stored: %i", core);
}

void LoadCore (int core) {
	u8 *c;
	int x;
	ResetStore ();
	if (core) {
		c = DynaSave;
	} else {
		c = InterpSave;
	}

		for (x = 0; x < 8*1024*1024; x += 1024) {
			Restore (&rdram[x], 1, 1024, c);
		}
		Restore (&idmem[0],    1, 0x2000, c);
		Restore (&SP[0],       1, 0x20  , c);
		Restore (&SP2[0],      1, 0x08  , c);
		Restore (&DPC[0],      1, 0x10  , c);
		Restore (&DPS[0],      1, 0x10  , c);
		Restore (&MI[0],       1, 0x10  , c);
		Restore (&VI[0],       1, 0x38  , c);
		Restore (&AI[0],       1, 0x18  , c);
		Restore (&PI[0],       1, 0x34  , c);
		Restore (&RI[0],       1, 0x20  , c);
		Restore (&SI[0],       1, 0x1C  , c);
		Restore (&pif[0],      1, 0x800 , c);
		Restore (&ramRegs0[0], 1, 0x30  , c);
		Restore (&ramRegs4[0], 1, 0x30  , c);
		Restore (&ramRegs8[0], 1, 0x30  , c);

		Restore (&VsyncTime, 1, sizeof(u32), c);
		Restore (&VsyncInterrupt, 1, sizeof(u32), c);
		Restore (&CpuRegs[0], 32, sizeof(u64), c);
		Restore (&MmuRegs[0], 32, sizeof(u64), c);
		Restore (&FpuRegs.w[0], 1, sizeof (R4K_FPU_union), c);
		Restore (&FpuControl[0], 32, sizeof(u32), c);
		Restore (&cpuHi, 1, sizeof (s64), c);
		Restore (&cpuLo, 1, sizeof (s64), c);
		Restore (&instructions, 1, sizeof (u32), c);
		Restore ((void *)&pc, 1, sizeof (u32), c);
		Restore ((void *)&InterruptNeeded, 1, sizeof (char), c);
		Restore (&tlb[0], 32, sizeof(s_tlb), c);
		Restore (&CountInterrupt, 1, sizeof(u32), c);
		RestoreEvents (c);
		//Debug (0, "Restored: %i", core);
}

void InitCoreSync () {
	InterpSave = (u8 *)malloc (9*1024*1024);
	DynaSave   = (u8 *)malloc (9*1024*1024);
	ResetStore ();
	SaveCore (1);
	SaveCore (0);
}

void DisassembleRange (u32, u32);
void CoreCompare () {
	int x;
	u64 *DynaRegs;
	DynaRegs = (u64 *)(DynaSave+8399280);
	for (x = 0; x < 32; x++) {
		if (CpuRegs[x] != DynaRegs[x]) {
			DisassembleRange (0x80000000, pc+0x100);
			Debug (0, "PC = %08X", pc);
			Debug (0, "CpuRegs[%i] (%08X) != DynaRegs[%i] (%08X)", x, CpuRegs[x], x, DynaRegs[x]);
			__asm int 3;
		}
	}
}