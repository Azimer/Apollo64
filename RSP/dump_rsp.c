#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"

typedef void (*rsp_instr)();
#define __F   ( (BYTE)(opcode) & 0x3F )
#define __SA  ( ((BYTE)(opcode >>  6)) & 0x1F )
#define __RD	  ( ((BYTE)(opcode >> 11)) & 0x1F )
#define __RT  ( ((BYTE)(opcode >> 16)) & 0x1F )
#define __RS  ( ((BYTE)(opcode >> 21)) & 0x1F )
#define __I   ( (unsigned __int16)(opcode) )
#define ____T ( opcode & 0x3ffffff)
#define __T   ( (Addr_counter & 0xf0000000) | (____T << 2) )
#define __O   ( (Addr_counter + 4 + (__I << 2)) & 0xFFFF)

char *Reg_Tb[] = { "R0", "AT", "V0", "V1", "A0", "A1", "A2", "A3", 
				   "T0", "T1", "T2", "T3", "T4", "T5", "T6", "T7", 
				   "S0", "S1", "S2", "S3", "S4", "S5", "S6", "S7",
				   "T8", "T9", "K0", "K1", "GP", "SP", "S8", "RA"};

char *Vec_Tb[] = { "[<none>]", "???", "[0q]", "[1q]", "[0h]", "[1h]", "[2h]", "[3h]",
				   "[0]", "[1]", "[2]", "[3]", "[4]", "[5]", "[6]", "[7]"};

char *Mt_Tb[] =  { "SP memory address",    "SP DRAM DMA address", 
			       "SP read DMA lenght",   "SP write DMA lenght", 
				   "SP status",			   "SP DMA full", 
				   "SP DMA busy",          "SP semaphore",
				   "DP CMD DMA start",     "DP CMD DMA end", 
				   "DP CMD DMA current",   "DP CMD status", 
				   "DP clock counter",	   "DP buffer busy counter", 
				   "DP pipe busy counter", "DP TMEM load counter"};

static void rsp_special();
static void rsp_regimm();
static void rsp_j();
static void rsp_jal();
static void rsp_beq();
static void rsp_bne();
static void rsp_blez();
static void rsp_bgtz();
static void rsp_addi();
static void rsp_addiu();
static void rsp_slti();
static void rsp_sltiu();
static void rsp_andi();
static void rsp_ori();
static void rsp_xori();
static void rsp_lui();
static void rsp_cop0();
static void rsp_cop2();
static void rsp_lb();
static void rsp_lh();
static void rsp_lw();
static void rsp_lbu();
static void rsp_lhu();
static void rsp_sb();
static void rsp_sh();
static void rsp_sw();
static void rsp_lwc2();
static void rsp_swc2();
static void rsp_reserved();



//** rsp special instructions 

static void rsp_special_sll();
static void rsp_special_srl();
static void rsp_special_sra();
static void rsp_special_sllv();
static void rsp_special_srlv();
static void rsp_special_srav();
static void rsp_special_jr();
static void rsp_special_jalr();
static void rsp_special_break();
static void rsp_special_add();
static void rsp_special_addu();
static void rsp_special_sub();
static void rsp_special_subu();
static void rsp_special_and();
static void rsp_special_or();
static void rsp_special_xor();
static void rsp_special_nor();
static void rsp_special_slt();
static void rsp_special_sltu();
static void rsp_special_reserved();



//** rsp regimm instructions 

static void rsp_regimm_bltz();
static void rsp_regimm_bgez();
static void rsp_regimm_bltzal();
static void rsp_regimm_bgezal();
static void rsp_regimm_reserved();



//** rsp cop0 instructions 

static void rsp_cop0_mf();
static void rsp_cop0_mt();
static void rsp_cop0_reserved();



//** rsp cop2 instructions 

static void rsp_cop2_mf();
static void rsp_cop2_cf();
static void rsp_cop2_mt();
static void rsp_cop2_ct();
static void rsp_cop2_vectop();
static void rsp_cop2_reserved();



//** rsp cop2 vectop instructions 

