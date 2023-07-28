/**************************************************************************
 *                                                                        *
 *               Copyright (C) 2000, Eclipse Productions                  *
 *                                                                        *
 *  Offical Source code of the Apollo Project.  DO NOT DISTRIBUTE!  Any   *
 *  unauthorized distribution of these files in any way, shape, or form   *
 *  is a direct infringement on the copyrights entitled to Eclipse        *
 *  Productions and you will be prosecuted to the fullest extent of       *
 *  the law.  Have a nice day... ^_^                                      *
 *                                                                        *
 *************************************************************************/

/**************************************************************************
 *
 *  Revision History:
 *		Date:		Comment:
 *	-----------------------------------------------------------------------
 *		03-29-00	Initial Version (Andrew)
 *		06-04-00	Fixed CVTs21 and CVTd21 (Andrew)
 *
 **************************************************************************/

/**************************************************************************
 *
 *  Notes:
 *	
 *	Sections of the FPU not correctly emulated: -KAH
 *	-All sections of the FC[31] except the rounding mode - And not initially filled.
 *  -FC[0] not filled with any data--should be revision register
 *  -real->integer enables OVERFLOW and DIV/0. Why?
 *  -The conditional opcodes for integer will currently fail with as a NI (not implemented)
 *  
 *  
 *
 **************************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include "WinMain.h"
#include "EmuMain.h"
#include "CpuMain.h"
#include "resource.h"

#define CPU_ERROR(x,y) Debug(0,"%s %08X",x,y)

void doInstr();

#define START_TRUNICATE_FPU() \
	u32 RCold = _control87(_RC_CHOP, _MCW_RC);
#define END_TRUNICATE_FPU() \
	_control87(RCold, _MCW_RC);
#define START_FLOOR_FPU() \
	u32 RCold = _control87(_RC_DOWN, _MCW_RC);
#define END_FLOOR_FPU() \
	_control87(RCold, _MCW_RC);
#define START_CEILING_FPU() \
	u32 RCold = _control87(_RC_UP, _MCW_RC);
#define END_CEILING_FPU() \
	_control87(RCold, _MCW_RC);
#define START_ROUND_FPU() \
	//u32 RCold = _control87(_RC_NEAR, _MCW_RC);
#define END_ROUND_FPU() \
	//_control87(RCold, _MCW_RC);

#define FC_UNSET() \
	FpuControl[31] &= 0xff7fffff;
	//__asm and FpuControl[31*4], 0xff7fffff;

#define FC_SET(x) \
	if ((x)) FpuControl[31] |= 0x00800000;

void opNI();

extern void (*FpuCommands16[0x40])();
extern void (*FpuCommands17[0x40])();
extern void (*FpuCommands20[0x40])();
extern void (*FpuCommands21[0x40])();

extern R4K_FPU_union FpuRegs;
#ifdef USE_OLD_FPU
void opCOP1(void) {
#else
void opCOP1z(void) {
#endif
//	static cnt = 0;
//	cnt ++;
/*	sop.rd = sop.rd;
	sop.rt = sop.rt;
	sop.sa = sop.sa;
*/
	//Debug (0, "COP1");
	bool CopUnuseableException(u32 addr, u32 copx);
	if (!(MmuRegs[12] & 0x20000000)) {
		pc-=4;
		CopUnuseableException(pc,1);
		return;
	}

	switch (sop.rs) {
	case 0: // MFC1
		CpuRegs[sop.rt] = (s32)FpuRegs.w[sop.rd];
		break;
	case 1: // DMFC1
		CpuRegs[sop.rt] = FpuRegs.l[sop.rd/2];
		break;
	case 2: // CFC1
		CpuRegs[sop.rt] = (s32)FpuControl[sop.rd];
		break;
	case 4: // MTC1
		FpuRegs.w[sop.rd] = (s32)CpuRegs[sop.rt];
		break;
	case 5: // DMTC1
		FpuRegs.l[sop.rd/2] = CpuRegs[sop.rt];
		break;
	case 6: // CTC1
		FpuControl[sop.rd] = (u32)CpuRegs[sop.rt];
		/*switch (FpuControl[31] & 3) {
		case 0:
			_control87(_RC_NEAR, _MCW_RC);
			break;
		case 1:
			_control87(_RC_CHOP, _MCW_RC);
			break;
		case 2:
			_control87(_RC_UP, _MCW_RC);
			break;
		default:
			_control87(_RC_DOWN, _MCW_RC);
			break;
		}*/
		break;
	case 8:
		switch (sop.rt & 3){
		case 0x0://BC1F
			if (!(FpuControl[31] & 0x00800000)) {
				u32 target = pc + ((s16(opcode)) << 2);
				doInstr();
				pc = target;
			}
			break;
		case 0x1://BC1T
			if (FpuControl[31] & 0x00800000) {
				u32 target = pc + ((s16(opcode)) << 2);
				doInstr();
				pc = target;
			}
			break;
		case 0x2://BC1FL
			if (!(FpuControl[31] & 0x00800000)) {
				u32 target = pc + ((s16(opcode)) << 2);
				doInstr();
				pc = target;
			} else {
				pc += 4;
			}
			break;
		case 0x3://BC1TL
			if (FpuControl[31] & 0x00800000) {
				u32 target = pc + ((s16(opcode)) << 2);
				doInstr();
				pc = target;
			} else {
				pc += 4;
			}
		default:
			break;
		}			
		break;
	case 16:
		FpuCommands16[sop.func]();
		break;
	case 17:
		FpuCommands17[sop.func]();
		break;
	case 20:
		FpuCommands20[sop.func]();
		break;
	case 21:
		FpuCommands21[sop.func]();
		break;
	default:
		CPU_ERROR("cop1 (fpu)",opcode);
		break;
	}
}

