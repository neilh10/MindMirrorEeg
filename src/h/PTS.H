/*
  pts.h

This file contains Peripheral Transaction Server definitions.

Note: These must be allocated to internal RAM in the CPU
*/

/* PTS channel defines - page 5-38
 * 765  000 PTS Block Transfer Mode
 *      100 PTS Single Transfer Mode 
 * 4 B/W Byte(1)/Word(0) transfer
 * 3 SU Update PTSSRC at end of PTS cycle
 * 2 DU Update PTSDST at end of PTS cycle
 * 1 SI PTSSRC auto-increment
 * 0 DI PTSDST auto-increment
 */
/*for single byte transfer we have 1001 1010 */
#define PTS_UART_TX_CTL 0x9A

/*
 * 765  110 PTS A/D Mode Mode
 *      011 PTS HSI Mode
 *      001 PTS HSO Mode 
 * 4 0
 * 3 UPDT Optional Update of Incremented value to
 *        A/D - S/D Register
 *        HSI - PTSDST
 *        HSO - PTSSRC
 * 2 0
 * 1 1
 * 0 0
 */
/* A/D Mode we have 1100 1010 ??? */
#define PTS_A_D_MODE 0xCA  /* ????? */

/*
 * PTS MASK Bits in PTSSEL and PTSSRV registers
 * Pg 5-37
 */
#define PTS_TIMER_OVF_MASK  0x0001
#define PTS_AD_DONE_MASK    0x0002
#define PTS_HSI_DATA_MASK   0x0004
#define PTS_HSO_PIN_MASK    0x0008
#define PTS_HSI_1_MASK      0x0010
#define PTS_SOFT_TIMER_MASK 0x0020
#define PTS_SER_PORT_MASK   0x0040
#define PTS_EXT_INT_MASK    0x0080
#define PTS_TI_MASK         0x0100
#define PTS_RI_MASK         0x0200
#define PTS_HSI4_MASK       0x0400
#define PTS_T2CAP_MASK      0x0800
#define PTS_T2_OVF_MASK     0x1000
#define PTS_EXT_INT1_MASK   0x2000
#define PTS_FIFO_FULL_MASK  0x4000
#define PTS_RSV_MASK        0x8000

 typedef struct { 
    /* See also definition for STran_ptscb in 80C196.h*/
    unsigned char ptscount;
    unsigned char ptscon;
    void *ptssrc ;
    void *ptsdst;
    /* two unused bytes go here */
 } Pts_single_transfer;

 typedef struct { 
    /* See also definition for BTran_ptscb in 80C196.h*/
    unsigned char ptscount;
    unsigned char ptscon;
    unsigned char *ptssrc ;
    unsigned char *ptsdst;
    unsigned char ptsblock;
    /* one unused bytes go here */
 } Pts_block_transfer;

 typedef struct { 
    /* See also definition for AD_ptscb in 80C196.h*/
    unsigned char ptscount;
    unsigned char ptscon;
    unsigned int  s_d;
    unsigned int  reg;
    /* two unused bytes go here */
 } Pts_ad_mode; /* Analog to digital */

 typedef struct { 
    /* See also definition for HSI_ptscb in 80C196.h*/
    unsigned char ptscount;
    unsigned char ptscon;
    unsigned char  *ptssrc ;
    unsigned int  unused;
    unsigned char ptsblock;
    /* one unused byte goes here */
 } Pts_hs_mode; /* high speed input and high speed output*/




