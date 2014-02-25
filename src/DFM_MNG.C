/* dfm_mng.c
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
 This modules responsibility is to manage the digital filters.

 These consist of a number of pass band filters invoked
         Low Pass Filter (256 times/sec every 4mS)
         128 times/sec - every 8mS
         Low Pass Filter (128 times/sec)
         64 times/sec - every 16mS
         Low Pass Filter (64 times/sec)
         32 times/sec - every 31mS

 Algorithm
   dfm_initialisation() called once at startup.
   dfm_schedule() to process the new digitized values.
   dfm_filter_description() to return a description of a filter.
   dfm_control() - to control this module

   dfm_digitized_values() to present new digitized values.

*/
#ifdef IC96
#pragma extend /* To allow reentrant function */
#pragma noreentrant
#pragma regconserve(FSCOPE)
#include <h\ncntl196.h>
#pragma nolistinclude
#pragma type
#pragma nodebug
#endif
#include <80C196.h>
#include <stdlib.h>
#include "general.h"

#include "iir.h"
#include "mm_coms.h"
#include "dim.h"
#include "proto.h"
#include "dfm.h"

/*
 * The following include is very complex and forms part of this
 * modules functions. It includes the storage for all delay sections.
 */
#ifdef IC96
/*#pragma listinclude*/
#endif
#include "filters.h"
#ifdef IC96
#pragma nolistinclude
#endif

/*************************
 * Internal enums/typedefs 
 */

/* Constants */
#define FIRST_LPF (NUMBER_FILTERS+LP_128MS_ENTRY)
#define SECND_LPF (NUMBER_FILTERS+LP_64MS_ENTRY)

#define START_128MS_ELEMENT (NUMBER_FILTERS-FIRST_128MS_ELEMENT-1)
#define START_64MS_ELEMENT  (NUMBER_FILTERS-FIRST_64MS_ELEMENT-1)
#define START_32MS_ELEMENT  (NUMBER_FILTERS-FIRST_32MS_ELEMENT-1)

#define MASK_64HZ 0x1 /*LSB_MASK 0x1*/
#define MASK_32HZ 0x3 /*TWO_LSB_MASK 0x3*/

/*******************
 * Shared variables 
 */
/* Externally defined variables */
 extern Longword list_main;

/**************************************************************
 *
 * Internal storage to this module 
 */
 /* dfmv_control - controls filtering
  *   Access through dfm_control
  *   0 disables filtering
  *   !0 enables filtering
  */
 register int dfmv_control;

/*
 * dfmu_data
 *
 * The output of all the filter sections
 * Note: The data is stored as follows
 *  filter data stored as follows
 *    1st (38Hz)      - dfmu_data.lobe_data[NUMBER_FILTERS-1]
 *    2nd             -     "              [NUMBER_FILTERS-2]
 *     "              -     "               "
 *    NUMBER_FILTERS  - dfmu_data.lobe_data[0]
 */
 Dfmt_data dfmu_data;
#ifndef TC
/* register static /* TC doesn't like this */
#endif /* SIMULATING */
/* Dfmt_lobe_data *dfmp; */

/*
 * time_list - a variable that is incremented on every time the
 * dfm_schedule() is called
 * Range: 0 to NUMBER_TIME_SLOTS-1
 */
#ifndef TC
 register static /* TC doesn't like this */
#endif /* SIMULATING */
 int time_list;

/*
 * Pointers used while processing filters
 */
#ifndef TC
 register static /* TC doesn't like this */
#endif /* SIMULATING */
 Tbl_filter_descriptors *iirp,/* Input to iir routine */
                        *iir128p,/*Temp holding reg 128Hz section*/
                        *iir64p,/* Temp holding reg 64Hz section */
                        *iir32p;/* Temp holding reg 32Hz section */
#ifndef TC
/* register static /* TC doesn't like this */
#endif /* SIMULATING */
/* Iir_filter_descriptor *iirlpp; */
#ifndef TC
 register static /* TC doesn't like this */
#endif /* SIMULATING */
 Frac *outp,                  /* Output from iir routine*/
      *out128p[NUMBER_LOBES],
      *out64p[NUMBER_LOBES],
      *out32p[NUMBER_LOBES],
      **outptrp;

#ifndef TC
 register static /* TC doesn't like this */
