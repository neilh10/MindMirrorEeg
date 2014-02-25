/* dim.c
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

 Data Import Module - subprogram responsible for acquiring the
 incoming data.

    dim_initialisation
    dim_new_data
    DimRtrvRaw     - Raw data
    DimSnapRaw - Snap Raw Data from A/D input
    DimRtrvSnapRaw  - set of samples packed


*/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#include <80C196.h>
#endif
/*#include <stdio.h>*/

/* Defined for testing */
/*#define RW_TEST 1 */

#include "general.h"
#include "iir.h"
/*#include "init.h"*/
#include "hw.h"
#include "mm_coms.h"
#include "dim.h"
/*#include "proto.h" */
#ifdef SIMULATING
//#include "pc_only.h"

// extern  
//{
#endif
/* Constants */

/* Shared variables */

/* Externally defined */
 extern Frac ad_result[/*NUMBER_AD_CONVERTERS*/];
 /*extern int montage, bat_fail; /* mfd.c*/
 extern Dplt_data bargraph_new;


/*********************************
 * Internal storage to this module
 */

/* dimu_input_data contains input captured data.
   All the accesses to this data are expected to be
   at the same level so that there is no contention for
   the data. 
  
 */
 /*static*/ Dimt_data dimu_input_data[1];
#define ACTIVE_BANK 0 /* Only one bank allowed */

 /* RtPktBuf[] contains the stream of data that has been
    captured. The storage of data is in a number of buffers to facilitate
    complete capture of a stream of data.
    
   The storage of the data is in two banks, 
       an active bank which is avaliable for any routines
         to call via dim_retrieve().
       an inactive bank which is updated with incoming data.
  
   'SnapBuf' the bank that is active.
   'SnapBufReady' the most recently available buffer
   'SnapCntr' the counter indicates where the incoming data is stored

   The size is large enough, so that a 2nd history buffer can be copied in
   when it is ready.
 */
 tRtPktBuf RtPktBuf[_DimNumSnapBufs_];

static int SnapBuf;
#define _InitSnapBank_ 0
 int SnapCntr;
