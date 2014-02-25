/* sch.c
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
 This modules is the scheduler module.
 It is responsibile for invoking all the modules in the
 correct order.

*/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
#include <80C196.h>
#include <stdio.h>
/*#include <stdlib.h>*/
#define DISABLE_ENABLE
#include "general.h"
#include "sch.h"
#include "hw.h"
#include "iir.h"
#include "dpl.h"
#include "init.h"
#include "dbg.h"
#include "kernel.h"
#include "ldd.h"
#include "uif.h"
#include "ssp.h"
#include "util.h"
#include "mm_coms.h"
#include "dim.h"
#include "proto.h"
#ifdef SIMULATING
/*#include "pc_only.h"*/
#endif

/**************************************************************
 * Internal enums/typedefs 
 */

/* Constants */
#define FILTER_UPDATE_NUMBER_PASSES 4
#define FILTER_UPDATE_TABLE_SIZE 8
 const char filter_update_table[FILTER_UPDATE_TABLE_SIZE] = {
 /* This table is very important. It is what the user sees as the
  * snappy-ness of the display, and it controls the rate data is retrieved
  * from the filter outputs. If the data is taken faster then the period
  * of the lowest frequency of a particular filter, then the output will
  * follow the amplitude of any detected outputs
  *                                  (Called every 0.5sec)
  *                                   0.25  0.5  1  2sec update
  */
 (NUMBER_FILTERS-6),/* 1st pass              X                   */
 (NUMBER_FILTERS-3),/* 2nd pass              X   X               */
 (NUMBER_FILTERS-6),/* 3rd pass              X                   */
 (NUMBER_FILTERS),  /* 4th pass              X   X   X           */

 (NUMBER_FILTERS-6),/* 5th pass        X                         *!/
 (NUMBER_FILTERS-3),/* 6th pass        X     X                   *!/
 (NUMBER_FILTERS-6),/* 7th pass        X                         *!/
 (NUMBER_FILTERS-1) /* 8th pass        X     X   X               */
 };
/**************************************************************
 * Shared variables 
 */
 int old,activity,direc;
 char SchTxData;/* Data rate on serial output
     //!0 if transmit at 8x/sec otherwise 0 for 2x/sec
     // Initialize to !0*/
 
/* Externally defined variables */

 extern Intt_activity intu_activity;
/* extern Dimt_data dimu_input_data[];*/
 extern Dfmt_data dfmu_data;
 extern Mfdt_data filter_data;
 extern const enum target_type target;

 extern unsigned char ioport1_mirror; /*debug only*/

 /*
  * This keeps track of the main list
  */
 extern Longword list_main;
#ifdef OUTPUT_CAPTURE
/*  extern Frac last_output[MAX_NUMBER_LOBES][NUMBER_FILTERS+NUMBER_LP_FILTERS];*/
#endif /* OUTPUT_CAPTURE */

 /*
  * This stores the latest a/d conversions
  *
  */
 extern Frac ad_result[NUMBER_AD_CONVERTERS];

/**************************************************************
 *
 * Internal storage to this module (initialisation)
 *
 */
#ifdef SAMPLE_INPUT
#define MAX_SAMPLES 4 /* Must be 4,8,16 ... */
 const int sample[MAX_SAMPLES] =  {
    0,0x4000,0x0000,0xc000
   };
 /* 
    {0,0x4000,0x2000,0x4000,
     0,0xc000,0xe000,0xc000} o/p is 0x383  or  899dec
    {0,0x4000,0,0xC000}      o/p is 0x1352 or 4946dec
    {0,0x2000,0,0xE000}      o/p is 0x9aa  or 2474dec 
 */
#endif /* SAMPLE_INPUT */
 int sample_no;

 Semaphore do_display;
 Semaphore SchCnt0_5sema;

/**************************************************************
 * Shared Storage
 */

/**************************************************************
 * Internal Prototypes
 */
/* void flash_led(enum led_flash); */