void fpADD16(void){
	FpuRegs.s[sop.sa] = FpuRegs.s[sop.rd] + FpuRegs.s[sop.rt];
}

void fpADD17(void){
	FpuRegs.d[sop.sa/2] = FpuRegs.d[sop.rd/2] + FpuRegs.d[sop.rt/2];
}

void fpADD20(void){
	FpuRegs.w[sop.sa] = FpuRegs.w[sop.rd] + FpuRegs.w[sop.rt];
}

void fpADD21(void){
	FpuRegs.l[sop.sa/2] = FpuRegs.l[sop.rd/2] + FpuRegs.l[sop.rt/2];
}

void fpSUB16(void){
	FpuRegs.s[sop.sa] = FpuRegs.s[sop.rd] - FpuRegs.s[sop.rt];
}

void fpSUB17(void){
	FpuRegs.d[sop.sa/2] = FpuRegs.d[sop.rd/2] - FpuRegs.d[sop.rt/2];
}

void fpSUB20(void){
	FpuRegs.w[sop.sa] = FpuRegs.w[sop.rd] - FpuRegs.w[sop.rt];
}

void fpSUB21(void){
	FpuRegs.l[sop.sa/2] = FpuRegs.l[sop.rd/2] - FpuRegs.l[sop.rt/2];
}

void fpMUL16(void){
	FpuRegs.s[sop.sa] = FpuRegs.s[sop.rd] * FpuRegs.s[sop.rt];
}

void fpMUL17(void){
	FpuRegs.d[sop.sa/2] = FpuRegs.d[sop.rd/2] * FpuRegs.d[sop.rt/2];
}

void fpMUL20(void){
	FpuRegs.w[sop.sa] = FpuRegs.w[sop.rd] * FpuRegs.w[sop.rt];
}

void fpMUL21(void){
	FpuRegs.l[sop.sa/2] = FpuRegs.l[sop.rd/2] * FpuRegs.l[sop.rt/2];
}

