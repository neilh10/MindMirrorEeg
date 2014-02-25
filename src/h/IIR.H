/*

 iir.h

 Header file to define IIR filter definitions

 */

 typedef struct {
 /*
  * This structure defines the way coefficients are stored
  * for fractional cascade type I and II filters.
  */
  COEFF_TYPE_F sect_shift_cnt;
  COEFF_TYPE_F b0; 
  COEFF_TYPE_F b1; COEFF_TYPE_F a1;
  COEFF_TYPE_F b2; COEFF_TYPE_F a2;
 } Iir_coeff;

 typedef struct {
 /*
  * This structure defines the way coefficients are stored
  * for floating point cascaded.
  */
  double b1; double b2;
  double a1; double a2;
 } Iir_coeff_fp;

 typedef struct {
 /*
   This defines the way a biquad section is laid out
  */
  SAMPLE_TYPE_F z2_f; /* This has been delayed two unit intervals */
  SAMPLE_TYPE_F z1_f; /* This has been delayed one unit intervals */

 } Biquad;
#define BIQUAD_INIT 0

 typedef struct {
 /*
   This defines the way a biquad section is laid out
  */
  double z2_d; /* This has been delayed two unit intervals */
  double z1_d; /* This has been delayed one unit intervals */

 } Biquad_fp;

/*
 * Macro to define common information for a filter
 *
 * Change it slowly, its very touchy.
 *      Doesn't like INT16
 */
#define iir_mac(expr1,expr2,expr3,expr4) \
 typedef struct {                   \
 int number_of_sections;            \
 int overall_shift_count;           \
 expr1 overall_gain;                \
 expr2  *coeff;                     \
 expr3  *biquad[MAX_NUMBER_LOBES];  \
/* expr1 input;                       \
 expr1 output;                      \
 expr1 output_peak;                 \
 expr1 output_bleed;                \
*/ char design;                       \
 char filter_type;                  \
 char analog_filter_type;           \
 float passband_ripple;             \
 float stopband_ripple;             \
 float passband1_cutoff_frequency; \
 float passband2_cutoff_frequency; \
 float stopband1_cutoff_frequency; \
 float stopband2_cutoff_frequency; \
 char filter_design_method;         \
 int sampling_frequency;            \
 char filter_order;                 \
 char no_quantized_bits;            \
 char quantization_type;            \
 char coefficients_scaled_for;      \
 } expr4

/*
 * Filter Descriptor Structure -  holds all the information for a
 * fractional fixed point filter
 */
 iir_mac(SAMPLE_TYPE_F, Iir_coeff, Biquad, Iir_filter_descriptor);

/*
 * Filter Descriptor Structure - holds all the information for a
 * block floating point filter
 */
 iir_mac(double, Iir_coeff_fp, Biquad_fp, Iir_filter_descriptor_fp) ;

 /*
  * Structure for table of filter descriptors
  */

 typedef struct {
    Iir_filter_descriptor *p;
 } Tbl_filter_descriptors;

 typedef struct {
   Tbl_filter_descriptors *t;
 } Time_table;

 typedef struct {
    Iir_filter_descriptor_fp *p;
 } Tbl_filter_descriptors_fp;

/*************************************************************
 *                 Constants
 *
 * filter_data() may be used to print all the following information
 * and should be updated if more is added.
 *
 * Filters
 */
 enum  {IIR_DESIGN,FIR_DESIGN};

/*
 * Filter_types
 */
 enum { LOW_PASS, /* HIGH_PASS, */
      BAND_PASS, BAND_STOP };

/*
 * Analog filter name
 */
 enum {BUTTERWORTH, TSCHEBYSCHEFF, INVERSE_TSCHEBYSCHEFF, ELLIPTIC, BESSEL };

/*
 * Filter design method
 */
 enum {BILINEAR_TRANSFORMATION};

/*
 * Quantization type
 */
 enum {FRACTIONAL_FIXED_POINT, FLOATING_POINT};

/*
 * Filter realization type
 */
 enum {CASCADE_FORM_I, CASCADE_FORM_II, FLOATING_POINT_IMPLEMENTATION};

/*
 * Prototype functions
 */
 double float_iir_fp(Iir_filter_descriptor_fp *ip,double input);
 Frac iir_i(Iir_filter_descriptor *ip, Frac input, int lobe);
 /*Frac iir_ii(Iir_filter_descriptor *ip); */
 int initialise_iir_i(Iir_filter_descriptor *ip, int number_lobes);
 /* int initialise_iir_ii(Iir_filter_descriptor *ip); */
 int initialise_iir_fp(Iir_filter_descriptor_fp *ip);
 Frac dfm_iir_i(Iir_filter_descriptor *iirlpp,Frac input,int lobe_lp);




