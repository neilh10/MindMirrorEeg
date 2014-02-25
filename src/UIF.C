/*  uif.c
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
  This module is responsible for interfacing to the user. 
  This is primarily through
       Keypad input
       The status line updates

  Any key presses are interpreted via a state machine and a series of
  events to other modules generated.

  The inputs to do this are
     Freeze/Event Switch
     Keypad

 The keys that are reported as an event are
     KEY_UP_ARROW
     KEY_DOWN_ARROW
     KEY_LEFT_ARROW
     KEY_RIGHT_ARROW
     KEY_DISPLAY
     KEY_TEST
     KEY_OPTIONS
     KEY_RESTART
     KEY_REVIEW

 In addition the following events are received
    KEY_REPEAT1  
    KEY_REPEAT2

 The following NULL inidcations are receivable
    NO_KEY -
    KEY_ACTIVE  - Keypad has keypressed, that has already been 
                  reported as an event.

 The way the key presses events are interpreted depends on the state the
 uif module is in. This is defined by "uife_mode" and is

       UIFE_DISPLAY     Displaying output of filters
       UIFE_REVIEW      Reviewing stored data
       UIFE_REVIEW_PLAY Plays reviewed data forward
       UIFE_OPTS        Presenting options to user
       UIFE_TEST        Self test mode
       UIFE_DEMO        Demonstration mode


Finite State Machine
  Key Repeat Status
     Two state machine controlled be semaphore "uif_no_repeats"
     Set to no repeats by
         init_initialisation()
         NO_KEY report in uif_poll()

     Cleared by module processing allowing key repeats - ie on
         individual module/keystroke processing.


Algorithms
  Note: some modules are not re-entrant
   uif_initialisation  - first
   uif_poll            - as necessary

Assumptions
It is assumed that if a cold initialisation is performed that the
system has invalid data. For a warm initialisation assume data is valid
as far as is possible.


*/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "general.h"
#include "ldd.h"
#include "iir.h"
#include "sch.h"
#include "hw.h"
#include "uif.h"
#include "drm.h"
#include "mm_coms.h"
#include "dim.h"
#include "proto.h"
#include "mfd.h"
#include "ssp.h"
#include "dfm.h"
#include "mmstatus.h"

/**************************************************************
 * Constants 
 */
 enum common_actions {
 /* These constants are used to define actions that are common to
  * different parts of the subprogram*/
  SET_NO_ACTION=0,
  SET_MODE_DISPLAY=0x01,    /* Force a change to UIFE_DISPLAY          */
  SET_MODE_OPTS=0x02,       /* Force a change to UIFE_OPTS             */
  SET_MODE_OPTS_OFF=0x04,   /* Turn UIFE_OPTS off                      */
  SET_MODE_TEST=0x08,       /* Force a change to UIFE_TEST and contact test*/
  SET_MODE_TEST_S=0x10,     /*Force a change to UIFE_TEST and self test*/
  SET_TEST_OFF=0x20,        /* Turn test functions off                 */
  SEND_LCD_DARKER=0x40,
  SEND_LCD_LIGHTER=0x80,
  DELAY_AND_READ_DATA=0x100,
  SET_MODE_DEMO=0x200,       /* Change to demonstration mode           */
  SET_MODE_DEMO_OFF=0x400,   /* Clear up for turning demo mode off     */
  SET_MODE_REVIEW=0x800,     /* Change to review mode                  */
  SET_MODE_REVIEW_OFF=0x1000,/* Clear up for turning review mode OFF   */
  SET_MODE_REVIEW_PLAY=0x2000,
  SET_MODE_REVIEW_FROM_PLAY=0x4000,
  CLEAR_DRM_BUFFER=0x8000    /* Clear ram buffer out                   */
 };
 typedef enum common_actions Uif_act; /* Common definition for actions */

