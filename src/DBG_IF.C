/* dbg_if.c 
 * 
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
 * 
 * debug interface module. This includes the functions need
 to drive the internal UART.

 The functions are called as follows

 dbg_if_init - initialisation
 dbg_putstr  - non-interrupt driven string print
 dbg_wr      - target/simulator simple string print
 dbg_output - to register an outgoing string.
 dbg_tx     - interrupt service routine

 From a client program
    dbg_output
       ldd_putchar - not implemented yet
       out_serial_port
          strncpy
          kn_qpost
          tx_start_req
             tx_nxt_buf
                tx_chars
                   putchar
                tx_nxt_buf
                   :::


 From an interrupt
    dbg_tx
      kn_rblock
      tx_start_req
         tx_nxt_buf
            tx_chars
               putchar
            tx_nxt_buf
               :::

*/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
#include <80C196.h>
#include <stdio.h>
#include <string.h>
/*#include <stdlib.h>*/

#include "general.h"
#include "iir.h"
#include "mm_coms.h"
#include "dim.h"
#include "proto.h"
#include "dbg.h"
#include "kernel.h"
#ifdef SIMULATING
#include "pc_only.h"
#endif

/**************************************************************
 * Internal enums/typedefs 
 */

/* Constants */
/**************************************************************
 * Shared variables 
 */
/* 
 * Local transmit buffer to this module
 */
#define NUM_BUFFERS 8
 static Byte tx_buf[DBGC_BUFFER_SIZE*NUM_BUFFERS];

/*#define MAC_CONSOLE_STRING 80
#define MAX_STR_BUF 4
*/
#define ASCII_NUMBER_OFFSET 0x30
/* 
 * str_msg_num  - Message number printed at the begining of each message.
 * tx_transmitterp - pointer to next character to be transmitted.
 */
 static int str_msg_num,missed_buffer;
 Byte *tx_transmitterp;


/* Externally defined variables */

/**************************************************************
 *
 * Internal storage to this module
 *
 */
 enum inite_console_type console_type;

 /*
  * This is the status of the debug transmit port
  *  TX_STATUS_OFF  set by dbg_tx(), tx_nxt_buf(), dbg_if_init()
  *  TX_STATUS_TRANSMITTING   set by tx_start_req()
  */
 enum enum_tx_status tx_status;

/**************************************************************
 * Internal Prototypes
 */
 void out_serial_port(const char *str);
 void tx_start_req(void);
 void tx_nxt_buf(void);
 void send_char(char *p);
 int tx_chars(void);

/*;*<*>********************************************************
  * dbg_if_init
  *
  * Initialises this module
  *
  **start*/
 enum inite_return dbg_if_init(Initt_system_state *ssp)
 {
   console_type = ssp->console_type;

   /*
    * Init buffers
    */
   str_msg_num = -1;
   missed_buffer=FALSE;
   tx_status = TX_STATUS_OFF;
   tx_transmitterp = NULL;

    /* Define the parameters to manage the partition */
    kn_pcreate(DBGC_TX_BLK,     /* ID of the partition              */
               DBGC_BUFFER_SIZE,/* Size of blocks in partition      */
               NUM_BUFFERS,     /* Number of blocks in partition    */
               tx_buf);         /* This routine allocates partition */

    /* Define a queue to be managed                        */
    kn_qcreate(DBGC_TX_QUEUE,/* Queue ID                    */
               NUM_BUFFERS); /* Number of elements in queue */

   /*
    * Initialise putchar()
    * There is a race hazard here. If this is enabled too early then
    * when an interrupt occurs it is later squashed just before 
    * interrupts are enabled
    */
   init_putchar();

   return PASS;
 } /*end dbg_if_init*/


/*;*<*>********************************************************
  * dbg_wr
  *
  * This function provides one line that can be printed to either
  * the PC screen or the dbg port on the target system
  *
  **start*/
 void dbg_wr(char string[])
 {
#ifdef SIMULATING
    printf("%s",string);
#endif
#ifdef IC96
    dbg_output((const char *)&string);
#endif
 } /*end dbg_wr*/

/*;*<*>********************************************************
  * dbg_output
  *
  * This functions outputs the input 'str' to any available
  * consoles
  *
  * console_type controls the number of consoles available
  * 
  * The consoles available are
  *    serial channel
  *    LCD screen
  *
  **start*/
 void dbg_output(const char *str)
 {
 char *p = (char *) str;
 int lp;
 size_t length;

    if (console_type & SERIAL_CONSOLE) {
       /*
        * Send string to the serial port for processing
        */
       out_serial_port(str);
    }

    if (console_type & LCD_CONSOLE) {
       length = strlen(str);
       for (lp = 0; lp < length; lp++,p++) {
         /*
          * Send character to LCD
          */
          /* ldd_putchar(*p); */
       }
    }
 } /*end dbg_output*/

