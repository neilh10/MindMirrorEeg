/* dpl_status.c ""
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
 Routines to update status line on display

Algorithms
    dpl_stat_initialisation
    dpl_status
    dpl_status_record_no - only called by PC based programs

Internal Data
*/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "general.h"
#include "sch.h"
#include "hw.h"
#include "uif.h"
#include "dpl.h"
#include "dpl_stat.h"
#define MMS_ATT_TABLE /* To allow definition of table in mmstatus.h" */
#include "mmstatus.h"

/**************************************************************
 * Constants 
 */

/**************************************************************
 * Externally defined 
 */
 extern char *uif_demo_statusp;

/**************************************************************
 * Internal storage to this module 
 */

/*extern enum UIFE_MODE uife_mode; */
/* enum UIFE_TEST_MODE test_mode; */
/* extern enum UIFE_MONTAGE uifv_montage; */


/**************************************************************
 * Internal prototypes 
 */
 void time_update(char *p, uint time_stamp);


/*;*<*>********************************************************
 * dpl_stat_initialisation
 *
 **start*/
void dpl_stat_initialisation(void)
 {


 } /*end dpl_stat_initialisation*/

/*;*<*>********************************************************
  *  dpl_status
  *
  * This function is responsible for 
  *         parsing the information for the display status line
  *         and then displaying it.
  * The position of the display is specified in
  *       dpl_stat.h
  * This is a different file for
  *       mmir unit programs
  *       mmir PC programs
  * 
  * When in UIFE_DEMO mode the display line shall display the
  *  string *uif_demo_statusp
  *
  * When  in the  UIFE_OPTS mode the display line is invalid, and no
  * updates should be done.
  *
  * When in all other modes the display line has the following meaning
  *             0123456789012345
  *             tttttssssssmmmmm
  * Where 
  *    ttttt  is a time indicator
  *    ssssss is a status indicator
  *    mmmmm  is a miscellaneous information indicator
  *
  *    ttttt is of the form mmdss
  *           mm for minutes ' 0' to the limit of the storage buffer
  *           ss is a seconds indicator 00 to 59 (may be turned off)
  *           d is a delimiter  : when recording
  *                             E for event detected
  *                             ?F for frozen screen
  *                             R for when the display buffer is reviewed
  *    ssssss is of the form
  *    qqquV      normal status line qqq being 1,3,5,10,30,50,100
  *     TEST      TEST flashing )
  *    S TEST     S TEST flashing)
  *
  *    mmmmm is of the form
  *     mprb
  *       m  - indicates montage DRB (Differential/Red/Black)
  *       p  - indicates whether data is being recorded P - pause else ' '
  *       r  - indicates recording rate Range: 1,2,4
  *       b  - battery status   B - Low Battery else ' '
  *
  * The display is built in the buffer mmir_status_buf
  *
  **start*/
 void dpl_status(char *mmir_status_buf,
                 int in_status,
                 int time_stamp,
                 int att
                 )
 {
  register int status = in_status;
  register int loop, cursor;


   if ( status & MMS_DEMO_MASK) {
      /* demo in operation - get the title string */
      strncpy(&mmir_status_buf[0],
              uif_demo_statusp,
              MMIR_STATUS_BUF_SIZE);
      return; /**********************/

   }

   /* Clear string */
    for (loop=0; loop<MMIR_STATUS_BUF_SIZE; loop++)
       mmir_status_buf[loop] = ' ';

   /*
    * Do the time section
    */
   time_update(mmir_status_buf,time_stamp);

   /*
    * Event/Review mode indicator
    */
   if((status & MMS_DM_MASK) == MMS_DM_REVIEW) {
#define TS_LSB_INDICATOR 0x01 
      /* Check the LSB to indicate what to indicate
       * The idea is that for every new record there should be a change
       * for the viewers to see
       */
      if (time_stamp & TS_LSB_INDICATOR) {
         cursor = 'r';
      } else {
         cursor = 'R';
      }
      *(mmir_status_buf + REVIEW_MODE_POS) = cursor;
   } /* else leave in state found */

   /*
    * Montage Indicator
    */
   switch (status & MMS_MONTAGE_MASK) {
   case MMS_MONTAGE_LATERAL:
      cursor = 'L';
      break;

   case MMS_MONTAGE_RED_GREEN:
      cursor = 'R';
      break;

   case MMS_MONTAGE_BLACK_GREY:
      cursor = 'B';
      break;

   default:
      cursor = '-';
      break;
   }
   *(mmir_status_buf+MONTAGE_POS) = cursor;

   /*
    * Pause Indicator
    */
   if (status & MMS_PAUSE_MASK) {
      /* Pause is indicated */
      cursor = 'P';
   } else {
      cursor = ' ';
   }
   *(mmir_status_buf+PAUSE_POS) = cursor;

   /*
    * Recording Rate
    */
   *(mmir_status_buf+RECORDING_RATE_POS) = ' ';

   /*
    * Battery Indicator
    */
   if (status & MMS_BATTERY_MASK) {
      /* Battery is low */
      cursor = 'B';
   } else {
      /* No battery indication */
      cursor = ' ';
   }
   *(mmir_status_buf+BATTERY_POS) = cursor;

   /*
    * Put the sss setting in place
    */
   strncpy( (mmir_status_buf+SCALE_STARTING_POS),
             mms_att_table[att],
             SSS_SIZE);

 } /*end dpl_status */

