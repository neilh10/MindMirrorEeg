/*
 * drm.h    -   ## Biomonitors Mind Mirror III ##
 *
 * Description
 *
 *
 *
 */
#ifndef drm_h
#define drm_h
/*
 * Constants
 */
/* Maximum size of data expected to be released by
 *   drm_data(,, *addr_of_buffer)
 */
#define DRMC_MAX_BUFFER_SIZE sizeof(Dplt_data)

 enum drme_control {
 /* Control parameters when calling drm_data() */
     DRMC_RESET,             /* clears the buffer                    */
     DRMC_1ST_TUPL,          /* jumps to the beginning of the buffer */
     /*DRMC_END,             /* jumps to the end of the buffer       */
     DRMC_CURRENT_TUPL,      /* Gets the addr of the current record  */
     DRMC_FETCH_FRWD_TUPL,   /* gets the next +time record           */
     DRMC_FETCH_FRWD_TUPL_30_SEC,/* As above, but forward 30 seconds */
     DRMC_FETCH_BACK_TUPL,   /* gets the next -time record           */
     DRMC_FETCH_BACK_TUPL_30_SEC,/* As above but backwards           */
     DRMC_PAUSE_ON,
     DRMC_PAUSE_OFF,

     DRMC_DPL_START,
     DRMC_DPL_FINISH,
     DRMC_DPL_FRWD_1,
     DRMC_DPL_FRWD_10_SEC,
     DRMC_DPL_FRWD_1_MIN,
     DRMC_DPL_BACK_1,
     DRMC_DPL_BACK_10_SEC,
     DRMC_DPL_BACK_1_MIN,

     DRMC_DATA_RATE          /* sets the value in requestor to be the
                              * data rate                            */
 };
 enum drme_requestor {
     DRMC_SSP,
     DRMC_DPL
 };
#define INIT_TUPL_NUMBER 2
#define TUPL_START_NUMBER 1
/*
 * Module Prototypes
 */
 enum inite_return drm_initialisation(Initt_system_state *ssp);
int drm_data(enum drme_control control,
             _eModuleId_ requestor,
             char *address_of_buffer);

/* Internal Prototypes */
 enum inite_return drm_ram_initialisation(Initt_system_state *ssp);
 int drm_ram_store(char *pdest,uint buf_size);
 int drm_ram_get(long int *addr,char *pdest, int number);
 int retrieve(long int *addr,char *pdest, uint number);
 int drm_ram_control(long int *p, enum drme_control action);

#endif /* drm_h */











