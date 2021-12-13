/* 

	Todo list for New TLB

   - I need to search through the TLBList for matching TLB entries (needed for speed)
   - I need to delete unused TLB entries
   - I need to detect a TLB Clear
   - I need to implement Miss and Invalid exception handling
   - I need to implement Random TLB
   - I need to clear out TLB on rom reset
   - I need to setup direct TLBTable access
   - Clean Code...  TLBAddEntry and TLBDelEntry functions would come in handy

*/


#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include "WinMain.h"
#include "EmuMain.h"
#include "CpuMain.h"
#include "resource.h"

#define MMU_INDEX		MmuRegs[0]
#define MMU_RANDOM		MmuRegs[1]
#define MMU_ENTRYLO0	MmuRegs[2]
#define MMU_ENTRYLO1	MmuRegs[3]
#define MMU_CONTEXT		MmuRegs[4]
#define MMU_PAGEMASK	MmuRegs[5]
#define MMU_WIRED		MmuRegs[6]
#define MMU_BADVADDR	MmuRegs[8]
#define MMU_ENTRYHI		MmuRegs[10]

#define TLB_LOAD		2
#define TLB_STORE		3

extern s_tlb tlb[32];
extern bool tlbinvalid;
void GenerateException(DWORD addr, DWORD type);

r4300iTLB TLBLUT;

extern int inDelay;
void GenerateTLBException (DWORD addr, DWORD type) {
		MMU_RANDOM = instructions & 0x1F;
		if (MMU_RANDOM < MMU_WIRED)
			MMU_RANDOM = 0x1F;

		//Debug (0, "MMU_RANDOM = %i", MMU_RANDOM);

		MMU_CONTEXT &= 0xFF800000;
		MMU_CONTEXT |= (addr >> 13) << 4;
		MMU_ENTRYHI &= 0x00001FFF;
		MMU_ENTRYHI = (addr & 0xFFFFE000);
		MMU_BADVADDR = addr;

		if (pc != addr)
			pc -= 4;

		Debug (0, "TLB Exception... pc = %08X - addr = %08X", pc, addr);

		if (inDelay == 1)
			Debug (0, "TLB Exception in Delay Slot at PC = %08X", pc);
		if (TLBLUT[addr >> 12] == 0xFFFFFFFE)
			tlbinvalid = true;

		if (type)
			GenerateException(pc, TLB_STORE);
		else
			GenerateException(pc, TLB_LOAD);

		//MMU_INDEX = 0;
		//Emulate ();
		//RaiseException (0xc0001337, 0,0, NULL); // TLB Exception
}

// Golden Eye Range... 7f000000,10034b30,1000000 (borrowed from UltraHLE 1.0)
void r4300iTLB::mapmem (u32 tlbaddy, u32 mapaddy, u32 size) {
	u32 thesize = size / 0x1000;
	u32 addy = tlbaddy >> 12;
	Debug (0, "Mapping Address %08X to %08X of size %08X", tlbaddy, mapaddy, size);
	for (u32 i = 0x0; i < thesize; i++) {
		this->TLBTable[addy+i] = (mapaddy+(i << 12));
	}
}

void r4300iTLB::ResetTLB(void) {
			memset(this->TLBTable, 0xFF, 0x400000); // 4MB Memory LUT
			memset(this->TLBList , 0x00, sizeof(s_tlb) * listSize); // 4MB Memory LUT
			for (DWORD i = 0x0; i < 0x800; i++) {
				this->TLBTable[0x80000+i] = ((0x80000+i) << 12);
				this->TLBTable[0xA0000+i] = ((0xA0000+i) << 12);
				//this->TLBTable[0x80000+i] = valloc + (i << 12);
				//this->TLBTable[0xA0000+i] = valloc + (i << 12);
			}
		}

