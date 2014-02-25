/* ssp.c
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
  This module is responsible for managing all data
  out of the serial port.

  It accepts indications from the
       initialisation module
       managed filter data module
       keypad manager
  of when to send data

Algorithms
  ssp_initialisation()  - initially
  ssp_action()          - any time 
  ssp_header_tx(void)

Internal Data
*/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
/*#include <stdio.h>
#include <math.h>
#include <stdlib.h>
*/
#include <string.h>

#include "general.h"
#include "iir.h"
#include "mm_coms.h"
#include "dim.h"
#include "proto.h"
#include "ssp.h"
#include "mfd.h"
#include "dbg.h"
#include "hw.h"

/**************************************************************
 * Constants 
 */
const Word log_to_phy[DLLC_INFO_LOG] = {1,3,5,10,30,50,100,300,500,
                                       HW_CONTACT_TEST,HW_SELF_TEST,
                                       0,0,0,0,0};

const uchar mm_ver[] = " 3.a  ";

/**************************************************************
 * Externally defined 
 */
extern char SchTxData;/* Data type on serial output */
extern Semaphore SchCnt0_5sema;

/**************************************************************
 * Internal storage to this module 
 *
 * tx_buff - the pll is assembled in this buffer. Only one exists
 *           as messages can't be queued.
 */
#define PLL_MAX_BUFFER 100  /* Guess */
 unsigned char tx_buff[PLL_MAX_BUFFER];


/* The rolling frame counts for the dll and pll layers */
 int dll_frame_num, pll_frame_num;

 Sspt_manage manage;

/* ssp permanent debug information */
 enum sspe_debug_control ssp_debug_control;
 static unsigned char debug_buf[(3*DBGC_BUFFER_SIZE)+3];


 /**************************************************************
 * Internal prototypes 
 */
 void tx_rt_data(void);
 void tx_filter_data(Dplt_data *dplp, uchar *tx_ptr);
 void TxRawData(tRtRawPkt *pRtRaw);
 int  CalcChecksum(uchar *p, int size);
/* void TxRawData(Dllt_rt_raw *pRtRaw,uchar *pTxBuf); */
 unsigned char *ssp_req_dll_buf(void);
 void send_dll(unsigned char *tx_ptr,int size,int payload_checksum,enum dlle_type type);
 void tx_history(void);
 void tx_filters(void);
 unsigned char ntoa(unsigned int nibble);
 int sum(unsigned char *str, int num);
 void filter_cpy(uchar *destp,Iir_filter_descriptor *filterp);

/*;*<*>********************************************************
 * ssp_initialisation
 *
 * This module initialise this subsystem.
 *
 * Note: During initialisation the ssp module does not own the UART.
 **start*/
 enum inite_return ssp_initialisation(Initt_system_state *ssp)
 {
  dll_frame_num = pll_frame_num = 0;
  ssp_action(SSP_TX_RT_EN);
  manage.tx_buffer_available = TX_BUF_AVAILABLE;
  ssp_debug_control = SSP_DEBUG_DISABLED;/* DISABLED/ENABLED */

  return PASS;
 } /*end ssp_initialisation*/

/*;*<*>********************************************************
 * ssp_header_tx
 *
 * This function transmits all the header information
 * that is needed to indicate the MMIR has started up.
 *
 * Note: It can only be invoked after initialisation subsection
 * has finished and releases the UART
 *
 **start*/
 void ssp_header_tx(void)
 {

   /* Send the information frame */
   ssp_info_tx();
   delay_ms(100); /* Allow time to send */

   /* Send the sync/mmir title frame */
   ssp_sync_tx();
   delay_ms(100); /* Allow time to send */

   /* Send the information frame */
   ssp_info_tx();
   delay_ms(100); /* Allow time to send */

   /* Send the filter description frames */
   ssp_filter_tx();
   delay_ms(100); /* Allow time to send */

 } /*end ssp_header_tx*/

/*;*<*>********************************************************
 * ssp_action
 *
 * This function is called when the following needs to be performed:-
 *     SSP_TX_HISTORY - transmit the history buffer
 *     SSP_FILTER     - transmit the filter characteristics
 *     SSP_TX_RT_EN   - enable transmitting of real time data
 *     SSP_TX_RT_DIS  - disable transmitting of real time data
 *     SSP_TX_RT      - real time data is available for transmission
 *
 *
 **start*/
 void ssp_action(enum sspe_action act)
 {

   switch (act) {
   case SSP_TX_RT:
     if (manage.transmit_rt_en == SSP_TX_RT_EN) {
        tx_rt_data();
     }
     break;

   case SSP_TX_RT_EN:
     manage.transmit_rt_en = SSP_TX_RT_EN;
     break;

   case SSP_TX_RT_DIS:
     manage.transmit_rt_en = SSP_TX_RT_DIS;
     break;

   case SSP_TX_HISTORY:
     tx_history();
     break;

   case SSP_FILTER:
     tx_filters();
     break;
   }

 } /*end ssp_action*/

