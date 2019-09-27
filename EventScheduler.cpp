#include <windows.h>
#include <stdio.h>
#include "WinMain.h"
#include "EmuMain.h"
#include "audiodll.h"
#include "cpumain.h"

#define RSP_DLIST_EVENT		0x1
#define RSP_EVENT			0x2
#define SI_DMA_EVENT		0x3
#define PI_DMA_EVENT		0x4
#define AI_DMA_EVENT		0x5
#define CHECK_MI_EVENT		0x6
#define AID_DMA_EVENT		0x7

#define MAXEVENTS 30 

extern u32 PiInterrupt, CountInterrupt, VsyncInterrupt;
extern u32 VsyncTime;
extern int InterruptTime;


typedef struct {
	u32 Time;
	u32 EventType;
	u32 extra[8];
} EventList;

EventList EList [MAXEVENTS];
u32 ListCnt = 0;
int NextEvent;

extern u32 PiInterrupt;

//extern u32 LastAudio;
extern u32 LastAint;

//	VsyncTime = MIN(MIN(CountInterrupt,PiInterrupt),VsyncInterrupt);
void PrintInterruptStatus () {
	Debug (0, "Instructions: %08X", instructions);
	Debug (0, "VsyncTime: %08X", VsyncTime);
	Debug (0, "VsyncInterrupt: %08X", VsyncInterrupt);
	Debug (0, "PiInterrupt: %08X", PiInterrupt);
	Debug (0, "CountInterrupt: %08X", CountInterrupt);
//	Debug (0, "LastAudio: %08X - LastAint: %08X", LastAudio, LastAint);
	for (int x = 0; x < MAXEVENTS; x++) {
		if (EList[x].EventType)
			switch (EList[x].EventType) {
				case RSP_DLIST_EVENT:
					Debug (0, "Pending DLIST Event: %08X", EList[x].Time);
				break;
				case RSP_EVENT:
					Debug (0, "Pending RSP Event: %08X", EList[x].Time);
				break;
				case SI_DMA_EVENT:
					Debug (0, "Pending SI Event: %08X", EList[x].Time);
				break;
				case PI_DMA_EVENT:
					Debug (0, "Pending PI Event: %08X", EList[x].Time);
				break;
				case AI_DMA_EVENT:
					Debug (0, "Pending AI Event: %08X", EList[x].Time);
				break;
			}
	}
}

void ScheduleNextEvent () {
	u32 LastTime = 0;
	static int cnt = 0;

	if (ListCnt == 0) {
		NextEvent = -1;
		return;
	}

	LastTime = 0xFFFFFFFF;
	instructions = (VsyncTime - InterruptTime);

	for (int x = 0; x < MAXEVENTS; x++) {
		if (EList[x].EventType)
			if (LastTime > (EList[x].Time-instructions)) {
				NextEvent = x;
				LastTime = (EList[x].Time-instructions);
			}
	}

	if (LastTime == 0xFFFFFFFF) {
		__asm int 3;
		return;
	}

	PiInterrupt = EList[NextEvent].Time;

	void GetNextInterrupt ();
	GetNextInterrupt ();

//	VsyncTime = MIN(MIN(CountInterrupt,PiInterrupt),VsyncInterrupt);
	//Debug (0, "PiInterrupt set to: %08X, VsyncTime set to: %08X", PiInterrupt, VsyncTime);
	//Debug (0, "VSyncInterrupt: %08X", VsyncInterrupt);
}

void ClearEventList () {
	ListCnt = 0;
	memset (EList, 0, sizeof(EventList)*10);
	PiInterrupt = -1;
	NextEvent = -1;
	/*for (int x = 0; x < MAXEVENTS; x++) {
		EList[x].Time = -1;
	}*/
}

