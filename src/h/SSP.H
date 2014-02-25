/*
 * ssp.h
 */
#ifndef ssp_h
#define ssp_h
/**************************************************************
 * Constants 
 */
 enum sspe_tx_buffer_available {TX_BUF_NOT_AVAILABLE,TX_BUF_AVAILABLE};
 enum sspe_action {SSP_TX_RT,SSP_TX_RT_EN,SSP_TX_RT_DIS,
                   SSP_TX_HISTORY,SSP_FILTER};
 enum sspe_debug_control {SSP_DEBUG_DISABLED=0,SSP_DEBUG_ENABLED};

 enum SSPE_DL_REQ {
    SSPC_TX_DPLT_DATA, /* Tx a Dplt_data type buffer       */
    SSPC_TX_OTHER      /* */
 };
 typedef struct {
    enum sspe_action transmit_rt_en; /* Status of rt tx i/f */
    enum sspe_tx_buffer_available tx_buffer_available; /* Status of tx buf */
 } Sspt_manage;

 /* Exported prototypes */
 enum inite_return ssp_initialisation(Initt_system_state *ssp);
 void ssp_header_tx(void);
 void ssp_action(enum sspe_action act);
 void ssp_dl_req(enum SSPE_DL_REQ dl_req, char *tx_buf);
 void ssp_filter_tx(void);
 void ssp_control(Sspt_manage *mngp);

 /* Internal prototypes */
 void ssp_sync_tx(void);
 void ssp_info_tx(void);
 void ssp_send_buf(unsigned char *buf,unsigned int number);
 void ssp_end_int(void);
 unsigned char *ssp_translate(unsigned char *buf,unsigned char *size);

/*
 * For test and debug
 */
 void ssp_send_buf_test(void);
 /*Dplt_data *ssp_mfd_supply_data(enum mfde_type type);*/
 
#endif /* ssp_h */
