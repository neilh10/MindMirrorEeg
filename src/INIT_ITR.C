/* init_itr.c
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
 This modules contains all the interrupt routines.


*/
#ifdef IC96
#pragma extend  /* To allow for asm instructions */
#pragma regconserve
#include <h\ncntl196.h>
/*#pragma debug */
#pragma nolistinclude 
#endif
#include <80C196.h>
#include <stdio.h>
/*#include <stdlib.h>*/

#include "general.h"
#include "iir.h"
#include "init.h"
#include "dbg.h"
#include "kernel.h"
#include "ldd.h"
#include "hw.h"
#include "ssp.h"
#include "mm_coms.h"
#include "dim.h"
#include "proto.h"
#ifdef SIMULATING
/*#include "pc_only.h"*/
#endif

/**************************************************************
 * Interrupts 
 * This is non ANSI INTEL IC-96 compiler directives
 * to set up the interrupts
 */
#ifdef IC96
#pragma interrupt( int_tmr_ovflw  = 0) /* Timer Overflow */
#pragma interrupt( int_ad_complete = 1)/* A/D conversion complete */
#pragma interrupt( int_hsi_avl = 2)    /* High speed input data available */
#pragma interrupt( int_hso = 3)        /* High speed outputs */
#pragma interrupt( int_hsi_o_pin = 4)  /* HSI.O pin */
#pragma interrupt( int_sw_tmr = 5)     /* Software Timer */
#pragma interrupt( int_ser_port= 6)    /* Serial port */
#pragma interrupt( int_extint = 7)     /* EXTINT */
#ifdef REAL_TARGET
#pragma interrupt( int_trap = 8)       /*reserved for RISM- Trap*/
#endif /* REAL_TARGET */
#pragma interrupt( int_undef_opcode =9)/* Undefined opcode*/
#pragma interrupt( int_transmit = 24)  /* Transmit*/
#pragma interrupt( int_receive = 25)   /* Receive*/
#pragma interrupt( int_hsi_fifo_4 = 26)/* HSI FIFO 4th Entry*/
#pragma interrupt( int_tmr2_cap = 27)  /* Timer 2 capture*/
#pragma interrupt( int_tmr2_ovflw = 28)/* Timer 2 overflow*/
#pragma interrupt( int_extint_pin = 29)/* EXTINT pin */
#pragma interrupt( int_hsi_fifo_full=30)/* HSI FIFO full*/
#ifdef REAL_TARGET
#pragma interrupt( int_nmi = 31)        /* reserved for RISM - NMI */
#endif /* REAL_TARGET */
#endif

/*#define RECORD_INT_ACTIVITY /* To record interrupt activity */
/* If defined enables interrupt (except UART) to o/p char*/
/*#define INT_HAPPENED /*puts a 'i'+'num' on uart */
/*#define INT_HAPPENED_UART*//* For UART - could be self referential*/

enum happened {TMR_OVRFLW,AD_COMPLETE,HSI_AVL,HSO,HSI_O_PIN,
               SW_TMR,SER_PORT,EXTINT,TRAP,UNDEF_OPCODE,
               TRANSMIT,RECEIVE,HSI_FIFO_4,TMR2_CAP,TMR2_OVFLW,
               EXTINT_PIN,HSI_FIFO_FULL,NMI};

/**************************************************************
 * Internal enums/typedefs 
 */

/**************************************************************
 *
 * Internal storage to this module 
 *
 */
 /*
  * Structure to record interrupt activity
  */
#ifdef RECORD_INT_ACTIVITY
 Intt_activity intu_activity;
#endif /*RECORD_INT_ACTIVITY*/

 /*
  * This keeps track of the main list
  */
 Longword list_main;

 /* This stores the latest a/d conversions
    To access use
         HW_AD_LB_LOBE 
         HW_AD_RB_LOBE 
	 HW_AD_LB_EMG 
	 HW_AD_RB_EMG 
  */
 Frac ad_result[NUMBER_AD_CHANNELS];

 /* Semaphore to indicate other A/D channels need reading
  * Each channel that maybe read asynchronously is has a
  * bit assigned to it. When this bit is set then it needs reading
  * and the ad_complete routine will read it, clear it and initiate
  * the corresponding A/D when it can.
  */
 enum ade_channel_masks ad_other_channels;

 extern Dimt_data dimu_input_data[];
 extern unsigned int hw_test_freq_flag;
 extern Semaphore do_display;
 extern unsigned char ioport2_mirror, ioport1_mirror;

 void int_happened(enum happened type);
 void init_debug(enum init_debug_type type);


