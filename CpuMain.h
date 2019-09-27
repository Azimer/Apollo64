#define incrementer 1
//#define USE_OLD_FPU
#define USE_NEW_TLB

class r4300iTLB {

	protected:
		u32 * TLBTable;
		s_tlb *TLBList;
		u32 listSize;

	public:
		r4300iTLB() {
			this->listSize = 32; // 32 is a good starting number
			this->TLBTable = (u32 *)VirtualAlloc(NULL, 0x400000, MEM_COMMIT, PAGE_READWRITE);
			this->TLBList = (s_tlb *)VirtualAlloc(NULL, sizeof(s_tlb) * listSize, MEM_COMMIT, PAGE_READWRITE);

			if (this->TLBTable == NULL)	_asm { int 3 }
			if (this->TLBList  == NULL) _asm { int 3 }
			this->ResetTLB();
		}
		~r4300iTLB() {
			VirtualFree(this->TLBTable, 0, MEM_RELEASE);
			VirtualFree(this->TLBList, 0, MEM_RELEASE);
		}
		void ResetTLB(void);
		void ResizeTLBList (void);
		void ClearEntry (int index);
		void Probe ();
		void IndexEntry (int index);
		void RandomEntry (int index);
		void mapmem (u32 tlbaddy, u32 mapaddy, u32 size);
		u32 operator[] (int index) {
			return TLBTable[index];
		}
};

extern r4300iTLB TLBLUT; 
extern int InterruptTime;
void QuickTLBExcept ();