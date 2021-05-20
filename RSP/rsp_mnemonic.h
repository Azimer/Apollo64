#ifndef __RSP_MNEMONIC_H
#define __RSP_MNEMONIC_H
        


//////////////////////////////////////////////////////////////////////
// Data Types
//////////////////////////////////////////////////////////////////////

// unsigned types
#define _u64   unsigned __int64   /* 64 bit */
#define _u32   unsigned __int32   /* 32 bit */
#define _u16   unsigned __int16   /* 16 bit */
#define _u8	   unsigned __int8    /*  8 bit */

// signed types
#define _s64   __int64   /* 64 bit */
#define _s32   __int32   /* 32 bit */
#define _s16   __int16   /* 16 bit */
#define _s8	   __int8    /*  8 bit */




/**********************\
*                      *
* instruction reader   *
*                      *
\**********************/

/*
#define READ_RSP_INSTRUCTION(addr)                      \
(                                                       \
        ( *((_u32 *)(rsp_imem + (addr & 0x0fff))) )   \
)*/



#define READ_RSP_INSTRUCTION(addr) Load32_IMEM(addr)





/**********************\
*                      *
* mnemonic definitions *
*                      *
\**********************/

#define iCODE          (rsp_reg.code = READ_RSP_INSTRUCTION(sp_reg_pc))
#define __NEXT_CODE     (READ_RSP_INSTRUCTION((sp_reg_pc + 4)))

#define __OPCODE        ((_u8)(iCODE >> 26))

#define iRS            (((_u8)(rsp_reg.code >> 21)) & 0x1f)

#define iRT            (((_u8)(rsp_reg.code >> 16)) & 0x1f)

#define iRD            (((_u8)(rsp_reg.code >> 11)) & 0x1f)

#define iSA            (((_u8)(rsp_reg.code >>  6)) & 0x1f)

#define __F             ( (_u8)(rsp_reg.code)       & 0x3f)

#define iI             ( (_s32)(_s16)rsp_reg.code )
#define __O             ( sp_reg_pc + 4 + (__I << 2) )
#define __RSP_O(x)      ( ((_s32)(_s8)((rsp_reg.code & 0x40) ? ((rsp_reg.code & 0x7f) | 0x80) : (rsp_reg.code & 0x7f))) << x )

#define ____T           (rsp_reg.code & 0x3ffffff)
#define __T             ( (sp_reg_pc & 0xf0000000) | (____T << 2) )





/**********************\
*                      *
* mnemonic debugging   *
*                      *
\**********************/

#define ACCU_2_VREG(accu)                       \
        rsp_reg.v[__SA][0] = rsp_reg.accu[0];   \
        rsp_reg.v[__SA][1] = rsp_reg.accu[1];   \
        rsp_reg.v[__SA][2] = rsp_reg.accu[2];   \
        rsp_reg.v[__SA][3] = rsp_reg.accu[3];   \
        rsp_reg.v[__SA][4] = rsp_reg.accu[4];   \
        rsp_reg.v[__SA][5] = rsp_reg.accu[5];   \
        rsp_reg.v[__SA][6] = rsp_reg.accu[6];   \
        rsp_reg.v[__SA][7] = rsp_reg.accu[7];





