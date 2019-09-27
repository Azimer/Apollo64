#include <windows.h>
#include <stdio.h>
#include "WinMain.h"

extern u8 *idmem;
extern u8 *rdram;

//#define rdram (char *)AudioInfo.RDRAM
#define dmem idmem
#define imem (u8 *)(idmem + 0x1000)

char *rspreg[0x20] = {
  "R0", "AT", "V0", "V1", "A0", "A1", "A2", "A3", "T0", "T1", "T2",
  "T3", "T4", "T5", "T6", "T7", "S0", "S1", "S2", "S3", "S4", "S5",
  "S6", "S7", "T8", "T9", "K0", "K1", "GP", "SP", "S8", "RA"
};

int multi[0xC] = {
	0, 1, 2, 3, 4, 4, 3, 3, 4, 4, 4, 4
};

char *StoreVec[0xC] = {
	"SBV", "SSV", "SLV", "SDV", "SQV", "SRV", "SPV", "SUV", "SHV", "SFV", "SWV", "STV"
};

char *LoadVec[0xC] = {
	"LBV", "LSV", "LLV", "LDV", "LQV", "LRV", "LPV", "LUV", "LHV", "LFV", "LWV", "LTV"
};

char *Cop0Name[0x20] = {
	"SP memory address", "SP DRAM DMA address", "SP read DMA length", "SP write DMA length",
	"SP status", "SP DMA full", "SP DMA busy", "SP semaphore",
	"DP CMD DMA start", "DP CMD DMA end", "DP CMD DMA current", "DP CMD status",
	"DP clock counter", "DP buffer busy counter", "DP pipe busy counter", "DP TMEM load counter",
	"Unknown CPR0", "Unknown CPR0", "Unknown CPR0", "Unknown CPR0",
	"Unknown CPR0", "Unknown CPR0", "Unknown CPR0", "Unknown CPR0",
	"Unknown CPR0", "Unknown CPR0", "Unknown CPR0", "Unknown CPR0",
	"Unknown CPR0", "Unknown CPR0", "Unknown CPR0", "Unknown CPR0"
};

#define _RS		((opcode >> 21) & 0x1f)
#define _RT		((opcode >> 16) & 0x1f)
#define _RD		((opcode >> 11) & 0x1f)
#define _SA		((opcode >> 06) & 0x1f)
#define _IMM	((opcode & 0xFFFF))
#define _FUNC	((opcode & 0x3F))

char *LoadStoreOp [0xC] = {
	"LB", "LH", "??", "LW", "LBU", "LHU", "??", "??",
	"SB", "SH", "??", "SW"
};

char *Space (char *word) {
	static char retVal [9];
	int cnt;
	memset (retVal, 32, 8);
	retVal[8] = '\0';
	cnt = 0;
	while ((cnt < 8) && (word[cnt] != '\0')) {
		retVal[cnt] = word[cnt];
		cnt++;
	}
	return retVal;
}

char *VectOp [0x40] = {
	"VMULF", "VMULU", "VRNDP", "VMULQ", "VMUDL", "VMUDM", "VMUDN", "VMUDH",
	"VMACF", "VMACU", "VRNDN", "VMACQ", "VMADL", "VMADM", "VMADN", "VMADH",
	"VADD ", "VSUB ", "?????", "VABS ", "VADDC", "VSUBC", "?????", "?????",
	"?????", "?????", "?????", "?????", "?????", "VSAW ", "?????", "?????",
	"VLT  ", "VEQ  ", "VNE  ", "VGE  ", "VCL  ", "VCH  ", "VCR  ", "VMRG ",
	"VAND ", "VNAND", "VOR  ", "VNOR ", "VXOR ", "VNXOR", "?????", "?????",
	"VRCP ", "VRCPL", "VRCPH", "VMOV ", "VRSQ ", "VRSQL", "VRSQH", "VNOOP",
	"?????", "?????", "?????", "?????", "?????", "?????", "?????", "?????",
};

char *SHIFTS[0x8] = {
	"SLL", "", "SRL", "SRA", "SLLV", "SRLV", "SRLV", "SRAV"
};

char *LOGICS[0xC] = {
	"ADD", "ADDU", "SUB", "SUBU", "AND", "OR", "XOR", "NOR",
	"", "", "SLT", "SLTU"
};

char *ElemSpec[0x10] = {
	"", "", "[0q]", "[1q]", "[0h]", "[1h]", "[2h]", "[3h]",
	"[0]", "[1]", "[2]", "[3]", "[4]", "[5]", "[6]", "[7]"
};