/*;*<*>********************************************************
 * tx_rt_data
 *
 * This function acquires the latest set of realtime data
 * and a free transmit buffer. 
 *
 * This function is only able to perform its task if it is given
 * enough time between calls for the transmit buffer to empty itself,
 * otherwise it returns without transmitting information
 *
 **start*/
 void tx_rt_data(void)
 {
   void *pData;
   unsigned char  *tx_ptr;  /* Pointer to the tx buffer */

   /*
    * Request a buffer
    */
   if ( (tx_ptr = ssp_req_dll_buf()) == NULL) {
      return; /* No buffers avaliable */
   }

   /*
    * Get the information to be sent
    */
   switch (mfd_supply_data(_ModuleSsp_,&pData)) {
	   case DLLE_RT:
		   tx_filter_data(pData, tx_ptr);
		   break;
	   case DLLE_RT_RAW:
		   /*TxRawData */
           TxRawData(pData);/* ??*/
		   break;
	   case DLLE_RT_RRESET:
		   send_dll(tx_ptr,(int) DLL_RT_RRESET_SIZE,
			    0/*checksum*/,DLLE_RT_RRESET);
		   break;
	   default: /* This could include DLLE_TYPE_INVALID */
		   /* No Action is taken as it may be valid not to send data*/
		   break;
   }
 } /*end tx_rt_data*/

/*;*<*>********************************************************
 * ssp_dl_req
 *
 * This function acquire a free transmit buffer and then transmits
 * the supplied data with it.
 *
 * This function is only able to perform its task if it is given
 * enough time between calls for the transmit buffer to empty itself,
 * otherwise it returns without transmitting information
 *
 **start*/
 void ssp_dl_req(enum SSPE_DL_REQ dl_req,
		 char *tx_buf)
 {
   unsigned char  *tx_ptr;  /* Pointer to the tx buffer */

   /*
    * Request a buffer
    */
   if ( (tx_ptr = ssp_req_dll_buf()) == NULL) {
      return; /* No buffers avaliable */
   }

   tx_filter_data((Dplt_data *) tx_buf, tx_ptr);

 } /*end ssp_dl_req*/

/*;*<*>********************************************************
 * tx_filter_data
 *
 * This function accepts Dplt_data buffer and an empty buffer and
 * processes it to be sent on the serial port.
 *
 **start*/
 void tx_filter_data(Dplt_data *dplp,/*Data to be transmitted*/
                     uchar *tx_ptr /* Empty transmit buffer*/) {
   register Dllt_rt *payldp;/* Pointer to the payload area of the tx buffer */
   register Dplt_lobe_data *lobep; /* Pointer to */
   register char *elemp, *barp;
   register int checksum = 0,bar_loop;
   int lobe_lp;
   register Wordbyte temp_char;
   enum dlle_type type;
   
   /*
    * Set up payload pointer into tx_ptr buffer
    */
   payldp = (Dllt_rt *)(tx_ptr + DLL_PAYLOAD_OFFSET);
   /*
    * Copy the information into the payload section
    */
   lobep=&dplp->lobe_data[LF_LOBE];
   /* Use left lobe values to set up 'status' & 'gain' values
    * for time being
    */
   temp_char.byte.al = dplp->status;
   payldp->status = temp_char.byte.al;
   checksum += temp_char.byte.al;

   /* Add gain to outgoing packet - needs updating to be a logical value */
   temp_char.byte.al = lobep->gain;
   payldp->log_att = temp_char.byte.al;
   checksum += temp_char.byte.al;

   /* Add the time_stamp  - its 16 bits */
   temp_char.word = dplp->time_stamp;
   payldp->time_stamp_lower = temp_char.byte.al;
   payldp->time_stamp_upper = temp_char.byte.ah;
   checksum += (temp_char.byte.al + temp_char.byte.ah);

   /* Left lobe */
   for (bar_loop = 0,
          elemp = (char *)&payldp->left_emg,
          barp  = (char *)&lobep->bar[0];
        bar_loop < NUMBER_BARGRAPHS;
        bar_loop++,elemp++,barp++) {
      *elemp = *barp;
      checksum += *barp;
   }

   /* Right lobe */
   lobep=&dplp->lobe_data[RF_LOBE];
   for (bar_loop = 0,
          elemp = (char *)&payldp->right_emg,
          barp  = (char *)&lobep->bar[0];
        bar_loop < NUMBER_BARGRAPHS;
        bar_loop++,elemp++,barp++) {
      *elemp = *barp;
      checksum += *barp;
   }

   /* Set the DLLE_TYPE */
   if (SchTxData) {
	   type = DLLE_RT2;
   } else {
	   type = DLLE_RT;
   }
   send_dll(tx_ptr,(int) DLL_RT_SIZE, checksum, type);

 } /*end tx_filter_data*/