static void rsp_cop2_vectop_vmulf();
static void rsp_cop2_vectop_vmulu();
static void rsp_cop2_vectop_vrndp();
static void rsp_cop2_vectop_vmulq();
static void rsp_cop2_vectop_vmudl();
static void rsp_cop2_vectop_vmudm();
static void rsp_cop2_vectop_vmudn();
static void rsp_cop2_vectop_vmudh();
static void rsp_cop2_vectop_vmacf();
static void rsp_cop2_vectop_vmacu();
static void rsp_cop2_vectop_vrndn();
static void rsp_cop2_vectop_vmacq();
static void rsp_cop2_vectop_vmadl();
static void rsp_cop2_vectop_vmadm();
static void rsp_cop2_vectop_vmadn();
static void rsp_cop2_vectop_vmadh();
static void rsp_cop2_vectop_vadd();
static void rsp_cop2_vectop_vsub();
static void rsp_cop2_vectop_vsut();
static void rsp_cop2_vectop_vabs();
static void rsp_cop2_vectop_vaddc();
static void rsp_cop2_vectop_vsubc();
static void rsp_cop2_vectop_vaddb();
static void rsp_cop2_vectop_vsubb();
static void rsp_cop2_vectop_vaccb();
static void rsp_cop2_vectop_vsucb();
static void rsp_cop2_vectop_vsad();
static void rsp_cop2_vectop_vsac();
static void rsp_cop2_vectop_vsum();
static void rsp_cop2_vectop_vsaw();
static void rsp_cop2_vectop_vlt();
static void rsp_cop2_vectop_veq();
static void rsp_cop2_vectop_vne();
static void rsp_cop2_vectop_vge();
static void rsp_cop2_vectop_vcl();
static void rsp_cop2_vectop_vch();
static void rsp_cop2_vectop_vcr();
static void rsp_cop2_vectop_vmrg();
static void rsp_cop2_vectop_vand();
static void rsp_cop2_vectop_vnand();
static void rsp_cop2_vectop_vor();
static void rsp_cop2_vectop_vnor();
static void rsp_cop2_vectop_vxor();
static void rsp_cop2_vectop_vnxor();
static void rsp_cop2_vectop_vrcp();
static void rsp_cop2_vectop_vrcpl();
static void rsp_cop2_vectop_vrcph();
static void rsp_cop2_vectop_vmov();
static void rsp_cop2_vectop_vrsq();
static void rsp_cop2_vectop_vrsql();
static void rsp_cop2_vectop_vrsqh();
static void rsp_cop2_vectop_vnoop();
static void rsp_cop2_vectop_vextt();
static void rsp_cop2_vectop_vextq();
static void rsp_cop2_vectop_vextn();
static void rsp_cop2_vectop_vinst();
static void rsp_cop2_vectop_vinsq();
static void rsp_cop2_vectop_vinsn();
static void rsp_cop2_vectop_reserved();



//** rsp instruction function pointer 

static rsp_instr instruction[64] =
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


//** rsp special function pointer 

static rsp_instr special_instruction[64] =
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



//** rsp regimm function pointer 

static rsp_instr regimm_instruction[32] =
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



//** rsp cop0 function pointer 

static rsp_instr cop0_instruction[32] =
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



//** rsp cop2 function pointer 

static rsp_instr cop2_instruction[32] =
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



//** rsp cop2 vectop function pointer 

static rsp_instr cop2_vectop_instruction[64] =
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

DWORD opcode;
FILE *rsp_stream;
void Write_RSP(char *debug, ...)
{
	va_list argptr;
	char	text[1024];

	va_start(argptr, debug);
	vsprintf(text, debug, argptr);
	va_end(argptr);

	fprintf(rsp_stream,"%s", text);	
}

DWORD Addr_counter;
extern char *pIMEM;
void Dump_RSP_UCode()
{
	DWORD *code, i;
	rsp_stream = fopen("c:/rsp_dump","wt"); fprintf(rsp_stream,"RSP microcode\n");
	fclose(rsp_stream);
	rsp_stream = fopen("c:/rsp_dump","at");

	Addr_counter = 0x1000;

	code = (DWORD*)pIMEM;
	for (i=0; i<0x1000; i=i+4) 
	{
		opcode = *code;
		Write_RSP("%x: <%08x>  ", Addr_counter, opcode);
		instruction[opcode >> 26]();
		Write_RSP("\n");
		code++;
		Addr_counter += 4;
	}
	fclose(rsp_stream);
}




/******************************************************************************\
*                                                                              *
*   RSP (Reality Signal Processor)                                             *
*                                                                              *
\******************************************************************************/





static void rsp_special()
{
	special_instruction[__F]();
}





static void rsp_regimm()
{
	(regimm_instruction[__RT])();
}





