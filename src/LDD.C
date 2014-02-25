/* LDD.c
 * 
 * http://www.biomonitors.com/
 * Copyright (c) 1992-2014 Neil Hancock
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
 LCD Device Driver Module for the PC

 This module is repsonsible for implementing virtual screen to 
 physical screen mapping. The virtual screen formats expected
 are

    15 by 1 bar graphs - for test
    15 by 2 bar graphs
    30 by 2 bar graphs
    Horizontal History
    Absolute Peak Amplitude

 This module is configured for a physical screen of 

     128 * 128

 Entry points are

    ldd_initialisation()
    ldd_graphics_bargraph()
    ldd_text_write()
    ldd_status(string) - writes status line

 To display 15 pairs of bargraphs with a header we have 
 a choice of the following

 +--------------+----------------+------------+--------+
 |Back Bargraph | Front Bargraph | Seperation | Header |
 +--------------+----------------+------------+--------+
 |    6         |      6         |     1      |  5     |
 |    6         |      6         |     0      | 20     |
 |    5         |      5         |     1      | 35     |
 +--------------+----------------+------------+--------+

  Try the  5/5/1 combination to begin with


*/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
/*#include <stdio.h>*/
/*#include <math.h>*/

#include <stdlib.h>

/*#include <graphics.h> */

#include "general.h"
/*#include "iir.h"*/
#include "ldd.h"
/*#include "proto.h"*/
#ifdef SIMULATING
/*#include "pc_only.h"*/
#endif


/***************************************************************
 *   Internal storage 
 ***************************************************************/
/*
 * Defines the module specific information
 */
 static Ldd_defs ldd_defs; 
 const char ldd_char1[8] = {1,1,1,0,1,1,1,0};
 const char ldd_char2[8] = {0x8,0x8,0x8,0,0x8,0x8,0x8,0};
/* Shared variables */

/* Externally defined */

#define LDDC_BAR LDDE_7BIT

/* Internal prototypes */
 void bar_15by1(int x_bar,int y_bar,
      int oldvalue_bar,int diff_bar);
 void bar_15by2(int x_bar,int y_bar,
      int oldvalue_bar,int diff_bar);

 /*;*<*>********************************************************
 * ldd_initialisation
 *
 * This is the master initialisation call for this subprogram.
 *
 * The initialisation is more complicated for this subprogram 
 * as some modules are specific for the PC simulator with corresponding
 * specific modules for the target MMIR environment.
 *
 * The modules specific to the PC environment are 
 *             ldd_pc.c - simulation 
 * specific for the LCD screens are
 *             ldd_1391.c - TLC-1391-E0
 *
 * If 
 *   INITE_TYPE_15BY1 then create a 128 by 128 window
 *   INITE_TYPE_15BY2 then create a 128 by 128 window
 *   INITE_TYPE_30BY2 then create a 640 by 200 window
 *
 **start*/
 enum inite_return ldd_initialisation(Initt_system_state *ssp)
{
 Ldd_defs *defsp=&ldd_defs;

    /*
     * Determine the type of screen attatched
     */
    ldd_determine_scrn(defsp);
    /*
     * Initialise device specific screen
     */
    ldd_init_device(defsp);
    ldd_init_cg_ram(LDD_CHAR1,&ldd_char1);
    ldd_init_cg_ram(LDD_CHAR2,&ldd_char1);

