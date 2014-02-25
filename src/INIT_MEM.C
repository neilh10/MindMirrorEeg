/* init_mem.c
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
 This module contains a number of memory test routines.

 They are seperated from the rest of init subsystem to facilitate
 testing.

*/
/*#define LDD_PUTCHAR_TEST /* Enables TLX scrn as console */
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
#include <80C196.h>
#include <stdio.h>
#include "general.h"
/*#include "iir.h"
#include "proto.h" */
#include "init.h"
#include "hw.h"
#include "hw_mem.h"

/*;*<*>********************************************************
  * Constant strings
  */
   const char bit_fail[] = "\n\rRAM BIT TEST FAILED at addr ";
   const char verify_fail[] = "\n\rVERIFY RAM TEST FAILED at addr ";
   const char bank_fail[]= "\n\rSWITCHING RAM BANK FAILED at addr ";
   const char got[] = " got ";
   const char expected[] = " expected ";

/*;*<*>********************************************************
  * Defines to this module
  */
#define BEGIN_MASK 1
#define END_MASK 0x100

#define TEST1_VALUE 0
#define TEST2_VALUE 0xaa
#define FILL_MEM_VALUE 0xff /* This is also a RESET instruction */

/*;*<*>********************************************************
  * Prototypes
  */
 enum inite_return ext_banked_ram_test(void);
 enum inite_return ram_bit_test(char *lower_addr);
 enum inite_return ram_addr_test(char *lower_addr, char *upper_addr);
 int verify_ram(char *lower_addr, char *upper_addr,int value);
 void fill_ram(char *lower_addr, char *upper_addr,int value);
 enum inite_return addr_test(char *lower_addr,char *upper_addr,char *tst_addr);
 enum inite_return verify_ram_bank_switch(char *addr);

/*;*<*>********************************************************
  * test_memory
  *
  * Tests the memory partition specififed
  *
  * Note: 1) Must check what memory is available to run this routine in
  *       2) This function is called with EXT_NON_BANKED_RAM when it 
  *          has the stack set to internal ram.
  **start*/
 enum inite_return test_memory(enum mem_type test_type)
 {

    switch(test_type) {
    case INTERNAL_RAM:
       return FAIL;
       break;

    case EXT_NON_BANKED_RAM:
       /* Note: Stack still set to internal ram */
       return ram_test((char *)LOWER_NB_RAM_ADDR,
                       (char *)UPPER_NB_RAM_ADDR
                      );
       break;

    case EXT_BANKED_RAM:
       return ext_banked_ram_test();
       break;

    case EXT_EPROM:
       return FAIL;
       break;

    }
    return PASS;
 } /*end test_mem*/

/*;*<*>********************************************************
  * ram_test
  *
  * Tests the range of RAM specified. Returns with a FAIL if
  * a failure is found, otherwise returns with amount of memory
  * tested.
  *
  * Note: 1) Must know what ram is available when this fn() is altered
  *       ie don't call it to test the client fn()s working ram.
  *       2) This function is called when it 
  *          has the stack set to internal ram.
  **start*/
  enum inite_return ram_test(char *lower_addr,char *upper_addr)
 {
#ifdef TC
 char buff[8];
#endif
   if( (int)ram_bit_test(lower_addr) == (int)FAIL)
      return FAIL;
#ifndef TC
   return ram_addr_test(lower_addr,upper_addr); 
#endif

#ifdef TC
   return ram_addr_test(buff,&buff[4]);
#endif

 } /*ram_test end*/

/*;*<*>********************************************************
  * ext_banked_ram_test
  *
  **start*/
enum inite_return ext_banked_ram_test(void)
 {
   enum ram_bank_number bank_loop;

   hw_ram_bank_set(RAM_BANK_0);
   if ( (int)FAIL == (int)ram_test((char *)LOWER_BANKED_RAM_ADDR,
                                   (char *)UPPER_BANKED_RAM_ADDR
                                  )
      )
   {
      return FAIL;
   }

   if (verify_ram_bank_switch((char *)LOWER_BANKED_RAM_ADDR) == FAIL) {
      return FAIL;
   }

   /*
    * Fill the banks with data
    */
   for (bank_loop=RAM_BANK_0;
       (int) bank_loop < (int) RAM_BANK_END; 
        bank_loop++)
   {
      hw_ram_bank_set(bank_loop);
      fill_ram((char *)LOWER_BANKED_RAM_ADDR,
               (char *)UPPER_BANKED_RAM_ADDR,
               FILL_MEM_VALUE);
   }

   return PASS;
 } /*end ext_banked_ram_test*/
