/*
 * filters.h
 *
 * Holds a description of the filters that are implemented
 *
 * The following defines control how this module builds
 *   filter_name_EQUATES  - allows the file to be used
 *   FILTER_COEFF - the coeff constants are built and
 *                  ROM space allocated
 *   FILTER_BIQUAD - ram space allocated
 *   FILTER_DESCRIPTOR - descriptor constants are built
 *                   and ROM space allocated.
 */
/* If defined, perform digital LOW PASS filtering
   If not defined, pass data through without low pass filter
*/
#define DO_LP_PROCESSING
#ifdef SIMULATING
/*#define DO_FP_PROCESSING*/
#endif

#define FILTER_COEFF
#define FILTER_DESCRIPTOR

/*******************************************************
 * Low Pass Filters
 */
#define F640_LPI_EQUATES  /* 256 Hz */
#include "640_LPI.hf"

#define F320_LPI_EQUATES   /* 128 Hz */
#include "320_LPI.hf"

#define F160_LPI_EQUATES  /* 64 Hz */
#include "160_LPI.hf"

/*******************************************************
 *  128Hz Sampling Filters
 */
#define F340_00T1_EQUATES /*128Hz */
#include "340_00T1.hf"

#define F270_70B1_EQUATES /*128Hz */
#include "270_70B1.hf"

#define F215_55B1_EQUATES /* 128Hz */
#include "215_55B1.hf"

/*******************************************************
 *  64Hz Sampling Filters
 */

#define F170_45B1_EQUATES   /* 64Hz */
#include "170_45B1.hf"

#define F135_35B1_EQUATES   /* 64Hz */
#include "135_35B1.hf"

#define F115_20B1_EQUATES   /* 64Hz */
#include "115_20B1.hf"

#define F095_20B1_EQUATES   /* 64Hz */
#include "095_20B1.hf"

#define F079_16B1_EQUATES  /* 64Hz */
#include "079_16B1.hf"

/*******************************************************
 *  32Hz Sampling Filters
 */
#define FILTER_BIQUAD

#define F068_11S_EQUATES   /* 32Hz */
#include "068_11S.hf"

#define F053_16B_EQUATES   /* 32Hz */
#include "053_16B.hf"

#define F037_16B_EQUATES   /* 32Hz */
#include "037_16B.hf"

#define F023_14B_EQUATES   /* 32Hz */
#include "023_14B.hf"

#define F011_12B_EQUATES  /* 32Hz */
#include "011_12B.hf"

#define F005__5S1_EQUATES /* 32Hz */
#include "005__5S1.hf"
/*
#ifdef DO_FP_PROCESSING
#define F005__5SF_EQUATES
#include "005__5SF.hf"
#endif
*/

/*******************************************************
 * Define table of filter descriptors
 *
 * General form
 *
 */
#define NUMBER_TIME_SLOTS 4

/*******************************************************
 * Low pass filters
 *
 * This table has ONLY two entries - that of the LP filters
 *
 */
#ifdef DO_LP_PROCESSING
 const Tbl_filter_descriptors tbl_lp_filter_descriptors[] = {
    &F640_LPI_filter_descriptor, /* 256 samples/sec */
    &F320_LPI_filter_descriptor, /* 128 samples/sec first */
    &F160_LPI_filter_descriptor, /* 64 samples/sec second */
    (void *) NULL 
 };
#define LP_256MS_ENTRY 0
#define LP_128MS_ENTRY 1
#define LP_64MS_ENTRY  2
#endif
/*******************************************************
 * Filter descriptors type i
 *
 * Filters to be invoked every 128ms
 */
 const Tbl_filter_descriptors tbl_128_filter_descriptors[] = {
    &F340_00T1_filter_descriptor,
    &F270_70B1_filter_descriptor,
    &F215_55B1_filter_descriptor,
 (void *) NULL /* Use to mark end */
 };
/*#ifdef LOG_SCRN_15BY1 */

#define NUMBER_128MS_FILTERS 3

/*#endif /* LOG_SCRN_15BY1 */
#ifdef LOG_SCRN_15BY2
/*#define NUMBER_128MS_FILTERS 0 */
#endif /* LOG_SCRN_15BY2 */

#define FIRST_128MS_ELEMENT 0

/*
 * Filters to be invoked every 64mS
 */
 const Tbl_filter_descriptors tbl_64_filter_descriptors[] = {
    &F170_45B1_filter_descriptor,
    &F135_35B1_filter_descriptor,
    &F115_20B1_filter_descriptor,
    &F095_20B1_filter_descriptor,
    &F079_16B1_filter_descriptor,
 (void *) NULL /* Use to mark end */
 };
#define NUMBER_64_1ST_PASS 2 /*2*/
#define NUMBER_64_2ND_PASS 3 /*3*/
#define NUMBER_64MS_FILTERS (NUMBER_64_1ST_PASS + NUMBER_64_2ND_PASS)
#define FIRST_64MS_ELEMENT NUMBER_128MS_FILTERS

/*
 * Filters to be invoked every 32mS
 */
 const Tbl_filter_descriptors tbl_32_filter_descriptors[] = {
    &F068_11S_filter_descriptor,
    &F053_16B_filter_descriptor,
    &F037_16B_filter_descriptor,
    &F023_14B_filter_descriptor,
    &F011_12B_filter_descriptor,
    &F005__5S1_filter_descriptor,
    (void *) NULL /* Use to mark end */
 };
#define FIRST_32MS_ELEMENT (NUMBER_64MS_FILTERS+NUMBER_128MS_FILTERS)
#define NUMBER_32_1ST_PASS 2/*2*/
#define NUMBER_32_2ND_PASS 1/*1*/
#define NUMBER_32_3RD_PASS 2/*2*/ /* if required - else set to 0 */
#define NUMBER_32_4TH_PASS 1/*1*/ /* if required - else set to 0 */
#define NUMBER_32MS_ELEMENTS (NUMBER_32_1ST_PASS+NUMBER_32_2ND_PASS+NUMBER_32_3RD_PASS+NUMBER_32_4TH_PASS)

#define TOT_NUM_IMPL_BP_FILTERS (FIRST_32MS_ELEMENT+NUMBER_32MS_ELEMENTS)
#define TOT_NUM_IMPL_FILTERS (TOT_NUM_IMPL_BP_FILTERS+NUMBER_LP_FILTERS)
 const int tot_num_impl_bp_filters=TOT_NUM_IMPL_BP_FILTERS;

/*
 * Table to point to the three previous tables
 */
 const Time_table time_table[] = {
   (Tbl_filter_descriptors *) &tbl_128_filter_descriptors,
   (Tbl_filter_descriptors *) &tbl_64_filter_descriptors,
   (Tbl_filter_descriptors *) &tbl_32_filter_descriptors,
    /* Any additions to this table require an update
     * to dim modules
     */
   (void *) NULL
 };

/**********************************************************
 * Filter descriptors for type fp
 */
#ifdef SIMULATING
Tbl_filter_descriptors_fp tbl_all_filter_descriptors_fp[] = {
#ifdef DO_FP_PROCESSING
 &F005__5SF_filter_descriptor,
#endif
 (void *) NULL
 };
#endif









