/* init_int.c
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
 system initialisation module

*/
/*#define DEBUG_FUNCTIONS_ENABLED /**/
/*#define DEBUG1_FUNCTIONS_ENABLED /**/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
#include <80C196.h>
#include <stdio.h>

#define DISABLE_ENABLE
#include "general.h"
#include "hw.h"
#include "hw_mem.h"
#include "iir.h"
#include "init.h"
#include "dbg.h"
#include "kernel.h"
#include "ldd.h"
#define LOGO_ADDRESS INIT_1_GRPH_HOME_ADDRESS
#include "logo.h"
#include "mmstatus.h"
#include "mm_coms.h"
#include "mfd.h"
#include "ssp.h"
#include "dim.h"
#include "proto.h"

/* Constants */
 const char logo_status_line[] = " MindMirror III ";


/* Shared variables */
#define INTERNAL_TOP_OF_STACK 0x1fe /* or 0x1ff */
#define EVAL_TOP_OF_STACK 0x7ff0
 int sp_addr;
#pragma locate(sp_addr=0x18)


/* Externally defined */

/* Internal storage to this module */
 Initt_system_state initu_system_state;

/* Internal prototypes */
  void init_initialisation(int test_state);
  void init_uart(void);
  void test_ok(int test);

/*;*<*>********************************************************
  * initialisation
  * 
  * This is the entry point from the assembler/startup routines
  *
  * This function prepares the machine for startup:
  * IMPORTANT: The stack is set to internal for the external memory test.
  *    Beware of using routines until stack is set to external ram.
  *
  *   1) Setup stack for internal use
  *   2) Start UART for checking
  *   3) Test external memory
  *   4) Set stack into external memory
  *   5) Initialise rest of subsystems
  *   6) Start software timer
  *   7) Enable interrupts
  *   8) Start main tasking and never return
  **start*/
 void initialisation()
 {
     /* NOTE:
      * The stack is internal and will be changed to external
      * after the external memory has been verified
      */
     register int temp,test_state; /* Must be register because
                                    * stack will change*/

     /*
      * Set stack pointer up for internal use
      */
     sp_addr = INTERNAL_TOP_OF_STACK;

     /*
      * Turn on UART - but only during init
      * Add some delay as takes time for RS232 chip voltages to get up
      */
     delay_ms(uart_driver(UART1_DRV_ON));
     init_uart();
     test_ok('?');  /* Let world know can do something */
     test_ok('\n'); /* Do something slightly prettier */
     test_ok('\r');
     test_ok('?');

     test_state = '1';
     test_ok(test_state);
     while ((int)test_memory(EXT_NON_BANKED_RAM) == (int) FAIL) {
        test_ok(test_state);
        delay_ms(100);
     };

     /*
      * External non-banked RAM works - so set stack into it
      */
     sp_addr = ( (UPPER_NB_RAM_ADDR+2) & 0xfffe);

     init_initialisation(test_state);


/* while (1) {
*	shift_test();
* }
*/

     disable();
     dbg_putstr("\n\rSystem up\n\r");
     disable(); /* Interrupts causing a bug for some reason */

     /*
      * Start S/W Timer
      */
     hso_command = START_SW_TIMER_0;
     hso_time = timer1 + LIST_SW_TIMER_STARTUP;


     /*
      * Clear all pending interrupts
      * 16 Bit embedded controller guide pg 5-30, sect 5.1 suggests
      * the best way of doing this is
      *     ANDB INT_PEND,#0
      * however doing it from 'c' in the following manner
      */
     int_mask = (0x22&INIT_INT_MASK);
     int_pending &= 0;

     enable(); /* Enable interrupts in system  */
     enable_pts();


/*while(1) {delay_ms(500);putchar('x');};*/

#ifdef DEBUG_FUNCTIONS_ENABLED
	 /* Diable following to turn off data flow out */
     /* ssp_action(SSP_TX_RT_DIS);/**/
#else /* DEBUG_FUNCTIONS_ENABLED */
     uart_driver(UART1_DRV_OFF);
#endif

     /*********************
      * Leave init        *
      *********************/
     scheduler();

   /* Should never return - but if does
    * Loop forever
    */
   delay(uart_driver(UART1_DRV_ON));
   printf("\n\r Scheduler returned\n\r");
   do { 
     /*
      * Flash bit p1.0
      */
     ioport1 |= PORT1BIT0; 
     ioport1 &= ~PORT1BIT0;
   } while (1);

 } /*end initialisation*/

 /*;*<*>********************************************************
 * init_initialisation
 *
 * This module controls the general system initialisation.
 **start*/
 void init_initialisation(int test_state)
 {
 Initt_system_state *ssp = &initu_system_state;
 int loop;

     disable();
     ssp->init_type = COLD;

     /* Put logo up 
      *   - Init LCD Screen
      *   - set contrast
      *   - write bit map
      */
     test_ok(++test_state);
#define INIT_LCD_CONTRAST 0x180
     hw_lcd_contrast_set(INIT_LCD_CONTRAST);

     while (ldd_initialisation(ssp) == FAIL) {
         test_ok(test_state);
         delay_ms(100);
     };
     ldd_status((char *) &logo_status_line[0]);
     ldd_logo((Logo *) &logo_struct);

    /*
     * Perform initialisation of all modules
     */
     test_ok(++test_state);
     while ((int)test_memory(EXT_BANKED_RAM) == (int) FAIL) {
        test_ok(test_state);
        delay_ms(100);
     };

    test_ok(++test_state);
    hw_init(ssp);

    test_ok(++test_state);
    delay_ms(2);  /* Wait for Uart to send character */
    sch_initialise();

    test_ok(++test_state);
    init_itr_initialise();
 
    drm_initialisation(ssp);
    ssp_initialisation(ssp);
    dim_initialisation(ssp);
    dfm_initialisation(ssp);
    mfd_initialisation(ssp);
    uif_initialisation(ssp);/*Put before dpl_ini..() sometime */ 
    dpl_initialisation(ssp);/*Requires some init from uif to work properly*/
    test_ok(++test_state);
    /*
     * Initialise Front End Settings
     */
    mfd_control(MFDC_MONTAGE,MMS_MONTAGE_LATERAL);
    hw_front_end_mode(HW_FE_LATERAL_IN, HWE_ATT_BOTH, HWE_REFRESH);


 } /*end init_initialisation*/