/*;*<*>********************************************************
 * time_update
 * 
 * This takes an integer value dplp->time_stamp and translates it to an 
 * ASCII string representing time.
 *
 * Input
 * *p - string to write the time to
 * dplp->time_stamp contains time value to be translated.
 *
 *
 **start*/
 void time_update(char *p_in,uint time_stamp)
 {
 register uint time = time_stamp; /* Note: time_stamp is also used*/
 register char *p = p_in,marker=':';
 static int seconds;  /* Seconds units */
 static int tseconds; /* Seconds tens  */
 static int minutes;  /* Minutes units */
 static int tminutes; /* Minutes tens  */
 static int hours;    /* Hours units   */
#define SAMPLES_PER_HOUR       3600
#define SAMPLES_PER_TEN_MINUTES 600
#define SAMPLES_PER_MINUTE       60
#define SAMPLES_PER_TEN_SECONDS  10


     /* Make into 1 sample per second */
     time >>= 1;

     /* Decode time into
      *    hours    0 -> 9
      *    tminutes 0 -> 5
      *    minutes  0 ->59
      *    tseconds 0 -> 5
      *    seconds  0 ->59
      */
     hours = (time/SAMPLES_PER_HOUR);
     time -= (hours * SAMPLES_PER_HOUR);

     tminutes = (time/SAMPLES_PER_TEN_MINUTES);
     time -= (tminutes * SAMPLES_PER_TEN_MINUTES);

     minutes = (time/SAMPLES_PER_MINUTE);
     time -= minutes * SAMPLES_PER_MINUTE;

     tseconds = (time/SAMPLES_PER_TEN_SECONDS);
     seconds = time - (tseconds*SAMPLES_PER_TEN_SECONDS);


     /* Put into the string to be printed 
      */
     *(p+MMIR_TIME_MIN_T_POS) = ('0' + tminutes);
     *(p+MMIR_TIME_MIN_U_POS) = ('0' + minutes);
     *(p+MMIR_TIME_SEC_T_POS) = ('0' + tseconds);
     *(p+MMIR_TIME_SEC_U_POS) = ('0' + seconds);

     /* Decode the hours
      *  - For the MMIR unit, replace the colon with a flashing hr unit
      *  - For the MMIR PC, use a separate field. But clear it
      *    when the hour unit is '0'
      */
     if (hours != 0) {
#if defined(MMIR_PC)
        *(p+MMIR_TIME_HOURS_POS) = ('0' + hours);
        *(p+MMIR_TIME_HCOLON_POS) = ':';      
#else
         marker = ('0' + hours);
#endif /* defined(MMIR_PC) */
     }
#if defined(MMIR_PC)
       else {
        *(p+MMIR_TIME_HOURS_POS) =  ' ';
        *(p+MMIR_TIME_COLON_POS) =  ' ';

     }
#endif /* defined(MMIR_PC) */

     /* Put a flashing colon on the screen.
      */
     if (time_stamp & 01) /* time_stamp is different to time */
     {
        *(p+MMIR_TIME_COLON_POS) = marker;
     } else {
        /* Assume string is same as previous 0.5 second 
         * Only change 'flashing' marker.
         */
        *(p+MMIR_TIME_COLON_POS) = ' ';
     }

 } /*end time_update*/

#if defined(MMIR_PC)
/*;*<*>********************************************************
 * dpl_status_record_no
 *
 * Prints the record number in the specified buffer
 **start*/
 void dpl_status_record_no(char *mmir_status_buf, unsigned long int record_no)
 {
#define MAX_RECORD_NO 999999L
   if (record_no > MAX_RECORD_NO)
      record_no = MAX_RECORD_NO;

   /*
    * Do the record_no update
    */
   sprintf(&mmir_status_buf[MMIR_RECORD_NO_BEGIN],"%6lu",record_no);

   /* Get rid of the terminating null introduced by sprintf */
   *(mmir_status_buf+(MMIR_RECORD_NO_BEGIN+MMIR_RECORD_NO_SIZE+1)) = ' ';

 } /*end dpl_status_record_no*/
#endif /* MMIR_PC */

/*;*<*>********************************************************
 * xyz
 **start*/
/* xyz()
 {

 } /*end xyz*/