void fpDIV16(void){
	if (FpuRegs.s[sop.rt]==0) FpuRegs.s[sop.sa] = 0;
	else FpuRegs.s[sop.sa] = FpuRegs.s[sop.rd] / FpuRegs.s[sop.rt];
}

void fpDIV17(void){
	if (FpuRegs.d[sop.rt/2]==0) FpuRegs.d[sop.sa/2] = 0;
	else FpuRegs.d[sop.sa/2] = FpuRegs.d[sop.rd/2] / FpuRegs.d[sop.rt/2];
}

void fpDIV20(void){
	if (FpuRegs.w[sop.rt]==0) FpuRegs.w[sop.sa] = 0;
	else FpuRegs.w[sop.sa] = FpuRegs.w[sop.rd] / FpuRegs.w[sop.rt];
}

void fpDIV21(void){
	if (FpuRegs.l[sop.rt/2]==0) FpuRegs.l[sop.sa/2] = 0;
	else FpuRegs.l[sop.sa/2] = FpuRegs.l[sop.rd/2] / FpuRegs.l[sop.rt/2];
}

void fpSRT16(void){
	FpuRegs.s[sop.sa] = (float)sqrt(FpuRegs.s[sop.rd]);
}

void fpSRT17(void){
	FpuRegs.d[sop.sa/2] = sqrt(FpuRegs.d[sop.rd/2]);
}

void fpSRT20(void){
	FpuRegs.w[sop.sa] = (long)sqrt((float)FpuRegs.w[sop.rd]);
}

void fpSRT21(void){
	FpuRegs.l[sop.sa/2] = (s64)sqrt((double)FpuRegs.l[sop.rd/2]);
}

void fpABS16(void){
	FpuRegs.s[sop.sa] = (float)fabs(FpuRegs.s[sop.rd]);
}

void fpABS17(void){
	FpuRegs.d[sop.sa/2] = fabs(FpuRegs.d[sop.rd/2]);
}

void fpABS20(void){
	FpuRegs.w[sop.sa] = (s32)fabs((float)FpuRegs.w[sop.rd]);
}

void fpABS21(void){
	FpuRegs.l[sop.sa/2] = (s64)fabs((double)FpuRegs.l[sop.rd/2]);
}

void fpMOV16(void){
	FpuRegs.s[sop.sa] = FpuRegs.s[sop.rd];
}

void fpMOV17(void){
	FpuRegs.d[sop.sa/2] = FpuRegs.d[sop.rd/2];
}

void fpNEG16(void){
	FpuRegs.s[sop.sa] = -FpuRegs.s[sop.rd];
}

void fpNEG17(void){
	FpuRegs.d[sop.sa/2] = -FpuRegs.d[sop.rd/2];
}

void fpNEG20(void){
	FpuRegs.w[sop.sa] = -FpuRegs.w[sop.rd];
}

void fpNEG21(void){
	FpuRegs.l[sop.sa/2] = -FpuRegs.l[sop.rd/2];
}

void fpTRCl16(void){
	FpuRegs.l[sop.sa/2] = (s64)FpuRegs.s[sop.rd];
}

void fpTRCl17(void){
	FpuRegs.l[sop.sa/2] = (s64)FpuRegs.d[sop.rd/2];
}

void fpCELl16(void){
	START_CEILING_FPU();
	FpuRegs.l[sop.sa/2] = (s64)FpuRegs.s[sop.rd];
	END_CEILING_FPU();
	Debug (0, "fpCELl16");
}

void fpCELl17(void){
	START_CEILING_FPU();
	FpuRegs.l[sop.sa/2] = (s64)FpuRegs.d[sop.rd/2];
	END_CEILING_FPU();
	Debug (0, "fpCELl17");
}

void fpRNDw16(void){
	START_ROUND_FPU();
	FpuRegs.w[sop.sa] = (s32)FpuRegs.s[sop.rd];
	END_ROUND_FPU();
}

void fpRNDw17(void){
	START_ROUND_FPU();
	FpuRegs.w[sop.sa] = (s32)FpuRegs.d[sop.rd/2];
	END_ROUND_FPU();
}