/**************************************************************
 * Externally defined 
 */
 extern Dplt_data bargraph_new;
 extern Frac ad_result[];
 extern enum ade_channel_masks ad_other_channels;/*Sempahore for extra A/Ds*/
/**************************************************************
 * Internal storage to this module 
 */
 enum UIFE_MODE uife_mode;
 enum UIFE_TEST_MODE test_mode;
 Uif_act uif_action; /* Storage as per enum common_actions */
 Semaphore uif_no_repeats;
 enum key_pressed_enum last_key_pressed;

 struct { /* For key repeat times */
    enum hwt_key_time timer1, timer2;
 } key_repeat_times;
#define KEY_REPEAT_DEF_TIME1 HWE_KEY_TIME_1SEC
#define KEY_REPEAT_DEF_TIME2 HWE_KEY_TIME_END

  /*review_data
   * Holds the data that is used to be reviewed/displayed on screen
   */
 Dplt_data review_data;

  /* Zeros the information fed to the dpl system
   */
 const Dplt_data zero_data = {
   0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0,
   0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0,
   0,0
 };
/**************************************************************
 * Shared Storage
 */
 /* This controls the state of the display */
 enum UIFE_SCREEN_STATE uife_screen_state;

/**************************************************************
 * Macros
 */
 /* This sets the poll_keypad timer default values */
#define set_key_repeat_times(tim1, tim2) \
   key_repeat_times.timer1 = (tim1);\
   key_repeat_times.timer2 = (tim2)

/**************************************************************
 * Internal prototypes 
 */
 void action_mode( enum UIFE_MODE mode, enum key_pressed_enum key);
 void repeat1(enum key_pressed_enum key,void (*comp)(enum key_pressed_enum));
 void mode_display(enum key_pressed_enum key);
 void mode_review(enum key_pressed_enum key);
 void mode_review_play(enum key_pressed_enum key);
 void mode_opts(enum key_pressed_enum key);
 void mode_test(enum key_pressed_enum key);
 void mode_demo(enum key_pressed_enum key);
 void common_uif_actions(Uif_act actions);

/*;*<*>********************************************************
  * uif_initialisation
  * 
  * This initialises this module.
  *
  * On a cold start the following are initialised
  *      uife_mode = display mode
  **start*/
 enum inite_return uif_initialisation(Initt_system_state *ssp)
 {
 int loop;

    if (ssp->init_type == COLD) {
       uife_mode = UIFE_DISPLAY;
       test_mode = UIFE_TEST_CONTACTS;
       uife_screen_state = UIFE_SCREEN_ACTIVE;
    } else {
      /*
       * Assume warm start
       */
      /* check mode is valid */
      if ((int)uife_mode >= (int)UIFE_INVALID) {
         uife_mode = UIFE_DISPLAY;
      }
      if ((int)test_mode >= (int)UIFE_TEST_MODE_INVALID) {
         test_mode = UIFE_TEST_CONTACTS;
      }
      if((int)uife_screen_state >= (int)UIFE_SCREEN_STATE_INVALID) {
         uife_screen_state = UIFE_SCREEN_ACTIVE;
      }

    }

    uif_display_pattern_initialisation(ssp);
    uif_opts_initialisation(ssp);
    uif_upld_initialisation(ssp);
    uif_revw_initialisation(ssp);

    uif_no_repeats = SET_SEMAPHORE;
    last_key_pressed = NO_KEY;
    set_key_repeat_times(KEY_REPEAT_DEF_TIME1,KEY_REPEAT_DEF_TIME2);

    return PASS;

 } /*end uif_initialisation*/

