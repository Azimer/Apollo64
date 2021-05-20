#ifndef THISISDISABLED
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "debug.h"

#include "rsp_mnemonic.h"
#include "rsp_registers.h"
#include "rsp.h"

extern char *pRDRAM;
extern char *pDMEM;
extern char *pIMEM;

extern void Dump_RSP_UCode();

//#define TEST_MODE // Enables logging

#ifdef TEST_MODE
#	define LogMe fprintf
	FILE *dfile=NULL;
#else
#	define LogMe // Disabled 
	int	dfile; // Just a generic type to make the compiler happy ;/
#endif
/*
#	define _assert_(_a_)	if (!(_a_)){\
								char szError [512];\
								sprintf(szError,"PC = %08X\n\nError localized at...\n\n  Line:\t %d\n  File:\t %s\n  Time:\t %s\n\nIgnore and continue?",sp_reg_pc, __LINE__,__FILE__,__TIMESTAMP__);\
								MessageBox (NULL, szError, "Assert", MB_OK);	\
								__asm int 3;	\
								rsp_reg.halt = 1;	\
							}
								//if(MessageBox(NULL,szError,"*** Assertion Report ***",MB_YESNO|MB_ICONERROR)==IDNO)__asm{int 3};\
*/

//////////////////////////////////////////////////////////////////////////////
// Creates a MessageBox()													//
//////////////////////////////////////////////////////////////////////////////
void __cdecl MBox(char *debug, ...)
{ 
	va_list argptr;
	char	text[1024];

	if ((debug == NULL) || (strlen(debug) > sizeof(text)))
	{
		strcpy(text, "(String missing or too large)");
	}

	va_start(argptr, debug);
	vsprintf(text, debug, argptr);
	va_end(argptr);

	MessageBox(NULL, text, "MSG", MB_OK|MB_ICONSTOP);	
}



BYTE *rsp_dmem;
BYTE *rsp_imem;
DWORD sp_reg_pc;

t_rsp_reg rsp_reg;

#include "rsp_helper.h"

_u8 __RT, __RD, __RS, __SA;
_s32 __I;

#define FIXED2DOUBLE(x) (((double)x) / 65536.0)
#define DOUBLE2FIXED(x) ((_u32)(x * 65536.0 + 0.5))
#define FIXED2FLOAT(x) (((float)x) / 65536.0)
#define FLOAT2FIXED(x) ((_u32)(x * 65536.0 + 0.5))

//_____________________________________________________________________
//
//
_u32 Load32_IMEM(_u32 offset)
{
	//_assert_ ((offset & 0xffffff) == ((offset & 0xfff)+0x1000));
	//return (*(_u32 *)&rsp_imem[offset & 0xfff]);
	return (*(_u32 *)&rsp_dmem[offset & 0x1fff]);
}



_u8 Load8_DMEM(_u32 offset)
{
	//_assert_ ((offset & 0xffffff) == (offset & 0xfff));
	offset ^= 0x03;
	return (*(_u8 *)&rsp_dmem[offset & 0xfff]);
}

_u16 Load16_DMEM(_u32 offset)
{
	_u16 value;
	//_assert_ ((offset & 0xffffff) == (offset & 0xfff));
	if (offset & 0x1) {
		value = Load8_DMEM (((offset+1)&0xfff));
		value |= (Load8_DMEM (((offset)&0xfff)) << 8);
		return value;
	} else {
		offset ^= 0x02;
		return (*(_u16 *)&rsp_dmem[offset & 0xfff]);
	}
}

_u32 Load32_DMEM(_u32 offset)
{
	_u32 value;
	int i;
	//_assert_ ((offset & 0xffffff) == (offset & 0xfff));
	LogMe (dfile, "offset = %08X\n", offset);
	if (offset & 0x3) {
		value = 0;
		for (i = 0; i < 4; i++) {
			value |= (Load8_DMEM((offset+i)&0xfff) << ((3-i)<<3));
		}
		return value;
	} else {
		return (*(_u32 *)&rsp_dmem[offset & 0xfff]);
	}
}

_u64 Load64_DMEM(_u32 offset)
{
	_u64 value;
	_u64 value2;
	int i;
	//_assert_ ((offset & 0xffffff) == (offset & 0xfff));
	LogMe (dfile, "offset = %08X\n", offset);
	if (offset & 0x3) {
		value = value2 = 0;
		for (i = 0; i < 4; i++) {
			value |= (Load8_DMEM((offset+i)&0xfff) << ((3-i)<<3));
		}
		for (i = 0; i < 4; i++) {
			value2 |= (Load8_DMEM((offset+i+4)&0xfff) << ((3-i)<<3));
		}
		value = (_u32)value;
		value2 = (_u32)value2;
		LogMe (dfile, "value = %08X%08X\n", (_u32)value, (_u32)value2);
		return ((value2) | (value << 32));
	} else {
		value = *(_u64 *)&rsp_dmem[offset & 0xfff];
		LogMe (dfile, "value = %08X%08X\n", value, (value >> 32));
		return( (value >> 32) | (value << 32) );
	}
}

_u16 Load16_DMEMLTV(_u32 offset)
{
	_u16 value;
	//_assert_ ((offset & 0xffffff) == (offset & 0xfff));
	if (offset & 0x1) {
		_u32 base = offset & 0xFFF0;
		value  =  Load8_DMEM (((offset+1)&0xf)+base);
		value |= (Load8_DMEM (((offset  )&0xf)+base) << 8);
		return value;
	} else {
		offset ^= 0x02;
		return (*(_u16 *)&rsp_dmem[offset & 0xfff]);
	}
}

//_____________________________________________________________________
//
//

void Save8_DMEM(_u8 what, _u32 addr)
{
	//_assert_ ((addr & 0xffffff) == (addr & 0xfff));
	addr ^= 0x03;
    *((_u8 *)(&rsp_dmem[addr & 0xfff])) = what;
}

void Save16_DMEM(_u16 what, _u32 addr)
{
	//_assert_ ((addr & 0xffffff) == (addr & 0xfff));
	if (addr & 0x1) {
		Save8_DMEM ((_u8)(what>>8), ((addr)&0xfff));
		Save8_DMEM ((_u8)(what)   , ((addr+1)&0xfff));
	} else {
		addr ^= 0x02;
		*((_u16 *)(&rsp_dmem[addr & 0xfff])) = what;
	}
}

void Save32_DMEM(_u32 what, _u32 addr)
{
	int i;
	//_assert_ ((addr & 0xffffff) == (addr & 0xfff));
	if (addr & 0x3) {
		for (i = 0; i < 4; i++) {
			Save8_DMEM ((_u8)(what>>((3-i)<<3)), ((addr+i)&0xfff));
		}
	} else {
		*((_u32 *)(&rsp_dmem[addr & 0xfff])) = what;
	}
}

void Save64_DMEM(_u64 what, _u32 addr)
{
	int i;
	//_assert_ ((addr & 0xffffff) == (addr & 0xfff));
	if (addr & 0x3) {
		for (i = 0; i < 4; i++) {
			Save8_DMEM ((_u8)(what>>((3-i)<<3)), ((addr+i+4)&0xfff));
		}
		for (i = 0; i < 4; i++) {
			Save8_DMEM ((_u8)(what>>((7-i)<<3)), ((addr+i)&0xfff));
		}

	} else {
		*((_u64 *)(&rsp_dmem[addr & 0xfff])) = (what << 32) | (what >> 32);
	}
}

//_____________________________________________________________________
//
//

void FillDummyVector(int vec, int kind)
{
	switch(kind)
	{
	case 0x0:
	case 0x1:
        rsp_reg.dummy->U16[0] = rsp_reg.v[vec]->U16[0];
        rsp_reg.dummy->U16[1] = rsp_reg.v[vec]->U16[1];
        rsp_reg.dummy->U16[2] = rsp_reg.v[vec]->U16[2];
        rsp_reg.dummy->U16[3] = rsp_reg.v[vec]->U16[3];
        rsp_reg.dummy->U16[4] = rsp_reg.v[vec]->U16[4];
        rsp_reg.dummy->U16[5] = rsp_reg.v[vec]->U16[5];
        rsp_reg.dummy->U16[6] = rsp_reg.v[vec]->U16[6];
        rsp_reg.dummy->U16[7] = rsp_reg.v[vec]->U16[7];
		break;
// 0q
	case 0x2: 
        rsp_reg.dummy->U16[0] = rsp_reg.v[vec]->U16[1];
        rsp_reg.dummy->U16[1] = rsp_reg.v[vec]->U16[1];
        rsp_reg.dummy->U16[2] = rsp_reg.v[vec]->U16[3];
        rsp_reg.dummy->U16[3] = rsp_reg.v[vec]->U16[3];
        rsp_reg.dummy->U16[4] = rsp_reg.v[vec]->U16[5];
        rsp_reg.dummy->U16[5] = rsp_reg.v[vec]->U16[5];
        rsp_reg.dummy->U16[6] = rsp_reg.v[vec]->U16[7];
        rsp_reg.dummy->U16[7] = rsp_reg.v[vec]->U16[7];
		break;
// 1q
	case 0x3:
        rsp_reg.dummy->U16[0] = rsp_reg.v[vec]->U16[0];
        rsp_reg.dummy->U16[1] = rsp_reg.v[vec]->U16[0];
        rsp_reg.dummy->U16[2] = rsp_reg.v[vec]->U16[2];
        rsp_reg.dummy->U16[3] = rsp_reg.v[vec]->U16[2];
        rsp_reg.dummy->U16[4] = rsp_reg.v[vec]->U16[4];
        rsp_reg.dummy->U16[5] = rsp_reg.v[vec]->U16[4];
        rsp_reg.dummy->U16[6] = rsp_reg.v[vec]->U16[6];
        rsp_reg.dummy->U16[7] = rsp_reg.v[vec]->U16[6];
		break;
// 0h
	case 0x4:
        rsp_reg.dummy->U16[0] = rsp_reg.v[vec]->U16[3];
        rsp_reg.dummy->U16[1] = rsp_reg.v[vec]->U16[3];
        rsp_reg.dummy->U16[2] = rsp_reg.v[vec]->U16[3];
        rsp_reg.dummy->U16[3] = rsp_reg.v[vec]->U16[3];
        rsp_reg.dummy->U16[4] = rsp_reg.v[vec]->U16[7];
        rsp_reg.dummy->U16[5] = rsp_reg.v[vec]->U16[7];
        rsp_reg.dummy->U16[6] = rsp_reg.v[vec]->U16[7];
        rsp_reg.dummy->U16[7] = rsp_reg.v[vec]->U16[7];
		break;
// 1h
	case 0x5:
        rsp_reg.dummy->U16[0] = rsp_reg.v[vec]->U16[2];
        rsp_reg.dummy->U16[1] = rsp_reg.v[vec]->U16[2];
        rsp_reg.dummy->U16[2] = rsp_reg.v[vec]->U16[2];
        rsp_reg.dummy->U16[3] = rsp_reg.v[vec]->U16[2];
        rsp_reg.dummy->U16[4] = rsp_reg.v[vec]->U16[6];
        rsp_reg.dummy->U16[5] = rsp_reg.v[vec]->U16[6];
        rsp_reg.dummy->U16[6] = rsp_reg.v[vec]->U16[6];
        rsp_reg.dummy->U16[7] = rsp_reg.v[vec]->U16[6];
		break;
// 2h
	case 0x6:
        rsp_reg.dummy->U16[0] = rsp_reg.v[vec]->U16[1];
        rsp_reg.dummy->U16[1] = rsp_reg.v[vec]->U16[1];
        rsp_reg.dummy->U16[2] = rsp_reg.v[vec]->U16[1];
        rsp_reg.dummy->U16[3] = rsp_reg.v[vec]->U16[1];
        rsp_reg.dummy->U16[4] = rsp_reg.v[vec]->U16[5];
        rsp_reg.dummy->U16[5] = rsp_reg.v[vec]->U16[5];
        rsp_reg.dummy->U16[6] = rsp_reg.v[vec]->U16[5];
        rsp_reg.dummy->U16[7] = rsp_reg.v[vec]->U16[5];
		break;
// 3h
	case 0x7:
        rsp_reg.dummy->U16[0] = rsp_reg.v[vec]->U16[0];
        rsp_reg.dummy->U16[1] = rsp_reg.v[vec]->U16[0];
        rsp_reg.dummy->U16[2] = rsp_reg.v[vec]->U16[0];
        rsp_reg.dummy->U16[3] = rsp_reg.v[vec]->U16[0];
        rsp_reg.dummy->U16[4] = rsp_reg.v[vec]->U16[4];
        rsp_reg.dummy->U16[5] = rsp_reg.v[vec]->U16[4];
        rsp_reg.dummy->U16[6] = rsp_reg.v[vec]->U16[4];
        rsp_reg.dummy->U16[7] = rsp_reg.v[vec]->U16[4];
		break;
// 0
	case 0x8:
        rsp_reg.dummy->U16[0] = rsp_reg.v[vec]->U16[7];
        rsp_reg.dummy->U16[1] = rsp_reg.v[vec]->U16[7];
        rsp_reg.dummy->U16[2] = rsp_reg.v[vec]->U16[7];
        rsp_reg.dummy->U16[3] = rsp_reg.v[vec]->U16[7];
        rsp_reg.dummy->U16[4] = rsp_reg.v[vec]->U16[7];
        rsp_reg.dummy->U16[5] = rsp_reg.v[vec]->U16[7];
        rsp_reg.dummy->U16[6] = rsp_reg.v[vec]->U16[7];
        rsp_reg.dummy->U16[7] = rsp_reg.v[vec]->U16[7];
		break;
// 1
	case 0x9:
        rsp_reg.dummy->U16[0] = rsp_reg.v[vec]->U16[6];
        rsp_reg.dummy->U16[1] = rsp_reg.v[vec]->U16[6];
        rsp_reg.dummy->U16[2] = rsp_reg.v[vec]->U16[6];
        rsp_reg.dummy->U16[3] = rsp_reg.v[vec]->U16[6];
        rsp_reg.dummy->U16[4] = rsp_reg.v[vec]->U16[6];
        rsp_reg.dummy->U16[5] = rsp_reg.v[vec]->U16[6];
        rsp_reg.dummy->U16[6] = rsp_reg.v[vec]->U16[6];
        rsp_reg.dummy->U16[7] = rsp_reg.v[vec]->U16[6];
		break;
// 2
	case 0xa:
        rsp_reg.dummy->U16[0] = rsp_reg.v[vec]->U16[5];
        rsp_reg.dummy->U16[1] = rsp_reg.v[vec]->U16[5];
        rsp_reg.dummy->U16[2] = rsp_reg.v[vec]->U16[5];
        rsp_reg.dummy->U16[3] = rsp_reg.v[vec]->U16[5];
        rsp_reg.dummy->U16[4] = rsp_reg.v[vec]->U16[5];
        rsp_reg.dummy->U16[5] = rsp_reg.v[vec]->U16[5];
        rsp_reg.dummy->U16[6] = rsp_reg.v[vec]->U16[5];
        rsp_reg.dummy->U16[7] = rsp_reg.v[vec]->U16[5];
		break;
// 3
	case 0xb:
        rsp_reg.dummy->U16[0] = rsp_reg.v[vec]->U16[4];
        rsp_reg.dummy->U16[1] = rsp_reg.v[vec]->U16[4];
        rsp_reg.dummy->U16[2] = rsp_reg.v[vec]->U16[4];
        rsp_reg.dummy->U16[3] = rsp_reg.v[vec]->U16[4];
        rsp_reg.dummy->U16[4] = rsp_reg.v[vec]->U16[4];
        rsp_reg.dummy->U16[5] = rsp_reg.v[vec]->U16[4];
        rsp_reg.dummy->U16[6] = rsp_reg.v[vec]->U16[4];
        rsp_reg.dummy->U16[7] = rsp_reg.v[vec]->U16[4];
		break;
// 4
	case 0xc:
        rsp_reg.dummy->U16[0] = rsp_reg.v[vec]->U16[3];
        rsp_reg.dummy->U16[1] = rsp_reg.v[vec]->U16[3];
        rsp_reg.dummy->U16[2] = rsp_reg.v[vec]->U16[3];
        rsp_reg.dummy->U16[3] = rsp_reg.v[vec]->U16[3];
        rsp_reg.dummy->U16[4] = rsp_reg.v[vec]->U16[3];
        rsp_reg.dummy->U16[5] = rsp_reg.v[vec]->U16[3];
        rsp_reg.dummy->U16[6] = rsp_reg.v[vec]->U16[3];
        rsp_reg.dummy->U16[7] = rsp_reg.v[vec]->U16[3];
		break;
// 5
	case 0xd:
        rsp_reg.dummy->U16[0] = rsp_reg.v[vec]->U16[2];
        rsp_reg.dummy->U16[1] = rsp_reg.v[vec]->U16[2];
        rsp_reg.dummy->U16[2] = rsp_reg.v[vec]->U16[2];
        rsp_reg.dummy->U16[3] = rsp_reg.v[vec]->U16[2];
        rsp_reg.dummy->U16[4] = rsp_reg.v[vec]->U16[2];
        rsp_reg.dummy->U16[5] = rsp_reg.v[vec]->U16[2];
        rsp_reg.dummy->U16[6] = rsp_reg.v[vec]->U16[2];
        rsp_reg.dummy->U16[7] = rsp_reg.v[vec]->U16[2];
		break;
// 6
	case 0xe:
        rsp_reg.dummy->U16[0] = rsp_reg.v[vec]->U16[1];
        rsp_reg.dummy->U16[1] = rsp_reg.v[vec]->U16[1];
        rsp_reg.dummy->U16[2] = rsp_reg.v[vec]->U16[1];
        rsp_reg.dummy->U16[3] = rsp_reg.v[vec]->U16[1];
        rsp_reg.dummy->U16[4] = rsp_reg.v[vec]->U16[1];
        rsp_reg.dummy->U16[5] = rsp_reg.v[vec]->U16[1];
        rsp_reg.dummy->U16[6] = rsp_reg.v[vec]->U16[1];
        rsp_reg.dummy->U16[7] = rsp_reg.v[vec]->U16[1];
		break;
// 7
	case 0xf:
        rsp_reg.dummy->U16[0] = rsp_reg.v[vec]->U16[0];
        rsp_reg.dummy->U16[1] = rsp_reg.v[vec]->U16[0];
        rsp_reg.dummy->U16[2] = rsp_reg.v[vec]->U16[0];
        rsp_reg.dummy->U16[3] = rsp_reg.v[vec]->U16[0];
        rsp_reg.dummy->U16[4] = rsp_reg.v[vec]->U16[0];
        rsp_reg.dummy->U16[5] = rsp_reg.v[vec]->U16[0];
        rsp_reg.dummy->U16[6] = rsp_reg.v[vec]->U16[0];
        rsp_reg.dummy->U16[7] = rsp_reg.v[vec]->U16[0];
		break;
	} // 	switch(kind)
}//*/
/* rsp instructions */