void fpRNDl16(void){
	START_ROUND_FPU();
	FpuRegs.l[sop.sa/2] = (s64)FpuRegs.s[sop.rd];
	END_ROUND_FPU();
	Debug (0, "fpRNDl16");
}

void fpRNDl17(void){
	START_ROUND_FPU();
	FpuRegs.l[sop.sa/2] = (s64)FpuRegs.d[sop.rd/2];
	END_ROUND_FPU();
	Debug (0, "fpRNDl17");
}


void fpFLRw16(void){
	START_FLOOR_FPU();
	FpuRegs.w[sop.sa] = (s32)FpuRegs.s[sop.rd];
	END_FLOOR_FPU();
//	Debug (0, "fpFLRw16");
}

void fpFLRw17(void){
	START_FLOOR_FPU();
	FpuRegs.w[sop.sa] = (s32)FpuRegs.d[sop.rd/2];
	END_FLOOR_FPU();
	Debug (0, "fpFLRw17");
}

void fpFLRl16(void){
	START_FLOOR_FPU();
	FpuRegs.l[sop.sa/2] = (s64)FpuRegs.s[sop.rd];
	END_FLOOR_FPU();
	Debug (0, "fpFLRl16");
}

void fpFLRl17(void){
	START_FLOOR_FPU();
	FpuRegs.l[sop.sa/2] = (s64)FpuRegs.d[sop.rd/2];
	END_FLOOR_FPU();
	Debug (0, "fpFLRl17");
}

void fpTRCw16(void){
	//START_TRUNICATE_FPU();
	FpuRegs.w[sop.sa] = (s32)FpuRegs.s[sop.rd];
	//END_TRUNICATE_FPU();
}

void fpTRCw17(void){
	//START_TRUNICATE_FPU();
	FpuRegs.w[sop.sa] = (s32)FpuRegs.d[sop.rd/2];
	//END_TRUNICATE_FPU();
}

void fpCELw16(void){
	START_CEILING_FPU();
	FpuRegs.w[sop.sa] = (s32)FpuRegs.s[sop.rd];
	END_CEILING_FPU();
}

void fpCELw17(void){
	START_CEILING_FPU();
	FpuRegs.w[sop.sa] = (s32)FpuRegs.d[sop.rd/2];
	END_CEILING_FPU();
}

void fpCVTs17(void){
	FpuRegs.s[sop.sa] = (float)FpuRegs.d[sop.rd/2];
}

void fpCVTs20(void){
	FpuRegs.s[sop.sa] = (float)FpuRegs.w[sop.rd];
}

void fpCVTs21(void){
	FpuRegs.s[sop.sa] = (float)FpuRegs.l[sop.rd/2];
}

void fpCVTd16(void){
	FpuRegs.d[sop.sa/2] = (double)FpuRegs.s[sop.rd];
}

void fpCVTd20(void){
	FpuRegs.d[sop.sa/2] = (double)FpuRegs.w[sop.rd];
}

void fpCVTd21(void){
	FpuRegs.d[sop.sa/2] = (double)FpuRegs.l[sop.rd/2];
}

void fpCVTw16(void){
	FpuRegs.w[sop.sa] = (s32)FpuRegs.s[sop.rd];
}

void fpCVTw17(void){
	FpuRegs.w[sop.sa] = (s32)FpuRegs.d[sop.rd/2];
}

void fpCVTl16(void){
	FpuRegs.l[sop.sa/2] = (s64)FpuRegs.s[sop.rd];
}

void fpCVTl17(void){
	FpuRegs.l[sop.sa/2] = (s64)FpuRegs.d[sop.rd/2];
}

void fpC_F16(void){
	//0000
	FC_UNSET();
}

void fpC_F17(void){
	//0000
	FC_UNSET();
}

