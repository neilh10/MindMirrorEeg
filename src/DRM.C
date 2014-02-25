/*  drm.c
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
 * data recorder module
* 
Import:   mfd_supply_data, and peak filter values

Export:   drm_schedule to scheduler
          drm_data to ssp at background and interrupt level
          drm_initialisation to init module

This module records the filtered data that appears on the screen in the
internal ram. It acts like a ram disk. On demand it restores the data from the
disk to a clients buffer.

The data may be requested from the display module or the serial module.
 
It is linked to the scheduler, which indicates when new data is ready. If it
is enabled to store data then it stores this data in its internal ram. 

The rate the data is stored in the ram may be set by a client (the user
interface module), this is very likely to be different from the rate that
scheduler reports the information is available. This module shall record the
maximums between scheduler calls and the recorded data rate. 

The scheduler calls the following entry point to indicate data is available.
     drm_schedule(SCHE_FAST, number_of_updates)

The following entry point is used to access the internal data
indication = drm_data(control,requestor,address_of_buffer,max_size_of_buffer)

where control is:-
     DRMC_RESET - resets the data in the buffer
     DRMC_START - jumps to the beginning of the buffer
     DRMC_END - jumps to the end of the buffer 
     DRMC_FETCH_FRWD_RECORD - gets the next +time record 
     DRMC_FETCH_BACK_RECORD - gets the next -time record
     DRMC_DATA_RATE - sets the value in requestor to be the data rate

requestor is display or serial modules

address_of_buffer (and its max_size_of_buffer) are were the data is returned.

and the returned indication is:
     PASS - request was sucessfull
     FAIL - was not sucessfull.

On initialisation the following is called
     drm_initialisation()

It may be called with the following parameters
     REPORT - reports on the status return PASS,FAIL
     COLD - destructively tests all ram and initialises it
     WARM - initialises but does not do any destructive tests

The initialisation is called twice, once with REPORT and then after that with
COLD or WARM.

The REPORT will report a PASS if all the following check OK
     - RAM powered bytes OK
     - internal pointers are consistent with expected data

Notes on methods of storing. 

If each record stored is 21 bytes * 2 hemispheres then 

If 4 samples per second are stored then 13 minutes requires 131Kbytes
If 2 samples per second are stored then 26 minutes requires 131Kbytes.
If 1 sample per second is stored, then 52 minutes requires 131Kbytes. 

(The ram is 128*1024 = 131 072bytes)

Default storage will be 2 samples per second.


Algorithms

Internal Data

*/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
/*#include <stdio.h>*/
/*#include <math.h>*/
/*#include <stdlib.h>*/

#include "general.h"
#include "iir.h"
#include "drm.h"
#include "mm_coms.h"
#include "dim.h"
#include "proto.h"
#include "mfd.h"

/**************************************************************
 * Constants 
 */

/**************************************************************
 * Externally defined 
 */
 extern const Dplt_data last_buf;

/**************************************************************
 * Internal storage to this module 
 */
  enum drme_data_storage_rate {
     /* The value of this enum is significant */
     DRMC_4_RECORDS_PER_SEC = 1,
     DRMC_2_RECORDS_PER_SEC = 2,
     DRMC_1_RECORD_PER_SEC  = 4,
     DRMC_2_SEC_PER_RECORD  = 8
};
  enum drme_data_storage_rate drm_data_storage_rate;

  /* data_sw
   * Flag to indicate whether new_data or maximums of data
   * to be used in drm_scheduler 
   *   == CLR_SEMAPHORE to indicate maximums are used
   *   == SET_SEMAPHORE to indicate raw data used
   */
  Semaphore data_sw;
  /* data_pass
   * Indicates when to store incoming data in ram disk
   * Incremented every time drm_scheduler is called and zeroed
   * when data stored.
   */
  int data_pass;

  /* max_data
   * Holds the maximums of the data retrieved from mfd_supply_data
   */
  Dplt_data max_data;

  /* Store capability */
#define DRMC_STORE_ON 0  /* - module will store data */
#define DRMC_DATA_PAUSE 1
/*#define OTHER_PAUSE 2 */
/* The variable has bits set in it by routines pausing the data collection.
 * Hence all bits must be off before data collection can proceed.
 */
  int drm_store_active;

 int last_stored_tupl_num;
/**************************************************************
 * Internal prototypes 
 */
 void store_max_data(Dplt_data *dplp, /* Supplied data      */
                     Dplt_data *max,  /* Maximum data array */
                     _eModuleId_ module
 );
 void zero_max_data(_eModuleId_ Module);

