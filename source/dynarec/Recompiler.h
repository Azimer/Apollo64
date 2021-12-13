#ifndef __RECOMPILER_DOT_H__
#define __RECOMPILER_DOT_H__

extern void (*r4300i[0x40])();
extern void (*special[0x40])();
extern void (*regimm[0x20])();
extern void (*MmuSpecial[0x40])();
extern void (*MmuNormal[0x20])();
extern void (*FpuCommands[0x40])();
extern void CheckInterrupts(void);
extern u8 *rdram;

#endif