/*;*<*>********************************************************
  *  void init_itr_initialise
  *
  * initialisation for this module
  *
  **start*/
 void init_itr_initialise(void)
 {
 int loop;
#ifdef RECORD_INT_ACTIVITY
    /* Initialise structure initu_activity */
    p = (char *) &intu_activity;
    for (loop = 0; loop < sizeof(intu_activity); loop++) {
        *(p++) = 0;
    }
#endif /*RECORD_INT_ACTIVITY*/

    /* Init ad_other_channels so that
     *    - kicks off a measurement of the battery ASAP
     */
    ad_other_channels = ADC_BAT_FAIL_MASK;
    for (loop = 0; loop < NUMBER_AD_CHANNELS; loop++) {
       ad_result[loop] = 0;
    }
 } /*end init_itr_initialise*/

/*;*<*>********************************************************
  * int_tmr_ovrflw
  **start*/
void int_tmr_ovflw()
 {
#ifdef INT_HAPPENED
 int_happened(TMR_OVRFLW);
#endif /* INT_HAPPENED*/
#ifdef RECORD_INT_ACTIVITY
 intu_activity.tmr_ovflw++; /* Record another interrupt */
#endif
 } /*end int_tmr_ovflw*/

/*;*<*>********************************************************
  * get_ad_result
  *
  * Performs all the collection of A/D data, and the
  * time dependent processing that is needed. This time
  * dependent processing takes up to 50% of the available processor
  * time and is VERY dependent on being completed sucessfully in
  * 8mS
  *
  * However this routine is called 4 mS and must be re-entrant. See
  *  dfm_schedule() for more information.
  *
  * Called by the int_ad_complete() every time there is a 
  * A/D has completed.
  *
  * algorithim
  *    collect A/D
  *    if more A/D then
  *        kick off next A/D and exit
  *    else
  *       check to see if need to generate test frequencies for analog brd
  *       check to see if need to call keypad fsm
  *       perform realtime digital filtering
  **start*/
 void get_ad_result() 
{
 register Wordbyte input;
 register int index,loop;

    input.byte.ah = ad_result_hi;
    input.byte.al = ad_result_lo;

#define AD_CHANNEL_MASK 0x7
#define AD_LO_LSB_MASK 0xC0
#define START_AD_CHANNEL 0x8

    index = input.byte.al & AD_CHANNEL_MASK; /* Get channel completed */
    input.byte.al &= AD_LO_LSB_MASK; /* Mask off all other data */

    ad_result[index] = input.word; /* Store 16 bits in fractional form */

    /*
     * Check to see if all A/D channels converted
     * If not start next channel
     */
    if ( ++index < NUMBER_AD_CONVERTERS) {
       ad_command = START_AD_CHANNEL | index;/*Start another AD conversion*/
    } else if ((int) ad_other_channels ){
       /* Another A/D conversion is neaded
        *  The only other one that can be used at the moment is
        *    - Bat fail
        */
        ad_other_channels = 0; /* &= (~ADC_BAT_FAIL_MASK); /* */
        ad_command = (START_AD_CHANNEL | ADC_BAT_FAIL);
    } else {
       /***********************************************
        * Completed all A/D processing
        ***********************************************
        * Check to see if test processing is needed
        */
        if (hw_test_freq_flag == (int) TEST_FREQ_ENABLE) {
           hw_test_int();
        }
#ifndef FILTER_IN_BACKGROUND
       /***********************************************
        * Execute the filtering routines
        */
       /*if ( (list_main.word.low & 0x1) == 0 )*/ {
	  /* The main function is called every 4ms - 256Hz
	     Only snapshot data every 8ms - 128Hz
	     */
          DimSnapRaw(_DimSnap_); /* SnapShot Raw Data*/
       }
       dim_new_data(); /* Get the data */
       dfm_schedule(); /* do the filtering */


#endif /* FILTER_IN_BACKGROUND */
       /***********************************************
        * Check whether to set background polling going
        *
        * If less than 0.5 sec then 0.5 Hz filter output oscillates
        *       LIST_8MS  LIST_4MS
        *  0xff 2.0 sec    1.0
        *  0x7f 1.0 sec    0.5
        *  0x3f 0.5 sec    0.25  
        *  0x1f 0.25 sec   0.125  - needed for keypad polling
        *  0x0f 0.125 sec  0.0625 - too fast at low freq
        */
       if ( (list_main.word.low & 0x1f) == 0 ) {
          do_display=SET_SEMAPHORE;
	      DimSnapRaw(_DimStart_); /* Called 8 times a second */
       }

    }

 } /* end get_ad_result */