    return PASS;
 } /*end ldd_initialisation */

 /*;*<*>********************************************************
 * ldd_graphics_bargraph
 *
 * This function translate an update request from a virtual
 * screen to physical screen.
 *
 * The virtual screen is 15 by 2 bargraphs. Each bargraph has
 * a range of 0 to 255.
 *
 * These values are translated to a physical bargraph driver
 * 15 by 2 bargraphs, range 1 to 63. The special case of 0
 * is squashed so as to leave a clear space in the center of the
 * bargraphs.
 *
 * Input
 *   x_bar     Range 0,1
 *   y_bar     Range 0 to 14
 *  oldvalue   The value the bar was, Range 0 to 255, maybe 0 to 0xffff
 *  diff       The difference of the bar Range  -256 to +256,maybe 0 to 0xffff
 *  Assumptions
 *     oldvalue + diff <= 256
 *  See routine range_ldd_graphics_bargraph() for simulator range check
 *
 * Returns
 *     0     - if no update was performed
 *     value - of the update performed. The returned value is of the
 *             same scale as that supplied by the caller
 *
 **start*/
 int ldd_graphics_bargraph(unsigned x_bar, unsigned y_bar,
      unsigned oldvalue,int diff)
 {
 register Ldd_defs *defsp = &ldd_defs;
 register int diff_bar=diff; /* Helps for speed*/
 register unsigned oldvalue_bar=oldvalue;

#define MAXIMUM_BARGRAPH_VALUE 0xff
       /*
        * Do some checking
        */
       if ( oldvalue_bar > MAXIMUM_BARGRAPH_VALUE) {
          /*
           * More than the allowed maximum so cap at maximum
           */
           oldvalue_bar = MAXIMUM_BARGRAPH_VALUE;
       }
       /* Do combined range check */
       if ( ((diff_bar+oldvalue_bar) > (MAXIMUM_BARGRAPH_VALUE+1)) ||
            (diff_bar+oldvalue_bar  < 0))
          {
          /* Combined total is too much, limit diff_bar */
          if (diff_bar < -1) {
             diff_bar = - oldvalue_bar;
          } else {
             diff_bar = MAXIMUM_BARGRAPH_VALUE - oldvalue_bar;
          }
       }

       /*****************************************************
        *
        * Logical screen 15 bars by 2 bar
        *
        * Change scale 0 to 63
        */
#define CHANGE_SCALE_SHIFT 2
       oldvalue_bar >>= CHANGE_SCALE_SHIFT;
       if (diff_bar < 0 ) {
	  /* Change to positive number and shift, then change
	   * back to negative number. Shifting negative number
	   * doesn't work for -1 (ie 0xffff)
	   *
	   * Also decrement diff and increment oldvalue to
	   * insure that get rid of any bits that get lost
	   * in rounding down.
	   */
          diff_bar = (~((~diff_bar+1)>>CHANGE_SCALE_SHIFT) +1);
       } else {
          diff_bar >>= CHANGE_SCALE_SHIFT;
       }
       /* 
        * Check if an update can be performed 
        */
       if (diff_bar == 0)
          return 0;

       /*
        * Squash special case so that there isn't a bargraph for
        *    oldvalue_bar = 0, 
        */
       if (oldvalue_bar == 0) {
          oldvalue_bar++; /* Increment over special case */
          diff_bar--;     /* Adjust diff_bar to cope     */
          if (diff_bar == 0) {
             return 1 << CHANGE_SCALE_SHIFT;
          }
       }
       bar_15by2(x_bar,y_bar,oldvalue_bar,diff_bar);

       /*
        * Return the amount of update actually performed
        */
       return (diff_bar << CHANGE_SCALE_SHIFT);

 } /*end ldd_graphics_bargraph */

 /*;*<*>********************************************************
 * bar_15by2
 *
 * This draws a graph on a physical 128 by 128 screen as follows
 *
 * The input is expected to be in range 0->63, and no checking is
 * performed to enforce this.
 *
 *        -- X -->
 *  [0,0] +--------------------+ [127,0]
 *     |  |  <--0,14 1,14-->   |
 *     |  |                    |
 *     |  |  <-diff- -diff->   |
 *   Y |  |                    |
 *    \|/ |  <--0,0   1,0-->   |
 *        | blank              |
 *[0,127] +--------------------+ [127,127]
 *  
 *   y_bar 0      1   ...  13   14
 *           translates to
 *   y_pos 112              8     0
 *
 *   x_bar 0     1
 *   translates to 
 *   y_pos 63   64
 *
 * Inputs
 *    x_bar         Range 0,1
 *    y_bar         Range 0 to 14
 *    oldvalue_bar  Range 0 to 63
 *    diff_bar      Range -63 to +63, But not 0
 *
 **start*/
 void bar_15by2(int x_bar,int y_bar,int oldvalue_bar,int diff_bar)
 {
 register int lp, end, x_pos,

    /*
     * Invert logical to physical position and
     * then multiply by 8
     */
    y_pos = ((NUMBER_BARGRAPHS - 1 - y_bar) << 3);
#define X0_OFFSET 63
#define X1_OFFSET 64

    /*
     * Check which direction bargraph is going to grow in
     */
    if (x_bar == 0) {
       /*
        * Positive values of diff_bar mean decreasing (number wise)
        *   bargraph output
        */
       oldvalue_bar = X0_OFFSET - oldvalue_bar;
       end = oldvalue_bar - diff_bar;
       if (diff_bar < 0) {
          for(lp=oldvalue_bar; lp < end; /*lp++*/)
            lp += ldd_graphics_clr(LDDC_BAR,lp,y_pos,(end-lp));
       } else {
          /*for (lp=end; lp <= oldvalue_bar; lp++){*/
          for(lp=oldvalue_bar; lp > end; ) {
            lp += ldd_graphics_set(LDDC_BAR,lp,y_pos,(end-lp));
          }
       }
    } else {
       /*
        * Positive values of diff_bar mean increasing bargraph output
        */
       oldvalue_bar += X1_OFFSET;
       end = oldvalue_bar + diff_bar;
       if (diff_bar < 0) {
          for (lp=oldvalue_bar; lp > end; /*lp--*/)
            lp += ldd_graphics_clr(LDDC_BAR,lp,y_pos,(end-lp));
       } else {
          for (lp=oldvalue_bar; lp < end; /*lp++*/) {
            lp += ldd_graphics_set(LDDC_BAR,lp,y_pos,(end-lp));
          }
       }
    }
 return;
 } /*end bar_15by2*/

 /*;*<*>********************************************************
 * bar_15by1
 *
 * This draws a graph on a physical 128 by 128 screen as follows
 *
 *        ---diff-->
 *  [0,0] +--------------------+ [127,0]
 *     |  | 14 -->             |
 *     |  |                    |
 *   Y |  |                    |
 *    \|/ |  0 -->             |
 *        | blank              |
 *[0,127] +--------------------+ [127,127]
 *  inputs
 *   y_bar 0      1   ...  13   14
 *   y_pos 112              8     0
 *
 * Note: x_bar has no effect in this routine
 *
 **start*/
