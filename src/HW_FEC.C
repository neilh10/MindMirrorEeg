/*  hw_fec.c - Front End Control
 
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
This module is responsible for controlling the analog front end. At
present this consists of
     attenuator and 
     mode settings
 on the analog board. B002 Rev 2


The following object like interfaces are defined for
the Mind Mirror III analog board:
    hw_fec_init     - called once
    **hw_attenuator   - to change attentuator settings**
    ***old call hw_front_end_mode()                   **
    hw_front_end_mode - to change input to front end 
    hw_att_value - to retrieve front end settings

Internally used functions for the above are
    hw_analog_set(REGISTER,value)                   partially TESTED rev1
    NOT IMPLEMENTED hw_analog_status(REGISTER,value)

where REGISTER is 

       AG_LEFT_CTL
       AG_RIGHT_CTL
       AG_TEST
       AG_LEFT_DC_ADJ1
       AG_LEFT_DC_ADJ2
       AG_LEFT_AD_ADJ
       AG_LEFT_GAIN_ADJ
       AG_RIGHT_DC_ADJ1
       AG_RIGHT_DC_ADJ2
       AG_RIGHT_AD_ADJ
       AG_RIGHT_GAIN_ADJ
 
 Due to the real time nature of the system, none of the above defined
 registers should be accessed directly by other routines.

Algorithms

Internal Data
*/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#include <80C196.h>
#endif
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "general.h"
#include "iir.h"
#include "mm_coms.h"
#include "dim.h"
#include "proto.h"
#include "hw.h"
#include "hw_mem.h"

/**************************************************************
 * Internal enums/typedefs 
 */
#define shift(p1) p1 = 0

/* 
 * Constants 
 */

/*
 * Attenuator control variable
 */
 const char hw_trans_table[HWE_END_ATT] = {
       NO_ATT,    /* Spacing     */
       NO_ATT,    /* HWE_ATT_1   */
       ATT_3,     /* HWE_ATT_3   */
       ATT_5,     /* HWE_ATT_5   */
       ATT_10,    /* HWE_ATT_10  */
       ATT_30,    /* HWE_ATT_30  */
       ATT_50,    /* HWE_ATT_50  */
       ATT_100,   /* HWE_ATT_100 */
       ATT_300,   /* HWE_ATT_300 */
       ATT_500    /* HWE_ATT_500 */
  };
#define HW_AUTO_ATT_LOWER HWE_ATT_1
#define HW_AUTO_ATT_UPPER HWE_ATT_500

/* DAC Control Register bits */
#define DAC_WR_MASK (~0x1)    /* D0/Q1  */
#define DAC_ADDR_MASK (~0x2)  /* D1/Q2  */
#define DCS1 (~0x4)   /* D2/Q3 */
#define DCS2 (~0x8)   /* D3/Q4 */
#define DCS3 (~0x10)  /* D4/Q5 */
#define DCS4 (~0x20)  /* D5/Q6 */
#define INACTIVE_DAC_CTL_REG 0xff /* Q1->8 inactive high */

/* Externally defined */
 extern unsigned char ioport1_mirror, ioport2_mirror;

/* Internal storage to this module */
  /* Struct to hold status information, accessibile through
   * hw_analag_stat to other procedures
   */
   hwt_fec_status status;

  /* hw_front_end - mirror for AG_LEFT and AG_RIGHT
   * Holds the last value written to the front end registers
   * Same front end for both filters 
   * This value is the one that is written to the LAT-L & LAT-R registers
   */
  int hw_front_end; 

/* I/O hardware on the analog board */
char lat_test;
#ifdef REAL_TARGET
#pragma locate(lat_test=LAT_TEST) /* LAT_TEST */
#endif /* REAL_TARGET */

char lat_l, lat_r, lat_dac, lat_com, clk_com, clk_test;
#ifdef REAL_TARGET
#pragma locate(lat_r=LAT_R, lat_l=LAT_L)
#pragma locate(lat_dac=LAT_DAC, lat_com=LAT_COM)
#pragma locate(clk_com=CLK_COM, clk_test=CLK_TEST)
#endif /* REAL_TARGET */


