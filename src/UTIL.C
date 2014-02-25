/*  util.c  Utility functions
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
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "general.h"
#include "util.h"

/**************************************************************
 * Constants 
 */

/**************************************************************
 * Externally defined 
 */

/**************************************************************
 * Internal storage to this module 
 */

/**************************************************************
 * Internal prototypes 
 */

/*;*<*>********************************************************
  * delay
  *
  * routine to cause a delay by the action of calling it
  **start*/
 void  delay(int count)
 {
 register int temp=0;

    do {} while ( temp++ <= count);

 } /*end delay*/

/*;*<*>********************************************************
  * delay_ms
  *
  * This routine causes a specified delay.
  *
  * count 0 to 0xffff in mS
  **start*/
 void delay_ms(int count)
 {
   register int lp;
   for (lp=0; lp < count; lp++) {
#if defined(CLOCKC20)
      delay(366); /* 293 is a guess calculated from 117*20/6.4 */

#elif defined(CLOCKC16)
      delay(293); /* 293 is a guess calculated from 117*16/6.4 */
#elif defined(CLOCKC12)
      delay(219); /* 219 is a guess calculated from 117*12/6.4 */
#elif defined(CLOCKC6_4)
      delay(117);
#else
      error - no valid SYSCLOCK
#endif
   }
 } /*end delay_ms */


/*;*<*>********************************************************
 * 
 **start*/
/* ()
 {

 } /*end*/