/* uchar SnapStarted[NUMBER_LOBES]; /* need to initialise */
#define _InitSnapCntr_ 0
uchar SnapBufReady;     /* Most recent ready buffer */

 /* Exported Indications*/
  uchar DimRawPkdDataSemaphore; /* Set when ready
                            Reset when task initiated
                          */
  
 /* Stores the last snapshot taken*/ 
 int LastSnap[NUMBER_LOBES];
 

 /*;*<*>********************************************************
 * dim_initialisation
 *
 * This function initialises the data import module
 * 
 *
 **start*/
 enum inite_return dim_initialisation(Initt_system_state *ssp)
 {
	 DimSnapRaw(_DimInit_);
     DimSnapRaw(_DimStart_);
	 /* Retrieve the set of results captured with _DimStart_ 
        so that the background doesn't receive an indication to use them */
     DimRtrvSnapRaw();
	 
  return PASS;
 } /*end dim_initialisation */


 /*;*<*>********************************************************
 * dim_new_data
 *
 * When this function is called, it populates 
 *     dimu_input_data[0].lobe_data[]
 * for the LF_LOBE & RF_LOBE
 *
 **start*/
 void dim_new_data()
 {
 register Dimt_lobe_data *dimlp = &(dimu_input_data[0].lobe_data[LF_LOBE]);
 register Dimt_lobe_data *dimrp = &(dimu_input_data[0].lobe_data[RF_LOBE]);

      /* Collect left lobe */
      /* disable(); not needed as done at interrupt level*/
      dimlp->input = ad_result[HW_AD_LB_LOBE]; 
      dimlp->emg   = high_byte(ad_result[HW_AD_LB_EMG]);
      dimlp->gain  = (int) hw_att_value(0);
      /*dimlp->status = ? */
      /* Collect right lobe */
      dimrp->input = ad_result[HW_AD_RB_LOBE]; 
      dimrp->emg   = high_byte(ad_result[HW_AD_RB_EMG]);
      dimrp->gain  = (int) hw_att_value(0);
      /*dimrp->status = ? */
      /* enable();*/
      
 } /*end dim_new_data */

 /*;*<*>********************************************************
 * DimRtrvRaw
 * 
 * When this function is called, it takes a copy of the latest data.
 * It guarentess that the data copied will be copied in a single 
 * transaction.
 *
 **start*/
 void DimRtrvRaw(Dfmt_data *digp)
 {
 register int lp;
 register Dimt_lobe_data *ip;
 register Dfmt_lobe_data *ldp=&(digp->lobe_data[0]);

    /* disable(); not needed as done in dim_new_data*/
    ip = &(dimu_input_data[ACTIVE_BANK].lobe_data[0]);
    for (lp=0;lp < NUMBER_LOBES; lp++,ip++,ldp++) {
       ldp->input = 
#ifndef SIMULATING
           init_cnvt_frac(ip->input);
#else
           ip->input;
#endif /* SIMULATING */

       ldp->emg      = ip->emg;
       ldp->gain     = ip->gain;
       ldp->status   = ip->status;
    }
    /* enable();*/


 } /*end DimRtrvRaw */

 /*;*<*>********************************************************
 *  DimSnapRaw

    Captures the analog readings in a buffer in a series of calls.
    This is then recovered using DimRtrvSnapRaw()

    This is called from interrupt level with _DimSnap_ & _DimStart_
    while other routines reference sempahore from background.
    
    Input:
       type
        _DimInit_ - Initialise at creation 
        _DimStart - Indicate a new capture sequence starting.
                    System status captured,
                    EMG peak captured in last buffer,
                    buffer released for transmission, and new allocated.
                    no EEG samples taken
        _DimSnap_  - Snap an Analog reading and store differential
                    in the current buffer

 **start*/
 void DimSnapRaw(eDimdType Type)
 {
 register int lp;
/* register Dfmt_lobe_data *ldp=&(digp->lobe_data[0]);*/
/* register tDlltRtRaw *pRaw;*/
 register tDlltRtRawLobe *pRawLobe;
 register int LastSnapTemp; /*Temporary i/p from AdInput before stored in LastSnap[]*/
 register int diff; /* Difference between old and new values */
 register Wordbyte AdInput;
 register Frac *pAd;

 static Wordbyte Emg[NUMBER_LOBES];

#define _InitEmg_ 0
 
 if (Type == _DimStart_) {
    /* Snapshot the EMG - this is called 8 times a second
     */ 
    for (lp=0; lp <NUMBER_LOBES; lp++) {
       /*Store Maximum of Emg from last pass*/
       /* Emg is not initialised 1st pass through*/
        pRawLobe = &((tDlltRtRaw *)RtPktBuf[SnapBuf].Drp.buf)->lobe[lp];
#ifdef RW_TEST
       AdInput.word = (int) &RtPktBuf[SnapBuf].Drp
       pRawLobe->EmgL = AdInput.byte.al; /* Debug */
       pRawLobe->EmgH = AdInput.byte.al; /* Debug */      
#else
       /* Capture from previous peak
       /* pRawLobe->EmgL = Emg[lp].byte.al;
       pRawLobe->EmgH = Emg[lp].byte.ah; */

       /* Debug */
       pRawLobe->EmgL = DLL_RT_RAW_SIZE;
       pRawLobe->EmgH = sizeof(tDlltRtRawLobe);
#endif
       Emg[lp].word = _InitEmg_;
    }
	/* Start New Buffer */
    SnapCntr = _InitSnapCntr_;
    SnapBufReady = SnapBuf; /* Full Buffer*/
	/* Start a new buffer */
	if (++SnapBuf >= _DimNumSnapBufs_){
	   SnapBuf = _InitSnapBank_;
	} 

#ifdef RW_TEST
    pRaw = (tDlltRtRaw *)RtPktBuf[SnapBuf].Drp.buf;
    pRaw->status = (char)(((int)(&RtPktBuf[SnapBuf].Drp))>>8); /*High*/ 
    /*pRaw->log_att = hw_att_value(0); */
#endif
    
    DimRawPkdDataSemaphore = 1;
  
 } else if (Type == _DimInit_) {
	 /* Initialise */
	SnapBuf = _InitSnapBank_;
    Emg[0].word = _InitEmg_;
    Emg[1].word = _InitEmg_;
 } else {  /* _DimSnap_ */
    pAd = &ad_result[HW_AD_LB_LOBE];

    for (lp=0;lp < NUMBER_LOBES; lp++,pAd++) {
       /* Capture EEG data
	   ** 030114: store full sample first pass - needs doing
	      This is called every 8mS - should eventually be 4mS
	      Derive signed char difference from incomng data
	      and stick in snapshot buffer
          Get raw data, and adjust it for the fact it is part of upper 10 bits
          */
#define _AdcShlAdjust_ 6  
       AdInput.word = ((*pAd  >>_AdcShlAdjust_) & 0x3ff);
       pRawLobe = &((tDlltRtRaw *)RtPktBuf[SnapBuf].Drp.buf)->lobe[lp];

       /* 030114: if Snap Started, store 16 bit word,
	      and inc SnapCntr += 2 */
       if (SnapCntr == 0) {
          pRawLobe->RawFirstL = AdInput.byte.al;
          pRawLobe->RawFirstH = AdInput.byte.ah;
          LastSnapTemp = AdInput.word; /* Initialise LastSnap[] */

       } else { /*  store differential */
#ifdef RW_TEST
           /*AdInput.word = (int)&pRawLobe->RawDiff[SnapCntr-1];*/
           pRawLobe->RawDiff[(2*(SnapCntr-1))  ] = AdInput.byte.al;
           pRawLobe->RawDiff[(2*(SnapCntr-1))+1] = AdInput.byte.ah;
#else     
          LastSnapTemp = LastSnap[lp];
          diff = (AdInput.word - LastSnapTemp);
		  /* Cap the rate of change of information
		    +127 to -128
		   */
#define _MaxPosRawDiff_ 127
#define _MaxNegRawDiff_ -128
          if (diff > _MaxPosRawDiff_) {
		     diff = _MaxPosRawDiff_;
		     LastSnapTemp += _MaxPosRawDiff_;
		  } else if (diff < _MaxNegRawDiff_) {
			 diff = _MaxNegRawDiff_;
			 LastSnapTemp += _MaxNegRawDiff_;
		  } else  {
			 LastSnapTemp = AdInput.word; 
		  }

		  pRawLobe->RawDiff[SnapCntr-1] = (char)diff;
          /* Increment snap pointer, but also cap it
           _DimStart_ handshakes with a new packet
	      */
#endif /* RW_TEST */
      }
      LastSnap[lp] = LastSnapTemp;
   }
   if (++SnapCntr > (_DimNumSamplesPerPkt_+1)) {
       /* actually this is an error condition - but here just in case */
      SnapCntr = _DimNumSamplesPerPkt_;
   }

   /* Detect the maximum of the EMG inputs
	*/ 
   pAd = &ad_result[HW_AD_LB_EMG]; /*- already setup*/
   for (lp=0;lp < NUMBER_LOBES; lp++,pAd++) {
	   AdInput.word = ((*pAd >>_AdcShlAdjust_) & 0x3ff);
       if (AdInput.word > Emg[lp].word) {
          Emg[lp].word = AdInput.word;
	   }
   }
 }
} /*end DimSnapRaw */
 
 /*;*<*>********************************************************
 *  DimRtrvSnapRaw
 *
 *  Provides a copy of the pointer to the latest filled buffer.
 *
 **start*/
 tRtRawPkt *DimRtrvSnapRaw(void)
 {
   register tDlltRtRaw *pRaw;
   /*register tRtRawPkt *pRawPkt;*/
   
   DimRawPkdDataSemaphore = 0;
   /* Do status settings */
   pRaw = ((tDlltRtRaw *)RtPktBuf[SnapBufReady].Drp.buf);
   pRaw->status = (char) bargraph_new.status;
   pRaw->log_att = hw_att_value(0);

   return &RtPktBuf[SnapBufReady].Drp;
   
 } /*end DimRtrvSnapRaw */

 /*;*<*>********************************************************
 * dim_dummy_in
 * this routine is for the PC simulator. It updates the internal
 * data to indicate that new data has been captured.
 *
 **start*/
#ifdef SIMULATING
 void dim_dummy_in(Dimt_data *digp)
 {
 int lp;
 Dimt_lobe_data *ip;
 Dimt_lobe_data *ldp=digp->lobe_data;

    ip = &dimu_input_data[ACTIVE_BANK].lobe_data[0];
    for (lp=0;lp < NUMBER_LOBES; lp++,ip++,ldp++) {
       ip->input  = ldp->input;
       ip->emg    = ldp->emg;
       ip->gain   = ldp->gain;
       ip->status = ldp->status;
    }

 } /*end dim_dummy_in */
#endif

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
   
#ifdef SIMULATING
/*#include "pc_only.h"*/
//}
#endif

