/* ssp_test.c
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
 This module contains routines used to verify the functionality
 of ssp.c module. 

Algorithms

Internal Data
*/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "general.h"
#include "ssp.h"
#include "mm_coms.h"
#include "dbg.h"

/**************************************************************
 * Constants 
 */
 const Dplt_data ssp_bargraph_new = {
 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,/*LF bargraphs */
 0x8aa5,0x8b81,0x8c82, /* input gain status */
 0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,/*RF bargraphs */
 0x9aa6,0x9b91,0x9c92 /* input gain status */
 };

/**************************************************************
 * Externally defined 
 */

/**************************************************************
 * Internal storage to this module 
 */

/**************************************************************
 * Internal prototypes 
 */

/*;*<*>********************************************************
 * ssp_send_buf_test
 *
 * This function tests ssp_send_buf().
 *
 **start*/
 void ssp_send_buf_test(void)
 {
#define INIT_CNTR  0
 static unsigned char cntr;
 const unsigned char buf[DBGC_BUFFER_SIZE] =
 "\n\rNJH  #1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
#define STR_LEN 70
#define BUF_SPACE  5


/*    buf[BUF_SPACE] = ntoa((cntr&0xf0) >> 4); */
/*    buf[BUF_SPACE+1] = ntoa(cntr&0x0f); */
    ++cntr;
    ssp_send_buf((unsigned char *)&buf[0],(unsigned int) STR_LEN);

 } /*end ssp_send_buf_test*/

/*;*<*>********************************************************
 *  send_dll_test
 *
 **start*/
 send_dll_test()
 {
 unsigned char buf[DBGC_BUFFER_SIZE] = 
    {0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5};
 static unsigned char dll_cnt;

      buf[DLL_PAYLOAD_OFFSET] = dll_cnt; /* add a payload */
      send_dll(&buf[0], /* Buffer */
                     1, /* Size   */
             dll_cnt++, /* Checksum */
                  0x69); /* Dlle_type */

 } /*end send_dll_test*/

 /*;*<*>********************************************************
 * ssp_mfd_supply_data
 *
 * This function supplies a similar value of mfd_supply_data 
 * for test purposes
 *
 **start*/
 Dplt_data *ssp_mfd_supply_data(_eModuleId type)
 {
   /*
    * Return with the address of the data
    */
   return (Dplt_data *) &ssp_bargraph_new;

 } /*end ssp_mfd_supply_data*/

/*;*<*>********************************************************
 * 
 **start*/
/* ()
 {

 } /*end*/