/*;*<*>********************************************************
  * uif_poll
  * 
  * This function is expected to be called periodically to check
  * for any user input.
  *
  * Its function is to poll sources of user input and take
  * action as appropiate.
  *
  * The keypad may mean different things depending on the state of
  * uife_mode.  When a key is pressed, then the control is transferred
  * to the function associated with that mode.
  *
  * Input
  *     sch_type may be 
  *     SCHE_FAST for time dependent processing
  *     SCHE_BACKGROUND for all other processing (not used at present)
  **start*/
 void uif_poll(enum sche_entry sch_type)
 {
 enum key_pressed_enum key;

  /* Check Keypad */
  hw_keypad_fsm();

  /* Poll for keypad event */
  key = hw_keypad_poll(key_repeat_times.timer1,key_repeat_times.timer2);

  if ((key != NO_KEY) & (key != KEY_ACTIVE)) {
     /* Perform the keyboard actions */
     action_mode(uife_mode,key);

  } else if (uife_mode == UIFE_OPTS) {
     uif_opts_entry(NO_KEY);

  } else if (uife_mode == UIFE_REVIEW_PLAY) {
     review_play_action();
  }

  /* Check if any of repeat keys else save it */
  if ( ((int)key != (int) KEY_ACTIVE) &
       ((int)key != (int) KEY_REPEAT1) &
       ((int)key != (int) KEY_REPEAT2)
      )
  {
     last_key_pressed = key;
  }

  /* Check for when keypad not pressed */
  if ( (int)key == (int)NO_KEY) {
     /* Set semaphore to stop future key repeats and init repeat timers*/
     uif_no_repeats = SET_SEMAPHORE;
     set_key_repeat_times(KEY_REPEAT_DEF_TIME1,KEY_REPEAT_DEF_TIME2);
  }

 } /*end uif_poll*/

/*;*<*>********************************************************
  * action_mode
  *
  * Function to make correct action happen
  **start*/
 void action_mode( enum UIFE_MODE mode, enum key_pressed_enum key)
 {

     uif_action = SET_NO_ACTION; /* Initialise - modules change it */

     switch (uife_mode) {
     case UIFE_DISPLAY:
        mode_display(key);
	break;

     case UIFE_REVIEW:
        mode_review(key);
        break;

     case UIFE_REVIEW_PLAY:
        mode_review_play(key);
        break;

     case UIFE_OPTS:
        mode_opts(key);
        break;

     case UIFE_TEST:
        mode_test(key);
        break;

     case UIFE_DEMO:
        mode_demo(key);
        break;
     }
     common_uif_actions(uif_action);

 } /*end action_mode*/

/*;*<*>********************************************************
  * repeat1
  * 
  * Checks whether a KEY_REPEAT1 event is allowed and then
  * if an allowed event, executes it.
  *
  **start*/
 void repeat1(enum key_pressed_enum key,void (*comp)(enum key_pressed_enum))
 {
    if (uif_no_repeats == SET_SEMAPHORE) {
       return;  /* No repeat keys allowed */
    }

    if ((int)key != (int) KEY_REPEAT1) {
       (*comp)(key); /* Execute repeat function*/
    }
 } /*end repeat1*/

/*;*<*>********************************************************
  * mode_display
  *
  * A key has been pressed while in the UIFE_DISPLAY mode.
  *
  **start*/
 void mode_display(enum key_pressed_enum key)
 {
   switch (key) {
   case KEY_UP_ARROW:
      uif_action = SEND_LCD_DARKER;
      uif_no_repeats = CLR_SEMAPHORE; /* Allow repeat key presses */
      break;
   case KEY_DOWN_ARROW:
      uif_action = SEND_LCD_LIGHTER;
      uif_no_repeats = CLR_SEMAPHORE; /* Allow repeat key presses */
      break;

   case KEY_LEFT_ARROW:
      hw_attenuator(HWE_ATT_BOTH,HWE_DECR_ATT);
      uif_no_repeats = CLR_SEMAPHORE; /* Allow repeat key presses */
      break;
   case KEY_RIGHT_ARROW:
      hw_attenuator(HWE_ATT_BOTH,HWE_INC_ATT);
      uif_no_repeats = CLR_SEMAPHORE; /* Allow repeat key presses */
      break;

   case KEY_DISPLAY:
      /* Ensure screen functioning */
      uife_screen_state = UIFE_SCREEN_ACTIVE;
      break;

   case KEY_TEST:
      uif_action = SET_MODE_TEST;
      break;

   case KEY_OPTIONS:
      uif_action = SET_MODE_OPTS;
      break;

   case KEY_RESTART:
      uif_action = CLEAR_DRM_BUFFER;
      break;

   case KEY_REVIEW:
   case KEY_EXT_EVENT:  /* Treat same as REVIEW for time being */
      uif_action = SET_MODE_REVIEW;
      break;

   case KEY_REPEAT1:
      repeat1(last_key_pressed, mode_display);
      break;
   }
 } /*end mode_display*/