/*;*<*>********************************************************
  * int_ad_complete
  **start*/
 void int_ad_complete()
 {
    /*  PUSHA code inserted by ISR preamble
     *   saves   PSW, INT_MASK, INT_MASK1, WSR 
     */

#ifdef INT_HAPPENED
 int_happened(AD_COMPLETE);
#endif /* INT_HAPPENED*/

    int_mask = AD_ACT_INT_MASK;
/*    asm ei; */
    asm epts;  /* Enable PTS to happen in background */
    get_ad_result();

#ifdef RECORD_INT_ACTIVITY
    intu_activity.ad_complete++; /* Record another interrupt */
#endif

    /* POPA iserted by ISR postcodeing
     * restores PSW, INT_MASK, INT_MASK1, WSR 
     */
 } /*end int_ad_complete*/

/*;*<*>********************************************************
  * int_hsi_avl
  **start*/
 void int_hsi_avl()
 {
#ifdef INT_HAPPENED
 int_happened(HSI_AVL);
#endif /* INT_HAPPENED*/

#ifdef RECORD_INT_ACTIVITY
 intu_activity.hsi_avl++; /* Record another interrupt */
#endif


 } /*end int_hsi_avl*/
/*;*<*>********************************************************
  * int_hso
  **start*/
 void int_hso()
 {
#ifdef INT_HAPPENED
 int_happened(HSO);
#endif /* INT_HAPPENED*/
#ifdef RECORD_INT_ACTIVITY
 intu_activity.hso++; /* Record another interrupt */
#endif


 } /*end int_hso*/

/*;*<*>********************************************************
  * int_hsi_o_pin
  **start*/
 void int_hsi_o_pin()
 {
#ifdef INT_HAPPENED
 int_happened(HSI_O_PIN);
#endif /* INT_HAPPENED*/

#ifdef RECORD_INT_ACTIVITY
 intu_activity.hsi_o_pin++; /* Record another interrupt */
#endif


 } /*end int_hsi_o_pin*/

/*;*<*>********************************************************
  * int_sw_tmr
  *
  * The sw timer is set up to expire every 3.906ms. The main task
  * of this routine is to
  *     start A/D going on channel 0
  *     update system time
  *
  * The A/D must be started right away, and nothing MUST interfere with
  * it being initiated at the same time instant for the digital
  * processing that is dependent on it.
  *
  **start*/
 void int_sw_tmr()
 {
#ifdef INT_HAPPENED
 int_happened(SW_TMR);
#endif /* INT_HAPPENED*/
    /*
     * Start A/D channel 0
     */
#define START_AD_CHANNEL_0 START_AD_CHANNEL
    ad_command = START_AD_CHANNEL_0;

    hso_command = START_SW_TIMER_0;
    hso_time = timer1 + LIST_SW_TIMER;

#define TEST_BIT 0x80
    ioport2_mirror ^= TEST_BIT;
    ioport2 = ioport2_mirror;

#ifdef RECORD_INT_ACTIVITY
    intu_activity.sw_tmr++; /* Record another interrupt */
#endif
    /*
     * Increment the main list timer
     */
    list_main.dword++;

 } /*end int_sw_tmr*/

/*;*<*>********************************************************
  * int_ser_port
  *
  * This isn't used - see seperate Rx and Tx interrupts
  *
  **start*/
 void int_ser_port()
 {
#ifdef INT_HAPPENED
#ifdef INT_HAPPENED_UART
 int_happened(SER_PORT);
#endif /*INT_HAPPENED_UART*/
#endif /* INT_HAPPENED*/
#ifdef RECORD_INT_ACTIVITY
 intu_activity.ser_port++; /* Record another interrupt */
#endif

 } /*end int_ser_port*/

/*;*<*>********************************************************
  * int_extint
  **start*/
 void int_extint()
 {
#ifdef INT_HAPPENED
 int_happened(EXTINT);
#endif /* INT_HAPPENED*/
#ifdef RECORD_INT_ACTIVITY
 intu_activity.extint++; /* Record another interrupt */
#endif


 } /*end int_extint*/

/*;*<*>********************************************************
  * int_trap
  **start*/
 void int_trap()
 {
#ifdef INT_HAPPENED
 int_happened(TRAP);
#endif /* INT_HAPPENED*/

#ifdef RECORD_INT_ACTIVITY
 intu_activity.trap++; /* Record another interrupt */
#endif

 init_debug(TRAP_OCCURED);

 } /*end int_trap*/

/*;*<*>********************************************************
  * int_undef_opcode
  **start*/
 void int_undef_opcode()
 {
#ifdef INT_HAPPENED
 int_happened(UNDEF_OPCODE);
#endif /* INT_HAPPENED*/
#ifdef RECORD_INT_ACTIVITY
 intu_activity.undef_opcode++; /* Record another interrupt */
#endif

 init_debug(UNIMPLEMENTED_OCCURED);

 } /*end int_undef_opcode*/

