/*  uif_upld.c  uif UPLOAD processing
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
 This file contains procedures for controling and displaying the uploading
  of internal ram data to a remote computer.

 The sequence is
    clear and initialise text screen
    do till finish
       request a record from drm module
       send record to ssp module
       update screen as to record 
       wait till ssp module has finsihed sending record

 Algorithims
     uif_upld_initialsation  - first
     uif_upld - as many times as needed afterwards

 */
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
#include <string.h>
#include "general.h"
#include "hw.h"
#include "uif.h"
#include "drm.h"
#include "ssp.h"
#include "ldd.h"

/**************************************************************
 * Constants 
 *
 * Perform all file scope initialization with the constant
 * prefix. These structures can't be modified by the program.
 */

/**************************************************************
 * Externally defined 
 */

/**************************************************************
 * Internal storage to this module 
 */

/**************************************************************
 * Internal prototypes 
 */
 void chk_ssp_status(void);
 void UintToStr(unsigned int number_in,char *bufp_in);
 void CheckValidToSendNextPkt(void);
 int  CheckForCharOnUart(void);

 /*;*<*>********************************************************
 * uif_upld_initialisation
 * 
 * Initialisation for this file
 **start*/
 void uif_upld_initialisation(Initt_system_state *ssp)
{

 } /*end uif_upld_initialisation*/

 /*;*<*>********************************************************
 * uif_upld
 *
 * This procedure is the normal entry point to this module.
 *
 * It is called from the menuing susbsystem, and has ownership of the
 *  screen while it is active
 *
 *   Data flow
 *  drm->buf_drm        - Dplt_data recorded copied into buffer
 *               ->ssp  - Dplt_data sent to serial module
 **start*/
 void uif_upld(void) {
#define STAT_BUF_SIZE 18
 register int old_result, result;
 char buf_drm[DRMC_MAX_BUFFER_SIZE], 
      stat_buf[STAT_BUF_SIZE];
 
    memset(stat_buf,' ',STAT_BUF_SIZE);

#define UU1 3
#define UU2 5
#define UU3 7
#define UU4 9
#define UU5 11
#define UU_DEBUG 13
#define RECORD_NUM_START 5
#define RECORD_NUM_LENGTH 4

    /* Enable RS232 driver */
    delay_ms(uart_driver(UART1_DRV_ON));


    /* Init text screen
     */
    ldd_clr(LDDE_CLR_TEXT);
    ldd_line(UU1,(uchar *)"   UPLOADING  ");
    ldd_line(UU2,(uchar *)"     RECORD  ");
    ldd_line(UU4,(uchar *)"     OUT OF  ");
    /* Write Record number to text screen */
    result = drm_data(DRMC_CURRENT_TUPL, _ModuleUif_, &buf_drm[0]);
    UintToStr(result,&stat_buf[RECORD_NUM_START]);
    stat_buf[(RECORD_NUM_START+RECORD_NUM_LENGTH+1)] = 0; /* Terminate str*/
    ldd_line(UU5,(uchar *)stat_buf);

    /* Init drm module to start sending data from beginning of buffer */
    drm_data(DRMC_1ST_TUPL, _ModuleUif_, &buf_drm[0]);


    old_result = 0xfffc; /* Init to obscure value - not 0*/
    do {
       result = drm_data(DRMC_FETCH_FRWD_TUPL, _ModuleUif_, &buf_drm[0]);
       if ( result == old_result) {
          /* By comparing records and finding out if they are the same,
           * the assumptions is that when the end-of-buffer is reached, 
           * the record number stays the same.
           */
          break; /* out of loop */
       }
       old_result = result;

       /* check if keypad requests return to display */
       hw_keypad_fsm();
       if ( hw_keypad_poll(HWE_KEY_TIME_END,HWE_KEY_TIME_END) == KEY_DISPLAY)
          break; /* out of loop */

	   CheckValidToSendNextPkt();

       /* Check status of ssp module to check if buffer tx available */
       chk_ssp_status();

       /* Indicate information can be sent on SSP */
       ssp_dl_req(SSPC_TX_DPLT_DATA,&buf_drm[0]);

       /* Write Record number to text screen */
       UintToStr(result,&stat_buf[RECORD_NUM_START]);
       stat_buf[(RECORD_NUM_START+RECORD_NUM_LENGTH+1)] = 0; /* Terminate str*/
       ldd_line(UU3,(uchar *)stat_buf);

    } while (1); /* Exit with break */

    /* Disable RS232 Driver */
    uart_driver(UART1_DRV_OFF);


 } /*end uif_upld*/

