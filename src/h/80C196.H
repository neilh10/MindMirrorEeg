/* 80C196.h
 * Copyright (c) 1990 Intel Corporation, ALL RIGHTS RESERVED.
 *
 * 80C196.h  - declarations for 80C196 SFRs (a superset of 
 *             8096 registers) and 80C196-specific library
 *             function declarations
 */

#ifndef _80c196h
#define _80c196h

extern volatile unsigned short r0;         /* at 0x00: r   */
extern volatile unsigned char ad_command;  /* at 0x02:   w */
extern volatile unsigned char ad_result_lo;/* at 0x02: r   */
extern volatile unsigned char ad_result_hi;/* at 0x03: r   */
extern volatile unsigned char hsi_mode;    /* at 0x03:   w */
extern volatile unsigned short hso_time;   /* at 0x04:   w */
extern volatile unsigned short hsi_time;   /* at 0x04: r   */
extern volatile unsigned char hso_command; /* at 0x06:   w */
extern volatile unsigned char hsi_status;  /* at 0x06: r   */
extern volatile unsigned char sbuf;        /* at 0x07: r/w */
extern volatile unsigned char int_mask;    /* at 0x08: r/w */
extern volatile unsigned char int_pending; /* at 0x09: r/w */
extern volatile unsigned char watchdog;    /* at 0x0a:   w */
extern volatile unsigned short timer1;     /* at 0x0a: r   */
extern volatile unsigned short timer2;     /* at 0x0c: r   */
extern volatile unsigned char baud_rate;   /* at 0x0e:   w */
extern volatile unsigned char ioport0;     /* at 0x0e: r   */
extern volatile unsigned char ioport1;     /* at 0x0f: r/w */
extern volatile unsigned char ioport2;     /* at 0x10: r/w */
extern volatile unsigned char sp_con;      /* at 0x11:   w */
extern volatile unsigned char sp_stat;     /* at 0x11: r   */
extern volatile unsigned char ioc0;        /* at 0x15:   w */
extern volatile unsigned char ios0;        /* at 0x15: r   */
extern volatile unsigned char ioc1;        /* at 0x16:   w */
extern volatile unsigned char ios1;        /* at 0x16: r   */
extern volatile unsigned char pwm_control; /* at 0x17:   w */

/****************************************************************************/
/*                                                                          */
/* Additional SFRs of the 80C196                                            */
/*                                                                          */
/****************************************************************************/

extern volatile unsigned char ioc2;        /* at 0x0b:   w */
extern volatile unsigned char ipend1;      /* at 0x12: r/w */
extern volatile unsigned char imask1;      /* at 0x13: r/w */
extern volatile unsigned char wsr;         /* at 0x14: r/w */
extern volatile unsigned char ios2;        /* at 0x17: r   */

/****************************************************************************/
/*                                                                          */
/* Additional SFRs of the 80C196KC                                          */
/*                                                                          */
/****************************************************************************/

extern volatile unsigned char  ad_time;     /* at 0x03: r/w */
extern volatile unsigned short ptssel;      /* at 0x04: r/w */
extern volatile unsigned short ptssrv;      /* at 0x06: r/w */
extern volatile unsigned char  t2control;   /* at 0x0c: r/w */
extern volatile unsigned char  pwm1_control;/* at 0x16: r/w */
extern volatile unsigned char  pwm2_control;/* at 0x17: r/w */

#if _ARCHITECTURE_ == 'KC' || _ARCHITECTURE_ == 'KR'
/*
 * Single Transfer PTS Control Block
 */
typedef struct STran_ptscb_t {
                             unsigned char  ptscount;
                             struct {
                                    unsigned int  di : 1,
                                                  si : 1,
                                                  du : 1,
                                                  su : 1,
                                                 b_w : 1,
                                                mode : 3;
                                    } ptscon;
                             void  *ptssrc;
                             void  *ptsdst;
                             int   :16; /* unused */
                             } STran_ptscb;


/*
 * Block Transfer PTS Control Block
 */
typedef struct BTran_ptscb_t {
                             unsigned char  ptscount;
                             struct {
                                    unsigned int  di : 1,
                                                  si : 1,
                                                  du : 1,
                                                  su : 1,
                                                 b_w : 1,
                                                mode : 3;
                                    } ptscon;
                             void  *ptssrc;
                             void  *ptsdst;
                             unsigned char  ptsblock;
                             int   :8; /* unused */
                             } BTran_ptscb;


/*
 * A/D Mode PTS Control Block
 */
typedef struct AD_ptscb_t {
                           unsigned char  ptscount;
                           struct {
                                  unsigned int  const1 : 3,
                                                updt   : 1,
                                                const2 : 1,
                                                mode   : 3;
                                  } ptscon;
                           unsigned int  s_d;
                           unsigned int  reg;
                           int   :16; /* unused */
                           } AD_ptscb;


/*
 *  HSI Mode PTS Control Block
 */
typedef struct HSI_ptscb_t {
                           unsigned char  ptscount;
                           struct {
                                  unsigned int  const1 : 3,
                                                updt   : 1,
                                                const2 : 1,
                                                mode   : 3;
                                  } ptscon;
                           unsigned int  :16;
                           unsigned int  ptsdst;
                           unsigned char ptsblock;
                           int   : 8; /* unused */
                          } HSI_ptscb;


/*
 * HSO Mode PTS Control Block
 */
typedef struct HSO_ptscb_t {
                           unsigned char  ptscount;
                           struct {
                                  unsigned int  const1 : 3,
                                                updt   : 1,
                                                const2 : 1,
                                                mode   : 3;
                                  } ptscon;
                           unsigned int  ptssrc;
                           unsigned int  :16;
                           unsigned char ptsblock;
                           int   : 8; /* unused */
                          } HSO_ptscb;


/*
 *  PTS A/D Table
 */
typedef struct AD_tab_t {
                         unsigned char  AD_command;
                         unsigned int   AD_result;
                        } AD_tab;

/*
 *  PTS HSI Table
 */
typedef struct HSI_tab_t {
                         unsigned char  HSI_status_lo;
                         unsigned char  HSI_status_hi;
                         unsigned int   HSI_time;
                        } HSI_tab;

/*
 *  PTS HSO Table
 */
typedef struct HSO_tab_t {
                         unsigned char  HSO_command;
                         unsigned int   HSO_time;
                        } HSO_tab;

#endif

/****************************************************************************/
/*                                                                          */
/* Additional C96.LIB functions supported by the 80C196 only                */
/*                                                                          */
/****************************************************************************/

void enable(void);
void disable(void);

void power_down(void);
void idle(void);

void enable_pts(void);
void disable_pts(void);

#endif /* _80c196h */
