/* Kn_blk.c
 * 
 * http://www.biomonitors.com/
 * Copyright (c) 1992-2014 Neil Hancock
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * 
 This module provides some basic functions for allocating and 
 deallocating from fixed size areas. They are

 kn_pcreate - create a partition or block
 kn_rblock  - return a block to a specified partition
 kn_gblock  - get a block from a specified partition

 kn_qcreate - create a queue
 kn_qpost   - post to a specified queue
 kn_qaccept - accept a pointer from a queue

 Conventions
   parition - the reserved space that a client supplies
   block    - the data that a partition is divided into
*/

#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
#include <80C196.h>
#include <stdio.h>
/*#include <stdlib.h>*/

#include "general.h"
#include "iir.h"
#include "proto.h"
#include "dbg.h"
#include "kernel.h"
#ifdef SIMULATING
#include "pc_only.h"
#endif
/**************************************************************
 * Internal enums/typedefs 
 */

/* Constants */
/**************************************************************
 * Shared variables 
 */

/**************************************************************
 *
 * Internal storage to this module
 *
 * kn_storage[] is the general managment storage area.
 *
 */
#define KN_STORAGE_SIZE 128
 Byte kn_storage[KN_STORAGE_SIZE];
 Byte *nxt_allocatep, *top_storagep;

/*
 *  The kn_partition[] array holds the pointers to the individual
 *  parition managers. nxt_partitionp points
 */
#define MAX_NUMBER_PARTITIONS 16
#define MAX_NUMBER_BLOCKS 32
 /* void *kn_partitionp[KN_NUMBER_PARTITIONS]; */
 int num_partitions;
 Byte *nxt_partitionp, *top_partitionp, *partitionp;

/* Structure defining the layout of the data used to manage 
 * a partition
 * Note: status is 32 bits, that are set if a partition is free
 */
  typedef struct {
  unsigned long status; /* Defines the status of the blocks       */
  int num;             /* Number of client blocks in partition   */
  int size;            /* Size of each client block in partition */
  Byte *startp;        /* Start of client partition              */
  } Knt_block;

/* Tracks the errors generated in this module */
 int kni_error;

/**************************************************************
 * Internal Prototypes
 */

/*;*<*>********************************************************
  * kn_blk_init
  *
  * This fn() initialises this module
  **start*/
 Knt_type kn_blk_init(Prt_id partitions)
 {
   nxt_allocatep = &kn_storage[0];
   top_storagep  = &kn_storage[KN_STORAGE_SIZE-1];

   /* Reserve storage for partition management */
   num_partitions = (int) partitions; /* Input */
   if (!(num_partitions < MAX_NUMBER_PARTITIONS))
      return kn_error(KN_OUT_OF_RANGE,KNC_BLK_INIT);

   partitionp = nxt_allocatep;
   nxt_allocatep = partitionp + num_partitions*sizeof(Knt_block);

   kni_error = 0;

   return KN_OK;
 } /*end kn_blk_init*/


/*;*<*>********************************************************
  * kn_pcreate
  *
  * This function manages a partition supplied by the client.
  * The client allocates the storage, but the management is
  * done in this modules workspace
  *
  * partition_id defines a partition in the range 0 < num_partitions
  * size - number of bytes in each block
  * num - number of blocks. A number between 0 < MAX_NUMBER_BLOCKS
  * *p - a pointer that points to the space reserved
  *
  * each partition looks as follows
  *  partitionp+partition_id->blockp
  *
  *  blockp-> Status_size status
  *           int         num
  *           int         size
  *           Byte        *clientp
  *
  **start*/
 Knt_type kn_pcreate(Prt_id partition_id,int size, int num, Byte *startp)
 {
 Knt_block *blockp;

    if ( (int) partition_id >= num_partitions)
       return kn_error(KN_OUT_OF_RANGE,KNC_PCREATE);

    if (num >= MAX_NUMBER_BLOCKS)
       return kn_error(KN_OUT_OF_RANGE,KNC_PCREATE);
#define INIT_BLOCK_STATUS 0xffffffffl

    blockp = (Knt_block *) (partitionp + partition_id*sizeof(Knt_block));
    blockp->status = INIT_BLOCK_STATUS;
    blockp->num    = num;
    blockp->size   = size;
    blockp->startp = startp;

    return KN_OK;
 } /*end kn_pcreate*/

