/* hw.c
 
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
 The hw subsystem provides basic functions for accessing the 
 Mind Mirror III digital and analog hardware.
 
 This subsystem owns the following hardware ports;
      Port 1        - 0   Keypad/Hw I.D.
                      2-3 Keypad
                      4   Data Expansion Port
                      5-7 Ram Bank
      Port 2          Ext RTC int
                      3,4 Keypad input
                      5   LCD PWM
                      6   Keypad input
                      7   Spare
rev3  HSi0   Pin24    keypad-int
rev4  ....   .....    Event, Remote Freeze that is polled
rev3  HSi1   Pin25    (not used)Low battery interrupt
rev4  ....   .....    (not used)DBG1
rev3  HSi2o4 Pin26    Test o/p HZ15
rev4  ...... .....    (not used)DBG2 or KEYpadEvent
rev3  HSi3o5 Pin27    i/p Remote Freeze
rev4  ...... .....    o/p ABNK4
      HSo0   Pin28    Switch Capacitor Freq
rev3  HSo1   Pin29    (not used)-PWR-OFF
rev4  ....   .....    Test Output HZ15
      HSo2   Pin34    RS232 STBY
      HSo3   Pin35    Test output HZ9


 The following object like interfaces are defined for
 the Mind Mirror III 80C196KC digital board
                      WR     RET
    hw_keypad_poll           int          hw_key.c                  TESTED Rev1
    hw_lcd_contrast_set int  int
    low_bat_stat              1
    low_bat_int               1     invoked by this module
    led_set            2
    rs232_stby_set     2 
    rs232_stby_stat           1
    pwr_off_int            
    hw_pwr_set
    hw_test_freq                                              TESTED rev1
    hw_attenuator                         hw_fec.c
    hw_ram_bank_set
    hw_ram_bank_stat

 This subsystem is responsible for all handshaking with the hardware, and
 unless otherwise stated, all routines may be invoked from an interrupt
 or a non-interrupt level. 

 Note: Most routines will disable global interrupts on entry and
 enable them on exit. Callers should take this into account.

*/

#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#include <80C196.h>
#endif /* IC96 */
/*#include <stdio.h>*/
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

/*
 * Constants
 */
#define TIMER1_MIN_DELAY 6

/**************************************************************
 * Shared variables 
 */

/**************************************************************
 *
 * Internal storage to this module
 *
 *
 */
/*#ifdef IC96 */
 unsigned char ioport1_mirror, ioport2_mirror;
 unsigned char lcd_contrast_mirror;
 unsigned int hw_test_freq_flag; /* semaphore */
          int test_front_hz, /* Timer */
              test_back_hz;  /* Timer */

#ifdef SIMULATING
 unsigned char hso_command,hso_time,timer1,ioport2,wsr,t2control,ioc2;
#endif /* SIMULATING */

/**************************************************************
 * Internal Prototypes
 */