void r4300iTLB:: ResizeTLBList (void) {
			s_tlb *oldTLBList = this->TLBList;
			this->listSize <<= 1; // Double the TLB List
			this->TLBList = (s_tlb *)VirtualAlloc(NULL, sizeof(s_tlb) * listSize, MEM_COMMIT, PAGE_READWRITE);
			memcpy (TLBList, oldTLBList, listSize >> 1); // Copy it over
			VirtualFree(oldTLBList, 0, MEM_RELEASE); // Free the old List
		}


		void RecreateLUT () {
		}

        // Clears an Entry
void r4300iTLB::ClearEntry (int index) {
			int page = ((tlb[index].hh >> 13) + 1) * 4096;
			int mask = ~(tlb[index].hh | 0x1fff);
			int maskLow = (tlb[index].hh | 0x1fff);
			u32 TLBStartLoc = (tlb[index].hl & mask) >> 12; // Start of TLB Entry
			u32 TLBLength   = (maskLow+1) >> 10;//tlb[index].hh; // Length of TLB Entry
			if ((TLBStartLoc >= 0x80000) && (TLBStartLoc < 0xBFFFF)) // Never unmap these locations
				return;
			memset (&this->TLBTable[TLBStartLoc], 0xFF, TLBLength);
		}

        // This should place the TLB entry in the Main TLB List...
		// Should I be picky about its location in the Main TLB List?
void r4300iTLB::Probe () {
	// TODO: Before I can do this, I will need to keep a list of things like the mask...
	//if (TLBTable[MMU_ENTRYHI >> 12] < 0xFFFFFFF0)
		/*	
			for (int i=0; i < 32; i++) {
				u32 mask = ~(TLBList[i].hh | 0x1fff);//& 0xffffe000;
				if((TLBList[i].hl&mask) == (MMU_ENTRYHI&mask)) {
					tlb[0x13].hh = TLBList[i].hh; tlb[0x13].hl = TLBList[i].hl;
					tlb[0x13].lh = TLBList[i].lh; tlb[0x13].ll = TLBList[i].ll;
					tlb[0x13].isGlobal = TLBList[i].isGlobal;
					MMU_INDEX=0x13;
					return;
				}
			}
		*/	
		}

        // Index Entry will ALWAYS overwrite previous entries

void r4300iTLB::IndexEntry (int index) {
			/* Not sure how I will handle this or if I will.  Perhaps I will replace it
			   entry by entry.
			if (MMU_ENTRYHI == 0x80000000) {
				this->RecreateLUT (); // Allows for the case where the TLB Tables are cleared
			}
			*/
			int page = ((tlb[index].hh >> 13) + 1) * 4096;
			int mask = ~(tlb[index].hh | 0x1fff);
			int maskLow = (tlb[index].hh | 0x1fff);
			// TODO!!!! : This NEEDS to be changed!!!  I need to seek out the old entry
			// in the TLB List and delete it and all its entries in the TLBTable!!!
			// A MUST for GoldenEye
			TLBList[index].hh = tlb[index].hh; TLBList[index].hl = tlb[index].hl;
			TLBList[index].lh = tlb[index].lh; TLBList[index].ll = tlb[index].ll;
			TLBList[index].isGlobal = tlb[index].isGlobal;
			if (MMU_ENTRYHI != 0x80000000) { // This ignores possible clearing! :-/
				u32 TLBStartLoc = (tlb[index].hl & mask); // Start of TLB Entry
				u32 TLBLength   = maskLow+1;//tlb[index].hh; // Length of TLB Entry
				u32 addr = TLBStartLoc;
				u32 cnt = 0;
				do {
					if (addr & page) {//odd page
						if (tlb[index].ll & 0x2) {
							this->TLBTable[addr >> 12] = 0x80000000 | ((tlb[index].ll << 6) & (mask >> 1)) | (addr & (maskLow >> 1));
						} else {
							this->TLBTable[addr >> 12] = 0xFFFFFFFE; // This should signify invalid entry when exception happens
						}
					} else {//even page
						if (tlb[index].lh & 0x2) {
							this->TLBTable[addr >> 12] = 0x80000000 | ((tlb[index].lh << 6) & (mask >> 1)) | (addr & (maskLow >> 1));
						} else {
							this->TLBTable[addr >> 12] = 0xFFFFFFFE; // This should signify invalid entry when exception happens
						}
					}
					//Debug (2, "	Mapped %08X to %08X", addr, this->TLBTable[addr >> 12]);
					addr += 0x1000; // 4K is minimum TLB Entry size
				} while (addr < (TLBLength+TLBStartLoc));
			}
			// Check 
		}

		// Random Entry will NEVER Overwrite previous entries