void fpC_UN16(void){
	//0001
	FC_UNSET();
	FC_SET(_isnan(FpuRegs.s[sop.rd]) || _isnan(FpuRegs.s[sop.rt])); 
}

void fpC_UN17(void){
	//0001
	FC_UNSET();
	FC_SET(_isnan(FpuRegs.d[sop.rd/2]) || _isnan(FpuRegs.d[sop.rt/2])); 
}

void fpC_EQ16(void){
	//0010
	FC_UNSET();
	FC_SET(FpuRegs.s[sop.rd]==FpuRegs.s[sop.rt]); 
}

void fpC_EQ17(void){
	//0010
	FC_UNSET();
	FC_SET(FpuRegs.d[sop.rd/2]==FpuRegs.d[sop.rt/2]); 
}

void fpC_UEQ16(void){
	//0011
	FC_UNSET();
	FC_SET(_isnan(FpuRegs.s[sop.rd]) || _isnan(FpuRegs.s[sop.rt])); 
	FC_SET(FpuRegs.s[sop.rd]==FpuRegs.s[sop.rt]); 
}

void fpC_UEQ17(void){
	//0011
	FC_UNSET();
	FC_SET(_isnan(FpuRegs.d[sop.rd/2]) || _isnan(FpuRegs.d[sop.rt/2])); 
	FC_SET(FpuRegs.d[sop.rd/2]==FpuRegs.d[sop.rt/2]); 
}

void fpC_OLT16(void){
	//0100
	FC_UNSET();
	FC_SET(FpuRegs.s[sop.rd]<FpuRegs.s[sop.rt]); 
}

void fpC_OLT17(void){
	//0100
	FC_UNSET();
	FC_SET(FpuRegs.d[sop.rd/2]<FpuRegs.d[sop.rt/2]); 
}

void fpC_ULT16(void){
	//0101
	FC_UNSET();
	FC_SET(_isnan(FpuRegs.s[sop.rd]) || _isnan(FpuRegs.s[sop.rt])); 
	FC_SET(FpuRegs.s[sop.rd]<FpuRegs.s[sop.rt]); 
}

void fpC_ULT17(void){
	//0101
	FC_UNSET();
	FC_SET(_isnan(FpuRegs.d[sop.rd/2]) || _isnan(FpuRegs.d[sop.rt/2])); 
	FC_SET(FpuRegs.d[sop.rd/2]<FpuRegs.d[sop.rt/2]); 
}

void fpC_OLE16(void){
	//0110
	FC_UNSET();
	FC_SET(FpuRegs.s[sop.rd] <= FpuRegs.s[sop.rt]); 
}

void fpC_OLE17(void){
	//0110
	FC_UNSET();
	FC_SET(FpuRegs.d[sop.rd/2] <= FpuRegs.d[sop.rt/2]); 
}

void fpC_ULE16(void){
	//0111
	FC_UNSET();
	FC_SET(_isnan(FpuRegs.s[sop.rd]) || _isnan(FpuRegs.s[sop.rt])); 
	FC_SET(FpuRegs.s[sop.rd]<=FpuRegs.s[sop.rt]); 
}

void fpC_ULE17(void){
	//0111
	FC_UNSET();
	FC_SET(_isnan(FpuRegs.d[sop.rd/2]) || _isnan(FpuRegs.d[sop.rt/2])); 
	FC_SET(FpuRegs.d[sop.rd/2]<=FpuRegs.d[sop.rt/2]); 
}

void fpC_SF16(void){
	//1000
	FC_UNSET();
	if (_isnan(FpuRegs.s[sop.rd]) || _isnan(FpuRegs.s[sop.rt])) { CPU_ERROR(L_STR(IDS_NAN_FOUND),opcode); }
	__asm int 3;
}

void fpC_SF17(void){
	//1000
	FC_UNSET();
	if (_isnan(FpuRegs.d[sop.rd/2]) || _isnan(FpuRegs.d[sop.rt/2])) { CPU_ERROR(L_STR(IDS_NAN_FOUND),opcode); }
	__asm int 3;
}

