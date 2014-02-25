/*
 * init.h
 *
 * Include module for init subsystem
 */
    /*
     * Start another timer immediately 
     * - interrupts must not happen between hso_command and hso_time
     */
#define START_SW_TIMER_0 0x18
    /*
     * Values to be set in timer1 for software interrupts. This
     * is expected to be at the rate of 256HZ or every 3.90625mS
     *
     * Timer 1 increments every 8 state times,
     *   a state time is defined as XTAL freq. divided by 2 (pg 5-73 KC chap)
     *   Hence for a 
     *       16MHz xtal timer 1 increments every 1 uS
     * In practise there, there is a theoritical latency to get to the 
     * interrupt routine (defined pg 5-33) of between 18 and 61 state times.
     * In practise the MUL instruction will be used a lot and the range is
     * likely to be 18 and 40 state times.
     *      Initially assume interrupt latency is 20 state times.
     *      Time taken to reach instruction 4 * 5 = 20 state times
     * Hence the formulea for calculating an 
     *  interrupt at 128Hz or 7.8125mS is
     *
     *      time - 50/8
     *      7812.5uS - 5 = 7807
     * Measurements suggest trying 7807 * 4370/4378 = 7793
     * More measurement suggest trying 7793 *4420/4422 = 7790
     * 
     * For interrupts at 256Hz or 3.90625mS
     *       3895   measured 3.6mS, try *3.9/3.6 = 4219
     *       4219   measured 4.3mS, try 4010
     *       4010   measured 4.03mS,try 3900
     *       3900   measured 3.92ms, try 3888 
     *       3888   measured 3.910mS, try 3882
     *       3882   measured 3.906 as accurately as possible
     * Measurements performed with a 
     *     RCS Electronics 250Mhz Counter Timer Type 801B
     *       S/N 801/58322
     * For other crystals, scale the result
     */
#define LIST_4_INTERVAL 3882  /* Decimal */
#define LIST_SW_INTERVAL LIST_4_INTERVAL
#define LIST_SW_TIMER_STARTUP 0xf000 /* A long time */

#if defined(CLOCKC20)
/*                     MM       Hand Watch
/* 3882*5/4 = 4852.5   42min00  41min56  */
/*         Try 4853    45min00  44min55  */
/*             4862    42min00  42min00  */ 
#define LIST_SW_TIMER 4862
/*#define LIST_SW_TIMER ((LIST_SW_INTERVAL*5)/4) */

#elif defined(CLOCKC16)
#define LIST_SW_TIMER LIST_SW_INTERVAL

#elif defined(CLOCKC12)
#define LIST_SW_TIMER ((LIST_SW_INTERVAL*3)/4)
#elif defined(CLOCKC6_4)
#define LIST_SW_TIMER ((LIST_SW_INTERVAL*2)/5)
#else
  problem;
#endif

/*
 * Interrupt enable/disable for the following (pg 5-31)
 * Bit    Name
 *  7    0 NMI - Can't be disabled, set 0
 *  6    0 FIFO Full
 *  5    0 Ext Int1
 *  4    0 T2 Ovf
 *  3    0 T2 Cap
 *  2    0 HSI4
 *  1    1 RI
 *  0    1 TI
 */
/*#define INIT_INT_MASK_1 03 */
#define INIT_INT_MASK_1 0 /* No ints enabled */

/*
 * Interrupt enable/disable for IMASK (pg 5-31)
 * Bit      Name    
 *  7    0 Ext Int
 *  6    1 Ser Port
 *  5    1 Soft Timer
 *  4    0 HSI.0 Pin
 *  3    0 HSO Pin
 *  2    0 HSI Data
 *  1    1 A/D Done
 *  0    0 Timer Ovf
 */
#define INIT_INT_MASK 0x22 
#define AD_ACT_INT_MASK INIT_INT_MASK /* A/D interrupt mask */

/*
 * Interrupt enable/disable for the following (pg 5-31) 
 * Bit    Name
 *  7    0 NMI - doesn't work
 *  6    0 FIFO Full
 *  5    0 EXT INT1
 *  4    0 T2 OVF
 *  3    0 T2 Capture
 *  2    0 HSI4
 *  1    1 Receive Interrupt
 *  0    1 Transmit Interrupt
 */
/*#define INIT_INT1_MASK 0x3*/

 typedef struct {
 /* This structure records interrupt activity */
 int tmr_ovflw;
 int ad_complete;
 int hsi_avl;
 int hso;
 int hsi_o_pin;
 int sw_tmr;
 int ser_port;
 int extint;
 int trap;
 int undef_opcode;
 int transmit;
 int receive;
 int hsi_fifo_4;
 int tmr2_cap;
 int tmr2_ovflw;
 int extint_pin;
 int hsi_fifo_full;
 int nmi;
 } Intt_activity;

/**************************************************************
 * Internal enums/typedefs to this subsystem
 */
 enum mem_type {INTERNAL_RAM,EXT_NON_BANKED_RAM,EXT_BANKED_RAM,EXT_EPROM};
 enum init_debug_type {TRAP_OCCURED,NMI_OCCURED,UNIMPLEMENTED_OCCURED};

/**************************************************************
 * Internal Prototypes to this subsystem
 */
 enum inite_return test_memory(enum mem_type);
 enum inite_return ram_test(char *lower_addr,char *upper_addr);
 enum inite_return ram_bit_test(char *lower_addr);
 void sch_initialise(void);
 void init_debug(enum init_debug_type);
 void init_tx_str(char *);
 void init_tx_bin_char(INT16 new_value);
 void init_tx_hex_word(INT16 new_value);
 void init_tx_hex_char(INT16   value);
 void init_tx_hex_nibble(INT16 value);
 void hold_it(void);






