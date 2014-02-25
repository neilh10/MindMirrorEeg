/* hw_key.c
  
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
 This module is responsibile for reporting keypress on the keypad.

 Entry points are
    hw_key_init
    hw_keypad_poll

*/
#ifdef IC96
#include <h\cntl196.h>
#include <80C196.h>
#pragma nolistinclude
#endif
/*#include <stdio.h>
#include <math.h>
#include <stdlib.h>
*/
#include "general.h"
#include "hw.h"

/**************************************************************
 * Constants 
 */
/* Keypad defaults */
#define NOKEYS_PRESSED 0
#define DEFAULT_KEYPAD_OUTPUT 0x7
#define INIT_STROBE_VALUE 0x01
#define END_OF_STROBE (INIT_STROBE_VALUE << 2)


 enum keypad_state_enum {NO_KEY_PRESSED, KEY_PRESSED, KEY_DETECTED};


/**************************************************************
 * Externally defined 
 */
 extern unsigned char ioport1_mirror, ioport2_mirror;

/**************************************************************
 * Internal storage to this module 
 */
 enum keypad_state_enum keypad_state; /* State of keypad */
 enum key_pressed_enum key_pressed; /* Last key_pressed on keypad */
 /* key_event
  * When a keypad event is detected this variable is updated by
  * hw_keypad_fsm() in key_event.key  The event is polled and acknowledged by
  * hw_keypad_poll().
  *
  * For every new key press recognized key_event.new_event is SET_SEMAPHORE.
  * 
  */
 hwt_key_status key_event;

 /* Sempahores to control access to the sending of
  * key repeat events
  */
 struct {
   Semaphore event1;
   Semaphore event2;
 } key_repeat;
 
/**************************************************************
 * Internal prototypes 
 */
 void keypad_set(int set);
 unsigned int keypad_stat(void);
 enum key_pressed_enum translate_key_strobe(unsigned int strobe_value);
 enum key_pressed_enum get_key(void);

/*;*<*>********************************************************
 * hw_key_init
 **start*/
 void hw_key_init(Initt_system_state *ssp)
 {
   /*
    * Initialise keypad variables
    */
    keypad_state = NO_KEY_PRESSED;
    key_event.key = key_pressed = NO_KEY;
    key_event.new_event = CLR_SEMAPHORE;
    /*key_event.time_status = KEY_TIME_NONE; */
    key_event.timer1 = key_event.timer2 = 0;
    keypad_set(DEFAULT_KEYPAD_OUTPUT);


 } /*end hw_key_init*/

/*;*<*>********************************************************
 * hw_keypad_poll
 *
 * This routine returns any new events in the keypad
 **start*/
 enum key_pressed_enum hw_keypad_poll(enum hwt_key_time time1, enum hwt_key_time time2)
 {
   enum key_pressed_enum key=KEY_ACTIVE;

   if (key_event.new_event == SET_SEMAPHORE) {
      hw_keypad_event_ack();  /* Acknowledge new key event */
      key=key_event.key;
      key_repeat.event1 = key_repeat.event2 = CLR_SEMAPHORE;

   } else if ( key_event.key == NO_KEY ){
      key = NO_KEY;
   } else {
      /* Check time values against key_event.timers  */
      if ( (int) key_event.timer1 > (int) time1)
      {
         key = KEY_REPEAT1;
         key_event.timer1 = 0;

/*      } else if ( ( (uint) key_event.timer2 > (uint) time2) &
                  ( key_repeat.event2 == CLR_SEMAPHORE )
                )

      {
         key = KEY_REPEAT2;
         key_repeat.event2 = SET_SEMAPHORE;

*/
      }
   }

   return key;

 } /*end hw_keypad_poll*/

/*;*<*>********************************************************
 * hw_keypad_status
 *
 * This routine returns the status of the key pad.
 **start*/
 hwt_key_status *hw_keypad_status(void)
 {
    return &key_event;
 } /*end hw_keypad_status*/

/*;*<*>********************************************************
 * hw_keypad_event_ack
 *
 * This routine accepts the acknowledgement of a new_event.
 *
 **start*/
 void hw_keypad_event_ack(void)
 {
    key_event.new_event = CLR_SEMAPHORE;
 } /*end hw_keypad_event_ack*/