#define STORE_REGISTER                                  \
                                                        \
        rsp_reg.v_src1[0] = rsp_reg.v[__RD][0];         \
        rsp_reg.v_src1[1] = rsp_reg.v[__RD][1];         \
        rsp_reg.v_src1[2] = rsp_reg.v[__RD][2];         \
        rsp_reg.v_src1[3] = rsp_reg.v[__RD][3];         \
        rsp_reg.v_src1[4] = rsp_reg.v[__RD][4];         \
        rsp_reg.v_src1[5] = rsp_reg.v[__RD][5];         \
        rsp_reg.v_src1[6] = rsp_reg.v[__RD][6];         \
        rsp_reg.v_src1[7] = rsp_reg.v[__RD][7];         \
                                                        \
        switch(__RS & 0x0f)                             \
        {                                               \
            case 0:                                     \
            case 1:                                     \
                rsp_reg.v_src2[0] = rsp_reg.v[__RT][0]; \
                rsp_reg.v_src2[1] = rsp_reg.v[__RT][1]; \
                rsp_reg.v_src2[2] = rsp_reg.v[__RT][2]; \
                rsp_reg.v_src2[3] = rsp_reg.v[__RT][3]; \
                rsp_reg.v_src2[4] = rsp_reg.v[__RT][4]; \
                rsp_reg.v_src2[5] = rsp_reg.v[__RT][5]; \
                rsp_reg.v_src2[6] = rsp_reg.v[__RT][6]; \
                rsp_reg.v_src2[7] = rsp_reg.v[__RT][7]; \
                break;                                  \
                                                        \
            case 2:                                     \
                rsp_reg.v_src2[0] = rsp_reg.v[__RT][0]; \
                rsp_reg.v_src2[1] = rsp_reg.v[__RT][0]; \
                rsp_reg.v_src2[2] = rsp_reg.v[__RT][2]; \
                rsp_reg.v_src2[3] = rsp_reg.v[__RT][2]; \
                rsp_reg.v_src2[4] = rsp_reg.v[__RT][4]; \
                rsp_reg.v_src2[5] = rsp_reg.v[__RT][4]; \
                rsp_reg.v_src2[6] = rsp_reg.v[__RT][6]; \
                rsp_reg.v_src2[7] = rsp_reg.v[__RT][6]; \
                break;                                  \
                                                        \
            case 3:                                     \
                rsp_reg.v_src2[0] = rsp_reg.v[__RT][1]; \
                rsp_reg.v_src2[1] = rsp_reg.v[__RT][1]; \
                rsp_reg.v_src2[2] = rsp_reg.v[__RT][3]; \
                rsp_reg.v_src2[3] = rsp_reg.v[__RT][3]; \
                rsp_reg.v_src2[4] = rsp_reg.v[__RT][5]; \
                rsp_reg.v_src2[5] = rsp_reg.v[__RT][5]; \
                rsp_reg.v_src2[6] = rsp_reg.v[__RT][7]; \
                rsp_reg.v_src2[7] = rsp_reg.v[__RT][7]; \
                break;                                  \
                                                        \
            case 4:                                     \
                rsp_reg.v_src2[0] = rsp_reg.v[__RT][0]; \
                rsp_reg.v_src2[1] = rsp_reg.v[__RT][0]; \
                rsp_reg.v_src2[2] = rsp_reg.v[__RT][0]; \
                rsp_reg.v_src2[3] = rsp_reg.v[__RT][0]; \
                rsp_reg.v_src2[4] = rsp_reg.v[__RT][4]; \
                rsp_reg.v_src2[5] = rsp_reg.v[__RT][4]; \
                rsp_reg.v_src2[6] = rsp_reg.v[__RT][4]; \
                rsp_reg.v_src2[7] = rsp_reg.v[__RT][4]; \
                break;                                  \
                                                        \
            case 5:                                     \
                rsp_reg.v_src2[0] = rsp_reg.v[__RT][1]; \
                rsp_reg.v_src2[1] = rsp_reg.v[__RT][1]; \
                rsp_reg.v_src2[2] = rsp_reg.v[__RT][1]; \
                rsp_reg.v_src2[3] = rsp_reg.v[__RT][1]; \
                rsp_reg.v_src2[4] = rsp_reg.v[__RT][5]; \
                rsp_reg.v_src2[5] = rsp_reg.v[__RT][5]; \
                rsp_reg.v_src2[6] = rsp_reg.v[__RT][5]; \
                rsp_reg.v_src2[7] = rsp_reg.v[__RT][5]; \
                break;                                  \
                                                        \
            case 6:                                     \
                rsp_reg.v_src2[0] = rsp_reg.v[__RT][2]; \
                rsp_reg.v_src2[1] = rsp_reg.v[__RT][2]; \
                rsp_reg.v_src2[2] = rsp_reg.v[__RT][2]; \
                rsp_reg.v_src2[3] = rsp_reg.v[__RT][2]; \
                rsp_reg.v_src2[4] = rsp_reg.v[__RT][6]; \
                rsp_reg.v_src2[5] = rsp_reg.v[__RT][6]; \
                rsp_reg.v_src2[6] = rsp_reg.v[__RT][6]; \
                rsp_reg.v_src2[7] = rsp_reg.v[__RT][6]; \
                break;                                  \
                                                        \
            case 7:                                     \
                rsp_reg.v_src2[0] = rsp_reg.v[__RT][3]; \
                rsp_reg.v_src2[1] = rsp_reg.v[__RT][3]; \
                rsp_reg.v_src2[2] = rsp_reg.v[__RT][3]; \
                rsp_reg.v_src2[3] = rsp_reg.v[__RT][3]; \
                rsp_reg.v_src2[4] = rsp_reg.v[__RT][7]; \
                rsp_reg.v_src2[5] = rsp_reg.v[__RT][7]; \
                rsp_reg.v_src2[6] = rsp_reg.v[__RT][7]; \
                rsp_reg.v_src2[7] = rsp_reg.v[__RT][7]; \
                break;                                  \
                                                        \
            case 8:                                     \
                rsp_reg.v_src2[0] = rsp_reg.v[__RT][0]; \
                rsp_reg.v_src2[1] = rsp_reg.v[__RT][0]; \
                rsp_reg.v_src2[2] = rsp_reg.v[__RT][0]; \
                rsp_reg.v_src2[3] = rsp_reg.v[__RT][0]; \
                rsp_reg.v_src2[4] = rsp_reg.v[__RT][0]; \
                rsp_reg.v_src2[5] = rsp_reg.v[__RT][0]; \
                rsp_reg.v_src2[6] = rsp_reg.v[__RT][0]; \
                rsp_reg.v_src2[7] = rsp_reg.v[__RT][0]; \
                break;                                  \
                                                        \
            case 9:                                     \
                rsp_reg.v_src2[0] = rsp_reg.v[__RT][1]; \
                rsp_reg.v_src2[1] = rsp_reg.v[__RT][1]; \
                rsp_reg.v_src2[2] = rsp_reg.v[__RT][1]; \
                rsp_reg.v_src2[3] = rsp_reg.v[__RT][1]; \
                rsp_reg.v_src2[4] = rsp_reg.v[__RT][1]; \
                rsp_reg.v_src2[5] = rsp_reg.v[__RT][1]; \
                rsp_reg.v_src2[6] = rsp_reg.v[__RT][1]; \
                rsp_reg.v_src2[7] = rsp_reg.v[__RT][1]; \
                break;                                  \
                                                        \
            case 10:                                    \
                rsp_reg.v_src2[0] = rsp_reg.v[__RT][2]; \
                rsp_reg.v_src2[1] = rsp_reg.v[__RT][2]; \
                rsp_reg.v_src2[2] = rsp_reg.v[__RT][2]; \
                rsp_reg.v_src2[3] = rsp_reg.v[__RT][2]; \
                rsp_reg.v_src2[4] = rsp_reg.v[__RT][2]; \
                rsp_reg.v_src2[5] = rsp_reg.v[__RT][2]; \
                rsp_reg.v_src2[6] = rsp_reg.v[__RT][2]; \
                rsp_reg.v_src2[7] = rsp_reg.v[__RT][2]; \
                break;                                  \
                                                        \
            case 11:                                    \
                rsp_reg.v_src2[0] = rsp_reg.v[__RT][3]; \
                rsp_reg.v_src2[1] = rsp_reg.v[__RT][3]; \
                rsp_reg.v_src2[2] = rsp_reg.v[__RT][3]; \
                rsp_reg.v_src2[3] = rsp_reg.v[__RT][3]; \
                rsp_reg.v_src2[4] = rsp_reg.v[__RT][3]; \
                rsp_reg.v_src2[5] = rsp_reg.v[__RT][3]; \
                rsp_reg.v_src2[6] = rsp_reg.v[__RT][3]; \
                rsp_reg.v_src2[7] = rsp_reg.v[__RT][3]; \
                break;                                  \
                                                        \
            case 12:                                    \
                rsp_reg.v_src2[0] = rsp_reg.v[__RT][4]; \
                rsp_reg.v_src2[1] = rsp_reg.v[__RT][4]; \
                rsp_reg.v_src2[2] = rsp_reg.v[__RT][4]; \
                rsp_reg.v_src2[3] = rsp_reg.v[__RT][4]; \
                rsp_reg.v_src2[4] = rsp_reg.v[__RT][4]; \
                rsp_reg.v_src2[5] = rsp_reg.v[__RT][4]; \
                rsp_reg.v_src2[6] = rsp_reg.v[__RT][4]; \
                rsp_reg.v_src2[7] = rsp_reg.v[__RT][4]; \
                break;                                  \
                                                        \
            case 13:                                    \
                rsp_reg.v_src2[0] = rsp_reg.v[__RT][5]; \
                rsp_reg.v_src2[1] = rsp_reg.v[__RT][5]; \
                rsp_reg.v_src2[2] = rsp_reg.v[__RT][5]; \
                rsp_reg.v_src2[3] = rsp_reg.v[__RT][5]; \
                rsp_reg.v_src2[4] = rsp_reg.v[__RT][5]; \
                rsp_reg.v_src2[5] = rsp_reg.v[__RT][5]; \
                rsp_reg.v_src2[6] = rsp_reg.v[__RT][5]; \
                rsp_reg.v_src2[7] = rsp_reg.v[__RT][5]; \
                break;                                  \
                                                        \
            case 14:                                    \
                rsp_reg.v_src2[0] = rsp_reg.v[__RT][6]; \
                rsp_reg.v_src2[1] = rsp_reg.v[__RT][6]; \
                rsp_reg.v_src2[2] = rsp_reg.v[__RT][6]; \
                rsp_reg.v_src2[3] = rsp_reg.v[__RT][6]; \
                rsp_reg.v_src2[4] = rsp_reg.v[__RT][6]; \
                rsp_reg.v_src2[5] = rsp_reg.v[__RT][6]; \
                rsp_reg.v_src2[6] = rsp_reg.v[__RT][6]; \
                rsp_reg.v_src2[7] = rsp_reg.v[__RT][6]; \
                break;                                  \
                                                        \
            case 15:                                    \
                rsp_reg.v_src2[0] = rsp_reg.v[__RT][7]; \
                rsp_reg.v_src2[1] = rsp_reg.v[__RT][7]; \
                rsp_reg.v_src2[2] = rsp_reg.v[__RT][7]; \
                rsp_reg.v_src2[3] = rsp_reg.v[__RT][7]; \
                rsp_reg.v_src2[4] = rsp_reg.v[__RT][7]; \
                rsp_reg.v_src2[5] = rsp_reg.v[__RT][7]; \
                rsp_reg.v_src2[6] = rsp_reg.v[__RT][7]; \
                rsp_reg.v_src2[7] = rsp_reg.v[__RT][7]; \
                break;                                  \
                                                        \
        } /* switch(__RS & 0x0f) */