void ScheduleEvent (u32 EventType, u32 Time, u32 size, void *extra) {
	int FreeEvent;
	FreeEvent = 0;
	
	while ((EList[FreeEvent].EventType != 0) && (FreeEvent < MAXEVENTS))
		FreeEvent++;

	if (FreeEvent == MAXEVENTS) {
		FreeEvent = 0;
		while (EList[FreeEvent].EventType != 5)
			FreeEvent++;
		Debug (0, "FreeEvent is maxed... hacking... ");
		//__asm int 3; // Assert that this will never be true...
	}

	//if (Time & 0x3)
	//	__asm int 3;

	instructions = (VsyncTime - InterruptTime);
	EList[FreeEvent].EventType = EventType;
	EList[FreeEvent].Time = (instructions + Time)+incrementer;
	if (extra != NULL)
		memcpy (EList[FreeEvent].extra, extra, size); // It might slow down emulation a lil
	else
		memset (EList[FreeEvent].extra, 0, sizeof(u32)*8);
/*
	switch (EList[FreeEvent].EventType) {
		case PI_DMA_EVENT:
			Debug (0, "PI Interrupt Scheduled");
			//Debug (0, "SI DMA Event Scheduled: Time: %08X, RealTime: %08X, COUNT: %08X", Time, EList[NextEvent].Time, instructions);
		break;
	}*/
	ListCnt++;
	ScheduleNextEvent ();
}

extern u8 *SI;
extern u8 *PI;
extern u8 *AI;
// Returns the InterruptNeeded flags for the scheduled event...
u32 aibuff = 0;
u32 ExecuteEvent () {
	u32 retVal = 0;
	u32 Event = EList[NextEvent].EventType;
	if (NextEvent == -1)
		return 0;
	switch (Event) {
		case RSP_EVENT:
			SignalRSP((u32 *)EList[NextEvent].extra);
		break;
		case PI_DMA_EVENT:
			extern u32 ChangeProtectionLevel;
			extern u32 romsize;
			if (EList[NextEvent].extra) {
				u32 *DMAINFO = (u32 *)EList[NextEvent].extra;
				if (ChangeProtectionLevel == PAGE_NOACCESS)
					VirtualProtect((void *)(valloc+0x10000000), romsize, PAGE_READONLY, &ChangeProtectionLevel); // Ok.. next read is bad				
				for (u32 x = 0; x <= DMAINFO[2]; x++) {
					*(u8 *)returnMemPointer((DMAINFO[0]+x)^3) = *(u8 *)returnMemPointer((DMAINFO[1]+x)^3);
				}
			}
			//Debug (2, "PI Done");
			((u32 *)PI)[4] &= ~0x3;
			retVal = PI_INTERRUPT;
		break;
		case SI_DMA_EVENT:
			((u32 *)SI)[6] &= ~0x1; // Clear the DMA Busy Flag
			retVal = SI_INTERRUPT;
		break;
		case CHECK_MI_EVENT:
			retVal = 0;
		break;

		case AID_DMA_EVENT:
			void AiInterrupt ();
			EList[NextEvent].EventType = 0; // Clears this event
			instructions = (VsyncTime - InterruptTime);
			MmuRegs[0x9] = instructions;
//			snddll.AiCallBack();
			AiInterrupt ();

			retVal = AI_INTERRUPT;
			ListCnt--;
			ScheduleNextEvent ();
			return retVal;
		break;

		case AI_DMA_EVENT: {
			extern u8 *AI;

			void AiCallBack ();
			AiCallBack ();

			retVal = 0;
		}
		break;
		default:
			Debug (0, "Unknown Task Executed!");
			retVal = 0;
	}
	EList[NextEvent].EventType = 0; // Clears this event
	ListCnt--;
	ScheduleNextEvent ();
	return retVal;
}


void SaveEvents (FILE *fp) {
	fwrite (&PiInterrupt, 1, sizeof(u32), fp);
	fwrite (EList, MAXEVENTS, sizeof(EventList), fp);
}

void LoadEvents (FILE *fp) {
	fread (&PiInterrupt, 1, sizeof(u32), fp);
	fread (EList, MAXEVENTS, sizeof(EventList), fp);
	ListCnt = 0;
	for (int x = 0; x < MAXEVENTS; x++) {
		if (EList[x].EventType != 0)
			ListCnt++;
	}
	ScheduleNextEvent ();
}
