#ifndef __RSP_H
#define __RSP_H


typedef void (*rsp_instr)();


extern void rsp_run();
extern BOOL rsp_step();
extern void rsp_reset();
extern void rsp_dis(_u32 code);


#endif