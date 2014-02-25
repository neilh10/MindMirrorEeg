/*
 prc_fdas.c

*/
unsigned char program_name[] = "PRC_FDAS";
unsigned char      version[] = "V 1.5";
/*
 20-Aug-91 V1.3 Added [NUMBER_LOBES] to Biquad declarations
 16-Oct-91 V1.4 Compensated for FREQUENCY/FREQUENCIES
                difference between bandpass/lowpass filter inputs
                by using FREQ. Also added const designatorto
                Iir_filter_descriptor which IC96 requires.
10-Sept-91 V1.5 Added #ifdef for FILTER_COEFF, FILTER_BIQUAD &
                FILTER_DESCRIPTOR

 *** Description ***

 This program takes the output of the FDAS I program and generates 
 an include file for digital filter programs.

   xxx_yy[zz].FLT  -->  xxx_yy[zz].HF

 There are three basic sections to the coefficient files

     - Header section with a lot of textual information
     - Coefficient Implementation Section
     - Coefficients in floating point form.

 Header Section
 --------------
 
 This is in the general form
     <const text> <information text>
 The general method is to take the <const text> from the coefficient file
 and generate an output that consists of 
 #define Fxxx_yy[zz]_<const_text> <information text>

 The input will be processed line by line. If the following cases the 
 <information text> will be examined, if not as expected an error message
 generated, and the program terminated:-

     IIR DESIGN
     BAND PASS
     
 Hence the output of the header section will look like

 #ifndef Fxxx_yy[zz]_h
 #define Fxxx_yy[zz]_h

<input record no>
 1> #define Fxxx_yy[zz]_FILTER_COEFFICIENT_FILE
 2> #define Fxxx_yy[zz]_DESIGN IIR_DESIGN
 3> #define Fxxx_yy[zz]_FILTER_TYPE BAND_PASS
 4> #define Fxxx_yy[zz]_ANALOG_FILTER_TYPE BUTTERWORTH
 5> #define Fxxx_yy[zz]_PASSBAND_RIPPLE -28
 6> #define Fxxx_yy[zz]_STOPBAND_RIPPLE -45
7a> #define Fxxx_yy[zz]_PASSBAND_CUTOFF_FREQUENCY_1 1.6
7b> #define Fxxx_yy[zz]_PASSBAND_CUTOFF_FREQUENCY_2 1.8
8a> #define Fxxx_yy[zz]_STOPBAND_CUTOFF_FREQUENCY_1 0.25
8b> #define Fxxx_yy[zz]_STOPBAND_CUTOFF_FREQUENCY_2 4.00
 9> #define Fxxx_yy[zz]_SAMPLING_FREQUENCY 128
10> #define Fxxx_yy[zz]_FILTER_DESIGN_METHOD BILINEAR_TRANSFORMATION
11> #define Fxxx_yy[zz]_FILTER_ORDER 5
12> #define Fxxx_yy[zz]_NUMBER_OF_SECTIONS 3
13> #define Fxxx_yy[zz]_NO_OF_QUANTIZED_BITS 16
14> #define Fxxx_yy[zz]_QUANTIZATION_TYPE FLOATING_POINT
15> #define Fxxx_yy[zz]_COEFFICIENTS_SCALED_FOR CASCADE_FORM_1
16> #define Fxxx_yy[zz]_SHIFT_COUNT_FOR_OVERALL_GAIN 0
17> #define Fxxx_yy[zz]_OVERALL_GAIN 32767


 Coefficient Section
 -------------------

 The input to this section looks like
  <dec num> <hex num> <string>

 The output will be of the form 

 Fxxx_yy[zz]_coeff[] = {
 < dec num >,  <string>
 .....
  repeated  (6 or 5) * number of sections
 };
 
 For the Fractional Fixed Form the coefficients must be shifted right
 by the shift count for each section.

 Coefficients in floating point form
 -----------------------------------

 This information is ignored.

 Final Output Section
 --------------------

 Then the following constant information will be printed to the file:

#ifdef Fxxx_yy[zz]_EQUATES

 /!*********************************************
  * Storage for fract integer processing
  *!/
  Biquad Fxxx_yy[zz]_biquad[NUMBER_LOBES][Fxxx_yy[zz]_NUMBER_OF_SECTIONS];

 /!*********************************************
  * Storage for general filter description table
  *!/
  Iir_filter_descriptor Fxxx_yy[zz]_filter_descriptor = {
  (INT16)        Fxxx_yy[zz]_NUMBER_OF_SECTIONS,
  (int)          Fxxx_yy[zz]_SHIFT_COUNT_FOR_OVERALL_GAIN
  (int)          Fxxx_yy[zz]_OVERALL_GAIN
  (Iir_coeff *) &Fxxx_yy[zz]_coeff,
  (Biquad *)    &Fxxx_yy[zz]_biquad[MAX_NUMBER_LOBES],
  (char) Fxxx_yy[zz]_DESIGN,
  (char) Fxxx_yy[zz]_FILTER_TYPE,
  (char) Fxxx_yy[zz]_ANALOG_FILTER_TYPE,
  (float) Fxxx_yy[zz]_PASSBAND_RIPPLE,
  (float) Fxxx_yy[zz]_STOBAND_RIPPLE,
  (float) Fxxx_yy[zz]_PASSBAND_CUTOFF_FREQUENCY_1,
  (float) Fxxx_yy[zz]_PASSBAND_CUTOFF_FREQUENCY_2,
  (float) Fxxx_yy[zz]_STOPBAND_CUTOFF_FREQUENCY_1,
  (float) Fxxx_yy[zz]_STOPBAND_CUTOFF_FREQUENCY_2,
  (char) Fxxx_yy[zz]_FILTER_DESIGN_METHOD,
  (INT16)  Fxxx_yy[zz]_SAMPLING_FREQUENCY,
  (char) Fxxx_yy[zz]_FILTER_ORDER,
  (char) Fxxx_yy[zz]_NO_OF_QUANTIZED_BITS,
  (char) Fxxx_yy[zz]_QUANTIZATION_TYPE,
  (char) Fxxx_yy[zz]_COEFFICIENTS_SCALED_FOR
  };
#endif /* #if Fxxx_yy[zz]_EQUATES defined *!/
#endif /* #if Fxxx_yy[zz]_h defined *!/
/!* end  xxx_yy[zz].hf *!/

 */

