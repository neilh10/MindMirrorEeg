/*  dbg_ptsr.c
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

Algorithms

Internal Data
*/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif

#include "general.h"
#include "dbg.h"

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
  * dbg_putstr
  *
  * This function provides one line that can be printed to either
  * the PC screen or the dbg port on the target system.
  *
  * The only system resource it uses is putchar(), and the function
  * only returns when the string has been printed
  *
  **start*/
 void dbg_putstr(char *string)
 {
   while (*string != 0) {
      putchar(*string++);
   }

 } /*end dbg_putstr*/

/*;*<*>********************************************************
 * 
 **start*/
/* ()
 {

 } /*end*/
