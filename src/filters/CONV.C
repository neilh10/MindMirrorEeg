/*

 conv.c
 dtofi - double convert to fractional integer
 fitod - fractional integer convert to double

*/
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "general.h"
#include "iir.h"
#include "proto.h"
/*#include "fract.h"*/
/*#include "f1_coeff.h"*/

#define EXPONENT_BIAS 0x3fe
#define OVERLOAD_VALUE 0
#define OVERLOADED_EXP 14
#define FI_SIGN_W_MASK 0x8000
#define FI_SIGN_B_MASK 0x80
#define MAX_FRACTION_VALUE 0x7FFF
/*
 * Internal prototypes
 */
 FRAC16 make_negative(FRAC16);
/*;*<*>********************************************************
 *
 * dtofi 
 *
 * Double precision to twos complement signed fractional
 *   integer conversion routine.
 *
 * input: Double precision in range of -1.0 < input < 1.0
 *
 * output: signed fractional integer for input -1.0 < input < 1.0
 *         0 for input >= 1.0 or <= -1.0
 *
 * Documentation:
 *
 * The double precision variable is expected to be in IEEE format as 
 * follows:
 *
 *
 *    |SEEE EEEE| 7         S = Sign Bit, 0 = +, 1 = -
 *    |EEEE MMMM| 6         E = 11 bit exponent
 *    |MMMM MMMM| 5         M = 68 bit mantisa
 *    |MMMM MMMM| 4
 *    |MMMM MMMM| 3
 *    |MMMM MMMM| 2
 *    |MMMM MMMM| 1
 *    |MMMM MMMM| <- address of double variable
 *
 *    The exponent has a bias of 1,023, and 0 means the double is zero.
 *    This expands to an implied format of
 *
 *    SIGN 1.[MANTISA digits as a fraction]  * 2**(EXPONET - 1023)
 *
 * The signed fractional integer format is
 *
 *    SNNN NNNN NNNN NNNN
 *
 *    The implied format is
 *
 *    SIGN 0.[N digits as a fraction] 
 *
 * eg
 * Decimal                Double                Fractional
 * 100.00              40 59 00 00 00 00 00 00    0000
 *   1.0               3f f0 00 00 00 00 00 00    0000
 *  -1.0               bf f0 00 00 00 00 00 00    8000
 *   0.5               3f e0 00 00 00 00 00 00    4000
 *  -0.5               bf e0 00 00 00 00 00 00    C000
 *   0.25              3f f0 00 00 00 00 00 00    2000
 *  -0.25                                         E000
 *   0.125                                        1000
 *  -0.125                                        F000
 *   0.00006103515625                             0002
 *  -0.00006103515625                             ????
 **start*/
 int dtofi(double input)
 {
    int i, shift_int;
    unsigned int frac_int, rnd_up;
    unsigned int sign, exp, mantisa;
    unsigned char *c_p, *c_pi;

    /*
     * Set up pointers to do funny accesses to double
     */
    c_p = (char *) &input;
    for (i=0; i < (SIZE_DOUBLE_VAR - 1); i++)
       c_p++;

    c_pi = c_p;

    /*
     * get sign bit
     */
    sign = (*c_pi & 0x80);

    /*
     * Get exponent
     */
    exp = ((*c_pi-- & 0x7f) << 4);
    exp = exp | ((*c_pi >> 4) & 0x0f);

    /*
     * Get the most significant 16 bits of the mantisa
     */
    mantisa = (((*c_pi-- & 0x0f) << 12) & 0xf000);
    mantisa = mantisa | ((*c_pi-- << 4) & 0x0ff0);
    mantisa = mantisa | (*c_pi-- & 0xf);

    if (exp == 0) {
       /* The exp==0, therefore the result is 0 */
       frac_int = 0;

    } else {
       shift_int = EXPONENT_BIAS - exp;

       /*
	* Shift right to make space for
	*      sign bit    bit 15
	*      implicit 1  bit 14
	*
	* Add implicit 1
	*/
       rnd_up = (mantisa & 0x02); /* Save for later */
       mantisa = (((mantisa >> 2) & 0x3fff) | 0x4000);

       /*
        * Check to see whether the 
        *  magnitude input >= 1.0
        */
       if (shift_int < 0) {
          /*
           * Input is overloaded |input| > 1.0
           */
	  frac_int = OVERLOAD_VALUE;
       /*
	* check to see if  no shifting required
	*/
       } else if (shift_int == 0) {
          /*
           * No shifting required
           * mantisa has already been rounded up
           */
	  frac_int = mantisa;
          /*
           * Check to see whether mantisa needs rounding up
           */
          if ((rnd_up) && (frac_int != MAX_FRACTION_VALUE))
             frac_int++;

	  if (sign)
	     /* Make the number negative  */
	     frac_int = make_negative(frac_int);

       } else {

	  if (shift_int >= OVERLOADED_EXP) {
            /*
             * The exponent is too large, so the required 
             * number of shifts would shift it out the end
             */
	     frac_int = 0;

	  } else {
             /*
              * Adjust the fractional int by the exponent
              */
             rnd_up   = mantisa & (01 << (shift_int -1));
	     frac_int = (mantisa >> shift_int);
             /*
              * Check to see whether mantisa needs rounding up
              */
	     if((rnd_up) && (frac_int != MAX_FRACTION_VALUE))
               frac_int++;

             /*
              * Make the MSB negative if needed
              */
	     if (sign)
		frac_int = make_negative(frac_int);
	  }
       }
    }
    return frac_int;
 } /*end dtofi */