/*
 Input Format is	 (from FDAS manual, section 4.1.1 and )

   Record 1 1-30 characters
		 constant='FILTER COEFFICIENT FILE'

	Record 2
		constant='IIR DESIGN'

	Record 3
		filter type

	Record 4
		analog filter type

	Record 5
		Passband ripple

	Record 6
		Stopband ripple

	Record 7
		Passband cutoff frequencies

	Record 8
		Stopband cutoff frequencies

	Record 9
		Sampling frequency

	Record 10
		Filter design method starting byte 24

	Record 11
		Filter order in decimal and hex
		Decimal number in bytes 23 through 26
		Hex in bytes 28->31

	Record 12
	   Number of 2nd order sections in decimal and hex
		Decimal 23 - 26, Hex 28 - 31

	Record 13
		Number of quantized bits in decimal and hex
		Decimal 23 - 26, Hex 28 -31

	Record 14
		Quantization type starting in bytes 21

	Record 15
		Realization type starting in bytes 25

	Record 16
		Shift count of overall gain 
		Decimal value right justified 1 - 11, Hex 12 - 19
		(which way?, when ?)

	Record 17
		Fractional value of overall gain
		Decimal value right justified 1 - 11, Hex 12 - 19

The next set of records (18 through 17 + 6*N) are the
coefficient records in fixed point format. 
	Record 1 (18)
		Section shift count
		( for left adjusting output of section, compensates for
			scaling coefficients?)

	Record 2 (19)
		Coefficient of numerator Z2 term	(b0)

	Record 3 (20)
		Coefficient of numerator Z term	(b1)

	Record 4 (21)
		Coefficient of numerator constant term	(b2)

	Record 5 (22)
		Negative of coefficient of numerator Z term (a1)					

	Record 6 (23)
		Negative of coefficient of numerator constant term
			(a2)

The final set of records 18 +6*N through 17 + 11*N) is the
coefficient	records in floating point format
	Record 1
		Coefficient of numerator Z2 term

	Record 2
		Coefficient of numerator Z term

	Record 3
		Coefficient of numerator constant term

	Record 4
		Coefficient of denominator Z term

	Record 5
		Coefficient of denominator constant term

	Each record has decimal and hex values
	Decimal 1-24 with 17 places to the right of the decimal point
	Hex 26-41 IEEE floating point representation of quantized coefficients

 */