void fpC_NGLE16(void){
	//1001
	FC_UNSET();
	if (_isnan(FpuRegs.s[sop.rd]) || _isnan(FpuRegs.s[sop.rt])) { FC_SET(1); CPU_ERROR(L_STR(IDS_NAN_FOUND),opcode); }
	__asm int 3;
}

void fpC_NGLE17(void){
	//1001
	FC_UNSET();
	if (_isnan(FpuRegs.d[sop.rd/2]) || _isnan(FpuRegs.d[sop.rt/2])) { FC_SET(1); CPU_ERROR(L_STR(IDS_NAN_FOUND),opcode); }
	__asm int 3;
}

void fpC_SEQ16(void){
	//1010
	FC_UNSET();
	if (_isnan(FpuRegs.s[sop.rd]) || _isnan(FpuRegs.s[sop.rt])) { CPU_ERROR(L_STR(IDS_NAN_FOUND),opcode); }
	FC_SET(FpuRegs.s[sop.rd]==FpuRegs.s[sop.rt]); 
}

void fpC_SEQ17(void){
	//1010
	FC_UNSET();
	if (_isnan(FpuRegs.d[sop.rd/2]) || _isnan(FpuRegs.d[sop.rt/2])) { CPU_ERROR(L_STR(IDS_NAN_FOUND),opcode); }
	FC_SET(FpuRegs.d[sop.rd/2]==FpuRegs.d[sop.rt/2]); 
}

void fpC_NGL16(void){
	//1011
	FC_UNSET();
	if (_isnan(FpuRegs.s[sop.rd]) || _isnan(FpuRegs.s[sop.rt])) { FC_SET(1); CPU_ERROR(L_STR(IDS_NAN_FOUND),opcode); }
	FC_SET(FpuRegs.s[sop.rd]==FpuRegs.s[sop.rt]); 
}

void fpC_NGL17(void){
	//1011
	FC_UNSET();
	if (_isnan(FpuRegs.d[sop.rd/2]) || _isnan(FpuRegs.d[sop.rt/2])) { FC_SET(1); CPU_ERROR(L_STR(IDS_NAN_FOUND),opcode); }
	FC_SET(FpuRegs.d[sop.rd/2]==FpuRegs.d[sop.rt/2]); 
}

void fpC_LT16(void){
	//1100
	FC_UNSET();
	if (_isnan(FpuRegs.s[sop.rd]) || _isnan(FpuRegs.s[sop.rt])) { CPU_ERROR(L_STR(IDS_NAN_FOUND),opcode); }
	FC_SET(FpuRegs.s[sop.rd]<FpuRegs.s[sop.rt]); 
}

void fpC_LT17(void){
	//1100
	FC_UNSET();
	if (_isnan(FpuRegs.d[sop.rd/2]) || _isnan(FpuRegs.d[sop.rt/2])) { CPU_ERROR(L_STR(IDS_NAN_FOUND),opcode); }
	FC_SET(FpuRegs.d[sop.rd/2]<FpuRegs.d[sop.rt/2]); 
}

void fpC_NGE16(void){
	//1101
	FC_UNSET();
	if (_isnan(FpuRegs.s[sop.rd]) || _isnan(FpuRegs.s[sop.rt])) { FC_SET(1); CPU_ERROR(L_STR(IDS_NAN_FOUND),opcode); }
	FC_SET(FpuRegs.s[sop.rd]<FpuRegs.s[sop.rt]); 
}

void fpC_NGE17(void){
	//1101
	FC_UNSET();
	if (_isnan(FpuRegs.d[sop.rd/2]) || _isnan(FpuRegs.d[sop.rt/2])) { FC_SET(1); CPU_ERROR(L_STR(IDS_NAN_FOUND),opcode); }
	FC_SET(FpuRegs.d[sop.rd/2]<FpuRegs.d[sop.rt/2]); 
}