#endif /* SIMULATING */
 int lowpass_output[NUMBER_LOBES],/*Output from low pass iir*/
              *outlpp;
#ifndef TC
 register static /* TC doesn't like this */
#endif /* SIMULATING */
 int input, /* Temp register */
             output,
      number_passes;

#ifndef TC
 register static /* TC doesn't like this */
#endif /* SIMULATING */
 int filter_lp /*, Temp for(){} registers */
                /*lobe_lp */;

/*
 * Store the values of the filter outputs
 * Used for debugging
 */
#ifdef OUTPUT_CAPTURE
 Frac last_output[NUMBER_LOBES][NUMBER_FILTERS+NUMBER_LP_FILTERS];
#endif
/**************************************************************
 * Internal Prototypes
 */
 Word abs_max(Word in1, Frac in2);

/*;*<*>********************************************************
 * 
 * dfm_initialisation
 *
 * This function is responsible for initialising the digital 
 * filtering module.
 *
 * The algorithm is
 *  Initialises all the filter delay sections.
 *
 **start*/
 enum inite_return dfm_initialisation(Initt_system_state *ssp)
 {
#ifdef OUTPUT_CAPTURE
 int filter_lp,lobe_lp;
#endif /* OUTPUT_CAPTURE */
 Time_table  *stp=&time_table[0];
 Tbl_filter_descriptors *tip;

    /*
     * Initialise statics
     */
    time_list = 0;
    dfm_control(DFMC_FILTERING_ON); /* Enable filtering in this module */

    /*
     * Initialise 128, 64 and 32 list filters
     */
    while(stp->t != (void *) NULL) {
       tip = stp->t;
       while(tip->p != (void *) NULL) {
	  initialise_iir_i(tip->p,NUMBER_LOBES);
	  tip++;
       }
       stp++;
    }
    /*
     * Initialise Low Pass filters
     */
#ifdef DO_LP_PROCESSING
    initialise_iir_i(tbl_lp_filter_descriptors[LP_128MS_ENTRY].p,NUMBER_LOBES);
    initialise_iir_i(tbl_lp_filter_descriptors[LP_64MS_ENTRY].p,NUMBER_LOBES);
#endif
#ifdef OUTPUT_CAPTURE
    for (lobe_lp = 0; lobe_lp < NUMBER_LOBES; lobe_lp++) {
       for(filter_lp = 0; filter_lp < NUMBER_128MS_FILTERS; filter_lp++) {
          last_output[lobe_lp][filter_lp] = 0;
       }
    }
#endif /* OUTPUT_CAPTURE */
    return PASS;
 } /*end dfm_initialisation */