#include <stdio.h>
/*#include <math.h> */
#include <stdlib.h>
#include <dir.h>
#include <string.h>

#include "general.h"
#include "iir.h"
/*#include "fract.h"
*/
/*
 * Constants
 */
#define ASCII_NUMBER_OFFSET 0x30
#define MAX_INPUT_BUFFER 81
 enum white_states {FIRST_CHAR,LAST_WHITE_CHAR,LAST_NOT_WHITE_CHAR};
 enum coeff_enum {COEFF_B0,COEFF_B1,COEFF_B2,COEFF_A1,COEFF_A2};

/*
 * Globals
 */
 FILE *infile, *outfile;
 unsigned char in_buffer[MAX_INPUT_BUFFER];
 unsigned char in_file_pathname[MAXPATH], in_file_drive[MAXDRIVE],
               in_file_dir[MAXDIR], in_file_name[MAXFILE],
               in_file_ext[MAXEXT],
               out_file_pathname[MAXPATH], out_file_prefix[MAXFILE+1],
               *pin,*pout;
 int filter_realization;

 int number_of_sections_i = 0;
 double  sampling_frequency_d = 1;

/* Temporary table for coefficients */
 typedef struct {
    int coeff; char comment[80];
 } struct_coeff;
 struct_coeff coeff_tbl[5];

/*
 * Internal prototypes
 */
 void help(char *);
 void header_processing(char *ip);
 void coeff_processing(char *ip);
 void tail_processing(char *ip);
 void gl(char *il);
 void strundbar(char *il);
 void initialization(void);
 char *strloc(char *string, int c, int num);
 double get_1_rmv(char *il);
 void prt_body_coeff(int temp_i,char *ip);
 int shr_coeff(int times,int acc,char *il);
 int active_bits(unsigned input1);

/*
 * Macros
 */
#define pr1(expr) fprintf(outfile, expr ,ip)
#define pr2(expr) fprintf(outfile, expr, ip, ip)
#define prt1(expr1) fprintf(outfile, expr1)
#define prt2(expr1,expr2) fprintf(outfile, expr1, expr2)
#define prt3(expr1,expr2,expr3) fprintf(outfile, expr1, expr2, expr3)
#define prt4(expr1,expr2,expr3,expr4) fprintf(outfile,expr1,expr2,expr3,expr4)
#define white(c) (c == ' ' ||  c == '\t' || c == '\n')

 /*;*<*>********************************************************
  *
  * main control program 
  **start*/
 main(int argc, char **argv)
 {

#define NUM_ARG_EXPECTED 2

   if( (argc < NUM_ARG_EXPECTED) || (argc > NUM_ARG_EXPECTED)) {
      help((char *) &argv[0]);
      exit(0);
   }

   strcpy(in_file_pathname,argv[1]);

   /* The following is not an ANSI library function */
   strupr(in_file_pathname); /* Convert to upper case */
   initialization();

   header_processing(out_file_prefix);
   coeff_processing(out_file_prefix);
   tail_processing(out_file_prefix);


   fclose(infile);
   fclose(outfile);
   return 0;
 }

 /*;*<*>********************************************************
  * help
  **start*/
 void help(char *p)
 {

    printf("\nUsage: %s <filename>",
	      *p);

    printf("\n       Processes FDAS produced <filename> to provide\
\n	 an include file for a digital filter");
    printf("\n\n");

 } /*end help */

