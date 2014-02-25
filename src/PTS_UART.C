/*
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
  pts_uart.c

  This module contains the buffer that is used in
  ssp_uart.c

  It is used by the PTS which places
  special requirements on where the ram is located.

  It is in a seperate module to enable the linker to place
  the PTS table in internal ram.

*/
#ifdef IC96
#include <h\ncntl196.h>
#pragma regconserve
#pragma nolistinclude
#endif
/*#include <80C196.h>*/
/*#include <stdio.h>
#include <math.h>
#include <stdlib.h>
*/
/*#include "general.h"
#include "ssp.h" */
#include "pts.h"
/*#include "hw.h" */

/**************************************************************
 * Internal storage to this module 
 *
 *
 * Definition table for transmitting on the UART.
 * Must be located in RAM on an adress boundary divisible by 8
 */
 Pts_single_transfer pts_ti_table;
 Pts_single_transfer *const pts_ti_vector=&pts_ti_table;
#pragma locate(pts_ti_vector=0x2050)