#define VECT_OP(op)                                                             \
{                                                                               \
        int     i;                                                              \
                                                                                \
        for(i=0; i<8; i++)                                                      \
        {                                                                       \
                rsp_reg.v[__SA][i] = rsp_reg.v_src1[i] op rsp_reg.v_src2[i];    \
        }                                                                       \
}





#define VECT_N_OP(op)                                                           \
{                                                                               \
        int     i;                                                              \
                                                                                \
        for(i=0; i<8; i++)                                                      \
        {                                                                       \
                rsp_reg.v[__SA][i] = ~(rsp_reg.v_src1[i] op rsp_reg.v_src2[i]); \
        }                                                                       \
}





#define VECT_OP_COMP(comp)                                                              \
{                                                                                       \
        int     i;                                                                      \
        _u32    flags = rsp_reg.flag[1];                                                \
                                                                                        \
                                                                                        \
        for(i=0; i<8; i++)                                                              \
        {                                                                               \
                if(rsp_reg.v_src1[i] comp rsp_reg.v_src2[i])                            \
                        flags |= 0x01 << i;                                             \
                else                                                                    \
                        flags &= ~(0x01 << i);                                          \
                if(0 comp 1)           /* VNE or VLT */                                 \
                {                                                                       \
                        if(1 comp 0)   /* VNE */                                        \
                        {                                                               \
                                if(rsp_reg.flag[0] & (1 << (i+8)))                      \
                                {                                                       \
                                        flags |= 0x01 << i;                             \
                                }                                                       \
                        }                                                               \
                        else           /* VLT */                                        \
                        {                                                               \
                                if(rsp_reg.flag[0] & (1 << i))                          \
                                {                                                       \
                                        if(rsp_reg.v_src1[i] == rsp_reg.v_src2[i])      \
                                                flags |= 0x01 << i;                     \
                                }                                                       \
                        }                                                               \
                }                                                                       \
                else                   /* VEQ or VGE */                                 \
                {                                                                       \
                        if(1 comp 0)   /* VGE */                                        \
                        {                                                               \
                                if(rsp_reg.flag[0] & (1 << i))                          \
                                {                                                       \
                                        if(rsp_reg.v_src1[i] == rsp_reg.v_src2[i])      \
                                                flags &= ~(0x01 << i);                  \
                                }                                                       \
                        }                                                               \
                        else           /* VEQ */                                        \
                        {                                                               \
                                if(rsp_reg.flag[0] & (1 << (i+8)))                      \
                                {                                                       \
                                        flags &= ~(0x01 << i);                          \
                                }                                                       \
                        }                                                               \
                }                                                                       \
        }                                                                               \
                                                                                        \
        rsp_reg.flag[1] = flags;                                                        \
}





