/* in_ram.c
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
 * This file defines the filter RAM storage that is to 
 * use the internal RAM.
 *
 */
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif

#include "general.h"
#include "iir.h"

#pragma nolistinclude

#define DO_LP_PROCESSING

#define FILTER_BIQUAD
/*******************************************************
 *  Low Pass Filters
 */
#define F640_LPI_EQUATES   /* 256Hz */
#include "640_LPI.hf"

#define F320_LPI_EQUATES  /* 128Hz */
#include "320_LPI.hf"

#define F160_LPI_EQUATES  /* 64Hz */
#include "160_LPI.hf"

/*******************************************************
 *  128Hz Sampling Filters
 */
#define F340_00T1_EQUATES  /* 128Hz */
#include "340_00T1.hf"

#define F270_70B1_EQUATES  /* 128Hz */
#include "270_70B1.hf"

#define F215_55B1_EQUATES  /* 128Hz */
#include "215_55B1.hf"

/*******************************************************
 *  64Hz Sampling Filters
 */

#define F170_45B1_EQUATES   /* 64Hz */
#include "170_45B1.hf"

#define F135_35B1_EQUATES   /* 64Hz */
#include "135_35B1.hf"

#define F115_20B1_EQUATES   /* 64Hz */
#include "115_20B1.hf"

#define F095_20B1_EQUATES   /* 64Hz */
#include "095_20B1.hf"

#define F079_16B1_EQUATES  /* 64Hz */
#include "079_16B1.hf"