static void rsp_j()
{
	Write_RSP("J         %08x", __T);
}





static void rsp_jal()
{
	Write_RSP("JAL	    %08x", __T);
}





static void rsp_beq()
{
	Write_RSP("BEQ       (%s==%s) --> %04x", 
		Reg_Tb[__RT], Reg_Tb[__RS], __O); 
}





static void rsp_bne()
{
	Write_RSP("BNE       (%s!=%s) --> %04x", 
		Reg_Tb[__RT], Reg_Tb[__RS], __O);
}





static void rsp_blez()
{
	Write_RSP("BLEZ      (%s<=0) --> %04x", 
		Reg_Tb[__RT], __O);
}





static void rsp_bgtz()
{
	Write_RSP("BGTZ      (%s>0) --> %04x", 
		Reg_Tb[__RT], __O);
}





static void rsp_addi()
{
	Write_RSP("ADDI      %s = %s + %04x", Reg_Tb[__RT], Reg_Tb[__RS], __I);
}





static void rsp_addiu()
{
	Write_RSP("ADDIU     %s = %s + %04x", Reg_Tb[__RT], Reg_Tb[__RS], __I);
}





static void rsp_slti()
{
	Write_RSP("SLTI ?-?");
}





static void rsp_sltiu()
{
	Write_RSP("SLTIU ?-?");
}





static void rsp_andi()
{
	Write_RSP("ANDI      %s = %s & %04x", Reg_Tb[__RT], Reg_Tb[__RS], __I);
}





static void rsp_ori()
{
	Write_RSP("ORI       %s = %s | %04x", Reg_Tb[__RT], Reg_Tb[__RS], __I);
}





static void rsp_xori()
{
	Write_RSP("XORI      %s = %s xor %04x", Reg_Tb[__RT], Reg_Tb[__RS], __I);
}





static void rsp_lui()
{
	Write_RSP("LUI       %s = %08x", Reg_Tb[__RT], __I << 16);
}





static void rsp_cop0()
{
	(cop0_instruction[__RS])();   
}





static void rsp_cop2()
{
	(cop2_instruction[__RS])();
}





static void rsp_lb()
{
	Write_RSP("LB        %s = [%s+%04x]", Reg_Tb[__RT], Reg_Tb[__RS], __I);
}





static void rsp_lh()
{
	Write_RSP("LH        %s = [%s+%04x]", Reg_Tb[__RT], Reg_Tb[__RS], __I);
}





static void rsp_lw()
{
	Write_RSP("LW        %s = [%s+%04x]", Reg_Tb[__RT], Reg_Tb[__RS], __I);
}





static void rsp_lbu()
{
	Write_RSP("LBU       %s = [%s+%04x]", Reg_Tb[__RT], Reg_Tb[__RS], __I);
}





static void rsp_lhu()
{
	Write_RSP("LHU       %s = [%s+%04x]", Reg_Tb[__RT], Reg_Tb[__RS], __I);
}





static void rsp_sb()
{
	Write_RSP("SB        [%s+%04x] = %s", Reg_Tb[__RS], __I,  Reg_Tb[__RT]);
}





static void rsp_sh()
{
	Write_RSP("SH        [%s+%04x] = %s", Reg_Tb[__RS], __I,  Reg_Tb[__RT]);
}





static void rsp_sw()
{
	Write_RSP("SW        [%s+%04x] = %s", Reg_Tb[__RS], __I,  Reg_Tb[__RT]);
}





static void rsp_lwc2()
{
	switch(__RD)
	{
		case 0x00:
			Write_RSP("LBV       vec%02i <%02i> = [%s + 0x%02x]", __RT, __SA >> 1, Reg_Tb[__RS], (opcode & 0xF));
			break;

		case 0x01:
			Write_RSP("LSV       vec%02i <%02i> = [%s + 0x%02x]", __RT, __SA >> 1, Reg_Tb[__RS], (opcode & 0xF) << 1);
			break;

		case 0x02:
			Write_RSP("LLV       vec%02i <%02i> = [%s + 0x%02x]", __RT, __SA >> 1, Reg_Tb[__RS], (opcode & 0xF) << 2);
			break;

		case 0x03:
			Write_RSP("LDV       vec%02i <%02i> = [%s + 0x%02x]", __RT, __SA >> 1, Reg_Tb[__RS], (opcode & 0xF) << 3);
			break;

		case 0x04:
			Write_RSP("LQV       vec%02i <%02i> = [%s + 0x%02x]", __RT, __SA >> 1, Reg_Tb[__RS], (opcode & 0xF) << 4);
			break;

		case 0x05:
			Write_RSP("LRV       vec%02i <%02i> = [%s + 0x%02x]", __RT, __SA >> 1, Reg_Tb[__RS], (opcode & 0xF) << 4);
			break;

		case 0x06:
			Write_RSP("LPV");
			break;

		case 0x07:
			Write_RSP("LUV");
			break;

		case 0x08:
			Write_RSP("LHV");
			break;

		case 0x09:
			Write_RSP("LFV");
			break;

		case 0x0a:
			Write_RSP("LWV");
			break;

		case 0x0b:
			Write_RSP("LTV");
			break;

		default:
			break;
	}
}