/*;*<*>********************************************************
  * hold_it
  *
  * Function to stop normal processing and send the lower case 
  * alphabet to the serial port.
  **start*/
  void hold_it() 
{
   register int temp;

   disable(); 

   temp = 'z';
   do { 
     /*
      * Flash bit p1.0
      */
     delay_ms(10);
     /*ioport1 |= PORT1BIT0;*/

     delay_ms(10);
     /*ioport1 &= ~PORT1BIT0;*/

     if (++temp > 'z') {
       putchar('\r');
       putchar('\n');
       temp = 'a';
     }
     putchar(temp);

   } while (1);
 } /*end hold_it*/

 /*;*<*>********************************************************
 * init_uart
 * 
 * Initialises the UART
 *
 * Note: This is called from initialisatoin with the stack set 
 * to internal ram.
 **start*/
void init_uart(void)
 {
     /*
      * Initialise serial port
      */
     sp_con    = SP_CON_INITIALISE;

     /* Stuff two bytes into baud_rate
      *       LSB first  (pg 5-54)
      *  then MSB 
      */
     baud_rate = 0xff & BAUD_REG_115200;
     baud_rate = 0xff & (BAUD_REG_115200 >> 8);

     ioc1      = ICON1_INITIALISE;

     /*
      * Initialise putchar()
      * There is a race hazard here. If this is enabled too early then
      * when an interrupt occurs it is later squashed before 
      * interrupts are enabled. Hence this may need to be initialised again.
      */
     init_putchar();

 } /*end init_uart*/

 /*;*<*>********************************************************
 * test_ok
 *
 * Note: This routine is called from initialisation with the stack set
 *    to internal RAM. Beware!
 **start*/
 void test_ok(int test)
 {

#ifdef DEBUG1_FUNCTIONS_ENABLED
   ldd_wrchar(0,TXT_LN_STATUS,(test-ASCII_OFFSET));
#endif
   putchar(test);
   disable();

 } /*end test_ok*/
 /*;*<*>********************************************************
 * 
 **start*/
/* ()
 {

 } /*end*/





