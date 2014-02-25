/*  uif_revw.c
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
Algorithms

Internal Data

*/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
/*#include <stdio.h>*/
/*#include <math.h>*/
/*#include <stdlib.h>*/

#include "general.h"
#include "hw.h"
#include "uif.h"
#include "drm.h"
/**************************************************************
 * Constants 
 */

/**************************************************************
 * Externally defined 
 */
extern enum UIFE_SCREEN_STATE uife_screen_state;
extern Dplt_data review_data;

/**************************************************************
 * Internal storage to this module 
 */
 /* review_play counters */
   int review_play_invocations, review_play_speed;
#define INIT_REVIEW_PLAY 3

/**************************************************************
 * Internal prototypes 
 */

/*;*<*>********************************************************
 * uif_revw_initialisation
 **start*/
void uif_revw_initialisation(Initt_system_state *ssp)
 {

    review_play_invocations = 0;
    review_play_speed = INIT_REVIEW_PLAY;

 } /*end uif_revw_initialisation*/

/*;*<*>********************************************************
  * review_play
  *
  * This is invoked under the following conditions
  *   uif_poll() is invoked                   AND
  *      UIFE_MODE == UIFE_REVIEW_PLAY        AND
  *      and no key has been pressed
  *
  * It fetches forward the next tupl in the internal memory
  **start*/
void review_play_action(void)
 {
     if (review_play_invocations++ < review_play_speed)
        return;  /* Don't run this module */

     review_play_invocations = 0;

     /* Play another tupl */
     uife_screen_state = UIFE_SCREEN_ACTIVE;
     drm_data(DRMC_FETCH_FRWD_TUPL,_ModuleUif_,(char *)&review_data);

 } /*end review_play*/

/*;*<*>********************************************************
 * review_play_incr
 *
 * This function increases the rate of reviewing
 **start*/
void review_play_incr(void)
 {
   return; /* Doesn't work properly - leave out */
   if ( --review_play_speed < 0) {
       review_play_speed = 0;
   }
 } /*end review_play_incr*/

/*;*<*>********************************************************
 * review_play_decr
 *
 * This function decreases the rate of reviewing
 **start*/
void review_play_decr(void)
 {

   return; /* Doesn't work properly - leave out */
#define SLOWEST_REVIEW_RATE 16
   if ( ++review_play_speed > SLOWEST_REVIEW_RATE) {
       review_play_speed = SLOWEST_REVIEW_RATE;
   }

 } /*end review_play_decr*/

/*;*<*>********************************************************
 * xyz
 **start*/
/* xyz()
 {

 } /*end xyz*/


