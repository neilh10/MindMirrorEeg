/*
 * dpl.h
 *
 * Include module for dpl subsystem
 */

/**************************************************************
 * Internal enums/typedefs to this subsystem
 */

#define mmir_status_buf_clr() strnset(mmir_status_buf,' ',MMIR_STATUS_BUF_SIZE)

/**************************************************************
 * Prototypes to this subsystem
 */
#ifndef MMIR_PC
 enum sys_return dpl_initialisation(Initt_system_state *ssp);
 void dpl_display(enum sche_entry sch_type);
#endif

/* end dpl.h */