/*;*<*>********************************************************
 * dfm_schedule
 *
 * This function is called to schedule the digital filter. It is assumed
 * that it is called every 256mS. This routine invokes the appropiate
 * digital filters.
 *
 * There are three lists that filters are called on
 *    8mS/128Hz  - bandpass + 32Hz lp filter
 *   16mS/64Hz   - bandpass + 16Hz lp filter
 *   31mS/32Hz   - bandpass
 * 
 * All the  128ms routines are called every pass
 *
 * The 64mS routines are split between 2 passes as defined by the
 *   constants
 *      NUMBER_64_1ST_PASS
 *      NUMBER_64_2ND_PASS
 *
 * The 32ms routines are split between 4 passes as defined by the
 *   constants
 *      NUMBER_32_1ST_PASS
 *      NUMBER_32_2ND_PASS
 *      NUMBER_32_3RD_PASS (if needed)
 *      NUMBER_32_4TH_PASS (if needed)
 *   these are stored in array number_32[]
 * 
 * Note: this routine is psuedo-reentrant.
 *    Due to processing limitations, when the full set of filters
 *    is invoked they don't finish in the 4mS between it and the
 *    next software interrupt. When this interrupt comes in, it only
 *    processes the LP_64HZ filter, before exiting. Hence this path is
 *    re-entrant.
 *    Because dfm_iir_i() is not reentrant, a second routine has
 *    been created. This may be illimanted in the future by
 *        - using the PTS to collect the A/D, and only leaving the
 *          sw interrupt enabled. Henc the ad_complete interrupt
 *          would only be serviced when this routine completes.
 *     OR - using the WSR in some fashion.
 *
 **start*/
 void dfm_schedule()
 {
/* register Iir_filter_descriptor *iirlpp; */
 register Dfmt_lobe_data *dfmp;
 register lobe_lp;
 Dfmt_data lp256hz_only; /* used to make 64HZ LP filter re-entrant*/

    /****************************************************
     * Check whether filtering is enabled
     */
    if ((int)dfmv_control == (int)DFMC_FILTERING_OFF)
       return;

    /****************************************************
     * 64Hz Low Pass Filter
     *
     * Get new data and do the filtering. This section
     * must be reentrant.
     *
     * Filter all frequencies above 64Hz out of the input
     * data stream. Save every second sample, so that all
     * the filters are working on the same data.
     *
     * 256 Hz sampling freq, filter out high frequencies for
     *  128Hz sampling of next stage
     */
#define LP256HZ_ONLY 0x1
    if ( list_main.word.low & LP256HZ_ONLY ) {
       /*
        * Just doing low pass filtering this pass
        * Output is thrown away, but the low pass filter
        * must run to update its internal values.
        */
       DimRtrvRaw(&lp256hz_only);/* get new data - not used elsewhere*/
       dfmp = &(lp256hz_only.lobe_data[0]);
    } else {
       DimRtrvRaw(&dfmu_data); /* get new data */
       dfmp = &(dfmu_data.lobe_data[0]);
    }
    for (lobe_lp = 0; 
         lobe_lp < NUMBER_LOBES;
         lobe_lp++, dfmp++ ) {
       /* Do LP filter */
       dfmp->input128 = dfm_iir_i_lp256(
             tbl_lp_filter_descriptors[LP_256MS_ENTRY].p,
                                             dfmp->input,
                                                  lobe_lp
                                  );
    }

    if ( list_main.word.low & LP256HZ_ONLY ) {
      /*
       * Just do low pass filtering
       */
      return;
    }

    /************************************
     * Do all 8mS/128Hz filters for all lobes
     */
    dfmp = &(dfmu_data.lobe_data[0]);
    for (lobe_lp = 0; lobe_lp < NUMBER_LOBES; 
         lobe_lp++, dfmp++) {
       iirp = time_table[TABLE_128MS].t;
       outp = &(dfmp->elem[START_128MS_ELEMENT]);
       /*
        * Do filters for current lobe
        */
       for(filter_lp=0; 
           filter_lp<NUMBER_128MS_FILTERS;
           filter_lp++, iirp++, outp--){
          /* Do filter */
          output = dfm_iir_i(iirp->p,
                      dfmp->input128,
                             lobe_lp );
	  *outp = abs_max(*outp,output);
#ifdef OUTPUT_CAPTURE
          last_output[lobe_lp][START_128MS_ELEMENT - filter_lp] = output;
#endif
       }
    }

    /****************************************************
     * 32Hz Low Pass Filter
     *
     * Filter all frequencies above 32Hz out of the input
     * data stream. Save every second sample, so that all
     * the filters are working on the same data.
     *
     * 128 Hz sampling freq, filter out high frequencies for
     *  64Hz sampling of next stage
     */
#ifdef DO_LP_PROCESSING
/*    iirlpp = tbl_lp_filter_descriptors[LP_128MS_ENTRY].p;*/
    dfmp = &(dfmu_data.lobe_data[0]);
    outlpp = &lowpass_output[0];
    for (lobe_lp = 0; 
         lobe_lp < NUMBER_LOBES;
         lobe_lp++, dfmp++, outlpp++) {
       /* Do LP filter */
       *outlpp = dfm_iir_i(tbl_lp_filter_descriptors[LP_128MS_ENTRY].p,
                   dfmp->input128,
                          lobe_lp);
#ifdef OUTPUT_CAPTURE
       last_output[lobe_lp][FIRST_LPF] = *outlpp;
#endif /*OUTPUT_CAPTURE*/
    }
#endif /*DO_LP_PROCESSING*/

    /*
     * Use every second output
     */
    if(!(time_list & MASK_64HZ)) {
       /* A periodic update (64 times a second) is done so that all
        *  16mS sampling time filters use the same data.
        */
       dfmp = &(dfmu_data.lobe_data[0]);
       outlpp = &lowpass_output[0];
       for( lobe_lp=0; 
            lobe_lp < NUMBER_LOBES;
            lobe_lp++, dfmp++, outlpp++) {
	  dfmp->input64 =
#ifdef DO_LP_PROCESSING
	       *outlpp;
#else
	       dfmp->input128;
#endif /* DO_LP_PROCESSING */
       }
    }
    /*************************
     * 16mS/64Hz list
     *
     * Determine which set of filters are to be run this pass
     */
    if(!(time_list & MASK_64HZ)) {
       /*
        * Its the first pass of the 64mS list
        */
       iir64p = time_table[TABLE_64MS].t;
       outptrp = &out64p[0];
       dfmp = &(dfmu_data.lobe_data[0]);
       for(lobe_lp=0; 
           lobe_lp < NUMBER_LOBES; 
           lobe_lp++, outptrp++, dfmp++) {
          *outptrp = &(dfmp->elem[START_64MS_ELEMENT]);
       }
       number_passes = NUMBER_64_1ST_PASS;
    } else {
       /*
        * Its the second pass of the 64mS list
        */
       number_passes = NUMBER_64_2ND_PASS;
    }

    /*******************************
     * Do 16mS/64Hz filters for this pass
     */
    outptrp = &out64p[0];
    dfmp = &(dfmu_data.lobe_data[0]);
    for (lobe_lp = 0; 
         lobe_lp < NUMBER_LOBES; 
         lobe_lp++, outptrp++, dfmp++) {
       iirp = iir64p;
       outp = *outptrp;
       /*
        * Do filters for current lobe
        */
       for (filter_lp=0; 
            filter_lp < number_passes; 
            filter_lp++, iirp++, outp--) {
          /* Do filter */
          output = dfm_iir_i( iirp->p,
                        dfmp->input64,
                              lobe_lp );
          *outp = abs_max(*outp,output);
#ifdef OUTPUT_CAPTURE
          last_output[lobe_lp][START_64MS_ELEMENT - filter_lp] = output;
#endif
       }
       /*
        * Save this particular output pointer for next pass
        */
       *outptrp = outp;
    }
    /* Save the input pointer for next pass 
     * - only applicable to the end of the 1st pass, 
     * but do for the 2nd pass as well
     */
    iir64p = iirp;


    /****************************************************
     * 16Hz Low Pass Filter
     *
     * Filter all frequencies above 16Hz out of the input
     * data stream.
     * Since this routines effective sample rate is 
     * 64 Hz, it is only invoked every second pass.
     *
     * Save every second output of the invocation, so that all
     * the filters are working on the same data.
     *
     */
    if(!(time_list & MASK_64HZ) ) {
#ifdef DO_LP_PROCESSING
/*     iirlpp = tbl_lp_filter_descriptors[LP_64MS_ENTRY].p; */
       dfmp = &(dfmu_data.lobe_data[0]);
       outlpp = &lowpass_output[0];
       for (lobe_lp = 0; 
            lobe_lp < NUMBER_LOBES; 
            lobe_lp++,dfmp++, outlpp++ /*njh*/) {
          /* Do LP filter */
          *outlpp = dfm_iir_i(tbl_lp_filter_descriptors[LP_64MS_ENTRY].p,
                       dfmp->input64,
                             lobe_lp);
#ifdef OUTPUT_CAPTURE
          last_output[lobe_lp][SECND_LPF] = *outlpp;
#endif /*OUTPUT_CAPTURE*/
       }
#endif /*DO_LP_PROCESSING*/
       /* 
        * Use every second output
        */
       if (!(time_list & MASK_32HZ)) {
          /* A periodic update (32 times a second) is done so that
           * all 32mS sampling time filters use the same data.
           */
          dfmp = &(dfmu_data.lobe_data[0]);
          outlpp = &lowpass_output[0];
          for (lobe_lp=0; 
               lobe_lp < NUMBER_LOBES; 
               lobe_lp++, dfmp++, outlpp++) {
             dfmp->input32 =
#ifdef DO_LP_PROCESSING 
                  *outlpp;
#else
                  dfmp->input128;
#endif /* DO_LP_PROCESSING */
          }
       }
    }

    /***********
     * 32mS/32Hz list
     *
     * Determine which set of filters are to be run this pass
     */
    switch (time_list) {
    case 0:
       /*
        * Its the first pass of the 32mS list
        */
       iir32p = time_table[TABLE_32MS].t;
       dfmp = &(dfmu_data.lobe_data[0]);
       outptrp = &out32p[0];
       for(lobe_lp=0; 
           lobe_lp < NUMBER_LOBES; 
           lobe_lp++,outptrp++,dfmp++) {
          *outptrp = &(dfmp->elem[START_32MS_ELEMENT]);
       }
       number_passes = NUMBER_32_1ST_PASS;
       break;

    case 1:
       /*
        * Its the second pass of the 64mS list
        */
       number_passes = NUMBER_32_2ND_PASS;
       break;

    case 2:
       /*
        * Its the third pass of the 64mS list
        */
       number_passes = NUMBER_32_3RD_PASS;
       break;

    case 3:
       /*
        * Its the fourth pass of the 64mS list
        */
       number_passes = NUMBER_32_4TH_PASS;
       break;

    default:
       number_passes = 0;
       break;
    }
    /*******************************
     * Do 32mS filters for this pass
     */
    outptrp = &out32p[0];
    dfmp = &(dfmu_data.lobe_data[0]);
    for (lobe_lp = 0; lobe_lp < NUMBER_LOBES; 
         lobe_lp++, outptrp++, dfmp++) {
       iirp = iir32p;
       outp = *outptrp;
       /*outp = out32p[lobe_lp]; */
       /*
        * Do filters for current lobe
        * 'number_passes' may be 0
        */
       for (filter_lp=0; filter_lp < number_passes; 
            filter_lp++, iirp++, outp--) {
          /* Do filter */
          output = dfm_iir_i(iirp->p,
                       dfmp->input32,
                             lobe_lp );
          *outp = abs_max(*outp, output);
#ifdef OUTPUT_CAPTURE
          last_output[lobe_lp][START_32MS_ELEMENT - filter_lp] = output;
#endif
       }
       /*
        * Save this particular output pointer for next pass
        */
       *outptrp = outp;
    }
    /*
     * Save the input pointer for next pass 
     * Only needed for the 1st, 2nd and 3rd passes,
     * but not for the last pass.
     */
    iir32p = iirp;

    /*
     * Increment for next pass or else wrap around to zero
     */
    if (++time_list >= NUMBER_TIME_SLOTS)
       time_list = 0;

 } /*end dfm_schedule*/