void DumpRSPLine(char *buffer, DWORD sppc) {
	DWORD opcode;
	buffer[0] = '\0';
	opcode = *(DWORD *)(imem+(sppc&0xFFF));
	sprintf (buffer, "0x%04X  %08X", sppc&0xFFFF, opcode);
	buffer += 8;
	switch ((opcode >> 26)) {
		case 0x0: // NOOP
			switch (_FUNC) {

				case 0x0: // SLL/NOP
					if (_RD == 0) {
						sprintf (buffer, "NOP");
						break;
					}
				case 0x2: // SRL
				case 0x3: // SRA
				case 0x4: // SLLV
				case 0x6: // SRLV
				case 0x7: // SRAV
					sprintf (buffer, "%s %s, %s, 0x%02X", Space (SHIFTS[_FUNC]), rspreg[_RD], rspreg[_RT], _SA);
				break;
				case 0x8: // JR
					sprintf (buffer, "%s %s", Space ("JR"), rspreg[_RS]);
				break;
				case 0x9: // JALR
					sprintf (buffer, "%s %s, %s", Space ("JALR"), rspreg[_RD], rspreg[_RS]);
				break;
				case 0xD: // BREAK
					sprintf (buffer, "%s", Space ("BREAK"));
				break;
				case 0x20: // ADD
					if (_RS == 0)
						sprintf (buffer, "%s %s, %s", Space ("MOVE"), rspreg[_RD], rspreg[_RT]);
					else if (_RT == 0)
						sprintf (buffer, "%s %s, %s", Space ("MOVE"), rspreg[_RD], rspreg[_RS]);
					else
						sprintf (buffer, "%s %s, %s, %s", Space ("ADD"), rspreg[_RD], rspreg[_RS], rspreg[_RT]);
				break;
				case 0x21: // ADDU
				case 0x22: // SUB
				case 0x23: // SUBU
				case 0x24: // AND
				case 0x25: // OR
				case 0x26: // XOR
				case 0x27: // NOR
				case 0x2A: // SLT
				case 0x2B: // SLTU
					sprintf (buffer, "%s %s, %s, %s", Space (LOGICS[_FUNC-0x20]), rspreg[_RD], rspreg[_RS], rspreg[_RT]);
				break;
			}		
		break;
		case 0x1: // REG IMM
			switch (_RT) {
				case 0x00: // BLTZ
					sprintf (buffer, "%s %s, 0x%04X", Space ("BLTZ"), rspreg[_RS], ((_IMM << 2)+sppc+4)&0xFFFF);
				break;
				case 0x01: // BGEZ
					sprintf (buffer, "%s %s, 0x%04X", Space ("BGEZ"), rspreg[_RS], ((_IMM << 2)+sppc+4)&0xFFFF);
				break;
				case 0x10: // BLTZAL
					sprintf (buffer, "%s %s, 0x%04X", Space ("BLTZAL"), rspreg[_RS], ((_IMM << 2)+sppc+4)&0xFFFF);
				break;
				case 0x11: // BGEZAL
					sprintf (buffer, "%s %s, 0x%04X", Space ("BGEZAL"), rspreg[_RS], ((_IMM << 2)+sppc+4)&0xFFFF);
				break;
			}
		break;
		case 0x2: // J
			sprintf (buffer, "%s 0x%04X", Space("J"), ((_IMM << 2) & 0xFFFF));
		break;
		case 0x3: // JAL
			sprintf (buffer, "%s 0x%04X", Space("JAL"), ((_IMM << 2) & 0xFFFF));
		break;
		case 0x4: // BEQ
			if (_RT == 0) {
				sprintf (buffer, "%s %s, 0x%04X", Space ("BEQZ"), rspreg[_RS], ((_IMM << 2)+sppc+4)&0xFFFF);
			} else if (_RS == 0) {
				sprintf (buffer, "%s %s, 0x%04X", Space ("BEQZ"), rspreg[_RT], ((_IMM << 2)+sppc+4)&0xFFFF);
			} else {
				sprintf (buffer, "%s %s, %s, 0x%04X", Space ("BEQ"), rspreg[_RS], rspreg[_RT], ((_IMM << 2)+sppc+4)&0xFFFF);
			}
		break;
		case 0x5: // BNE
			if (_RT == 0) {
				sprintf (buffer, "%s %s, 0x%04X", Space ("BNEZ"), rspreg[_RS], ((_IMM << 2)+sppc+4)&0xFFFF);
			} else if (_RS == 0) {
				sprintf (buffer, "%s %s, 0x%04X", Space ("BNEZ"), rspreg[_RT], ((_IMM << 2)+sppc+4)&0xFFFF);
			} else {
				sprintf (buffer, "%s %s, %s, 0x%04X", Space ("BNE"), rspreg[_RS], rspreg[_RT], ((_IMM << 2)+sppc+4)&0xFFFF);
			}
		break;
		case 0x6: // BLEZ
			sprintf (buffer, "%s %s, 0x%04X", Space ("BLEZ"), rspreg[_RS], ((_IMM << 2)+sppc+4)&0xFFFF);
		break;
		break;
		case 0x7: // BGTZ
			sprintf (buffer, "%s %s, 0x%04X", Space ("BGTZ"), rspreg[_RS], ((_IMM << 2)+sppc+4)&0xFFFF);
		break;
		case 0x8: // ADDI
			sprintf (buffer, "%s %s, %s, 0x%04X", Space("ADDI"), rspreg[_RT], rspreg[_RS], _IMM); 
		break;
		case 0xC: // ANDI
			sprintf (buffer, "%s %s, %s, 0x%04X", Space("ANDI"), rspreg[_RT], rspreg[_RS], _IMM); 
		break;
		case 0xD: // ORI
			sprintf (buffer, "%s %s, %s, 0x%04X", Space("ORI"), rspreg[_RT], rspreg[_RS], _IMM); 
		break;
		case 0xF: // LUI
			sprintf (buffer, "%s %s, 0x%04X", Space("LUI"), rspreg[_RT], _IMM); 
		break;
		case 0x10: // COP0
			switch (_RS) {
				case 0x00: // MFC0
					sprintf (buffer, "%s %s, %s", Space("MFC0"), rspreg[_RT], Cop0Name[_RD]);
				break;
				case 0x04: // MTC0
					sprintf (buffer, "%s %s, %s", Space("MTC0"), rspreg[_RT], Cop0Name[_RD]);
				break;
			}
		break;
		case 0x12: // RSP Vector Ops
			if (_RS < 0x10) {
				switch (_RS) {
					case 0x00: // MFC2
						sprintf (buffer, "%s %s, $v%i [%i]", Space("MFC2"), rspreg[_RT], _RD, ((_SA >> 1) & 0xF));
					break;
					case 0x02: // CFC2
						sprintf (buffer, "%s %s, %i", Space("CFC2"), rspreg[_RT], (_RD & 0x3));
					break;
					case 0x04: // MTC2
						sprintf (buffer, "%s %s, $v%i [%i]", Space("MFC2"), rspreg[_RT], _RD, ((_SA >> 1) & 0xF));
					break;
					case 0x06: // CTC2
						sprintf (buffer, "%s %s, %i", Space("CTC2"), rspreg[_RT], (_RD & 0x3));
					break;
				}
			} else {
				if (VectOp[_FUNC][0] != '?') {
					sprintf (buffer, "%s $v%i, $v%i, $v%i %s", Space (VectOp[_FUNC]), _SA, _RD, _RT, ElemSpec[_RS&0xF]);
				}
			}
		break;
		case 0x20: // LB
		case 0x21: // LH
		case 0x23: // LW
		case 0x24: // LBU
		case 0x25: // LBH
		case 0x28: // SB
		case 0x29: // SH
		case 0x2B: // SW
			sprintf (buffer, "%s %s, 0x%04X (%s)", Space(LoadStoreOp[(opcode>>26)-0x20]), rspreg[_RT], _IMM, rspreg[_RS]);
		break;
		case 0x32: // LWC2
			sprintf (buffer, "%s $v%i [%i], 0x%04X (%s)", Space(LoadVec[_RD]), _RT, ((_SA>>1)&0xF), (_FUNC << multi[_RD]), rspreg[_RS]);
		break;
		case 0x3A: // SWC2
			sprintf (buffer, "%s $v%i [%i], 0x%04X (%s)", Space(StoreVec[_RD]), _RT, ((_SA>>1)&0xF), (_FUNC << multi[_RD]), rspreg[_RS]);
		break;
		default:
			break;
	}
}

void RspDump () {
	FILE *output;
	char Buffer[256];
	FILE *dfile;
	int i;
	u8 *dm = (u8 *)dmem;
	char str[0x10];
	dfile = fopen ("c:\\audio.txt", "wt");

	output = fopen ("c:\\rspdump.txt", "wt");

	for (DWORD sppc = 0xA4001000; sppc < 0xA4002000; sppc+=4) {
		DumpRSPLine(Buffer, sppc);
		strcat (Buffer, "\n");
		fprintf (output, Buffer);
	}
	fclose (output);

	for (i=0; i < 0x1000; i++) {
		if ((i & 0xF) == 0x0)
			fprintf (dfile, "%04X: ", i);
		fprintf (dfile, "%02X ", dm[i^3]);
		str[i&0xf] = dm[i^3];
		if ((i & 0xF) == 0xF)
			fprintf (dfile, "\n", str);
	}

	fclose (dfile);
	Debug (0, "Dumped RSP");
	//__asm int 3;
}