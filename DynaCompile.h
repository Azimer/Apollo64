// TODO: I need to detect ERET and COP1 Jumps!
inline bool isJump (u32 code) { // JAL and JALR are not included since they are special.
	return (
		((code & 0xFC000000) == 0x08000000) || // J
		((code & 0xFC000000) == 0x0C000000)    // JAL
		);
/*
	retVal = 
    opSPECIAL,	opREGIMM,	opJ,	opJAL,		opBEQ,	opBNE,	opBLEZ,	opBGTZ, //00-07
    opADDI,		opADDIU,	opSLTI,	opSLTIU,	opANDI,	opORI,	opXORI,	opLUI,	//08-0F
    opCOP0,		opCOP1,		opCOP2,	opNI,		opBEQL,	opBNEL,	opBLEZL,opBGTZL,//10-17
// Mask RegImm with FC1F0000
// RegImm
    opBLTZ,		opBGEZ,		opBLTZL,	opBGEZL,	opNI,	opNI,	opNI,	opNI,
    opTGEI,		opTGEIU,	opTLTI,		opTLTIU,	opTEQI,	opNI,	opTNEI,	opNI,
    opBLTZAL,	opBGEZAL,	opBLTZALL,	opBGEZALL,	opNI,	opNI,	opNI,	opNI,
    opNI,		opNI,		opNI,		opNI,		opNI,	opNI,	opNI,	opNI,*/
}

inline bool isBranch (u32 code) {
	return (
		((code & 0xFC000000) == 0x10000000) || // BEQ
		((code & 0xFC000000) == 0x14000000) || // BNE
		((code & 0xFC000000) == 0x18000000) || // BLEZ
		((code & 0xFC000000) == 0x1C000000) || // BGTZ
		((code & 0xFC000000) == 0x50000000) || // BEQL
		((code & 0xFC000000) == 0x54000000) || // BNEL
		((code & 0xFC000000) == 0x58000000) || // BLEZL
		((code & 0xFC000000) == 0x5C000000) || // BGTZL
		((code & 0xFC1F0000) == 0x04000000) || // BLTZ
		((code & 0xFC1F0000) == 0x04010000) || // BGEZ
		((code & 0xFC1F0000) == 0x04020000) || // BLTZL
		((code & 0xFC1F0000) == 0x04030000) || // BGEZL
		((code & 0xFC1F0000) == 0x04100000) || // BLTZAL
		((code & 0xFC1F0000) == 0x04110000) || // BGEZAL
		((code & 0xFC1F0000) == 0x04120000) || // BLTZALL
		((code & 0xFC1F0000) == 0x04130000)    // BGEZALL
		);
}
/*
typedef struct {
		unsigned int	func:6;
		unsigned int	sa:5;		
		unsigned int	rd:5;
		unsigned int	rt:5;		
		unsigned int	rs:5;		
		unsigned int	op:6;
} s_sop;
void (*special[0x40])() = {
    opSLL,	opNI,		opSRL,	opSRA,	opSLLV,		opNI,		opSRLV,		opSRAV,  
    opJR,	opJALR,		opNI,	opNI,	opSYSCALL,	opBREAK,	opNI,		opSYNC,  
*/
inline bool isERET (u32 code) {
	return (code == 0x42000018);
}

inline bool isFPBranch (u32 code) {
	return ((code & 0xFFE00000) == 0x45000000);
}

inline bool isJR (u32 code) {
	return (
		((code & 0xFC00003F) == 0x00000008) || // JR
		((code & 0xFC00003F) == 0x00000009)    // JALR
	);
}
inline bool isBreakable (u32 code) {
	return (isBranch(code) || isJump (code) || isJR (code) || isERET (code) || isFPBranch (code));
}

inline bool isExceptionable (u32 code) {
	return (
		((code & 0xFC000000) == 0x44000000) ||
		((code & 0xFC000000) == 0xC4000000) ||
		((code & 0xFC000000) == 0xD4000000) ||
		((code & 0xFC000000) == 0xE4000000) ||
		((code & 0xFC000000) == 0xF4000000) ||
		((code & 0xFC00003F) == 0x0000000C)
	);
}

int CompileOpCode (u32 *op, u32 dynaPC);