/* Not used
 int bar_15by1(int x_bar,int y_bar,int oldvalue_bar,int diff_bar)
 {
 register int lp,
     end = oldvalue_bar + diff_bar,
    /* 
     * Invert logical to physical position and
     * then multiply by 8
     *!/
     y_pos = ((NUMBER_BARGRAPHS - 1 - y_bar) << 3);


    if (diff_bar < 0) {
       for (lp=oldvalue_bar; lp >= end; lp--) {
	 ldd_graphics_clr(LDDC_BAR,lp,y_pos,1);
       }
    } else {
       for (lp=oldvalue_bar; lp <= end; lp++)
         ldd_graphics_set(LDDC_BAR,lp,y_pos,1);
    }

 return LDDE_PASS;
 } /*end bar_15by1*/

 /*;*<*>********************************************************
 * ldd_text_write
 *
 **start*/
/* void ldd_text_write()
 {
 return LDDE_PASS;
 } /*end ldd_text_write*/

 /*;*<*>********************************************************
 * ldd_status
 *
 * This function prints the characters in
 * string, on the status line of the TLX-1391
 *
 * The status line is defined as the bottom line.
 **start*/
 void ldd_status(char *string)
 {
   ldd_line(TXT_LN_STATUS, string);
   return;

 } /*end ldd_status */

 /*;*<*>********************************************************
 * tst_pattern
 *
 * Test pattern for specified hemisphere
 **start*/
 void tst_pattern(int hemisphere)
 {

 #define BAR_38HZ 14
 #define BAR_30HZ 13
 #define BAR_24HZ 12
 #define BAR_19HZ 11
 #define BAR_15HZ 10
 #define BAR_12_5HZ 9
 #define BAR_10_5HZ 8
 #define BAR_9HZ 7
 #define BAR_7_5HZ 6
 #define BAR_6HZ 5
 #define BAR_4_5HZ 4
 #define BAR_3HZ 3
 #define BAR_1_5HZ 2
 #define BAR__75HZ 1
 #define BAR_EMG 0

 bar_15by2(hemisphere,BAR_38HZ,0,1);
 bar_15by2(hemisphere,BAR_30HZ,0,7);
 bar_15by2(hemisphere,BAR_24HZ,0,20);
 bar_15by2(hemisphere,BAR_19HZ,0,36);

 bar_15by2(hemisphere,BAR_15HZ,0,50);
 bar_15by2(hemisphere,BAR_12_5HZ,0,57);
 bar_15by2(hemisphere,BAR_10_5HZ,0,62);

 bar_15by2(hemisphere,BAR_9HZ,0,62);
 bar_15by2(hemisphere,BAR_7_5HZ,0,57);
 bar_15by2(hemisphere,BAR_6HZ,0,50);

 bar_15by2(hemisphere,BAR_4_5HZ,0,36);
 bar_15by2(hemisphere,BAR_3HZ,0,20);
 bar_15by2(hemisphere,BAR_1_5HZ,0,7);
 bar_15by2(hemisphere,BAR__75HZ,0,1);

 bar_15by2(hemisphere,BAR_EMG,0,3);

 } /*end tst_pattern*/

 /*;*<*>********************************************************
 * ldd_bar_test
 * 
 * This function writes test bargraphs on the screen.
 **start*/
 void ldd_bar_test(void)
 {
#define LHEMISPH 0
#define RHEMISPH 1

    tst_pattern(LHEMISPH);
    tst_pattern(RHEMISPH);

 } /*end ldd_bar_test*/

#ifdef SIMULATING
 /*;*<*>********************************************************
 * range_ldd_graphics_bargraph
 *
 * This routine is compiled when simulating.
 * It provides a range check for ldd_graphics_bargraph()
 **start*/
 void range_ldd_graphics_bargraph(void)
 {

  ldd_graphics_bargraph(0,0,0,1);
  ldd_graphics_bargraph(0,0,0,255);
  ldd_graphics_bargraph(0,0,255,1);
  ldd_graphics_bargraph(0,0,255,-255);
  ldd_graphics_bargraph(0,0,255,-256);
  ldd_graphics_bargraph(0,0,255,2);
  ldd_graphics_bargraph(0,0,256,1);
  ldd_graphics_bargraph(0,0,0xffff,2);

 } /*end range_ldd_graphics_bargraph*/
#endif /*SIMULATING*/

 /*;*<*>********************************************************
 * 
 **start*/
/* ()
 {

 } /*end*/