/*;*<*>********************************************************
  * mode_review
  *
  * A key has been pressed while in the UIFE_REVIEW mode.
  *
  **start*/
 void mode_review(enum key_pressed_enum key)
 {
   switch (key) {
   case KEY_UP_ARROW:
      /* KEY_UP_ARROW pressed in modes UIFE_REVIEW & UIFE_REVIEW_PLAY */
      uife_screen_state = UIFE_SCREEN_ACTIVE;
      drm_data(DRMC_FETCH_FRWD_TUPL_30_SEC,_ModuleUif_,(char *)&review_data);
      uif_no_repeats = CLR_SEMAPHORE; /* Allow repeat key presses */
      break;

   case KEY_DOWN_ARROW:
      /* KEY_DOWN_ARROW pressed in modes UIFE_REVIEW & UIFE_REVIEW_PLAY */
      uife_screen_state = UIFE_SCREEN_ACTIVE;
      drm_data(DRMC_FETCH_BACK_TUPL_30_SEC,_ModuleUif_,(char *)&review_data);
      uif_no_repeats = CLR_SEMAPHORE; /* Allow repeat key presses */
      break;

   case KEY_LEFT_ARROW:
      uife_screen_state = UIFE_SCREEN_ACTIVE;
      drm_data(DRMC_FETCH_BACK_TUPL,_ModuleUif_,(char *)&review_data);
      uif_no_repeats = CLR_SEMAPHORE; /* Allow repeat key presses */
      set_key_repeat_times(HWE_KEY_TIME_0_5SEC,KEY_REPEAT_DEF_TIME2);
      break;

   case KEY_RIGHT_ARROW:
      uife_screen_state = UIFE_SCREEN_ACTIVE;
      drm_data(DRMC_FETCH_FRWD_TUPL,_ModuleUif_,(char *)&review_data);
      uif_no_repeats = CLR_SEMAPHORE; /* Allow repeat key presses */
      set_key_repeat_times(HWE_KEY_TIME_0_5SEC,KEY_REPEAT_DEF_TIME2);
      break;

   case KEY_DISPLAY:
      /* DISPLAY key pressed while in UIFE_REVIEW & UIFE_REVIEW_PLAY modes
       * See mode_review_play() KEY_DISPLAY section.
       */
   case KEY_EXT_EVENT:  /* Treat same as DISPLAY for time being */
      uife_mode = UIFE_DISPLAY;
      uif_action = SET_MODE_REVIEW_OFF;
      break;

   case KEY_TEST:
      /* Set mode to review play*/
      uif_action = SET_MODE_REVIEW_PLAY;
      break;

   case KEY_OPTIONS:
      common_uif_actions(SET_MODE_REVIEW_OFF);
      uif_action = SET_MODE_OPTS;
      break;

   case KEY_RESTART:
      /* KEY_RESTART pressed in modes UIFE_REVIEW & UIFE_REVIEW_PLAY */
      uife_screen_state = UIFE_SCREEN_ACTIVE;
      drm_data(DRMC_1ST_TUPL,_ModuleUif_,(char *)&review_data);
      drm_data(DRMC_FETCH_BACK_TUPL,_ModuleUif_,(char *)&review_data);
      drm_data(DRMC_FETCH_FRWD_TUPL,_ModuleUif_,(char *)&review_data);
      break;

   case KEY_REVIEW:
      uife_screen_state = UIFE_SCREEN_ACTIVE;
      drm_data(DRMC_CURRENT_TUPL,_ModuleUif_,(char *)&review_data);
      break;

   case KEY_REPEAT1:
      repeat1(last_key_pressed, mode_review);
      break;
   }

 } /*end mode_review*/

