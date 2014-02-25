/*  ver_ctl.c
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
 Program to read the MSDOS file structure and allocate a version
 number and time of compiling to an output file called

      version.c

*/
unsigned char program_name[] = "VER_CTL";
unsigned char      ver_ctl[] = "V 1.0";
/*
 27-July-92 V1.0 Initial

 */

#include <stdio.h>
/*#include <math.h> */
#include <stdlib.h>
#include <dir.h>
#include <string.h>
#include <time.h>

#include "general.h"
/*
 * Constants
 */
#define ASCII_NUMBER_OFFSET 0x30
#define MAX_INPUT_BUFFER 81
 enum white_states {FIRST_CHAR,LAST_WHITE_CHAR,LAST_NOT_WHITE_CHAR};

/*
 * Globals
 */
 FILE *infile, *outfile;
/* unsigned char in_buffer[MAX_INPUT_BUFFER]; */
 unsigned char full_file_pathname[MAXPATH],
               full_file_drive[MAXDRIVE],
               full_file_dir[MAXDIR],
               full_file_name[MAXFILE],
               full_file_ext[MAXEXT],
               out_file_pathname[MAXPATH],
               out_file_prefix[MAXFILE+1]
               /* *pin,*pout */;

/*
 * Internal prototypes
 */
 void help(char *p);
 void header_processing(void);
 void ver_processing(void);
 void tail_processing(void);

/*
 * Macros
 */
#define pr1(expr) fprintf(outfile, expr ,ip)
#define pr2(expr) fprintf(outfile, expr, ip, ip)

#define prt1(expr1) fprintf(outfile, expr1)
#define prt2(expr1,expr2) fprintf(outfile, expr1, expr2)
/*
#define prt3(expr1,expr2,expr3) fprintf(outfile, expr1, expr2, expr3)
#define prt4(expr1,expr2,expr3,expr4) fprintf(outfile,expr1,expr2,expr3,expr4)
#define white(c) (c == ' ' ||  c == '\t' || c == '\n')
*/
 /*;*<*>********************************************************
  *
  * main control program 
  **start*/
 main(int argc, char **argv)
 {

#define NUM_ARG_EXPECTED 1

   if( (argc < NUM_ARG_EXPECTED) || (argc > NUM_ARG_EXPECTED)) {
      help((char *) &argv[0]);
      exit(0);
   }

   strcpy(out_file_pathname,"version.c");

   if ( (outfile = fopen(out_file_pathname,"w")) == NULL) {
      printf("Error opening %s",out_file_pathname);
      exit(0);
   }

   header_processing();
   ver_processing();
   tail_processing();

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

    printf("\n       Produces a version.c for including in released programs");
    printf("\n\n");

 } /*end help */

/*;*<*>********************************************************
  *  header_processing
  *
  **start*/
 void header_processing(void)
 {
/*
  unsigned char il[MAX_INPUT_BUFFER], tmpb[MAX_INPUT_BUFFER], *p;
  int loop, temp_i;
  double temp_d1=0.0, temp_d2=0.0;
*/
  prt1("/*");
  prt2("\n * File name:  %s ",out_file_pathname);
  prt1("\n * ");
  prt1("\n * File for defining version of directory automatically");
  prt1("\n */\n\n");
  prt1("\n#ifdef IC96");
  prt1("\n#include <h\\cntl196.h>");
  prt1("\n#pragma nolistinclude");
  prt1("\n#endif");
  prt1("\n#include \"general.h\"\n\n");

 } /*end header_processing */

/*;*<*>********************************************************
  * ver_processing
  *
  * This writes out the version number to the file
  *
  **start*/
 void ver_processing(void)
 {
   char *ver_p, *und_pos_p, und_pos, *timep;
   uchar ver_str[] = "   .    ";
   int ver_len, temp;
   time_t *timeptr, bintime;
   /*struct tm t_localp;*/
   char date[20], time_str[20];

 getcwd(full_file_pathname,MAXPATH);

#ifdef TC
/*   fnsplit(full_file_pathname,
              full_file_drive,
                full_file_dir,
               full_file_name,
                 full_file_ext);
*/
#else
  *** Problem. NON ANSI library routine. Compatible TC only.
      MSC version is  _makepath
#endif

   /*
    * Set up version data
    */
   ver_p = strrchr((const char *) full_file_pathname, '\\' );
   ver_p++;  /* Increment over \ */
   ver_len = strlen(ver_p);
   und_pos = strcspn(ver_p,"_");
   und_pos_p = ver_p + und_pos;

#define MAX_PRE_UND_POS 3
#define MAX_POST_UND_POS 3
   temp = MAX_PRE_UND_POS - und_pos;
   /*strncpy(&ver_str[0],"    ",temp);*/
   strncpy(&ver_str[temp],ver_p,und_pos);
   temp = ver_len - (und_pos+1);
   if (temp > 0) {
      strncpy(&ver_str[(MAX_PRE_UND_POS+1)],++und_pos_p, temp);
   }
   prt2("\n const unsigned char version[VERSION_SIZE] = \"%s\";",ver_str);

   /* get time */
   time(&bintime);

   /* translate to character string */
   timep = ctime(&bintime);
   *(timep + strlen(timep)-1) = '\0';

#define LEN1 7
#define LEN2 4
   /* Copy date string */
   strncpy(date,timep+4,LEN1);        /* 3 digit month + days date */
   strncpy(&date[LEN1],timep+20,LEN2);  /* 4 digit year  */
   date[LEN1+LEN2] = 0;                /* terminator */

#define LEN3 8
   /* Copy time string */
   strncpy(time_str,timep+11,LEN3);
   time_str[LEN3] = 0;

   prt2("\n const unsigned char ver_date[VERSION_DATE_SIZE] = \"%s\";",date);
   prt2("\n const unsigned char ver_time[VERSION_TIME_SIZE] = \"%s\";",time_str);
   prt2("\n const unsigned char ver_dir[] = \"%s\";",full_file_pathname);


 } /*end ver_processing */

 /*;*<*>********************************************************
  * tail_processing
  *
  * Print the end tail processing of the file
  *
  **start*/
 void tail_processing(void)
 {

 prt2("\n\n /* end of %s */\n",out_file_pathname);

 } /*end tail_processing */

 /*;*<*>********************************************************
  * 
  **start*/
/* ()
 {

 } /*end*/