void r4300iTLB::RandomEntry (int index) {
	// Command will be the most complicated to keep accurate (and the reason for a TLB list)
	/*MMU_RANDOM = instructions & 0x1F;
	if (MMU_RANDOM < MMU_WIRED)
		MMU_RANDOM = 0x1F;*/
	IndexEntry (index);
	//__asm int 3;
}

void DisassembleRange (u32 Start, u32 End);
#ifdef USE_NEW_TLB
u32 VirtualToPhysical(u32 addr, int type) {
	u32 oldAddr = addr;
	u32 pAddr = addr;
	bool match = false;
	/*
	if (addr == 0xFFFFFFFC) {
		DisassembleRange (0x80004090, 0x80005410);
		__asm int 3;
	}*/

	if (TLBLUT[addr >> 12] < 0xFFFFFFFE) {
		return oldAddr = TLBLUT[addr>>12] + (addr & 0xfff);
	} else {
//		Debug (0, "TLB Issue at PC %08X accessing addr %08X  %08X", pc, addr);
//		RaiseException (0xc0001337, 0,0, NULL); // TLB Exception
//		oldAddr = TLBLUT[addr>>12];
		// Invalid or Miss Exception!
//		Debug (1, "Unhandled Miss/Invalid Exception! (Unimplemented in New TLB)");
//		cpuIsDone = true;
//		cpuIsReset = true;
		if (TLBLUT[addr >> 12] == 0xFFFFFFFE)
			tlbinvalid = true;
	}

//	__asm int 3;
/*
	if ((addr & 0xC0000000)==0xC0000000) ;
	else if (addr & 0x80000000) { 
		return addr;
	}
*/
/*
	int page;
	int mask;
	int maskLow;
	int i;
	for (i=31; i >= 0; i--) {
		page = ((tlb[i].hh >> 13) + 1) * 4096;
		mask = ~(tlb[i].hh | 0x1fff);
		maskLow = (tlb[i].hh | 0x1fff);

		if ((addr & mask) == (tlb[i].hl & mask)) {
			//match
			match = true;
				if (addr & page) {//odd page
					if (tlb[i].ll & 0x2) 
						addr = 0x80000000 | ((tlb[i].ll << 6) & (mask >> 1)) | (addr & (maskLow >> 1));
					else {
						match = false;
						tlbinvalid = true;
						//Debug (0, "%08X: TLB Invalid Exception %08X", pc, addr); 
					}
				} else {//even page
					if (tlb[i].lh & 0x2) {
						addr = 0x80000000 | ((tlb[i].lh << 6) & (mask >> 1)) | (addr & (maskLow >> 1));
					} else {
						match = false;
						tlbinvalid = true;
						//Debug (0, "%08X: TLB Invalid Exception %08X", pc, addr); 
					}
				}
			if (match != false) {
				if (addr != oldAddr)
					__asm int 3;
			}
			break;
		}
	}
*/	
	
	if (match == false) { // TLB Hacks galore!!!
		Debug (0, "TLB Miss: %08X", pc);
		
/*		if (tlbinvalid == true) {
			if (oldAddr != 0xFFFFFFFE)
				__asm int 3;
		} else {
			if (oldAddr != 0xFFFFFFFF) {
				__asm int 3;
				cpuIsDone = cpuIsReset = true;
			}
		}*/
		//if (type)
		//	Debug(0,"TLB Store Miss at %08X, pc:%08X, exception!",addr,pc-4);
		//else
		//	Debug(0,"TLB Load Miss at %08X, pc:%08X, exception!",addr,pc-4);

		/*for (int i = MMU_WIRED; i < 32; i++) {
			if (tlb[i].hl == 0x80000000)
				break;
			if ((tlb[i].ll & tlb[i].lh & 2)==0)
				break;
		}

		if (i < 32) {
			MMU_RANDOM = i;
		} else*/ {
			MMU_RANDOM = instructions & 0x1F;
			if (MMU_RANDOM < MMU_WIRED)
				MMU_RANDOM = 0x1F;
		}

		//Debug (0, "MMU_RANDOM = %i", MMU_RANDOM);

		MMU_CONTEXT &= 0xFF800000;
		MMU_CONTEXT |= (addr >> 13) << 4;
		MMU_ENTRYHI &= 0x00001FFF;
		MMU_ENTRYHI = (addr & 0xFFFFE000);
		MMU_BADVADDR = addr;

		if (pc != addr) {
			//if ((pc != 0x150A916C) && (pc != 0x150A914C))
			//	Debug (2, "%08X: Non Instruction Miss\n", pc);
			pc -= 4;
		} 

//		Debug (2, "TLB Miss or Invalid at %08X\n", addr);

		//Debug (2, "%08X: ", addr);
		//if (inDelay == 1)
		//	Debug (0, "TLB Exception in Delay Slot!");
		if (type)
			GenerateException(pc, TLB_STORE);
		else
			GenerateException(pc, TLB_LOAD);

		//MMU_INDEX = 0;
		//Emulate ();
		RaiseException (0xc0001337, 0,0, NULL); // TLB Exception
		return 0;
	}
	return addr;
	//return oldAddr;
}