void rsp_special();
void rsp_regimm();
void rsp_j();
void rsp_jal();
void rsp_beq();
void rsp_bne();
void rsp_blez();
void rsp_bgtz();
void rsp_addi();
void rsp_addiu();
void rsp_slti();
void rsp_sltiu();
void rsp_andi();
void rsp_ori();
void rsp_xori();
void rsp_lui();
void rsp_cop0();
void rsp_cop2();
void rsp_lb();
void rsp_lh();
void rsp_lw();
void rsp_lbu();
void rsp_lhu();
void rsp_sb();
void rsp_sh();
void rsp_sw();
void rsp_lwc2();
void rsp_swc2();
void rsp_reserved();

// lwc2
void rsp_lbv();
void rsp_lsv();
void rsp_llv();
void rsp_ldv();
void rsp_lqv();
void rsp_lrv();
void rsp_lpv();
void rsp_luv();
void rsp_lhv();
void rsp_lfv();
void rsp_lwv();
void rsp_ltv();


// swc2
void rsp_sbv();
void rsp_ssv();
void rsp_slv();
void rsp_sdv();
void rsp_sqv();
void rsp_srv();
void rsp_spv();
void rsp_suv();
void rsp_shv();
void rsp_sfv();
void rsp_swv();
void rsp_stv();


/* rsp special instructions */

void rsp_special_sll();
void rsp_special_srl();
void rsp_special_sra();
void rsp_special_sllv();
void rsp_special_srlv();
void rsp_special_srav();
void rsp_special_jr();
void rsp_special_jalr();
void rsp_special_break();
void rsp_special_add();
void rsp_special_addu();
void rsp_special_sub();
void rsp_special_subu();
void rsp_special_and();
void rsp_special_or();
void rsp_special_xor();
void rsp_special_nor();
void rsp_special_slt();
void rsp_special_sltu();
void rsp_special_reserved();



/* rsp regimm instructions */

void rsp_regimm_bltz();
void rsp_regimm_bgez();
void rsp_regimm_bltzal();
void rsp_regimm_bgezal();
void rsp_regimm_reserved();



/* rsp cop0 instructions */

void rsp_cop0_mf();
void rsp_cop0_mt();
void rsp_cop0_reserved();



/* rsp cop2 instructions */

void rsp_cop2_mf();
void rsp_cop2_cf();
void rsp_cop2_mt();
void rsp_cop2_ct();
void rsp_cop2_vectop();
void rsp_cop2_reserved();



/* rsp cop2 vectop instructions */

void rsp_cop2_vectop_vmulf();
void rsp_cop2_vectop_vmulu();
void rsp_cop2_vectop_vrndp();
void rsp_cop2_vectop_vmulq();

void rsp_cop2_vectop_vmudl();
void rsp_cop2_vectop_vmudm();
void rsp_cop2_vectop_vmudn();
void rsp_cop2_vectop_vmudh();

void rsp_cop2_vectop_vmacf();
void rsp_cop2_vectop_vmacu();
void rsp_cop2_vectop_vrndn();
void rsp_cop2_vectop_vmacq();
void rsp_cop2_vectop_vmadl();
void rsp_cop2_vectop_vmadm();
void rsp_cop2_vectop_vmadn();
void rsp_cop2_vectop_vmadh();
void rsp_cop2_vectop_vadd();
void rsp_cop2_vectop_vsub();
void rsp_cop2_vectop_vsut();
void rsp_cop2_vectop_vabs();
void rsp_cop2_vectop_vaddc();
void rsp_cop2_vectop_vsubc();
void rsp_cop2_vectop_vaddb();
void rsp_cop2_vectop_vsubb();
void rsp_cop2_vectop_vaccb();
void rsp_cop2_vectop_vsucb();
void rsp_cop2_vectop_vsad();
void rsp_cop2_vectop_vsac();
void rsp_cop2_vectop_vsum();
void rsp_cop2_vectop_vsaw();
void rsp_cop2_vectop_vlt();
void rsp_cop2_vectop_veq();
void rsp_cop2_vectop_vne();
void rsp_cop2_vectop_vge();
void rsp_cop2_vectop_vcl();
void rsp_cop2_vectop_vch();
void rsp_cop2_vectop_vcr();
void rsp_cop2_vectop_vmrg();
void rsp_cop2_vectop_vand();
void rsp_cop2_vectop_vnand();
void rsp_cop2_vectop_vor();
void rsp_cop2_vectop_vnor();
void rsp_cop2_vectop_vxor();
void rsp_cop2_vectop_vnxor();
void rsp_cop2_vectop_vrcp();
void rsp_cop2_vectop_vrcpl();
void rsp_cop2_vectop_vrcph();
void rsp_cop2_vectop_vmov();
void rsp_cop2_vectop_vrsq();
void rsp_cop2_vectop_vrsql();
void rsp_cop2_vectop_vrsqh();
void rsp_cop2_vectop_vnoop();
void rsp_cop2_vectop_vextt();
void rsp_cop2_vectop_vextq();
void rsp_cop2_vectop_vextn();
void rsp_cop2_vectop_vinst();
void rsp_cop2_vectop_vinsq();
void rsp_cop2_vectop_vinsn();
void rsp_cop2_vectop_reserved();

//_____________________________________________________________________
// rsp instruction function pointer
//
rsp_instr instruction[64] =
{
        rsp_special,
        rsp_regimm,
        rsp_j,
        rsp_jal,
        rsp_beq,
        rsp_bne,
        rsp_blez,
        rsp_bgtz,
        rsp_addi,
        rsp_addiu,
        rsp_slti,
        rsp_sltiu,
        rsp_andi,
        rsp_ori,
        rsp_xori,
        rsp_lui,
        rsp_cop0,
        rsp_reserved,
        rsp_cop2,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_lb,
        rsp_lh,
        rsp_reserved,
        rsp_lw,
        rsp_lbu,
        rsp_lhu,
        rsp_reserved,
        rsp_reserved,
        rsp_sb,
        rsp_sh,
        rsp_reserved,
        rsp_sw,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_lwc2,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_swc2,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved,
        rsp_reserved
};

//_____________________________________________________________________
// rsp lwc2 function pointer
//
rsp_instr lwc2_instruction[32] =
{
		rsp_lbv,
		rsp_lsv,
		rsp_llv,
		rsp_ldv,
		rsp_lqv,
		rsp_lrv,
		rsp_lpv,
		rsp_luv,
		rsp_lhv,
		rsp_lfv,
		rsp_lwv,
		rsp_ltv,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved
};

//_____________________________________________________________________
// rsp swc2 function pointer
//
rsp_instr swc2_instruction[32] =
{
		rsp_sbv,
		rsp_ssv,
		rsp_slv,
		rsp_sdv,
		rsp_sqv,
		rsp_srv,
		rsp_spv,
		rsp_suv,
		rsp_shv,
		rsp_sfv,
		rsp_swv,
		rsp_stv,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved,
		rsp_reserved
};

//_____________________________________________________________________
// rsp special function pointer
//
rsp_instr special_instruction[64] =
{
        rsp_special_sll,
        rsp_special_reserved,
        rsp_special_srl,
        rsp_special_sra,
        rsp_special_sllv,
        rsp_special_reserved,
        rsp_special_srlv,
        rsp_special_srav,
        rsp_special_jr,
        rsp_special_jalr,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_break,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_add,
        rsp_special_addu,
        rsp_special_sub,
        rsp_special_subu,
        rsp_special_and,
        rsp_special_or,
        rsp_special_xor,
        rsp_special_nor,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_slt,
        rsp_special_sltu,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved,
        rsp_special_reserved
};

//_____________________________________________________________________
// rsp regimm function pointer
//
rsp_instr regimm_instruction[32] =
{
        rsp_regimm_bltz,
        rsp_regimm_bgez,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_bltzal,
        rsp_regimm_bgezal,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved,
        rsp_regimm_reserved
};

//_____________________________________________________________________
// rsp cop0 function pointer
//
rsp_instr cop0_instruction[32] =
{
        rsp_cop0_mf,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_mt,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved,
        rsp_cop0_reserved
};

