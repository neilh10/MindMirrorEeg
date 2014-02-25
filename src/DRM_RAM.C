/* data recorder ram storage module
  drm_ram.c
 
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

 Data is stored in ram as simple tupls of length sizeof(Dplt_data).
 Two tables maintain a description of the next avaiable byte -
 only one table is accurate and active at any time. 
 The inactive table is updated, and when
 the update has occured, it is commited by changing the active index byte.

 The physical ram is divided up into the following physical areas
      8K of general purpose ram
      8 * 16K banks of data storage ram

 The following is maintained in the general purpose ram
      housekeeping
      index1
      index2

The housekeeping area has the following functions
     power failure signature 
     active index indication

The index area stores
    the next free byte

Algorithms
   drm_ram_initialisation
   drm_ram_store
   drm_ram_get

Internal Data

Tests
 13/Sept/92 - In a standalone test (from start of schedule()), filled
 banked ram 615 times and checked by reading all banked ram.
 14/Sept/92 - Did 630 full filling banked ram, and then checking it
 being called from Scheduler() as part of multitasking environment.

*/
#ifdef IC96
#pragma extend /* To allow reentrant function */
#pragma noreentrant
#pragma regconserve /*(FSCOPE) */
#include <h\ncntl196.h>
#pragma nolistinclude
#endif
/*#include <stdio.h>*/
/*#include <math.h>*/
/*#include <stdlib.h>*/

#include "general.h"
#include "iir.h"
#include "hw.h"
#include "hw_mem.h"
#include "drm.h"
#include "mmstatus.h"
#include "mm_coms.h"
#include "mfd.h"
#include "dim.h"
#include "proto.h"

/**************************************************************
 * Constants 
 */
 typedef struct {unsigned int low, high;} UWordreg;
 typedef union {unsigned long dword; UWordreg word;} ULongword;

  /* This initialises the first record in the banked ram */
 const Dplt_data init_buf = {
  0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,HWE_BOTTOM_NULL,
  0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,HWE_BOTTOM_NULL,
  0x00,  /* Time stamp */
  MMS_DM_REVIEW   /* Status     */
 };
 const Dplt_data last_buf = {
  0,0x10,0,0x20,0,0x30,0,0x40,0,0x50,0,0x60,0,0x80,0,HWE_END_ATT,
  0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,HWE_END_ATT,
  0xffff,  /* Time stamp */
  MMS_DM_REVIEW   /* Status     */
 };

/**************************************************************
 * Externally defined 
 */
extern int last_stored_tupl_num;

/**************************************************************
 * Internal storage to this module 
 */

 typedef struct {
  /* IF the size of this is changed then the sizof mus be manually updated
   * in the locate( dir= , ram_bank_0=) */
    enum ram_bank_number bank_no;
    char *addrp;
    uint  tupl_no;  /* Number of this tupl - starting at 1 for 1st */
 } Index;

 struct Housekeeping {
  /* IF the size of this is changed then the sizof mus be manually updated
   * in the locate( dir= , ram_bank_0=) */
  int power_signature;
  int active_index;
 } housekeeping;

 struct Dir {
  /* IF the size of this is changed then the sizof mus be manually updated
   * in the locate( dir= , ram_bank_0=) */
   Index index[2];
 } dir;

 /* Define banked ram at right position */
 char ram_bank[BANKED_RAM_SIZE-1];
#if defined(REAL_TARGET) & !defined(SIMULATING)
#pragma locate(ram_bank=LOWER_BANKED_RAM_ADDR)
#endif /* REAL_TARGET */

/**************************************************************
 * Internal prototypes 
 */
 void store(char *p,uint buf_size);

/*;*<*>********************************************************
 * drm_ram_initialisation
 *
 * The initialisation section initialises for
 *       COLD
 *       WARM
 * and can also be asked to issue a REPORT on the state
 * of the data stored in ram.
 *
 *   Returns
 *         PASS/FAIL
 **start*/
 enum inite_return drm_ram_initialisation(Initt_system_state *ssp)
 {
#define BRAM_POWER_SIGNATURE 0xa55a

   switch (ssp->init_type) {
   case COLD:
      housekeeping.power_signature = BRAM_POWER_SIGNATURE;
      housekeeping.active_index = 0;
      dir.index[0].addrp = (char *) &ram_bank[0];
      dir.index[0].bank_no = RAM_BANK_0;
      dir.index[0].tupl_no = TUPL_START_NUMBER;
      drm_ram_store((char *)&init_buf, sizeof(Dplt_data));
      break;

   case WARM:
      break;

   case REPORT:
      if (housekeeping.power_signature != BRAM_POWER_SIGNATURE) {
         return FAIL;
      }
      break;
   }
   return PASS;

 } /*end drm_ram_initialisation*/