/* Internal prototypes */
 void hw_analog_set(int type, int value);
 int att_translate(enum hwe_att_setting request_setting, int *filter_att);
 int get_dac_addr(int type);
 void send_to_test(int value);
 void send_to_other(int value);
 void set_dout(int dout);

/* Sample */
/*void shift_test() {
/!*
lat_dac, Y
lat_com,  Y 
lat_l, Y
lat_r, Y
clk_com, 
--
lat_test;
clk_test;
*!/
	clk_test =0;
}
*/
/*;*<*>********************************************************
  * hw_fec_init
  *
  **start*/
 void hw_fec_init(Initt_system_state *ssp)
 {

    if (ssp->init_type == COLD) {
       /*
        * Only perform on COLD initialisation
        */
       status.montage = HW_FE_LATERAL_IN;
       status.hw_att_setting = HWE_ATT_5; /* Default attenuator setting */
       /*hw_front_end = NORMAL_FRONT_END;*/
    } 
    /* Ensure hardware initialized to default setting */
    hw_front_end_mode(status.montage,HWE_ATT_BOTH,status.hw_att_setting);
/*
    hw_attenuator(HWE_ATT_BOTH,status.hw_att_setting);
    hw_analog_set(AG_TEST,INIT_AG_TEST);
*/
 } /*end hw_fec_init*/

/*;*<*>********************************************************
  * hw_attenuator
  *
  * This function changes the attenuation settings in the 
  * front end.
  *
  * For the time being both filters are changed together
  *
  **start*/
 void hw_attenuator(enum hwe_which_att which_filter,enum hwe_att_setting type)
 {
 int filter_att;
 /*********
  * Check to see if change rather than absolute filter setting
  */
 if (type == HWE_INC_ATT) {
    status.hw_att_setting++;
 } else if (type == HWE_DECR_ATT) {
    status.hw_att_setting--;
 } else if (type == HWE_REFRESH) {
   /* take no action -as using internal value */
 } else {
    status.hw_att_setting = type;
 } 

 /*********
  * Do some limit checking
  */
 if ((int)status.hw_att_setting > (int)HW_AUTO_ATT_UPPER) {
    status.hw_att_setting = HW_AUTO_ATT_UPPER;
 }
 if ((int)status.hw_att_setting < (int)HW_AUTO_ATT_LOWER) {
    status.hw_att_setting = HW_AUTO_ATT_LOWER;
 }
 /*
  * Translate to real hardware values
  */
 if (att_translate(status.hw_att_setting,&filter_att) == (int)FAIL)
    /* Couldn't translate - don't update */
    return;
 /*
  * Change the hardware
  *   - first update the mirror
   *  - then change the hardware
  */
 hw_front_end = ((filter_att & JUST_FRONT_END_ATT_MASK) |
                 (hw_front_end & ~JUST_FRONT_END_ATT_MASK) );
 hw_analog_set(AG_LEFT_CTL,hw_front_end);
 hw_analog_set(AG_RIGHT_CTL,hw_front_end);

 } /*end hw_attenuator*/