//_____________________________________________________________________
// rsp cop2 function pointer
//
rsp_instr cop2_instruction[32] =
{
        rsp_cop2_mf,
        rsp_cop2_reserved,
        rsp_cop2_cf,
        rsp_cop2_reserved,
        rsp_cop2_mt,
        rsp_cop2_reserved,
        rsp_cop2_ct,
        rsp_cop2_reserved,
        rsp_cop2_reserved,
        rsp_cop2_reserved,
        rsp_cop2_reserved,
        rsp_cop2_reserved,
        rsp_cop2_reserved,
        rsp_cop2_reserved,
        rsp_cop2_reserved,
        rsp_cop2_reserved,
        rsp_cop2_vectop,
        rsp_cop2_vectop,
        rsp_cop2_vectop,
        rsp_cop2_vectop,
        rsp_cop2_vectop,
        rsp_cop2_vectop,
        rsp_cop2_vectop,
        rsp_cop2_vectop,
        rsp_cop2_vectop,
        rsp_cop2_vectop,
        rsp_cop2_vectop,
        rsp_cop2_vectop,
        rsp_cop2_vectop,
        rsp_cop2_vectop,
        rsp_cop2_vectop,
        rsp_cop2_vectop
};

//_____________________________________________________________________
// rsp cop2 vectop function pointer 
//
rsp_instr cop2_vectop_instruction[64] =
{
        rsp_cop2_vectop_vmulf,
        rsp_cop2_vectop_vmulu,
        rsp_cop2_vectop_vrndp,
        rsp_cop2_vectop_vmulq,
        rsp_cop2_vectop_vmudl,
        rsp_cop2_vectop_vmudm,
        rsp_cop2_vectop_vmudn,
        rsp_cop2_vectop_vmudh,
        rsp_cop2_vectop_vmacf,
        rsp_cop2_vectop_vmacu,
        rsp_cop2_vectop_vrndn,
        rsp_cop2_vectop_vmacq,
        rsp_cop2_vectop_vmadl,
        rsp_cop2_vectop_vmadm,
        rsp_cop2_vectop_vmadn,
        rsp_cop2_vectop_vmadh,
        rsp_cop2_vectop_vadd,
        rsp_cop2_vectop_vsub,
        rsp_cop2_vectop_vsut,
        rsp_cop2_vectop_vabs,
        rsp_cop2_vectop_vaddc,
        rsp_cop2_vectop_vsubc,
        rsp_cop2_vectop_vaddb,
        rsp_cop2_vectop_vsubb,
        rsp_cop2_vectop_vaccb,
        rsp_cop2_vectop_vsucb,
        rsp_cop2_vectop_vsad,
        rsp_cop2_vectop_vsac,
        rsp_cop2_vectop_vsum,
        rsp_cop2_vectop_vsaw,
        rsp_cop2_vectop_reserved,
        rsp_cop2_vectop_reserved,
        rsp_cop2_vectop_vlt,
        rsp_cop2_vectop_veq,
        rsp_cop2_vectop_vne,
        rsp_cop2_vectop_vge,
        rsp_cop2_vectop_vcl,
        rsp_cop2_vectop_vch,
        rsp_cop2_vectop_vcr,
        rsp_cop2_vectop_vmrg,
        rsp_cop2_vectop_vand,
        rsp_cop2_vectop_vnand,
        rsp_cop2_vectop_vor,
        rsp_cop2_vectop_vnor,
        rsp_cop2_vectop_vxor,
        rsp_cop2_vectop_vnxor,
        rsp_cop2_vectop_reserved,
        rsp_cop2_vectop_reserved,
        rsp_cop2_vectop_vrcp,
        rsp_cop2_vectop_vrcpl,
        rsp_cop2_vectop_vrcph,
        rsp_cop2_vectop_vmov,
        rsp_cop2_vectop_vrsq,
        rsp_cop2_vectop_vrsql,
        rsp_cop2_vectop_vrsqh,
        rsp_cop2_vectop_vnoop,
        rsp_cop2_vectop_vextt,
        rsp_cop2_vectop_vextq,
        rsp_cop2_vectop_vextn,
        rsp_cop2_vectop_reserved,
        rsp_cop2_vectop_vinst,
        rsp_cop2_vectop_vinsq,
        rsp_cop2_vectop_vinsn,
        rsp_cop2_vectop_reserved
};





//_____________________________________________________________________
// reset
//
void rsp_reset()
{
	int i, j;

	// debugging 
	rsp_reg.halt = 1;
	rsp_reg.count = 0;

	// gpr registers 
	for(i=0; i<32; i++)
		rsp_reg.r[i] = 0;

	// flags (ccr) 
	for(i=0; i<4; i++)
		rsp_reg.flag[i] = 0;

	// vector registers 
	for(j=0; j<32; j++)
		for(i=0; i<8; i++)
			rsp_reg.v[j]->U16[i] = 0;

    // hidden registers 
	rsp_reg.vrcph_source = 0;
	rsp_reg.vrcph_result = 0;
}



/******************************************************************************\
*                                                                              *
*   RSP (Reality Signal Processor)                                             *
*                                                                              *
\******************************************************************************/

//_____________________________________________________________________
// SPECIAL
//
void rsp_special()
{
	(special_instruction[__F])();
}

//_____________________________________________________________________
// REGIMM
//
void rsp_regimm()
{
	(regimm_instruction[__RT])();
}

//_____________________________________________________________________
// J
//
void rsp_j()
{
	rsp_reg.delay = DO_RSP_DELAY;
	rsp_reg.pc_delay = __T;
}

//_____________________________________________________________________
// JAL
//
void rsp_jal()
{
	rsp_reg.delay = DO_RSP_DELAY;
	rsp_reg.pc_delay = __T;
	rsp_reg.r[31] = (sp_reg_pc + 8) & 0x1fff;
}

//_____________________________________________________________________
// BEQ
//
void rsp_beq()
{
	if(rsp_reg.r[__RS] == rsp_reg.r[__RT])
	{
		rsp_reg.delay = DO_RSP_DELAY;
		rsp_reg.pc_delay = __O;
	}
}

//_____________________________________________________________________
// BNE
//
void rsp_bne()
{
	if(rsp_reg.r[__RS] != rsp_reg.r[__RT])
	{
		LogMe (dfile,  "%08X: BNE: rsp_reg.r[%i](%08X) != rsp_reg.r[%i](%08X)\n", sp_reg_pc, __RS, rsp_reg.r[__RS], __RT, rsp_reg.r[__RT]);
		rsp_reg.delay = DO_RSP_DELAY;
		rsp_reg.pc_delay = __O;
	} else {
		LogMe (dfile,  "%08X: BNE: rsp_reg.r[%i](%08X) == rsp_reg.r[%i](%08X)\n", sp_reg_pc, __RS, rsp_reg.r[__RS], __RT, rsp_reg.r[__RT]);
	}
	//LogMe (dfile, "BNE Skipped: rsp_reg.r[%i](%08X) != rsp_reg.r[%i](%08X)\n", __RS, rsp_reg.r[__RS], __RT, rsp_reg.r[__RT]);
}

//_____________________________________________________________________
// BLEZ
//
void rsp_blez()
{
	if(((_s32)rsp_reg.r[__RS]) <= 0)
	{
		rsp_reg.delay = DO_RSP_DELAY;
		rsp_reg.pc_delay = __O;
	}
}

//_____________________________________________________________________
// BGTZ
//
void rsp_bgtz()
{
	if(((_s32)rsp_reg.r[__RS]) > 0)
	{
		rsp_reg.delay = DO_RSP_DELAY;
		rsp_reg.pc_delay = __O;
	}
}

//_____________________________________________________________________
// ADDI
//
void rsp_addi()
{
	rsp_reg.r[__RT] = (_s32)(rsp_reg.r[__RS] + __I);
	rsp_reg.r[0] = 0;
}

//_____________________________________________________________________
// ADDIU
//
void rsp_addiu()
{
	rsp_reg.r[__RT] = (_s32)(rsp_reg.r[__RS] + __I);
	rsp_reg.r[0] = 0;
}

//_____________________________________________________________________
// SLTI
//
void rsp_slti()
{
	if(rsp_reg.r[__RS] <(_s32) __I)
		rsp_reg.r[__RT] = 1;
	else
		rsp_reg.r[__RT] = 0;
}

//_____________________________________________________________________
// SLTIU
//
void rsp_sltiu()
{
	if((_u32)rsp_reg.r[__RS] < (_u32)(_s32)__I)
		rsp_reg.r[__RT] = 1;
	else
		rsp_reg.r[__RT] = 0;
}

//_____________________________________________________________________
// ANDI
//
void rsp_andi()
{
	rsp_reg.r[__RT] = rsp_reg.r[__RS] & (_u32)(_u16)__I;
}

//_____________________________________________________________________
// ORI
//
void rsp_ori()
{
	rsp_reg.r[__RT] = rsp_reg.r[__RS] | (_u32)(_u16)__I;
}

//_____________________________________________________________________
// XORI
//
void rsp_xori()
{
	rsp_reg.r[__RT] = rsp_reg.r[__RS] ^ (_u32)(_u16)__I;
}

//_____________________________________________________________________
// LUI
//
void rsp_lui()
{
	rsp_reg.r[__RT] = (_s32)(__I << 16);
}

//_____________________________________________________________________
// COP1
//
void rsp_cop0()
{
	(cop0_instruction[__RS])();   
}

//_____________________________________________________________________
// COP2
//
void rsp_cop2()
{
	(cop2_instruction[__RS])();
}

//_____________________________________________________________________
// LH
//
void rsp_lb()
{
	_u32 addr = rsp_reg.r[__RS]+__I;
	rsp_reg.r[__RT] = (_s32)(_s8)Load8_DMEM(addr);
}

//_____________________________________________________________________
// LH
//
void rsp_lh()
{
	_u32 addr = rsp_reg.r[__RS]+__I;
	rsp_reg.r[__RT] = (_s32)(_s16)Load16_DMEM(addr);
}

//_____________________________________________________________________
// LW
//
void rsp_lw()
{
	_u32 addr = rsp_reg.r[__RS]+__I;
	rsp_reg.r[__RT] = (_s32)Load32_DMEM(addr);
}

//_____________________________________________________________________
// LBU
//
void rsp_lbu()
{
	_u32 addr = rsp_reg.r[__RS]+__I;
	rsp_reg.r[__RT] = (_s32)(_u8)Load8_DMEM(addr);
}

//_____________________________________________________________________
// LHU
//
void rsp_lhu()
{
	_u32 addr = rsp_reg.r[__RS]+__I;
	rsp_reg.r[__RT] = (_s32)(_u16)Load16_DMEM(addr);
}

//_____________________________________________________________________
// SB
//
void rsp_sb()
{
	_u32 data, addr;

	data = rsp_reg.r[__RT];
	addr = rsp_reg.r[__RS]+__I;

	Save8_DMEM((_u8)data, addr);
}

//_____________________________________________________________________
// SH
//
void rsp_sh()
{
	_u32 data, addr;

	data = rsp_reg.r[__RT];
	addr = rsp_reg.r[__RS]+__I;

	Save16_DMEM((_u16)data, addr);
}

//_____________________________________________________________________
// SW
//
void rsp_sw()
{
	_u32 data, addr;

	data = rsp_reg.r[__RT];
	addr = rsp_reg.r[__RS]+__I;

	Save32_DMEM((_u32)data, addr);
}




/////////////////////////////////////////////////////////////////////////////
// LWC2
/////////////////////////////////////////////////////////////////////////////

//_____________________________________________________________________
// LBV stores a byte
//
void rsp_lbv() // Works now... - 01-09-2001 - Azimer
{
	_u32	offset = (__SA >> 1) ^ 0xf;
	_u32	addr = __RSP_O(0) + rsp_reg.r[__RS];

	rsp_reg.v[__RT]->U8[offset] = Load8_DMEM(addr);
}

//_____________________________________________________________________
// 
//
void rsp_lsv() 
{
	_u32	offset = ((__SA >> 1) / 2) ^ 7;
	_u32	addr =  __RSP_O(1) + rsp_reg.r[__RS];
	int i;

	LogMe (dfile, "LSV $v%i[%i], $%04X(R%i) (Reg = %X)\n", __RT, offset, __RSP_O(4), __RS, rsp_reg.r[__RS]);
	LogMe (dfile, "Boundary passed by: %i bytes\n", addr & 0x2);

	rsp_reg.v[__RT]->U16[offset] = Load16_DMEM(addr);

	for (i=0; i < 8; i++) {
		LogMe (dfile, "LSV $v%i element %i = %04X\n", __RT, i, rsp_reg.v[__RT]->U16[i]);
		//Debug (0, "LSV $v%i element %i = %04X\n", __RT, i, rsp_reg.v[__RT]->U16[i]);
	}
}

//_____________________________________________________________________
// 
//
void rsp_llv() // Load Long Vector (WIP) - 02-05-2001 - Azimer
// NOTE!!!: Possible bug if (__SA >> 2) is odd.. furthur research is needed
{
	_u32	offset = 6-(__SA >> 2);
	_u32	addr = __RSP_O(2) + rsp_reg.r[__RS];
	rsp_reg.v[__RT]->U32[offset/2] = Load32_DMEM(addr);
}

//_____________________________________________________________________
// LDV loads a double (doubleword, 64-bits)
//
void rsp_ldv() // Works now... No funky business like LQV... - 01-09-2001 - Azimer
{
	_u32	offset = ((__SA >> 1) / 8) ^ 1;
	_u32	addr = __RSP_O(3) + rsp_reg.r[__RS];

	rsp_reg.v[__RT]->U64[offset] = Load64_DMEM(addr);
}

