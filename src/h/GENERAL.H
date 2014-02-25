/************************************************
 * General.h
 *
 * General includes
 *
 * Compiler information must be defined on the compiler com line
 * #define TC turbo C
 * #define IC96 for intel 96 compiler
 */
#include "mmir_cfg.h" /* Controls the way the image builds */
/******
   debug
* /
!#ifndef DISABLE_ENABLE
!#define INTERRUPT_FLAG_MASK 0x40
!#define disable() ioport1 |= INTERRUPT_FLAG_MASK
!#define enable() ioport1 &= ~INTERRUPT_FLAG_MASK
!#endif
*/
/* If TEST_FUNCTIONS_ENABLED is defined then self test results are displayed
 * on the bottom line of the screen
 */
/*#define TEST_FUNCTIONS_ENABLED*/
/************************************************
 * General Definitions
 */
#ifndef FALSE
#define FALSE 0
#endif /* FALSE */
#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

/************************************************
 *
 * Target definitions
 */
 enum target_type {EPROM_TARGET,EVAL_TARGET,PC_SIMULATOR_TARGET};

/************************************************
 * Useful globally definitions
 */

#define INT16 unsigned      /* 16 bit unsigned int    */
 typedef int Frac;           /* 16 bit signed fraction */
#define INT32 unsigned long int
#define MATH long double
#define FRAC16 int
#define COEFF_TYPE_F  FRAC16
#define SAMPLE_TYPE_F FRAC16

#define SIZE_DOUBLE_VAR 8
#define BYTE_MASK 0xff

#ifndef NULL
#define NULL 0L
#endif /* NULL */

#define low_byte(data)  ((char) data)
#define high_byte(data) ((char)(data >> 8))

#ifdef IC96
#define max(a,b) (a > b) ? a : b
#endif /* IC96 */

 typedef struct {unsigned char al, ah;} Bytereg;
 typedef union {Frac word; Bytereg byte;} Wordbyte;

 typedef struct {int low, high;} Wordreg;
 typedef union {long dword; Wordreg word;} Longword;

 typedef unsigned char Byte; /*  8 bit unsigned byte   */
#define uchar unsigned char 
#define cchar const char
 typedef unsigned      Word; /* 16 bit unsigned int    */
#define uint unsigned int
 typedef unsigned      Semaphore;
#define SET_SEMAPHORE 1
#define CLR_SEMAPHORE 0

/************************************************
 * Clock
 *
 * The system CLOCK may be one of the following. IF a module is dependent
 * on the system clock then it must do its own checks for one of the following
 * ONLY ONE of the following must be define
 */
#define CLOCKC20 1
/*#define CLOCKC16  1 - not operational on real target 27-dec-91 */
/*#define CLOCKC12  1 */
/*#define CLOCKC6_4 1 */

/************************************************
 * Version Information
 */
#define VERSION_SIZE 9
#define VERSION_DATE_SIZE 12
#define VERSION_TIME_SIZE 9


/************************************************
 * System section
 */
#define NUMBER_FILTERS 14
#define NUMBER_LP_FILTERS 2
#define TOTAL_NUMBER_FILTERS (NUMBER_FILTERS+NUMBER_LP_FILTERS)
#define NUMBER_BARGRAPHS (NUMBER_FILTERS+1)

/* 
 * defines for NUMBER_LOBES 
 */
#if defined(LOG_SCRN_15BY1)
#define NUMBER_LOBES     1      /* Test         */
#else   /* assume defined(LOG_SCRN_15BY2) */
#define NUMBER_LOBES     2      /* For small LCD */
#endif /* LOG_SCRN_15BY1 */

#define MAX_NUMBER_LOBES 4 /* irrespective of which MMIR version */
#define LF_LOBE 0 /* Left Front Lobe identifier  */
#define RF_LOBE 1 /* Left Back Lobe identifier   */
#define LB_LOBE 2 /* Right Front Lobe identifier */
#define RB_LOBE 3 /* Right Back Lobe identifier  */
/* 
 * Defines for frequencies
 */
#define BG_EMG  0
#define BG_0_75 1
#define BG_1_5  2
#define BG_3    3
#define BG_4_5  4
#define BG_6    5
#define BG_7_5  6
#define BG_9    7
#define BG_10_5 8
#define BG_12_5 9 
#define BG_15   10
#define BG_19   11
#define BG_24   12
#define BG_30   13
#define BG_38   14

/************************************************
 * Defines for time_table[] 
 */
#define TABLE_128MS 0
#define TABLE_64MS 1
#define TABLE_32MS 2

/************************************************
 * Master sampling frequency set here
 *
 * The values specified in filter specifications
 * are not used.
 */
#define SAMPLING_FREQUENCY 256


/*****************************************************
 * General enums
 */
 enum sys_return {SYS_OK,SYS_FAIL,SYS_NOT_AVAILABLE};
 enum inite_type {COLD,WARM,REPORT};
 enum sche_entry {SCHE_FAST,SCHE_BACKGND};