/*;*<*>********************************************************
  *  header_processing
  *
  **start*/
 void header_processing(char *ip)
 {
  unsigned char il[MAX_INPUT_BUFFER], tmpb[MAX_INPUT_BUFFER], *p;
  int loop, temp_i;
  double temp_d1=0.0, temp_d2=0.0;

  pr1("/*");
  prt2("\n * File name:  %s ",out_file_pathname);
  pr1("\n * ");
  fprintf(outfile,"\n * Include file generated by %s %s",program_name,version);
  pr1("\n */\n\n");
  pr1("\n#ifndef %s_h");
  pr1("\n#define %s_h\n");

  /* Record 1 - const header */
  gl(il);
  strundbar(il);
  prt3("\n#define %s_%s",ip,il);

  /* record 2 - Design */
  gl(il);
  strundbar(il);
  prt3("\n#define %s_DESIGN %s",ip,il);

  /* record 3 filter type */
  gl(il);
  strundbar(il);
  p = strloc(il,'_',(int) 2);
  *p = ' ';
  prt3("\n#define %s_%s",ip,il);

  /* record 4 - analog filter type */
  gl(il);
  strundbar(il);
  p = strloc(il,'_',(int) 3);
  *p = ' ';
  prt3("\n#define %s_%s",ip,il);

  /* record 5 & 6 - Stopband ripple */
  for (loop = 0; loop < 2; loop++ ){
     gl(il);
     p = strloc(il,' ',(int) 4);
     temp_d1 = atof(p);
     p = strloc(il,' ',(int) 2);
     *p = '\0';
     strundbar(il);
     prt4("\n#define %s_%s %f",ip,il,temp_d1 );
  }

  /* record 7 and  8 - Passband and stopband cutoff frequency */
  for (loop = 0; loop < 2; loop++ ){
     gl(il);
     strundbar(il); /* sets all white spaces to _. Easier for searching */
     p = strloc(il,'_',(int) 4);
     temp_d2 = atof(++p); /* Get first cutoff frequency
                             terminates when finds _ */
     p = strloc(il,'_',(int) 3);
     temp_d1 = atof(++p); /* Get second cutoff frequency
			     terminates when finds ) */
     /* Search for commong ending point BANDPASS and LOW PASS
      * use slightly different words */
     p = strloc(il,'Q',(int) 1);
     *(++p) = '\0';           /* end string here */
     prt4("\n#define %s_1_%s %f",ip,il,temp_d1);
     prt4("\n#define %s_2_%s %f",ip,il,temp_d2);
  }
  /* record 9 Sampling Frequency*/
  gl(il);
  p = strloc(il,' ',(int) 2);
  sampling_frequency_d = atof(p);
  strundbar(il);
  p = strloc(il,'_',(int) 2);
  *p = '\0';
  prt4("\n#define %s_%s %d",ip,il,((int)sampling_frequency_d));

  /* record 10 - Filter Design Method */
  gl(il);
  p = strloc(il,':',(int)1); /* Find : and turn into space */
  *p = ' ';
  strundbar(il);
  p = strloc(il,'_',(int)3);
  *p = ' ';
  prt3("\n#define %s_%s",ip,il);

  /* record 11 - Filter order */
  gl(il);
  p= strloc(il,' ',(int)2 );
  temp_i = atoi(p);
  *p = '\0';
  strundbar(il);
  prt4("\n#define %s_%s %d",ip,il,temp_i);

  /* record 12 - Number of sections */
  gl(il);
  p= strloc(il,' ',(int) 3 );
  number_of_sections_i = atoi(p);
  *p = '\0';
  strundbar(il);
  prt4("\n#define %s_%s %d",ip,il,number_of_sections_i);

  /* record 13 - No of quantized bits */
  gl(il);
  p = strloc(il,' ',(int) 4 );
  temp_i = atoi(p);
  *p = '\0';
  p = strloc(il,'.',(int) 1 );
  *p = ' ';
  strundbar(il);
  prt4("\n#define %s_%s %d",ip,il,temp_i);

  /* record 14 Quantization type */
  gl(il);
  p = strloc(il,'-',(int)1); /* Find : and turn into space */
  *p = ' ';
  strundbar(il);
  p = strloc(il,'_',(int)2);
  *p = ' ';
  prt3("\n#define %s_%s",ip,il);
  /* Need to find whether the quantization type is
   *     FLOATING_POINT or
   *     FRACTIONAL_FIXED_POINT.
   */
   if ( strstr(il,"FLOATING_POINT\0") != NULL) {
      /* we have FLOATING_POINT */
      filter_realization = FLOATING_POINT;
   } else {
      /* Assume we have FRACTIONAL_FIXED_POINT */
      filter_realization = FRACTIONAL_FIXED_POINT;
   }

  /* record 15 - Coefficients scaled for */
  gl(il);
  strundbar(il);
  p = strloc(il,'_',(int)3);
  *p = ' ';
  prt3("\n#define %s_%s",ip,il);

 } /*end header_processing */