/*;*<*>********************************************************
  * mode_review_play
  *
  * A key has been pressed while in the UIFE_REVIEW_PLAY mode.
  *
  **start*/
 void mode_review_play(enum key_pressed_enum key)
 {

   switch (key) {
   case KEY_UP_ARROW:
   case KEY_DOWN_ARROW:
   case KEY_RESTART:
      mode_review(key);
      break;

   case KEY_LEFT_ARROW:
   /****not implemented yet ***
      review_play_decr();
      break;
   */
   case KEY_RIGHT_ARROW:
   /****not implemented yet ***
      review_play_incr();
      break;
   */
   case KEY_EXT_EVENT:
   case KEY_TEST:
   case KEY_OPTIONS:
   case KEY_REVIEW:
      /* Switch back to straight review mode */
      uif_action = SET_MODE_REVIEW_FROM_PLAY;
      break;

   case KEY_DISPLAY:
      common_uif_actions(SET_MODE_REVIEW_FROM_PLAY);
      mode_review(KEY_DISPLAY);
      break;

   case KEY_REPEAT1:
      repeat1(last_key_pressed, mode_review_play);
      /* Repeats allowed for KEY_UP_ARROW & KEY_DOWN_ARROW */
      break;
   }

 } /*end mode_review_play*/

/*;*<*>********************************************************
  *  mode_opts
  *
  * A key has been pressed while in the UIFE_OPTS mode.
  *
  **start*/
 void mode_opts(enum key_pressed_enum key)
 {
 enum UIFE_OPTS_RET result = UIFC_NO_ACTION;

   switch (key) {
   case KEY_UP_ARROW:
      result = uif_opts(UIFE_OPTS_UP_ITEM);
      uif_no_repeats = CLR_SEMAPHORE; /* Allow repeat key presses */
      break;

   case KEY_DOWN_ARROW:
      result = uif_opts(UIFE_OPTS_DOWN_ITEM);
      uif_no_repeats = CLR_SEMAPHORE; /* Allow repeat key presses */
      break;

   case KEY_LEFT_ARROW:
   case KEY_RIGHT_ARROW:
   case KEY_RESTART:
      result = uif_opts_entry(key);
      break;

   case KEY_DISPLAY:
      uif_action = (SET_MODE_OPTS_OFF | SET_MODE_DISPLAY);
      break;

/*   case KEY_TEST:
      break;
*/
   case KEY_OPTIONS:
      result = uif_opts(UIFE_OPTS_ACTION);
      break;

/*   case KEY_REVIEW:
      break;
*/
   case KEY_REPEAT1:
      repeat1(last_key_pressed, mode_opts);
      break;

   }
   if (result != UIFC_NO_ACTION ) {
      uif_action = (SET_MODE_OPTS_OFF | SET_MODE_DISPLAY);
   }

 } /*end mode_opts*/