/*;*<*>********************************************************
  * hw_keypad_fsm
  *
  * This function determines if a key has been pressed, and returns
  * the key name as an event in 'key_event'. It must be called periodically
  * - in this case from interrupt.
  *
  * The ON/OFF key is not polled as part of this function.
  *
  * When this function finds a key pressed, it debounces the keypad,
  * (waiting for a second call to this function)
  * before returning the key name. The key name is only returned once
  * per the key being pressed.
  *
  * Once a key has been detected as pressed, then it must be released
  * before any other key is recognized. Hence, two keys can't be pressed
  * at the same time
  *
  * return:
  *   Values returned are defined in key_pressed_enum
  *
  * Algorithm  
  *
  *  Initialisation   key pressed         same key pressed
  *      \|/      +====>====>====+       +=====>======>==+
  *       |       |             \|/     /|\             \|/
  *    +--+-------+---+        +-+-------+-+       +-----+------+
  *    |NO_KEY_PRESSED|        |KEY_PRESSED|       |KEY_DETECTED|
  *    +------+---+---+        +-+------+--+       +-----+------+
  *           |   |              |      |                |
  *          /|\ /|\  different \|/    \|/              \|/
  *           |   |   key pressed|      |                |
  *           |   +=====<========+      | no key pressed |
  *           +===<=======<======<======+=====<======<===+
  *
  **start*/  
 void hw_keypad_fsm(void)
 {
  register enum key_pressed_enum this_key_press;

  this_key_press = get_key();

  switch (keypad_state) {
  case NO_KEY_PRESSED:
     key_event.timer1 = key_event.timer2 = 0; /* Reset timer */
     if ((this_key_press == NO_KEY) ||
         (this_key_press == UNDEFINED_KEY)) {
        key_event.key = NO_KEY; /* Set to known state */
        return;
     }
     keypad_state = KEY_PRESSED;
     key_pressed = this_key_press;
     return;

  case KEY_PRESSED:
     if (this_key_press == key_pressed) {
        /*
         * A keypad event has been detected and debounced.
         * Generate an event
         */
        keypad_state = KEY_DETECTED;
        key_event.key = this_key_press;
        key_event.new_event = SET_SEMAPHORE;

        return;
     } else {
        keypad_state = NO_KEY_PRESSED;
        return;
     }
     /* Should never get here*/

  case KEY_DETECTED:
     if (this_key_press == NO_KEY) {
        keypad_state = NO_KEY_PRESSED;
        key_event.key = NO_KEY; /* Set to known state */
     } else {
        key_event.timer1++;
        key_event.timer2++;
        key_event.key = this_key_press;
     }
     return;

  default:
     /* Should never get here .. but if it does */
     keypad_state = NO_KEY_PRESSED;
     key_event.key = NO_KEY; /* Set to known state */

  };
  return;

 } /*end hw_keypad_fsm*/

/*;*<*>********************************************************
  * get_key
  *
  * This function determines if a key has been pressed and
  * which one it is.
  *
  * For 99.99999% of the time, no key will have been pressed,
  * so it first tests for any active keys.
  *
  * The keypad is a
  *
  *                K33    K32    K31 -- Y3
  *                K23    K22    K21 -- Y2 ( polled )
  *                K13    K12    K11 -- Y1
  *                 |      |      |
  *                 X3     X2     X1
  *                  ( strobed )
  * where
  *    Kyx is the key. 
  *    x is the strobed value
  *    y is the input polled
  * AND
  *   the external event key
  *
  * Normally for no keys pressed, the outputs Y1,2,3 will be low or 0.
  * X1,2,3 are alternatively strobed (in keypad_set), and when a 
  * key is pressed the appropiate output goes high.
  *
  * Assumptions:
  * On entering the function, it is assumed that the
  * keypad has been left with keypad_set(0x7).
  *
  *
  **start*/
 enum key_pressed_enum get_key(void)
{
register int strobe;
register enum key_pressed_enum key = NO_KEY;
unsigned int new_poll;

/*   char buf[20]; */

   if (hw_event_key() != 0) {
      /*
       * event key has been pressed
       */
      return KEY_EXT_EVENT;
   }

   if (keypad_stat() == NOKEYS_PRESSED) {
      /*
       * No Key pressed ......
       */
      return;
   }
   /*
    * A key has been pressed, find it
    */
   for (strobe = INIT_STROBE_VALUE;
        strobe <= END_OF_STROBE;
        strobe <<= 1)
   {
      /*
       * Setup hardware for strobe
       */
      keypad_set(strobe);

      /*
       * Check for a key press
       */
      new_poll = keypad_stat();
      if (new_poll != NOKEYS_PRESSED) {
         /*
          * A key has been found.
          * Translate it and abort out of this loop
          */
         key = translate_key_strobe(new_poll|strobe);

/*   sprintf(buf,"  %X %X",(new_poll|strobe),(int)key);
   ldd_line(14,buf);
*/
         break; /* exit for(){} loop */
      }
   }
   keypad_set(DEFAULT_KEYPAD_OUTPUT);
   return key;

 } /*end get_key*/