/*;*<*>********************************************************
  * coeff_processing
  *
  * This reads in the coefficients from the input file and outputs
  * them in the correct order.
  *
  **start*/
 void coeff_processing(char *ip)
 {
 unsigned char il[MAX_INPUT_BUFFER];
 int no_coeff_records, lp1, lp2, temp_i, sect_shift_count;
 double temp_d;

  /* OVERALL_SHIFT_COUNT 
   * OVERALL_GAIN
   */
  switch(filter_realization) {
  case FLOATING_POINT:
     no_coeff_records = 4;
     /* Shift Count for overall gain - no such thing for floating point
      * but provide a dummy
      */
     prt2("\n#define %s_OVERALL_SHIFT_COUNT 0 /* Dummy Value */",ip);

     /* Overall Gain */
     gl(il);
     temp_d =  get_1_rmv(il);
     prt4("\n#define %s_OVERALL_GAIN %f %s",ip,temp_d,il);
     break;

  case FRACTIONAL_FIXED_POINT:
  default:
     no_coeff_records = 6;
     /* Shift Count for overall gain */
     gl(il);
     temp_i = (int) get_1_rmv(il);
     prt4("\n#define %s_OVERALL_SHIFT_COUNT %d %s",ip,temp_i,il);
     /* Overall Gain */
     gl(il);
     temp_i = (int) get_1_rmv(il);
     prt4("\n#define %s_OVERALL_GAIN %d %s",ip,temp_i,il);

     break;
  }

  prt2( "\n\n#ifdef %s_EQUATES",ip);
  prt1("\n\n#ifdef FILTER_COEFF");

  /*
   * print start of structure 
   */
  prt1("\n\n");

  switch(filter_realization) {
  case FLOATING_POINT:
     prt2("const Iir_coeff_fp %s_coeff[] = {",ip);
     /*
      * print body of structure
      */
     for ( lp1 = 0; lp1 < number_of_sections_i; lp1++) {
        for ( lp2 = 0; lp2 < no_coeff_records; lp2++) {
           gl(il);
           temp_d = get_1_rmv(il);
	   prt3("\n   %f,\t%s",temp_d,il);
	}
     }
     break;

  case FRACTIONAL_FIXED_POINT:
  default:
     prt2("const Iir_coeff %s_coeff[] = {",ip);
     /*
      * print body of structure
      */
     for ( lp1 = 0; lp1 < number_of_sections_i; lp1++) {
        /*
         * Process each section
         *  1) Get sect_shift_count for section and print it out
         *  2) Get the coefficients and shift them right by
         *     sect_shift_count before printing them out.
         */
        gl(il);
        sect_shift_count = (int) get_1_rmv(il);
        prt_body_coeff(sect_shift_count,il);
        for ( lp2 = 0; lp2 < (no_coeff_records-1); lp2++) {
           gl(il);
           temp_i = (int) get_1_rmv(il);
           coeff_tbl[lp2].coeff = shr_coeff(sect_shift_count, temp_i, il);
           strcpy(&(coeff_tbl[lp2].comment[0]),il);
        }

        prt_body_coeff(coeff_tbl[COEFF_B0].coeff,
                       &(coeff_tbl[COEFF_B0].comment[0]));
        prt_body_coeff(coeff_tbl[COEFF_B1].coeff,
                       &(coeff_tbl[COEFF_B1].comment[0]));
        prt_body_coeff(coeff_tbl[COEFF_A1].coeff,
                       &(coeff_tbl[COEFF_A1].comment[0]));
        prt_body_coeff(coeff_tbl[COEFF_B2].coeff,
                       &(coeff_tbl[COEFF_B2].comment[0]));
        prt_body_coeff(coeff_tbl[COEFF_A2].coeff,
                       &(coeff_tbl[COEFF_A2].comment[0]));
     }
     break;
  }

  /* 
   * print end of structure
   */
  prt2("\n   0\t/* end %s_coeff[] */\n }; \n\n",ip);
  prt1("\n#else  /* FILTER_COEFF */");
  switch(filter_realization) {
  case FLOATING_POINT:
     prt2("\n   extern const Iir_coeff_fp %s_coeff[];",ip);
     break;
  case FRACTIONAL_FIXED_POINT:
  default:
     prt2("\n   extern const Iir_coeff %s_coeff[];",ip);
     break;
  }
  prt1("\n#endif /* FILTER_COEFF */\n");

 } /*end coeff_processing */

 /*;*<*>********************************************************
  * tail_processing
  *
  * Print the end tail processing of the include file
  *
  **start*/
 void tail_processing(char *ip)
 {
 int temp_i;
 double temp_d;

 pr1("\n /*********************************************");
 pr1("\n  * Storage for fract integer processing");
 pr1("\n  */");
 prt1("\n#ifdef FILTER_BIQUAD\n");
 switch(filter_realization) {
 case FLOATING_POINT:
    pr2("\n Biquad_fp %s_biquad[NUMBER_LOBES][%s_NUMBER_OF_SECTIONS];\n");
    prt1("\n#else  /* FILTER_BIQUAD*/");
    pr2("\n extern Biquad_fp %s_biquad[NUMBER_LOBES][%s_NUMBER_OF_SECTIONS];");
    break;

 case FRACTIONAL_FIXED_POINT:
 default:
    pr2("\n Biquad %s_biquad[NUMBER_LOBES][%s_NUMBER_OF_SECTIONS];\n");
    prt1("\n#else  /* FILTER_BIQUAD*/");
    pr2("\n extern Biquad %s_biquad[NUMBER_LOBES][%s_NUMBER_OF_SECTIONS];");
 }
 prt1("\n#endif /* FILTER_BIQUAD*/\n");

 pr1("\n/*********************************************");
 pr1("\n * Storage for general filter description table");
 pr1("\n */");
 prt1("\n#ifdef FILTER_DESCRIPTOR\n");
 switch(filter_realization) {
 case FLOATING_POINT:
    pr1("\n Iir_filter_descriptor_fp %s_filter_descriptor = {");
    pr1("\n (INT16)          %s_NUMBER_OF_SECTIONS,");
    pr1("\n (int)            %s_OVERALL_SHIFT_COUNT,");
    pr1("\n (double)         %s_OVERALL_GAIN,");
    pr1("\n (Iir_coeff_fp *) &%s_coeff,");
    pr1("\n (Biquad_fp *)    &%s_biquad[0],");
    pr1("\n (Biquad_fp *)    &%s_biquad[1],");
    pr1("\n (Biquad_fp *)    &%s_biquad[2],");
    pr1("\n (Biquad_fp *)    &%s_biquad[3],");
    break;


 case FRACTIONAL_FIXED_POINT:
 default:
    pr1("\n const Iir_filter_descriptor %s_filter_descriptor = {");
    pr1("\n (INT16)       %s_NUMBER_OF_SECTIONS,");
    pr1("\n (int)         %s_OVERALL_SHIFT_COUNT,");
    pr1("\n (int)         %s_OVERALL_GAIN,");
    pr1("\n (Iir_coeff *) &%s_coeff,");
    pr1("\n (Biquad *)    &%s_biquad[0],");
    pr1("\n (Biquad *)    &%s_biquad[1],");
    pr1("\n (Biquad *)    &%s_biquad[2],");
    pr1("\n (Biquad *)    &%s_biquad[3],");
    break;
 }

 pr1("\n (char) %s_DESIGN,");
 pr1("\n (char) %s_FILTER_TYPE,");
 pr1("\n (char) %s_ANALOG_FILTER_TYPE,");
 pr1("\n (float) %s_PASSBAND_RIPPLE,");
 pr1("\n (float) %s_STOPBAND_RIPPLE,");
 pr1("\n (float) %s_1_PASSBAND_CUTOFF_FREQ,");
 pr1("\n (float) %s_2_PASSBAND_CUTOFF_FREQ,");
 pr1("\n (float) %s_1_STOPBAND_CUTOFF_FREQ,");
 pr1("\n (float) %s_2_STOPBAND_CUTOFF_FREQ,");
 pr1("\n (char) %s_FILTER_DESIGN_METHOD,");
 pr1("\n (int)  %s_SAMPLING_FREQUENCY,");
 pr1("\n (char) %s_FILTER_ORDER,");
 pr1("\n (char) %s_NO_OF_QUANTIZED_BITS,");
 pr1("\n (char) %s_QUANTIZATION_TYPE,");
 pr1("\n (char) %s_COEFFICIENTS_SCALED_FOR");
 pr1("\n };\n");

 prt1("\n#else  /* FILTER_DESCRIPTOR */");

 switch(filter_realization) {
 case FLOATING_POINT:
    pr1("\n extern Iir_filter_descriptor_fp %s_filter_descriptor;");
    break;


 case FRACTIONAL_FIXED_POINT:
 default:
    pr1("\n extern const Iir_filter_descriptor %s_filter_descriptor;");
    break;
 }

 prt1("\n#endif /* FILTER_DESCRIPTOR */\n");

 pr1("\n#endif /* #if %s_EQUATES defined */");
 pr1("\n#endif /* #if %s_h defined */");
 pr1("\n/* end  %s.hf */");

 } /*end tail_processing */