/*;*<*>********************************************************
  * ram_bit_test
  *
  * This walks a bit through the specified byte
  *
  * Reports any failures to the console
  *
  * Note:    This function is called when it 
  *          has the stack set to internal ram.
  **start*/
 enum inite_return ram_bit_test(char *lower_addr)
 {
   register int loop;
   register char new_value; /* must be char - otherwise sign ext prblm */

   for (loop=BEGIN_MASK; loop < END_MASK; loop <<= 1) {
      *lower_addr = loop;
      new_value = *lower_addr;
      if (new_value != (char) loop) {
         init_tx_str((char *)bit_fail);      /* Tell operator */
         init_tx_hex_word((INT16) lower_addr);
         init_tx_str((char *)expected);      /* Tell operator */
         /* Output binary value for failed mask */
         init_tx_bin_char(loop);             /* expected value */
         init_tx_str((char *)got);
         init_tx_bin_char((INT16)new_value); /* actual value */

         /* let operator debug problem */
         return FAIL;  /* Need to return for time being */
         for(;;) {
           new_value = *lower_addr;
         }
         return FAIL; /* Fail */
      }
   }
   return PASS;
 } /*end ram_bit_test*/

/*;*<*>********************************************************
  *  ram_addr_test
  *
  * This performs a destructive test on the range of ram specified
  * 
  *    1) Write 0s to all ram and check all written to 0
  *    2) Perform addr line check
  * Note: This routine or the functions called must only use memory 
  *        on the stack
  **start*/
 enum inite_return ram_addr_test(char *lower_addr, char *upper_addr)
 {
 register int mask;

     if (lower_addr >= upper_addr)
        return PASS;

     fill_ram(lower_addr, upper_addr,TEST1_VALUE);
     if ((int)verify_ram(lower_addr,upper_addr,TEST1_VALUE) == (int) FAIL)
        return FAIL;


     *lower_addr = TEST2_VALUE;
     if ((int)verify_ram(lower_addr+1,upper_addr,TEST1_VALUE) == (int) FAIL)
         return FAIL;

     *lower_addr = TEST1_VALUE;
     for (mask=1; (lower_addr+mask)<=upper_addr; mask <<= 1) {
         if (addr_test(lower_addr,upper_addr,lower_addr+mask)==FAIL){
            return FAIL;
         }
     }

     fill_ram(lower_addr, upper_addr, FILL_MEM_VALUE);
     if (verify_ram(lower_addr,upper_addr,FILL_MEM_VALUE) == (int) FAIL)
        return FAIL;

     return PASS;

 } /*end ram_addr_test*/

/*;*<*>********************************************************
  * fill_ram
  *
  **start*/
 void fill_ram(char *lower_addr, char *upper_addr, int value)
 {
   register char *ptr;
   register int loop,max_value=upper_addr-lower_addr;

   for (ptr=lower_addr,loop=0; loop <= max_value; loop++){
      *ptr++=value;
   }

 } /*end fill_ram*/

/*;*<*>********************************************************
  * verify_ram
  *
  * Reads range of ram specified, and verifies against supplied
  * value
  **start*/
 int verify_ram(char *lower_addr, char *upper_addr, int value)
 {
   register char *ptr, new_value;
   register int loop,max_value=upper_addr-lower_addr;

   for (ptr=lower_addr,loop=0; loop <= max_value; loop++){
      new_value = *ptr++;
      if ( new_value != (char)value) {

         init_tx_str((char *)verify_fail);
         init_tx_hex_word((int)ptr);

         init_tx_str((char *)expected);
         init_tx_hex_char(value);

         init_tx_str((char *)got);
         init_tx_hex_char(new_value);
         return FAIL;
      };
   }
   return PASS;
 } /*end verify_ram*/

/*;*<*>********************************************************
  * init_tx_bin_char
  *
  * Transmits an 8bit char as binary ASCII on the UART, MSB first
  *
  * Total transmission - 8 ASCII '1's or '0's
  **start*/
 void init_tx_bin_char(INT16 new_value)
 {
   register int loop;

   for (loop=(END_MASK>>1); loop >= BEGIN_MASK; loop >>=1) {
      if (loop & new_value) {  /* check for bit 1/0 */
         /* This bit is a '1' */
         putchar('1');
#ifdef LDD_PUTCHAR_TEST
         ldd_putchar('1');
#endif
      } else {
         /* This bit is a '0' */
         putchar('0');
#ifdef LDD_PUTCHAR_TEST
         ldd_putchar('0');
#endif
      }
   }

 } /* init_tx_bin_char end*/

