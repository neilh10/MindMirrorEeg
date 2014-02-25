/* dfm_i196.c
 * 
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
   contains modules for initialising 80C196 versions of dfm modules
   (simulator has different initialise_iir_i call)

   initialise_iir_i

 */
#ifdef IC96
#include <h\cntl196.h> /* Nonstandard version */
#pragma nolistinclude
#endif
#include "general.h"
#include "iir.h"
#ifdef SIMULATING
/*#include "pc_only.h"*/
#endif
#include "mm_coms.h"
#include "dim.h"
#include "proto.h"

/*************************
 * Internal enums/typedefs 
 */

/* Constants */

/*******************
 * Shared variables 
 */
/* Externally defined variables */
/* extern Dfmt_data dfmu_data; */

/**************************************************************
 *
 * Internal storage to this module 
 */
/*;*<*>********************************************************
 * 
 * initialise_iir_i
 *
 *    Initialise delay storage locations to 0
 *     for fraction iir.
 *
 **start*/
 int initialise_iir_i(Iir_filter_descriptor *ip,int number_lobes)
 {
 Biquad *p;
 int loop_i, lobe_lp;

 for (lobe_lp=0; lobe_lp < number_lobes; lobe_lp++) {
    p = ip->biquad[lobe_lp];
    for (loop_i=0; loop_i < ip->number_of_sections; loop_i++) {
       p->z1_f = 0;
       p->z2_f = 0;
       p++;
    }
 }
 return 0;
 } /*end init_iir_i */

/*;*<*>********************************************************
  * 
  **start*/
/* ()
 {

 } /*end*/




