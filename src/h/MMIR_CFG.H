/*
 * mmir_cfg.h
 *
 * Configuration module for all the types of mmir OMFs
 *
 */

/* Definetype of target  one or the other - not both */
#ifdef IC96
/*#define REAL_TARGET_REV_001*/
#define REAL_TARGET
/*#define EVAL_TARGET*/
#endif /* IC96 */
#ifndef IC96
#define SIMULATING
#define TC
#endif /* IC96 */

#define TLX_1391
/*#define TLX_1013*/
/* When defined causes the output of the iir biquads to be captured */
/*#define OUTPUT_CAPTURE */

/* When defined cause the iir filters to run in background 
   When not defined the filters run off the 128mS interrupt
*/
/* #define FILTER_IN_BACKGROUND */

/* When defined cause the input to the filters to be simulated
   When not defined, the input is taken from an A/D port
*/
/*#define SAMPLE_INPUT */

/*
 * Defines to control the way ldd module builds
 *
 * Only one of the PHY_SCRN_... defines must be defined
 */
#define PHY_SCRN_128BY128
/*#define PHY_SCRN_640BY200 */
/*
 * Only one of the LOG_SCRN_... defines must be defined
 */
/*#define LOG_SCRN_15BY1*/
#define LOG_SCRN_15BY2 

/*
 * If defined the following allows debug statements to be printed.
 * The use of sprintf etc results in interrupts being disabled
 * and the sw_timer sometimes gets delayed. This has a knock on
 * effect with the filters as sampling is no longer accurate
 */
/* 
#ifdef IC96
#define IC96_DEBUG_ENABLED 
#endif 
*/










