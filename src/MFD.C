/* mfd.c
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
 Manage Filtered Data module - subprogram responsible for managing the
 access to the data outputs. The data outputs are produced by the Digital 
 Filtering Module or the Data Recording Module 
      * Filtered Data (from DFM or DRM)
      * Raw Data (only from DFM)

 The data is and consumed by the 

        * Display Module 
        * Serial Module
    * Data Recorder Module (can't be consumer/producer at same time)

This module sets/adapts the following indicators.
   time stamp - sets the time stamp value for each tupl it processes
   the gain value in tupls - changes this to TEST when the system is in
         test mode
  the DATA/REVIEW indicator - when the mfd_supply_data supplies data
         from any other source other than the filters, this bit is set in
         the status value

 Algorithms

 The mfd module functions are called in the following order
    mfd_initialization *** Only once ***
    mfd_supply_data    *** Everytime new data is required ***
    MfdSetInput      *** Defines source of mfd_supply_data
    mfd_control        *** Controls aspects of mfd
    mfd_acquire_data   *** To release new data ***

 Internal Data

 This module maintains the following internal data structures
  bargraph_new[] - The currently released positions of the
                   output
  
*/
#ifdef IC96
#include <h\cntl196.h>
#include <80C196.h>
#pragma nolistinclude
#endif

#include "general.h"
#include "iir.h"
#include "mm_coms.h"
#include "dim.h"
#include "proto.h"
#include "mfd.h"
#include "mmstatus.h"
#include "hw.h"
#ifdef SIMULATING
/*#include "pc_only.h" */
#endif

/* Constants */

/* Shared variables */

/* Externally defined */

/* Internal storage to this module */
/*
 * Captured filtered data
 */
 /*static*/ Mfdt_data filter_data;
/*
 * bargraph_new
 * This holds the requested positions of the bargraphs
 */
 Dplt_data bargraph_new;

/*
 * Pointer to determine where the input data is collected from
 *   IF NULL then use the output of the system filters
 *    other wise pointer has address of data
 */
 Dplt_data *pDplOutPointer; /* Display Module */
 Dplt_data *pDrmOutPointer; /* Data Recording Module */
/* Dplt_data *pSspOutPointer; /* Serial Module */

 /* Output Indication
 */
 _eMfdDataIn DataToSsp; /*FilteredData or RawData */
 _eMfdDataIn DataToDpl;/*FilteredData or RawData or BothDatad*/
 
/*
 * Time stamp - incremented every time a new tupl of data
 * is acquired.
 */
 int time_stamp;

/*
 * Montage indicator - added to very record.
 * contains mmstatus.h:MMS_MONTAGE_MASK information
 */
 int montage, bat_fail, recording_paused;
/*
 * Status information as defined in mmstatus.h
 */
 enum hwe_att_setting test_mode_status;

