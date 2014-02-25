/*
  hw.h

  Defines the hardware interfaces
*/
#ifndef hw_h
#define hw_h
#define DIG_HW_REV4
/*
 * Hardware calibration setting
 * This is the setting of the bars on the screen, and indicates
 * that the measured signal is at the indicated strength
 *ie 1uV,3uV... etc
 */
#define HW_CALIBRATION_SETTING 42  /* Approx at present */
/*
 * Interrupt mask definitions for INT_MASK and INT_PEND
 */
#define INT_TIMER_OVF_MASK  0x01
#define INT_AD_DONE_MASK    0x02
#define INT_HSI_DATA_MASK   0x04
#define INT_HSO_PIN_MASK    0x08
#define INT_HSI_0_MASK      0x10
#define INT_SOFT_TIMER_MASK 0x20
#define INT_SER_PORT_MASK   0x40
#define INT_EXT_INT_MASK    0x80

/*
 * Interrupt mask definitions for INT_MASK1 and INT_PEND1
 */
#define INT1_TI_MASK        0x01
#define INT1_RI_MASK        0x02
#define INT1_HSI4_MASK      0x04
#define INT1_T2CAP_MASK     0x08
#define INT1_T2OVF_MASK     0x10
#define INT1_EXT_INT1_MASK  0x20
#define INT1_FIFO_FULL_MASK 0x40
/*#define INT1_NMI_MASK 1 */
/*
 * CAM Interfaces
 * Pg 5-49 Fig 10-1
 */
#define FLUSH_CAM 0xc0    /* Written to ioc2 */
/*#define EN_CAM_LOCK 0x40  /* writtern to ioc2 */
    /* The following are written to hso_command */
#define LED_ON 0x80  /* Lock in CAM ?, HSO0=0? */
#define LED_OFF 0x0a0/* Lock in CAM ?, HSO=1?  */
   /* For controlling analog switched cap filter chip 
    *     - Generate 4.4Khz/227uS even mark space ratio
    *
    * The following frequencies were generated and measured
    * using a RCS Electronics 250MHz Counter-Timer Type 801B
    *                Freq      Value
    *                5.320KHz  70
    *                4.968KHz  75
    *                4.841KHz  77
    *                4.778KHz  78
    *                4.718KHz  79
    *                4.660KHz  80
    *                4.603KHz  81
    *                4.547KHz  82
    *                4.492KHz  83
    *                4.439KHz  84
    *                4.337KHz  86
    *                4.286KHz  87
    *                4.238KHz  88
    *                4.191KHz  89
    *     - Lock in CAM
    *     - based on timer 2
    *     - HSO.0
    */
#define SET_FILTER_FREQ_OP  0xe0 /*0xa0 0xe0 */
#define CLR_FILTER_FREQ_OP  0xc0 /*0x80 0xc0 */
#define RESET_TIMER2        0xce

#if defined(IC96)
#define SET_FILTER_FREQ_CONST 78  /* Based on @12Mhz */

#if defined(CLOCKC20)
#define SET_FILTER_FREQ_VALUE ((SET_FILTER_FREQ_CONST*20)/12)/*20Mhz*/

#elif defined(CLOCKC16)
#define SET_FILTER_FREQ_VALUE ((SET_FILTER_FREQ_CONST*16)/12)/*16Mhz*/

#elif defined(CLOCKC12)
#define SET_FILTER_FREQ_VALUE SET_FILTER_FREQ_CONST /*12MHz*/

#elif defined(CLOCKC6_4)
#define SET_FILTER_FREQ_VALUE ((SET_FILTER_FREQ_CONST*64)120)/*6.4MHz*/
#else
 error - no valid SYSCLOCK
#endif

#define CLR_FILTER_FREQ_VALUE (2 * SET_FILTER_FREQ_VALUE)
#endif /* defined(IC96) */
   /*
    * For controlling HW test generation frequencies
    *    - not locked in CAM
    *    - based on timer 1
    *    - HSO.3 & HSO.4
    *    - HZ9 & HZ15
    *    See Pg 10-1 80C196KC 1990 16 Bit Embedded Controllers Manual
    */
