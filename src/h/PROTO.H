/*
 * proto.h
 * 
 * Function prototypes for Mind Mirror III system
 */
  void dbg_print(int err_num,char *mod_name, char *str);


 /* void init_initialisation(void); */
 Frac init_cnvt_frac(int number);
 void initialisation(void);
 enum inite_return ldd_initialisation(Initt_system_state *ssp);

 enum inite_return dim_initialisation(Initt_system_state *ssp);
 void dim_new_data(void);
/* void DimSnapRaw(eDimdType Type);*/
 void dim_dummy_in(Dimt_data *digp);

 enum inite_return dfm_initialisation(Initt_system_state *ssp);
 void dfm_digitized_values(Dimt_data *digp);
 void dfm_schedule(void);
 void dfm_peak_filter_values(int number_elem,Mfdt_data *p);
 Iir_filter_descriptor *dfm_filter_description(Word elem);
 int dfm_adjust_elem(int elem);
 Frac fmul(Frac a,Frac b);

 enum inite_return dbg_if_init(Initt_system_state *ssp);
 void dbg_putstr(char *);
 void dbg_wr(char []);
 void dbg_output(const char *str);
 void dbg_tx(void);

 void hw_init(Initt_system_state *ssp);
 void hw_test_freq(int type);

 enum inite_return uif_initialisation(Initt_system_state *ssp);



