void QuickTLBExcept () {
	if (TLBLUT[pc >> 12] == 0xFFFFFFFE) {
		tlbinvalid = true;
		MMU_RANDOM = instructions & 0x1F;
		if (MMU_RANDOM < MMU_WIRED)
			MMU_RANDOM = 0x1F;
	}

	MMU_CONTEXT &= 0xFF800000;
	MMU_CONTEXT |= (pc >> 13) << 4;
	MMU_ENTRYHI &= 0x00001FFF;
	MMU_ENTRYHI = (pc & 0xFFFFE000);
	MMU_BADVADDR = pc;

	//Debug (2, "%08X: ", pc);

	if (inDelay == 1)
		Debug (0, "TLB Exception in Delay Slot at PC = %08X", pc);

	GenerateException(pc, TLB_LOAD);
}

void opTLBP(void){
	//Debug (0, "TLB Probe");
	MMU_INDEX|=0xFFFFFFFF80000000;
	TLBLUT.Probe (); // Need to put the current TLB entry in the Table
	for (int i=0; i < 32; i++) {
		u32 mask = ~(tlb[i].hh | 0x1fff);//& 0xffffe000;
		if((tlb[i].hl&mask) == (MMU_ENTRYHI&mask)) {//assumes it is always global
			MMU_INDEX=i;
			return;
		}
	}
}

void opTLBR(void){
	//Debug (0, "TLB Read");
	int j = MMU_INDEX & 0x1f;
	MMU_PAGEMASK = tlb[j].hh;
	MMU_ENTRYHI = tlb[j].hl & (~tlb[j].hh);
	MMU_ENTRYLO0 = tlb[j].lh | tlb[j].isGlobal;
	MMU_ENTRYLO1 = tlb[j].ll | tlb[j].isGlobal;
}