/*;*<*>********************************************************
  * hw_front_end_mode
  *
  * This function sets up the front end amplifiers to receive the
  * specified signals from a number of sources. These are
  *    - differentially from the head electrode inputs
  *    - front single ended from the head electrode 
  *    - back single ended from the head electrode
  *    - self test
  *    - contact test
  *    - open circuit (ie not connected to anything)
  *    - closed circuit (ie s/c together)
  * Inputs
  *  fe_in
  *                           RB1 RB2 RB3 RB4
  *                           LB1 LB2 LB3 LB4 TEST    ATT
  *    HW_FE_LATERAL_IN        X       X            saved setting
  *    HW_FE_FRONT_IN              X   X              ???
  *    HW_FE_BACK_IN               X       X          ???
  *    HW_FE_SELF_TEST                          X     ???
  *    HW_FE_CNTCT_TEST        X       X        X     ???
  *    HW_FE_OPEN_CCT_TEST                            1uV
  *    HW_FE_SHORT_CCT_TEST    X           X          1uV
  *  which_filter
  *    HWE_ATT_BOTH
  *    HWE_ATT_LEFT  *** NOT SUPPORTED ***
  *    HWE_ATT_RIGHT *** NOT SUPPORTED ***
  *
  *  type
  *     (see enum definition)
  *
  **start*/
 void hw_front_end_mode(enum hwe_fe_in fe_in,
                        enum hwe_which_att which_filter,
                        enum hwe_att_setting type )
{

   status.montage = fe_in;

   switch (fe_in) {

   case HW_FE_LATERAL_IN:
      /*
       * Set the front end to lateral montage input mode
       * and use specified attenuator settings
       *
       *  - change the hardware mirror for montage setting
       *  - set the att, which also writes the hardware mirror.
       *  - set the TEST latch
       */
      hw_front_end = (LATERAL_MONTAGE_FRONT_END |
                 (hw_front_end & (~JUST_FRONT_END_MONTAGE_MASK)) );
      hw_attenuator(HWE_ATT_BOTH, type);
      hw_analog_set(AG_TEST,INIT_AG_TEST);
      break;

   case HW_FE_FRONT_IN:
      /*
       * Set the front end to front montage input mode
       * and use specified attenuator settings
       *
       *  - change the hardware mirror for montage setting
       *  - set the att, which also writes the hardware mirror.
       *  - set the TEST latch
       */
      hw_front_end = (FRONT_MONTAGE_FRONT_END |
                 (hw_front_end & (~JUST_FRONT_END_MONTAGE_MASK)) );
      hw_attenuator(HWE_ATT_BOTH, type);
      hw_analog_set(AG_TEST,INIT_AG_TEST);
      break;

   case HW_FE_BACK_IN:
      /*
       * Set the front end to back montage input mode
       * and use specified attenuator settings
       *
       *  - change the hardware mirror for montage setting
       *  - set the att, which also writes the hardware mirror.
       *  - set the TEST latch
       */
      hw_front_end = (BACK_MONTAGE_FRONT_END |
                 (hw_front_end & (~JUST_FRONT_END_MONTAGE_MASK)) );
      hw_attenuator(HWE_ATT_BOTH, type);
      hw_analog_set(AG_TEST,INIT_AG_TEST);
      break;

   case HW_FE_SELF_TEST:
      hw_front_end = FE_SELF_TEST;
      hw_analog_set(AG_LEFT_CTL,FE_SELF_TEST);
      hw_analog_set(AG_RIGHT_CTL,FE_SELF_TEST);
      hw_analog_set(AG_TEST,FE_TEST_ENABLE);
      break;

   case HW_FE_CNTCT_TEST:
      hw_front_end = FE_CNTCT_TEST;
      hw_analog_set(AG_LEFT_CTL,FE_CNTCT_TEST);
      hw_analog_set(AG_RIGHT_CTL,FE_CNTCT_TEST);
      hw_analog_set(AG_TEST,FE_TEST_ENABLE);
      break;

   case HW_FE_OPEN_CCT_TEST:
      hw_front_end = FE_OPEN_CCT;
      hw_analog_set(AG_LEFT_CTL,FE_OPEN_CCT);
      hw_analog_set(AG_RIGHT_CTL,FE_OPEN_CCT);
      hw_analog_set(AG_TEST,INIT_AG_TEST);
      break;

   case HW_FE_SHORT_CCT_TEST:
      hw_front_end = FE_SHORT_CCT;
      hw_analog_set(AG_LEFT_CTL,FE_SHORT_CCT);
      hw_analog_set(AG_RIGHT_CTL,FE_SHORT_CCT);
      hw_analog_set(AG_TEST,INIT_AG_TEST);
      break;
   default:
      break;
   }
 } /*end hw_front_end_mode*/

