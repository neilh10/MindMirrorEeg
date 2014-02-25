/* drm test programs - not for production build
  drm_tst.c
 
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
  test_linear_ram - tests out the following primitives
      drm_ram_store
      drm_ram_get

Algorithms

Internal Data

*/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
#include <stdio.h>
/*#include <math.h>*/
/*#include <stdlib.h>*/

#include "general.h"
#include "iir.h"
#include "proto.h"
#include "drm.h"
#include "ssp.h"

/**************************************************************
 * Constants 
 */
  const Dplt_data trial = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,
 50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67};


/**************************************************************
 * Externally defined 
 */

/**************************************************************
 * Internal storage to this module 
 */
 extern int debug;
 long int pass, bad;
 int ok;
 enum brame_coop  {
    BRAM_COOP_WRITE_START,
    BRAM_COOP_WRITE,
    BRAM_COOP_READ_START,
    BRAM_COOP_READ
 };
 enum brame_coop bram_coop_state;
 int bram_coop_loop;

/**************************************************************
 * Internal prototypes 
 */
 void temp_print(char *p,int size);
 int cmp_buffers(char *p1,char *p2,long int *addr);

/*;*<*>********************************************************
 * test_linear_ram
 * 
 * This test fills the banked ram as a linear array, and then 
 * checks the result. 
 * 
 * It does this continuously, and never exits.
 *
 * It outputs the results on the serial port.
 *
 * Assumptions
 *  Owns serial port
 **start*/
 void test_linear_ram(void)
 {
  Initt_system_state ssp;
  long int tupl_addr,pass=0, bad=0;
  char ret_buf[80], scrn_buf[100];
  register int loop, ok;

    ssp.init_type = COLD;

    drm_ram_initialisation(&ssp);
    do {
       ok = 0;
       sprintf(scrn_buf,"\n\rWriting data pass=%4.4X, ",++pass);
       dbg_putstr(scrn_buf);

       drm_ram_control(NULL,DRMC_RESET);
       for( loop=0; loop < 2970; loop++) {
          drm_ram_store(&trial,sizeof(Dplt_data));
       }

       tupl_addr = 0;

       sprintf(scrn_buf,"Reading data, ");
       dbg_putstr(scrn_buf);

       for( loop=0; loop < 2970; loop++, tupl_addr += sizeof(Dplt_data)) {
          drm_ram_get(&tupl_addr,&ret_buf[0],sizeof(Dplt_data));

          if ( cmp_buffers((char *)&trial, &ret_buf[0], &tupl_addr) ) {
             ok++;
          } else {
             bad++;
         }
       }
       sprintf(scrn_buf," OK this pass=%4X, Total BAD=%8lX",
                 ok, bad);
       dbg_putstr(scrn_buf);
    } while(1);

 } /*end test_linear_ram*/

/*;*<*>********************************************************
 * test_bram_cooperatively
 * 
 * This test fills the banked ram as a linear array, and then 
 * checks the result. 
 * 
 * It does this by writing/reading a record every time called.
 *
 * It outputs the results on the serial port.
 *
 * Assumptions
 *  Owns serial port
 *  init_test_bram_cooperatively called first
 *  drm_ram_initialisation() called first
 *
 **start*/
 void test_bram_cooperatively(void)
 {
  static long int tupl_addr;
  register int loop;
  char ret_buf[80], scrn_buf[100];

   switch(bram_coop_state) { 
   case BRAM_COOP_WRITE_START:
   default:
      bram_coop_loop = 0;
      ok = 0;

      sprintf(scrn_buf,"\n\rWriting data pass=%4.4X, ",++pass);
      dbg_putstr(scrn_buf);

      /* Set pointers to beginning of buffer */
      drm_ram_control(NULL,DRMC_RESET);
      bram_coop_state = BRAM_COOP_WRITE;
      /* Fall through */

   case BRAM_COOP_WRITE:
#define MAX_NUMBER_BRAM_TUPLS 2970
      drm_ram_store(&trial,sizeof(Dplt_data));
      if (++bram_coop_loop >= MAX_NUMBER_BRAM_TUPLS) {
         bram_coop_state = BRAM_COOP_READ_START;
      }
      break;

   case BRAM_COOP_READ_START:
      tupl_addr = 0;
      bram_coop_loop = 0;

      sprintf(scrn_buf,"Reading data, ");
      dbg_putstr(scrn_buf);
      bram_coop_state = BRAM_COOP_READ;

   case BRAM_COOP_READ:

      drm_ram_get(&tupl_addr,&ret_buf[0],sizeof(Dplt_data));

      if ( cmp_buffers((char *)&trial, &ret_buf[0], &tupl_addr) ) {
         ok++;
      } else {
         bad++;
      }
      /* Set up for next pass */
      tupl_addr += sizeof(Dplt_data);

      if (++bram_coop_loop >= MAX_NUMBER_BRAM_TUPLS ) {
         bram_coop_state = BRAM_COOP_WRITE_START;
         sprintf(scrn_buf," OK this pass=%4X, Total BAD=%8lX",
                 ok, bad);
         dbg_putstr(scrn_buf);
      }
      break;
   }

 } /*end test_bram_cooperatively*/

/*;*<*>********************************************************
 * init_test_bram_cooperatively
 *
 **start*/
 void init_test_bram_cooperatively(void)
 {
    bad = 0;
    pass = 0;
    bram_coop_state = BRAM_COOP_WRITE_START;

    /* Turn of RT communications so that can use UART */
    ssp_action(SSP_TX_RT_DIS);


 } /*end init_test_bram_cooperatively*/


/*;*<*>********************************************************
 * temp_print
 *
 * Prints the required number of hex chars to screen
 **start*/
 void temp_print(char *p,int size)
 {
 int loop, pos =0;
 char buf[10];

    sprintf(buf,"\r\n");
    dbg_putstr(buf);

    for (loop = 0; loop < size; loop++) {
       sprintf(buf,"%2.2X ",*p++);
       dbg_putstr(buf);
       pos += 3;
       if ( pos >77) {
          pos = 0;
          sprintf(buf,"\r\n");
          dbg_putstr(buf);
       }
    }

    sprintf(buf,"\r\n");
    dbg_putstr(buf);


 } /*end temp_print*/

/*;*<*>********************************************************
 * cmp_buffers
 **start*/
 int cmp_buffers(char *p1,char *p2,long int *addr)
 {
 int loop, ret=1;
 char scrn_buf[85];

    for (loop = 0; loop < sizeof(Dplt_data); loop++,p1++,p2++) {
       if (*p1 != *p2) {
          ret = 0;
          sprintf(scrn_buf,"\n\rDif addr=%8.8lX loop=%2.2d %2.2X,%2.2X",
                 *addr,loop,*p1,*p2);
          dbg_putstr(scrn_buf);
       }
    }
    return ret;
 } /*end cmp_buffers*/

/*;*<*>********************************************************
 * xyz
 **start*/
/* xyz()
 {

 } /*end xyz*/