#define RES_HSO_1    0x01 /* Based on timer1 */
#define SET_HSO_1    0x21 /* Based on timer1 */
#define RES_HSO_3    0x03 /* Based on timer1 */
#define SET_HSO_3    0x23 /* Based on timer1 */
#define RES_HSO_4    0x04 /* Based on timer1 */
#define SET_HSO_4    0x24 /* Based on timer1 */

#define SET_HSO_HZ9  SET_HSO_3
#define RES_HSO_HZ9  RES_HSO_3

#if defined(DIG_HW_REV3)
#define SET_HSO_HZ15 SET_HSO_4
#define RES_HSO_HZ15 RES_HSO_4
#elif defined(DIG_HW_REV4)
#define SET_HSO_HZ15 SET_HSO_1
#define RES_HSO_HZ15 RES_HSO_1
#endif

/* Event Input Defintions 
* See manual Pg 5-47 HSI_STATUS
*/
#define HSI0_STATUS_MASK 0x0002
#define HSI3_STATUS_MASK 0x0080

#if defined(DIG_HW_REV3)
#define EVENT_HSI_MASK HSI3_STATUS_MASK
#elif defined(DIG_HW_REV4)
#define EVENT_HSI_MASK HSI0_STATUS_MASK
#endif
/*
 * LED flashing rates
 */
 enum led_flash {SYS_AWAKE,SYS_GETTING_UP};

/*
 * Uart driver commands
 */
 enum uart_driver_cmd {UART1_DRV_OFF,UART1_DRV_ON};

/*
 * Port bit defines
 */
#define PORT1BIT0 0x01
#define PORT1BIT1 0x02
#define PORT1BIT2 0x04
#define PORT1BIT3 0x08

/*
 * Horizontal window access
 * to be written to 'wsr' in any HWINDOW
 */
#define HWINDOW0 0
#define HWINDOW1 1
#define HWINDOW15 0x0f
/*
 * Serial Port Initialiser
 *  80C196KC Pg 5-54
 * Bits
 * 01  01 MODE1 Start,8 data, Stop bit (10 bits)
 * 2   0  PEN - Disables Parity
 * 3   1  REN - Enables Receiver
 * 4   0  TB8 - Not Used
 * 567 0  Future Compatibility
 */
#define SP_CON_INITIALISE 0x09

/*
 * IOC1 mask
 * Pg 5-108 80C196KC
 *
 * Bit R3 R4                                1  /  0
 *  0  1  1 HSI.0 Input                 enable/disable
 *  1  0  0 Timer 2 Reset Each Write    enable/disable
 *  2  1  1 HSI.1 Input                 enable/disable
 *  3  0  0 Timer 2 external reset      enable/disable
 *  4  0  0 HSI.2 Input                 enable/disable
 *  5  0  0 Timer 2 reset source        HSI.0/T2RST
 *  6  1  0 HSI.3 input                 enable/disable
 *  7  0  0 Timer 2 clock source        HSI.1/T2CLK
 */
#define HW_DIG_REV4
#if defined(HW_DIG_REV3)
#define IOC0_INITIALISE  0x45
#elif defined(HW_DIG_REV4)
#define IOC0_INITIALISE  0x05
#endif

/*
 * IOC1 mask
 * Pg 5-108 80C196KC
 *
 * Bit R3 R4                                1  /  0
 *  0  1  1 Select                        PWM / P2.5
 *  1  0  0 Select External Interrupt    ACH7 / EXTINT
 *  2  0  0 Select Timer1 Overflow Int Enable / Disable
 *  3  0  0 Select Timer2 Overflow Int Enable / Disable
 *  4  1  1 HSO.4 output               Enable / Disable
 *  5  1  1 Select                     TXD    / P2.0
 *  6  0  1 HSO.5 output               Enable / Disable
 *  7  0  0 HSI Interrupt           Fifo Full / Holding reg loaded
 */