//_____________________________________________________________________
// LQV loads a quad (quadword, 128-bits)
//
void rsp_lqv() { // Works great! - 01-09-2001 - Azimer
	
	_u32    offset = (__SA >> 1) / 16;
	_u32	addr   = __RSP_O(4) + rsp_reg.r[__RS];
	_u64	value1 = 0;
	_u64	value2 = 0;
	int		shifter;
	_u64	ander1 = 0xFFFFFFFFFFFFFFFF;
	_u64	ander2 = 0xFFFFFFFFFFFFFFFF;
	int		i;

	_assert_(offset == 0);

	shifter = ((addr & 0xf) << 3);
	LogMe (dfile, "LQV $v%i[%i], $%04X(R%i) (Reg = %X)\n", __RT, offset, __RSP_O(4), __RS, rsp_reg.r[__RS]);
	LogMe (dfile, "Boundary passed by: %i bytes\n", addr & 0xf);
	if (shifter == 0) {
		rsp_reg.v[__RT]->U64[offset] = Load64_DMEM(addr+8);
		rsp_reg.v[__RT]->U64[offset+1] = Load64_DMEM(addr);
	} else {
		value1 = Load64_DMEM(addr+8);
		value2 = Load64_DMEM(addr);
		if (shifter > 0x40) {
			ander2 = ((ander2 >> (shifter-0x40)) << (shifter-0x40));
			ander1 = 0;
		} else {
			ander1 = ((ander1 >> shifter) << shifter);
		}
		value1 = value1 & ander1;
		value2 = value2 & ander2;

		rsp_reg.v[__RT]->U64[offset] = rsp_reg.v[__RT]->U64[offset] & ~ander1;
		rsp_reg.v[__RT]->U64[offset+1] = rsp_reg.v[__RT]->U64[offset+1] & ~ander2;

		rsp_reg.v[__RT]->U64[offset] = rsp_reg.v[__RT]->U64[offset] | value1;
		rsp_reg.v[__RT]->U64[offset+1] = rsp_reg.v[__RT]->U64[offset+1] | value2;
	}

	for (i=0; i < 8; i++) {
		//rsp_reg.v[__RT]->U16[i] = Load16_DMEM(addr+(i*2));
		LogMe (dfile, "$v%i element %i = %04X\n", __RT, i, rsp_reg.v[__RT]->U16[i]);
	}
}

//_____________________________________________________________________
//
//
void rsp_lrv() {
	_s32	addr;
	int     offset, length;
    int     i;

	addr = __RSP_O(4) + rsp_reg.r[__RS];
	offset = (addr & 0xf) - 1;
	length = (addr & 0xf);
	addr &= 0xff0;
	for (i=0; i<length; i++) {
		rsp_reg.v[__RT]->U8[offset-i] = rsp_dmem[(addr ^ 3)];
		addr++;
	}
}