/*;*<*>********************************************************
  * scheduler
  *
  * This  function is the executive/scheduler for the running
  * system. It operates on a "run to completition" and 
  * cooperative multitasking basis.
  *
  * That is 
  *     - each function when called, gets control of the cpu.
  *     - each function must be designed so as to consider the other
  *       functions the scheduler calls.
  *
  * There are two ways tasks may be called
  *     SCHE_FAST    for processing every 125mS
  *     SCHE_BACKGND for mopping up background time
  *
  * The fast scheduled functions are
  *     (these are time critical routines)
  *     - snapshot and translate dfmu_data[] 
  *       (filtered maximum outputs) at interrupt level 
  *       to filter_data[] for use by background routines.
  *
  *     - user interface 
  *     - transmit filter_data[] on the fiber port routines
  *     - store filter_data[] in the internal ram routines
  *     - inform LCD routines of new data
  *
  * The background scheduled functions are
  *     - display filter_data[] on the LCD routines 
  *     - poll RS232 serial port routine
  *
  *
  * Note1: The mfd_acquire() routine plays a critical role. It acquires
  *    filtered data from the interrupt level. It thus guarentess that
  *    the filtered data will not change when routines are processing
  *    the filtered output. However all routines MUST assume that the
  *    data can change between calls from the scheduler.
  *
  * Note2: The RS232 interface and fiber interface use the same port.
  *    The information transmitted on the RS-232 interface will appear
  *    on the fiber interface. At all other times the RS-232 interface
  *    is not enabled.
  *
  **start*/
 void  scheduler(void)
 {
 long time;
 int filter_update_number=0; /* Used to maintain the filter updates */
 unsigned long int sch_cnt=0; /* Increments every time the scheduler runs */
/* unsigned char buf[DBGC_BUFFER_SIZE] = 
    {0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5};
*/
  SchTxData=0;
  
/*
 * Defines for use with sch_cnt
 * Assumes it gets updated every 1/8 second
 */
#define SCH_CNT_0_125 0x00   /*  every 1/8 second */
#define SCH_CNT_0_25  0x01   /*  every 1/4 second */
#define SCH_CNT_0_5   0x03   /*  every 1/2 second */
#define SCH_CNT_1     0x07   /*  every second */
#define SCH_CNT_2     0x0f   /*  every 2 seconds */


    /* njh - test section
     */
   /* To turn on RS232 interface- see DEBUG_FUNCTIONS_ENABLED */
   /* init_test_bram_cooperatively(); /* Temp */
   /*ssp_action(SSP_TX_RT_DIS); /* Temp */


   /* Delay to allow interrupt drive filtering to settle down */
   delay_ms(500);

   /*
    * Clear the screen ready for spectograph
    */
   ldd_clr(LDDE_CLR_GRAPHICS);
   /*
    * Acquire data to clear any initial settings out of the
    * filtered data
    */
   mfd_acquire(NUMBER_FILTERS); /* Do any update on data */
   dpl_display(SCHE_FAST);

   /*
    * Loop forever 
    */
   while(1) {
      /*
       * Main scheduler loop.
       *
       * Check do_display for fast list 
       *       otherwise execute backgroud list
       *
       * Expect a message every 1/8 second from interrupt level
       */
      if (do_display == SET_SEMAPHORE) {
         do_display = CLR_SEMAPHORE;
         /*
          * The following section is for routines that must run every
          * time do_display is set.
          *
          * Note: The actual update of display - which the user sees
          * is performed in the background section. There
          * must be oddles of time for the background section to run.
          */
         ++sch_cnt;
         time =  (list_main.dword >> 7);

         /*
          * Check user input - perform check before screen
          * processing is performed
          */
         if (!(sch_cnt & SCH_CNT_0_125)) {
            uif_poll(SCHE_FAST);
         }

         /*
          * Get output of filters
          *   0.75Hz(1.3Sec) every 2 sec  (pass number 4)
          *   1.5   (660mS)  every 1 sec  (pass number 1 and 5)
          *   3     (333mS)  every 0.5 sec
          *   4.5   (220mS)        0.5
          *   6     (166mS)        0.25
          *  7.5    (133mS)        0.25
          *   9     (111mS)        0.25
          * 10.5    ( 95mS)        0.125
          * 12.5                   0.125
          * 15, 19, 24, 30, 38     0.125
          */
         SchCnt0_5sema = !(sch_cnt & SCH_CNT_0_5);
         if (SchCnt0_5sema) {
             /*
              * Snap shot the filters when required
              */
             mfd_acquire(filter_update_table[filter_update_number]);
             if (++filter_update_number >= FILTER_UPDATE_NUMBER_PASSES) {
                 filter_update_number = 0;
                 }
             }

         if ((SchTxData) | SchCnt0_5sema) {
            /*
             * Call the fiber section to transmit the latest data
             */
            ssp_action(SSP_TX_RT);
         }

         if (SchCnt0_5sema) {
            /*
             * Call the stored data routine to save the latest data
             */
            drm_scheduler(SCHE_FAST);
         }
         
         if ((SchTxData) | !(sch_cnt & SCH_CNT_0_5)) {
            /*
             * Inform LCD routines that new data is available
             */
            dpl_display(SCHE_FAST);

         }
         if (!(sch_cnt & SCH_CNT_2)) {
            /* Misc actions - done every two seconds
             */
            uif_misc();
         }

      } else {
         /*****************************************************************
          * This section is for co-operative tasks that use more time
          * than can be supplied between settings of do_display semaphore.
          *
          * The cooperative tasks must give up execution periodically
          * for the do_display semaphore to be checked
          *****************************************************************
          */

         /*
          * Perform co-operative multitasking by informing
          * dpl_display() that it can process old data
          * updates
          */
         dpl_display(SCHE_BACKGND);

         /*
          * Transmit stored data
          *
          */
         /*ram_sch(SCHE_BACKGND) */

         /* njh test */
         /*test_bram_cooperatively(); /* */

      }
   };
         /*  This code fragment is never compiled or executed
          * but is left here to show how to use
          * debugging functions
          *
          * Note: sprintf turns off interrupts, which can
          * impact when int_sw_tmr() is invoked and then where
          * the A/D sample is performed.
          *
          * Send to LCD 
          *!/

         sprintf(buf,"%5d %4X %4X",(int)time,ad_result[0],
                 ad_result[1]);
         ldd_status(buf);
        */
 } /*end scheduler*/

