#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include "WinMain.h"
#include "EmuMain.h"
#include "CpuMain.h"

void *FPRFloatLocation[32];
void *FPRDoubleLocation[32];
int RoundingModel = _RC_NEAR;
extern void (*opCOP1_S_OPS[0x40])();
extern void (*opCOP1_D_OPS[0x40])();
extern void (*opCOP1_W_OPS[0x40])();
extern void (*opCOP1_L_OPS[0x40])();
void doInstr();

//R4K_FPU_union FpuRegs2;

void ResetFPU () {
	int count;
	//memset (FpuRegs2.s, 0, sizeof(R4K_FPU_union));
	memset (FpuRegs.s, 0, sizeof(R4K_FPU_union));
	for (count = 0; count < 32; count ++) {
		FPRFloatLocation[count] = (void *)(&FpuRegs.W[count >> 1][count & 1]);
		FPRDoubleLocation[count] = (void *)(&FpuRegs.DW[count >> 1]);
	}
}

void SetFpuLocations () {
	int count;

	if ((MmuRegs[12] & 0x04000000) == 0) {
		for (count = 0; count < 32; count ++) {
			FPRFloatLocation[count] = (void *)(&FpuRegs.W[count >> 1][count & 1]);
			//FPRDoubleLocation[count] = FPRFloatLocation[count];
			FPRDoubleLocation[count] = (void *)(&FpuRegs.DW[count >> 1]);
		}
	} else {
		for (count = 0; count < 32; count ++) {
			FPRFloatLocation[count] = (void *)(&FpuRegs.W[count][1]);
			//FPRDoubleLocation[count] = FPRFloatLocation[count];
			FPRDoubleLocation[count] = (void *)(&FpuRegs.DW[count]);
		}
	}
}

void opCOP1_BC (void) {
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
	//((void (*)()) R4300i_CoP1_BC[ sop.rt ])();
//	Debug (0, "Missing COP1_BC");
}

void opCOP1_S (void) {
	_controlfp(RoundingModel,_MCW_RC);
	((void (*)()) opCOP1_S_OPS[ sop.func ])();
}

void opCOP1_D (void) {
	_controlfp(RoundingModel,_MCW_RC);
	((void (*)()) opCOP1_D_OPS[ sop.func ])();
}

void opCOP1_W (void) {
	((void (*)()) opCOP1_W_OPS[ sop.func ])();
}

void opCOP1_L (void) {
	((void (*)()) opCOP1_L_OPS[ sop.func ])();
}

void opCOP1_MF (void) {
	CpuRegs[sop.rt] = *(s32 *)FPRFloatLocation[sop.rd];
}

void opCOP1_DMF (void) {
	CpuRegs[sop.rt] = *(s64 *)FPRDoubleLocation[sop.rd];
}

void opCOP1_CF (void) {
	if (sop.rd != 31 && sop.rd != 0) {
		Debug(0, "CFC1 what register are you writing to ?");
		return;
	}
	CpuRegs[sop.rt] = (s32)FpuControl[sop.rd];
}

void opCOP1_MT (void) {
	*(s32 *)FPRFloatLocation[sop.rd] = (s32)CpuRegs[sop.rt];
}

void opCOP1_DMT (void) {
	*(s64 *)FPRDoubleLocation[sop.rd] = (s64)CpuRegs[sop.rt];
}

void opCOP1_CT (void) {
	if (sop.rd == 31) {
		FpuControl[sop.rd] = (s32)CpuRegs[sop.rt];
		switch((FpuControl[sop.rd] & 3)) {
		case 0: RoundingModel = _RC_NEAR; break;
		case 1: RoundingModel = _RC_CHOP; break;
		case 2: RoundingModel = _RC_UP; break;
		case 3: RoundingModel = _RC_DOWN; break;
		}
		return;
	}
	Debug (0, "CTC1 what register are you writing to ?");
}