/*;*<*>********************************************************
  * kn_rblock
  *
  * This returns a block of data to the pool of data specified by
  * block_id. The *p must be in the range of one of the blocks of 
  * data.
  *
  * This routine is reentrant. This is done by making sure the
  * transaction on the status bits is uninterruptable.
  *
  **start*/
 Knt_type kn_rblock(Prt_id partition_id, Byte *clientp)
 {
 Knt_block *blockp;
 int lp, answ=FALSE;
 Knt_type err;
 register int num;
 register Byte *p = (Byte *) clientp, *startp;
 register long mask;

 blockp = (Knt_block *) (partitionp + partition_id*sizeof(Knt_block));
 num = blockp->num;
 startp = (blockp->startp);
 for (lp = 0 ;lp < num; ++lp) {
   p -= blockp->size;
   if ( startp > p) {
      answ = TRUE;
      break;
   }
 }
 if (answ) {
    /* clientp fitted into one of the buffer ranges */
    mask = 1 << lp;
    err = KN_OK;
    disable(); /* Make reentrant */
    if (blockp->status & mask) /* Alreay set */
       err = kn_error(KN_ERR,KNC_RBLOCK);

    blockp->status |= mask; /* Set appropiate bit */
    enable();
    return err;
 } else {
    /* clientp did not fit into one of the buffer ranges */
    return kn_error(KN_OUT_OF_RANGE,KNC_RBLOCK);
 }
 } /*end kn_rblock*/

/*;*<*>********************************************************
  * find_block
  *
  * This routine searches through the Knt_block.status
  * for a free buffer and then returns the start address of it.
  *
  * A one in status indicates a free 
  *
  * This fn() is reentrant. This is done by making sure the
  * transaction of searching for a free buffer and resetting
  * the free buffer bit is performed without interruption.
  *
  * Future: It may be possible to use a NORML 8096 instruction
  *         to speed up this instruction
  **start*/
 void *find_block(Knt_block *blockp,Knt_type *err)
 {
 int  answ=FALSE;
 register int lp;
 register int num_blocks=blockp->num;
 register long mask=1;
 register long status;

    /*
     * Find a set bit in status
     */
    disable(); /* Make reentrant */
    status=blockp->status;
    for (lp=0;lp < num_blocks; lp++) {
       if (mask & status) {
           answ = TRUE;
           break;
       }
       mask <<= 1;
    }
    if (answ) {
        /* Reset the appropiate bit ASAP*/
       blockp->status &= ~mask;
    }
    enable(); /* Allow interrupts */

    if (answ) {
       /* A free block was found */
       *err = KN_OK;
       return (blockp->startp + blockp->size*lp);
    } else {
       /* No free blocks found */
       *err = kn_error(KN_ERR,KNC_FIND_BLOCK);
       return NULL;
    }
 } /*end find_block*/

/*;*<*>********************************************************
  * kn_gblock
  **start*/
 Byte *kn_gblock(Prt_id partition_id, Knt_type *err)
 {
 Knt_block *blockp;

 blockp = (Knt_block *) (partitionp + partition_id*sizeof(Knt_block));

 return find_block(blockp,err);
 } /*end kn_gblock*/

/*;*<*>********************************************************
  * kn_error
  *
  * This routine tracks the number of errors generated
  *
  **start*/
 Knt_type kn_error(Knt_type err,Knt_fnid id)
 {
 char buf[60];

#ifdef IC96_DEBUG_ENABLED
    sprintf(buf,"Kn_error num %x: Kernel Error Knt_fnid = %x, Knt_type = %x\n",
            kni_error,id,err);

    if ((int)id < (int)KNC_GBLOCK) { /*cast to int for IC96 workaround */
       /*
        * Errors from some routines can cause a cascade of errors.
        * So only print routines that shouldn't cause problems
        */
        dbg_output(buf);
    }
#endif /* IC96_DEBUG_ENABLED */
#ifdef SIMULATING
    printf("\n%s",buf);
#endif

    ++kni_error;
    return err;
 } /*end kn_error*/

/*;*<*>********************************************************
  * 
  **start*/
/* ()
 {

 } /*end*/