/*;*<*>********************************************************
  * keypad_set
  *
  * This function sets 3 pins on the keypad.
  *
  * The 3 bits are defined in bits B0->2 of 'set'
  *
  * Hardware Descripition
  *   The hardware signals occur in port1 as follows
  *  7   6   5   4   3   2   1   0
  *                      X3  X2  X1
  * Hardware Note:
  *   The 'set' values are inverted when they are written to the hardware.
  **start*/
void keypad_set(int set)
 {
#define PORT1_KEY_MASK 0x7
/*#define INPUT_KEY_MASK 0x7 */

#ifndef SIMULATING
  ioport1 = 
#endif /* SIMULATING */
  ioport1_mirror = ((ioport1_mirror & (~PORT1_KEY_MASK))
                              | ((~set) & PORT1_KEY_MASK));
 } /*end keypad_set*/

/*;*<*>********************************************************
  * keypad_stat
  *
  * This function returns with the status of the keypad.
  *
  *
  * Returns
  *     15 14 .. 7  6  5  4  3  2  1  0
  *      0  0 .. 0  Y3 Y2 Y1 0  0  0  0
  *
  * where
  * Y1->Y3 are the outputs
  *
  * Hardware Description:
  *   The hardware signals occur in port2 as follows
  * The input is in port2 as follows
  *  7   6   5   4   3   2   1   0
  *      Y3      Y2  Y1
  *
  *  and are strobed low. Hence they are inverted before the routine
  *  returns.
  *
  * Debugging information
  *  For X1 low expect i/p x1x10xxx  or 0x50 returns 0x10
  *      X2 low expect i/p x1x01xxx  or 0x48 returns 0x20
  *      X3 low expect i/p x0x11xxx  or 0x18 returns 0x40
  **start*/
  unsigned int keypad_stat()
 {
    register int temp,temp1;

    temp = ioport2;
    /*
     * Change the value stored in temp to 
     *
     *  7   6   5   4   3   2   1   0
     *     ~Y3 ~Y2  ~Y1
     */
#define Y2Y1_MASK 0x30
#define Y3_MASK 0x40
#define KEYSTAT_MASK 0x70
    temp1 = temp <<1;
    temp1 &= Y2Y1_MASK;
    temp &= Y3_MASK;
    temp = temp | temp1;
    temp = ((~temp) & KEYSTAT_MASK);
    return temp;
    /*return  (KEYSTAT_MASK & (~((temp&Y2Y1_MASK) | ((temp>>1) & Y3_MASK))));
      */
 } /*end keypad_stat*/

/*;*<*>********************************************************
  * translate_key_strobe
  *
  * This function, takes the polled values and translates
  * it to a value from key_pressed_enum
  *
  * If more than one line is pressed then an UNDEFINED_KEY
  * is returned.
  *
  * Note: Key translation values seemed to be screwed up.
  *       The following was arrived at empirically  22-Mar-92 b001-1 hw
  **start*/
 /* strobe_value*/
#define STROBE_K33  0x44  /* K33 */
#define STROBE_K32  0x42  /* K32 */
#define STROBE_K31  0x41  /* K31 */
#define STROBE_K23  0x24  /* K23 */
#define STROBE_K22  0x22  /* K22 */
#define STROBE_K21  0x21  /* K21 */
#define STROBE_K13  0x14  /* K13 */
#define STROBE_K12  0x12  /* K12 */
#define STROBE_K11  0x11  /* K11 */
 enum key_pressed_enum translate_key_strobe(unsigned int strobe_value)
{

 switch (strobe_value){
   case STROBE_K33:
      return KEY_DISPLAY;

   case STROBE_K32:
      return KEY_UP_ARROW;

   case STROBE_K31:
      return KEY_TEST;

   case STROBE_K23:
      return KEY_LEFT_ARROW;

   case STROBE_K22:
      return KEY_OPTIONS;

   case STROBE_K21:
      return KEY_RIGHT_ARROW;

   case STROBE_K13:
      return KEY_RESTART;

   case STROBE_K12:
      return KEY_DOWN_ARROW;

   case STROBE_K11:
      return KEY_REVIEW;

   default:
      /*
       * If we get here, it is expected, that a double
       * key has been pressed.
       */
      return UNDEFINED_KEY;
   }
}/*end translate_key_strobe*/

/*;*<*>********************************************************
 * hw_event_key
 *
 * This function determines how long the event key has been pressed.
 *
 * The event key can be read 
 *           at bit 7(0x80), HI3 in hsi_status Fig 9-2 pg5-47
 *           IOC0.6 must be (set?) for HSI Fig 9.4 pg5-48
 * Returns
 *      0 if key open circuit or jack not inserted
 *    != 0 if key closed
 *
 **start*/
 int hw_event_key(void)
 {
   return ( (~(unsigned int)hsi_status) & EVENT_HSI_MASK);

 } /*end hw_event_key*/

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