/*;*<*>********************************************************
 * TxRawData
 *
 * This function accepts "tRtRawPkt" buffer and 
 * processes it to be sent on the serial port.
 * The processing is only on the payload/buf sections
 *
 **start*/
void TxRawData(tRtRawPkt *pRtRaw) /*In: Ptr to data in */
{
    register int checksum;
    register unsigned char DllType;

    /* This is a kludege - as populating DIM RtRawPkt from the beginning.
       It would be nice to have all subsystems have a packet big enough to
       populate to begin with.
     */
    manage.tx_buffer_available = TX_BUF_AVAILABLE;

   /* Calculate Checksum */
    checksum = CalcChecksum(pRtRaw->buf,DLL_RT_RAW_SIZE);
/*    p = pRtRaw->buf;
    for (loop = 0; loop < DLL_RT_RAW_SIZE; loop++) {
       checksum += *p++;
       }*/
    DllType = (unsigned char) DLLE_RT_RAW;
    if (SchCnt0_5sema) {
       DllType |= 0x80; /* Indicate extended DLLE_RT_RAW */
    }
    
    send_dll((unsigned char *)pRtRaw,(int) DLL_RT_RAW_SIZE, checksum, DllType);

} /*end TxRawData */


int CalcChecksum(uchar *p, int size) /*In: to data in */
{
  register int checksum = 0;
  register int loop;

  for (loop = 0; loop < size; loop++) {
     checksum += *p++;
  }
  return checksum;
} /*end CalcChecksum */

#ifdef OldTxRawData
 void TxRawData(tDllt_rt_raw *pRtRaw, /*In: Ptr to data in */
		uchar *pTxBuf)   /*Out: Ptr to where to put data*/
		 {
   register int checksum = 0;
   register char *p;
   tDllt_rt_raw *pPayLd;
   int loop;

   /* Copy All Raw Data To Output */
   pPayLd =(tDllt_rt_raw *) (pTxBuf + DLL_PAYLOAD_OFFSET);
   *pPayLd = *pRtRaw;

   /* Calculate Checksum */
   p = (char *) pRtRaw;
   for (loop = 0; loop < DLL_RT_RAW_SIZE; loop++) {
       checksum += *p++;
       }
   send_dll(pTxBuf,(int) DLL_RT_SIZE, checksum, DLLE_RT_RAW);
	 
 } /*end TxRawData */
#endif
/*;*<*>********************************************************
 * ssp_req_dll_buf
 *
 * This function requests a transmit buffer (only one)
 * and returns a pointer pointing at the start of the
 * dll payload area.
 *
 * The dll_buffers frame indicator is incremented every time
 *   an attempt is made to obtain a tx buffer - irrespective if it
 *   succeeds or fails - ie the assumption is made that if a request to
 *   send data is made then it will be thrown away if unsuccesfull
 * RETURNS:
 *          NULL if buffer not available
 *          ptr to payload area if available
 *
 **start*/
 unsigned char *ssp_req_dll_buf(void)
 {
 /* 
  * Check availability of buffer and leave a record of having tried
  * to get it.
  */
 if (++dll_frame_num > 7)
    dll_frame_num = 0;

 if (manage.tx_buffer_available == TX_BUF_NOT_AVAILABLE){
    /* NOT AVAILABLE so abort */
    return NULL;
 }

 /*
  * Hold tx_buffer
  */
 manage.tx_buffer_available = TX_BUF_NOT_AVAILABLE;
 return &tx_buff[0];

 } /*end ssp_req_dll_buf*/