/*;*<*>********************************************************
 * drm_initialisation
 *
 *
 **start*/
 enum inite_return drm_initialisation(Initt_system_state *ssp)
 {

  drm_ram_initialisation(ssp);

  /* Enable storing capability */
  drm_store_active = DRMC_STORE_ON;

  switch (ssp->init_type) {
    case COLD:
       drm_data_storage_rate = DRMC_2_RECORDS_PER_SEC;
       /* Drop through and do warm initialisation */

    case WARM:
       /* WARM initialisation - also performed by COLD */
       data_sw = SET_SEMAPHORE;
       data_pass = 0;
       break;
  }


 } /*end drm_initialisation */

/*;*<*>********************************************************
 * drm_scheduler
 *
 * When this function is called, it indicates that there is some
 * data available to be recorded. It is called 4 times a second.
 *
 * The data may be recorded at a varable rate, of up to 4 records
 * a second. If the data is not record at 4 times a second, then
 * the maximum values between each storage session are recorded
 *
 * The input type_sch is ignored.
 *   SCHE_FAST
 *   SCHE_BACKGND
 * 
 * Algorithim
 *    Get new data
 *    If storage rate not max then
 *        store maximum from new data
 *    When storeage time arrives
 *        store data (or maximums) in ram
 *
 **start*/
 void drm_scheduler( enum sche_entry type_sch)
 {
   Dplt_data *dplp;
   /*
    * Is the store capability of this module active
    */
   if (drm_store_active != DRMC_STORE_ON) {
      /* Not active */
      return;
   }
   /*
    * Get the information to be sent
    */
   mfd_supply_data(_ModuleDrm_,(void *)&dplp);

   /* Store raw data */
   drm_ram_store((char *)dplp,sizeof(Dplt_data));


   return;

   /* Does the incoming data have to be maximised
    *!/
   if (++data_pass < (int) drm_data_storage_rate) {
      data_sw = CLR_SEMAPHORE;
      /* store maximums of data *!/
      store_max_data(dplp,&max_data,_ModuleDrm_);

   }  else {
      data_pass = 0;
      zero_max_data(_ModuleDrm_);
      if (data_sw) {
         /* Use raw data *!/
         drm_ram_store((char *)dplp,sizeof(Dplt_data));
      } else {
        /* Use maximums of data *!/
        drm_ram_store((char *)&max_data,sizeof(Dplt_data));
      }
   }
   */
 } /*end drm_scheduler*/

/*;*<*>********************************************************
 * drm_data
 *
 * This function manupilates the data stored in banked ram.
 *
 *  DRMC_RESET - Clears the banked ram buffer and sets current_tupl_addr
 *               to begining of buffer;
 *  DRMC_1ST_TUPL & DRMC_CURRENT_TUPL sets up internal pointers
 *       so that DRMC_FETCH_FRWD_TUPL & DRMC_FETCH_BACK_TUPL
 *       can use them.
 *  DRMC_1ST_TUPL sets next access to 1st record
 *  DRMC_CURRENT_TUPL sets next access to last record stored
 *  DRMC_FETCH_FRWD_TUPL - gets the currently pointed to tupl and
 *                increments to point to next tupl. ie moves forward
 *                in time. Stops at end of recorded data.
 *  DRMC_FETCH_FRWD_TUPL_30_SEC - as above but jump back 30 seconds
 *  DRMC_FETCH_BACK_TUPL - gets the currently pointed to tupl and
 *                decrements to point to next tupl. ie moves backwards
 *                in time. Stops at begining of recorded data.
 *  DRMC_FETCH_BACK_TUPL_30_SEC - as above but jump back 30 seconds
 *  DRMC_DATA_RATE - sets the value to be the requested data rate.
 *  DRMC_PAUSE_ON
 *  DRMC_PAUSE_OFF

 *  DRMC_START     - start to feed data to mfd module
 *  DRMC_FINISH    - finish feeding data to mfd module
 *
 * Input
 *  control - action to be taken
 *  requestor  - subsystem requesting action SSP or UIM
 *  *address_of_buffer - for Dplt_data to be supplied in
 *
 * Returns
 *  for DRMC_RESET   INIT_TUPL_NUMBER
 *  for all others the value of the current tupl applicable to
 *            _ModuleSsp_ or every one else
 *
 **start*/
