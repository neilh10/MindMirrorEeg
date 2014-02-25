/*
 dbg.h
 */
/**************************************************************
 * Internal enums/typedefs 
 */
 enum enum_tx_status {TX_STATUS_TRANSMITTING=1,TX_STATUS_OFF};

 /* Note: These are value sensitive - see *state_table[][] in dbg_tim.c */
 enum dbge_tim {DBGE_START,DBGE_STOP,DBGE_PAUSE_ON,DBGE_PAUSE_OFF};

/*
 * Buffer control constants
 */
#define DBGC_BUFFER_SIZE 81

/* Partition 
 * Define number of partitions to be managed and their associated ID
 * Note: KNC_NUM_PARTITIONS defines the number of allowed partitions,
 *  and must be equal to or greater than the number of partition IDs
 *  Hence, KNC_NUM_PARTITIONS must always be the last entry in the
 *  enum list.
 */

 enum enum_partition_id { 
      /* Put partition IDs first */
      DBGC_TX_BLK, 
      /* No more partition IDs after this */
      KNC_NUM_PARTITIONS};
 typedef enum enum_partition_id Prt_id;

/*
 * Queue
 * KNC_NUM_QUEUES must equal or be greater than the number of
 * IDs that follow.
 */
 enum enum_queue_id {/* Put queue IDs first */
      DBGC_TX_QUEUE,
      DBGC_SEC_QUEUE,
      DBGC_INPUT_QUEUE,
      /* No more partition IDs after this */
      KNC_NUM_QUEUES};
 typedef enum enum_queue_id Que_id;

/*
 * Typedefs for output of dbg_tim
 */
 typedef struct {enum dbge_tim state;
                 Word number_pauses;
                 unsigned long time;
                 Word out_of_range; /* debug */
                 Word ignored;
   } Dbgt_tim_result;
/*
 * Prototypes for debug module
 */
 enum inite_return dbg_tim_init(Initt_system_state *ssp);
 void dbg_tim(enum dbge_tim requested_state);
 int dbg_result(Dbgt_tim_result *);








