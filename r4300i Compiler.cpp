
/*
** r4300i Recompiler, Copyright JaboSoft 1994,2002
** Liscensed to and modified by Apollo, Copyright Azimer 1997,2002
** 
** Entry Points:
**  void r4300iCompiler_Execute(void)
** 
** Revision list
** - 2001-11-02, initial revision (jabo)
** 
*/

#include <windows.h>
#include <stdio.h>
#include "WinMain.h"
#include "EmuMain.h"
#include "CpuMain.h"

extern void (*r4300i[0x40])();
extern void (*special[0x40])();
extern void (*regimm[0x20])();
extern void (*MmuSpecial[0x40])();
extern void (*MmuNormal[0x20])();
extern void (*FpuCommands[0x40])();
extern void CheckInterrupts(void);

#define R4300I_BLOCK_SIZE 0x1000
#define R4300I_READOPCODE(x) vr32((x))

typedef union {
	DWORD * udwPtr;
	WORD * uwPtr;
	BYTE * ubPtr;
	void * ptr;
} BUFFER_POINTER;

typedef struct {
	union {
		struct {
			unsigned int func:6;
			unsigned int sa:5;
			unsigned int rd:5;
			unsigned int rt:5;
			unsigned int rs:5;
			unsigned int op:6;
		};
		unsigned int UW;
	};
} R4300I_OPCODE;

class r4300iRecompiler {

	public:
		BUFFER_POINTER X86CodePos;
		R4300I_OPCODE CurrentOpcode;
		DWORD ProgramCounter, BranchCompare, BlockStart;

	private:
		BUFFER_POINTER X86Code, JumpTable, CheckTable, ParentTable;
		FILE * hStream;

	public:

		void SetJumpTableToPos(DWORD location) {
			location &= 0x7FFFFF;
			*(this->JumpTable.udwPtr + (location >> 2)) = (DWORD)this->X86CodePos.ptr;
		}

		void SetJumpTableEntry(DWORD location, void * CodePtr) {
			location &= 0x7FFFFF;
			*(this->JumpTable.udwPtr + (location >> 2)) = (DWORD)CodePtr;
		}

		void * GetJumpTableEntry(DWORD location) {
			location &= 0x7FFFFF;
			return (void*)*(this->JumpTable.udwPtr + (location >> 2));
		}

		void * GetJumpTablePtr(DWORD location) {
			location &= 0x7FFFFF;
			return (void*)(this->JumpTable.udwPtr + (location >> 2));
		/*	return (void*)&(this->JumpTable.udwPtr[(location >> 2)]); */
		}

		void SetCheckValue(DWORD location, DWORD Value) {
			location &= 0x7FFFFF;
			*(this->CheckTable.udwPtr + (location >> 2)) = Value;
		}

		DWORD GetCheckValue(DWORD location) {
			location &= 0x7FFFFF;
			return *(this->CheckTable.udwPtr + (location >> 2));
		}

		void SetParentAddress(DWORD Child, DWORD Parent) {
			Child &= 0x7FFFFF;
			*(this->ParentTable.udwPtr + (Child >> 2)) = Parent;
		}

		DWORD GetParentAddress(DWORD Child) {
			Child &= 0x7FFFFF;
			return *(this->ParentTable.udwPtr + (Child >> 2));
		}

		DWORD GetParentCheckValue(DWORD Child) {
			Child &= 0x7FFFFF;
			DWORD location = *(this->ParentTable.udwPtr + (Child >> 2));
			location &= 0x7FFFFFF;
			return *(this->CheckTable.udwPtr + (location >> 2));
		}

		void ResetCode(void) {
			memset(this->JumpTable.ptr, 0, 0x800000);
			memset(this->CheckTable.ptr, 0, 0x800000);
			memset(this->ParentTable.ptr, 0, 0x800000);
			memset(this->X86Code.ptr, 0, 0x1000000);

			this->X86CodePos.ptr = this->X86Code.ptr;
		}