#define VECT_OP_COPY(which)                                     \
{                                                               \
        int i;                                                  \
                                                                \
        for(i=0; i<8; i++)                                      \
        {                                                       \
                if(which & (1 << i))                            \
                        rsp_reg.v[__SA][i] = rsp_reg.v_src1[i]; \
                else                                            \
                        rsp_reg.v[__SA][i] = rsp_reg.v_src2[i]; \
        }                                                       \
}





#define VECT_OP_ABS                                                             \
{                                                                               \
        int     i;                                                              \
                                                                                \
                                                                                \
                                                                                \
        for(i=0; i<8; i++)                                                      \
        {                                                                       \
                if(rsp_reg.v_src1[i] == 0)                                      \
                        rsp_reg.v[__SA][i] = 0;                                 \
                else if(rsp_reg.v_src1[i] < 0)                                  \
                {                                                               \
                        rsp_reg.v[__SA][i] = -rsp_reg.v_src2[i];                \
                        if(((_u16)rsp_reg.v[__SA][i]) == 0x8000)               \
                                rsp_reg.v[__SA][i] = 0x7fff;                    \
                }                                                               \
                else if(rsp_reg.v_src1[i] > 0)                                  \
                        rsp_reg.v[__SA][i] = rsp_reg.v_src2[i];                 \
        }                                                                       \
}