#if defined(HW_DIG_REV3)
#define ICON1_INITIALISE  0x31
#elif defined(HW_DIG_REV4)
#define ICON1_INITIALISE  0x71
#endif 
/*
 * IOC2 mask
 * Pg 5-109 80C196KC
 *
 * Bit                                 1  /  0
 *  0  0 Enable fast increment of T2 Enable / Disable
 *  1  0 Enable T2 as UP/Down Cntr   Enable / Disable
 *  2  1 Enable /2 prescaler on PWMs Enable / Disable
 *  3  0 Enable 80C196KC A/D Modes   Enable / Disable
 *  4  0 A/D Clock prescaler disable Disable/ Enable 
 *  5  0 T2 Alternate int @ 8000H    Enable / Disable
 *  6  1 Enable Locked CAM entries   Enable / Disable
 *  7  0 Clear Entire CAM            Clear  / NOP
 * 
 */
#define IOC2_INITIALISE 0x44

/*
 * Baud rate initialiser for 9600 baud.
 * Use Xtal - Bit7 = 1
 * Note Different formuleas for 8096 and 80C196x
 * Formulea for 80C196KC
 *   BAUD_REG = (XTAL1/(Baud Rate * 16) -1 ) | 0x8000
 *    ie in decimal
 *            (16E6/(9600 * 16) -1  = 103
 *       in hex  =67
 *   BAUD_REG = 0x8067
 *
 *   BAUD_REG = (XTAL1/(Baud Rate * 16) -1 ) | 0x8000
 *   for   9600baud  (20,000,000/(9600 * 16) -1  = 129
 *       in hex  =0x81 BAUD_REG = 0x8081
 *
 *   for  19200baud  (20,000,000/(19200 * 16) -1  = 64
 *       in hex  =0x40     BAUD_REG = 0x8040
 *
 *   for 115,200baud (20,000,000/(115200 * 16) -1  = 10
 *       in hex  =0x0A     BAUD_REG = 0x800A

  *
 */
#if defined(IC96)

#if defined(CLOCKC20)
 /* at 80c196Kx at 20MHz. Pg 5-55 */
#define BAUD_REG_9600   0x8081
#define BAUD_REG_19200  0x8040
#define BAUD_REG_115200 0x800A

#elif defined(CLOCKC16)
 /* at 80c196Kx at 16MHz. Pg 5-55 */
#define BAUD_REG_9600   0x8067
#elif defined(CLOCKC12)
 /* at 80c196Kx at 12MHz. Pg 5-55 */
#define BAUD_REG_9600   0x804d
#elif defined(CLOCKC6_4)
 /* 80C196Kx at 6.4MHz. Formulea Pg 5-55 */
#define BAUD_REG_9600   0x8029
#else
 error - no valid SYSCLOCK
#endif
#endif /* defined(IC96) */

/* A/D Channel
 */
#define NUMBER_AD_CONVERTERS 4 /*Number used by analog board 1->8*/
#define NUMBER_AD_CHANNELS 8   /* Total number on chip */

/* A/D Channel Labels - rev1->4
 * Expect HW_AD_LB_LOBE, HW_AD_RB_LOBE to be consecutive
 * Expect HW_AD_LB_EMG,  HW_AD_RB_EMG to be consecutive
 */
#define HW_AD_LB_LOBE 0
#define HW_AD_RB_LOBE 1
#define HW_AD_LB_EMG  2
#define HW_AD_RB_EMG  3
 

/* A/D Channels numbers other than the first four
 */
 enum ade_channels {
      ADC_S1_INPUT=6, /* ADC_S1_INPUT_MASK must be kept in sync */
      ADC_BAT_FAIL=7  /* ADC_BAT_FAIL_MASK must be kept in sync */
 };
 enum ade_channel_masks {
      ADC_S1_INPUT_MASK=0x40,
      ADC_BAT_FAIL_MASK=0x80
 };
/*
 * Front end routing
 */