void fpC_LE16(void){
	//1110
	FC_UNSET();
	if (_isnan(FpuRegs.s[sop.rd]) || _isnan(FpuRegs.s[sop.rt])) { CPU_ERROR(L_STR(IDS_NAN_FOUND),opcode); }
	FC_SET(FpuRegs.s[sop.rd]<=FpuRegs.s[sop.rt]); 
}

void fpC_LE17(void){
	//1110
	FC_UNSET();
	if (_isnan(FpuRegs.d[sop.rd/2]) || _isnan(FpuRegs.d[sop.rt/2])) { CPU_ERROR(L_STR(IDS_NAN_FOUND),opcode); }
	FC_SET(FpuRegs.d[sop.rd/2]<=FpuRegs.d[sop.rt/2]); 
}

void fpC_NGT16(void){
	//1111
	FC_UNSET();
	if (_isnan(FpuRegs.s[sop.rd]) || _isnan(FpuRegs.s[sop.rt])) { FC_SET(1); CPU_ERROR(L_STR(IDS_NAN_FOUND),opcode); }
	FC_SET(FpuRegs.s[sop.rd]<=FpuRegs.s[sop.rt]); 
}

void fpC_NGT17(void){
	//1111
	FC_UNSET();
	if (_isnan(FpuRegs.d[sop.rd/2]) || _isnan(FpuRegs.d[sop.rt/2])) { FC_SET(1); CPU_ERROR(L_STR(IDS_NAN_FOUND),opcode); }
	FC_SET(FpuRegs.d[sop.rd/2]<=FpuRegs.d[sop.rt/2]); 
}

void (*FpuCommands16[0x40])() =
{
    fpADD16,	fpSUB16,	fpMUL16,	fpDIV16,	fpSRT16,	fpABS16,	fpMOV16,	fpNEG16,
    fpRNDl16,	fpTRCl16,	fpCELl16,	fpFLRl16,	fpRNDw16,	fpTRCw16,	fpCELw16,	fpFLRw16,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    opNI,		fpCVTd16,	opNI,		opNI,		fpCVTw16,	fpCVTl16,	opNI,		opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    fpC_F16,	fpC_UN16,	fpC_EQ16,	fpC_UEQ16,	fpC_OLT16,	fpC_ULT16,	fpC_OLE16,	fpC_ULE16,
    fpC_SF16,	fpC_NGLE16,	fpC_SEQ16,	fpC_NGL16,	fpC_LT16,	fpC_NGE16,	fpC_LE16,	fpC_NGT16
};

void (*FpuCommands17[0x40])() =
{
    fpADD17,	fpSUB17,	fpMUL17,	fpDIV17,	fpSRT17,	fpABS17,	fpMOV17,	fpNEG17,
    fpRNDl17,	fpTRCl17,	fpCELl17,	fpFLRl17,	fpRNDw17,	fpTRCw17,	fpCELw17,	fpFLRw17,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    fpCVTs17,	opNI,		opNI,		opNI,		fpCVTw17,	fpCVTl17,	opNI,		opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    fpC_F17,	fpC_UN17,	fpC_EQ17,	fpC_UEQ17,	fpC_OLT17,	fpC_ULT17,	fpC_OLE17,	fpC_ULE17,
    fpC_SF17,	fpC_NGLE17,	fpC_SEQ17,	fpC_NGL17,	fpC_LT17,	fpC_NGE17,	fpC_LE17,	fpC_NGT17
};

void (*FpuCommands20[0x40])() =
{
    fpADD20,	fpSUB20,	fpMUL20,	fpDIV20,	fpSRT20,	fpABS20,	opNI,		fpNEG20,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    fpCVTs20,	fpCVTd20,	opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI
};

void (*FpuCommands21[0x40])() =
{
    fpADD21,	fpSUB21,	fpMUL21,	fpDIV21,	fpSRT21,	fpABS21,	opNI,		fpNEG21,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    fpCVTs21,	fpCVTd21,	opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI,		opNI
};