/*;*<*>********************************************************
  * att_translate
  *
  * Translates requested attenuator to a value the hardware
  * can use.
  * Input
  *  request setting
  **start*/
 int att_translate(enum hwe_att_setting request_setting, int *filter_att)
 {
 int temp;
    if ((int)request_setting < (int)HWE_END_ATT){
       *filter_att = hw_trans_table[request_setting];
       return PASS;
    }
    return FAIL;

 } /*end att_translate*/

/*;*<*>********************************************************
  * hw_att_value
  *
  * Function to get the hw_att_setting - called a lot so made it fast.
  *
  * Must be re-entrant as called from interrupts
  **start*/
  enum hwe_att_setting hw_att_value(int side)
 {
   return status.hw_att_setting;
 } /*end hw_att_value*/

/*;*<*>********************************************************
  * hw_analog_set
  *
  * This function sets the hardware registers on the analog board.
  *
  * The "value" byte is sent to the 'type' register on the analog
  * board. 
  *
  * Algorithm for writing to AG_TEST
  *   Disable interrupts
  *   send 'value' to test shift register
  *   Write anything to LAT-TEST
  *   Enable interrupts
  *
  * Algorithm for writing to AG_LEFT_CTL, AG_RIGHT_CTL & AG_TEST
  *   Disable interrupts
  *   send 'value' to other shift registers
  *   Write anything to LAT-L    for AG_LEFT_CTL
  *                      LAT-R    for AG_RIGHT_CTL
  *   Enable interrupts
  *
  * Algorithm for the DACs (Digital-to-Analog-Converters)
  *   Disable interrupts
  *   setup DAC control register including address bit
  *   Write anything to LAT-DAC
  *   send 'value' to other shift registers
  *   Write anything to LAT-COM
  *   send write bit to DAC control register
  *   Write anything to LAT-DAC
  *   Setup DAC control register to inactive data
  *   Write anything to LAT-DAC
  *   Enable interrupts
  *
  *  Note: 1) Initial B002 rev 3 hardware is not stuffed with the TEST ics
  *   U85 & U84. Instead R83 is stuffed so that EMG-RIGHT goes to TEST.
  *        2) DACs have been removed from b002-3 hardware, however one
  *           control bit for the test points is still used.
  **start*/
 void hw_analog_set(int type, int value)
 {
 int dac_addr;

#ifndef SIMULATING 
 disable();
#endif

 switch(type) {
 case AG_TEST:          /* Control of test cct                 */
    send_to_test(value);
    shift(lat_test);    /* B002-2 U65  */
    break;

 case AG_LEFT_CTL:      /* Control of left cct                 */
    send_to_other(value);
    shift(lat_l);       /* B002-2 U67 */
    break;

 case AG_RIGHT_CTL:     /* Control of right cct                */
    send_to_other(value);
    shift(lat_r);       /*  B002-2 U68 */
    break;

 default:
    dac_addr = get_dac_addr(type);
    send_to_other(dac_addr);
    shift(lat_dac);
    send_to_other(value);
    shift(lat_com);
    send_to_other(dac_addr & DAC_WR_MASK);
    shift(lat_dac);
    send_to_other(INACTIVE_DAC_CTL_REG);
    shift(lat_dac);
 }
#ifndef SIMULATING
 enable();
#endif
 } /*end analog_set*/

/*;*<*>********************************************************
  * hw_fec_status
  *
  * Copies to a user supplied buffer the status of this module
  **start*/
 void hw_fec_status(hwt_fec_status *p)
 {
    p->montage        = status.montage;
    p->hw_att_setting = status.hw_att_setting;

 } /*end hw_fec_status*/