enum hwe_fe_in { HW_FE_LATERAL_IN,  /* lateral/differential montage */
                 HW_FE_FRONT_IN, /* montage reference to earth*/
                 HW_FE_BACK_IN,  /* montage reference to earth*/
     HW_FE_SELF_TEST, HW_FE_CNTCT_TEST, HW_FE_OPEN_CCT_TEST,
     HW_FE_SHORT_CCT_TEST };

/*
 * Analog front end initialisation
 *
 * 
 */
#define DG308_CLOSE   1  /* HW gate is closed */
#define DG308_OPEN    0  /* HW gate is open   */
#define BIT_LB1   0
#define BIT_LB2   1
#define BIT_LB3   2
#define BIT_LB4   3
#define BIT_LG3   4
#define BIT_LG5   5
#define BIT_LG10  6
#define BIT_LG100 7
#define JUST_FRONT_END_ATT_MASK 0xf0 /* Bits LG3->LG100  */
#define JUST_FRONT_END_MONTAGE_MASK 0x0f /* Bits LB1->LB4 */

#define NO_ATT                ( \
   (DG308_OPEN  << BIT_LG100) | \
   (DG308_OPEN  << BIT_LG10)  | \
   (DG308_OPEN  << BIT_LG5)   | \
   (DG308_OPEN  << BIT_LG3)   )

#define ATT_3                 ( \
   (DG308_OPEN  << BIT_LG100) | \
   (DG308_OPEN  << BIT_LG10)  | \
   (DG308_OPEN  << BIT_LG5)   | \
   (DG308_CLOSE  << BIT_LG3)   )

#define ATT_5                 ( \
   (DG308_OPEN  << BIT_LG100) | \
   (DG308_OPEN  << BIT_LG10)  | \
   (DG308_CLOSE << BIT_LG5)   | \
   (DG308_OPEN  << BIT_LG3)   )

#define ATT_10                ( \
   (DG308_OPEN  << BIT_LG100) | \
   (DG308_CLOSE << BIT_LG10)  | \
   (DG308_OPEN  << BIT_LG5)   | \
   (DG308_OPEN  << BIT_LG3)   )

#define ATT_15                ( \
   (DG308_OPEN  << BIT_LG100) | \
   (DG308_OPEN  << BIT_LG10)  | \
   (DG308_CLOSE << BIT_LG5)   | \
   (DG308_CLOSE << BIT_LG3)   )

#define ATT_30                ( \
   (DG308_OPEN  << BIT_LG100) | \
   (DG308_CLOSE << BIT_LG10)  | \
   (DG308_OPEN  << BIT_LG5)   | \
   (DG308_CLOSE << BIT_LG3)   )

#define ATT_50                ( \
   (DG308_OPEN  << BIT_LG100) | \
   (DG308_CLOSE << BIT_LG10)  | \
   (DG308_CLOSE << BIT_LG5)   | \
   (DG308_OPEN  << BIT_LG3)   )

#define ATT_100               ( \
   (DG308_CLOSE << BIT_LG100) | \
   (DG308_OPEN  << BIT_LG10)  | \
   (DG308_OPEN  << BIT_LG5)   | \
   (DG308_OPEN  << BIT_LG3)   )

#define ATT_300               ( \
   (DG308_CLOSE << BIT_LG100) | \
   (DG308_OPEN  << BIT_LG10)  | \
   (DG308_OPEN  << BIT_LG5)   | \
   (DG308_CLOSE << BIT_LG3)   )

#define ATT_500               ( \
   (DG308_CLOSE << BIT_LG100) | \
   (DG308_OPEN  << BIT_LG10)  | \
   (DG308_CLOSE << BIT_LG5)   | \
   (DG308_OPEN  << BIT_LG3)   )

/* Was NORMAL_FRONT_END */
#define LATERAL_MONTAGE_FRONT_END ( \
   (DG308_OPEN  << BIT_LB4)   | \
   (DG308_CLOSE << BIT_LB3)   | \
   (DG308_OPEN  << BIT_LB2)   | \
   (DG308_CLOSE << BIT_LB1) )