static void rsp_swc2()
{
	switch(__RD)
	{
		case 0x00:
			Write_RSP("SBV");
			break;

		case 0x01:
			Write_RSP("SSV       [%s + 0x%02x] = vec%02i <%02i>", Reg_Tb[__RT], (opcode & 0xF) << 1, __RD, __SA >> 1);
			break;

		case 0x02:
			Write_RSP("SLV       [%s + 0x%02x] = vec%02i <%02i>", Reg_Tb[__RT], (opcode & 0xF) << 3, __RD, __SA >> 1);
			break;

		case 0x03:
			Write_RSP("SDV       [%s + 0x%02x] = vec%02i <%02i>", Reg_Tb[__RT], (opcode & 0xF) << 2, __RD, __SA >> 1);
			break;

		case 0x04:
			Write_RSP("SQV       [%s + 0x%02x] = vec%02i <%02i>", Reg_Tb[__RT], (opcode & 0xF) << 4, __RD, __SA >> 1);
			break;

		case 0x05:
			Write_RSP("SRV       [%s + 0x%02x] = vec%02i <%02i>", Reg_Tb[__RT], (opcode & 0xF) << 4, __RD, __SA >> 1);
			break;

		case 0x06:
			Write_RSP("SPV");
			break;

		case 0x07:
			Write_RSP("SUV");
			break;

		case 0x08:
			Write_RSP("SHV");
			break;

		case 0x09:
			Write_RSP("SFV");
			break;

		case 0x0a:
			Write_RSP("SWV");
			break;

		case 0x0b:
			Write_RSP("STV");
			break;

		default:
			break;
	}
}






static void rsp_reserved()
{
	Write_RSP("RESEREVED");
}





static void rsp_special_sll()
{
	if (opcode == 0) Write_RSP("NOP       ");
	else
		Write_RSP("SLL       %s = %s<<%i", Reg_Tb[__RD], Reg_Tb[__RS], __SA);
}





static void rsp_special_srl()
{
	Write_RSP("SRL       %s = %s>>%i", Reg_Tb[__RD], Reg_Tb[__RS], __SA);
}





static void rsp_special_sra()
{
	Write_RSP("SRA       %s = %s>>%i", Reg_Tb[__RD], Reg_Tb[__RS], __SA);
}





static void rsp_special_sllv()
{
	Write_RSP("SLLV      %s = %s<<%s", Reg_Tb[__RT], Reg_Tb[__RD], Reg_Tb[__RS & 0x1f]);
}





static void rsp_special_srlv()
{
	Write_RSP("SRLV      %s = %s>>%s", Reg_Tb[__RT], Reg_Tb[__RD], Reg_Tb[__RS & 0x1f]);
}





static void rsp_special_srav()
{
	Write_RSP("SRAV      %s = %s>>%s", Reg_Tb[__RT], Reg_Tb[__RD], Reg_Tb[__RS & 0x1f]);
}





static void rsp_special_jr()
{
	Write_RSP("JR        %s", Reg_Tb[__RS]);
}





static void rsp_special_jalr()
{
	Write_RSP("JALR ?-?");
}





static void rsp_special_break()
{
	Write_RSP("BREAK");
}





static void rsp_special_add()
{
	Write_RSP("ADD       %s = %s+%s", Reg_Tb[__RD], Reg_Tb[__RS], Reg_Tb[__RT]);
}





static void rsp_special_addu()
{
	Write_RSP("ADDU      %s = %s+%s", Reg_Tb[__RD], Reg_Tb[__RS], Reg_Tb[__RT]);
}





