/* dbg_tim.c debug timing module. 
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

 This module provides functions to time pieces of code.

 The functions are called as follows

 dbg_tim_init - initialisation
 dbg_tim  - DBGE_START
            DBGE_STOP
            DBGE_PAUSE_ON
            DBGE_PAUSE_OFF
 dbg_results

 From a client program
    dbg_tim_init
    dbg_tim


 When dbg_tim(DBGE_START) is called, it starts a record of time
 passing. If dbg_tim(DBGE_PAUSE_ON) is called then a temporary suspension of
 timing will result, until dbg_tim(DBGE_PAUSE_OFF) is called when timing will
 start again. 

 Legal calls are

   DBGE_START ------------------------+-----------------+
        \|/                           |                 |
      start timing, zeroing counters \|/               \|/
   DBGE_PAUSE_ON ---------+        DBGE_START         DBGE_PAUSE_OFF
         |                |       restart timing,     no effect
      no timing          \|/      zeroing counters
   DBGE_PAUSE_OFF      DBGE_STOP
         |               End timing and
      timing             calculate results
   DBGE_STOP
      end timing and
      calculate results


 Limitations:
   At present limited to the length of timer1, which is CLOCK*8*0xffff cycles
   This limitation could be changed by having timer1s overflow cause an
   interrupt.

*/
#ifdef IC96
#pragma types
#pragma nodebug
#include <h\cntl196.h>
#pragma nolistinclude
#endif /* IC96 */
#include <80C196.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "general.h"
#include "iir.h"
#include "proto.h"
#include "dbg.h"
#include "kernel.h"
#ifdef SIMULATING
#include "pc_only.h"
#endif

/* If INTS is defined, then each function disables()/enables()
 * interrupts when invoked
 */
/*#define INTS */

/**************************************************************
 * Internal enums/typedefs 
 */

/* Constants */
/* 
 * START_ADJUSTMENT and PAUSE_OFF_ADJUSMENT are the 
 * measured adjustment to the time dues calling 
 *    1) dbg_tim(DBGE_START) followed by dbg_tim(DBGE_STOP)
 *    2) inserting a DBGE_PAUSE_ON + DBGE_PAUSE_OFF
 */
#define START_ADJUSTMENT 0x2d
#define PAUSE_OFF_ADJUSTMENT 0x26

/**************************************************************
 * Shared variables 
 */
 Dbgt_tim_result result;
 Word time_started;

/* Externally defined variables */

/**************************************************************
 * Internal Prototypes
 */
 void start(void);
 void pause_on(void);
 void pause_off(void);
 void stop(void);
 void abrt(void);
 void ignore();

/**************************************************************
 *
 * Internal storage to this module
 *
 */
 /*
  * State table
  */
 const void (*state_table[(int)DBGE_PAUSE_ON+1][(int)DBGE_PAUSE_OFF+1])(void);
/* = { 
   The following initialisation doesn't work in IC96 V2.1
             /* START,       STOP,          PAUSE_ON,        PAUSE_OFF * /
/*start   * /{       &start,       &stop,  &pause_on,       &ignore   },
/*stop    * /{        start,     ignore,      ignore,        ignore   },
/*pause_on* /{        start,       abrt,      ignore,        pause_off}
 };*/

/*;*<*>********************************************************
  * dbg_tim_init
  *
  * Initialises this module
  *
  **start*/
 enum inite_return dbg_tim_init(Initt_system_state *ssp)
 {
 int req_state;
   /*
    * Initialise the state
    */
   result.state = DBGE_STOP;
   result.time = 0x5555;
   /* debug njh */
   result.out_of_range = 0;
   result.ignored = 0;
   /*
    * Initialise the state machine 
    *
    * In IC96 V2.1 I couldn't make it do it automatically
    */
   req_state = 0;
   /* Current state is START */
   state_table[DBGE_START][req_state++] = (void *)start;   /* req START    */
   state_table[DBGE_START][req_state++] = (void *)stop;    /* req STOP     */
   state_table[DBGE_START][req_state++] = (void *)pause_on;/* req PAUSE_ON */
   state_table[DBGE_START][req_state++] = (void *)ignore;  /* req PAUSE_OFF*/

   /* Current state is STOP */
   req_state = 0;
   state_table[DBGE_STOP][req_state++] = (void *)start;   /* req START    */
   state_table[DBGE_STOP][req_state++] = (void *)ignore;  /* req STOP     */
   state_table[DBGE_STOP][req_state++] = (void *)ignore;  /* req PAUSE_ON */
   state_table[DBGE_STOP][req_state++] = (void *)ignore;  /* req PAUSE_OFF*/

   /* Current state is PAUSE_ON */
   req_state = 0;
   state_table[DBGE_PAUSE_ON][req_state++] = (void *)start;  /*req START    */
   state_table[DBGE_PAUSE_ON][req_state++] = (void *)abrt;   /*req STOP     */
   state_table[DBGE_PAUSE_ON][req_state++] = (void *)ignore; /*req PAUSE_ON */
   state_table[DBGE_PAUSE_ON][req_state++]=(void *)pause_off;/*req PAUSE_OFF*/

   return PASS;
 } /*end dbg_if_init*/