/*;*<*>********************************************************
  * mode_test(key);
  *
  * A key has been pressed while in the UIFE_TEST mode.
  *
  **start*/
 void mode_test(enum key_pressed_enum key)
 {

   switch (key) {
   case KEY_UP_ARROW:
      uif_action = SEND_LCD_DARKER;
      uif_no_repeats = CLR_SEMAPHORE; /* Allow repeat key presses */
      break;
   case KEY_DOWN_ARROW:
      uif_action = SEND_LCD_LIGHTER;
      uif_no_repeats = CLR_SEMAPHORE; /* Allow repeat key presses */
      break;
   case KEY_LEFT_ARROW:
      uif_action = SET_MODE_TEST;
      break;
   case KEY_RIGHT_ARROW:
      uif_action = SET_MODE_TEST_S;
      break;
   case KEY_DISPLAY:
      uif_action = (SET_TEST_OFF | SET_MODE_DISPLAY);
      break;
   case KEY_TEST:
      /* Already in this mode -
       * Don't do it again as it will save wrong montage state
       */
      /* uif_action = SET_MODE_TEST; */
      break;
   case KEY_OPTIONS:
      break;
   case KEY_RESTART:
      uif_action = CLEAR_DRM_BUFFER;
      /*uif_action = CLEAR_DRM_BUFFER; /* Sorts out funny bug */
      break;
   case KEY_REVIEW:
      break;

   case KEY_REPEAT1:
      repeat1(last_key_pressed, mode_test);
      break;
   }

 } /*end mode_test*/

/*;*<*>********************************************************
  * uif_set_mode_demo
  *
  * This function is called when in the OPTIONS/ACTION menu
  * to set the display mode to demonstrations
  *
  **start*/
 void uif_set_mode_demo(void)
 {
  Uif_act actions;
     actions = (SET_MODE_DEMO | SET_MODE_OPTS_OFF);
     common_uif_actions(actions);

 } /*end uif_set_mode_demo*/

/*;*<*>********************************************************
  * mode_demo;
  *
  * A key has been pressed while in the UIFE_DEMO mode.
  *
  **start*/
 void mode_demo(enum key_pressed_enum key)
 {

   switch (key) {
   case KEY_UP_ARROW:
      uif_action = SEND_LCD_DARKER;
      uif_no_repeats = CLR_SEMAPHORE; /* Allow repeat key presses */
      break;

   case KEY_DOWN_ARROW:
      uif_action = SEND_LCD_LIGHTER;
      uif_no_repeats = CLR_SEMAPHORE; /* Allow repeat key presses */
      break;

   case KEY_LEFT_ARROW:
      /* change display demonstration */
      uif_display_pattern(UIFE_DEMO_BACK);
      uif_no_repeats = CLR_SEMAPHORE; /* Allow repeat key presses */
      break;

   case KEY_RIGHT_ARROW:
      /* change display demonstration */
      uif_display_pattern(UIFE_DEMO_FORWARD);
      uif_no_repeats = CLR_SEMAPHORE; /* Allow repeat key presses */
      break;

   case KEY_DISPLAY:
      uif_action = (SET_MODE_DEMO_OFF | SET_MODE_DISPLAY);
      /*uife_mode = UIFE_DISPLAY;*/

     break;

   case KEY_TEST:
      break;

   case KEY_OPTIONS:
      break;

   case KEY_RESTART:
      break;

   case KEY_REVIEW:
      break;

   case KEY_REPEAT1:
      repeat1(last_key_pressed, mode_test);
      break;

   }

 } /*end mode_test*/

