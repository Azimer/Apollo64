/*************************** x86 ***************************/
__inline void PushFD() {
	DynaLog("   pushfd");
	*(x86BlockPtr.ubPtr++) = 0x9C;
}

__inline void PopFD() {
	DynaLog("   popfd");
	*(x86BlockPtr.ubPtr++) = 0x9D;
}

__inline void Lahf () {
	DynaLog("   lahf");
	*(x86BlockPtr.ubPtr++) = 0x9F;
}

__inline void Sahf () {
	DynaLog("   sahf");
	*(x86BlockPtr.ubPtr++) = 0x9E;
}

__inline void SetBranch8b(void * JumpByte, void * Destination) {
	/* calculate 32-bit relative offset */
	signed int n = (BYTE*)Destination - ((BYTE*)JumpByte + 1);
	*(BYTE*)(JumpByte) = (BYTE)n;
}

__inline void SetBranch32b(void * JumpByte, void * Destination) {
	*(DWORD*)(JumpByte) = (DWORD)((BYTE*)Destination - (BYTE*)((DWORD*)JumpByte + 1));
}

__inline void MoveConstToVariable (DWORD Const, void * Variable, char * VariableName) {
	DynaLog("   mov [%s], 0x%08X", VariableName, Const);
	*(x86BlockPtr.uwPtr++) = 0x05C7;
    *(x86BlockPtr.udwPtr++) = (DWORD)Variable;
    *(x86BlockPtr.udwPtr++) = Const;
}

__inline void CallFunctionDirect(void * FunctAddress, char * FunctName) {
	DynaLog("   call %s", FunctName);
	*(x86BlockPtr.ubPtr++) = 0xE8;
	*(x86BlockPtr.udwPtr++) = (DWORD)FunctAddress-(DWORD)x86BlockPtr.ptr - 4;
}

__inline void AddConstToVariable (DWORD Const, void * Variable, char * VariableName) {
	DynaLog("   add [%s], 0x%08X", VariableName, Const);
	*(x86BlockPtr.uwPtr++) = 0x0581;
	*(x86BlockPtr.udwPtr++) = (DWORD)Variable;
	*(x86BlockPtr.udwPtr++) = Const;
}

__inline void SubConst8ToVariable (BYTE Const, void * Variable, char * VariableName) {
	DynaLog("   sub [%s], 0x%08X", VariableName, Const);
	*(x86BlockPtr.uwPtr++) = 0x2D83;
	*(x86BlockPtr.udwPtr++) = (DWORD)Variable;
	*(x86BlockPtr.ubPtr++) = Const;
}

__inline void Ret(void) {
	DynaLog("   ret");
	*(x86BlockPtr.ubPtr++) = 0xC3;
}

__inline void MoveConstToEax(DWORD constant) {
	DynaLog("   mov eax, 0%08Xh", constant);
	*(x86BlockPtr.ubPtr++) = 0xB8;
    *(x86BlockPtr.udwPtr++) = (DWORD)constant;
}

__inline void MoveVariableToEax(void *Variable, char *VariableName) {
	DynaLog("   mov eax, [%s]", VariableName);
	*(x86BlockPtr.uwPtr++) = 0x058B;
    *(x86BlockPtr.udwPtr++) = (DWORD)Variable;
}

__inline void MoveEaxToVariable(void * Variable, char * VariableName) {
	DynaLog("   mov dword ptr [%s], eax",VariableName);
	*(x86BlockPtr.uwPtr++) = 0x0589;
    *(x86BlockPtr.udwPtr++) = (DWORD)Variable;
}

__inline void CompConstToVariable(DWORD Const, void * Variable, char * VariableName) {
	DynaLog("   cmp dword ptr [%s], 0x%X",VariableName, Const);
	*(x86BlockPtr.uwPtr++) = 0x3D81;
	*(x86BlockPtr.udwPtr++) = (DWORD)Variable;
	*(x86BlockPtr.udwPtr++) = Const;
}

__inline void CompEaxToVariable(void * Variable, char * VariableName) {
	DynaLog("   cmp eax, dword ptr [%s]",VariableName);
	*(x86BlockPtr.uwPtr++) = 0x053B;
	*(x86BlockPtr.udwPtr++) = (DWORD)Variable;
}

__inline void CompConstToEax(DWORD Const) {
	DynaLog("   cmp eax, 0x%08X", Const);
	*(x86BlockPtr.uwPtr++) = 0xF881;
	*(x86BlockPtr.udwPtr++) = (DWORD)Const;
}

__inline void JeLabel8(char * Label, BYTE Value) {
	DynaLog("   je $%s", Label);
	*(x86BlockPtr.ubPtr++) = 0x74;
	*(x86BlockPtr.ubPtr++) = Value;
}

__inline void JneLabel8(char * Label, BYTE Value) {
	DynaLog("   jne $%s", Label);
	*(x86BlockPtr.ubPtr++) = 0x75;
	*(x86BlockPtr.ubPtr++) = Value;
}

__inline void JlLabel8(char * Label, BYTE Value) {
	DynaLog("   jl $%s", Label);
	*(x86BlockPtr.ubPtr++) = 0x7C;
	*(x86BlockPtr.ubPtr++) = Value;
}

__inline void JgeLabel8(char * Label, BYTE Value) {
	DynaLog("   jge $%s", Label);
	*(x86BlockPtr.ubPtr++) = 0x7D;
	*(x86BlockPtr.ubPtr++) = Value;
}

__inline void JleLabel8(char * Label, BYTE Value) {
	DynaLog("   jle $%s", Label);
	*(x86BlockPtr.ubPtr++) = 0x7E;
	*(x86BlockPtr.ubPtr++) = Value;
}

__inline void JgLabel8(char * Label, BYTE Value) {
	DynaLog("   jg $%s", Label);
	*(x86BlockPtr.ubPtr++) = 0x7F;
	*(x86BlockPtr.ubPtr++) = Value;
}

__inline void JmpLabel8(char * Label, BYTE Value) {
	DynaLog("   jmp $%s", Label);
	*(x86BlockPtr.ubPtr++) = 0xEB;
	*(x86BlockPtr.ubPtr++) = Value;
}

__inline void JumpEax(void) {
	DynaLog("   jmp eax");
	*(x86BlockPtr.uwPtr++) = 0xe0ff;
}

__inline void JmpIndirectLabel32(char * Label,DWORD location) {
	DynaLog("   jmp dword ptr [%s]", Label);
	*(x86BlockPtr.uwPtr++) = 0x25ff;
	*(x86BlockPtr.udwPtr++) = location;
}

__inline void Int3 () {
	*(x86BlockPtr.ubPtr++) = 0xCC;
}