/*;*<*>********************************************************
  * dbg_tim
  *
  * This function is the entry point to this module.
  *
  * It drives a state machine, which does the actual work.
  *
  **start*/
 void dbg_tim(enum dbge_tim requested_state)
 {
 void (*comp)(void);
    /*
     * Check boundaries
     */
    if ( ((int)result.state > (int)DBGE_PAUSE_ON)     |
         ((int)requested_state > (int)DBGE_PAUSE_OFF) ) {
       result.out_of_range++;
       return;
    }
    comp = (void *)state_table[result.state][requested_state];
    /* Index into the state table */
    (*comp)();

 } /*end dbg_tim*/

/*;*<*>********************************************************
  * start
  *
  * This function starts the process of timing an event.
  *
  * A copy of timer1 is taken and stored in 'time_started'.
  * 'total_time' is cleared.
  *
  * dbg_state = DBGE_START
  **start*/
 void start()
 {
#ifdef INTS
    disable();
#endif /* INTS */
    time_started = (Word)((Word)timer1 + START_ADJUSTMENT);
    result.time = 0;
    result.number_pauses = 0;
    result.state= DBGE_START;

#ifdef INTS
    enable();
#endif /* INTS */

 } /*end start*/

/*;*<*>********************************************************
  *  pause_on
  *
  * This function pauses the process of timing an event. This is done by 
  * getting timer1 and differencing against 'time_started' and then
  * adding the result to 'total_time'.
  *
  * dbg_state = DBGE_PAUSE_ON
  *
  **start*/
 void pause_on()
 {
#ifdef INTS
   disable();
#endif /* INTS */

   result.time += (Word)((Word)timer1-(Word)time_started);
   result.state = DBGE_PAUSE_ON;
   result.number_pauses++;

#ifdef INTS
   enable();
#endif /* INTS */

 } /*end pause_on*/

/*;*<*>********************************************************
  * pause_off
  *
  * This function starts the process of timing an event, this is
  * done by getting timer1 and storing it in 'time_started'
  *
  * dbg_state = DBGE_START
  **start*/
 void pause_off()
 {
#ifdef INTS
   disable();
#endif /* INTS */

   time_started = timer1+PAUSE_OFF_ADJUSTMENT;
   result.state = DBGE_START;

#ifdef INTS
   enable();
#endif /* INTS */

 } /*end pause_off*/

/*;*<*>********************************************************
  * stop
  *
  * This function stops the process of timing an event. This is done
  * by getting timer1 and differencing against 'time_started' and then adding
  * the result to 'total_time'.
  *
  * dbg_state = DBGE_STOP
  **start*/
 void stop()
 {
 Word timer_value;

#ifdef INTS
   disable();
#endif /* INTS */

   timer_value = (Word)timer1;
   result.time += (Word)(timer_value-time_started);

   result.state = DBGE_STOP;

#ifdef INTS
   enable();
#endif /* INTS */

 } /*end stop*/
/*;*<*>********************************************************
  * abrt
  *
  * This function aborts the process of timing an event.
  *
  * dbg_state = DBGE_STOP
  *
  * Interrupts are not disabled as the routine can be performed
  * in a single transaction
  *
  **start*/
 void abrt()
 {

   result.state = DBGE_STOP;

 } /*end abort*/

/*;*<*>********************************************************
  * ignore
  *
  * This function takes no action
  **start*/
 void ignore()
 {
   result.ignored++;
 } /*end ignore*/

/*;*<*>********************************************************
  * dbg_result
  *
  **start*/
 int dbg_result(Dbgt_tim_result *to_resultp)
 {
 register int *fmp= (int *) &result;
 register int *top = (int *) to_resultp;
 register int loop;

    disable();

    /* Copy the array result to the user */

    /*
    to_resultp->state = result.state;
    to_resultp->time = result.time;
    to_resultp->number_pauses = result.number_pauses;
    */

    for (loop=0;loop < sizeof(Dbgt_tim_result);loop++){
       *top++ = *fmp++;
    }

    enable();

    if (to_resultp->state = DBGE_STOP) {
       return PASS;
    } else {
       return FAIL;
    }
 } /*end dbg_result*/

/*;*<*>********************************************************
  * 
  *
  **start*/
 /*()
 {

 } /*end*/