/*;*<*>********************************************************
 * drm_ram_info
 **start*/
/* drm_ram_info()
 {
     
 } /*end drm_ram_info*/

/*;*<*>********************************************************
 * drm_ram_store
 * 
 *  The tupl pointed at by *p of size 'size' is stored
 * in the banked ram.
 *
 *  Returns
 *     0 if no update is performed or else
 *       the tupl number of the tupl recorded
 *
 * Note:
 * 1) The IC96 compiler V2.1 is very sensitive to unsigned/signed long int,
 *   and doesn't appear to cast properly between them. Hence source
 *   should be unsigned, rather than trying to cast to it. Checkout the
 *   way it puts MAX_BANK_ADDR into a 'unsigned long'
 *
 * 2) The buf_size is assumed to be constant across all callings. 
 *    When the internal buffer fills up, it always stores the last
 *    value it was given. 
 **start*/
 int drm_ram_store(char *pdest,uint buf_size)
 {
 register enum ram_bank_number bank_no;
 register char *addrp;
 register char *p = pdest;
 register int size=buf_size; /* Use reg for speed */
 register int loop, active_index;
 register Index *indexp;
 int tupl_no;

   active_index = housekeeping.active_index;
   indexp = &dir.index[active_index];
   addrp = indexp->addrp;
   tupl_no = indexp->tupl_no;
   bank_no = indexp->bank_no;

   /* Setup hardware */
   hw_ram_bank_set(bank_no);

   /* Check for overflowing one bank into another */
   /* Note: Becarefull about adjusting the following compare */
   if ( ((uint)addrp + (uint)size) < (uint)addrp) {

      /**** Will overflow this bank ****/

      /* Check to see if in the last bank.
       * If so then ASSUMING constant size of incoming tupls
       * squash last tupl and write in its place instead.
       */
      if ( (int)bank_no >= ((int)RAM_BANK_END-1)) {
         /* This tupl will overflow the last bank
          * Reset addrp to record this last tupl.
          */
         addrp -= size; 
         tupl_no--;
        /* This means data captured is paused - so report it */
        mfd_control(MFDC_PAUSE,MMS_PAUSE_MASK );
      }
      for (loop = 0; loop < size; loop++) {
         *addrp++ = *p++;
         /* Check if bank boundary overflowed - assumes a 16 bit pointer */
         if (addrp == NULL) {

            /* Overflowed a bank boundary
             * change to next bank up.
             */
            if (((int)++bank_no) < (int)RAM_BANK_END) {
               hw_ram_bank_set(bank_no);
            } else {

               /* At the end of banked ram storage */
               return 0;  /***** EXIT ****/

            }
            addrp += (uint)&ram_bank[0]; /* Adjust pointer back into banked ram */

         }
      }

   } else {
      /* Fast Copy - all within this bank*/
      for (loop = 0; loop < size; loop ++) {
         *addrp++ = *p++;
      }
   }
   /* Toggle LSB of active index */
   active_index ^= 0x01;

   /* Update inactive index */
   indexp = &dir.index[active_index];
   indexp->addrp = addrp;      /* Store addr number of next record */
   indexp->tupl_no = tupl_no+1;/* Store tupl of next record    */
   indexp->bank_no = bank_no;  /* Store bank no of next record */

   /* Commit change to array */
   housekeeping.active_index = active_index;


   last_stored_tupl_num = tupl_no;

   return tupl_no;  /* tupl number of this record */

 } /*end store*/