#ifdef USE_OLD_FPU
void opCOP1z(void) {
#else
void opCOP1(void) {
#endif
	bool CopUnuseableException(u32 addr, u32 copx);
	if (!(MmuRegs[12] & 0x20000000)) {
		pc-=4;
		CopUnuseableException(pc,1);
		return;
	}

	switch (sop.rs) {
		case 0:
			opCOP1_MF ();
		break;
		case 1:
			opCOP1_DMF ();
		break;
		case 2:
			opCOP1_CF ();
		break;
		case 4:
			opCOP1_MT ();
		break;
		case 5:
			opCOP1_DMT ();
		break;
		case 6:
			opCOP1_CT ();
		break;

		case 8:
			opCOP1_BC ();
		break;

		case 16:
			opCOP1_S ();
		break;
		case 17:
			opCOP1_D ();
		break;
		case 20:
			opCOP1_W ();
		break;
		case 21:
			opCOP1_L ();
		break;
		default:
			Debug (1, "Unknown COP1 op: %08X", sop.rs);
	}
}

//************************** COP1: S functions ************************
__inline void Float_RoundToInteger32( int * Dest, float * Source ) {
	_asm {
		mov esi, [Source]
		mov edi, [Dest]
		fld dword ptr [esi]
		fistp dword ptr [edi]
	}
}

__inline void Float_RoundToInteger64( __int64 * Dest, float * Source ) {
	_asm {
		mov esi, [Source]
		mov edi, [Dest]
		fld dword ptr [esi]
		fistp qword ptr [edi]
	}
}

void opCOP1_S_ADD (void) {
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[sop.sa] = (*(float *)FPRFloatLocation[sop.rd] + *(float *)FPRFloatLocation[sop.rt]); 
}

void opCOP1_S_SUB (void) {
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[sop.sa] = (*(float *)FPRFloatLocation[sop.rd] - *(float *)FPRFloatLocation[sop.rt]); 
}

void opCOP1_S_MUL (void) {
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[sop.sa] = (*(float *)FPRFloatLocation[sop.rd] * *(float *)FPRFloatLocation[sop.rt]); 
}

void opCOP1_S_DIV (void) {
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[sop.sa] = (*(float *)FPRFloatLocation[sop.rd] / *(float *)FPRFloatLocation[sop.rt]); 
}

void opCOP1_S_SQRT (void) {
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[sop.sa] = (float)sqrt(*(float *)FPRFloatLocation[sop.rd]);
}

void opCOP1_S_ABS (void) {
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[sop.sa] = (float)fabs(*(float *)FPRFloatLocation[sop.rd]);
}

void opCOP1_S_MOV (void) {
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[sop.sa] = *(float *)FPRFloatLocation[sop.rd];
}

void opCOP1_S_NEG (void) {
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[sop.sa] = (*(float *)FPRFloatLocation[sop.rd] * -1.0f);
}

void opCOP1_S_TRUNC_L (void) {
	_controlfp(_RC_CHOP,_MCW_RC);
	Float_RoundToInteger64(&*(__int64 *)FPRDoubleLocation[sop.sa],&*(float *)FPRFloatLocation[sop.rd]);
}

void opCOP1_S_ROUND_W (void) {
	_controlfp(_RC_NEAR,_MCW_RC);
	Float_RoundToInteger32(&*(int *)FPRFloatLocation[sop.sa],&*(float *)FPRFloatLocation[sop.rd]);
}

void opCOP1_S_TRUNC_W (void) {
	_controlfp(_RC_CHOP,_MCW_RC);
	Float_RoundToInteger32(&*(int *)FPRFloatLocation[sop.sa],&*(float *)FPRFloatLocation[sop.rd]);
}

void opCOP1_S_FLOOR_W (void) {
	_controlfp(_RC_DOWN,_MCW_RC);
	Float_RoundToInteger32(&*(int *)FPRFloatLocation[sop.sa],&*(float *)FPRFloatLocation[sop.rd]);
}

void opCOP1_S_CVT_D (void) {
	_controlfp(RoundingModel,_MCW_RC);
	*(double *)FPRDoubleLocation[sop.sa] = (double)(*(float *)FPRFloatLocation[sop.rd]);
}

void opCOP1_S_CVT_W (void) {
	_controlfp(RoundingModel,_MCW_RC);
	Float_RoundToInteger32(&*(int *)FPRFloatLocation[sop.sa],&*(float *)FPRFloatLocation[sop.rd]);
}

void opCOP1_S_CVT_L (void) {
	_controlfp(RoundingModel,_MCW_RC);
	Float_RoundToInteger64(&*(__int64 *)FPRDoubleLocation[sop.sa],&*(float *)FPRFloatLocation[sop.rd]);
}

void opCOP1_S_CMP (void) {
	int less, equal, unorded, condition;
	float Temp0, Temp1;

	Temp0 = *(float *)FPRFloatLocation[sop.rd];
	Temp1 = *(float *)FPRFloatLocation[sop.rt];

	if (_isnan(Temp0) || _isnan(Temp1)) {
		Debug (0, "Nan ? - %X", sop.func);
		less = FALSE;
		equal = FALSE;
		unorded = TRUE;
		if ((sop.func & 8) != 0) {
			Debug (0, "Signal InvalidOperationException\r\nin opCOP1_S_CMP\r\n%X  %ff\r\n%X  %ff",
				Temp0,Temp0,Temp1,Temp1);
		}
	} else {
		less = Temp0 < Temp1;
		equal = Temp0 == Temp1;
		unorded = FALSE;
	}
	
	condition = ((sop.func & 4) && less) | ((sop.func & 2) && equal) | 
		((sop.func & 1) && unorded);

	if (condition) {
		FpuControl[31] |= 0x00800000;
	} else {
		FpuControl[31] &= ~0x00800000;
	}
	
}

void opCOP1_S_UNK () {
	Debug (1, "Unknown S-Type FPU Opcode: %X", sop.func);
}

//************************** COP1: D functions ************************
__inline void Double_RoundToInteger32( int * Dest, double * Source ) {
	_asm {
		mov esi, [Source]
		mov edi, [Dest]
		fld qword ptr [esi]
		fistp dword ptr [edi]
	}
}

__inline void Double_RoundToInteger64( __int64 * Dest, double * Source ) {
	_asm {
		mov esi, [Source]
		mov edi, [Dest]
		fld qword ptr [esi]
		fistp qword ptr [edi]
	}
}

void opCOP1_D_ADD (void) {
	*(double *)FPRDoubleLocation[sop.sa] = *(double *)FPRDoubleLocation[sop.rd] + *(double *)FPRDoubleLocation[sop.rt]; 
}

void opCOP1_D_SUB (void) {
	*(double *)FPRDoubleLocation[sop.sa] = *(double *)FPRDoubleLocation[sop.rd] - *(double *)FPRDoubleLocation[sop.rt]; 
}

void opCOP1_D_MUL (void) {
	*(double *)FPRDoubleLocation[sop.sa] = *(double *)FPRDoubleLocation[sop.rd] * *(double *)FPRDoubleLocation[sop.rt]; 
}

void opCOP1_D_DIV (void) {
	*(double *)FPRDoubleLocation[sop.sa] = *(double *)FPRDoubleLocation[sop.rd] / *(double *)FPRDoubleLocation[sop.rt]; 
}

void opCOP1_D_SQRT (void) {
	*(double *)FPRDoubleLocation[sop.sa] = (double)sqrt(*(double *)FPRDoubleLocation[sop.rd]); 
}

void opCOP1_D_ABS (void) {
	*(double *)FPRDoubleLocation[sop.sa] = fabs(*(double *)FPRDoubleLocation[sop.rd]);
}

void opCOP1_D_MOV (void) {
	*(__int64 *)FPRDoubleLocation[sop.sa] = *(__int64 *)FPRDoubleLocation[sop.rd];
}

void opCOP1_D_NEG (void) {
	*(double *)FPRDoubleLocation[sop.sa] = (*(double *)FPRDoubleLocation[sop.rd] * -1.0);
}

void opCOP1_D_ROUND_W (void) {
	_controlfp(_RC_NEAR,_MCW_RC);
	Double_RoundToInteger32(&*(int *)FPRFloatLocation[sop.sa],&*(double *)FPRDoubleLocation[sop.rd] );
}

void opCOP1_D_TRUNC_W (void) {// May need to be unsigned destination??
	_controlfp(RC_CHOP,_MCW_RC);
	Double_RoundToInteger32(&*(int *)FPRFloatLocation[sop.sa],&*(double *)FPRDoubleLocation[sop.rd] );
}

void opCOP1_D_CVT_S (void) {
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[sop.sa] = (float)*(double *)FPRDoubleLocation[sop.rd];
}

void opCOP1_D_CVT_W (void) {// May need to be unsigned destination??
	_controlfp(RoundingModel,_MCW_RC);
	Double_RoundToInteger32(&*(int *)FPRFloatLocation[sop.sa],&*(double *)FPRDoubleLocation[sop.rd] );
}

void opCOP1_D_CVT_L (void) { // May need to be unsigned destination??
	_controlfp(RoundingModel,_MCW_RC);
	Double_RoundToInteger64(&*(__int64 *)FPRDoubleLocation[sop.sa],&*(double *)FPRDoubleLocation[sop.rd]);
}

void opCOP1_D_CMP (void) {
	int less, equal, unorded, condition;
	double Temp0, Temp1;

	Temp0 = *(double *)FPRDoubleLocation[sop.rd];
	Temp1 = *(double *)FPRDoubleLocation[sop.rt];

	if (_isnan(Temp0) || _isnan(Temp1)) {
		Debug(0, "Nan ?");
		less = FALSE;
		equal = FALSE;
		unorded = TRUE;
		if ((sop.func & 8) != 0) {
			Debug (0, "Signal InvalidOperationException\nin opCOP1_D_CMP");
		}
	} else {
		less = Temp0 < Temp1;
		equal = Temp0 == Temp1;
		unorded = FALSE;
	}
	
	condition = ((sop.func & 4) && less) | ((sop.func & 2) && equal) | 
		((sop.func & 1) && unorded);

	if (condition) {
		FpuControl[31] |= 0x00800000;
	} else {
		FpuControl[31] &= ~0x00800000;
	}	
}

void opCOP1_D_UNK () {
	Debug (1, "Unknown D-Type FPU Opcode: %X", sop.func);
}

//************************** COP1: W functions ************************
void opCOP1_W_CVT_S (void) {
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[sop.sa] = (float)*(int *)FPRFloatLocation[sop.rd];
}

void opCOP1_W_CVT_D (void) {
	_controlfp(RoundingModel,_MCW_RC);
	*(double *)FPRDoubleLocation[sop.sa] = (double)*(int *)FPRFloatLocation[sop.rd];
}
void opCOP1_W_UNK () {
	Debug (1, "Unknown W-Type FPU Opcode: %X", sop.func);
}

//************************** COP1: L functions ***********************
void opCOP1_L_CVT_S (void) {
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[sop.sa] = (float)*(__int64 *)FPRDoubleLocation[sop.rd];
}

void opCOP1_L_CVT_D (void) {
	_controlfp(RoundingModel,_MCW_RC);
	*(double *)FPRDoubleLocation[sop.sa] = (double)*(__int64 *)FPRDoubleLocation[sop.rd];
}

void opCOP1_L_UNK () {
	Debug (1, "Unknown L-Type FPU Opcode: %X", sop.func);
}

void (*opCOP1_S_OPS[0x40])() =
{
    opCOP1_S_ADD,	opCOP1_S_SUB,		opCOP1_S_MUL,	opCOP1_S_DIV,	opCOP1_S_SQRT,		opCOP1_S_ABS,		opCOP1_S_MOV,	opCOP1_S_NEG,
    opCOP1_S_UNK,	opCOP1_S_TRUNC_L,	opCOP1_S_UNK,	opCOP1_S_UNK,	opCOP1_S_ROUND_W,	opCOP1_S_TRUNC_W,	opCOP1_S_UNK,	opCOP1_S_FLOOR_W,
    opCOP1_S_UNK,	opCOP1_S_UNK,		opCOP1_S_UNK,	opCOP1_S_UNK,	opCOP1_S_UNK,		opCOP1_S_UNK,		opCOP1_S_UNK,	opCOP1_S_UNK,
    opCOP1_S_UNK,	opCOP1_S_UNK,		opCOP1_S_UNK,	opCOP1_S_UNK,	opCOP1_S_UNK,		opCOP1_S_UNK,		opCOP1_S_UNK,	opCOP1_S_UNK,
    opCOP1_S_UNK,	opCOP1_S_CVT_D,		opCOP1_S_UNK,	opCOP1_S_UNK,	opCOP1_S_CVT_W,		opCOP1_S_CVT_L,		opCOP1_S_UNK,	opCOP1_S_UNK,
    opCOP1_S_UNK,	opCOP1_S_UNK,		opCOP1_S_UNK,	opCOP1_S_UNK,	opCOP1_S_UNK,		opCOP1_S_UNK,		opCOP1_S_UNK,	opCOP1_S_UNK,
    opCOP1_S_CMP,	opCOP1_S_CMP,		opCOP1_S_CMP,	opCOP1_S_CMP,	opCOP1_S_CMP,		opCOP1_S_CMP,		opCOP1_S_CMP,	opCOP1_S_CMP,
    opCOP1_S_CMP,	opCOP1_S_CMP,		opCOP1_S_CMP,	opCOP1_S_CMP,	opCOP1_S_CMP,		opCOP1_S_CMP,		opCOP1_S_CMP,	opCOP1_S_CMP
};

void (*opCOP1_D_OPS[0x40])() =
{
    opCOP1_D_ADD,	opCOP1_D_SUB,	opCOP1_D_MUL,	opCOP1_D_DIV,	opCOP1_D_SQRT,		opCOP1_D_ABS,		opCOP1_D_MOV,	opCOP1_D_NEG,
    opCOP1_D_UNK,	opCOP1_D_UNK,	opCOP1_D_UNK,	opCOP1_D_UNK,	opCOP1_D_ROUND_W,	opCOP1_D_TRUNC_W,	opCOP1_D_UNK,	opCOP1_D_UNK,
    opCOP1_D_UNK,	opCOP1_D_UNK,	opCOP1_D_UNK,	opCOP1_D_UNK,	opCOP1_D_UNK,		opCOP1_D_UNK,		opCOP1_D_UNK,	opCOP1_D_UNK,
    opCOP1_D_UNK,	opCOP1_D_UNK,	opCOP1_D_UNK,	opCOP1_D_UNK,	opCOP1_D_UNK,		opCOP1_D_UNK,		opCOP1_D_UNK,	opCOP1_D_UNK,
    opCOP1_D_CVT_S,	opCOP1_D_UNK,	opCOP1_D_UNK,	opCOP1_D_UNK,	opCOP1_D_CVT_W,		opCOP1_D_CVT_L,		opCOP1_D_UNK,	opCOP1_D_UNK,
    opCOP1_D_UNK,	opCOP1_D_UNK,	opCOP1_D_UNK,	opCOP1_D_UNK,	opCOP1_D_UNK,		opCOP1_D_UNK,		opCOP1_D_UNK,	opCOP1_D_UNK,
    opCOP1_D_CMP,	opCOP1_D_CMP,	opCOP1_D_CMP,	opCOP1_D_CMP,	opCOP1_D_CMP,		opCOP1_D_CMP,		opCOP1_D_CMP,	opCOP1_D_CMP,
    opCOP1_D_CMP,	opCOP1_D_CMP,	opCOP1_D_CMP,	opCOP1_D_CMP,	opCOP1_D_CMP,		opCOP1_D_CMP,		opCOP1_D_CMP,	opCOP1_D_CMP
};

void (*opCOP1_W_OPS[0x40])() =
{
    opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,		opCOP1_W_UNK,		opCOP1_W_UNK,	opCOP1_W_UNK,
    opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,		opCOP1_W_UNK,		opCOP1_W_UNK,	opCOP1_W_UNK,
    opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,		opCOP1_W_UNK,		opCOP1_W_UNK,	opCOP1_W_UNK,
    opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,		opCOP1_W_UNK,		opCOP1_W_UNK,	opCOP1_W_UNK,
    opCOP1_W_CVT_S,	opCOP1_W_CVT_D,	opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,		opCOP1_W_UNK,		opCOP1_W_UNK,	opCOP1_W_UNK,
    opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,		opCOP1_W_UNK,		opCOP1_W_UNK,	opCOP1_W_UNK,
    opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,		opCOP1_W_UNK,		opCOP1_W_UNK,	opCOP1_W_UNK,
    opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,	opCOP1_W_UNK,		opCOP1_W_UNK,		opCOP1_W_UNK,	opCOP1_W_UNK,
};

void (*opCOP1_L_OPS[0x40])() =
{
    opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,		opCOP1_L_UNK,		opCOP1_L_UNK,	opCOP1_L_UNK,
    opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,		opCOP1_L_UNK,		opCOP1_L_UNK,	opCOP1_L_UNK,
    opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,		opCOP1_L_UNK,		opCOP1_L_UNK,	opCOP1_L_UNK,
    opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,		opCOP1_L_UNK,		opCOP1_L_UNK,	opCOP1_L_UNK,
    opCOP1_L_CVT_S,	opCOP1_L_CVT_D,	opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,		opCOP1_L_UNK,		opCOP1_L_UNK,	opCOP1_L_UNK,
    opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,		opCOP1_L_UNK,		opCOP1_L_UNK,	opCOP1_L_UNK,
    opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,		opCOP1_L_UNK,		opCOP1_L_UNK,	opCOP1_L_UNK,
    opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,	opCOP1_L_UNK,		opCOP1_L_UNK,		opCOP1_L_UNK,	opCOP1_L_UNK,
};
/*
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

*/




/*
//************************* COP1: BC1 functions ***********************
void opCOP1_BCF (void) {
	TEST_COP1_USABLE_EXCEPTION
	NextInstruction = DELAY_SLOT;
	if ((FpuControl[31] & 0x00800000) == 0) {
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
	} else {
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void opCOP1_BCT (void) {
	TEST_COP1_USABLE_EXCEPTION
	NextInstruction = DELAY_SLOT;
	if ((FpuControl[31] & 0x00800000) != 0) {
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
	} else {
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void opCOP1_BCFL (void) {
	TEST_COP1_USABLE_EXCEPTION
	if ((FpuControl[31] & 0x00800000) == 0) {
		NextInstruction = DELAY_SLOT;
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
	} else {
		NextInstruction = JUMP;
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void opCOP1_BCTL (void) {
	TEST_COP1_USABLE_EXCEPTION
	if ((FpuControl[31] & 0x00800000) != 0) {
		NextInstruction = DELAY_SLOT;
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
	} else {
		NextInstruction = JUMP;
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

*/