/*;*<*>********************************************************
  * dfm_filter_description
  *
  * This function returns a description of the requested filter
  *
  * The filter number returned is as follows
  *     element
  * element range: 0 to TOT_NUM_FILTERS-1
  *
  **start*/
 Iir_filter_descriptor *dfm_filter_description(Word elem)
 {
 Tbl_filter_descriptors *tp;
 
 Iir_filter_descriptor *iirp;
    /*
     * Compensate for filters counted in reverse order
     * between 0 and NUMBER_FILTERS-1
     */
    elem=dfm_adjust_elem(elem);

    if (elem  < FIRST_64MS_ELEMENT) {
       /* in the 128MS table */
       tp = time_table[TABLE_128MS].t;
	/* elem offset is from 0 */

    } else if (elem < FIRST_32MS_ELEMENT) {
       /* in the 64ms table */
       tp = time_table[TABLE_64MS].t;
       elem -= FIRST_64MS_ELEMENT;

    } else if (elem < (TOT_NUM_IMPL_BP_FILTERS)) {
       /* in the 32ms table */
       tp= time_table[TABLE_32MS].t;
       elem -= FIRST_32MS_ELEMENT;

    } else if ( elem < (TOTAL_NUMBER_FILTERS)) {
       if ( elem < NUMBER_FILTERS) {
	  /* filter not in use yet */
	  return NULL;
       } else {
#ifdef DO_LP_PROCESSING
	  /* filter in the low pass filter table */
	  tp = tbl_lp_filter_descriptors;
	  elem -= NUMBER_FILTERS;
#else
	  return NULL;
#endif
       }
    } else {
        return NULL;
    }
    tp += elem; /* adjust to point to the right filter */
    iirp = tp->p; /* get the filter descriptor         */
    return iirp;

 } /*end dfm_filter_description*/

/*;*<*>********************************************************
  * dfm_control
  *
  **start*/
 enum inite_return dfm_control(enum dfme_control type)
 {
   switch ( type) {
   case DFMC_FILTERING_OFF:
      dfmv_control = 0;
      break;

   case DFMC_FILTERING_ON:
      dfmv_control = 1;
      break;

   }

 } /*end dfm_control*/

/*;*<*>********************************************************
  * 
  **start*/
/* ()
 {

 } /*end*/