#define CLIP_RESULT_ADD                                                                         \
{                                                                                               \
        int     i;                                                                              \
                                                                                                \
        for(i=0; i<8; i++)                                                                      \
        {                                                                                       \
                if((~rsp_reg.v_src1[i] & ~rsp_reg.v_src2[i] & rsp_reg.v[__SA][i]) & 0x8000)     \
                        rsp_reg.v[__SA][i] = 0x7fff;                                            \
                else if((rsp_reg.v_src1[i] & rsp_reg.v_src2[i] & ~rsp_reg.v[__SA][i]) & 0x8000) \
                        rsp_reg.v[__SA][i] = 0x8000;                                            \
        }                                                                                       \
}





#define CLIP_RESULT_SUB                                                                         \
{                                                                                               \
        int     i;                                                                              \
                                                                                                \
        for(i=0; i<8; i++)                                                                      \
        {                                                                                       \
                if((~rsp_reg.v_src1[i] & rsp_reg.v_src2[i] & rsp_reg.v[__SA][i]) & 0x8000)      \
                        rsp_reg.v[__SA][i] = 0x7fff;                                            \
                else if((rsp_reg.v_src1[i] & ~rsp_reg.v_src2[i] & ~rsp_reg.v[__SA][i]) & 0x8000)\
                        rsp_reg.v[__SA][i] = 0x8000;                                            \
        }                                                                                       \
}