/*;*<*>********************************************************
  * hw_init
  *
  * This module initialise the hardware that it owns.
  *
  * This includes
  *      initialising the port mirrors
  *      initialising the HSO CAM
  *      setting up 8KHz signal for the analog board. 
  *      This uses TIMER2 and HSO0
  *        and threee entries in the HSO CAM.
  *      enable HSO.4
  *
  * The following variables are initialised on a COLD init only
  *    
  **start*/
 void hw_init(Initt_system_state *ssp)
 {

   /***************************************
    * Initialise Port 1 and Port 2 hardware
    * and associated mirrors
    *
    * Note: Do not write to ioport0 as it
    * is a read-only register. The write action
    * writes to the BAUD RATE register.
    */
#define INIT_PORT 0xff;  /* Set all high as quasi i/o ports */
#ifndef SIMULATING
   ioport1 =
#endif /* SIMULATING */
   ioport1_mirror = INIT_PORT;

#ifndef SIMULATING
   ioport2 = 
#endif
   ioport2_mirror = INIT_PORT;


   /***************************************
    * Initialise IOC0 -
    *  enable HSI.0 HSI.1 HSI.3 as inputs
    *
    */
   ioc0 = IOC0_INITIALISE;

   /***************************************
    * Initialise IOC1 -
    *   initialised in  init_act:initialisation() as
    *   UART must be available for self test.
    *   See hw.h for INIT_IOC1_INITIALISE
    */

   /***************************************
    * Initialise IOC2 (Pg 5-109), this controls
    *     Timer 2 modes & Prescaler
    *     A/D Modes & prescaler
    *     HS CAM -
    *
    */
   ioc2 = (FLUSH_CAM | IOC2_INITIALISE);
/*   ioc2 = EN_CAM_LOCK; del 17-May-92 */


   /***************************************
    * Init the 8KHz analog filter frequency
    *
    * Set TIMER2 to internal clocking
    *  T2CNTC.0 = 1 Pg 5-45 & 5-109
    *   (t2control is in HWINDOW1)
    */
   wsr = HWINDOW1;
#define INIT_T2CONTROL 1
   t2control = INIT_T2CONTROL;
   wsr = HWINDOW0;

   /*
    * Lock entries into the CAM to
    *   Set  HSO.0 after 62.5uS relative to TIMER2
    *   Clear HSO.0 after 125uS relative to TIMER2
    *   Reset TIMER2 after 125uS relative to TIMER2
    */
   hso_command = SET_FILTER_FREQ_OP;
   hso_time    = SET_FILTER_FREQ_VALUE;

   hso_command = CLR_FILTER_FREQ_OP;
   hso_time    = CLR_FILTER_FREQ_VALUE;
 
   hso_command = RESET_TIMER2;
   hso_time    = CLR_FILTER_FREQ_VALUE;

   /***************************************
    * Initialise the Ports Resposnibile for
    * maintaining power to the system.
    *   CPU Power P2.7 hw reset state  1,  set to 1
    *   Aux Power HSO1 hw reset state  0,  set to 0
    */
#define HW_PWR_BIT 0x80
#define AUX_PWR_OFF 0x01
#define AUX_PWR_ON 0x21
#ifndef SIMULATING 
   ioport2 = 
#endif /* SIMULATING */
   ioport2_mirror = (ioport2_mirror | (HW_PWR_BIT));

/* Was never used
*    hso_command = AUX_PWR_ON;
*    hso_time    = hso_time + 10; /* immediately *!/
*/
   /***************************************
    * Initialise PWM P2.5
    *   Reset state semi-weak pull down Pg 5-75
    *   The following has been done already
    *     - IOC1.0 set to select PWM for P2.5 (Pg 5-109)
    *     - IOC2.2 input to be state time clock/2(pg 5-109)
    *
    */
#define INIT_PWM_LCD_CONTRAST 0x80
   pwm_control = lcd_contrast_mirror = INIT_PWM_LCD_CONTRAST;

   /***************************************
    * Initialise software generated test timers
    */
#define INIT_TEST_TIMER 0
   test_front_hz = test_back_hz = INIT_TEST_TIMER;
   hso_command = SET_HSO_HZ15; /* Set for debugging */
   hso_time = timer1 + TIMER1_MIN_DELAY;
   hso_command = SET_HSO_HZ9; /* Set for debugging */
   hso_time = timer1 + TIMER1_MIN_DELAY;

    /*
     * Init other files in this module
     */
    hw_fec_init(ssp);
    hw_key_init(ssp);

 } /*end init_kn_hw*/

/*;*<*>********************************************************
  * hw_lcd_contrast_set
  *
  * This function adjust the contrast of the LCD. It does this by
  * altering the PWM0/P2.5 output.
  *
  * Input
  *   if greater than 0xff, the value is masked with 0xff and written
  *      directly to the PWM register
  *   otherwise assumed to be between +0x7f and -0x80, and is added to the
  *      pwm_mirror and then written to the pwm register 
  *
  * Hardware PWM description pg
  *   A value of 0 sets the output to 0V
  *   A value of 0xff sets the PWM output to +5v
  **start*/
 int hw_lcd_contrast_set(int pwm_value)
 {
 int lcd_temp;
#define PWM_VALUE_THRESHOLD 0xff
   if (pwm_value > PWM_VALUE_THRESHOLD) {
      lcd_contrast_mirror = ((unsigned char)pwm_value); /* Auto Masking */
   } else {
      lcd_temp = ((unsigned int)lcd_contrast_mirror + pwm_value);
#define LCD_PWM_LOWER_LIMIT 0
#define LCD_PWM_UPPER_LIMIT 0xff
      if (lcd_temp < LCD_PWM_LOWER_LIMIT) {
         lcd_temp = LCD_PWM_LOWER_LIMIT;
      } else if (lcd_temp > LCD_PWM_UPPER_LIMIT) {
         lcd_temp = LCD_PWM_UPPER_LIMIT;
      }
      lcd_contrast_mirror = (unsigned char) lcd_temp;
   }
   pwm_control = lcd_contrast_mirror;
   return lcd_contrast_mirror;

 } /*end hw_lcd_contrast_set*/