static void rsp_special_sub()
{
	Write_RSP("SUB       %s = %s-%s", Reg_Tb[__RD], Reg_Tb[__RS], Reg_Tb[__RT]);
}





static void rsp_special_subu()
{
	Write_RSP("SUBU      %s = %s-%s", Reg_Tb[__RD], Reg_Tb[__RS], Reg_Tb[__RT]);
}





static void rsp_special_and()
{
	Write_RSP("AND       %s = %s & %s", Reg_Tb[__RD], Reg_Tb[__RS], Reg_Tb[__RT]);
}





static void rsp_special_or()
{
	Write_RSP("OR        %s = %s | %s", Reg_Tb[__RD], Reg_Tb[__RS], Reg_Tb[__RT]);
}





static void rsp_special_xor()
{
	Write_RSP("XOR       %s = %s xor %s", Reg_Tb[__RD], Reg_Tb[__RS], Reg_Tb[__RT]);
}





static void rsp_special_nor()
{
	Write_RSP("NOR       %s = %s nor %s", Reg_Tb[__RD], Reg_Tb[__RS], Reg_Tb[__RT]);
}





static void rsp_special_slt()
{
	Write_RSP("SLT ?-?");
}





static void rsp_special_sltu()
{
	Write_RSP("SLTU ?-?");
}





static void rsp_special_reserved()
{
	Write_RSP("SPECIAL RESERVED");
}




static void rsp_regimm_bltz()
{
	Write_RSP("BLTZ ?-?");
}





static void rsp_regimm_bgez()
{
	Write_RSP("BGEZ ?-?");
}





static void rsp_regimm_bltzal()
{
	Write_RSP("BLTZAL ?-?");
}





static void rsp_regimm_bgezal()
{
	Write_RSP("BGEZAL ?-?");
}





static void rsp_regimm_reserved()
{
	Write_RSP("REGIMM RESERVED");
}





static void rsp_cop0_mf()
{
	Write_RSP("MFC0      %s = %s", Reg_Tb[__RD], Mt_Tb[__RT & 0x1f]);
	
}





static void rsp_cop0_mt()
{
	Write_RSP("MTC0      %s = %s", Mt_Tb[__RT & 0x1f], Reg_Tb[__RD]);
}





static void rsp_cop0_reserved()
{
	Write_RSP("COP0 RESERVED");
}





static void rsp_cop2_mf()
{
	Write_RSP("MFC2      %s = vec%i <%02i>", Reg_Tb[__RT], __RD, (__SA>>1) & 0xF);
}





static void rsp_cop2_cf()
{
	Write_RSP("CFC2      %s = Flag[0x%x]", Reg_Tb[__RT], __RD & 0x03);
}





static void rsp_cop2_mt()
{
	Write_RSP("MTC2      vec%i <%02i> = %s", __RD, (__SA>>1) & 0xF, Reg_Tb[__RT]);
}





static void rsp_cop2_ct()
{
	Write_RSP("CTC2      Flag[0x%x] = %s",__RD & 0x03, Reg_Tb[__RT]);
}





static void rsp_cop2_vectop()
{
	(cop2_vectop_instruction[__F])();
}





static void rsp_cop2_reserved()
{
	Write_RSP("COP2_RESEVERED");
}





/* rsp cop2 vectop instructions */