/*;*<*>********************************************************
 * drm_ram_get
 *
 * Retrieves 'number' of bytes starting at 'addr'
 *  and storing them in '*pdest'.
 *
 * No updates are made to the index.
 *
 * Returns
 *     0 if no records exist or else
 *     1  
 *
 * Algorithim
 * To translate the *addr to a banked addr the following alogorthim is used
 *    addrp = Least significant 14 bits + base address of ram bank
 *    bank_no = (bits 17,16,15)
 *
 **start*/
 int drm_ram_get(long int *addr, /* pointer to address in linear ram   */
                  char *pdest,   /* pointer to buffer to copy them into*/
                  int number    /* Number of bytes to copy            */
 ) {
 register ULongword dest_addr;
 register enum ram_bank_number bank_no;
 register char *addrp;
 register int loop;
 register char *p = pdest;
 register int size=number; /* Use reg for speed */

#define BANK_ADDR_MASK 0x3fff
#define BANK_MASK 0x7

   /* Get the linear ram address */
   dest_addr.dword = *addr;

   /* Change linear addr to banked addr + bank_no */
   addrp = (char *)((dest_addr.word.low & BANK_ADDR_MASK) + (uint)&ram_bank[0]);
   dest_addr.dword <<= 2; /* Shift b14->b16 */
   bank_no = (enum ram_bank_number)(dest_addr.word.high & BANK_MASK);

   /* Set up the correct bank number */
   hw_ram_bank_set(bank_no);

   /* Check for overflowing one bank into another */
   /* Note: Becarefull about adjusting the following compare */
   if ( ((uint)addrp + (uint)size) < (uint)addrp) {
    /* Will run into the top of the current bank  */
      for (loop = 0; loop < size; loop ++) {
         *p++ = *addrp++;
         if (addrp == NULL) {
            /* Overflowed a bank boundary 
             * Change to next bank up.
             */
            hw_ram_bank_set(++bank_no);
            addrp += (uint) &ram_bank[0];
         }
      }

   } else {
      /* Fast Copy - all within this bank*/
      for (loop = 0; loop < size; loop ++) {
         *p++ = *addrp++;
      }
   }
   return 1;

 } /*end drm_ram_get*/

/*;*<*>********************************************************
 * drm_ram_control
 *
 * Retrieves the address of the last tupl stored in the 
 * ram store.
 *
 * The following operations are supported:-
 *      DRMC_RESET        - initialise the active index to TUPL_START_NUMBER
 *      DRMC_CURRENT_TUPL - get current active index
 *
 * Returns
 *     0 if no records exist or else
 *       tupl number
 *
 * Algorithm
 * The algorithim for converting the 'dir' addr to a long int is
 *   long_int =  ((bank_no << 14 ) + next_addr) - sizeof_last_tupl
 *
 **start*/
 int drm_ram_control(long int *p,enum drme_control action)
 {
  register ULongword temp_ul;
  register Index *indexp;
 
    indexp = &dir.index[housekeeping.active_index];

    switch(action) {
    case DRMC_RESET:
       /* reset index to begining of buffer */
       housekeeping.active_index = 0;
       dir.index[0].addrp = (char *) &ram_bank[0];
       dir.index[0].bank_no = RAM_BANK_0;
       dir.index[0].tupl_no = TUPL_START_NUMBER;
       drm_ram_store((char *)&init_buf, sizeof(Dplt_data));
       break;

    case DRMC_CURRENT_TUPL:

       temp_ul.word.high = (uint) indexp->bank_no;
       temp_ul.word.low = 0;
       temp_ul.dword >>= 2;
       temp_ul.word.low |= ( (uint)(indexp->addrp) & BANK_ADDR_MASK);

       *p = temp_ul.dword - sizeof(Dplt_data);

       if (*p < 0) {
          /* Record not-retrieved sucessfully */
          /* RAM store empty */
          return 0; 
       }
       break;
    }
    return (indexp->tupl_no - 1);

 } /*end drm_ram_control*/

/*;*<*>********************************************************
 * xyz
 **start*/
/* xyz()
 {

 } /*end xyz*/

/*;*<*>********************************************************
 * xyz
 **start*/
/* xyz()
 {

 } /*end xyz*/

/*;*<*>********************************************************
 * xyz
 **start*/
/* xyz()
 {

 } /*end xyz*/

/*;*<*>********************************************************
 * xyz
 **start*/
/* xyz()
 {

 } /*end xyz*/