/* enum inite_virtual_screen_type {INITE_TYPE_15BY1,INITE_TYPE_15BY2,
       INITE_TYPE_30BY2,
       INITE_TYPE_HISTORY,INITE_TYPE_ABS_AMPLITUDE};
*/
 enum inite_return {PASS,FAIL};
 enum inite_console_type {INIT_CONSOLE_TYPE,SERIAL_CONSOLE=1,LCD_CONSOLE=2};
#define ASCII_OFFSET 0x20
#define ASCII_NUM_OFFSET 0x30
/*
 * Names of lines on TLX display
 */
#define TXT_LN_FIRST_FILTER 0
#define TXT_LN_LAST_FILTER 13
#define TXT_LN_EMG    14
#define TXT_LN_STATUS 15

/************************************************
 * General typedefs
 */
 typedef struct {
 int x;
 int y;
 } Pixels;

 typedef struct {
 enum inite_type init_type;
/* enum inite_virtual_screen_type virtual_screen_type; */
 enum inite_console_type console_type; /*Types of consoles supported */
 } Initt_system_state;
#define _eModuleId_ enum _ModuleId_
 _eModuleId_ {
    /* defines IDs of system modules */
    _ModuleSsp_,
    _ModuleDpl_,
    _ModuleUif_,
    _ModuleDrm_
 };
#define _eMfdSetInput enum _MfdSetInput_
 _eMfdSetInput {
    /* defines IDs of system modules */
    _ModuleDplFiltered,
    /*_ModuleDplRaw, Not used at present*/
    /*_ModuleDplFilteredAndRaw, Not used at present*/
    _ModuleDrmFiltered,
    /*_ModuleDrmRaw, May never be used? */
    _ModuleSspFiltered,
    _ModuleSspRaw,
    _ModuleSetLast
 };

/*************************************************
 * Data Import Module
 */
 typedef struct {
	/* Input data from h/w */
    Frac input; /* data input from H/W */
    Frac emg;   /* emg input from H/W */
    int gain;   /* h/w gain settings */
    int status;
 } Dimt_lobe_data;

 typedef struct {
    Dimt_lobe_data lobe_data[NUMBER_LOBES];
 } Dimt_data;

#define _NumberStrmSamples_ 32
 typedef struct {
	 /* Packed snapshot of input stream */
	 char input[_NumberStrmSamples_]; /* input stream */
	 int emg; /* filler */
	 int status;
 } tDfmRawPacked;

/*****************************************************
 * Digital Filter Module
 */
 typedef struct {
    Frac elem[NUMBER_FILTERS];
    int emg; /* Only LSByte is used */
    int input;
    int input128;
    int input64;
    int input32;
    int gain;
    int status;
 } Dfmt_lobe_data;

 typedef struct {
    Dfmt_lobe_data lobe_data[NUMBER_LOBES];
 } Dfmt_data;

 typedef struct {
    /* Structure that defines the gain of the elements relative to
     * each other
     */
    Frac elem[NUMBER_FILTERS];
    int emg; /* Only LSByte is used */
 } Dfmt_gain_adj;

/************************************************
 * Manage Filtered Data Module
 */
 enum mfde_req_type {
  /* Definition of the types of updates that can be performed by
   * mfd_acquire() module
   */
  MFDE_ALL_DATA,      /* All data */
  MFDE_UP_TO_4_5_HZ   /* Up to and includeing 4.5Hz data */
 };
/* use _eModuleId
 enum mfde_type {MFDE_NEW,
                 MFDE_REQUEST
                 };
*/
 typedef struct {
    Frac elem[NUMBER_FILTERS];
    int emg; /* Only LSByte is used */
    int input;
    int gain;
    int status;
 } Mfdt_lobe_data;

 typedef struct {
    Mfdt_lobe_data lobe_data[NUMBER_LOBES];
 } Mfdt_data;

/*************************************************
 * Display Module
 */
 typedef struct {
    Byte bar[NUMBER_BARGRAPHS];
    int gain;
 } Dplt_lobe_data;

 typedef struct {
    Dplt_lobe_data lobe_data[NUMBER_LOBES];
    int time_stamp; /* Unique id per tupl           */
    int status;     /* See mmstatus.h for definition */
 } Dplt_data;

/*************************************************
 * Analog HW interface
 */
 enum analog_registers {
       AG_LEFT_CTL,      /* Control of left cct                 */
       AG_RIGHT_CTL,     /* Control of right cct                */
       AG_TEST,          /* Control of test cct                 */
       AG_LEFT_DC_ADJ1,  /* First stage front end DC adjustment */
       AG_LEFT_DC_ADJ2,  /* Second stage front end DC adjustment*/
       AG_LEFT_AD_ADJ,   /* A/D DC shift adjusment              */
       AG_LEFT_GAIN_ADJ, /* Fourth Stage gain adjusment         */
       AG_RIGHT_DC_ADJ1, /* First stage front end DC adjustment */
       AG_RIGHT_DC_ADJ2, /* Second stage front end DC adjustment*/
       AG_RIGHT_AD_ADJ,  /* A/D DC shift adjusment              */
       AG_RIGHT_GAIN_ADJ /* Fourth Stage gain adjusment         */
 };



/************************************************
 * 
 */