int drm_data(enum drme_control control,
             _eModuleId_ requestor,
             char *address_of_buffer)
 {
 /* persistent variables to this routine - need to be protected */
 static int current_ssp_tupl_num, current_dpl_tupl_num;
 
 register int tupl_num;
 long int tupl_addr;

   disable();
 
   /* Get tupl_num */
   if (requestor == _ModuleSsp_) {
      tupl_num = current_ssp_tupl_num;
   } else {
      /* Default assumes that is for the display and could be 
       * _ModuleDpl_ or _ModuleUif_ or _ModuleDrm_ */
      tupl_num = current_dpl_tupl_num;
   }

   switch (control) {
   case DRMC_RESET:         /* clears the buffer                    */
      drm_ram_control(&tupl_addr, /* Nothing is returned here */
                       control);
      tupl_num = INIT_TUPL_NUMBER;
      break;

   case DRMC_CURRENT_TUPL:/* Gets the addr of the current record  */
      tupl_num = drm_ram_control(&tupl_addr,control);
      drm_ram_get(&tupl_addr,address_of_buffer,sizeof(Dplt_data));
      break;

   case DRMC_1ST_TUPL:
      /* jumps to the beginning of the buffer */
      tupl_num = INIT_TUPL_NUMBER;
      break;

   case DRMC_FETCH_FRWD_TUPL_30_SEC:
#define DRMC_30_SECONDS_OF_TUPLS 60
      tupl_num += (DRMC_30_SECONDS_OF_TUPLS - 1);
      /* Fall through to fetching forward one tupl*/

   case DRMC_FETCH_FRWD_TUPL:
      /* gets a tupl and then increments the next fetch record           */
      if (++tupl_num >= last_stored_tupl_num) {
        /* Debug/Add the following some time
        strncpy(address_of_buffer,
                 &last_buf,   /* Constant data for end of buffer *!!!/
                 sizeof(Dplt_data));
         */
         tupl_num = last_stored_tupl_num;
      }
      tupl_addr = (long int)(tupl_num-1) * sizeof(Dplt_data);
      drm_ram_get(&tupl_addr,address_of_buffer,sizeof(Dplt_data));

      break;

   case DRMC_FETCH_BACK_TUPL_30_SEC:
      tupl_num -= (DRMC_30_SECONDS_OF_TUPLS - 1);
      /* Fall through to fetching backward one tupl*/

   case DRMC_FETCH_BACK_TUPL:
      /* gets a tupl and then decrements the next fetch record           */
      if (--tupl_num < TUPL_START_NUMBER) {
         tupl_num = TUPL_START_NUMBER;
      }
      tupl_addr = (long int)(tupl_num-1) * sizeof(Dplt_data);
      drm_ram_get(&tupl_addr,address_of_buffer,sizeof(Dplt_data));
      break;

  case DRMC_PAUSE_ON:
     drm_store_active |= DRMC_DATA_PAUSE;
     break;

  case DRMC_PAUSE_OFF:
     drm_store_active &= (~DRMC_DATA_PAUSE);
     break;
/*
   case DRMC_DPL_START:
      /* Start to feed data to mfd module 
       *   - Initialise buffer review_data and internal pointers
       *   - redirect input in mfd routines
       *!!!!!/
      drm_data(DRMC_CURRENT_TUPL,_ModuleDrm_,&review_data);
      drm_data(DRMC_FETCH_BACK_TUPL,_ModuleDrm_,&review_data);
       MfdSetInput (_ModuleDrm_,&review_data);
      break;

   case DRMC_DPL_FINISH:
      /* Finish feeding data to mfd module *!/
      MfdSetInput(_ModuleDrm_,NULL);
      break;

   case DRMC_DPL_FRWD_1:
      drm_data(DRMC_FETCH_FRWD_TUPL,_ModuleDrm_,&review_data);
      break;


   case DRMC_DPL_FRWD_1_MIN:
      break;

   case DRMC_DPL_BACK_1:
      drm_data(DRMC_FETCH_BACK_TUPL,_ModuleDrm_,&review_data);
      break;

   case DRMC_DPL_BACK_10_SEC:
      break;
   case DRMC_DPL_BACK_1_MIN:
      break;
*/

   case DRMC_DATA_RATE:
      /* sets the value in requestor to be the data rate */
      break;
   }

   /* Save tupl_num */
   if (requestor == _ModuleSsp_) {
      current_ssp_tupl_num = tupl_num;
   } else {
      /* See assumptions when tupl_num initialised */
      current_dpl_tupl_num = tupl_num;
   }

   enable();

   return tupl_num;

   
 } /*end drm_data*/

/*;*<*>********************************************************
 * store_max_data
 *
 * Takes the supplied data and compares it with the
 * values in *max, and stores the new value if it is
 * numerically larger.
 *
 * The data is not stored for the following conditions
 *    GAIN has changed 
 *    STATUS is not valid
 *
 **start*/
 void store_max_data(Dplt_data *dplp, /* Supplied data      */
                     Dplt_data *max,  /* Maximum data array */
                     _eModuleId_ module
 ) {

 } /*end store_max_data*/

/*;*<*>********************************************************
 * zero_max_data
 *
 * Indicates that the data of a particular type is not valid
 *
 **start*/
 void zero_max_data(_eModuleId_ Id)
 {

 } /*end zero_max_data*/


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

