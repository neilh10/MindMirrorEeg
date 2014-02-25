/* temp\dummy.c
 
 * http://www.biomonitors.com/
 * Copyright (c) 2014 Neil Hancock
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * 

*/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
#include <80C196.h>
#include <stdio.h>

#define DISABLE_ENABLE
#include "general.h"
#include "hw.h"
#include "iir.h"
#include "init.h"
#include "dbg.h"
#include "kernel.h"
#include "ldd.h"
#include "mm_coms.h"
#include "dim.h"
#include "proto.h"

/* Dimt_lobe_data dimu_input_data[1];*/
/* enum inite_return dim_initialisation(Initt_system_state *ssp){}*/
/* enum inite_return dfm_initialisation(Initt_system_state *ssp){} */
/* enum inite_return mfd_initialisation(Initt_system_state *ssp){} */
/* enum sys_return dpl_initialisation(Initt_system_state *ssp){}*/
/* enum inite_return dbg_if_init(Initt_system_state *ssp){} */
 enum inite_return dbg_tim_init(Initt_system_state *ssp){}
 Knt_type kn_que_init(Que_id number){}
 Knt_type kn_blk_init(Prt_id partitions){}
 Knt_type kn_pcreate(Prt_id partition_id, int size, int num, Byte *startp) {}
 Knt_type kn_rblock(Prt_id partition_id, Byte *p) {}
 Byte   *kn_gblock(Prt_id partition_id, Knt_type *err) {}
/* void dfm_schedule(void) {} */
/* void mfd_acquire(void){} */
/* void dpl_display(void){} */
/* void dbg_putstr(char *string){} */
/* void dbg_tx(void){} */
/* enum inite_return dbg_tim_init(Initt_system_state *ssp){} */
 Knt_type kn_qcreate(Que_id que_id, int size){}
 Knt_type kn_qpost(Que_id que_id,void *p){}
 void *kn_qaccept(Que_id que_id, Knt_type *err){}

