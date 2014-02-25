/*
 * mfd.h
 */
#ifndef mfd_h
#define mfd_h
/**************************************************************
 * Constants 
 */
enum mfde_control { MFDC_TIME_STAMP,
                     MFDC_TEST_MODE,
                     MFDC_MONTAGE,
                     MFDC_BAT_FAIL,
                     MFDC_PAUSE
                     };
/* enum mfde_data {
	 MFDC_RT_FILTER_DATA, /* Supplies Filter Data *!/
	 MFDC_RT_RAW_DATA,    /* Supplies Raw Data *!/
	 MFDC_DATA_END
 }; */

 /* Prototypes */
#define _eMfdDataIn enum _MfdDataIn
 _eMfdDataIn {_FilteredData_,_RawData_,_BothData_};
 extern _eMfdDataIn DataToSsp; /* FilteredData or RawData */
	 
 enum inite_return mfd_initialisation(Initt_system_state *ssp);
 edlle_type mfd_supply_data(_eModuleId_ Id,void **pData);
 void MfdSetInput(_eMfdSetInput Id, void *p);
 void mfd_acquire(int number_elem);
 void mfd_control(enum mfde_control type,int value);

#endif /* ssp_h */



