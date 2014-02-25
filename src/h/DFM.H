/*
 * dfm.h    -   ## Biomonitors Mind Mirror III ##
 *
 * Description
 *
 * Module header for dfm - 
 *     most are still in proto.h
 *
 */
#ifndef dfm_h
#define dfm_h
/*
 * Constants
 */
enum dfme_control {
     DFMC_FILTERING_OFF, /* Default - enables filtering        */
     DFMC_FILTERING_ON,  /* Disables DFM actively filtering    */
     DFMC_CONTROL_NOP
};

/*
 * Module Prototypes
 */
 enum inite_return dfm_control(enum dfme_control type);


#endif /* dfm_h */