#define SET_CARRY_ADD                                                                                   \
{                                                                                                       \
        int     i;                                                                                      \
                                                                                                        \
                                                                                                        \
                                                                                                        \
        rsp_reg.flag[0] = 0;                                                                            \
                                                                                                        \
        for(i=0; i<8; i++)                                                                              \
        {                                                                                               \
                if(((_s32)(_u16)rsp_reg.v_src1[i] + (_s32)(_u16)rsp_reg.v_src2[i]) != (_s32)(_u16)rsp_reg.v[__SA][i])  \
                        rsp_reg.flag[0] |= 1 << i;                                                      \
        }                                                                                               \
}





#define SET_CARRY_SUB                                                                                   \
{                                                                                                       \
        int     i;                                                                                      \
                                                                                                        \
                                                                                                        \
                                                                                                        \
        rsp_reg.flag[0] = 0;                                                                            \
                                                                                                        \
        for(i=0; i<8; i++)                                                                              \
        {                                                                                               \
                if(((_s32)(_u16)rsp_reg.v_src1[i] - (_s32)(_u16)rsp_reg.v_src2[i]) != (_s32)(_u16)rsp_reg.v[__SA][i])  \
                        rsp_reg.flag[0] |= 1 << i;                                                      \
        }                                                                                               \
}





#define VECT_ACCU_OP(accu, pre, cast, op, shift)                                                                \
{                                                                                                               \
        int     i;                                                                                              \                                                                                                              \
        for(i=0; i<8; i++)                                                                                      \
        {                                                                                                       \
                rsp_reg.accu[i] pre##= ((_u32)(cast rsp_reg.v[__RD][i] op cast rsp_reg.v[__RT][i])) shift;      \
        }                                                                                                       \
}





/* zero? */
#define SET_ZERO_FLAGS(x)                       \
        rsp_reg.flag[0] &= 0x00ff;              \
        if(rsp_reg.v[x][0] != 0)                \
                rsp_reg.flag[0] |= 0x0100;      \
        if(rsp_reg.v[x][1] != 0)                \
                rsp_reg.flag[0] |= 0x0200;      \
        if(rsp_reg.v[x][2] != 0)                \
                rsp_reg.flag[0] |= 0x0400;      \
        if(rsp_reg.v[x][3] != 0)                \
                rsp_reg.flag[0] |= 0x0800;      \
        if(rsp_reg.v[x][4] != 0)                \
                rsp_reg.flag[0] |= 0x1000;      \
        if(rsp_reg.v[x][5] != 0)                \
                rsp_reg.flag[0] |= 0x2000;      \
        if(rsp_reg.v[x][6] != 0)                \
                rsp_reg.flag[0] |= 0x4000;      \
        if(rsp_reg.v[x][7] != 0)                \
                rsp_reg.flag[0] |= 0x8000;





#define ADD_CARRY(x)                    \
        if(rsp_reg.flag[0] & 0x01)      \
                rsp_reg.v[x][0]++;      \
        if(rsp_reg.flag[0] & 0x02)      \
                rsp_reg.v[x][1]++;      \
        if(rsp_reg.flag[0] & 0x04)      \
                rsp_reg.v[x][2]++;      \
        if(rsp_reg.flag[0] & 0x08)      \
                rsp_reg.v[x][3]++;      \
        if(rsp_reg.flag[0] & 0x10)      \
                rsp_reg.v[x][4]++;      \
        if(rsp_reg.flag[0] & 0x20)      \
                rsp_reg.v[x][5]++;      \
        if(rsp_reg.flag[0] & 0x40)      \
                rsp_reg.v[x][6]++;      \
        if(rsp_reg.flag[0] & 0x80)      \
                rsp_reg.v[x][7]++;      \
        rsp_reg.flag[0] &= 0xff00;





