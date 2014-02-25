/*****************************************************************
 * mm_coms.h  - Mind Mirror III communications header file
 *
 *  Copyright Biomonitors Ltd 1992
 *
 * This file contains the structures that are used to
 * access messages 
 *      - received from Biomonitors Biofeedback Equipment
 *      - sent to Biomonitors Biofeedback Equipment
 *
 * The contents of this file are the property of
 *
 *      Biomonitors Ltd
 *
 * Biomonitors Ltd reserves the right to add/change it at
 * any time. However, we will do our best to keep interested
 * parties informed (where they register their interest) and
 * to consult interested parties when additions/changes are
 * needed.
 * 
 * The information provided in this file is used at your own 
 * risk, and it is your responsibility to see that the code
 * that may result works with the required protocols.
 *
 * History
 * Revision
 *  0.4 NJH 21 May-95 Added to DLLE_TYPE
 *  0.3 NJH 15 Jul-92 Updated DLLE_TYPE
 *  0.2 NJH 24 May-92 First compileable
 *  0.1 NJH 29 Jan-92 Orginal Release
 *
 * This module use the OSI concept of layered modules
 *       PHYSICAL LAYER
 *       DATA LINK LAYER
 *
 * The PHYSICAL LAYER guarentess that the data is correct. If it is
 *    not correct then the receiver must throw it away as there is
 *    nothing that can be done with it. The receiver may also indicate
 *    to an operator that some action needs to be taken to correct the 
 *    problem.
 *
 * The DATA LINK LAYER has the flesh-and-bones of the data. The bones are the
 * "data_link_layer_header" that defines the rest of the structure
 * The data link layer frames are
 *    Sychronisation frame   -   dll_sync
 *    Initialisation frame   -   dll_init
 *    Filter information frame - dll_filter
 *    Real-time data frame -     dll_rt
 *    History data frame -       dll_history
 *
 * The data transmitted on the serial link is 8 bits and may be
 * any value. The Octet only has meaning by where it appears in the
 * above frames - ie don't expect ASCII with its control codes.
 *
 * Implementation details 
 *  mm_coms.h/rel 3 20 July 1992
 *   The physical packet structure has been developed and tested
 *     by sending it to a terminal emulation screen translating 
 *     the HEX Octet values to ASCII space padded values.
 *   The Dllt_sync, Dllt_info and Dllt_rt structures have also been
 *     checked.
 *   No code has been developed for receiving the frames in the PC.
 *   At present limited measurements indicate the maximum data rate 
 *     is approxiametly 260 Octets per second. At 9600 baud, with 10
 *     bits per octet (START/STOP) then the theoritical maximum data
 *     rate is 960 Octets per second.
 */

#ifndef mm_coms_h
#define mm_coms_h
/*
 * Prelimanry definitions
 */
#ifndef uchar
#define uchar unsigned char   /* One Octet or 8 bits, sometimes
                               * called a byte
                               */
#endif
#ifndef uint
#define uint unsigned int  /* Two Octets or 16 bits, sometimes
                               * called a word
                               */
#endif

/*****************************************************************
 * Timing Information
 *
 * The expected sequence of DLL frames is
 *    MMIR             PC
 *      --Dllt_sync---->
 *      --Dllt_info---->
 *      --Dllt_filter-->
 *      --Dllt_rt------>
 *      --Dllt_rt------>
 *      --Dllt_rt------>
 *      :: ::::: ::::::
 *
 * If the time between octets in a frame is excessively long then the
 * receiving PC must expect a reset/sync sequence.
 */
#define MM_RESET_TIME 500  /* In mS - ie 0.5 seconds */

/*****************************************************************
 *
 * Physical Link Layer Definitions
 *
 * The Physical Link Layer Frame consists of
 *    physical_link_layer_header
 *    data_link_layer_frame   (dll)
 *    physical_link_layer_tail (Not used)
 *
 */

  typedef struct {
     /* First byte transmitted on link */
     uchar first_byte; /* use PLL_SYNC_MASK to access this byte.
                        * PLL_SYNC_MASK bits alternate between 
                        * PLL_SYNC_1010 & PLL_SYNC_0101
                        */
     uchar size;       /* size of incoming physical link layer frame
                        * including pll_header,dll_frame ,pll_tail
                        * Range: 5     for no payload data
                        *       ->255 for 250 octets of payload data
                        */
     uchar checksum;   /* Adjusted so that the sum of all the octets
                        * in the pll frame is '0'
                        */
     uchar dll_frame[1];/* Start of dll_frame
                        */
  } Pllt_header;

/* Defines for physical_link_layer_frame.first_byte */
#define PLL_1ST_OCTET_RESERVED_MASK  0xf0 /* 0 at moment  */
#define PLL_SYNC_MASK 0x0f /*0,1,2,3,0,... */
#define PLL_SYNC_1010 0x0A
#define PLL_SYNC_0101 0x05

