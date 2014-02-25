/*
 kernel.h
 */

 enum enum_kn_type {KN_OK,KN_ERR,KN_OUT_OF_RANGE,KN_NOT_AVAILABLE};
 typedef enum enum_kn_type Knt_type;

 enum enum_kn_fn /* Of all functions to KN_ subsystem */
 {KNC_BLK_INIT,KNC_QUE_INIT,KNC_PCREATE,KNC_QCREATE,
  KNC_QPOST,KNC_QACCEPT,
  KNC_RBLOCK,
  /* Everything at/above KNC_GBLOCK is not reported by kn_error() */
  KNC_GBLOCK,KNC_FIND_BLOCK
  };
 typedef enum enum_kn_fn Knt_fnid;

 Knt_type kn_error(Knt_type err,enum enum_kn_fn);

 Knt_type kn_blk_init(Prt_id partitions);
 Knt_type kn_pcreate(Prt_id partition_id, int size, int num, Byte *startp);
 Knt_type kn_rblock(Prt_id partition_id, Byte *p);
 Byte   *kn_gblock(Prt_id partition_id, Knt_type *err);

 Knt_type kn_que_init(Que_id number);
 Knt_type kn_qcreate(Que_id que_id, int size);
 Knt_type kn_qpost(Que_id que_id,void *p);
 void *kn_qaccept(Que_id que_id, Knt_type *err);




