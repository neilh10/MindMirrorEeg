/* dpl.c
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
 Copyright Neil Hancock, 1992-1995

 Display Module - subprogram responsible for managing the display.
 This consists of translating digital filter outputs to a virtual screen.


 The following modes of display are handled
          * Bargraph format
    NO    * Horizontal History Display with time
    NO    * Peak absolute amplitude with time

 These are to be displayed on the following virtual screens
          * Display TYPE_15BY2 = 15 bargraphs by 2
     ??   * Display TYPE_30BY2 = 30 bargraphs by 2 - not implemented
     ??   * Display TYPE_HISTORY                   - not implemented
     ??   * Display TYPE_ABS_AMPLITUDE             - not implemented

 This module draws bargraphs on a virtual screen. Device drivers
 implement the virtual to physical screen mapping.

 The bar graphs are decoded on a virtual screen.

 The virtual screen consists of

           Left       Right
          Column     Column
        +====================+
 [0,0]  ! +------+ +-------+ ! [1,1]
        ! |  <-- | | -->   | !
        ! +------+ +-------+ !
        ! :      : :       : !
        ! +------+ +-------+ !
        ! |  <-- | | -->   | !
        ! +------+ +-------+ !
 [14,0] +====================+ [14,1]
  OR
 [29,0]                        [29,1]


 Algorithms

 The dpl module functions are called in the following order
    dpl_initialization() *** Only once, or on changing display type ***
    dpl_display()        *** To cause a display update ***

 Internal Data

 This module maintains the following internal data structures

  bargraph_pos[] - defines the present position of active bargraphs
  scrn_map[]     - maintains a description of how the bargraph_new[]
                  data maps to the active virtual display


*/

#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "general.h" /* There is different depending on whether
                      * MMIR_UNIT or MMIR_PC being compiled
                      */
#ifdef MMIR_UNIT
#include "sch.h"
#include "iir.h"
#include "dbg.h"
#endif /* endif MMIR_UNIT */
#include "hw.h"
#include "uif.h"
#include "dpl.h"
#include "dpl_stat.h"
#include "mm_coms.h"
#include "mfd.h"
#include "mmstatus.h"
#include "dim.h"
#include "proto.h"
/*#ifdef MMIR_PC
#include "pmd.h"
#endif /* endif MMIR_PC */

/* Constants */
/*#define BARGRAPH_POS_DIFF 11
*#define LF_START_POS 185
*#define LB_START_POS 180
*#define RF_START_POS LF_START_POS
*#define RB_START_POS LB_START_POS
*/

#define CUR_SIZE LDDE_5BIT
 enum dple_valid {NOT_VALID=0,VALID};
 enum dple_x_pos {LEFT_SIDE=0,RIGHT_SIDE=1};
 enum dple_inc_by {FOR_2_LOBES=1,FOR_4_LOBES=2};

/* Shared variables */

/* 
 * Externally defined 
 */
 extern Dplt_data bargraph_new;
 extern enum UIFE_SCREEN_STATE uife_screen_state;

/* *******************************
 * Internal storage to this module 
 */
 int dpl_sch_pass_cnt;
/*
 * bargraph arrays
 * 
 * bargraph_pos
 * This describes the present position of the 
 * displays.
 */
 Dplt_data bargraph_pos;

/*
 * Structure defining what lobes are displayed
 */
 typedef struct {
  enum dple_valid valid;  /* NOT_VALID/VALID - whether lobe to be displayed */
  enum dple_x_pos x_pos;  /* LEFT_SIDE or RIGHT_SIDE */
  int  y_pos;             /* 0 to 29 */
  enum dple_inc_by inc_by;/* depends on number of lobes */
  int  number_bargraphs;
 } Dplu_struct;

 static Dplu_struct scrn_map[NUMBER_LOBES];

/* Status line for dpl_stat.c routines */
 char mmir_status_buf[MMIR_STATUS_BUF_SIZE];

