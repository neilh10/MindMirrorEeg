/*
 * mmstatus.h    -   ## Biomonitors Mind Mirror III ##
 *
 * Description
 *
 * Defines for the status and gain/log_att bytes in 
 *    Dllt_rt
 *  & Dplt_data
 *
 */
#ifndef mmstatus_h
#define mmstatus_h
/*
 * Constants
 */

/******* Status indications *******
 *
 * This an 8 bit byte
 */
/* Bit 0 - Battery Bit
 *  0 - Battery OK 
 *  1 - Battery LOW
 */
#define MMS_BATTERY_MASK 1

/* Bit 1  - MMIR Display Mode mask
 * Defines the type of data received
 */
#define MMS_DM_MASK 0x0002
#define MMS_DM_DATA   0x00
#define MMS_DM_REVIEW 0x02  /* Data is being reviewed               */

/* Bit 2 - Demonstration Bit
 * Indicates that demonstration is in operation
 *  0 - data is not a demonstration pattern
 *  1 - indicates demonstration pattern
 */
#define MMS_DEMO_MASK 0x04
#define MMS_DEMO_ACTIVE MMS_DEMO_MASK

/* Bit 3 - Pause Bit
 * Indicates that recordings are not in operation
 *  0 - data is being recorded in Mind Mirror III internal RAM
 *  1 - indicates pause in operation
 */
#define MMS_PAUSE_MASK 0x08

/* Bit 4,5, - Recording rate
 * Defines the update period between records.
 *
 * 26/Sept/92 Only MMS_RR_0_5 is supported
 */
#define MMS_RR_MASK  0x30
#define MMS_RR_0_125 0x00 /* Every 1/8 second */
#define MMS_RR_0_25  0x10 /* Every 1/4 second */
#define MMS_RR_0_5   0x20 /* Every 1/2 second */
#define MMS_RR_1_0   0x30 /* Every Second     */

/* Bit 6,7 - Montage Arrangement
 */
#define MMS_MONTAGE_MASK       0xc0
#define MMS_MONTAGE_LATERAL    0x40
#define MMS_MONTAGE_RED_GREEN  0x80
#define MMS_MONTAGE_BLACK_GREY 0xc0

/* Bit 15 - Statistic Record
 * PC only - defines tupl as statistically generated record
 */
#define MMS_STAT_MASK       0x8000

/* Define the state a message is normally initialised to
 */
#define MMS_INIT_STATE (MMS_RR_0_5|MMS_DM_DATA)


#ifdef MMS_ATT_TABLE
/*
 * Attenuator control variable
 */
#define SS_SIZE 13
 const char mms_att_table[SS_SIZE][6] = {
      {'B','E','G','I','N',' '},    /* Spacing     */
      {' ',' ',' ','1','u','V'},    /* NO_ATT      */
      {' ',' ',' ','3','u','V'},    /* HWE_ATT_3   */
      {' ',' ',' ','5','u','V'},    /* HWE_ATT_5   */
      {' ',' ','1','0','u','V'},    /* HWE_ATT_10  */
      {' ',' ','3','0','u','V'},    /* HWE_ATT_30  */
      {' ',' ','5','0','u','V'},    /* HWE_ATT_50  */
      {' ','1','0','0','u','V'},    /* HWE_ATT_100 */
      {' ','3','0','0','u','V'},    /* HWE_ATT_300 */
      {' ','5','0','0','u','V'},    /* HWE_ATT_500 */
      {'E','R','R','O ','R','2'},   /* Spacing     */
      {' ','T','E','S','T',' '},    /* HWE_TEST    */
      {'S',' ','T','E','S','T'}     /* HWE_TEST_S  */
  };

#endif  /* MMS_ATT_TABLE */


/*
 * Module Prototypes
 */

#endif /* mmstatus_h */