/*;*<*>********************************************************
 * send_dll
 *
 * This function is responsibile for adding dll and pll information
 * to a packet and sending it to the remote computer. It is assumed
 * that the payload is in the correct area of the buffer.
 *
 * The pll_buffers frame indicator is incremented every time
 *   a frame is actually sent.
 *
 * Inputs
 *     tx_ptr - pointer to buffer to be sent
 *     size - size of payload in buffer 0 to ... bytes
 *     payload_checksum - additive value of all octets being
 *            sent in the buffer.
 *     type - one of enum ..dlle_type
 * Algorithm
 *    Add dll information
 *    Add pll information
 *    send buffer
 *  
 **start*/
 void send_dll(unsigned char *tx_ptr, /* Begining of the buffer */
               int payload_size, 
               int payload_checksum,
               enum dlle_type type) /* Msg type*/
 {
 register Pllt_header *pll = (Pllt_header *) tx_ptr;
 register Dllt_header *dll = (Dllt_header *) &pll->dll_frame[0];
 register int checksum=payload_checksum;

 /*
  * perform data link layer functions
  */
 dll->dll_type = (unsigned char) type;
 checksum += (unsigned int) type;

 dll->frame_num = (unsigned char) dll_frame_num;
 checksum += (unsigned int) dll_frame_num;

 /*
  * perform physical link layer functions
  * Note: PLL_1ST_OCTET_RESERVED_MASK section is set to 0 
  */
#define PLL_CHECK_NUM 0x1
 if (PLL_CHECK_NUM & pll_frame_num++) {
    pll->first_byte = PLL_SYNC_1010;
 } else {
    pll->first_byte = PLL_SYNC_0101;
 }
 checksum += (unsigned int) pll->first_byte;

 pll->size =  (PLL_ADD_SIZE + DLL_ADD_SIZE + payload_size);
 checksum += (unsigned int) pll->size;

 pll->checksum = (char) (0 - checksum) ;
 if (ssp_debug_control != SSP_DEBUG_DISABLED) {
    /* The following is a debug routine 
     * It translates the outgoing message into ASCII characters
     */
    tx_ptr = ssp_translate(tx_ptr,&(pll->size));
 }

 ssp_send_buf(tx_ptr,(unsigned int)pll->size);

 } /*end send_dll*/

/*;*<*>********************************************************
 * ssp_control
 *
 *  Returns management information on the SSP interface.
 *
 **start*/
 void ssp_control(Sspt_manage *mngp)
 {
    mngp->tx_buffer_available = manage.tx_buffer_available;
    mngp->transmit_rt_en      = manage.transmit_rt_en;
    /*memcpy((char *)mngp,(const char *) &manage, sizeof(Sspt_manage));*/
 } /*end ssp_control*/

/*;*<*>********************************************************
 * tx_history
 *
 * This function transmits the history buffer.
 *
 * It returns when the buffer has been sent.
 *
 **start*/
 void tx_history(void)
 {

 } /*end tx_history*/

/*;*<*>********************************************************
 * tx_filters
 *
 * This function transmits the filter descriptions.
 *
 **start*/
 void tx_filters(void)
 {

 } /*end tx_filters*/

/*;*<*>********************************************************
 * ssp_translate
 *
 * This function takes the supplied string and translates it
 * into printable ASCII characters.
 *
 * Hence   0x23 becomes 0x22 0x23 0x20
 *                        2   3    _ (space)
 *
 * NOT REENTRANT.
 *
 **start*/
 unsigned char *ssp_translate(unsigned char *buf,unsigned char *size)
 {
 register unsigned int temp;
 register int loop;
 register unsigned int number = (unsigned int) *size;
 register unsigned char *p=debug_buf;
 
   for (loop=0; loop < number; loop++) {
      temp = (unsigned int) *buf++;
      *p++ = ntoa((temp&0xf0) >> 4);
      *p++ = ntoa(temp&0x0f);
      *p++ = ' ';
   }
   *p++ = '\n';
   *p++ = '\r';
   *size = ((number * 3)+2);

   return &debug_buf[0];

 } /*end ssp_translate*/

/*;*<*>********************************************************
 * ntoa - nibble to ASCII
 * Translates a character from 4 bit binary to hexadecimal
 *
 **start*/
 unsigned char ntoa(unsigned int nibble)
 {
   if (nibble > 9) {
      return (('A'-0x0a)+nibble);
   } else {
      return ('0'+nibble);
   }
 } /*end ntoa*/