/*;*<*>********************************************************
  * get_dac_addr
  *
  * This routine translates the type to a value that can
  * be sent to the DAC control register.
  * 
  * The hardware expects the following signals
  *  HW Sig   SW Name          A0 bit    DAC bit
  *  VLA1     AG_LEFT_DC_ADJ1    0       DCS1 Q3
  *  VLA2     AG_LEFT_DC_ADJ2    1       DCS1 Q3
  *  VLA3     AG_LEFT_AD_ADJ     0       DCS2 Q4
  *  AL1/2    AG_LEFT_GAIN_ADJ   0       DCS4 Q6
  *  VRA1     AG_RIGHT_DC_ADJ1   0       DCS3 Q5
  *  VRA2     AG_RIGHT_DC_ADJ2   1       DCS3 Q5
  *  VRA3     AG_RIGHT_AD_ADJ    1       DCS2 Q4
  *  AR1/2    AG_RIGHT_GAIN_ADJ  1       DCS4 Q6
  *
  * The translation to the return value is
  *    Q1  Q2  Q3  Q4  Q5  Q6  Q7  Q8
  *    D0  D1  D2  D3  D4  D5  D6  D7
  * where it is expected that D7(Q8) is shifted out first.
  *
  **start*/
 int get_dac_addr(int type)
 {

    switch (type) {
    case AG_LEFT_DC_ADJ1:  /* First stage front end DC adjustment */
       return(DCS1 & DAC_ADDR_MASK);

    case AG_LEFT_DC_ADJ2:  /* Second stage front end DC adjustment*/
       return (DCS1 | ~DAC_ADDR_MASK);

    case AG_LEFT_AD_ADJ:   /* A/D DC shift adjusment              */
       return (DCS2 & DAC_ADDR_MASK);

    case AG_LEFT_GAIN_ADJ: /* Fourth Stage gain adjusment         */
       return (DCS4 & DAC_ADDR_MASK);

    case AG_RIGHT_DC_ADJ1: /* First stage front end DC adjustment */
       return (DCS3 & DAC_ADDR_MASK);

    case AG_RIGHT_DC_ADJ2: /* Second stage front end DC adjustment*/
       return (DCS3 | ~DAC_ADDR_MASK);

    case AG_RIGHT_AD_ADJ:  /* A/D DC shift adjusment              */
       return (DCS2 | ~DAC_ADDR_MASK);

    case AG_RIGHT_GAIN_ADJ:/* Fourth Stage gain adjusment         */
       return (DCS4 | ~DAC_ADDR_MASK);

    default:
       return INACTIVE_DAC_CTL_REG;
    }

 } /*end get_dac_addr*/

/*;*<*>********************************************************
  * send_to_test
  *
  * This functions sends 'value' to the analog hardware test
  * shift register
  *
  * The LSB is sent first.
  **start*/
 void send_to_test(int value)
 {
   int loop;

   for(loop=0x80;loop > 0; loop >>= 1) {
     set_dout(loop&value);
     shift(clk_test);
   }

 } /*end send_to_test*/

/*;*<*>********************************************************
  * send_to_other
  *
  * This functions sends 'value' to the all the analog boards 
  * shift registers except 'test'.
  *
  **start*/
void send_to_other(int value)
 {
   int loop;

   for(loop=0x80;loop > 0; loop >>= 1) {
     set_dout(loop&value);
     shift(clk_com);
   }

 } /*end send_to_other*/

/*;*<*>********************************************************
  * set_dout
  * 
  * This function sets/resets the bit that is used to
  * shift values to the analog hardware
  *
  * If the 'input' is 0, then bit is set to 0
  * If the 'input' is not 0, then the bit is set to 1
  **start*/
 void set_dout(int dout)
 {
#define DOUT_IS_1 0x10  /* Port 1.4 bit */
    if (dout) {
#ifndef SIMULATING
       ioport1 = 
#endif
       ioport1_mirror = ioport1_mirror | DOUT_IS_1;
    } else {
#ifndef SIMULATING
       ioport1 = 
#endif
       ioport1_mirror = ioport1_mirror & ~DOUT_IS_1;
    }
 } /*end set_dout*/

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