/*
void rsp_lrv() // Fixed - Works now... ByteSwapping Issue fixed - 01-09-2001 - Azimer
{
	_s32	addr;
	int     offset = (__SA >> 1);
    int     i;

	addr = __RSP_O(4) + rsp_reg.r[__RS];
	addr &= 0xfff;

	for(i=0; i < (addr & 0xf); i++)
	{
		if(i & 1)
		{
			rsp_reg.v[__RT]->U16[(i>>1)] &= 0x00ff;
			rsp_reg.v[__RT]->U16[(i>>1)] |= rsp_dmem[(addr-i-1)^3] << 8;
		}
		else
		{
			rsp_reg.v[__RT]->U16[(i>>1)] &= 0xff00;
			rsp_reg.v[__RT]->U16[(i>>1)] |= rsp_dmem[(addr-i-1)^3];
		}
	}
}*/
/*
[18:27] <zilmar> void RSP_Opcode_LRV ( void ) {
[18:27] <zilmar> 	DWORD Addr = RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4);
[18:27] <zilmar> 	int length, Count, offset;
[18:27] <zilmar> 	if (RSPOpC.del != 0) {
[18:27] <zilmar> 		rsp_UnknownOpcode();
[18:27] <zilmar> 		return;
[18:27] <zilmar> 	}
[18:27] <zilmar> 	offset = (Addr & 0xF) - 1;
[18:27] <zilmar> 	length = (Addr & 0xF);
[18:27] <zilmar> 	Addr &= 0xFF0;
[18:27] <zilmar> 	for (Count = 0; Count < length; Count ++ ){
[18:27] <zilmar> 		RSP_Vect[RSPOpC.rt].B[offset - Count] = *(RSPInfo.DMEM + (Addr ^ 3));
[18:27] <zilmar> 		Addr += 1;
[18:27] <zilmar> 	}
[18:27] <zilmar> }
*/
//_____________________________________________________________________
// 
//
void rsp_lpv() 
{
	MessageBox (NULL, "Need to implement LPV", "LPV", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_luv() 
{
	MessageBox (NULL, "Need to implement LUV", "LUV", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_lhv() 
{
	MessageBox (NULL, "Need to implement LHV", "LHV", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_lfv() 
{
	MessageBox (NULL, "Need to implement LFV", "LFV", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_lwv() 
{
	MessageBox (NULL, "Need to implement LWV", "LWV", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// Load Transposed Vector
//

void RSP_LTV_DMEM ( DWORD Addr, int vect, int element ) {
	int del, count, length;
	
	length = 8;
	if (length > 32 - vect) {
		length = 32 - vect;
	}
	Addr = ((Addr + 8) & 0xFF0) + (element & 0x1);	
	for (count = 0; count < length; count ++) {
		del = ((8 - (element >> 1) + count) << 1) & 0xF;
		rsp_reg.v[vect + count]->U8[15 - del] = *(&rsp_dmem[0] + (Addr ^ 3));
		rsp_reg.v[vect + count]->U8[14 - del] = *(&rsp_dmem[0] + ((Addr + 1) ^ 3));
		Addr += 2;
	}
}

void rsp_ltv() { // Works... zilmar approved - Azimer
	DWORD Address = (((_u16)rsp_reg.r[__RS] + (DWORD)((_u8)(__I) << 4)) &0xFFF);
	RSP_LTV_DMEM( Address, __RT, (__SA >> 1));
}

/////////////////////////////////////////////////////////////////////////////
// SWC2
/////////////////////////////////////////////////////////////////////////////

//_____________________________________________________________________
// SBV stores a byte
//
void rsp_sbv() 
{
	_u32	offset = (__SA >> 1) ^ 0xf;
	_u32	addr = __RSP_O(0) + rsp_reg.r[__RS];

	Save8_DMEM(rsp_reg.v[__RT]->U8[offset], addr);
}

//_____________________________________________________________________
// SSV stores a short (halfword, 16-bits)
//
void rsp_ssv() 
{
	_u32	offset = ((__SA >> 1) / 2) ^ 7;
	_u32	addr =  __RSP_O(1) + rsp_reg.r[__RS];

	Save16_DMEM(rsp_reg.v[__RT]->U16[offset], addr);
}

//_____________________________________________________________________
// SLV stores a long (word, 32-bits)
//
void rsp_slv() // Works! - 2-5-2001 - Azimer 
{
	_u32	offset = 6-(__SA >> 2);
	_u32	addr = __RSP_O(2) + rsp_reg.r[__RS];

	Save32_DMEM (rsp_reg.v[__RT]->U32[offset/2], addr);
}

//_____________________________________________________________________
// SDV stores a double (doubleword, 64-bits)
//
void rsp_sdv() 
{
	_u32	offset = ((__SA >> 1) / 8) ^ 1;
	_u32	addr = __RSP_O(3) + rsp_reg.r[__RS];

	Save64_DMEM(rsp_reg.v[__RT]->U64[offset], addr);
}

//_____________________________________________________________________
// SQV stores a quad (quadword, 128-bits)
//

void rsp_sqv() { // Works great! - 01-09-2001 - Azimer
	
	_u32    offset = (__SA >> 1) / 16;
	_u32	addr   = __RSP_O(4) + rsp_reg.r[__RS];
	_u64	value1 = 0;
	_u64	value2 = 0;
	int		shifter;
	_u64	ander1 = 0xFFFFFFFFFFFFFFFF;
	_u64	ander2 = 0xFFFFFFFFFFFFFFFF;

	_assert_(offset == 0);

	shifter = ((addr & 0xf) << 3);
	if (shifter == 0) {
		Save64_DMEM(rsp_reg.v[__RT]->U64[offset], addr+8);
		Save64_DMEM(rsp_reg.v[__RT]->U64[offset+1], addr);
	} else {
		value1 = Load64_DMEM(addr+8);
		value2 = Load64_DMEM(addr);

		if (shifter > 0x40) {
			ander2 = ((ander2 >> (shifter-0x40)) << (shifter-0x40));
			ander1 = 0;
		} else {
			ander1 = ((ander1 >> shifter) << shifter);
		}

		value1 = value1 & ~ander1;
		value2 = value2 & ~ander2;

		value1 |= (rsp_reg.v[__RT]->U64[offset  ] & ander1);
		value2 |= (rsp_reg.v[__RT]->U64[offset+1] & ander2);
		

		Save64_DMEM(value1, addr+8);
		Save64_DMEM(value2, addr  );
	}
}

//_____________________________________________________________________
// 
//
/*void rsp_srv() 
{
	MessageBox (NULL, "Need to implement SRV", "SRV", MB_OK);
	_assert_(0);
}
*/
/*
void RSP_SRV_DMEM ( DWORD Addr, int vect, int element ) {
	int length, Count, offset;
	length = (Addr & 0xF);
	offset = (0x10 - length) & 0xF;
	Addr &= 0xFF0;
	for (Count = element; Count < (length + element); Count ++ ){
		*(RSPInfo.DMEM + (Addr & 0xFFF)) = RSP_Vect[vect].B[15 - ((Count + offset) & 0xF)];
		Addr += 1;
	}
}

void rsp_srv ( void ) {
	__asm int 3;
	//DWORD Address = (((_u16)rsp_reg.r[__RS] + (DWORD)((_u8)(__I) << 4)) &0xFFF);
	//DWORD Address = ((rsp_reg.rRSP_GPR[RSPOpC.base].UW + (DWORD)(RSPOpC.voffset << 4)) &0xFFF);
	//RSP_SRV_DMEM( Address, RSPOpC.rt, RSPOpC.del);
}
*/
void RSP_SRV_DMEM ( DWORD Addr, int vect, int element ) {
	int length, Count, offset;
	length = (Addr & 0xF);
	offset = (0x10 - length) & 0xF;
	Addr &= 0xFF0;
	for (Count = element; Count < (length + element); Count ++ ){
		*(&rsp_dmem[0] + ((Addr & 0xFFF)^3)) = rsp_reg.v[vect]->U8[15 - ((Count + offset) & 0xF)];
		Addr += 1;
	}
}

void rsp_srv ( void ) {
	//MessageBox (NULL, "Need to implement SRV", "SRV", MB_OK);
	//_assert_(0);
	//return;
	DWORD Address = (((_u16)rsp_reg.r[__RS] + (DWORD)((_u8)(__I) << 4)) &0x3FF);
	/*SRV $v7 [0], 0x03F0 (T3)*/
	LogMe (dfile, "%08X: SRV: Address: %04X+%04X=%04X, vect: %i, element: %i\n", iCODE, rsp_reg.r[__RS], ((_u8)(__I&0x3F) << 4), Address, __RT, (__SA >> 1));
	LogMe (dfile, "SRV $v%i [%i], %04X[%i]\n", __RT, (__SA>>1), (DWORD)((_u8)(__I&0x3F) << 4), __RS);
	RSP_SRV_DMEM( Address, __RT, (__SA >> 1));
}

//_____________________________________________________________________
// 
//
void rsp_spv() 
{
	MessageBox (NULL, "Need to implement SPV", "SPV", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_suv() 
{
	MessageBox (NULL, "Need to implement SUV", "SUV", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_shv() 
{
	MessageBox (NULL, "Need to implement SHV", "SHV", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_sfv() 
{
	rsp_reg.halt = 1;
	return;
	//MessageBox (NULL, "Need to implement SFV", "SFV", MB_OK);
	//_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_swv() 
{
	MessageBox (NULL, "Need to implement SWV", "SWV", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//

void RSP_STV_DMEM ( DWORD Addr, int vect, int element ) {
	int del, count, length;
	
	length = 8;
	if (length > 32 - vect) {
		length = 32 - vect;
	}
	length = length << 1;
	del = element >> 1;
	for (count = 0; count < length; count += 2) {
		*(&rsp_dmem[0] + ((Addr ^ 3) & 0xFFF)) = rsp_reg.v[vect + del]->U8[15 - count];
		*(&rsp_dmem[0] + (((Addr + 1) ^ 3) & 0xFFF)) = rsp_reg.v[vect + del]->U8[14 - count];
		del = (del + 1) & 7;
		Addr += 2;
	}
}

void rsp_stv() { // Works now... zilmar approved
	DWORD Address = (((_u16)rsp_reg.r[__RS] + (DWORD)((_u8)(__I) << 4)) &0xFFF);
	int x, y;
	//Debug (0, "STV: $v%i [%i], 0x%04X (%04X)", __RT, (__SA >> 1), (__I << 4 & 0xFFF), (_u16)rsp_reg.r[__RS]);
	for (x=0; x < 32; x++) {
		for (y=0;y<8;y++)
			LogMe (dfile,  "STV: $v%i[%i] = %04X\n", x, y, rsp_reg.v[x]->U16[y]);
	}
	LogMe (dfile,  "Code: %08X", iCODE);
	RSP_STV_DMEM( Address, __RT, (__SA >> 1));
}

//_____________________________________________________________________
// LWC2
//
void rsp_lwc2()
{
//        _s32   addr;
  //      int     offset = __SA >> 1;
    //    int     i;

        //printf("RS: %x\nRT: %x\n(RD: %x)\nSA: %x\nF:  %x\n", __RS, __RT, __RD, __SA, __F);


(lwc2_instruction[__RD])();

return;

/*        switch(__RD)
        {
            case 0x00:
                addr = __RSP_O(0) + rsp_reg.r[__RS];
                addr &= 0xfff;

                if(offset & 1)
                {
                        rsp_reg.v[__RT][offset>>1] &= 0xff00;
                        rsp_reg.v[__RT][offset>>1] |= rsp_dmem[addr^3];
                }
                else
                {
                        rsp_reg.v[__RT][offset>>1] &= 0x00ff;
                        rsp_reg.v[__RT][offset>>1] |= rsp_dmem[addr^3] << 8;
                }
                break;

            case 0x01:
                addr = __RSP_O(1) + rsp_reg.r[__RS];
                addr &= 0xfff;

                rsp_reg.v[__RT][offset>>1] = (rsp_dmem[addr^3] << 8) | rsp_dmem[(addr+1)^3];
                break;

            case 0x02:

                addr = __RSP_O(2) + rsp_reg.r[__RS];
                addr &= 0xfff;

                rsp_reg.v[__RT][offset>>1] = (rsp_dmem[(addr)^3] << 8) | rsp_dmem[(addr+1)^3];
                rsp_reg.v[__RT][(offset>>1)+1] = (rsp_dmem[(addr+2)^3] << 8) | rsp_dmem[(addr+3)^3];
                break;

            case 0x03:

                addr = __RSP_O(3) + rsp_reg.r[__RS];
                addr &= 0xfff;
	
                rsp_reg.v[__RT][offset>>1]     = (rsp_dmem[(addr)^3] << 8) | rsp_dmem[(addr+1)^3];
                rsp_reg.v[__RT][(offset>>1)+1] = (rsp_dmem[(addr+2)^3] << 8) | rsp_dmem[(addr+3)^3];
                rsp_reg.v[__RT][(offset>>1)+2] = (rsp_dmem[(addr+4)^3] << 8) | rsp_dmem[(addr+5)^3];
                rsp_reg.v[__RT][(offset>>1)+3] = (rsp_dmem[(addr+6)^3] << 8) | rsp_dmem[(addr+7)^3];
                break;

            case 0x04:

                addr = __RSP_O(4) + rsp_reg.r[__RS];
                addr &= 0xfff;

                for(i=0; i < 8; i++)
                {
						if(i & 1)
                        {
                                rsp_reg.v[__RT][i>>1] &= 0xff00;
                                rsp_reg.v[__RT][i>>1] |= rsp_dmem[(addr+i)^3];
                        }
                        else
                        {
                                rsp_reg.v[__RT][i>>1] &= 0x00ff;
                                rsp_reg.v[__RT][i>>1] |= rsp_dmem[(addr+i)^3] << 8;
                        }
                }
                break;


// quad
            case 0x05:

                addr = __RSP_O(4) + rsp_reg.r[__RS];
                addr &= 0xfff;

                for(i=0; i < (addr & 0xf); i++)
                {
                        if(i & 1)
                        {
                                rsp_reg.v[__RT][7-(i>>1)] &= 0x00ff;
                                rsp_reg.v[__RT][7-(i>>1)] |= rsp_dmem[(addr-i-1)^3] << 8;
                        }
                        else
                        {
                                rsp_reg.v[__RT][7-(i>>1)] &= 0xff00;
                                rsp_reg.v[__RT][7-(i>>1)] |= rsp_dmem[(addr-i-1)^3];
                        }
                }
                break;

            case 0x06:

                addr = __RSP_O(3) + rsp_reg.r[__RS];
                addr &= 0xfff;

    // BUG: ??? offset should be removed ??? 
                rsp_reg.v[__RT][offset>>1] = rsp_dmem[addr^3] << 8;
                rsp_reg.v[__RT][(offset>>1)+1] = rsp_dmem[(addr+1)^3] << 8;
                rsp_reg.v[__RT][(offset>>1)+2] = rsp_dmem[(addr+2)^3] << 8;
                rsp_reg.v[__RT][(offset>>1)+3] = rsp_dmem[(addr+3)^3] << 8;
                rsp_reg.v[__RT][(offset>>1)+4] = rsp_dmem[(addr+4)^3] << 8;
                rsp_reg.v[__RT][(offset>>1)+5] = rsp_dmem[(addr+5)^3] << 8;
                rsp_reg.v[__RT][(offset>>1)+6] = rsp_dmem[(addr+6)^3] << 8;
                rsp_reg.v[__RT][(offset>>1)+7] = rsp_dmem[(addr+7)^3] << 8;
                break;

            case 0x07:

                addr = __RSP_O(3) + rsp_reg.r[__RS];
                addr &= 0xfff;

    // BUG: ??? offset should be removed ??? 
                for(i=0; i<8; i++)
                {
                        rsp_reg.v[__RT][((offset)>>1)+i] = rsp_dmem[(addr+i)^3] << 7;
                }
                break;

            case 0x08:

                addr = __RSP_O(4) + rsp_reg.r[__RS];
                addr &= 0xfff;

    // BUG: ??? offset should be removed ??? 
                for(i=0; i < (16 - (addr & 0xf)); i+=2)
                {
                        rsp_reg.v[__RT][(offset+i)>>1] = rsp_dmem[(addr+i)^3] << 7;
                }
                break;

            case 0x09:

                addr = __RSP_O(4) + rsp_reg.r[__RS];
                addr &= 0xfff;

                for(i=0; i < (16 - (addr & 0xf)); i+=4)
                {
                        rsp_reg.v[__RT][(offset>>1)+(i>>2)] = rsp_dmem[(addr+i)^3] << 7;
                }
                break;

            case 0x0a:

                addr = __RSP_O(4) + rsp_reg.r[__RS];
                addr &= 0xfff;

                for(i=0; i<16; i++)
                {
                        if(i >= offset)
                        {
                                if(i & 1)
                                {
                                        rsp_reg.v[__RT][i>>1] &= 0xff00;
                                        rsp_reg.v[__RT][i>>1] |= rsp_dmem[(addr+i-offset)^3];
                                }
                                else
                                {
                                        rsp_reg.v[__RT][i>>1] &= 0x00ff;
                                        rsp_reg.v[__RT][i>>1] |= rsp_dmem[(addr+i-offset)^3] << 8;
                                }
                        }
                        else
                        {
                                if(i & 1)
                                {
                                        rsp_reg.v[__RT][i>>1] &= 0xff00;
                                        rsp_reg.v[__RT][i>>1] |= rsp_dmem[(addr+i+16-offset)^3];
                                }
                                else
                                {
                                        rsp_reg.v[__RT][i>>1] &= 0x00ff;
                                        rsp_reg.v[__RT][i>>1] |= rsp_dmem[(addr+i+16-offset)^3] << 8;
                                }
                        }
                }
                break;

            case 0x0b:

                addr = __RSP_O(4) + rsp_reg.r[__RS];
                addr &= 0xfff;

                for(i=0; i<8; i++)
                {
                        rsp_reg.v[__RT+i][(8-(offset>>1)+i)&7] = (rsp_dmem[(addr+(i<<1))^3] << 8) | rsp_dmem[(addr+(i<<1)+1)^3];
                }
                break;

            default: 
				break;

                

        } // switch(__RD) 
		*/

}





//_____________________________________________________________________
// SWC2
//
void rsp_swc2()
{
    //    _s32   addr;
  //      int     offset = __SA >> 1;
//        int     i;

(swc2_instruction[__RD])();
return;

/*        switch(__RD)
        {
            case 0x00:

                addr = __RSP_O(0) + rsp_reg.r[__RS];
                addr &= 0xfff;

                if(offset & 1)
                        rsp_dmem[addr^3] = (_u8)rsp_reg.v[__RT][offset>>1];
                else
                        rsp_dmem[addr^3] = (_u8)rsp_reg.v[__RT][offset>>1] >> 8;
                break;

            case 0x01:

                addr = __RSP_O(1) + rsp_reg.r[__RS];
                addr &= 0xfff;

                rsp_dmem[addr^3] = rsp_reg.v[__RT][offset>>1] >> 8;
                rsp_dmem[(addr+1)^3] = (_u8)rsp_reg.v[__RT][offset>>1];
                
                break;

            case 0x02:

                addr = __RSP_O(2) + rsp_reg.r[__RS];
                addr &= 0xfff;

                rsp_dmem[addr^3] = (_u8)(rsp_reg.v[__RT][offset>>1] >> 8);
                rsp_dmem[(addr+1)^3] = (_u8)(rsp_reg.v[__RT][offset>>1]);
                rsp_dmem[(addr+2)^3] = (_u8)(rsp_reg.v[__RT][(offset>>1)+1] >> 8);
                rsp_dmem[(addr+3)^3] = (_u8)rsp_reg.v[__RT][(offset>>1)+1];
                break;

            case 0x03:

                addr = __RSP_O(3) + rsp_reg.r[__RS];
                addr &= 0xfff;

                rsp_dmem[addr^3] = (_u8)(rsp_reg.v[__RT][offset>>1] >> 8);
                rsp_dmem[(addr+1)^3] = (_u8)(rsp_reg.v[__RT][offset>>1]);
                rsp_dmem[(addr+2)^3] = (_u8)(rsp_reg.v[__RT][(offset>>1)+1] >> 8);
                rsp_dmem[(addr+3)^3] = (_u8)(rsp_reg.v[__RT][(offset>>1)+1]);
                rsp_dmem[(addr+4)^3] = (_u8)(rsp_reg.v[__RT][(offset>>1)+2] >> 8);
                rsp_dmem[(addr+5)^3] = (_u8)(rsp_reg.v[__RT][(offset>>1)+2]);
                rsp_dmem[(addr+6)^3] = (_u8)(rsp_reg.v[__RT][(offset>>1)+3] >> 8);
                rsp_dmem[(addr+7)^3] = (_u8)(rsp_reg.v[__RT][(offset>>1)+3]);
                break;

            case 0x04:

                addr = __RSP_O(4) + rsp_reg.r[__RS];
                addr &= 0xfff;

                for(i=0; i < (16 - (addr & 0xf)); i++)
                {
                        if((offset+i) & 1)
                                rsp_dmem[(addr+i)^3] = (_u8)(rsp_reg.v[__RT][(offset+i)>>1]);
                        else
                                rsp_dmem[(addr+i)^3] = (_u8)(rsp_reg.v[__RT][(offset+i)>>1] >> 8);
                }
                break;

            case 0x05:

                addr = __RSP_O(4) + rsp_reg.r[__RS];
                addr &= 0xfff;

                for(i=0; i < (addr & 0xf); i++)
                {
                        if((offset+i) & 1)
                                rsp_dmem[(addr-i-1)^3] = (_u8)(rsp_reg.v[__RT][7-(i>>1)] >> 8);
                        else
                                rsp_dmem[(addr-i-1)^3] = (_u8)(rsp_reg.v[__RT][7-(i>>1)]);
                }
                break;

            case 0x06:

                addr = __RSP_O(3) + rsp_reg.r[__RS];
                addr &= 0xfff;

    // BUG: ??? offset should be removed ??? 
                rsp_dmem[addr^3] = (_u8)(rsp_reg.v[__RT][offset>>1] >> 8);
                rsp_dmem[(addr+1)^3] = (_u8)(rsp_reg.v[__RT][(offset>>1)+1] >> 8);
                rsp_dmem[(addr+2)^3] = (_u8)(rsp_reg.v[__RT][(offset>>1)+2] >> 8);
                rsp_dmem[(addr+3)^3] = (_u8)(rsp_reg.v[__RT][(offset>>1)+3] >> 8);
                rsp_dmem[(addr+4)^3] = (_u8)(rsp_reg.v[__RT][(offset>>1)+4] >> 8);
                rsp_dmem[(addr+5)^3] = (_u8)(rsp_reg.v[__RT][(offset>>1)+5] >> 8);
                rsp_dmem[(addr+6)^3] = (_u8)(rsp_reg.v[__RT][(offset>>1)+6] >> 8);
                rsp_dmem[(addr+7)^3] = (_u8)(rsp_reg.v[__RT][(offset>>1)+7] >> 8);
                break;

            case 0x07:

                addr = __RSP_O(3) + rsp_reg.r[__RS];
                addr &= 0xfff;

    // BUG: ??? offset should be removed ??? 
                for(i=0; i<8; i++)
                {
                        rsp_dmem[(addr+i)^3] = rsp_reg.v[__RT][((offset)>>1)+i] >> 7;
                }
                break;

            case 0x08:

                addr = __RSP_O(4) + rsp_reg.r[__RS];
                addr &= 0xfff;

    // BUG: ??? offset should be removed ??? 
                for(i=0; i < (16 - (addr & 0xf)); i+=2)
                {
                        rsp_dmem[(addr+i)^3] = rsp_reg.v[__RT][(offset+i)>>1] >> 7;
                }
                break;

            case 0x09:

                addr = __RSP_O(4) + rsp_reg.r[__RS];
                addr &= 0xfff;

    // BUG: ??? offset should be removed ??? 
                for(i=0; i < (16 - (addr & 0xf)); i+=4)
                {
                        rsp_dmem[(addr+i)^3] = rsp_reg.v[__RT][(offset>>1)+(i>>2)] >> 7;
                }
                break;

            case 0x0a:

                addr = __RSP_O(4) + rsp_reg.r[__RS];
                addr &= 0xfff;

                for(i=0; i<16; i++)
                {
                        if(i >= offset)
                        {
                                if(i & 1)
                                        rsp_dmem[(addr+i-offset)^3] = (_u8)(rsp_reg.v[__RT][i>>1]);
                                else
                                        rsp_dmem[(addr+i-offset)^3] = (_u8)(rsp_reg.v[__RT][i>>1] >> 8);
                        }
                        else
                        {
                                if(i & 1)
                                        rsp_dmem[(addr+i+16-offset)^3] = (_u8)(rsp_reg.v[__RT][i>>1]);
                                else
                                        rsp_dmem[(addr+i+16-offset)^3] = (_u8)(rsp_reg.v[__RT][i>>1] >> 8);
                        }
                }
                break;

            case 0x0b:

                addr = __RSP_O(4) + rsp_reg.r[__RS];
                addr &= 0xfff;

                // The following was VERY VERY hard to figure out!!! 
                for(i=0; i<8; i++)
                {
                        _s32   o = offset ? addr + (16-offset) : addr;
                        rsp_dmem[((o & ~0x0f) | ((o + (i<<1)) & 0x0f))^3] = rsp_reg.v[__RT+i][(8-(offset>>1)+i)&7] >> 8;
                        rsp_dmem[(((o & ~0x0f) | ((o + (i<<1)) & 0x0f)) + 1)^3] = (_u8)(rsp_reg.v[__RT+i][(8-(offset>>1)+i)&7]);
                }
                break;

            default:
				break;

                

        } // switch(__RD) 
*/
}

//_____________________________________________________________________
// RESERVED
//
void rsp_reserved()
{
	char buffer[256];
	sprintf (buffer, "Unknown? opcode: %08X\n", iCODE);
	MessageBox (NULL, buffer, "Unknown Opcode Error", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// SLL
//
void rsp_special_sll()
{
	rsp_reg.r[__RD] = (_u32)rsp_reg.r[__RT] << __SA;
}

//_____________________________________________________________________
// SRL
//
void rsp_special_srl()
{
	rsp_reg.r[__RD] = (_u32)rsp_reg.r[__RT] >> __SA;
}

//_____________________________________________________________________
// SRA
//
void rsp_special_sra()
{
	rsp_reg.r[__RD] = (_s32)rsp_reg.r[__RT] >> __SA;
}

//_____________________________________________________________________
//SLLV
//
void rsp_special_sllv()
{
	rsp_reg.r[__RD] = rsp_reg.r[__RT] << (rsp_reg.r[__RS] & 0x1f);
}

//_____________________________________________________________________
// SRLV
//
void rsp_special_srlv()
{
	rsp_reg.r[__RD] = (_u32)rsp_reg.r[__RT] >> (rsp_reg.r[__RS] & 0x1f);
}

//_____________________________________________________________________
// SRAV
//
void rsp_special_srav()
{
	rsp_reg.r[__RD] = (_s32)rsp_reg.r[__RT] >> (rsp_reg.r[__RS] & 0x1f);
}

//_____________________________________________________________________
// JR
//
void rsp_special_jr()
{
	rsp_reg.delay = DO_RSP_DELAY;
	rsp_reg.pc_delay = 0x04000000 | (rsp_reg.r[__RS] & 0x1fff);
}

//_____________________________________________________________________
// JALR
//
void rsp_special_jalr()
{
	rsp_reg.delay = DO_RSP_DELAY;
	rsp_reg.pc_delay = 0x04000000 | (rsp_reg.r[__RS] & 0x1fff);
	rsp_reg.r[__RD] = (sp_reg_pc + 8) & 0x1fff;
}

//_____________________________________________________________________
// BREAK
//
void rsp_special_break()
{
	rsp_reg.halt = 1;
/*        PRINT_DEBUGGING_INFO("BREAK", NOTHING)

        ((_u32 *)mem.sp_reg)[4] |= 0x00000203;

        if(((_u32 *)mem.sp_reg)[4] & 0x40)
                reg.do_or_check_sthg |= RS4300I_DO_SP_INTERRUPT;

        rsp_reg.halt = 1;
        //puts(ST_FG_BRO "RSP BREAK: RSP halted!" ST_FG_DEF);
        //fflush(stdout);
*/
}

//_____________________________________________________________________
// ADD
//
void rsp_special_add()
{
	rsp_reg.r[__RD] = rsp_reg.r[__RS] + rsp_reg.r[__RT];
}

//_____________________________________________________________________
// ADDU
//
void rsp_special_addu()
{
	rsp_reg.r[__RD] = rsp_reg.r[__RS] + rsp_reg.r[__RT];
}

//_____________________________________________________________________
// SUB
//
void rsp_special_sub()
{
	rsp_reg.r[__RD] = rsp_reg.r[__RS] - rsp_reg.r[__RT];
}

//_____________________________________________________________________
// SUBU
//
void rsp_special_subu()
{
	rsp_reg.r[__RD] = rsp_reg.r[__RS] - rsp_reg.r[__RT];
}

//_____________________________________________________________________
// AND
//
void rsp_special_and()
{
	rsp_reg.r[__RD] = rsp_reg.r[__RS] & rsp_reg.r[__RT];
}

//_____________________________________________________________________
// OR
//
void rsp_special_or()
{
	rsp_reg.r[__RD] = rsp_reg.r[__RS] | rsp_reg.r[__RT];
}

//_____________________________________________________________________
// XOR
//
void rsp_special_xor()
{
	rsp_reg.r[__RD] = rsp_reg.r[__RS] ^ rsp_reg.r[__RT];
}

//_____________________________________________________________________
// NOR
//
void rsp_special_nor()
{
	rsp_reg.r[__RD] = ~(rsp_reg.r[__RS] | rsp_reg.r[__RT]);
}

//_____________________________________________________________________
// SLT
//
void rsp_special_slt()
{
	if(rsp_reg.r[__RS] < rsp_reg.r[__RT])
		rsp_reg.r[__RD] = 1;
	else
		rsp_reg.r[__RD] = 0;
}

//_____________________________________________________________________
// SLTU
//
void rsp_special_sltu()
{
	if( ((_u32)rsp_reg.r[__RS]) < ((_u32)rsp_reg.r[__RT]) )
		rsp_reg.r[__RD] = 1;
	else
		rsp_reg.r[__RD] = 0;
}

//_____________________________________________________________________
// SPECIAL RESERVED
//
void rsp_special_reserved()
{
	_assert_(0);
}

//_____________________________________________________________________
// BLTZ
//
void rsp_regimm_bltz()
{
	if( ((_s32)rsp_reg.r[__RS]) < 0 )
	{
		rsp_reg.delay = DO_RSP_DELAY;
		rsp_reg.pc_delay = sp_reg_pc + 4 + (__I << 2);
	}
}

//_____________________________________________________________________
// BGEZ
//
void rsp_regimm_bgez()
{
	if( ((_s32)rsp_reg.r[__RS]) >= 0 )
	{
		rsp_reg.delay = DO_RSP_DELAY;
		rsp_reg.pc_delay = sp_reg_pc + 4 + (__I << 2);
	}
}

//_____________________________________________________________________
// BLTZAL
//
void rsp_regimm_bltzal()
{
	rsp_reg.r[31] = sp_reg_pc + 8;

	if( ((_s32)rsp_reg.r[__RS]) < 0 )
	{
		rsp_reg.delay = DO_RSP_DELAY;
		rsp_reg.pc_delay = sp_reg_pc + 4 + (__I << 2);
	}
}

//_____________________________________________________________________
// BGEZAL
//
void rsp_regimm_bgezal()
{
	rsp_reg.r[31] = sp_reg_pc + 8;

	if( ((_s32)rsp_reg.r[__RS]) >= 0 )
	{
		rsp_reg.delay = DO_RSP_DELAY;
		rsp_reg.pc_delay = sp_reg_pc + 4 + (__I << 2);
	}
}

//_____________________________________________________________________
// 
//
void rsp_regimm_reserved()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop0_mf()
{
    if(__RD < 8)
	{
		switch(__RD)
		{
		case 0:	//
			rsp_reg.r[__RT] = 0;
			break;
		case 1:	//SP_DRAM_ADDR_REG
			rsp_reg.r[__RT] = 0;
			break;
		case 2:	//
			rsp_reg.r[__RT] = 0;
			break;
		case 3:	//
			rsp_reg.r[__RT] = 0;
			break;
		case 4:	//SP_STATUS_REG
			rsp_reg.r[__RT] = 0;
			break;
		case 5:	//SP_DMA_FULL_REG
			rsp_reg.r[__RT] = 0;
			break;
		case 6:	//SP_DMA_BUSY_REG
			rsp_reg.r[__RT] = 0;
			break;
		case 7:	//SP_SEMAPHORE_REG
			rsp_reg.r[__RT] = 0;
			break;
		}
	}
    else
	{
		switch(__RD & ~0x08)
		{
		case 0:	//DPC_START_REG
			rsp_reg.r[__RT] = 0;
			break;
		case 1:	//DPC_END_REG
			rsp_reg.r[__RT] = 0;
			break;
		case 2:	//DPC_CURRENT_REG
			rsp_reg.r[__RT] = 0;
			break;
		case 3:	//DPC_STATUS_REG
			rsp_reg.r[__RT] = 0;
			break;
		case 4:	//DPC_CLOCK_REG
			rsp_reg.r[__RT] = 0;
			break;
		case 5:	//DPC_BUFBUSY_REG
			rsp_reg.r[__RT] = 0;
			break;
		case 6:	//DPC_PIPEBUSY_REG
			rsp_reg.r[__RT] = 0;
			break;
		case 7:	//DPC_TMEM_REG
			rsp_reg.r[__RT] = 0;
			break;
		}
	}
}

//_____________________________________________________________________
// 
//
_u32 DMEM_Address, RDRAM_Address;
void rsp_cop0_mt()
{
    if(__RD < 8)
	{
		switch(__RD)
		{
		case 0:	//
			DMEM_Address = (rsp_reg.r[__RT] & 0x1FFF);
			break;
		case 1:	//SP_DRAM_ADDR_REG
			RDRAM_Address = (rsp_reg.r[__RT] & 0xFFFFFF);
			break;
		case 2:	// WRITE TO DMEM
			{
				_u32 length = rsp_reg.r[__RT];
				_u32 i;
				for(i=0; i<=length; i++)
				{
					 pDMEM[(i+DMEM_Address)^3] = pRDRAM[(i+RDRAM_Address)^3];
				}
				//memcpy(&pDMEM[DMEM_Address], &pRDRAM[RDRAM_Address], length+1)
			}
			break;
		case 3:	// READ
			{
				_u32 length = rsp_reg.r[__RT];
				_u32 i;
				for(i=0; i<=length; i++)
				{
					pRDRAM[(i+RDRAM_Address)^3] = pDMEM[(i+DMEM_Address)^3];
				}
				//memcpy(&pRDRAM[RDRAM_Address], &pDMEM[DMEM_Address], length+1);
			}

		case 4:	//SP_STATUS_REG
		case 5:	//SP_DMA_FULL_REG
		case 6:	//SP_DMA_BUSY_REG
		case 7:	//SP_SEMAPHORE_REG
			break;
		default:
			MBox("MFC0 SP unknown %x (pc = %08x)", __RD, sp_reg_pc);
			break;
		}
	}
    else
	{
		switch(__RD & ~0x08)
		{
		case 0:	//DPC_START_REG
		case 1:	//DPC_END_REG
		case 2:	//DPC_CURRENT_REG
		case 3:	//DPC_STATUS_REG
		case 4:	//DPC_CLOCK_REG
		case 5:	//DPC_BUFBUSY_REG
		case 6:	//DPC_PIPEBUSY_REG
		case 7:	//DPC_TMEM_REG
		default:
			MBox("MFC0 DPC unknown %x (pc = %08x)", __RD, sp_reg_pc);
			break;
		}
	}

}

//_____________________________________________________________________
// 
//
void rsp_cop0_reserved()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_mf()
{ // Fixed 02-03-2001 - Endianess Issue
	_u8 offs;

	offs = 15-((__SA >> 1) & 0x0f);
       
	//low = rsp_reg.v[__RD]->U8[offs];
	//high = rsp_reg.v[__RD]->U8[offs+1];
	rsp_reg.r[__RT] = (_s32)rsp_reg.v[__RD]->S16[offs/2];//(_s32)(_s16)((high << 8) | low);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_cf()
// 01-13-2001 - Made it into a switch for 100% accuracy - Azimer
{
	switch (__RD) {
		case 0:
			rsp_reg.r[__RT] = rsp_reg.flag[0];
		break;
		case 1:
			rsp_reg.r[__RT] = rsp_reg.flag[1];
		break;
		case 2:
			rsp_reg.r[__RT] = rsp_reg.flag[2];
		break;
		case 3:
			rsp_reg.r[__RT] = rsp_reg.flag[2];
		break;
	}
}

//_____________________________________________________________________
// Moves the lower 16-bit portion of a GPR to an element
//
void rsp_cop2_mt() // Still an issue with offs not being an even number.  Will fix later
{
	_u8    offs;

	offs = 15-((__SA >> 1) & 0x0f);
	rsp_reg.v[__RD]->U16[offs/2] = (_u16)rsp_reg.r[__RT];
}

//_____________________________________________________________________
// 
//
void rsp_cop2_ct()
// 01-13-2001 - Made it into a switch for 100% accuracy - Azimer
{
	switch (__RD) {
		case 0:
			rsp_reg.flag[0] = (_u16)rsp_reg.r[__RT];
		break;
		case 1:
			rsp_reg.flag[1] = ((_u16)rsp_reg.r[__RT]);
		break;
		case 2:
			rsp_reg.flag[2] = ((_u16)rsp_reg.r[__RT] & 0xFF);
		break;
		case 3:
			rsp_reg.flag[2] = ((_u16)rsp_reg.r[__RT] & 0xFF);
		break;
	}
}

//_____________________________________________________________________
// cop2 vectop instruction
//
void rsp_cop2_vectop()
{
	(cop2_vectop_instruction[__F])();
}

//_____________________________________________________________________
// 
//
void rsp_cop2_reserved()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//

void rsp_cop2_vectop_vmulf() // Works well... - 01-10-2001 - Azimer
{
	int i;
	FillDummyVector(__RT, __RS & 0xF);

	for (i=0; i<8; i++)
	{
		_u64 temp = (rsp_reg.v[__RD]->S16[i] * (((_s64)rsp_reg.dummy->S16[i]) << 1));
		if (temp & 0x8000)
			temp = (temp^0x8000) + 0x10000;
		else
			temp = (temp^0x8000);

		rsp_reg.accum[i] = (temp & 0xFFFFFFFFFFFF);
			
		temp = (_s32)(temp >> 16);
		if ((_s32)temp > 32767) 
			temp = 32767;
		if ((_s32)temp < -32768) 
			temp = -32768;
		rsp_reg.v[__SA]->U16[i] = (_u16)(temp);
	} 
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vmulu()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vrndp()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vmulq()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vmudl() 
// Works... had a lil sign problem at first - 01-11-2001 - Azimer
{
	int i;
	FillDummyVector(__RT, __RS & 0xF);

	for (i=0; i<8; i++)
	{
		_u64 temp = ((_u32)rsp_reg.v[__RD]->U16[i] * (_u32)rsp_reg.dummy->U16[i]);
		rsp_reg.accum[i] = ((temp >> 16) & 0xFFFFFFFFFFFF);
		rsp_reg.v[__SA]->S16[i] = (_s16)(temp >> 16);
	} 
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vmudm() // Works good now... - 01-10-2001 - Azimer
{
	int i;
	FillDummyVector(__RT, __RS & 0xF);

	for (i=0; i<8; i++)
	{
		_u64 temp = (rsp_reg.v[__RD]->S16[i] * rsp_reg.dummy->U16[i]);
		rsp_reg.accum[i] = (temp & 0xFFFFFFFFFFFF);
		rsp_reg.v[__SA]->S16[i] = (_s16)(temp >> 16);
	} 
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vmudn() // Works fine... - 01-10-20001 - Azimer
{
	int i;

	FillDummyVector(__RT, __RS & 0xF);

	for (i=0; i<8; i++) {
		_u64 temp = (rsp_reg.v[__RD]->U16[i] * rsp_reg.dummy->S16[i]);
		rsp_reg.accum[i] = (temp & 0xFFFFFFFFFFFF);
		rsp_reg.v[__SA]->U16[i] = (_u16)temp;		
	}
}

//_____________________________________________________________________
// 
//

void rsp_cop2_vectop_vmudh() // Works well now - 01-10-2001 - Azimer
{
	int i;
	FillDummyVector(__RT, __RS & 0xF);

	for (i=0; i<8; i++)
	{
		_u64 temp = (rsp_reg.v[__RD]->S16[i] * rsp_reg.dummy->S16[i]);
		
		rsp_reg.accum[i] = ((temp << 16) & 0xFFFFFFFFFFFF);
		
		if ((_s32)temp > 32767) temp = 32767;
		if ((_s32)temp < -32768) temp = -32768;

		rsp_reg.v[__SA]->S16[i] = (_s16)temp;
	}
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vmacf() // Works... there seems to be no round down. - 01-10-2001 - Azimer
{
	int i;
	FillDummyVector(__RT, __RS & 0xF);

	for (i=0; i<8; i++)
	{
		_u64 temp = (rsp_reg.v[__RD]->S16[i] * (((_s64)rsp_reg.dummy->S16[i]) << 1));

		rsp_reg.accum[i] += (temp & 0xFFFFFFFFFFFF);
		temp = rsp_reg.accum[i];
			
		temp = (_s32)(temp >> 16);
		if ((_s32)temp > 32767) 
			temp = 32767;
		if ((_s32)temp < -32768) 
			temp = -32768;
		rsp_reg.v[__SA]->U16[i] = (_u16)(temp);
	} 
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vmacu()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vrndn()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vmacq()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vmadl()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//

void rsp_cop2_vectop_vmadm() // Works well... thanks zilly! - 01-10-2001 - Azimer
{
	int i;

	FillDummyVector(__RT, __RS & 0xF);

	for (i=0; i<8; i++)
	{
		_s64 temp = (rsp_reg.v[__RD]->S16[i] * rsp_reg.dummy->U16[i]);
		rsp_reg.accum[i] +=  temp;
		rsp_reg.accum[i] &= 0xFFFFFFFFFFFF;
		temp = rsp_reg.accum[i];

		temp = (temp >> 0x10); // Middle part

		if ((_s32)temp < -32768) 
			temp = 0x8000;
		else if ((_s32)temp > 32767) 
			temp = 0x7FFF;

		rsp_reg.v[__SA]->S16[i] = (_s16)(temp);
	} 
}

//_____________________________________________________________________
// 
//

void rsp_cop2_vectop_vmadn() // Complete - 01-10-2001 - Azimer
{
	int i;
	_s32 clamper;

	FillDummyVector(__RT, __RS & 0xF);

	LogMe (dfile, "%08X: VMADN $v%i, $v%i, $v%1[%i]\n", sp_reg_pc, __SA, __RD, __RT, (__RS)&0xF);
	for (i=0; i<8; i++) {
		_s64 temp = (rsp_reg.v[__RD]->U16[i] * rsp_reg.dummy->S16[i]);
		LogMe (dfile, "Before Accum[%i] = %08X%08X\n", i, (rsp_reg.accum[i] >> 32), rsp_reg.accum[i] & 0xFFFFFFFF); // 48bit Accumulator!!!
		rsp_reg.accum[i] += temp;
		rsp_reg.accum[i] &= 0xFFFFFFFFFFFF;
		
		temp = rsp_reg.accum[i];
		clamper = (_s32)(temp >> 16);

		if (clamper < -32768) 
			temp = 0x0000;
		else if (clamper > 32767) 
			temp = 0xFFFF;
		rsp_reg.v[__SA]->U16[i] = (_u16)temp;
		LogMe (dfile, "After  Accum[%i] = %08X%08X\n", i, (rsp_reg.accum[i] >> 32), rsp_reg.accum[i] & 0xFFFFFFFF); // 48bit Accumulator!!!
		LogMe (dfile, "rsp_reg.v[%i]->S16[%i] (%04X) = rsp_reg.v[%i]->U16[%i] (%04X) * rsp_reg.dummy->S16[%i] (%04X)\n",
			__SA, i, rsp_reg.v[__SA]->S16[i],
			__RD, i, rsp_reg.v[__RD]->U16[i],
			i, rsp_reg.dummy->S16[i]);
	}
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vmadh() // It works nice - 01-09-2001 - Azimer
{
	int i;
	FillDummyVector(__RT, __RS & 0xF);

	for (i=0; i<8; i++)
	{
		_s64 temp = (rsp_reg.v[__RD]->S16[i] * rsp_reg.dummy->S16[i]);
		rsp_reg.accum[i] +=  (temp << 16);
		rsp_reg.accum[i] &= 0xFFFFFFFFFFFF;
		
		temp = (rsp_reg.accum[i] >> 16);

		if ((_s32)temp > 32767) 
			temp = 32767;
		else if ((_s32)temp < -32768) 
			temp = -32768;
		rsp_reg.v[__SA]->S16[i] = (_s16)temp;
	} 
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vadd() // Works now - 01-09-2001 - Azimer
{
	int i;
	int temp;
	FillDummyVector(__RT, (__RS) & 0xF);

	for (i = 0; i < 8; i++) {
		temp = (int)(short)rsp_reg.v[__RD]->U16[i] + (int)(short)rsp_reg.dummy->U16[i];
		if (rsp_reg.flag[0] & (1 << (7 - i)))
			temp++;
		if (temp > 32767)
			temp = 0x7FFF;
		else if (temp < -32768)
			temp = 0x8000;
		rsp_reg.v[__SA]->U16[i] = (_u16)temp;
	}
	rsp_reg.flag[0] = 0;
}

//_____________________________________________________________________
// Vector Subtraction
//
void rsp_cop2_vectop_vsub() // Works... just like VADD - 01-09-2001 - Azimer
{
	int i;
	int temp;
	FillDummyVector(__RT, (__RS) & 0xF);

	for (i = 0; i < 8; i++) {
		temp = (int)(short)rsp_reg.v[__RD]->U16[i] - (int)(short)rsp_reg.dummy->U16[i];
		if (rsp_reg.flag[0] & (1 << (7 - i)))
			temp--;
		if (temp > 32767)
			temp = 0x7FFF;
		else if (temp < -32768)
			temp = 0x8000;
		rsp_reg.v[__SA]->U16[i] = (_u16)temp;
	}
	rsp_reg.flag[0] = 0;
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vsut()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vabs()
{
	_assert_(0);
}

//_____________________________________________________________________
// Vector Addition and set Carry
// 
void rsp_cop2_vectop_vaddc() // Works - 01-09-2001 - Azimer
{
	int i;
	_u32 temp;
	FillDummyVector(__RT, (__RS) & 0xF);

	rsp_reg.flag[0] = 0;
	for (i = 0; i < 8; i++) {
		temp = (_u32)rsp_reg.v[__RD]->U16[i] + (_u32)rsp_reg.dummy->U16[i];
		if (temp & 0xffff0000)
			rsp_reg.flag[0] |= (1 << (7 - i));
		rsp_reg.v[__SA]->U16[i] = (_u16)temp;
	}
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vsubc() // Works perfectly... thanks zilly - 01-09-2001 - Azimer
{
	int i;
	_u32 temp;
	_u16 tmpflag = rsp_reg.flag[0];
	FillDummyVector(__RT, (__RS) & 0xF);

	rsp_reg.flag[0] = 0;
	for (i = 0; i < 8; i++) {
		temp = (_u32)rsp_reg.v[__RD]->U16[i] - (_u32)rsp_reg.dummy->U16[i];

		if ((temp & 0xffff) != 0)
			rsp_reg.flag[0] |= (1 << (15 - i));
		if ((temp & 0xffff0000) != 0)
			rsp_reg.flag[0] |= (1 << (7 - i));


		rsp_reg.v[__SA]->U16[i] = (_u16)temp;
	}
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vaddb()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vsubb()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vaccb()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vsucb()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vsad()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vsac()
{
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vsum()
{
	_assert_(0);
}

//_____________________________________________________________________
// Read out the Accu
//
void rsp_cop2_vectop_vsaw() // Works now - 01-10-2001 - F|RES
							// comment: possible that we have to exchange
							// 0x08 and 0x0a
{
	int i;
	switch((_u8)(__RS & 0xF))
	{
	case 0x08:
		for (i = 0; i<8; i++)
		{
			rsp_reg.v[__SA]->U16[i] = (_u16)(rsp_reg.accum[i]>>32); 
		}
		break;
	case 0x09:
		for (i = 0; i<8; i++)
		{
			rsp_reg.v[__SA]->U16[i] = (_u16)(rsp_reg.accum[i]>>16); 
		}
		break;
	case 0x0a:
		for (i = 0; i<8; i++)
		{
			rsp_reg.v[__SA]->U16[i] = (_u16)(rsp_reg.accum[i]>>0); 
		}
		break;
	default:
		_assert_(0);
	}
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vlt() // Works! - 2-5-2001 - Azimer
{
	int i;
	FillDummyVector(__RT, (__RS) & 0xF);

	LogMe (dfile, "%08X: VLT $v%i, $v%i, $v%i[%i]\n", sp_reg_pc, __SA, __RD, __RT, (__RS)&0xF);
	LogMe (dfile, "VLT flag[0] is: %04X VLT flag[1]: %04X\n", rsp_reg.flag[0],  rsp_reg.flag[1]);
	rsp_reg.flag[0] = rsp_reg.flag[1];
	rsp_reg.flag[1] = 0;
	for (i=0; i<8; i++) {
		LogMe (dfile, "rsp_reg.v[%i]->U16[%i] (%X) <= rsp_reg.dummy->U16[%i] (%X)", 
			__RD, i, rsp_reg.v[__RD]->U16[i], i, rsp_reg.dummy->U16[i]);
		
		if ((rsp_reg.flag[0] & (1 << (7-i))) == 0) {
			if (rsp_reg.v[__RD]->S16[i] <= rsp_reg.dummy->S16[i]) {
				//rsp_reg.flag[1] |= (1 << (7 - i)); // TODO: Correct this!
				rsp_reg.v[__SA]->S16[i] = rsp_reg.v[__RD]->S16[i];
				LogMe (dfile, " : TRUE\n");
			} else {
				rsp_reg.v[__SA]->S16[i] = rsp_reg.dummy->S16[i];
				LogMe (dfile, " : FALSE\n");
			}
		} else {
			LogMe (dfile, " : SKIP\n");
			rsp_reg.v[__SA]->S16[i] = rsp_reg.dummy->S16[i];
		}
	}
	rsp_reg.flag[0] = 0;
	LogMe (dfile, "VLT flag[0] is: %04X VLT flag[1]: %04X\n", rsp_reg.flag[0],  rsp_reg.flag[1]);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_veq() 
// Worked nicely!   - 01-08-2001 - Azimer
// Minor Correction - 01-11-2001 - Azimer
{
	int i;
	FillDummyVector(__RT, (__RS) & 0xF);

	LogMe (dfile, "%08X: VEQ $v%i, $v%i, $v%i[%i]\n", sp_reg_pc, __SA, __RD, __RT, (__RS)&0xF);
	LogMe (dfile, "VEQ flag[0] is: %04X VEQ flag[1]: %04X\n", rsp_reg.flag[0],  rsp_reg.flag[1]);
	rsp_reg.flag[1] = 0;
	for (i=0; i<8; i++) {
		if (rsp_reg.v[__RD]->U16[i] == rsp_reg.dummy->U16[i]) {
			if ((rsp_reg.flag[0] & (1 << (15-i))) == 0) {
				rsp_reg.flag[1] |= (1 << (7 - i));
			}
		}
		LogMe (dfile, "rsp_reg.v[%i]->U16[%i] (%X) == rsp_reg.dummy->U16[%i] (%X)\n", 
			__RD, i, rsp_reg.v[__RD]->U16[i], i, rsp_reg.dummy->U16[i]);
		rsp_reg.v[__SA]->U16[i] = rsp_reg.dummy->U16[i];
	}
	rsp_reg.flag[0] = 0;
	LogMe (dfile, "VEQ flag[0] is: %04X VEQ flag[1]: %04X\n", rsp_reg.flag[0],  rsp_reg.flag[1]);
}
//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vne() // Works! - 2-5-2001 - Azimer
{
	int i;
	FillDummyVector(__RT, (__RS) & 0xF);

	LogMe (dfile, "%08X: VNE $v%i, $v%i, $v%i[%i]\n", sp_reg_pc, __SA, __RD, __RT, (__RS)&0xF);
	LogMe (dfile, "VNE flag[0] is: %04X VNE flag[1]: %04X\n", rsp_reg.flag[0],  rsp_reg.flag[1]);
	rsp_reg.flag[1] = 0;
	for (i=0; i<8; i++) {
		if ((rsp_reg.flag[0] >> (15-i) & 1) == 1) {
				rsp_reg.flag[1] |= (1 << (7 - i));
		} else { if (rsp_reg.v[__RD]->U16[i] != rsp_reg.dummy->U16[i]) {
				rsp_reg.flag[1] |= (1 << (7 - i));
			}
		}
		rsp_reg.v[__SA]->U16[i] = rsp_reg.v[__RD]->U16[i];
		LogMe (dfile, "rsp_reg.v[%i]->U16[%i] (%X) == rsp_reg.dummy->U16[%i] (%X)\n", 
			__RD, i, rsp_reg.v[__RD]->U16[i], i, rsp_reg.dummy->U16[i]);
	}
	rsp_reg.flag[0] = 0;
	LogMe (dfile, "VNE flag[0] is: %04X VNE flag[1]: %04X\n", rsp_reg.flag[0],  rsp_reg.flag[1]);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vge() // Works! - 2-5-2001 - Azimer
{
	int i;
	FillDummyVector(__RT, (__RS) & 0xF);

	LogMe (dfile, "%08X: VGE $v%i, $v%i, $v%i[%i]\n", sp_reg_pc, __SA, __RD, __RT, (__RS)&0xF);
	LogMe (dfile, "VGE flag[0] is: %04X VGE flag[1]: %04X\n", rsp_reg.flag[0],  rsp_reg.flag[1]);
	rsp_reg.flag[0] = rsp_reg.flag[1];
	rsp_reg.flag[1] = 0;
	for (i=0; i<8; i++) {
		LogMe (dfile, "rsp_reg.v[%i]->U16[%i] (%X) >= rsp_reg.dummy->U16[%i] (%X)", 
			__RD, i, rsp_reg.v[__RD]->U16[i], i, rsp_reg.dummy->U16[i]);
		
		if ((rsp_reg.flag[0] & (1 << (7-i))) == 0) {
			if (rsp_reg.v[__RD]->S16[i] >= rsp_reg.dummy->S16[i]) {
				//rsp_reg.flag[1] |= (1 << (7 - i)); // TODO: Correct this...
				rsp_reg.v[__SA]->S16[i] = rsp_reg.v[__RD]->S16[i];
				LogMe (dfile, " : TRUE\n");
			} else {
				rsp_reg.v[__SA]->S16[i] = rsp_reg.dummy->S16[i];
				LogMe (dfile, " : FALSE\n");
			}
		} else {
			LogMe (dfile, " : SKIP\n");
			rsp_reg.v[__SA]->S16[i] = rsp_reg.dummy->S16[i];
		}
	}
	rsp_reg.flag[0] = 0;
	LogMe (dfile, "VGE flag[0] is: %04X VGE flag[1]: %04X\n", rsp_reg.flag[0],  rsp_reg.flag[1]);
}

//_____________________________________________________________________
// 
// Vector Clamp Low
void rsp_cop2_vectop_vcl() // Work in Progress - 01-11-2001 - Azimer
{
	int i;
	FillDummyVector(__RT, (__RS) & 0xF);
	rsp_reg.flag[1] = 0;
	for (i=0; i<8; i++) {
		if (rsp_reg.v[__RD]->S16[i] <= rsp_reg.dummy->S16[i])
			rsp_reg.v[__SA]->S16[i] = rsp_reg.v[__RD]->S16[i];
		else
			rsp_reg.v[__SA]->S16[i] = rsp_reg.dummy->S16[i];
		if (rsp_reg.v[__SA]->S16[i] == 0) 
			rsp_reg.flag[1] |= (1 << (7 - i));
	}

	//_assert_(0);
	/*
	int i;
	FillDummyVector(__RT, (__RS) & 0xF);
	rsp_reg.flag[1] = 0;
	LogMe (dfile, "VCL %i, %i, %i[%i]\n", __SA, __RD, __RT, ((__RS) & 0xF));
	LogMe (dfile, "Flags[1] = %04X, Flags[2] = %04X, Flags[3] = %04X\n", rsp_reg.flag[1], rsp_reg.flag[2], rsp_reg.flag[3]);
	for (i=0; i<8; i++) {
		if (rsp_reg.dummy->S16[i] == 0) {
			rsp_reg.v[__SA]->S16[i] = (_s32)rsp_reg.v[__RD]->S16[i];
		} else {
			if ((rsp_reg.flag[1] >> (7-i) & 1) == 0)			
				if (rsp_reg.v[__RD]->S16[i] < rsp_reg.dummy->S16[i])
					rsp_reg.v[__SA]->S16[i] = rsp_reg.v[__RD]->S16[i];
				else
					rsp_reg.v[__SA]->S16[i] = rsp_reg.dummy->S16[i];
			else
				if (rsp_reg.v[__RD]->S16[i] > rsp_reg.dummy->S16[i])
					rsp_reg.v[__SA]->S16[i] = rsp_reg.v[__RD]->S16[i];
				else
					rsp_reg.v[__SA]->S16[i] = rsp_reg.dummy->S16[i];
		}
		if (rsp_reg.v[__SA]->S16[i] == 0) 
			rsp_reg.flag[1] |= (1 << (7 - i));
		LogMe (dfile, "VCL: SA: %04X (%i), RD: %04X (%i), RT: %04X (%i)\n", 
			rsp_reg.v[__SA]->S16[i], rsp_reg.v[__SA]->S16[i],
			rsp_reg.v[__RD]->S16[i], rsp_reg.v[__RD]->S16[i],
			rsp_reg.dummy->S16[i], rsp_reg.dummy->S16[i]);
	}
	rsp_reg.flag[0] = 0;
	*/
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vch()
{
	MessageBox (NULL, "Needs VCH", "Unimplemented RSP op", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vcr()
{
	MessageBox (NULL, "Needs VCR", "Unimplemented RSP op", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vmrg()
{
	//MessageBox (NULL, "Needs VMRG", "Unimplemented RSP op", MB_OK);
	//_assert_(0);
}
/*
zilmar@work: (8:06 PM) 
void RSP_Vector_VMOV (void) {
	RSP_Vect[RSPOpC.sa].UHW[7 - (RSPOpC.rd & 0x7)] =
		RSP_Vect[RSPOpC.rt].UHW[EleSpec[RSPOpC.rs].B[(RSPOpC.rd & 0x7)]];
}

void RSP_Vector_VMRG (void) {
	int count, el, del;

	for (count = 0;count < 8; count++) {
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		if ((RSP_Flags[1].UW & ( 1 << (7 - el))) != 0) {
			RSP_Vect[RSPOpC.sa].HW[el] = RSP_Vect[RSPOpC.rd].HW[el];
		} else {
			RSP_Vect[RSPOpC.sa].HW[el] = RSP_Vect[RSPOpC.rt].HW[del];
		}
	}
}
*/

//_____________________________________________________________________
// Performs a bitwise AND operation
//
void rsp_cop2_vectop_vand() // Works well - 01-09-2001
{
	int i;

	FillDummyVector(__RT, __RS & 0xF);

	for (i=0; i<8; i++)
		rsp_reg.v[__SA]->U16[i] = rsp_reg.v[__RD]->U16[i] & rsp_reg.dummy->U16[i];
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vnand()  // Works! - 2-5-2001 - Azimer
{
	int i;

	FillDummyVector(__RT, __RS & 0xF);

	for (i=0; i<8; i++)
		rsp_reg.v[__SA]->U16[i] = ~(rsp_reg.v[__RD]->U16[i] & rsp_reg.dummy->U16[i]);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vor()  // Works! - 2-5-2001 - Azimer
{
	int i;

	FillDummyVector(__RT, __RS & 0xF);

	for (i=0; i<8; i++)
		rsp_reg.v[__SA]->U16[i] = rsp_reg.v[__RD]->U16[i] | rsp_reg.dummy->U16[i];
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vnor() // Works! - 2-5-2001 - Azimer
{
	int i;

	FillDummyVector(__RT, __RS & 0xF);

	for (i=0; i<8; i++)
		rsp_reg.v[__SA]->U16[i] = ~(rsp_reg.v[__RD]->U16[i] | rsp_reg.dummy->U16[i]);
}

//_____________________________________________________________________
// Performs a bitwise XOR operation
//
void rsp_cop2_vectop_vxor() // Works well - 01-09-2001
{
	int i;
	FillDummyVector(__RT, __RS & 0xF);

	for (i=0; i<8; i++)
		rsp_reg.v[__SA]->U16[i] = rsp_reg.v[__RD]->U16[i] ^ rsp_reg.dummy->U16[i];

}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vnxor() // Works well - 01-09-2001
{
	int i;
	FillDummyVector(__RT, __RS & 0xF);

	for (i=0; i<8; i++) {
		rsp_reg.v[__SA]->U16[i] = ~(rsp_reg.v[__RD]->U16[i] ^ rsp_reg.dummy->U16[i]);
	}
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vrcp()
{
	MessageBox (NULL, "Needs VRCP", "Unimplemented RSP op", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vrcpl()
{
	MessageBox (NULL, "Needs VRCPL", "Unimplemented RSP op", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vrcph()
{
	MessageBox (NULL, "Needs VRCPH", "Unimplemented RSP op", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vmov(_u32 Instruction)
{


//	CopyVector(__SA,__RT,__RS & 0xF);
//    MessageBox (NULL, "Needs VMOV", "Unimplemented RSP op", MB_OK);
//    _assert_(0);
//	{
//		int i;
//		FillDummyVector(__RT, __RS & 0xF);
//		FillDummyVector(__RT, __RS & 0x0);

//		for (i=0; i<8; i++) {
			rsp_reg.v[__SA]->U16[~__RD & 0x0f] = rsp_reg.v[__RT]->U16[~__RS & 0x0f];
//		}
//	}
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vrsq()
{
	MessageBox (NULL, "Needs VRSQ", "Unimplemented RSP op", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vrsql()
{
	MessageBox (NULL, "Needs VRSQL", "Unimplemented RSP op", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vrsqh()
{
	MessageBox (NULL, "Needs VRSQH", "Unimplemented RSP op", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vnoop()
{
	MessageBox (NULL, "Needs VNOOP", "Unimplemented RSP op", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vextt()
{
	MessageBox (NULL, "Needs VEXTT", "Unimplemented RSP op", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vextq()
{
	MessageBox (NULL, "Needs VEXTQ", "Unimplemented RSP op", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vextn()
{
	MessageBox (NULL, "Needs VEXTN", "Unimplemented RSP op", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vinst()
{
	MessageBox (NULL, "Needs VINST", "Unimplemented RSP op", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vinsq()
{
	MessageBox (NULL, "Needs VINSQ", "Unimplemented RSP op", MB_OK);
	_assert_(0);        
}

//_____________________________________________________________________
// 
//
void rsp_cop2_vectop_vinsn()
{
	MessageBox (NULL, "Needs VINSN", "Unimplemented RSP op", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// COP2 reserved
//
void rsp_cop2_vectop_reserved()
{
	MessageBox (NULL, "Reserved", "Bad Opcode", MB_OK);
	_assert_(0);
}

//_____________________________________________________________________
// 
//
void CheckForHLE();
_u32 code;
BOOL rsp_step()
{
	while (1) {
        /* execute instruction */

		//CheckForHLE();
		LogMe (dfile, "Executed at: %08X\n", sp_reg_pc);

		code = iCODE;
		__SA = iSA;
		__RT = iRT;
		__RD = iRD;
		__RS = iRS;
		__I  = iI;
		/*
		if (sp_reg_pc == 0x04001560)
			if (rsp_reg.v[27]->U16[7] != 0)
				__asm int 3;*/
	    (instruction[code>>26])();

	     /* calculate next pc */

        switch(rsp_reg.delay)
        {
            case EXEC_RSP_DELAY:   /* next instruction is at pc_delay */
                rsp_reg.delay = NO_RSP_DELAY;
                sp_reg_pc = rsp_reg.pc_delay;
                break;

            case DO_RSP_DELAY:   /* a delayed pc was stored into pc_delay      */
                       /* next instr is at pc+4 (case 0 is executed) */
                       /* next instr is at pc_delay                  */
                rsp_reg.delay = EXEC_RSP_DELAY;

            case NO_RSP_DELAY:    /* normal execution */
                sp_reg_pc += 4;

        } /* switch(rsp_reg.delay) */



        /* do common things */

        rsp_reg.r[0] = 0;
        rsp_reg.count++;

		if (rsp_reg.halt != 0)
			return FALSE;
		//return TRUE;
	}

} /* void rsp_step() */


void rsp_run()
{
#ifdef TEST_MODE
	dfile = fopen ("c:\\rspdebug.txt", "wt");
#endif

	rsp_dmem = pDMEM;
	rsp_imem = pIMEM;
	rsp_reset();

	sp_reg_pc = 0x04001000;
	LogMe (dfile, "RSP Started at %08X\n", sp_reg_pc);
	rsp_reg.halt = 0;

//	Debugger_Open();

	while( rsp_step() );
	LogMe (dfile, "RSP Left at %08X", sp_reg_pc);
#ifdef TEST_MODE
	Dump_RSP_UCode();
	fclose (dfile);
#endif
}
#endif