/*;*<*>********************************************************
  * addr_test
  *
  * This function sets the specified "tst_addr" to a value and
  * then verifes that the rest of the ram is as expected
  *
  **start*/
 enum inite_return addr_test(char *lower_addr,char *upper_addr,char *tst_addr)
 {

   *tst_addr = TEST2_VALUE;

   if ( (verify_ram(lower_addr,tst_addr-1,TEST1_VALUE) ||
         verify_ram(tst_addr+1,upper_addr,TEST1_VALUE)) == (int) FAIL) {
     return FAIL;
   }
   *tst_addr = TEST1_VALUE;
   return PASS;

 } /*end addr_test*/


/*;*<*>********************************************************
  * init_tx_str
  *
  * Routine to transmit a buffer on the stdio
  **start*/
 void init_tx_str(char *buf)
 {
    while (*buf != 0) {
       putchar(*buf);
#ifdef LDD_PUTCHAR_TEST
       ldd_putchar(*buf);
#endif
       buf++;
    }
 } /* init_tx_str end*/

/*;*<*>********************************************************
  * init_tx_hex_word
  *
  * Transmits a 16bit char as hex ASCII on the UART, MSB first
  *
  * Total transmission - 4 ASCII characters range 0->9,A->F
  **start*/
 void init_tx_hex_word(INT16 new_value)
 {

    init_tx_hex_nibble(new_value >> 12);
    init_tx_hex_nibble(new_value >>  8);
    init_tx_hex_nibble(new_value >>  4);
    init_tx_hex_nibble(new_value);

 } /* init_tx_hex_word end*/

/*;*<*>********************************************************
  * init_tx_hex_char
  *
  * Transmits a 8bit char as hex ASCII on the UART, MSB first
  *
  * Total transmission - 2 ASCII characters range 0->9,A->F
  **start*/
 void init_tx_hex_char(INT16 new_value)
 {

    init_tx_hex_nibble(new_value >>  4);
    init_tx_hex_nibble(new_value);

 } /* init_tx_hex_char end*/

/*;*<*>********************************************************
  * init_tx_hex_nibble
  *
  * Transmits the lowest hex nibble as an ASCII character on the
  * UART
  **start*/
 void init_tx_hex_nibble(INT16 value)
 {
   value = value & 0xf;
   if (value < 0xa) {
      putchar(value + '0');
#ifdef LDD_PUTCHAR_TEST
      ldd_putchar(value + '0');
#endif
   } else {
      putchar( value - 0x0A + 'A');
#ifdef LDD_PUTCHAR_TEST
      ldd_putchar(value - 0x0a + 'A');
#endif
   }
  
 } /*end init_tx_hex_nibble*/

/*;*<*>********************************************************
  * verify_ram_bank_switch
  *
  * Checks wether the bank switch mechanism is working.
  *
  * Input: *addr - pointer to a location to check for switching.
  *                 The value is destroyed it.
  *
  **start*/
 enum inite_return verify_ram_bank_switch(char *addr)
 {
   register enum ram_bank_number bank_loop;
   register int new_value;

   /*
    * Set the bank number
    */
   for (bank_loop=RAM_BANK_0;
       (int) bank_loop < (int) RAM_BANK_END; 
        bank_loop++)
   {
      hw_ram_bank_set(bank_loop);
      *addr = (char)bank_loop;

   }

   /*
    * Verify the bank number is still the same
    */
   for (bank_loop=RAM_BANK_0;
       (int) bank_loop < (int) RAM_BANK_END; 
        bank_loop++)
   {
      hw_ram_bank_set(bank_loop);
      new_value = (int)*addr;
      if ( !(new_value == (int) bank_loop)) {

         init_tx_str((char *)bank_fail);
         init_tx_hex_word((int)addr);

         init_tx_str((char *)expected);
         init_tx_hex_char(bank_loop);

         init_tx_str((char *)got);
         init_tx_hex_char(new_value);

         return FAIL;
      }
   }
 } /*end verify_ram_bank_switch*/

/*;*<*>********************************************************
  * 
  **start*/
/* ()
 {

 } / *end*/




