#include "../common.h"
#include "Recompiler.h"
#include "RecompilerDebug.h"
#include "X86Regs.h"
#include "X86Ops.h"
#include "../EmuMain.h"
#include "../WinMain.h"
#include "../CpuMain.h"

u32 oscall2[][18]={
#include "RecompilerHLECodes.h"
{0}};

unsigned long CRCRec2 (unsigned char *data, int size) {


	register unsigned long crc;
	int c = 0;

	crc = 0xFFFFFFFF;
	u8 temp[4];

	while(c<size) {
		u32 t = *(u32 *)(data+c);
		//crc = (crc >> 8) | (crc << 24);
		crc ^= t;
		c+=4;
	}
	return( crc^0xFFFFFFFF );
}


inline void StripConst (u32 &op, u32 &comp, u8 regs[]) {
	s_sop myop;
	if (comp == 0) {
		op = 0;
		return;
	} else if (comp == -1) {
		comp = 0; op = 0;
		return;
	}
	*(u32 *)&myop = op;
	switch (myop.op) {
		case 0x2: 
		case 0x3:	
			op = 0; comp = 0; 
		break;
		case 0x8: 
		case 0x9: 
		case 0xD:
			if (regs[myop.rs] == 1)	{ 
				op &= 0xFFFF0000; comp &= 0xFFFF0000; 
			}
		break;
		case 0xF:
			if (((op & 0xFFFF) >= 0x8000) && ((op & 0xFFFF) < 0x9000)) {
				op &= 0xFFFF0000; comp &= 0xFFFF0000;
				regs[myop.rt] = 1;
			}
		break;
		case 0x20: case 0x21: case 0x23: case 0x24: case 0x25: case 0x27:
		case 0x28: case 0x29: case 0x2B: case 0x37: case 0x3F: 
			{
				if (regs[myop.rs] == 1) {
					op &= 0xFFFF0000; comp &= 0xFFFF0000;
				}
			}
	}
}

#define STRIPCONST(op) 


bool FindHLEFunc(u32 addy, char *func) {
	bool badLUI;
	u8 badregs[32];
	u32 *op;
	u32 value, comp;
	if (addy > 0xa4000000)
		return false;
	if (TLBLUT[addy>>12] > 0xFFFFFFF0)
		return false; // Can't HLE yet... 
	op = (u32 *)returnMemPointer(TLBLUT[addy>>12]+(addy & 0xfff));

	int i = 0;

	while (oscall2[i][0] != 0) {
		badLUI = false;
		value = *op;
		comp = oscall2[i][0];
		memset (badregs, 0, 32);
//		if (comp == 0)					{ value = 0; } 
//		else if ((comp&0xFFFFFF) == 0)	{ value &= 0xFF000000; } 
//		else if ((comp&0xFFFF) == 0)	{ value &= 0xFFFF; }
		StripConst(value, comp, badregs);
		if (value == comp) { // Possible Detection...
			for (int x = 0; x < 14; x++) {
				value = *(op+x); comp = oscall2[i][x];
//				if (comp == 0)					{ value = 0; } 
//				else if ((comp&0xFFFFFF) == 0)	{ value &= 0xFF000000; } 
//				else if ((comp&0xFFFF) == 0)	{ value &= 0xFFFF; }
				StripConst(value, comp, badregs);
				if (value != comp) {
					break;
				}
				if (x == 13) { // Function found??
					if (oscall2[i][15] == 0) {
						Debug (0, "Possible HLE Function @ %08X of %s", addy, (char *)oscall2[i][17]);
						if (func != NULL)
							strcpy (func, (char *)oscall2[i][17]);
						oscall2[i][15] = addy;
						return true;
					}
					else if (oscall2[i][15] != addy)
						Debug (0, "Duplicate OS Function Found...");
					oscall2[i][15] = addy;
				}
			}
		}
		i++;
	}
	return false;

}
//0x80022A10
void ScanHLEFunction () {
	u32 count = 0;
	u32 startAddy = pc;
	u32 op;
	u32 crc;
	static u32 cnt = 0;
	bool badLUI = false;

	FindHLEFunc(pc, NULL);

	return;

	if (pc > 0xa4000000)
		return;
	count = 0;

	do {
		if (TLBLUT[startAddy>>12] > 0xFFFFFFF0)
			return; // Can't HLE yet... 
		op = *(u32 *)returnMemPointer(TLBLUT[startAddy>>12]+(startAddy & 0xfff));
		switch (op>>0x1A) {
			case 0x2: // J - Always mask
			case 0x3: // JAL - Always mask
				op &= 0xFC000000;
			break;
			case 0x8: // ADDI - Not sure...
			case 0x9: // ADDIU - Not sure...
			case 0xD: // ORI - Not sure
//				if (badLUI)
					op &= 0xFFFF0000;
//				badLUI = false;
			break;
			case 0xF: // LUI - Not sure... but prolly
//				if (((op & 0xFFFF) >= 0x8000) && ((op & 0xFFFF) < 0x9000)) {
//					badLUI = true;
					op &= 0xFFFF0000;
//				}
			break;
		}
		crc ^= op;
		startAddy+=4;
		count++;
	} while ((op != 0x03e00008) && (count < 139));
	if (count == 0x600)
		Debug (0, "Super Long Function: %08X", pc);
	if (pc == 0x8024DEC8)
		Debug (0, "PC %08X to %08X - CRC: %08X - count: %i", pc, startAddy, (crc ^ 0xFFFFFFFF), count);
	if (pc == 0x80022A10)
		Debug (0, "PC %08X to %08X - CRC: %08X - count: %i", pc, startAddy, (crc ^ 0xFFFFFFFF), count);
	switch (crc^0xFFFFFFFF) {
		case 0x005140B7:
			Debug (0, "osInitialize (haha)");
		break;
		case 0x8003513F:
		case 0x8003704D:
			Debug (0, "osInitialize (Mario64)"); // Must be an older version
		break;
		case 0x3F785BCC:
		case 0x3F787A2F:
			Debug (0, "osInitialize (StarFox)"); // Must be a newer version then M64
		break;
		case 0x6F881E28: // Hydro Thunder
			Debug (0, "osInitialize (Hydro Thunder)");
		break;
	}
}