		r4300iRecompiler() {
			/*
			 * Tables = 8 + 8 + 8 = 24 megs
			 * X86Cod = 16
			 */
			this->JumpTable.ptr = VirtualAlloc(NULL, 0x800000, MEM_COMMIT, PAGE_READWRITE);
			this->CheckTable.ptr = VirtualAlloc(NULL, 0x800000, MEM_COMMIT, PAGE_READWRITE);
			this->ParentTable.ptr = VirtualAlloc(NULL, 0x800000, MEM_COMMIT, PAGE_READWRITE);
			this->X86Code.ptr = VirtualAlloc(NULL, 0x1000000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

			if (this->JumpTable.ptr == NULL) {
				_asm { int 3 }
			}
			if (this->CheckTable.ptr == NULL) {
				_asm { int 3 }
			}
			if (this->ParentTable.ptr == NULL) {
				_asm { int 3 }
			}
			if (this->X86Code.ptr == NULL) {
				_asm { int 3 }
			}
			this->ResetCode();
			this->hStream = NULL;
		}

		~r4300iRecompiler() {
			VirtualFree(this->X86Code.ptr, 0, MEM_RELEASE);
			VirtualFree(this->JumpTable.ptr, 0, MEM_RELEASE);
			VirtualFree(this->CheckTable.ptr, 0, MEM_RELEASE);
			VirtualFree(this->ParentTable.ptr, 0, MEM_RELEASE);
			this->X86CodePos.ptr = NULL;
			this->X86Code.ptr = NULL;
			this->JumpTable.ptr = NULL;
			this->CheckTable.ptr = NULL;
			this->ParentTable.ptr = NULL;

			if (this->hStream != NULL) {
				fclose(this->hStream);
				this->hStream = NULL;
			}
		}

		void Log(char * Msg, ...);
};

void r4300iRecompiler::Log(char * Message, ...) {
	if (this->hStream == NULL) {
		char Path[256], Dir[256], Drive[10];

		GetModuleFileName(NULL, Path, sizeof(Path));
		_splitpath(Path, Drive, Dir, 0, 0);
		sprintf(Path, "%s%s%s", Drive, Dir, "RecompileX86Code.log");

		if (NULL == (this->hStream = fopen(Path, "a+"))) {
			return;
		}
	}

	char Msg[512];
	va_list ap;

	va_start(ap, Message);
	vsprintf(Msg, Message, ap);
	va_end(ap);

	fprintf(this->hStream, "%s\n", Msg);
}

r4300iRecompiler Recompiler;

/*************************** x86 ***************************/

void SetBranch8b(void * JumpByte, void * Destination) {
	/* calculate 32-bit relative offset */
	signed int n = (BYTE*)Destination - ((BYTE*)JumpByte + 1);
	*(BYTE*)(JumpByte) = (BYTE)n;
}

void SetBranch32b(void * JumpByte, void * Destination) {
	*(DWORD*)(JumpByte) = (DWORD)((BYTE*)Destination - (BYTE*)((DWORD*)JumpByte + 1));
}

void MoveConstToVariable (DWORD Const, void * Variable, char * VariableName) {
	Recompiler.Log("   mov [%s], 0x%08X", VariableName, Const);
	*(Recompiler.X86CodePos.uwPtr++) = 0x05C7;
    *(Recompiler.X86CodePos.udwPtr++) = (DWORD)Variable;
    *(Recompiler.X86CodePos.udwPtr++) = Const;
}

void CallFunctionDirect(void * FunctAddress, char * FunctName) {
	Recompiler.Log("   call %s", FunctName);
	*(Recompiler.X86CodePos.ubPtr++) = 0xE8;
	*(Recompiler.X86CodePos.udwPtr++) = (DWORD)FunctAddress-(DWORD)Recompiler.X86CodePos.ptr - 4;
}

void AddConstToVariable (DWORD Const, void * Variable, char * VariableName) {
	Recompiler.Log("   add [%s], 0x%08X", VariableName, Const);
	*(Recompiler.X86CodePos.uwPtr++) = 0x0581;
	*(Recompiler.X86CodePos.udwPtr++) = (DWORD)Variable;
	*(Recompiler.X86CodePos.udwPtr++) = Const;
}

void Ret(void) {
	Recompiler.Log("   ret");
	*(Recompiler.X86CodePos.ubPtr++) = 0xC3;
}

void MoveVariableToEax(void *Variable, char *VariableName) {
	Recompiler.Log("   mov eax, [%s]", VariableName);
	*(Recompiler.X86CodePos.uwPtr++) = 0x058B;
    *(Recompiler.X86CodePos.udwPtr++) = (DWORD)Variable;
}

void MoveEaxToVariable(void * Variable, char * VariableName) {
	Recompiler.Log("   mov dword ptr [%s], eax",VariableName);
	*(Recompiler.X86CodePos.uwPtr++) = 0x0589;
    *(Recompiler.X86CodePos.udwPtr++) = (DWORD)Variable;
}

void CompConstToVariable(DWORD Const, void * Variable, char * VariableName) {
	Recompiler.Log("   cmp dword ptr [%s], 0x%X",VariableName, Const);
	*(Recompiler.X86CodePos.uwPtr++) = 0x3D81;
	*(Recompiler.X86CodePos.udwPtr++) = (DWORD)Variable;
	*(Recompiler.X86CodePos.udwPtr++) = Const;
}

void CompEaxToVariable(void * Variable, char * VariableName) {
	Recompiler.Log("   cmp eax, dword ptr [%s]",VariableName);
	*(Recompiler.X86CodePos.uwPtr++) = 0x053B;
	*(Recompiler.X86CodePos.udwPtr++) = (DWORD)Variable;
}

void CompConstToEax(DWORD Const) {
	Recompiler.Log("   cmp eax, 0x%08X", Const);
	*(Recompiler.X86CodePos.uwPtr++) = 0xF881;
	*(Recompiler.X86CodePos.udwPtr++) = (DWORD)Const;
}

void JeLabel8(char * Label, BYTE Value) {
	Recompiler.Log("   je $%s", Label);
	*(Recompiler.X86CodePos.ubPtr++) = 0x74;
	*(Recompiler.X86CodePos.ubPtr++) = Value;
}

void JneLabel8(char * Label, BYTE Value) {
	Recompiler.Log("   jne $%s", Label);
	*(Recompiler.X86CodePos.ubPtr++) = 0x75;
	*(Recompiler.X86CodePos.ubPtr++) = Value;
}

void JgLabel8(char * Label, BYTE Value) {
	Recompiler.Log("   jle $%s", Label);
	*(Recompiler.X86CodePos.ubPtr++) = 0x7F;
	*(Recompiler.X86CodePos.ubPtr++) = Value;
}

void JumpEax(void) {
	Recompiler.Log("   jmp eax");
	*(Recompiler.X86CodePos.uwPtr++) = 0xe0ff;
}

void JmpIndirectLabel32(char * Label,DWORD location) {
	Recompiler.Log("   jmp dword ptr [%s]", Label);
	*(Recompiler.X86CodePos.uwPtr++) = 0x25ff;
	*(Recompiler.X86CodePos.udwPtr++) = location;
}

/************************** opcodes ************************/

void r4300iCompiler_Opcode(void);

void r4300i_Compiler_Branch(void * Helper, BOOL Link) {
	DWORD target = Recompiler.ProgramCounter + 4 + ((short)Recompiler.CurrentOpcode.UW << 2);
	BOOL bIsLikely = FALSE;
	BYTE * JumpByte, * JumpByte2;

	MoveConstToVariable(Recompiler.CurrentOpcode.UW, &sop, "GlobalOpcode");
	CallFunctionDirect(Helper, "BranchHelper");
	
	if (Recompiler.CurrentOpcode.op == 0x1) {
		if ((Recompiler.CurrentOpcode.func == 0x2) || (Recompiler.CurrentOpcode.func == 0x3)) {
			bIsLikely = TRUE;
		}
	} else if (Recompiler.CurrentOpcode.op & 0x10) {
		bIsLikely = TRUE;
	}

	/* compile delay slot (doesnt check interrupts, neither does interpreter) */
	if (FALSE == bIsLikely) {
		Recompiler.Log("\n   (- begin delay slot -)");
		Recompiler.ProgramCounter += 4;
		r4300iCompiler_Opcode();
		Recompiler.ProgramCounter -= 4;
		Recompiler.Log("   (- end delay slot -)\n");
	} 
	
	CompConstToVariable(TRUE, &Recompiler.BranchCompare, "BranchCompare");
			
	JneLabel8("DoNotJump", 0);
	JumpByte = Recompiler.X86CodePos.ubPtr - 1;

	if (Link == TRUE) {
		DWORD * RegRA = (DWORD*)&CpuRegs[31];
		MoveConstToVariable(Recompiler.ProgramCounter + 8, &RegRA[0], "RA.Lo");
		MoveConstToVariable(0, &RegRA[1], "RA.Hi");
	}

	if (TRUE == bIsLikely) {
		Recompiler.Log("\n   Branch Likely");
		Recompiler.Log("\n   (- begin delay slot -)");
		Recompiler.ProgramCounter += 4;
		r4300iCompiler_Opcode();
		Recompiler.ProgramCounter -= 4;
		Recompiler.Log("   (- end delay slot -)\n");
	}

	if (target >= Recompiler.BlockStart && target < Recompiler.BlockStart + 0x1000) {
	//if (0) {
		MoveVariableToEax(Recompiler.GetJumpTablePtr(target), "JumpRegAddress");
		CompConstToEax(0);
		JneLabel8("ExecuteJump", 0);
		JumpByte2 = Recompiler.X86CodePos.ubPtr - 1;
	
		MoveConstToVariable(target, &pc, "PC");
		Ret();

		Recompiler.Log("  ExecuteJump:");
		SetBranch8b(JumpByte2, Recompiler.X86CodePos.ptr);
		JumpEax();
	} else {
		MoveConstToVariable(target, &pc, "PC");
		Ret();
	}

	Recompiler.Log("  DoNotJump:");
	SetBranch8b(JumpByte, Recompiler.X86CodePos.ptr);

	/* delay slot is already done */
	Recompiler.ProgramCounter += 4;
}

void r4300i_Compiler_BEQ(void) {
	Recompiler.BranchCompare = (CpuRegs[sop.rt] == CpuRegs[sop.rs]) ? TRUE : FALSE;
}

void r4300i_Compiler_BNE(void) {
	Recompiler.BranchCompare = (CpuRegs[sop.rt] != CpuRegs[sop.rs]) ? TRUE : FALSE;
}

void r4300i_Compiler_BLEZ(void) {
	Recompiler.BranchCompare = (((s64)CpuRegs[sop.rs]) <= 0) ? TRUE : FALSE;
}

void r4300i_Compiler_BLTZ(void) {
	Recompiler.BranchCompare = (((s64)CpuRegs[sop.rs]) < 0) ? TRUE : FALSE;
}

void r4300i_Compiler_BGTZ(void) {
	Recompiler.BranchCompare = (((s64)CpuRegs[sop.rs]) > 0) ? TRUE : FALSE;
}

void r4300i_Compiler_BGEZ(void) {
	Recompiler.BranchCompare = (((s64)CpuRegs[sop.rs]) >= 0) ? TRUE : FALSE;
}

void * JumpRegAddress;	/* lazy */
DWORD JumpRegister;

void r4300i_Compiler_JumpRegHelper(void) {
	/*
	 * mov eax, register
	 * push eax
	 * call GetJumpTableEntry
	 * add esp, 4
	 * result = eax
	 */
	JumpRegAddress = Recompiler.GetJumpTableEntry((DWORD)CpuRegs[sop.rs]);
}

void r4300i_Compiler_JumpReg(BOOL Link) {
	BYTE * JumpByte;
/*
	MoveConstToVariable(Recompiler.CurrentOpcode.UW, &sop, "GlobalOpcode");
	CallFunctionDirect(r4300i_Compiler_JumpRegHelper, "r4300i_Compiler_JumpRegHelper");
*/
	MoveVariableToEax(&CpuRegs[sop.rs], "CPU_REG[rs]");
	MoveEaxToVariable(&JumpRegister, "JumpRegister");

	/* compile delay slot (doesnt check interrupts, neither does interpreter) */
	Recompiler.Log("\n   (- begin delay slot -)");
	Recompiler.ProgramCounter += 4;
	r4300iCompiler_Opcode();
	Recompiler.ProgramCounter -= 4;
	Recompiler.Log("   (- end delay slot -)\n");

	if (Link == TRUE) {
		DWORD * RegRA = (DWORD*)&CpuRegs[31];
		MoveConstToVariable(Recompiler.ProgramCounter + 8, &RegRA[0], "RA.Lo");
		MoveConstToVariable(0, &RegRA[1], "RA.Hi");
	}

	/* cant risk the self mod code */
	if (0) {
		MoveVariableToEax(&JumpRegAddress, "JumpRegAddress");
		CompConstToEax(0);
		JneLabel8("ExecuteJump", 0);
		JumpByte = Recompiler.X86CodePos.ubPtr - 1;

		MoveVariableToEax(&JumpRegister, "JumpRegister");
		MoveEaxToVariable(&pc, "PC");
		Ret();
		
		Recompiler.Log("  ExecuteJump:");
		SetBranch8b(JumpByte, Recompiler.X86CodePos.ptr);
		JumpEax();
	} else {
		MoveVariableToEax(&JumpRegister, "JumpRegister");
		MoveEaxToVariable(&pc, "PC");
		Ret();
	}

	/* delay slot is already done */
	Recompiler.ProgramCounter += 4;
}

void r4300i_Compiler_Jump(BOOL Link) {
	static void * tempTargetPtr;
	DWORD target = (Recompiler.ProgramCounter & 0xf0000000) + ((Recompiler.CurrentOpcode.UW << 2) & 0x0fffffff);

	/* compile delay slot (doesnt check interrupts, neither does interpreter) */
	Recompiler.Log("\n   (- begin delay slot -)");
	Recompiler.ProgramCounter += 4;
	r4300iCompiler_Opcode();
	Recompiler.ProgramCounter -= 4;
	Recompiler.Log("   (- end delay slot -)\n");

	if (Link == TRUE) {
		DWORD * RegRA = (DWORD*)&CpuRegs[31];
		MoveConstToVariable(Recompiler.ProgramCounter + 8, &RegRA[0], "RA.Lo");
		MoveConstToVariable(0, &RegRA[1], "RA.Hi");
	}

	if (target >= Recompiler.BlockStart && target < Recompiler.BlockStart + 0x1000) {
		BYTE * JumpByte;

		MoveVariableToEax(Recompiler.GetJumpTablePtr(target), "JumpTable");
		CompConstToEax(0);
		JneLabel8("ExecuteJump", 0);
		JumpByte = Recompiler.X86CodePos.ubPtr - 1;
		MoveConstToVariable(target, &pc, "PC");
		Ret();
		
		Recompiler.Log("  ExecuteJump:");
		SetBranch8b(JumpByte, Recompiler.X86CodePos.ptr);
		JumpEax();
	} else {
		MoveConstToVariable(target, &pc, "PC");
		Ret();
	}

	/* delay slot is already done */
	Recompiler.ProgramCounter += 4;
}

void r4300i_Compiler_ERET(void) {
	extern void opERET(void);
	CallFunctionDirect(&opERET,"opERET");
	Ret();
}

/***********************************************************/

void r4300iCompiler_UseInterpreter(void) {
	void * Function;

	switch (Recompiler.CurrentOpcode.op) {
	case 0x00: /* R4300I_SPECIAL */
		Function = special[Recompiler.CurrentOpcode.func];
		break;

	case 0x01: /* R4300I_REGIMMEDIATE */
		Function = regimm[Recompiler.CurrentOpcode.rt];
		break;

	case 0x10: /* R4300I_COP0 */
		if (Recompiler.CurrentOpcode.rs == 16) {
			Function = MmuSpecial[Recompiler.CurrentOpcode.func];
		} else {
			Function = MmuNormal[Recompiler.CurrentOpcode.rs];
		}
		break;

	default:
		Function = r4300i[Recompiler.CurrentOpcode.op];
		break;
	}

	MoveConstToVariable(Recompiler.CurrentOpcode.UW, &sop, "GlobalOpcode");
	CallFunctionDirect(Function, "r4300iOpcode");
}

extern void OpcodeLookup(DWORD, char *);

void r4300iCompiler_Opcode(void) {

	char Asm[256];
	OpcodeLookup(Recompiler.ProgramCounter, Asm);
	Recompiler.Log("[%08X] %s (X86: %08X)", Recompiler.ProgramCounter, Asm, Recompiler.X86CodePos);

	Recompiler.CurrentOpcode.UW = R4300I_READOPCODE(Recompiler.ProgramCounter);

	if (Recompiler.CurrentOpcode.UW == 0x00000000) {
		return;
	}

	switch (Recompiler.CurrentOpcode.op) {
	case 0x00: /* R4300I_SPECIAL */
		switch (Recompiler.CurrentOpcode.func) {
		case 0x08: r4300i_Compiler_JumpReg(FALSE); break;
		case 0x09: r4300i_Compiler_JumpReg(TRUE); break;
		default:
			Recompiler.Log("");
			r4300iCompiler_UseInterpreter();
		}
		break;

	case 0x01: /* R4300I_REGIMMEDIATE */
		switch (Recompiler.CurrentOpcode.rt) {
		case 0x00: r4300i_Compiler_Branch(&r4300i_Compiler_BLTZ, FALSE); break;
		case 0x01: r4300i_Compiler_Branch(&r4300i_Compiler_BGEZ, FALSE); break;
		case 0x02: r4300i_Compiler_Branch(&r4300i_Compiler_BLTZ, FALSE); break;
		case 0x03: r4300i_Compiler_Branch(&r4300i_Compiler_BGEZ, FALSE); break;
		case 0x10: r4300i_Compiler_Branch(&r4300i_Compiler_BLTZ, TRUE); break;
		case 0x11: r4300i_Compiler_Branch(&r4300i_Compiler_BGEZ, TRUE); break;
		case 0x12: Recompiler.Log("REGIMM_BLTZALL"); break;
		case 0x13: Recompiler.Log("REGIMM_BGEZALL"); break;

		default:
			Recompiler.Log("");
			r4300iCompiler_UseInterpreter();
		}
		break;

	case 0x02: r4300i_Compiler_Jump(FALSE); break;
	case 0x03: r4300i_Compiler_Jump(TRUE); break;
	case 0x04: r4300i_Compiler_Branch(&r4300i_Compiler_BEQ, FALSE); break;
	case 0x05: r4300i_Compiler_Branch(&r4300i_Compiler_BNE, FALSE); break;
	case 0x06: r4300i_Compiler_Branch(&r4300i_Compiler_BLEZ, FALSE); break;
	case 0x07: r4300i_Compiler_Branch(&r4300i_Compiler_BGTZ, FALSE); break;

	case 0x10: /* R4300I_COP0 */
		switch (Recompiler.CurrentOpcode.rs) {
		case 0x10:
			switch (Recompiler.CurrentOpcode.func) {
			case 0x18: /* COP0_ERET */
				r4300i_Compiler_ERET();				
				break;

			default:
				Recompiler.Log("");
				r4300iCompiler_UseInterpreter();
			}
			break;

		default:
			Recompiler.Log("");
			r4300iCompiler_UseInterpreter();
		}
		break;

	case 0x11: /* R4300I_COP1 */
		switch (Recompiler.CurrentOpcode.rs) {
		case 0x08: /* COP1_BRANCH */
			switch (Recompiler.CurrentOpcode.rt & 3){
			case 0x00: Recompiler.Log("COP1_BC1F"); break;
			case 0x01: Recompiler.Log("COP1_BC1T"); break;
			case 0x02: Recompiler.Log("COP1_BC1FL"); break;
			case 0x03: Recompiler.Log("COP1_BC1TL"); break;
				
			default:
				Recompiler.Log("");
				r4300iCompiler_UseInterpreter();
			}
			break;

		default:
			Recompiler.Log("");
			r4300iCompiler_UseInterpreter();
		}
		break;

	case 0x14: r4300i_Compiler_Branch(&r4300i_Compiler_BEQ, FALSE); break;
	case 0x15: r4300i_Compiler_Branch(&r4300i_Compiler_BNE, FALSE); break;
	case 0x16: r4300i_Compiler_Branch(&r4300i_Compiler_BLEZ, FALSE); break;
	case 0x17: r4300i_Compiler_Branch(&r4300i_Compiler_BGTZ, FALSE); break;

	default:
		Recompiler.Log("");
		r4300iCompiler_UseInterpreter();
	}
}

BOOL IsOpcodeBranch(DWORD ProgramCnt) {
	R4300I_OPCODE tmp;
	tmp.UW = R4300I_READOPCODE(ProgramCnt);

	switch (tmp.op) {
	case 0x00: /* R4300I_SPECIAL */
		switch (tmp.func) {
		case 0x08: /* SPECIAL_JR */
		case 0x09: /* SPECIAL_JALR */
			return TRUE;
		}
		break;

	case 0x01: /* R4300I_REGIMMEDIATE */
		switch (tmp.rt) {
		case 0x00: /* REGIMM_BLTZ */
		case 0x01: /* REGIMM_BGEZ */
		case 0x02: /* REGIMM_BLTZL */
		case 0x03: /* REGIMM_BGEZL */
		case 0x10: /* REGIMM_BLTZAL */
		case 0x11: /* REGIMM_BGEZAL */
		case 0x12: /* REGIMM_BLTZALL */
		case 0x13: /* REGIMM_BGEZALL */
			return TRUE;
		}
		break;

	case 0x02: /* R4300I_J */
	case 0x03: /* R4300I_JAL */
	case 0x04: /* R4300I_BEQ */
	case 0x05: /* R4300I_BNE */
	case 0x06: /* R4300I_BLEZ */
	case 0x07: /* R4300I_BGTZ */
		return TRUE;

	case 0x11: /* R4300I_COP1 */
		switch (tmp.rs) {
		case 0x08: /* COP1_BRANCH */
			return TRUE;
		}
		break;

	case 0x14: /* R4300I_BEQL */
	case 0x15: /* R4300I_BNEL */
	case 0x16: /* R4300I_BLEZL */
	case 0x17: /* R4300I_BGTZL */
		return TRUE;
	}

	return FALSE;
}

DWORD r4300iCompiler_CalculateMemoryCheck(DWORD location) {
	DWORD CalculatedValue = 0, Count = 0;

	for (;;) {
		CalculatedValue ^= R4300I_READOPCODE(location + Count);
		Count += 256;
		
		/*
		 * blocks may go over the block size limitation if
		 * the current instruction is a branch so it can
		 * do the delay slot as well, always
		 */

		if (Count >= R4300I_BLOCK_SIZE && TRUE != IsOpcodeBranch(location + Count)) {
			break;
		}
	}

	return CalculatedValue;
}

/*
 * TODO:
 * 1) branches need to be uncheated, very reliable now tho
 * 2) interrupt handling slows us down a great deal 
 */

void * r4300iCompiler_CreateBlock(void) {
	static DWORD TestValue = 0;
	static DWORD BlockID = 0;

	BYTE * JumpByte, * JumpByte2;
	BUFFER_POINTER Block;
	DWORD CheckValue = r4300iCompiler_CalculateMemoryCheck(pc);

	DWORD BlockEnd = pc + R4300I_BLOCK_SIZE;

	Recompiler.Log("");
	Recompiler.Log("---------------");
	Recompiler.Log("Block=%i", BlockID++);
	Recompiler.Log("Start=%08X", pc);
	Recompiler.Log("CRC=%08X", CheckValue);
	Recompiler.Log("X86=%08X", Recompiler.X86CodePos);
	Recompiler.Log("---------------");

	Recompiler.BlockStart = pc;

	/* setup parent check value */
	Recompiler.SetCheckValue(pc, CheckValue);
	Recompiler.ProgramCounter = pc;

	for (;;) {
		Block.ptr = Recompiler.GetJumpTableEntry(Recompiler.ProgramCounter);
		if (Block.ptr != NULL) {
			/* link the block up and stop loop */
			Recompiler.Log("ERROR: Linking detected at %08X", Recompiler.ProgramCounter);
			break;
		}

		Recompiler.SetJumpTableToPos(Recompiler.ProgramCounter);

	//	if (pc==Recompiler.ProgramCounter) {*(Recompiler.X86CodePos.ubPtr++) = 0xCC;}

		/* give the child opcode the parents index in check values */
		Recompiler.SetParentAddress(Recompiler.ProgramCounter, pc);

		if (++TestValue == 128 || IsOpcodeBranch(Recompiler.ProgramCounter)) {
			extern int InterruptTime;

			AddConstToVariable((incrementer * TestValue) * -1, &InterruptTime, "Instruction Count");
			CompConstToVariable(0, &InterruptTime, "InterruptTime");
			JgLabel8("ContinueExecution", 0);
			JumpByte2 = Recompiler.X86CodePos.ubPtr - 1;
			
			MoveConstToVariable(Recompiler.ProgramCounter, &pc, "PC");
			CallFunctionDirect(&CheckInterrupts,"CheckInterrupts");
			
			CompConstToVariable(Recompiler.ProgramCounter, &pc, "PC");			
			JeLabel8("ContinueExecution", 0);
			JumpByte = Recompiler.X86CodePos.ubPtr - 1;
			
			Ret();

			Recompiler.Log("  ContinueExecution:");
			SetBranch8b(JumpByte, Recompiler.X86CodePos.ptr);
			SetBranch8b(JumpByte2, Recompiler.X86CodePos.ptr);
			TestValue = 0;
		}

		r4300iCompiler_Opcode();
		Recompiler.ProgramCounter += 4;

/*		AddConstToVariable(incrementer, &instructions, "Instruction Count");
		MoveConstToVariable(Recompiler.ProgramCounter, &pc, "PC");
		CallFunctionDirect(&CheckInterrupts,"CheckInterrupts");
		Ret();
*/
		if (Recompiler.ProgramCounter >= BlockEnd) {
			break;
		}
	}

	/* we cant risk invalidating the selfmod right now */
/*
	MoveVariableToEax(Recompiler.GetJumpTablePtr(pc + Count), "JumpTable + PC");
	CompConstToEax(0);
	JeLabel8("  InvalidPointer", 0);
	JumpByte = Recompiler.X86CodePos.ubPtr - 1;
	JumpEax();
	SetBranch8b(JumpByte, Recompiler.X86CodePos.ptr);
	Recompiler.Log("  InvalidPointer:");
*/
	MoveConstToVariable(Recompiler.ProgramCounter, &pc, "pc");
	Ret();

	return Recompiler.GetJumpTableEntry(pc);
}

BOOL r4300iCompiler_CheckMemory(void) {
	DWORD Parent = Recompiler.GetParentAddress(pc);
	//Recompiler.Log("r4300iCompiler_CheckMemory (%08X, Parent = %08X)", pc, Parent);

	DWORD CurrentValue = Recompiler.GetParentCheckValue(pc);
	//Recompiler.Log("r4300iCompiler_CheckMemory: %08X (Current)", CurrentValue);

	DWORD CalculatedValue = r4300iCompiler_CalculateMemoryCheck(Parent);
	//Recompiler.Log("r4300iCompiler_CheckMemory: %08X (Calculated)", CalculatedValue);

	if (CalculatedValue != CurrentValue) {
		DWORD count = 0;
		//Recompiler.Log("r4300iCompiler_CheckMemory(%08X): %08X != %08X", pc, CalculatedValue, CurrentValue);

		for (;;) {
			Recompiler.SetJumpTableEntry(pc + count, NULL);
			count += 4;
			
			/*
			 * delay slot is irrelevant jump table entry is
			 * not relevant, it will be a separate block and
			 * by default will be recompiled for this one
			 */

			if (count >= R4300I_BLOCK_SIZE) {
				break;
			}
		}
		return FALSE;
	} else {
		//Recompiler.Log("r4300iCompiler_CheckMemory(%08X): %08X == %08X", pc, CalculatedValue, CurrentValue);
	}
	return TRUE;
}

void r4300iCompiler_Execute(void) {
	BUFFER_POINTER Block;

	Recompiler.Log("r4300iCompiler_Execute(%08X)", pc);

	while (FALSE == cpuIsReset) {
		Block.ptr = Recompiler.GetJumpTableEntry(pc);

		if (Block.ptr && FALSE == r4300iCompiler_CheckMemory()) {
			Block.ptr = NULL;
		}

		if (Block.ptr == NULL) {
			Recompiler.Log("Block.ptr(%08X) == NULL, Calling create", pc);
			Block.ptr = r4300iCompiler_CreateBlock();
		}
		_asm {
			pushad
			call [Block.ptr]
			popad
		}
	}
}