/*;*<*>********************************************************
  * gl 
  * Get a line of characters, and change them to upper case
  * Change an '\n' to ' '.
  *
  **start*/
 void gl(char *il)
 {
    char *p;

    fgets(il, (int) MAX_INPUT_BUFFER, infile);
    strupr(il);
    if ( (p = strchr(il,'\n')) != NULL)
       *p = ' ';
    printf("%s\n",il);
 } /*end*/

 /*;*<*>********************************************************
  * strundbar
  *
  * Strips leading and trailing white spaces, and converts the rest
  * to "_"
  *
  **start*/
 void strundbar(char *il)
 {
    size_t len = strlen(il);
    char *tp = il, c;
    enum white_states state = FIRST_CHAR;

    /* Check for null string, or improperly terminated string */
    if ((len == 0) || (len > MAX_INPUT_BUFFER))
       return;

    /* Check for trailing white spaces and remove them */
    tp += len-1;
    while (white(*tp)) {
       *tp = '\0';
       tp--;
    }
    while ( (c = *il) != '\0' ) {
       if (white(c)) {
          switch (state) {
	  case FIRST_CHAR:
	  case LAST_WHITE_CHAR:
	     /*
	      * Remove extra white space
              */
	     strcpy(il,il+1);
	     il--; /* decrement so can be incremented next */
	     break;

          case LAST_NOT_WHITE_CHAR:
             /*
              * Replace white space with _
              */
	     *il = '_';
	     state = LAST_WHITE_CHAR;
	     break;

          default:
             state = LAST_NOT_WHITE_CHAR;
          }
       } else {
          state = LAST_NOT_WHITE_CHAR;
       }
       il++;
    }

 } /*end strundbar */

 /*;*<*>********************************************************
  * initialization
  *
  **start*/
 void initialization