/* Null - commented out
  typedef struct {
  } Pllt_tail;
*/
#define PLL_ADD_SIZE (sizeof(Pllt_header)-1)
#define PLL_MAX_SIZE 256 /* Max size of pll header + payload */
#define PLL_SIZE_POS 2   /* Position of .size in pll_header  */

/*****************************************************************
 *
 * Data Link Layer Definitions
 * (This is more like an Application level entity - but is left like
 *  this for backwards level compatibility )
 *
 * The data link layer frame consists of
 *  dll_header
 *  dll_xxxxxx
 *
 */

#define edlle_type enum dlle_type
  edlle_type {
 /* Definitions for all the different types of DLL programs
  * Should align with mm_file:[enum mmfe_rec_type]
  */
                 DLLE_UNINIT=0, /* Unitialised area should not occur  */
                 DLLE_SYNC=1,   /* Syncronisation frame               */
                 DLLE_INFO=2,   /* Information frame                  */
                 DLLE_FILTER=3, /* Filter definition frame            */
                 DLLE_RT=4,     /* Real Time Filter information frame */
                 DLLE_HISTORY=5,/* History information frame          */
                 DLLE_TEST=6,   /* Test - after setting test option   */
		 DLLE_RT_RAW=7, /* RealTime Raw Packed Data           */
		 DLLE_RT_RRESET=8,/* RealTime Raw Packed Reset        */
		 DLLE_RT2=9,	/* Real Time Filter information frame */
						/* 8 times a second */
		 DLLE_TYPE_INVALID}; /* This and above should not occur */
               

 typedef struct {
 /* This is the header struture and always follows the Pllt_header    */
    /* First dll byte transmitted                                     */
    uchar dll_type;  /* expect one of enum dlle_type                  */
    uchar frame_num; /* rolling frame number 1,2..255,0,1..255,0,1    */
                     /* Incr every time an attempt is made to send    */
                     /*  a frame. If a number is out of sequence then */
                     /*  a data frame has been lost.                  */
    uchar body[1];   /* start of main body  or payload portion        */
 } Dllt_header;
#define DLL_ADD_SIZE (sizeof(Dllt_header)-1)

  /*****************************************************
   * Synchronisation frame
   *      Only sent on power up or MMIR III reset
   */
#define DLLC_SYNC 8
  typedef struct {
     /* This is used to indicate a reset has happened and the 
      * PC must synchronise to the new data 
      */
    /*const not for c++ */uchar sync[DLLC_SYNC]; /*="\r\nSync\r\n" */
  } Dllt_sync;

  /*****************************************************
   * Information frame
   *     Initialising receivers constants. Sent on
   *        - power up
   *        - request from keypad/user
   */
#define DLLC_TITLE_LEN 16
#define DLLC_VER_LEN    5
#define DLLC_TAIL_LEN   2
#define DLLC_INFO_LOG 16
#define DLLC_INFO_COPYRIGHT 34
#define DLLC_INFO_SN 10
  typedef struct {
     /*
      * This will contain
      *    logical to physical settings on Mind Mirror
      *    output of filter when it has reached calibrated setting
      *    version of Mind Mirror software
      *
      */
      uchar title[DLLC_TITLE_LEN]; /*="Mind Mirror III "        */
      uchar ver[DLLC_VER_LEN];     /*="xx.yy" Version number inserted by s/w */
      uchar cpyrght[DLLC_INFO_COPYRIGHT];
      uchar id[DLLC_INFO_SN];  /* serial number of hardware */
      uchar tail[DLLC_TAIL_LEN];  /*="\r\n" End of message, prints nicely   */
      /* First byte transmitted is att_log_phy[0] */
      uint att_log_phy[DLLC_INFO_LOG];
                             /* Logical to physical att settings 
                              * for dll_rt.att 
                              */
      uchar clbr_setting;      /* Output of a filter that corresponds
                              * to the value set in
                              * dll_init.att_log_phy[dll_rt.log.att]
                              */
  } Dllt_info;
/* The following special values apply to Dllt_info.log_to_phy */
#define HW_CONTACT_TEST 0xfffe /* Contact test is active */
#define HW_SELF_TEST   0xfffd  /* Self test is active    */

  /*****************************************************
   * Describes all filters in the Mind Mirror III
   */