/*;*<*>********************************************************
  * int_transmit
  **start*/
 void int_transmit()
 {
#ifdef INT_HAPPENED
#ifdef INT_HAPPENED_UART
 int_happened(TRANSMIT);
#endif /*INT_HAPPENED_UART*/
#endif /* INT_HAPPENED*/
#ifdef RECORD_INT_ACTIVITY
    intu_activity.transmit++; /* Record another interrupt */
#endif
    /*
     * Call the interrupt handler
     */
    ssp_end_int();


 } /*end int_transmit*/

/*;*<*>********************************************************
  * int_receive
  **start*/
 void int_receive()
 {
#ifdef INT_HAPPENED
#ifdef INT_HAPPENED_UART
 int_happened(RECEIVE);
#endif /*INT_HAPPENED_UART*/
#endif /* INT_HAPPENED*/
#ifdef RECORD_INT_ACTIVITY
 intu_activity.receive++; /* Record another interrupt */
#endif


 } /*end int_receive*/

/*;*<*>********************************************************
  * int_hsi_fifo_4
  **start*/
 void int_hsi_fifo_4()
 {
#ifdef INT_HAPPENED
 int_happened(HSI_FIFO_4);
#endif /* INT_HAPPENED*/
#ifdef RECORD_INT_ACTIVITY
 intu_activity.hsi_fifo_4++; /* Record another interrupt */
#endif


 } /*end int_hsi_fifo_4*/

/*;*<*>********************************************************
  * int_tmr2_cap
  **start*/
 void int_tmr2_cap()
 {
#ifdef INT_HAPPENED
 int_happened(TMR2_CAP);
#endif /* INT_HAPPENED*/
#ifdef RECORD_INT_ACTIVITY
 intu_activity.tmr2_cap++; /* Record another interrupt */
#endif


 } /*end int_tmr2_cap*/

/*;*<*>********************************************************
  * int_tmr2_ovflw
  **start*/
 void int_tmr2_ovflw()
 {
#ifdef INT_HAPPENED
 int_happened(TMR2_OVFLW);
#endif /* INT_HAPPENED*/

#ifdef RECORD_INT_ACTIVITY
 intu_activity.tmr2_ovflw++; /* Record another interrupt */
#endif


 } /*end int_tmr2_ovflw*/

/*;*<*>********************************************************
  * int_extint_pin
  **start*/
 void int_extint_pin()
 {
#ifdef INT_HAPPENED
 int_happened(EXTINT_PIN);
#endif /* INT_HAPPENED*/

#ifdef RECORD_INT_ACTIVITY
 intu_activity.extint_pin++; /* Record another interrupt */
#endif


 } /*end int_extint_pin*/

/*;*<*>********************************************************
  * int_hsi_fifo_full
  **start*/
 void int_hsi_fifo_full()
 {
#ifdef INT_HAPPENED
 int_happened(HSI_FIFO_FULL);
#endif /* INT_HAPPENED*/
#ifdef RECORD_INT_ACTIVITY
 intu_activity.hsi_fifo_full++; /* Record another interrupt */
#endif


 } /*end int_hsi_fifo_full*/

/*;*<*>********************************************************
  * int_nmi
  **start*/
 void int_nmi()
 {
#ifdef INT_HAPPENED
 int_happened(NMI);
#endif /* INT_HAPPENED*/
#ifdef RECORD_INT_ACTIVITY
 intu_activity.nmi++; /* Record another interrupt */
#endif
 init_debug(NMI_OCCURED);

 } /*end int_nmi*/

/*;*<*>********************************************************
  * int_happened
  *
  * This is executed if compiler flag INT_HAPPENED is defined
  * and interrupt occurs.
  *
  * The following interrupts won't be executed as they
  * will be self referential
  **start*/
#ifdef INT_HAPPENED
 void int_happened(enum happened type)
 {

 putchar('i');
 putchar('0'+type);

 } /*end int_happened*/
#endif /*INT_HAPPENED*/

/*;*<*>********************************************************
  * int_debug
  *
  * intialization subsystem debug routine to catch annoying interrupts
  *
  * This routine never returns
  **start*/
  void init_debug(enum init_debug_type type)
 {
   char buf[30];
     switch(type) {
     case TRAP_OCCURED:
        /*
         * 
         */
        sprintf(buf,"\nTrap Occured\n");
        dbg_putstr(buf);
        break;

     case NMI_OCCURED:
        /*
         * 
         */
        sprintf(buf,"\nNMI Occured\n");
        dbg_putstr(buf);
        break;
     case UNIMPLEMENTED_OCCURED:
        /*
         * 
         */
        sprintf(buf,"\nUnimplemented Op Occured\n");
        dbg_putstr(buf);
        break;
     }

 } /*init_debug end*/

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

/*;*<*>********************************************************
  * 
  **start*/
/* ()
 {

 } /*end*/