/*;*<*>********************************************************
  * low_bat_stat
  **start*/
/* ()
 {

 } /*end*/
/*;*<*>********************************************************
  * led_set
  **start*/
/* ()
 {

 } /*end*/
/*;*<*>********************************************************
  * rs232_stby_set
  **start*/
/* ()
 {

 } /*end*/
/*;*<*>********************************************************
  * rs232_stby_stat
  **start*/
/* ()
 {

 } /*end*/
/*;*<*>********************************************************
  * pwr_off
  **start*/
/* ()
 {

 } /*end*/
/*;*<*>********************************************************
  * hw_test_freq
  *
  * This function enables/disables the generation of
  * test frequencies.
  *
  * Hence this module works by enabling some processing in the
  * sw timer interrupt routine
  *
  * see function hw_test_int()
  **start*/
 void hw_test_freq(int type)
 {

   if (type == (int) TEST_FREQ_ENABLE) {
      hw_test_freq_flag = (int) TEST_FREQ_ENABLE;
   } else {
      hw_test_freq_flag = (int) TEST_FREQ_DISABLE;
   }
 } /*end hw_test_frequency*/

/*;*<*>********************************************************
  * hw_test_int
  *
  * This function is called from the interrupt level if 
  *   hw_test_freq_flag == TEST_FREQ_ENABLED
  *
  * It generates the test frequencies
  *           9 Hz on HSO.3 and 
  *          15 Hz on HSO.4
  *
  *  9 Hz has a period of 111.111mS or approx 28 256 Hz interrupts
  * 15 Hz has a period of  66.667mS or approx 16 256 Hz interrupts
  *
  * Algorithm
  *    Note: Brute Force Features, no intelligence.
  *    if ++timer less than half the required period then HS = 0
  *    else if timer less than  the required period then HS = 1
  *    else timer = 0
  *
  * Assumptions
  *    Interrupts not enabled - so nothing can interrupt
  *     between hso_command being set and hso_time being written to
  *
  * Rev-2 Hardware
  *     HSO3 is for 9 Hz output
  *     HSO4 is for 15 Hz output
  **start*/
 void hw_test_int()
 {
#define FRONT_TEST_FREQ 7 /* Frequency inserted into front probe */
#define BACK_TEST_FREQ 18 /* Frequency inserted into back probe  */
#define MAKE_EVEN_NUM 0xfffe
#define HW_TEST_FRONT_CNT ((SAMPLING_FREQUENCY/FRONT_TEST_FREQ) & MAKE_EVEN_NUM)
#define HW_TEST_BACK_CNT ((SAMPLING_FREQUENCY/BACK_TEST_FREQ) & MAKE_EVEN_NUM)
  /*
   * Front lead frequency generator
   */
  if ( ++test_front_hz <= (HW_TEST_FRONT_CNT >> 1)) {
     /*
      * HSO.hz9 = 0
      */
     hso_command = RES_HSO_HZ9;
     hso_time = timer1 + TIMER1_MIN_DELAY;

  } else if (test_front_hz < HW_TEST_FRONT_CNT) {
     /*
      * HSO.hz9 = 1
      */
     hso_command = SET_HSO_HZ9;
     hso_time = timer1 + TIMER1_MIN_DELAY;

  } else  {
     test_front_hz = INIT_TEST_TIMER;
  }

  /*
   * Back lead frequency generator
   */
  if ( ++test_back_hz <= (HW_TEST_BACK_CNT>>1)) {
     /*
      * HSO.hz15 = 0
      * (rev3 HSO.4=0 / rev4 HSO.1=0)
      */
     hso_command = RES_HSO_HZ15;
     hso_time = timer1 + TIMER1_MIN_DELAY;

  } else if (test_back_hz < HW_TEST_BACK_CNT) {
     /*
      * HSO.hz15 = 1
      * (rev3 HSO.4=1 / rev4 HSO.1=1)
      */
     hso_command = SET_HSO_HZ15;
     hso_time = timer1 + TIMER1_MIN_DELAY;

  } else  {
     test_back_hz = INIT_TEST_TIMER;
  }

 } /*end hw_test_int*/