/*;*<*>********************************************************
  * flash_led
  * 
  * This routine sets the external LED flashing.
  *
  * For timer 1, the period is a constant
  *    65mS at 16MHz
  *
  * freq time1   time2          OFF/ON
  *  16  0x1000  0x9000         32.5/32.5
  *  16  0x1000  0x2000         61  / 4
  *
  **start*/
/* void flash_led(enum led_flash flash_rate)
 {
 int time1, time2;

    time1 = 0x1000;
    switch(flash_rate) {
    case SYS_AWAKE:
        time2 = 0x2000;
        break;
    case SYS_GETTING_UP:
        time2 = 0x9000;
        break;
    default:
        time2= 0x0e000;
        break;
    }

    ioc2 = FLUSH_CAM;
    ioc2 = EN_CAM_LOCK;

    hso_command = LED_ON;
    hso_time = time1;

    /!* delay *!/
    delay(2);

    hso_command = LED_OFF;
    hso_time = time2;

    return;
 } /* flash_led end*/

/*;*<*>********************************************************
  * cnvt_frac
  *
  * This procedure changes a 16bit integer to a binary fraction
  *
  * Binary fractions have a range $7FFF (=1-2**15) to 0x8000(=-1)
  *
  * Some examples are
  * eg   +   Num               -
  *  4000   0.5               C000
  *  2000   0.25              E000
  *  1000   0.125             F000
  *  0800   0.0625            F800
  *  0400   0.03125           FC00
  *  0200   0.015625          FE00
  *  0100   0.0078125         FF00
  *  0080   0.00390625        FF80
  *  0040   0.001953125       FFC0
  *  0020   0.0009765625      FFE0
  *  0010   0.00048828125     FFF0
  *  0008   0.000244140625    FFF8
  *  0004   0.0001220703125   FFFC
  *  0002   0.00006103515625  FFFE
  *  0001   0.000030517578125 FFFF
  *
  * to make a +ve number negative, complement and inc
  * to make a -ve number postive, complement and inc
  *
  * The analog to digital converter runs from 0 to 0xffff
  *
  * Hence for
  *  ad(f) <0x8000                 ad(f) >= 0x8000
  *    output = 0x7fff-ad(f)        output = 0xffff-(ad(f)&0x7fff)
  *
  * Input: An integer in range 0 to 0xffff
  * Output: A fraction range 0x7fff to 0x8000
  **start*/
 Frac init_cnvt_frac(int in)
 {
#ifdef SIMULATING
   /* Simulator provides the data in fractional form */
   return(in);
#else
#ifdef SAMPLE_INPUT
   return(in);
#else
   if (in < 0x8000) {
      return (0x7fff-in);
   } else {
      return (0xffff-(in&0x7fff));
   }
#endif /* SAMPLE_INPUT */
#endif /* SIMULATING */
 } /*end init_cnvt_frac*/

/*;*<*>********************************************************
  * sch_initialise
  * 
  * Initialization module
  *    Interrupt enables
  *    kernel block and queues
  **start*/
 void sch_initialise(void)
 {
    int loop;
    char *p;
    Initt_system_state ssp;

    /*
     * Interrupt enabling
     *
     * This should be performed before any other routines
     * (like UARTs) start their initialisation, as it does
     * squash any pending interrupts
     */
    int_mask = INIT_INT_MASK;
    int_pending &= 0; /* Clear any pending interrupts pg 5-30 */

    imask1 = INIT_INT_MASK_1;
    ipend1 &= 0; /* Clear any pending interrupts */

    list_main.dword = 0; /* Init main list */

    /* initialise ad_result[] */
    for (loop = 0; loop < NUMBER_AD_CONVERTERS; loop++)
       ad_result[loop] = loop+0x80;

    /*
     * Initialise management of partitions
     */
    kn_blk_init(KNC_NUM_PARTITIONS);

    /*
     * Initialise management of queues
     */
    kn_que_init(KNC_NUM_QUEUES);

    ssp.console_type = SERIAL_CONSOLE;
    dbg_if_init(&ssp);
    dbg_tim_init(&ssp);
    /*
     * Init queue for this module
     */
#define NUM_QUEUE 2
    kn_qcreate(DBGC_SEC_QUEUE,/* Queue ID                    */
               NUM_QUEUE);    /* Number of elements in queue */

    /*
     * Init queue for tracking inputs to dfm subsystem
     */
#define DFM_INPUT_QUE_BUFFERS 64
    kn_qcreate(DBGC_INPUT_QUEUE,      /* Queue ID                */
               DFM_INPUT_QUE_BUFFERS);/* Number of elements in queue */

  /* Init what IC96 won't do */
   old = activity = direc = 0;
   sample_no = 0;

 } /*end sch_initialise*/


/*;*<*>********************************************************
  * 
  **start*/
/* ()
 {

 } / *end*/