static void rsp_cop2_vectop_vmulf()
{
	Write_RSP("VMULF     vec%02i = vec%02i * vec%02i%s", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vmulf() */





static void rsp_cop2_vectop_vmulu()
{
	Write_RSP("VMULU ?-?");
} /* static void rsp_cop2_vectop_vmulu() */





static void rsp_cop2_vectop_vrndp()
{
	Write_RSP("VRNDP ?-?");
} /* static void rsp_cop2_vectop_vrndp() */





static void rsp_cop2_vectop_vmulq()
{
	Write_RSP("VMULQ ?-?");
} /* static void rsp_cop2_vectop_vmulq() */





static void rsp_cop2_vectop_vmudl()
{
	Write_RSP("VMUDL     vec%02i = ( acc = vec%02i * vec%02i%s      )", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vmudl() */





static void rsp_cop2_vectop_vmudm()
{
	Write_RSP("VMUDM     vec%02i = ( acc = vec%02i * vec%02i%s >> 16)", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vmudm() */





static void rsp_cop2_vectop_vmudn()
{
	Write_RSP("VMUDN     vec%02i = ( acc = vec%02i * vec%02i%s      ) >> 16", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vmudn() */





static void rsp_cop2_vectop_vmudh()
{
	Write_RSP("VMUDH     vec%02i = ( acc = vec%02i * vec%02i%s >> 16) >> 16", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vmudh() */





static void rsp_cop2_vectop_vmacf()
{
	Write_RSP("VMACF     vec%02i+= vec%02i * vec%02i%s", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vmacf() */





static void rsp_cop2_vectop_vmacu()
{
	Write_RSP("VMACU ?-?");
} /* static void rsp_cop2_vectop_vmacu() */





static void rsp_cop2_vectop_vrndn()
{
	Write_RSP("VRDN ?-?");
} /* static void rsp_cop2_vectop_vrndn() */





static void rsp_cop2_vectop_vmacq()
{
	Write_RSP("VMACQ ?-?");
} /* static void rsp_cop2_vectop_vmacq() */





static void rsp_cop2_vectop_vmadl()
{
	Write_RSP("VMADL ?-?");
} /* static void rsp_cop2_vectop_vmadl() */





static void rsp_cop2_vectop_vmadm()
{
	Write_RSP("VMADM     vec%02i = ( acc+= vec%02i * vec%02i%s >> 16)", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vmadm() */





static void rsp_cop2_vectop_vmadn()
{
	Write_RSP("VMADN     vec%02i = ( acc+= vec%02i * vec%02i%s      ) >> 16", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vmadn() */





static void rsp_cop2_vectop_vmadh()
{
	Write_RSP("VMADH     vec%02i = ( acc+= vec%02i * vec%02i%s >> 16) >> 16", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vmadh() */





static void rsp_cop2_vectop_vadd()
{
	Write_RSP("VADD      vec%02i = vec%02i + vec%02i%s", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vadd() */





static void rsp_cop2_vectop_vsub()
{
	Write_RSP("VSUB      vec%02i = vec%02i - vec%02i%s", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vsub() */





static void rsp_cop2_vectop_vsut()
{
	Write_RSP("VSUT ?-?");
} /* static void rsp_cop2_vectop_vsut() */





static void rsp_cop2_vectop_vabs()
{
	Write_RSP("VABS ?-?");
} /* static void rsp_cop2_vectop_vabs() */





static void rsp_cop2_vectop_vaddc()
{
	Write_RSP("VADDC	vec%02i = vec%02i + vec%02i%s", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vaddc() */





static void rsp_cop2_vectop_vsubc()
{
	Write_RSP("VSUBC     vec%02i = vec%02i - vec%02i%s ??", __SA, __RD, __RT, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vsubc() */





static void rsp_cop2_vectop_vaddb()
{
	Write_RSP("VADDB ?-?");
} /* static void rsp_cop2_vectop_vaddb() */





static void rsp_cop2_vectop_vsubb()
{
	Write_RSP("VSUBB ?-?");
} /* static void rsp_cop2_vectop_vsubb() */





static void rsp_cop2_vectop_vaccb()
{
	Write_RSP("VACCB ?-?");
} /* static void rsp_cop2_vectop_vaccb() */





static void rsp_cop2_vectop_vsucb()
{
	Write_RSP("VSUCB ?-?");
} /* static void rsp_cop2_vectop_vsucb() */





static void rsp_cop2_vectop_vsad()
{
	Write_RSP("VSAD ?-?");
} /* static void rsp_cop2_vectop_vsad() */





static void rsp_cop2_vectop_vsac()
{
	Write_RSP("VSAC ?-?");
} /* static void rsp_cop2_vectop_vsac() */





static void rsp_cop2_vectop_vsum()
{
	Write_RSP("VSUM ?-?");
} /* static void rsp_cop2_vectop_vsum() */





static void rsp_cop2_vectop_vsaw()
{
	Write_RSP("VSAW $v%i, $v%i, $v%i[%s]", __SA, __RD, __RT, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vsaw() */





static void rsp_cop2_vectop_vlt()
{
	Write_RSP("VLT ?-?");
} /* static void rsp_cop2_vectop_vlt() */





static void rsp_cop2_vectop_veq()
{
	Write_RSP("VEQ       flag[1] = (vec%02i == vec%02i%s)", __RD, __RT, Vec_Tb[__RS & 0xF]);	
} /* static void rsp_cop2_vectop_veq() */





static void rsp_cop2_vectop_vne()
{
	Write_RSP("VNE ?-?");
} /* static void rsp_cop2_vectop_vne() */





static void rsp_cop2_vectop_vge()
{
	Write_RSP("VGE       vec%02i = (vec%02i >= vec%02i%s)", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vge() */





static void rsp_cop2_vectop_vcl()
{
	Write_RSP("VCL ?-?");
} /* static void rsp_cop2_vectop_vcl() */





static void rsp_cop2_vectop_vch()
{
	Write_RSP("VCH ?-?");
} /* static void rsp_cop2_vectop_vch() */





static void rsp_cop2_vectop_vcr()
{
	Write_RSP("VCR ?-?");
} /* static void rsp_cop2_vectop_vcr() */





static void rsp_cop2_vectop_vmrg()
{
	Write_RSP("VMRG ?-?");
} /* static void rsp_cop2_vectop_vmrg() */





static void rsp_cop2_vectop_vand()
{
	Write_RSP("VAND      vec%02i = vec%02i and vec%02i%s", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vand() */





static void rsp_cop2_vectop_vnand()
{
	Write_RSP("VNAND     vec%02i = vec%02i nand vec%02i%s", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vnand() */





static void rsp_cop2_vectop_vor()
{
	Write_RSP("VOR       vec%02i = vec%02i or vec%02i%s", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vor() */





static void rsp_cop2_vectop_vnor()
{
	Write_RSP("VNOR      vec%02i = vec%02i nor vec%02i%s", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vnor() */





static void rsp_cop2_vectop_vxor()
{
	Write_RSP("VXOR      vec%02i = vec%02i xor vec%02i%s", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);	
} /* static void rsp_cop2_vectop_vxor() */





static void rsp_cop2_vectop_vnxor()
{
	Write_RSP("VNXOR     vec%02i = vec%02i nxor vec%02i%s", __SA, __RT, __RD, Vec_Tb[__RS & 0xF]);
} /* static void rsp_cop2_vectop_vnxor() */





static void rsp_cop2_vectop_vrcp()
{
	Write_RSP("VRCP ?-?");
} /* static void rsp_cop2_vectop_vrcp() */





static void rsp_cop2_vectop_vrcpl()
{
	Write_RSP("VRCPL ?-?");
} /* static void rsp_cop2_vectop_vrcpl() */





static void rsp_cop2_vectop_vrcph()
{
	Write_RSP("VRCPH ?-?");
} /* static void rsp_cop2_vectop_vrcph() */





static void rsp_cop2_vectop_vmov()
{
	Write_RSP("VMOV ?-?");
} /* static void rsp_cop2_vectop_vmov() */





static void rsp_cop2_vectop_vrsq()
{
} /* static void rsp_cop2_vectop_vrsq() */





static void rsp_cop2_vectop_vrsql()
{
	Write_RSP("VRSQL ?-?");
} /* static void rsp_cop2_vectop_vrsql() */





static void rsp_cop2_vectop_vrsqh()
{
	Write_RSP("VRSQH ?-?");
} /* static void rsp_cop2_vectop_vrsqh() */





static void rsp_cop2_vectop_vnoop()
{
	Write_RSP("VNOOP");
} /* static void rsp_cop2_vectop_vnoop() */





static void rsp_cop2_vectop_vextt()
{
	Write_RSP("VEXTT ?-?");
} /* static void rsp_cop2_vectop_vextt() */





static void rsp_cop2_vectop_vextq()
{
	Write_RSP("VEXTQ ?-?");
} /* static void rsp_cop2_vectop_vextq() */





static void rsp_cop2_vectop_vextn()
{
	Write_RSP("VEXTN ?-?");
} /* static void rsp_cop2_vectop_vextn() */





static void rsp_cop2_vectop_vinst()
{
	Write_RSP("VINST ?-?");
} /* static void rsp_cop2_vectop_vinst() */





static void rsp_cop2_vectop_vinsq()
{
	Write_RSP("VINSQ ?-?");
} /* static void rsp_cop2_vectop_vinsq() */





static void rsp_cop2_vectop_vinsn()
{
	Write_RSP("VINSN ?-?");
} /* static void rsp_cop2_vectop_vinsn() */





static void rsp_cop2_vectop_reserved()
{
	Write_RSP("VECTOP RESEVERD");
} /* static void rsp_cop2_vectop_reserved() */