#define FRONT_MONTAGE_FRONT_END ( \
   (DG308_OPEN  << BIT_LB4)   | \
   (DG308_CLOSE << BIT_LB3)   | \
   (DG308_CLOSE << BIT_LB2)   | \
   (DG308_OPEN  << BIT_LB1) )

#define BACK_MONTAGE_FRONT_END ( \
   (DG308_CLOSE << BIT_LB4)   | \
   (DG308_OPEN  << BIT_LB3)   | \
   (DG308_CLOSE << BIT_LB2)   | \
   (DG308_OPEN  << BIT_LB1) )

#define SC_FE                 ( \
   (DG308_CLOSE << BIT_LB4)   | \
   (DG308_OPEN  << BIT_LB3)   | \
   (DG308_CLOSE << BIT_LB2)   | \
   (DG308_CLOSE << BIT_LB1) )

#define OC_FE                 ( \
   (DG308_OPEN  << BIT_LB4)   | \
   (DG308_OPEN  << BIT_LB3)   | \
   (DG308_OPEN  << BIT_LB2)   | \
   (DG308_OPEN  << BIT_LB1) )

/* TEST Shift register definitions */
#define BIT_TEST_ACT 0
#define BIT_T1       1
#define BIT_T2       2
#define BIT_T3       3
#define BIT_T4       4
#define BIT_T5       5
#define BIT_T6       6
#define BIT_T7       7

#define BIT_EMG_RIGHT  BIT_T1
#define BIT_HZ9_OP     BIT_T2
#define BIT_HZ12_OP    BIT_T3
#define BIT_L_PRE2     BIT_T4
#define BIT_L_PRE1     BIT_T5
#define BIT_R_PRE2     BIT_T6
#define BIT_R_PRE1     BIT_T7

#define AG_TEST_DISABLE (DG308_OPEN  << BIT_TEST_ACT)
#define AG_TEST_ENABLE  (DG308_CLOSE << BIT_TEST_ACT)
#define EMG_RIGHT           (\
   (DG308_CLOSE << BIT_EMG_RIGHT) |\
   (DG308_OPEN  << BIT_HZ9_OP) |\
   (DG308_OPEN  << BIT_HZ12_OP) |\
   (DG308_OPEN  << BIT_L_PRE2) |\
   (DG308_OPEN  << BIT_L_PRE1) |\
   (DG308_OPEN  << BIT_R_PRE2) |\
   (DG308_OPEN  << BIT_R_PRE1)  )

#define INIT_AG_FRONT_END   (NO_ATT  )
#define FE_ATT_0            (NO_ATT  )
#define FE_ATT_3            (ATT_3   )
#define FE_ATT_5            (ATT_5   )
#define FE_ATT_10           (ATT_10  )
#define FE_ATT_15           (ATT_15  )
#define FE_ATT_30           (ATT_30  )
#define FE_ATT_50           (ATT_50  )
#define FE_ATT_100          (ATT_100 )
#define FE_ATT_300          (ATT_300 )
#define FE_ATT_500          (ATT_500 )
#define FE_SHORT_CCT        (NO_ATT  | SC_FE)
#define FE_OPEN_CCT         (NO_ATT  | OC_FE)
#define FE_SELF_TEST        (ATT_100 | OC_FE)
#define FE_CNTCT_TEST       (ATT_100 | LATERAL_MONTAGE_FRONT_END)

#define INIT_AG_TEST        (EMG_RIGHT | AG_TEST_DISABLE)
#define FE_TEST_DISABLE     INIT_AG_TEST
#define FE_TEST_ENABLE      (EMG_RIGHT | AG_TEST_ENABLE)

/*
 * Test frequency generation
 */
enum test_freq_type {TEST_FREQ_ENABLE, TEST_FREQ_DISABLE};
/*
 * Keys defined Mind Mirror keypad
 */
enum key_pressed_enum {NO_KEY=0,
                 KEY_DISPLAY=1,KEY_UP_ARROW=2,KEY_TEST=3,
                 KEY_LEFT_ARROW=4,KEY_OPTIONS=5,KEY_RIGHT_ARROW=6,
                 KEY_RESTART=7,KEY_DOWN_ARROW=8,KEY_REVIEW=9,
                 KEY_EXT_EVENT,KEY_REPEAT1,KEY_REPEAT2,KEY_ACTIVE,
                 UNDEFINED_KEY};