/*;*<*>********************************************************
  * hw_ram_bank_set
  *
  * Sets the external banked ram to the specified bank
  *
  * Port 1 bits 5,6,7 are used for bank switching
  * New Rev 4 Digital H/W 2 more bits used.
  *                        Port 2.7 HI3H05.
  *     If only 128K ram used then Port 2.7 must be set high.
  *
  * Input: bank_no Range 0->7,
  **start*/
  void hw_ram_bank_set(enum ram_bank_number bank_no)
 {
#define IOPORT1_RAM_BANK_MASK 0xe0
#define RAM_BANK_SHIFT 5
#define ABNK5_MASK 0x80 /* Port2.7 */
   /* Turn off interrupts while accessing hw and mirrors */	  
   /*disable(); */

#ifndef SIMULATING
   ioport1 =
#endif /* SIMULATING */
   ioport1_mirror = (((bank_no << RAM_BANK_SHIFT) & IOPORT1_RAM_BANK_MASK) |
                     (ioport1_mirror & ~IOPORT1_RAM_BANK_MASK)
                    );

   /* Set ioport2 mirror until we start using large rams */
#ifndef SIMULATING
   ioport2 =
#endif /* SIMULATING */
   ioport2_mirror |= ABNK5_MASK;

   /*enable();*/

   } /*end hw_ram_bank_set*/

/*;*<*>********************************************************
  * hw_ram_bank_stat
  *
  * Returns the bank that the external ram is set to.
  *
  * Return enum ram_bank_number
  **start*/
/* enum ram_bank_number hw_ram_bank_stat(void)
 {

  return ((ioport1_mirror & IOPORT1_RAM_BANK_MASK) >> RAM_BANK_SHIFT);

 } /*end hw_ram_bank_stat*/

/*;*<*>********************************************************
  * hw_sn
  *
  * This function reads the hardware serial number and
  * returns it in the supplied function.
  *
  * Its is expected that it will read a DS2400 witha 48-bit
  *  serial number. Thus there will be a maximum of 6*8bits
  *
  * DLLC_INFO_SN defines the maximum length of the serial number
  *
  **start*/
 Byte *hw_sn(Byte *serial_number_p)
 {
     strncpy(serial_number_p," 102345 ",8);

 } /*end hw_sn*/

/*;*<*>********************************************************
  * uart_driver
  *
  * This function turns the RS232 driver for the internal UART
  * ON or OFF
  *
  * The RS232 driver is turned ON by setting the HSO2 to a 1.
  *                           OFF by setting the HSO2 to a 0.
  *
  * Ths HSO2 hardware output is a 0 following RESET.
  *
  * return values
  *    For RS232 driver ON the return value is the delay in ms
  *         before the RS232 driver comes on
  *    For RS232 driver OFF the return value is 0
  *
  * Note: 1) LT1180CS manual suggest turnon time for drivers
  *       is 200uS.
  *       2) This function is called with the stack set to internal
  *          ram during initialisation.
  **start*/
  int uart_driver(enum uart_driver_cmd cmd)
 {
#define INTERNAL_UART_DRIVER_ON 0x22  /* Set HSO2 to 1 */
#define INTERNAL_UART_DRIVER_OFF 0x02 /* Set HSO2 to 0 */
   register int ret_value=0; /* Must be register */

   if (cmd == UART1_DRV_ON) {
      hso_command = INTERNAL_UART_DRIVER_ON;
      ret_value = 200;  /* calling routine must delay 100mS */
    } else { /* UART1_DRV_OFF */
      hso_command = INTERNAL_UART_DRIVER_OFF;
    }
    hso_time = timer1 + 2;  /* Anything guarenteed to turn it on soon */
    return ret_value;

 } /* uart_driver end*/

/*;*<*>********************************************************
  * 
  **start*/
/* ()
 {

 } /*end*/