#define SUB_CARRY(x)                    \
        if(rsp_reg.flag[0] & 0x01)      \
                rsp_reg.v[x][0]--;      \
        if(rsp_reg.flag[0] & 0x02)      \
                rsp_reg.v[x][1]--;      \
        if(rsp_reg.flag[0] & 0x04)      \
                rsp_reg.v[x][2]--;      \
        if(rsp_reg.flag[0] & 0x08)      \
                rsp_reg.v[x][3]--;      \
        if(rsp_reg.flag[0] & 0x10)      \
                rsp_reg.v[x][4]--;      \
        if(rsp_reg.flag[0] & 0x20)      \
                rsp_reg.v[x][5]--;      \
        if(rsp_reg.flag[0] & 0x40)      \
                rsp_reg.v[x][6]--;      \
        if(rsp_reg.flag[0] & 0x80)      \
                rsp_reg.v[x][7]--;      \
        rsp_reg.flag[0] &= 0xff00;


#define REG_R0			 rsp_reg.r[0x00]
#define REG_AT			 rsp_reg.r[0x01]
#define REG_V0			 rsp_reg.r[0x02]
#define REG_V1			 rsp_reg.r[0x03]
#define REG_A0			 rsp_reg.r[0x04]
#define REG_A1			 rsp_reg.r[0x05]
#define REG_A2			 rsp_reg.r[0x06]
#define REG_A3			 rsp_reg.r[0x07]
#define REG_T0			 rsp_reg.r[0x08]
#define REG_T1			 rsp_reg.r[0x09]
#define REG_T2			 rsp_reg.r[0x0A]
#define REG_T3			 rsp_reg.r[0x0B]
#define REG_T4			 rsp_reg.r[0x0C]
#define REG_T5			 rsp_reg.r[0x0D]
#define REG_T6			 rsp_reg.r[0x0E]
#define REG_T7			 rsp_reg.r[0x0F]
#define REG_S0			 rsp_reg.r[0x10]
#define REG_S1			 rsp_reg.r[0x11]
#define REG_S2			 rsp_reg.r[0x12]
#define REG_S3			 rsp_reg.r[0x13]
#define REG_S4			 rsp_reg.r[0x14]
#define REG_S5			 rsp_reg.r[0x15]
#define REG_S6			 rsp_reg.r[0x16]
#define REG_S7			 rsp_reg.r[0x17]
#define REG_T8			 rsp_reg.r[0x18]
#define REG_T9			 rsp_reg.r[0x19]
#define REG_K0			 rsp_reg.r[0x1A]
#define REG_K1			 rsp_reg.r[0x1B]
#define REG_GP			 rsp_reg.r[0x1C]
#define REG_SP			 rsp_reg.r[0x1D]
#define REG_S8			 rsp_reg.r[0x1E]
#define REG_RA			 rsp_reg.r[0x1F]

int table[16][8] = 
{
	{0,1,2,3,4,5,6,7},
	{0,0,0,0,0,0,0,0},
	{0,1,2,3,0,1,2,3},
	{4,5,6,7,4,5,6,7},
	{0,1,0,1,0,1,0,1},
	{2,3,2,3,2,3,2,3},
	{4,5,4,5,4,5,4,5},
	{6,7,6,7,6,7,6,7},
	{0,0,0,0,0,0,0,0},
	{1,1,1,1,1,1,1,1},
	{2,2,2,2,2,2,2,2},
	{3,3,3,3,3,3,3,3},
	{4,4,4,4,4,4,4,4},
	{5,5,5,5,5,5,5,5},
	{6,6,6,6,6,6,6,6},
	{7,7,7,7,7,7,7,7}
};


#endif