#define DLL_NUMBER_FILTERS 14
#define FILTER_TYPE_LEN 14 /* Keep Even */
#define RIPPLE_LEN 8
#define FREQ_LEN 6
  typedef struct {
     /* First byte tx is */
     uchar filter_type[FILTER_TYPE_LEN];
     uchar passband_ripple[RIPPLE_LEN];/* ripple in the passband of the filter        */
     uchar stopband_att[RIPPLE_LEN];   /*attenuation(db) in the stopband of the filter*/
     uchar passband1_cutoff_frequency[FREQ_LEN]; /* -3db point of edge passband     */
     uchar passband2_cutoff_frequency[FREQ_LEN]; /* -3db point of edge passband     */
     uchar stopband1_cutoff_frequency[FREQ_LEN]; /* freq stopband att. attained     */
     uchar stopband2_cutoff_frequency[FREQ_LEN]; /* freq stopband att. attained     */
  } Filter_descr;

  typedef struct {
     /* This contains
      * Mind Mirror III Filter characteristics
      *   as specified in literature
      *   This is for verification of filters implemented.
      *
      */
    /* First byte tx is LSB, then MSB */
    int type; /* Type of this filter = 1 */
    Filter_descr filter[DLL_NUMBER_FILTERS];
  } Dllt_filter;

  /*****************************************************
   * Filter Data output
   */
  typedef struct {
     /* This contains a set of data to construct a
      * spectograph.
      */
     uchar status;       /* Status of the MMIR III - reserved        */
     uchar log_att;      /* logical attenuation settings - see below */
     uchar time_stamp_lower;/* Time stamp value                      */
     uchar time_stamp_upper;/* Time stamp value                      */
     /* The following are filter output values with a
      *     RANGE: 0->0xff
      *            0 represents no output of filter
      *            dll_init.clbr_setting represents the calibrated
      *                 setting of the output
      *            0xff maximum output of filter
      */
     /* First byte tx is left_emg followed by left_filters[0] */
     uchar left_emg;
     uchar left_filters[14]; 
     uchar right_emg;
     uchar right_filters[14];
  } Dllt_rt;
#define DLL_RT_SIZE (sizeof(Dllt_rt))

/* Defines for dll_rt.status */
    /* Undefined at present  */
    /* Will include something like
     *     - Left Hemisphere invalid (EMG high)
     *     - Right Hemisphere innvalid (EMG high)
     */
/* Defines for dll_rt.att */
#define left_att_mask 0xf0
#define right_att_mask 0x0f
#define get_left_log_att(value) ((value >> 4) & 0xf)
#define get_right_log_att(value) (value & 0x0f)

#define DLL_PAYLOAD_OFFSET (DLL_ADD_SIZE + PLL_ADD_SIZE)

  /*****************************************************
   * Raw Data output - DLLE_RT_RAW
   */
/*#define DLL_RAW_SAMPLES_PER_PKT 64*/
#define _DimNumSnapBufs_ 2 

#define _DimNumSamplesPerPkt_ 0x1f
     /* this is the Number of samples per packet.
        It is a differential reading - the first sample is stored
        as a full integer and then differential after that  
        The sample rate is 8 ms or 128 samples per second
        There are 8 packets per second - so 32 samples less first sample
     */
     /* Njh The above is defined by packet rate and is extremely sensitive */
/* For debugging - just use one lobe */
/*#ifdef RW_TEST
#define RW_NUMBER_LOBES 1
#else
#define RW_NUMBER_LOBES 2
#endif*/
typedef struct {
/*     char Status; */
 /*    unsigned int Init : 1; /*Set to '1' when initialising packing*/
 /*    unsigned int Status : 7; /* */
/*#ifdef RW_TEST*/
  char EmgL; /*maximum emg recorded Low Byte */
  char EmgH; /*maximum emg recorded High Byte*/
     /* The following are raw data values from the A/D
      *     RANGE: 0->0xff
      *            0 represents no difference
      *            dll_init.clbr_setting represents the calibrated
      *                 setting of the output
      *            0xff represents maximum difference 
      */
  char RawFirstL; /* First sample Low Byte*/
  char RawFirstH; /* First sample High Byte*/
  char RawDiff[_DimNumSamplesPerPkt_]; /* Follows same format as RawFirstL/H */
/*#else
     char Emg; /*maximum emg recorded * /
     int RawFirst; /* First sample* /
     char RawDiff[_DimNumSamplesPerPkt_]; 
#endif*/
    } tDlltRtRawLobe; /* raw data for single lobe*/

  typedef struct {
     uchar  status;       /* Status of the MMIR III - reserved        */
     uchar log_att;      /* logical attenuation settings - see mmstatus.h*/
     /*uchar time_stamp_lower;/* Time stamp value                      */
     /*uchar time_stamp_upper;/* Time stamp value                      */
     /* First lobe is the left hand one */
     tDlltRtRawLobe lobe[NUMBER_LOBES];

  } tDlltRtRaw;

  typedef struct {
    uchar pllu_header[sizeof(Pllt_header)-1];/*Reserved for header */
    uchar dllu_header[sizeof(Dllt_header)-1];/*Reserved for header */
/*    char nuller; /*even everything up*/
/*    tDlltRtRaw buf; /* payload */
    uchar buf[sizeof(tDlltRtRaw)]; /* payload */

    } tRtRawPkt;

#define DLL_RT_RAW_SIZE sizeof(tDlltRtRaw)
#define DLL_RT_RRESET_SIZE 0

  typedef struct {
      tRtRawPkt Drp;/* was DimRawPkd */
      Dllt_rt   Frt;
      } tRtPktBuf;
  /*****************************************************
   * Raw Data output - DLLE_RT_RRESET
   * There is no data to this packet
   */
#endif /* mm_coms_h */
/* end mm_coms.h */



