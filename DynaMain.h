#include <stdio.h>

//#define __DYNALOG__

typedef union {
	DWORD * udwPtr;
	WORD * uwPtr;
	BYTE * ubPtr;
	void * ptr;
} BUFFER_POINTER;

typedef struct {
	u32 target;
	u8 *mem;
} LinkInfo;

typedef struct {
	u32 target;
	u8 *mem;
	u32 crc;
	u32 blocksize; // in N64 CPU PC units
	u32 memused;   // in PC CPU units
	u32 RecOp;
} BlockType;


extern void (*r4300i[0x40])();
extern void (*special[0x40])();
extern void (*regimm[0x20])();
extern void (*MmuSpecial[0x40])();
extern void (*MmuNormal[0x20])();
extern void (*FpuCommands[0x40])();
extern void CheckInterrupts(void);
extern u8 *rdram;

extern BUFFER_POINTER x86Buff, x86BlockPtr;
extern BlockType BlockInfo[];
// Function Definitions
bool DynaCompileBlock(u32 blockCount);

unsigned long GenerateCRC (unsigned char *data, int size);
void OpcodeLookup (DWORD addy, char *out);
unsigned long CRCRec (unsigned char *data, int size);
inline void DynaLog(char * Message, ...) {


#ifdef __DYNALOG__
	char Msg[512];
	va_list ap;

	va_start(ap, Message);
	vsprintf(Msg, Message, ap);
	va_end(ap);

	fprintf(dynaLogFile, "%s\n", Msg);
#endif
}