/*;*<*>********************************************************
  *  common_uif_actions(actions)
  *
  * This function implements common actions between all the functions
  * in this module.
  *
  * See the definition for 'Uif_act'
  *
  **start*/
 void common_uif_actions(Uif_act actions_in)
 {
 Initt_system_state ssp;
 static hwt_fec_status fe_status; /* Temporary location - used by
                                    * SET_MODE_TEST/OFF
                                    */
 register Uif_act actions = actions_in;

   if (actions & SET_MODE_DISPLAY) {
      uife_mode = UIFE_DISPLAY;
      uife_screen_state = UIFE_SCREEN_ACTIVE;
   }

   if ( actions & SET_MODE_OPTS) {
      uife_mode = UIFE_OPTS;
      uife_screen_state = UIFE_SCREEN_FROZEN;
      dfm_control(DFMC_FILTERING_OFF); /* Disable filtering routine as not used*/
      ldd_clr(LDDE_CLR_GRAPHICS);
      ssp_action(SSP_TX_RT_DIS);  /* Disable Uart Comms */
      drm_data(DRMC_PAUSE_ON,_ModuleUif_,(char *)&zero_data);
      uif_opts(UIFE_OPTS_NEW);
   }

   if ( actions & SET_MODE_OPTS_OFF) {
      uif_opts(UIFE_OPTS_END);
      uife_screen_state = UIFE_SCREEN_ACTIVE;
      dfm_control(DFMC_FILTERING_ON); /* Enable filtering routine */
      ssp.init_type = WARM;
      dpl_initialisation(&ssp);
      ssp_action(SSP_TX_RT_EN);  /* Enable Uart Comms */
      drm_data(DRMC_PAUSE_OFF,_ModuleUif_,(char *)&zero_data);
   }
   if (actions & SET_MODE_TEST){
       /* 
        *A request to switch to test mode has been initiated
        * Switch the contacts/test in as default
        */
       uife_mode = UIFE_TEST;
       test_mode = UIFE_TEST_CONTACTS;
       hw_fec_status(&fe_status);  /* Get current montage setting */
       hw_test_freq(TEST_FREQ_ENABLE);
       hw_front_end_mode(HW_FE_CNTCT_TEST,0,0);
       mfd_control(MFDC_TEST_MODE,(int)HWE_TEST);

   }
   if (actions & SET_MODE_TEST_S){
       /* 
        *A request to switch to test mode has been initiated
        *Implement a self test, switching the contacts out.
        */
       uife_mode = UIFE_TEST;
       test_mode = UIFE_TEST_SELF;
       hw_test_freq(TEST_FREQ_ENABLE);
       hw_front_end_mode(HW_FE_SELF_TEST,0,0);
       mfd_control(MFDC_TEST_MODE,(int)HWE_TEST_S);

   }
   if (actions & SET_TEST_OFF) {
       hw_test_freq(TEST_FREQ_DISABLE);
       /* There is a bug without act1
        * If just act2 is used, then when self test is initiated
        * and DISPLAY key is pressed, the front end isn't re-enabled
        * somehow.
        * It may be possible to combine act1 & act2 at some stage
        */
       hw_front_end_mode(HW_FE_CNTCT_TEST,0,0); /*act1*/
       hw_front_end_mode(fe_status.montage,HWE_ATT_BOTH,HWE_REFRESH);/*act2*/
       mfd_control(MFDC_TEST_MODE,(int)HWE_BOTTOM_NULL);
   }

   if (actions & SEND_LCD_DARKER) {
#define LCD_INCR 0x20
     hw_lcd_contrast_set(LCD_INCR);
   }
   if (actions & SEND_LCD_LIGHTER) {
#define LCD_DECR -0x20
     hw_lcd_contrast_set(LCD_DECR);
   }

   if (actions & ((int)DELAY_AND_READ_DATA)) {
      /*
       * This is for when there is any sudden change in data
       * It allows a waiting period for the changes to
       * propogate into the filters, and then clears the
       * data from the filters. No updates is performed.
       *
       * Note: Just delaying and then reading filters does not work
       * very well. Seperate timers needs to be enabled to turn off the
       * output of various filtes for different times.
       */
      delay_ms(200);
      mfd_acquire(NUMBER_FILTERS); /* Zero filter outputs*/
   }

   if ( actions & SET_MODE_DEMO) {
      uife_mode = UIFE_DEMO;
      uif_display_pattern(UIFE_DEMO_START);
      drm_data(DRMC_PAUSE_ON,_ModuleUif_,(char *)&review_data);
   }
   if ( actions & SET_MODE_DEMO_OFF) {
      uif_display_pattern(UIFE_DEMO_FINISH);
      drm_data(DRMC_PAUSE_OFF,_ModuleUif_,(char *)&review_data);
   }

   if ( actions & SET_MODE_REVIEW) {
       /* Switching to review mode
        *    - change mode
        *    - freeze screen. This is a little trick, as the user
        *      has possibly pressed REVIEW to freeze what s/he is
        *      seeing on the screen. Must be unfrozen later.
        *    - initialise drm to current record (but doesn't get
        *      a new one yet)
        *    - change output of filters to be from review_data
        */
       uife_mode = UIFE_REVIEW;
       /*uife_screen_state = UIFE_SCREEN_FROZEN; */
       drm_data(DRMC_PAUSE_ON,_ModuleUif_,(char *)&review_data);
       drm_data(DRMC_CURRENT_TUPL,_ModuleUif_,(char *)&review_data);
       MfdSetInput(_ModuleDplFiltered,&review_data);
       if (DataToSsp == _FilteredData_)
       {/*In mode to update data */
	       MfdSetInput(_ModuleSspFiltered,&review_data);
       }
   }

   if ( actions & SET_MODE_REVIEW_OFF) {
       /* Clean up after review is turned off
        *   - enable screen display incase it hasn't happened elsewhere
        *   - turn filter outputs back to filtering routines
        */
       uife_screen_state = UIFE_SCREEN_ACTIVE;
       MfdSetInput(_ModuleDplFiltered,NULL);
       if (DataToSsp == _FilteredData_)
       {/*In mode to update data */
	       MfdSetInput(_ModuleSspFiltered,NULL);
       }
       drm_data(DRMC_PAUSE_OFF,_ModuleUif_,(char *)&review_data);
   }
   if ( actions & SET_MODE_REVIEW_PLAY) {
       /* Switching to playing review mode from UIFE_REVIEW
        *    - change mode to UIFE_REVIEW_PLAY
        *    - switch off filtering
        */
       uife_mode = UIFE_REVIEW_PLAY;
       dfm_control(DFMC_FILTERING_OFF); /* Disable filtering routine as
                                         * could use CPU time in display*/
   }

   if ( actions & SET_MODE_REVIEW_FROM_PLAY) {
       /* Clean up after review play is turned off
        *   - set mode back to UIFE_REVIEW
        *   - turn filtering back on
        */
       uife_mode = UIFE_REVIEW;
       dfm_control(DFMC_FILTERING_ON); /* Enable filtering routine */
   }


   if ( actions & CLEAR_DRM_BUFFER) {
      drm_data(DRMC_RESET,_ModuleUif_,(char *)&review_data);
      mfd_control(MFDC_TIME_STAMP,0); /* Reset to 0        */
      mfd_control(MFDC_PAUSE,0); /* Clear pause indication */

   }

 } /*end common_uif_actions*/

/*;*<*>********************************************************
  * uif_misc
  *
  * This procedure implements miscellaneous actions relating
  * to the user interface. These are
  *      - battery monitoring
  **start*/
 void uif_misc(void)
 {
#define ADC_THREE_VOLTS ((0xffC0/5)*3)
#define BATTERY_GOOD ADC_THREE_VOLTS
#define BATTERY_REPORT_OK 0

     if (ad_result[ADC_BAT_FAIL] > BATTERY_GOOD) { 
        /* The battery is OK */
        mfd_control(MFDC_BAT_FAIL, BATTERY_REPORT_OK);
     } else {
        mfd_control(MFDC_BAT_FAIL, MMS_BATTERY_MASK);
     }
     /* Indicate another measurement should be made
      *  - make sure its atomic, as measurements are made at
      *    interrupt level
      */
     disable();
     ad_other_channels |= ADC_BAT_FAIL_MASK;
     enable();

 } /*end uif_misc*/

/*;*<*>********************************************************
  * set_key_repeat_times
  *
  **start*/
/* void set_key_repeat_times(timer1,timer2)
 {

 } /*end set_key_repeat_timesxxx*/

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