/* Internal prototypes */
 void translate_data(Mfdt_data *);

 /*;*<*>********************************************************
 * mfd_initialisation
 *
 * This function is invoked at initialization and is responsible
 * for the initialisation of this module
 *
 * A COLD initialisation zeros all buffers
 * A WARM initialisation acquires a valid set of data
 *
 **start*/
 enum inite_return mfd_initialisation(Initt_system_state *ssp)
 {
 int lobes_lp,fltr_lp;
 Mfdt_lobe_data *lobep;
 Frac *elemp;

 /* Set output of this module from the filters */
 pDplOutPointer = NULL;
 pDrmOutPointer = NULL;

 test_mode_status = HWE_BOTTOM_NULL;
 time_stamp = 0;
 

 if (ssp->init_type == COLD) {
    for(lobes_lp=0, lobep = /*(Mfdt_lobe_data *)*/ &filter_data.lobe_data[0];
        lobes_lp < NUMBER_LOBES;
        lobes_lp++,lobep++)
    {
       for(fltr_lp = 0, elemp = &lobep->elem[0];
           fltr_lp < NUMBER_FILTERS;
           fltr_lp++ ) {
      *elemp++ = 0;
       }
       lobep->emg    = 0;
       lobep->gain   = 0;
       lobep->input  = 0;
       lobep->status = 0;
    }
    
 } else {
    mfd_acquire(NUMBER_FILTERS);
 }
 montage = bat_fail = recording_paused = 0;
 DataToDpl = DataToSsp = _FilteredData_;
 
 return PASS;
 } /*end mfd_initialisation*/

 /*;*<*>********************************************************
 * mfd_supply_data
 *
 * This function supplies a pointer to the current displayable
 * outputs
 *
 * This function may be called from interrupt and background routines
 *
 *Input:
     Id:
     _ModuleDpl - this is for screen - only filtered data
     _ModuleDrm - only filtered data
     _ModuleSsp - filtered or raw data
 **start*/
 edlle_type mfd_supply_data(_eModuleId_ Id, /* In: Requestor, */
                void **pData   /*Out: Pointer to data */
               )
 {
   if ((Id == _ModuleSsp_) && (DataToSsp == _RawData_) ) {
       /* Raw Data to Ssp */
       *pData = DimRtrvSnapRaw();
       return DLLE_RT_RAW; /* Exit */
   }

   if ( pDplOutPointer == NULL ) {
      /*
       * Return with the address of the data
       * Only valid till next
       *      mfd_acquire()
       */

      *pData = &bargraph_new;
      return DLLE_RT;
      
   } else {
      /*
       * Output is not from filters
       * Update status byte
       */
      pDplOutPointer->status |= MMS_DM_REVIEW;
      *pData = pDplOutPointer;
      return DLLE_RT;
   }

 } /*end mfd_supply_data */

 /*;*<*>********************************************************
 * MfdSetInput
 *
 * This function determines where the mfd_supply_data() gets
 * its data from.
 *
 * See definition of Dplt_data *pDplOutPointer
 * The sources are
 *            Filter Outputs
 *            Client supplied
 *
 *
 **start*/
 void MfdSetInput(_eMfdSetInput Id,/*In: The module whos output
                     is being set. */
            void *p)/*In: Where data is coming from
                          Null has special meaning */
 {

     switch (Id) {
        case _ModuleDplFiltered:
        case _ModuleDrmFiltered:
            DataToDpl = _FilteredData_;
            pDplOutPointer = (Dplt_data *) p;
            pDrmOutPointer = (Dplt_data *) p;
            break;

        case _ModuleSspFiltered:
            DataToSsp = _FilteredData_;
            /*pSspOutPointer = (Dplt_data *) p;*/
            break;
            
        case _ModuleSspRaw:
            DataToSsp = _RawData_;
            /*pSspOutPointer = (Dplt_data *) p;*/
            break;

     }
 } /*end MfdSetInput*/

 /*;*<*>********************************************************
 * mfd_acquire
 * 
 * This function acquires data from the digital filter module and
 * stores it for local access
 *
 **start*/
 void mfd_acquire(int number_elem)
 {

    disable(); /* insure no other access */
    dfm_peak_filter_values(number_elem,&filter_data);
    enable();
    /*
     * Translate the filtered data to an internal representation
     */
    translate_data(&filter_data);

    /*
     * Add a unique indicator to each tupl
     */
    bargraph_new.time_stamp = time_stamp++;

    /* Add status indications to tupl
     *    - Default settings
     *    - current montage setting
     */
    bargraph_new.status = MMS_INIT_STATE | montage | bat_fail | recording_paused;

    /* Check if gain needs updating
     */
    if (test_mode_status != HWE_BOTTOM_NULL) {
       bargraph_new.lobe_data[LF_LOBE].gain = (int) test_mode_status;
    }
 
 } /*end mfd_acquire*/

 /*;*<*>********************************************************
 * translate_data
 *
 * The incoming structure *indp has 16 bit absolute (all +ve ie 
 * bit15=0) values. Hence there are 15 bits of data left shifted.
 *
 * The 8 data bits in bits 7->14 are copied into array
 * bargraph_new. Empirically it was found that a further 6 left shifts
 * were needed.
 *
 * The emg value is setup as element[0].
 *
 **start*/
 void translate_data(Mfdt_data *indp)
 {
 Mfdt_lobe_data *lbp=&indp->lobe_data[0];
 register Frac *fp,input;
 register Wordbyte reg;
 register Byte *arrayp;
 register int lobe_lp, elem_lp;
 register Dplt_lobe_data *bar_newp = &(bargraph_new.lobe_data[0]);
 register Temp0;
 
#define TRANSLATE_SHIFT 5 /* O/P attenuation - 5 approx 16 times*/
#define TRANSLATE_MASK (~(0xFFFF >> TRANSLATE_SHIFT))
#define _MaxEmgIn_ 0x7f
#define _MaxEmgOut_ 0xff
#define CapEmg(data) ((Byte)((data<_MaxEmgIn_) ? (data<<1):_MaxEmgOut_))
/*#define CapEmg(data) (Byte)data*/
 
    for (lobe_lp = 0; lobe_lp < NUMBER_LOBES; lobe_lp++,lbp++,bar_newp++) {
       fp = &(lbp->elem[0]);
       arrayp = &(bargraph_new.lobe_data[lobe_lp].bar[0]);
       /*
    * Update emg element - put it in first elem
    * herafter it is treated as a filter element
    */
       Temp0 = 0xff & lbp->emg;
       *(arrayp++) = CapEmg(Temp0);
       for (elem_lp = 0; elem_lp < NUMBER_FILTERS; elem_lp++,arrayp++) {
          /* 
           * Check for overflow 
           */
      if ( (reg.word = (*fp++)) & TRANSLATE_MASK) {
             /* Has exceeded maximum - limit to maximum */
             reg.word = 0xffff;
      }
          reg.word <<= TRANSLATE_SHIFT;/*This does the translation*/
      *arrayp = reg.byte.ah;
       }
       bar_newp->gain   = lbp->gain ;
       /*bar_newp->input  =lbp->input;/* input removed from Dplt_lobe_data*/
       /*bar_newp->status =lbp->status;/*status removed from Dplt_lobe_data*/
    }
 } /*end translate_data */

 /*;*<*>********************************************************
 * mfd_control
 *
 * Updates aspects of internal status indicators
 *
 *    MFDC_TIME_STAMP   - updates the time stamp value
 *    MFDC_TEST_MODE    - updates the test mode value
 *
 * Note: This is fairly primitive and not objected orientated,
 * it does however have to work with the values in mmstatus.h
 *
 **start*/
 void mfd_control(enum mfde_control type,int value)
 {
   switch (type) {
   case MFDC_TIME_STAMP:
      time_stamp = value;
      break;
   case MFDC_TEST_MODE:
      test_mode_status = (enum hwe_att_setting) value;
      break;
   case MFDC_MONTAGE:
      montage = value;
      break;
   case MFDC_BAT_FAIL:
      bat_fail = value;
      break;
   case MFDC_PAUSE:
      recording_paused = value;
      break;

   }
 } /*end mfd_control*/

 /*;*<*>********************************************************
 * 
 **start*/
/* ()
 {

 } /*end*/

 /*;*<*>********************************************************
 * 
 **start*/
/* ()
 {

 } /*end*/

 /*;*<*>********************************************************
 * 
 **start*/
/* ()
 {

 } /*end*/