/*;*<*>********************************************************
  * dbg_tx
  * 
  * This fn() is called when
  *    the UART has finished transmitting a character
  * 
  * This fn()s state is described by tx_status
  *    TX_STATUS_OFF
  *    TX_STATUS_TRANSMITTING
  *
  * NOTE: 
  *   This routine is 
  *      is called from interrupt level
  *      assumes global interrupts are enabled
  *
  * Algorithm
  *
  * If tx_status == TX_STATUS_OFF
  *    then return with no action taken
  *
  * If tx_status == TX_STATUS_TRANSMITTING then tx_transmitterp points
  *    to the next character to be sent. This is then sent unless it
  *    is a terminating \0. If it is a terminating \0
  *        then tx_transmitterp is returned to the pool TX_BUFFER
  *           and the next message is started.
  *
  **start*/
 void dbg_tx()
 {
    if (tx_status == TX_STATUS_OFF )
        return;

    if (tx_status == TX_STATUS_TRANSMITTING ) {
       if( !(tx_chars()) ) {
          /* tx_chars() has returned a false 
           * - a terminating zero was found 
           */
          kn_rblock(DBGC_TX_BLK,tx_transmitterp);
          disable();
          tx_nxt_buf();
          enable();
       }
    }

 } /*end dbg_tx*/

/*;*<*>********************************************************
  * out_serial_port
  *
  * This function takes the supplied "str" and queues it up for
  * transmitting on the serial port. This is done as follows:
  * 
  * 1) The str_msg_num is incremented to indicate an attempt at 
  *    sending a message. A buffer is obtained from the TX_BUFFER pool.
  *    If no buffers are available then the incoming message is discarded.
  * 2) If a buffer is received then the str_msg_num is added to the front
  *    of the buffer.
  * 3) The 'str' is copied into the buffer
  * 4) The buffer is posted to the TX_QUEUE
  * 5) The tx_start_req function is called, to ensure that the transmitter
  *    section is active.
  *
  * Assumptions:
  *    "str" is not longer then MAX_CONSOLE_STRING including \0
  *    "str" is NULL terminated
  *    This fn() is reentrant
  *    This fn() owns the to_p string between the 
  *       kn_gblock() and kn_qpost() function calls. No other
  *       fn() will access the string in this period.
  **start*/
 void out_serial_port(const char *str)
 {
 Byte *to_p;
 Knt_type err;
    /*
     * Increment to leave trail of attempt to send message
     */
    if (++str_msg_num > 9)
       str_msg_num=0;
    if ( (to_p = kn_gblock(DBGC_TX_BLK,(Knt_type *) &err)) != NULL) {
       if (missed_buffer == TRUE) {
          *to_p = '+';
          missed_buffer=FALSE;
       } else {
          *to_p = ASCII_NUMBER_OFFSET + str_msg_num;
       }
       ++to_p;
       strcpy((char *)to_p, str);
       --to_p;
       /*
        * Send the message to the transmitter section.
        * If the transmitter routine is turned OFF, then
        * nudge it to get it going.
        */
       kn_qpost(DBGC_TX_QUEUE,to_p);
       tx_start_req();
    } else {
       missed_buffer = TRUE;
    }

 } /*end out_serial_port*/

/*;*<*>********************************************************
  * tx_start_req
  *
  * This function is called to ensure that the transmitter
  * section is active.
  *
  * Note that this routine must be reentrant. It is conceivable that
  *   a background routine will attempt to send a message and
  *   while this is in progress an interrupt performs the same request. 
  *   In this case the interrupt routine will issue a tx_start_req() before
  *   the background routine issues a tx_start_req().
  * 
  **start*/
 void tx_start_req()
 {
    disable();
    if ( tx_status == TX_STATUS_OFF) {
       /*
        * The transmitter is turned off
        */
       tx_status = TX_STATUS_TRANSMITTING;
       tx_nxt_buf(); /* Start transmission */
    }
    enable();
 } /*end tx_start_req*/

/*;*<*>********************************************************
  * tx_nxt_buf
  *
  * This fn() attempts to accept a message from TX_QUEUE and start sending
  * it.
  *
  * If message present on TX_QUEUE
  *    set up pointers
  *    start sending the message
  * else 
  *    set tx_status = TX_STATUS_OFF
  * endif
  *
  * Note: This routine assumes it is not reentrant, however it
  *       does call itself recursively.
  *
  **start*/
 void tx_nxt_buf()
 {
 register Byte *p;
 Knt_type err;

    p = kn_qaccept(DBGC_TX_QUEUE,(Knt_type *) &err);
    if ( p != NULL) {
       tx_transmitterp = p;
       if ( !tx_chars() )
          tx_nxt_buf(); /* Recursive */
    } else {
       tx_status = TX_STATUS_OFF;
    }
 } /*end tx_nxt_buf*/


/*;*<*>********************************************************
  * tx_chars
  * 
  * This routine will transmit a character(s) pointed at by the
  * *tx_transmitterp. It will translate any \n to \r\n characters
  * otherwise characters are not changed. If it sees a zero it will
  * exit with a fail otherwise it passes
  *
  * Future options: Stuff as many characters in the UART as possible
  **start*/
 int tx_chars()
 {
 register char  var;

    var = *tx_transmitterp;

    if (var == 0)
       return FALSE;

    tx_transmitterp++;

    if (var == '\n') /* Check for new line */
       putchar('\r'); /* Output <CR> as well */

    putchar(var);
    return TRUE;

 } /*end send_char*/


/*;*<*>********************************************************
  * 
  **start*/
/* ()
 {

 } /*end*/