/*;*<*>********************************************************
  * chk_ssp_status
  *
  * Checks to see if the Tx buf is available. If not waits a few mS
  * and tries again.
  *
  **start*/
 void chk_ssp_status(void)
 {
 Sspt_manage ssp_status;

    ssp_control(&ssp_status); /* Get status */

    while (ssp_status.tx_buffer_available == TX_BUF_NOT_AVAILABLE) {
       delay_ms(2); /* Delay,  and try again */
       ssp_control(&ssp_status); /* Get status */
    };

 } /*end chk_ssp_status*/


/*;*<*>********************************************************
  * UintToStr
  *
  * Convert integer to decimal ascii
  *
  * The input must be less than 9999 (decimal)
  *
  **start*/
 const hex_to_ascii_table[] = 
   {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
 void UintToStr(unsigned int HexNumber,char *bufp_in)
 {
   register int Number, NumberStart;
   register char *bufp = bufp_in;

#define _NibbleMask_ 0x0f  

#define _Thousands_ 1000
#define _Hundreds_  100
#define _Tens_      10

   /* Get the 1000s */
   Number = HexNumber/_Thousands_;
   *bufp++ = hex_to_ascii_table[(Number & _NibbleMask_)];
	NumberStart = HexNumber - Number*_Thousands_;
	
   /* 100 */
	Number = NumberStart/_Hundreds_;
   *bufp++ = hex_to_ascii_table[(Number & _NibbleMask_)];
	NumberStart -= Number*_Hundreds_;

   /* 10 */
	Number = NumberStart/_Tens_;
   *bufp++ = hex_to_ascii_table[(Number & _NibbleMask_)];
	NumberStart -= Number*_Tens_;
	
   /* 1 */
   *bufp = hex_to_ascii_table[(NumberStart & _NibbleMask_)];

 } /*end IntToDec*/

/*;*<*>********************************************************
  * CheckValidToSendNextPkt
  *
  * Checks to see if the next packet can be sent. This is done by
  *     - Checking if an ACK is received on the serial port
  *     - waiting for a set time
  *
  **start*/
void CheckValidToSendNextPkt(void)
 {
	 int Timer;
#define _AsciiAck_ 0x06
#define _AsciiNak_ 0x14

#define _MinPcDelay_ 110  /* This is the minimum time that must be allowed
                           * between sending packets, so that the PC can
                           * resynhronize a packet frame should there
                           * be a problem.
                           * The PC requires 55 to 110mS between packets
                           * as of Jan/93
						   * As of 2.0.2 3/3/96 5 records a second with the delay
                           */

	 Timer = 0;
	 while ((_AsciiAck_ != CheckForCharOnUart()) && /*Has PC acknowledged*/
			(Timer++ < _MinPcDelay_)) /* Timeout*/
	 {
		 delay_ms(1);/*Insert delay to allow for inter-packet delay */
			 
	 }
	 
	 
 } /* end CheckValidToSendNextPkt */

/*;*<*>********************************************************
  * CheckForCharOnUart
  *
  * If there is a character on the input it returns with the
  * character, else it returns with -1
  *
  * This can be moved to utils.c when working
  *
  **start*/
int CheckForCharOnUart(void)
 {
	 
	 /* No character present on UART */
	 return -1;
	 
	 /* Character present in UART - get it */
	 /*return getchar(); */
	 
 } /*end CheckForCharOnUart*/

/*;*<*>********************************************************
  * xxx
  **start*/
/* xxx()
 {

 } /*end xxx*/

/*;*<*>********************************************************
  * xxx
  **start*/
/* xxx()
 {

 } /*end xxx*/

/*;*<*>********************************************************
  * xxx
  **start*/
/* xxx()
 {

 } /*end xxx*/