(void)
 {

   if ( (infile = fopen(in_file_pathname,"r")) == NULL) {
      printf("Couldn't open %s",in_file_pathname);
      exit(0);
   }

#ifdef TC
   fnsplit(in_file_pathname,
            in_file_drive,in_file_dir,in_file_name,in_file_ext);
#else
  *** Problem. NON ANSI library routine. Compatible TC only.
      MSC version is  _makepath
#endif

   /*
    * Set up output file name
    */
   strcpy(out_file_pathname,"");
   strcat(out_file_pathname,in_file_name);
   strcat(out_file_pathname,".hf");

   /*
    * Set up prefix for constants
    */
   strcpy(out_file_prefix,"F");
   strcat(out_file_prefix,in_file_name);

   if ( (outfile = fopen(out_file_pathname,"w")) == NULL) {
      printf("Error opening file: %s\n", out_file_pathname);
      exit(0);
   }

 } /*end initialization */

 /*;*<*>********************************************************
  * strloc
  * 
  * Description
  * The strloc function searches for the 'num' occurence of the 
  * character 'c' in the C string 'string'. The terminating null
  * character is included in the search; it can also be the character
  * to be located.
  *
  * Returns
  * If the character 'c' is found, strloc returns a pointer to the 'num'
  * occurence of 'c' in 'string'. If the search fails strloc returns a
  * NULL pointer.
  *
  * Includes
  * #include <string.h>
  *
  **start*/
  char *strloc(char *string,int c, int num)
 {
 int loop;
     for (loop = 0; loop < num; loop++) {
	if ( (string = strchr(++string,c)) == NULL)
	   return NULL;

     }
     return (char *) string;
 } /*end strloc */

 /*;*<*>********************************************************
  * get_1_rmv
  *
  * Get first decimal digit, and remove all characters in string
  * 'il' until the start of the first comment /
  *
  * return a double
  **start*/
 double get_1_rmv(char *il)
 {
 double temp_d;
 char *p;

    temp_d = atof(il);
    p = strloc(il,'/',(int) 1); /* locate begining of comment */
    strcpy(il,p);  /* squash everything before comment /  */

    return temp_d;
 } /*end get_1_rmv */

 /*;*<*>********************************************************
  *  prt_body_coeff
  *
  * Pretty prints the body of each coeff line. Takes into account
  * whether the first decimal value printed is small enough to require
  * an extra pretty print tab
  *
  **start*/
 void prt_body_coeff(int temp_i,char *il)
 {
    prt2("\n   %d,\t",temp_i);
    if ((temp_i > -99) && (temp_i < 999))
       prt1("\t"); /*Cosmetic prints*/
    prt2("%s",il);
 } /*end prt_body_coeff */

 /*;*<*>********************************************************
  * shr_coeff
  *
  * Shift 'acc' right 'times', preserving the sign bit and
  * shifting in a 0. If 'times' not 0, then add a comment to
  * *il about the number of times shifted.
  *
  * Input: times - a positive number 0 to 9
  *        acc - 16 bit fractional number
  *        *il - pointer to a character string with the
  *              the following in it
  *              coefficient ....[012]
  * Output:
  *        If times not 0
  *      *il - comment added to indicate the number of times
  *        acc shifted
  * Return:
  *       acc shifted right 'times'
  * Assumptions:
  *       The string is in the format specified in the FDAS V1 manual 
  *       For an example Fig 4-7 pg 4-14, coefficient comments section
  **start*/
 int shr_coeff(int times,int acc,char *il)
 {
 char buffer[20];
 int temp_i;
 double temp_d;

    if (times == 0)  /* No action needed */
       return acc;

    /*
     * Shift "acc " right "times"
     * Take into account negative numbers
     */
#define FRAC_SIGN_MASK 0x8000
#define FRAC_BODY_MASK 0x7fff
/*
    if (acc & FRAC_SIGN_MASK) {
       /* Number is negative *!/
       acc = ~acc + 1;  /* Make positive *!/
       acc = acc >> times;
       acc = ~acc + 1;  /* Make negative *!/
       acc = acc | FRAC_SIGN_MASK;
    } else {
       /* Number is positive *!/
       acc = acc >> times;
       acc = acc & FRAC_BODY_MASK;
    } */
    /* Look for particular string in comment */
    if ((il = strstr(il,"COEFFICIENT")) == NULL)
       return acc; /* Not found - so abort */

    /* Look for end of coefficient number eg B0,B1,B2,A1,A2 */
    if ((il = strpbrk(il,"012")) == NULL)
       return acc; /* Not found so abort */

    ++il;
    *il++ = '/'; /* Put division sign after number */
    /* Add divisor to comment string */
    switch (times) {
    case 1:
      *il = '2';
      break;
    case 2:
      *il = '4';
      break;
    case 3:
      *il = '8';
      break;
    default:
      *il = '?';
      break;
    }

    fitod(acc,&temp_d);
    il += 2;
    *il++ = '=';
    *il++ = ' ';
#define NUM_DIGITS  12
    gcvt((double) temp_d, (int)NUM_DIGITS, buffer);
/* #define min(a,b) (( a < b) ? a : b) */
    temp_i = min(strlen(buffer),(NUM_DIGITS + 1));
    strncpy(il,buffer,(size_t) temp_i);

    return acc; 

 } /*end shl_coeff */

 /*;*<*>********************************************************
  * active_bits
  *
  * counts number of active bits in an unsigned integer
  *
  **start*/
 int active_bits(unsigned input1)
 {
 int loop;
    for (loop = 16; loop > 0; loop--) {
       input1 <<= 1;
       if (input1 == 0)
          break;
    }
 return loop;

 } /*end active_bits*/

 /*;*<*>********************************************************
  * 
  **start*/
/* ()
 {

 } /*end*/


