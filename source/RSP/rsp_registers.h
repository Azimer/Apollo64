#ifndef __RSP_REGISTERS_H
#define __RSP_REGISTERS_H



typedef union {
	_s64			S64[2];
	_u64			U64[2];
	_s32			S32[4];
	_u32			U32[4];
	_s16			S16[8];
	_u16			U16[8];
	_s8				U8[16];
	_u8				S8[16];
} VECTOR;

typedef struct 
{
        
        _s32  r[32];           /* general purpose registers */

        VECTOR  v[32][1];       /* vector registers */
        _s16  v_src1[8];      /* dummy source 1 vector registers */
        _s16  v_src2[8];      /* dummy source 2 vector registers */
        VECTOR  dummy[1];      /* dummy source 2 vector registers */

		_s64  accum[8];

        _s16  ah[8];          /* high portion of accu */
        _s16  al[8];          /* low  portion of accu */
        _s16  flag[4];        /* cop2 control registers */

        _s16  vrcph_result;       /* calculated by VRCPL - stored by VRCPH */
        _s16  vrcph_source;       /* used by VRCPL - stored by VRCPH */



        _u32    pc_delay;   /* delayed program counter */
        int     delay;      /* this is for correct emulation of the 6 stage pipeline */
                            /* if(delay ==  0) { next instr is pc + 4 } */
                            /* if(delay ==  1) { next instr is delayed pc} */
                            /* if(delay == -1) { exec delayed pc} */
        
        _u32    code;       /* current code (needed for speed) */
        int     halt;       /* if '!0' then stop execution */
        int     do_or_check_sthg; /* if '!0' then the rs4300i has to do or check sthg */
                            /* look at the defines that follow this struct */
        _u32    count;      /* just for debugging */

} t_rsp_reg; /* struct rsp_reg */



/* rsp_reg.delay */

#define NO_RSP_DELAY    0
#define DO_RSP_DELAY    1
#define EXEC_RSP_DELAY  2

// GPR Regs
#define R0 0x00
#define AT 0x01
#define V0 0x02
#define V1 0x03
#define A0 0x04
#define A1 0x05
#define A2 0x06
#define A3 0x07
#define T0 0x08
#define T1 0x09
#define T2 0x0A
#define T3 0x0B
#define T4 0x0C
#define T5 0x0D
#define T6 0x0E
#define T7 0x0F
#define S0 0x10
#define S1 0x11
#define S2 0x12
#define S3 0x13
#define S4 0x14
#define S5 0x15
#define S6 0x16
#define S7 0x17
#define T8 0x18
#define T9 0x19
#define K0 0x1A
#define K1 0x1B
#define GP 0x1C
#define SP 0x1D
#define S8 0x1E
#define RA 0x1F


#endif