/*  ssp_uart.c
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
  This module is responsible for accepting a buffer and 
  transmitting it on the UART. It uses the PTS which places
  special requirements on where the ram is located.

  Functions
     ssp_send_buf  - verified 15/June/92
     ssp_end_int

Algorithms

Internal Data
*/
#ifdef IC96
#include <h\ncntl196.h>
#pragma regconserve
#pragma nolistinclude
#endif
#include <80C196.hr>
/*#include <stdio.h>
#include <math.h>
#include <stdlib.h>
*/
#include "general.h"
#include "ssp.h"
#include "pts.h"
#include "hw.h"

/**************************************************************
 * Constants 
 */

/**************************************************************
 * Externally defined 
 */
 extern Sspt_manage manage;
 extern Pts_single_transfer pts_ti_table;

/**************************************************************
 * Internal storage to this module 
 *
 */


/**************************************************************
 * Internal prototypes 
 */

/*;*<*>********************************************************
 * ssp_send_buf
 * 
 * This function takes a buffer and sets up the PTS
 * to transmit it.
 *
 * See page 5-40 Figure 6-8 for example code
 *
 * Assumptions
 *    The PTS global enable bit is active, including in time consuming ISRs
 *    The TI interrupt is enabled
 *
 * 17/July/92
 * Tested by sending out the following 70 characters
 *  "\n\rNJH  #1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"; 
 *  repeatedly. The two spaces were filled with a number.
 * 256 buffers were sent out in 128seconds, and they were received by PROCOMM
 * This is at the rate of 2 buffers a second.  The amount of screen update
 * did not affect the buffer. The effective transmitted bit rate is 
 * 140*8 = 1120 bits/sec
 *
 * At 4 buffers a second truncation happened on the 'u' of each message,
 * implying a maximum throughput of 65*4 = 260 characters per second. 
 * This gives an upper limit of 65*4*8 = 2080 bits/sec.
 *
 * The baud rate is set to 9600baud, with start, stop and parity, or 
 * a theoritical maximum of approx 872 chars per/second.
 *
 * The efficiency of the link appears to be somewhere in the
 * order of 260/872 = 30%. This is puzzling as I would expect it
 * to be higher.
 *
 **start*/
 void ssp_send_buf(unsigned char *buf,unsigned int number)
 {

    /*
     * Set up constants in table
     * These could be moved to an initialisation section
     */
    pts_ti_table.ptscon = PTS_UART_TX_CTL;
    pts_ti_table.ptsdst = (void *) &sbuf;

    /*
     * Initialise table for new buffer
     */
    pts_ti_table.ptscount = number - 1;
    pts_ti_table.ptssrc = buf +1;

    /*
     * Enable PTS Tx servicing
     */
#define HWINDOW1 1
#define EN_PTS_TI_SERVICE PTS_TI_MASK
#define INT1_TI_MASK_NEG 0xfe

asm  pusha; /* Save psw/int_mask/int_mask1/wsr and disable them*/
asm  ldb  wsr,#HWINDOW1;
asm  or   ptssel,#EN_PTS_TI_SERVICE;
asm  andb ipend1,#INT1_TI_MASK_NEG;   /* Squash any pending TI ints  */
asm  popa;                                /* Restore registers and enable*/
asm  orb  imask1,#INT1_TI_MASK;        /* Enables TI interrupt        */
/*asm  epts; */

    /* Start transmission manually */
    sbuf = *buf;

 } /*end ssp_send_buf */

/*;*<*>********************************************************
 * ssp_end_int
 *
 * This function is called at interrupt level when a physical link
 * layer message has finished.
 *
 * Called at interrupt level.
 *
 * The buffer semaphore manage.tx_buffer_available is set to
 *  TX_BUF_AVAILABLE;
 **start*/
 void ssp_end_int(void)
 {

  manage.tx_buffer_available = TX_BUF_AVAILABLE;

 } /*end ssp_end_int*/

/*;*<*>********************************************************
 * 
 **start*/
/* ()
 {

 } /*end*/

/*;*<*>********************************************************
 * 
 **start*/
/* ()
 {

 } /*end*/