/* defines for hw_keypad_poll key_repeat_times
 */
#define HWC_KEYPAD_FSM_CALLS 8
 enum hwt_key_time { /* Times in number of calls of hw_keypad_fsm */
    HWE_KEY_TIME_FASTEST=0,
    HWE_KEY_TIME_0_25SEC=HWC_KEYPAD_FSM_CALLS/4,/* 0.25 Seconds */
    HWE_KEY_TIME_0_5SEC=HWC_KEYPAD_FSM_CALLS/2, /* 0.5 Seconds  */
    HWE_KEY_TIME_1SEC=HWC_KEYPAD_FSM_CALLS,   /* 1 second of key press */
    HWE_KEY_TIME_2SEC=2*HWC_KEYPAD_FSM_CALLS, /* 2 seconds of key press */
    HWE_KEY_TIME_5SEC=5*HWC_KEYPAD_FSM_CALLS, /* 5 seconds of key press */
    HWE_KEY_TIME_END=0x7000
 };
/* defines for setting time in hwt_key_status.key
 */
/*#define HWC_KEY_TIME1 0x4000*/
/*#define HWC_KEY_TIME2 0x8000 */
/*#define HWC_KEY_TIME_MASK (HWC_KEY_TIME1|HWC_KEY_TIME2) */
/*
 * Interfaces to hw_attenuator
 */
 enum hwe_att_setting {HWE_BOTTOM_NULL, /* Bottom - also marker in
                                         * drm_ram for start of buffer
                                         */
      /* Note: Order is important - see hw_attenuator() &hw_trans_table
       *       Must align with values in  mms_att_table in mmstatus.h 
       */
                       HWE_ATT_1,HWE_ATT_3,HWE_ATT_5,HWE_ATT_10,
                       HWE_ATT_30,HWE_ATT_50,HWE_ATT_100,
                       HWE_ATT_300,HWE_ATT_500,
                       HWE_END_ATT, /* End marker - also marker in drm_ram*/
                       HWE_TEST,HWE_TEST_S, /* Test values used in display */
                       HWE_INC_ATT,HWE_DECR_ATT,HWE_REFRESH
                       };
enum hwe_which_att {HWE_ATT_BOTH,HWE_ATT_LEFT,HWE_ATT_RIGHT};

  /* Struct to hold status information, accessibile through
   * hw_analag_stat to other procedures
   */
 typedef struct {
  /* hwe_att_setting
   * Current attenuator setting in hardware
   */
  enum hwe_att_setting hw_att_setting;
  /* Current montage setting in hardware */
  enum hwe_fe_in montage;
 } hwt_fec_status;

 /* struct to hold info about keys pressed
  */
 typedef struct {
    enum key_pressed_enum key;
    /*enum key_time_enum time_status;*/
    uint timer1, timer2;
    Semaphore new_event;
 } hwt_key_status;

/*
 * Prototypes
 */
 void hw_test_int(void);  /* Should only be called from one place */
 enum key_pressed_enum hw_keypad_poll(enum hwt_key_time time1, enum hwt_key_time time2);
 hwt_key_status *hw_keypad_status(void);
 void hw_keypad_event_ack(void);
 void hw_keypad_fsm(void);
 void hw_front_end_mode(enum hwe_fe_in fe_in,enum hwe_which_att which_filter,enum hwe_att_setting type );
 void hw_attenuator(enum hwe_which_att which,enum hwe_att_setting type);
 void hw_att_init(Initt_system_state *ssp);
 enum hwe_att_setting hw_att_value(int side);
 int  hw_lcd_contrast_set(int pwm_value);
 Byte *hw_sn(Byte *serial_number_p);
 int hw_event_key(void);
 int uart_driver(enum uart_driver_cmd cmd);
 void hw_fec_status(hwt_fec_status *p);

#endif /* hw_h */