void opTLBWI(void){
	u32 j = MMU_INDEX & 0x1f;

	TLBLUT.ClearEntry (j);

/*	if (MMU_ENTRYHI != 0x80000000) {
		Debug (0, "TLB Write Index @ %08X", pc-4);
		Debug (0, "- INDEX    = %08X", MMU_INDEX);
		Debug (0, "- PAGEMASK = %08X", MMU_PAGEMASK);
		Debug (0, "- ENTRYHI  = %08X", MMU_ENTRYHI);
		Debug (0, "- ENTRYLO0 = %08X", MMU_ENTRYLO0);
		Debug (0, "- ENTRYLO1 = %08X", MMU_ENTRYLO1);
		Debug (0, "- G = %08X", (MMU_ENTRYLO0 & MMU_ENTRYLO1 & 0x1));
	}//*/
	tlb[j].hh = MMU_PAGEMASK;
	tlb[j].hl = MMU_ENTRYHI & (~MMU_PAGEMASK);
	tlb[j].lh = MMU_ENTRYLO0 & 0xFFFFFFFE;
	tlb[j].ll = MMU_ENTRYLO1 & 0xFFFFFFFE;
	tlb[j].isGlobal = (u8)(MMU_ENTRYLO0 & MMU_ENTRYLO1 & 0x1);

//		int page = ((tlb[j].hh >> 13) + 1) * 4096;
//		int mask = ~(tlb[j].hh | 0x1fff);
//		int maskLow = (tlb[j].hh | 0x1fff);
//	Debug (0, "Page = %08X", page);
//	Debug (0, "mask = %08X", mask);
//	Debug (0, "maskLow = %08X", maskLow);
//	Debug (0, "hl = %08X", tlb[j].hl);
	TLBLUT.IndexEntry (j);
}
/*
	for (int i=31; i >= 0; i--) {
		int page = ((tlb[i].hh >> 13) + 1) * 4096;
		int mask = ~(tlb[i].hh | 0x1fff);
		int maskLow = (tlb[i].hh | 0x1fff);

		if ((addr & mask) == (tlb[i].hl & mask)) {
			//match
			match = true;
				if (addr & page) {//odd page
					if (tlb[i].ll & 0x2) 
						addr = 0x80000000 | ((tlb[i].ll << 6) & (mask >> 1)) | (addr & (maskLow >> 1));
					else {
						match = false;
						tlbinvalid = true;
						//Debug (0, "%08X: TLB Invalid Exception %08X", pc, addr); 
					}
				} else {//even page
					if (tlb[i].lh & 0x2) {
						addr = 0x80000000 | ((tlb[i].lh << 6) & (mask >> 1)) | (addr & (maskLow >> 1));
					} else {
						match = false;
						tlbinvalid = true;
						//Debug (0, "%08X: TLB Invalid Exception %08X", pc, addr); 
					}
				}
			break;
		}
	}
*/

void opTLBWR(void){
	u32 j = MMU_RANDOM & 0x1f;
	TLBLUT.ClearEntry (j); // This shouldn't be here.  This will only prove to make things redundant
/*		Debug (0, "TLB Write Random @ %08X", pc-4);
		Debug (0, "- RANDOM    = %08X", MMU_RANDOM);
		Debug (0, "- PAGEMASK = %08X", MMU_PAGEMASK);
		Debug (0, "- ENTRYHI  = %08X", MMU_ENTRYHI);
		Debug (0, "- ENTRYLO0 = %08X", MMU_ENTRYLO0);
		Debug (0, "- ENTRYLO1 = %08X", MMU_ENTRYLO1);
		Debug (0, "- G = %08X", (MMU_ENTRYLO0 & MMU_ENTRYLO1 & 0x1));//*/
	tlb[j].hh = MMU_PAGEMASK;
	tlb[j].hl = MMU_ENTRYHI & (~MMU_PAGEMASK);
	tlb[j].lh = MMU_ENTRYLO0 & 0xFFFFFFFE;
	tlb[j].ll = MMU_ENTRYLO1 & 0xFFFFFFFE;
	tlb[j].isGlobal = (u8)(MMU_ENTRYLO0 & MMU_ENTRYLO1 & 0x1);
	TLBLUT.RandomEntry (j);
}
#endif