/*;*<*>********************************************************
 * sum
 * 
 * Returns the additive value of the specified characters 
 * in the string
 **start*/
 int sum(unsigned char *str, int num)
 {
 register unsigned int sum=0;
 register          int loop;
 register unsigned char *p = str;

   for (loop=0; loop < num; loop++) {
      sum = (unsigned int) *p++;
   }
   return sum;
 } /*end sum*/

/*;*<*>********************************************************
 * ssp_sync_tx
 *
 * Sets up the title buffer and transmits it.
 *
 **start*/
 void ssp_sync_tx(void)
 {
 /*
  * MM title buffer
  */
 Dllt_sync sync_buf = {"\r\nSync\r\n"};
 register Dllt_sync *sync_p = &sync_buf;

   /*
    * Request a buffer to increment dl buffer numbers
    */
   ssp_req_dll_buf();
   /*
    * Set up the title_buf
    */

   /*
    * Send Initialisation frame
    */
   send_dll((unsigned char *) sync_p,
                 sizeof(Dllt_sync),
       sum((unsigned char *)sync_p,sizeof(Dllt_sync)),
                         DLLE_SYNC);

 } /*end ssp_sync_tx*/

/*;*<*>********************************************************
 * ssp_info_tx
 *
 * Transmits the information frame
 **start*/
 void ssp_info_tx(void)
 {
 Byte buf[DBGC_BUFFER_SIZE], sn_buf[DLLC_INFO_SN];
 register Dllt_info  *info_p;

   /*
    * Request a buffer to increment dl buffer numbers
    */
   ssp_req_dll_buf();
   /*
    * Setup the information frame
    */
   info_p = (Dllt_info *)(&buf[0] + DLL_PAYLOAD_OFFSET);

   strncpy((char *)info_p->title,(cchar *)"Mind Mirror III ",DLLC_TITLE_LEN);
   strncpy((char *)info_p->ver, (cchar *)mm_ver,DLLC_VER_LEN);
   strncpy((char *)info_p->cpyrght,
          (cchar *)"\n\rCOPYRIGHT BIOMONITORS LTD 1995-2003\n\r",
          (size_t)DLLC_INFO_COPYRIGHT);

   strncpy((char *)info_p->id,
          (cchar *)hw_sn(sn_buf),
         (size_t)DLLC_INFO_SN);

   strncpy((char *)info_p->tail,(cchar *)"\r\n",DLLC_TAIL_LEN);

   strncpy((char *)info_p->att_log_phy,
           (cchar *)log_to_phy,
     (size_t)(2*DLLC_INFO_LOG));

   info_p->clbr_setting = HW_CALIBRATION_SETTING;

   /*
    * Send the information frame
    */
   send_dll(                   buf,
                 sizeof(Dllt_info),
   sum((unsigned char *)info_p,sizeof(Dllt_info)),
                        DLLE_INFO);


 } /*end ssp_info_tx*/

/*;*<*>********************************************************
 * ssp_filter_tx
 *
 * Transmits the definitions of the filters on the serial link
 **start*/
 void ssp_filter_tx(void)
 {
 Iir_filter_descriptor *iirp;
 uchar buf[sizeof(Filter_descr)+DLL_PAYLOAD_OFFSET+2];
 register uchar *destp, *payldp;
 int loop;

   return; /* See comment in filter_cpy() */

   /*
    * Request a buffer to increment dl buffer numbers
    */
   ssp_req_dll_buf();

   payldp = &buf[0] + DLL_PAYLOAD_OFFSET;
   for (loop = 0; loop < DLL_NUMBER_FILTERS; loop++) {

      /* 
       * Transfer filter description into buf[] 
       *!/
      filter_cpy(payldp,              /* point to buffer to copy it into *!/
                 dfm_filter_description(loop)); /* Get filter description*/

      /* Make it screen readable */
      strncpy((char *)(payldp+sizeof(Filter_descr)),(cchar *)"\r\n",2);

      /*
       * Send the filter frame
       */
      send_dll(                      buf,
                    sizeof(Filter_descr),
        sum(payldp,sizeof(Filter_descr)),
                            DLLE_FILTER);

      delay_ms(100); /* Allow time for buffer to be sent */

   }
 } /*end ssp_filter_tx*/

/*;*<*>********************************************************
 * filter_cpy
 * 
 * Populates *destp from the *filterp doing all the neccessary
 * translation in the process.
 **start*/
 void filter_cpy(uchar *destp,Iir_filter_descriptor *filterp)
 {

   /* float to ASCII conversion represents a problem at present */

 } /*end filter_cpy*/

/*;*<*>********************************************************
 * 
 **start*/
/* ()
 {

 } /*end*/

