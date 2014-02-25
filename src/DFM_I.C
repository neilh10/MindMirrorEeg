/* dfm_i.c
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
   dfm_peak_filter_values() to snapshot the absolute maximum
            values, for display processing.
   abs_max
   dfm_adjust_elem

 */
#ifdef IC96
#include <h\cntl196.h> /* Nonstandard version */
#pragma nolistinclude
#endif
#include "general.h"
#include "iir.h"
#ifdef SIMULATING
#include "stdlib.h"
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
 const Dfmt_gain_adj dfmu_gain_adj = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 1};

/* Externally defined variables */
 extern Dfmt_data dfmu_data;

/**************************************************************
 *
 * Internal storage to this module 
 */

/*;*<*>********************************************************
  * dfm_peak_filter_values
  *
  * This function takes a snapshot of the outputs of the active
  * filters. 
  * 
  * After taking a snapshot, the peak outputs are all set back to
  * zero.
  *
  * This function assumes that the digital filter routines will not
  * run while it is active. ie not a multitasking system 
  *
  * This function assumes all exports are to the mfd module.
  **start*/
 void dfm_peak_filter_values(int number_elem, Mfdt_data *p)
 {
 register int lobes_lp, filter_lp;
 register Mfdt_lobe_data  *to_ldp=&(p->lobe_data[0]);
 register Dfmt_lobe_data  *fm_ldp=&(dfmu_data.lobe_data[0]);
 register Frac *top, *fmp;
 /*register Frac *gainp=&(dfmu_gain_adj.elem[0]); */


    for(lobes_lp = 0; lobes_lp < NUMBER_LOBES;
                  lobes_lp++,to_ldp++, fm_ldp++) {
       to_ldp->emg = fm_ldp->emg;
       /*fm_ldp->emg = 0; /* Init back to zero as maximum */
       to_ldp->gain = fm_ldp->gain;
       to_ldp->input= fm_ldp->input128;
       to_ldp->status = fm_ldp->status;
       top = &(to_ldp->elem[NUMBER_FILTERS-1]);
       fmp = &(fm_ldp->elem[NUMBER_FILTERS-1]);
       for(filter_lp = 0; filter_lp < number_elem; filter_lp++) {
		   *top-- = /* (*gainp * */ *fmp;
		   *fmp-- = 0; /* reset back to 0 as max value*/
       }
    }

 } /*end dfm_peak_filter */


/*;*<*>********************************************************
  * abs_max
  *
  * This function determines the abs magnitude of 'in1' and 
  * then returns the larger, in1 or in2;
  *
  * Input
  *   in2 - 16bit signed fraction
  *   in1 - 16bit unsigned value
  *
  **start*/
 Word abs_max(Word in1, Frac in2)
 {
    if (in2 < 0)
       in2 = ~in2+1;
    return max(in1,in2);

 } /*end abs_max*/

/*;*<*>********************************************************
  * dfm_adjust_elem
  * 
  * adjust the numbering of the way the elements are accessed 
  * so that
  *            (value in elem_monitored)
  *  filter number     0                last filter implemented
  *  filter number     1                second last filter implemented
  *     :      :       n                 :     :
  *  filter number NUMBER_FILTERS-1     1st filter implemented
  *  filter number TOT_NUMBER_FILTERS-2 16Hz low pass filter
  *  filter number TOT_NUMBER_FILTERS-1 32Hz low pass filter
  **start*/
 dfm_adjust_elem(int elem)
 {
    if (elem < NUMBER_FILTERS)
       elem = NUMBER_FILTERS -1 -elem;

    return elem;

 } /*end dfm_adjust_elem*/

/*;*<*>********************************************************
  * 
  **start*/
/* ()
 {

 } /*end*/