/*;*<*>********************************************************
 * fitod
 *
 * Convert fractional integer to double
 **start*/
 void fitod(int frac_int,double *outputp)
{
 int i;
 unsigned int exp=EXPONENT_BIAS;
 unsigned int sign=0, mantisa=0;
 unsigned char *c_p;
 double output;

/*
 * Initialize the
 *   temporary output
 *   and the callers expected value
 *   so can abort at any stage and will be kosher
 */
 *outputp = output = 0;
 if (frac_int == 0)
   return; /* output* already initialized */

 if (frac_int & FI_SIGN_W_MASK)
    sign = FI_SIGN_B_MASK;

 /* 
  * adjust for sign bit
  */

 mantisa = frac_int << 1;
 /*
  * Check to see if mantisa is 0, in which case
  * we have -1.0000. Skip to end as
  *    exp has already been initialised to correct value
  *    and mantisa is correct
  */
 if (mantisa != 0) {
   /*
    * Check to see if negative, if so the number
    * is in twos complement form and must be 
    *     complemented
    *     incremented
    */
    if (sign) {
       /* number is in twos complement form */
       mantisa = ~mantisa;
       mantisa++;
    }

   /*
    * Look for the first bit set in the
    * mantisa. The mantisa is not 0 so there will
    * be a bit set.
    */ 
#define MSB_MANTISA_MASK 0x8000
    while (!(mantisa  & MSB_MANTISA_MASK)) {
       mantisa = mantisa << 1;
       exp--;
    }
    /*
     * Found a 1. Shift left again as double implies a 1
     * Adjust the exponent to expected value
     * EXP_BIAS plus the shift count
     */
    mantisa = mantisa << 1;
 }
/*
 * Copy  sign, exponent and mantisa to the 
 * double output
 */
 c_p = (char *) &output; /* Get addr of begining double */

 /* point to sign/exponent byte */
 for (i=0; i < (SIZE_DOUBLE_VAR - 1); i++)
     c_p++;

 /* add exponent to sign byte */ 
 sign = sign | (( exp  >> 4) & ~FI_SIGN_B_MASK);
 *c_p-- = sign;  /* Store in byte 7 */

 /* add LS nibble of exponent and MSB nibble of mantisa */
 exp = ((exp << 4) & 0xF0);
 *c_p-- = (unsigned char)(exp | ((mantisa >> 12) & 0x0F));/*Store in byte 6*/
 /* Add in rest of mantisa */
 *c_p-- = ((mantisa >> 4) & 0xFF); /* Store in byte 5 */
 *c_p-- = (mantisa & 0x0F);     /* Store in byte 4 */
/*
 * Copy the output to where caller is expecting it
 */
 *outputp = output;
 return;
 } /*end fitod */

 /*;*<*>********************************************************
 * fiabs
 *
 * Fraction Integer absolute function. Computes the absolute value
 * of input.
 *
 **start*/
 SAMPLE_TYPE_F fiabs(SAMPLE_TYPE_F input)
 {
 if (input & FI_SIGN_W_MASK) {
    /* Input is negative, so invert it */
     input = ~input;
     input++;
 }
 return input;
 } /*end fiabs*/

 /*;*<*>********************************************************
  * make_negative
  *
  * makes a 16 bit fraction negative by
  *     inverting
  *     incrementing
  *     setting the MSB
  **start*/
  FRAC16 make_negative(FRAC16 input)
 {
   input = ~input;
   input++;
   return(input | FI_SIGN_W_MASK);

 } /*end make_negative */

 /*;*<*>********************************************************
  * 
  **start*/
/* ()
 {

 } /*end*/