/* Internal prototypes */
/* void update_bargraphs(enum sche_entry sch_type); */

 /*;*<*>********************************************************
 * dpl_initialisation
 *
 * This function initialises the logical display
 * 
 * A COLD initialisation will clear the display and redraw all
 * text and layout information
 *
 * A WARM initialisation will attempt to refresh the display. If
 * it can't it will perform a COLD initialisation.
 *
 * The following data structures are initialised
 *  scrn_map[]
 *
 **start*/
 enum sys_return dpl_initialisation(Initt_system_state *ssp)
 {
/* int startpos; */
 Byte *posp,*newp;
 register int lobe_lp, elem_lp, loop;
 /* unsigned num_b; */
 Dplu_struct *dpl_cntrlp=scrn_map;

    if (ssp->init_type == COLD) {
       /* - Initialise and hence write all 16 characters of status line */
       strcpy(mmir_status_buf,"     hello!     ");
    }
    /*
     * Initialise the display control structure
     */
    for(lobe_lp=0; lobe_lp < NUMBER_LOBES; lobe_lp++) {
       scrn_map[lobe_lp].valid = NOT_VALID;
    }

    /*
     * 2 lobes to be displayed
     * Default to displaying left and right front lobes
     */
    scrn_map[LF_LOBE].valid  = VALID;
    scrn_map[LF_LOBE].x_pos  = LEFT_SIDE;
    scrn_map[LF_LOBE].y_pos  = 0;
    scrn_map[LF_LOBE].inc_by = FOR_2_LOBES;
    scrn_map[LF_LOBE].number_bargraphs = NUMBER_BARGRAPHS;

    scrn_map[RF_LOBE].valid  = VALID;
    scrn_map[RF_LOBE].x_pos  = RIGHT_SIDE;
    scrn_map[RF_LOBE].y_pos  = 0;
    scrn_map[RF_LOBE].inc_by = FOR_2_LOBES;
    scrn_map[RF_LOBE].number_bargraphs = NUMBER_BARGRAPHS;


    for(lobe_lp = 0; lobe_lp < NUMBER_LOBES; lobe_lp++,dpl_cntrlp++){
       /*
        * Repeat for each valid lobe
        */
      /* if (dpl_cntrlp->valid) { */
	 posp   = &(bargraph_pos.lobe_data[lobe_lp].bar[0]);
	 newp   = &(bargraph_new.lobe_data[lobe_lp].bar[0]);

         /*num_b  =  dpl_cntrlp->number_bargraphs;*/
         for(elem_lp = 0; 
             elem_lp < NUMBER_BARGRAPHS /*num_b*/;
             elem_lp++){
            /*
             * Repeat for each bargraph
             */
             *posp++ = 0;
             *newp++ = 0;
         }
     /* } */
   }

/*   for (loop = 0; loop < MMIR_STATUS_BUF_SIZE; loop++) {
*      mmir_status_buf[loop] = ' ';
*   }
*/
   for (loop=(int)TXT_LN_FIRST_FILTER;loop <= (int)(TXT_LN_LAST_FILTER); loop++) {
      /* The following sets the graduation marks on the screen for the filters
       * This is calculated as being 2/3 of FSD
       * ie 63 * 0.667 = 42 dots
       * Hence on a centered scale of 0 to 127 this is
       * at x-axis (63-42=)21 & (66+42=)108
       */
      /*ldd_wrchar(2,5,LDD_CHAR1);
      ldd_wrchar(14,5,LDD_CHAR2);*/
      /* This results in the RHS '|' at 109 */
      ldd_line(loop,(uchar *)"  |          |  ");
   }
   /* EMG graduation are different */
   ldd_line((TXT_LN_LAST_FILTER+1),(uchar *)"    |      |    ");

   ldd_status(mmir_status_buf);


   /*
    * Initialise dependicies
    */
   dpl_display(SCHE_FAST);

   dpl_stat_initialisation();

 return SYS_OK;
 } /*end dpl_initialisation */

 /*;*<*>********************************************************
 * dpl_display
 *
 * This function is responsible for updating the display.
 *
 * It is invoked by the scheduler with
 *     SCHE_FAST
 *     SCHE_BACKGND
 *
 * When called with the SCHE_FAST, is used to reset the
 * screen updates. This indicates new data has been made available
 * and starts the display updates at the fastest section of the LCD.
 *
 * When called with the SCHE_BACKGND is used to perform the actual updates
 * on the LCD. However only one update is performed before control is
 * surrended back to the scheduler - in the interests of multitasking.
 *
?????
 * For the filters outputs (including EMG) data:-
 *    obtain a snapshot of filtered data
 *    translate the filtered data to an internal representation
 *    difference the data and orginate calls to logical device driver
 *
?????
 * For the gain values
 *    if needed update the screen to reflect new gain
 *
 * 
 **start*/
 void dpl_display(enum sche_entry sch_type)
 {
 Dplt_data *bargraph_newp; /* Not Static !!!! */
 static /*register*/ Byte *posp1,*posp2,*newp1,*newp2;
 static /*register*/ unsigned  y_pos1, y_pos2;
 static /*register*/ Dplu_struct *dpl_cntrlp1, *dpl_cntrlp2;

 static /*register*/ int elem_lp;
 static /*register*/ unsigned req_newvalue, oldvalue;

#define MAXIMUM_NUMBER_SCH_CALLS (NUMBER_BARGRAPHS*NUMBER_LOBES)

    if (uife_screen_state != UIFE_SCREEN_ACTIVE) {
       /*
        * Screen is inactive - don't do any updates
        */
       return;
    }

    if (sch_type == SCHE_FAST) {
       /*
        * Reset screen pointers so starts at begining of poll
        */
       dpl_sch_pass_cnt = 0;
       /*
        * Initialise for next working pass
        */
       mfd_supply_data(_ModuleDpl_,(void *) &bargraph_newp);
       posp1   = &(bargraph_pos.lobe_data[0].bar[0]);
       newp1   = &bargraph_newp->lobe_data[0].bar[0];
       posp2   = &(bargraph_pos.lobe_data[1].bar[0]);
       newp2   = &bargraph_newp->lobe_data[1].bar[0];
       dpl_cntrlp1=&scrn_map[0];
       dpl_cntrlp2=&scrn_map[1];

       y_pos1  =  dpl_cntrlp1->y_pos;
       y_pos2  =  dpl_cntrlp2->y_pos;
       elem_lp =  0;

       /*
        * Update status line
        */
       dpl_status(&mmir_status_buf[0],
                  bargraph_newp->status,
                  bargraph_newp->time_stamp,
                  bargraph_newp->lobe_data[LF_LOBE].gain
                  );
       ldd_status(mmir_status_buf);

       return;

    } else if (dpl_sch_pass_cnt >= MAXIMUM_NUMBER_SCH_CALLS) {
      /*
       * Take no action - in the interests of co-operative
       * multi-tasking.
       */
      return;
    }
    /*
     * We have a working pass
     */

    if (!(dpl_sch_pass_cnt++ & 1)) {
       /*
        * Do for left hemisphere
        * This pass is done first
        */
       req_newvalue = BYTE_MASK & ((unsigned) *newp1);
       oldvalue = BYTE_MASK & ((unsigned) *posp1);

       /*
        * Request update on screen and update new position
        */
       *posp1 = (Byte) (oldvalue + ldd_graphics_bargraph(
                            (unsigned) dpl_cntrlp1->x_pos,
                                                   y_pos1,
                                                 oldvalue,
                      ((int)req_newvalue - (int)oldvalue) /* The difference */
                                                        )
                       );
    } else {
       /*
        * Do for Right hemisphere
        * This pass is done after the Left hemisphere and must therefore
        * update the pointers
        */
       req_newvalue = BYTE_MASK & ((unsigned) *newp2);
       oldvalue = BYTE_MASK & ((unsigned) *posp2);

       /* request update on screen */
       *posp2 = (Byte)( oldvalue + ldd_graphics_bargraph(
                            (unsigned) dpl_cntrlp2->x_pos,
                                                   y_pos2,
                                                 oldvalue,
                                (req_newvalue - oldvalue) /*The difference*/
                                                        )
                      );
       /*
        * Get ready for next pass
        */
       elem_lp++,newp1++,posp1++,newp2++,posp2++;
       y_pos1 += (unsigned)dpl_cntrlp1->inc_by;
       y_pos2 += (unsigned)dpl_cntrlp2->inc_by;
    }

 } /*end dpl_display */

 /*;*<*>********************************************************
 * 
 **start*/
/* ()
 {

 } /*end*/
   




