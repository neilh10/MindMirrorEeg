/* Mind Mirror Unit status line layout
 * dpl_stat.h    -   ## Biomonitors Mind Mirror III ##
 *
 * Description
 *
 * Defines for placing the status line characters on
 * the Mind Mirror Unit display line.
 *
 * 16 Characters available
 *
 */
#ifndef dpl_stat_h
#define dpl_stat_h
/*
 * Constants
 */
#define MMIR_STATUS_BUF_SIZE 20/* should be same as LDD_MAX_STATUS_LENGTH*/

/*
 * Status Line Positioning constants
 */
/* Defines for time placement
 *
 *  012345
 *  mm:ss
 */
#define MMIR_TIME_SIZE      6
#define MMIR_TIME_MIN_T_POS 0
#define MMIR_TIME_MIN_U_POS 1
#define MMIR_TIME_COLON_POS 2
#define MMIR_TIME_SEC_T_POS 3
#define MMIR_TIME_SEC_U_POS 4
#define MMIR_TIME_SPC       5

#define SCALE_STARTING_POS 5
#define SSS_SIZE 6
#define REVIEW_MODE_POS    MMIR_TIME_COLON_POS
#define MONTAGE_POS        12  /* decimal */
#define RECORDING_RATE_POS 13  /* decimal */
#define PAUSE_POS          14  /* decimal */
#define BATTERY_POS        15  /* decimal */

/*
 * Module Prototypes
 */
 void dpl_status(char *mmir_status_buf,
                 int in_status,
                 int time_stamp,
                 int att
                 /* int record_no Not used in this version */
                 );


#endif /* dpl_stat_h */











