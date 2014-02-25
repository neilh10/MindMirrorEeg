/* Kn_que.c
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
 This module provides some basic functions for enqueueing and
 dequeing. They are


 Conventions
       +--------+
       |        | <-top    (largest addr)
       |        | <-in
       | XXXXXX |       /|\
       | XXXXXX |       pointers asscend
       | XXXXXX | <-out /|\
       |        | <-bottom (lowest addr)
       +--------+

 definitions
   bottom - always points to the lowest physically used addr
            When pointers wrap they are set to this addr.
   top    - always points to the highest physically used addr
            When a pointer has added/retrieved its data from this
            addr, it is set to the bottom
   out    - this points to the next value out, providing number!=0
            When a value is to be removed from the stack,then the 
            following is performed in a single transaction:- 
               if (number > 0)
                  retrieve data
                  number--
                  if (out < top)
                     then out++ 
                     else out = bottom

   in     - this points to the next available location. When a new
            value needs to be added then the following is performed
            in a single transaction
               if (number < size)
                  put data
                  number++
                  if (in < top)
                     then in++
                     else in = bottom

*/
#ifdef IC96
#pragma type
#endif
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
#include <80C196.h>
#include <stdio.h>
/*#include <stdlib.h>*/

#include "general.h"
#include "iir.h"
#include "dbg.h"
#include "proto.h"
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
 * kn_qstorage[] is the general managment storage area.
 *
 */
#define KN_QSTORAGE_SIZE 256
 Byte kn_qstorage[KN_QSTORAGE_SIZE];
 Byte *nxt_qallocatep, *top_qstoragep;

 int num_queues;   /* Total number queues allowed
                    * All queue_ids must be between 0 and <num_queues
                    */
 Byte *queue_mngrp;/* Start of the queue manager data area
                    * A queue_id is used to index into this
                    * area to find the right structure as:-
                    * queue_mngrp + queue_id*sizeof(Knt_queue)
                    */
/*
 * Structure defining the layout of the data used to manage
 * a queue.
 */

 typedef struct {
  int size;      /* Total entries allowed in queue      */
  int number;    /* Number of entities in queue         */
  Byte **bottom;/* First physical queue position       */
  Byte **top;   /* Last physical queue position        */
  Byte **out;   /* Where the next entry is pulled from */
  Byte **in;    /* Where the next entry is pushed to   */
 } Knt_queue;

/**************************************************************
 * Internal Prototypes
 */

/*;*<*>********************************************************
  * kn_init_queue
  *
  * This fn() initializes the data area for this module
  *
  **start*/
 Knt_type kn_que_init(Que_id number)
 {
    nxt_qallocatep = &kn_qstorage[0];
    top_qstoragep  = &kn_qstorage[KN_QSTORAGE_SIZE-1];

    /* Reserve storage for queue manager */
    num_queues = (int) number;
#define MAX_NUMBER_QUEUES 16
    if (!(num_queues < MAX_NUMBER_QUEUES))
       return kn_error(KN_OUT_OF_RANGE,KNC_QUE_INIT);

    queue_mngrp = nxt_qallocatep;
    nxt_qallocatep = queue_mngrp + num_queues*sizeof(Knt_queue);

    return KN_OK;

 } /*end kn_init_queue*/

/*;*<*>********************************************************
  * kn_qcreate
  *
  * This fn() initializes one of the queue entries.
  *
  * Returns something from Knt_type
  **start*/
 Knt_type kn_qcreate(Que_id queue_id,int size)
 {
  Knt_queue *qp;
  Byte *example[1];

    if ( (int)queue_id >= num_queues)
        return kn_error(KN_OUT_OF_RANGE,KNC_QCREATE);

    qp = (Knt_queue *) (queue_mngrp + queue_id*sizeof(Knt_queue));

    qp->bottom = qp->in = qp->out = (Byte **) nxt_qallocatep;

    qp->top = (Byte **) (nxt_qallocatep + size * sizeof(example[0]));
    nxt_qallocatep = (Byte *) qp->top;

    if (++nxt_qallocatep > top_qstoragep )
        return kn_error(KN_OUT_OF_RANGE,KNC_QCREATE);

    qp->size = size;
    qp->number = 0;

    return KN_OK;
 } /*end kn_qcreate*/

/*;*<*>********************************************************
  * kn_qpost
  *
  * This fn() posts the pointer p to queue que_id.
  *
  * The overview of this process is described at the begining of
  * this module under the heading "Conventions"
  *
  **start*/
 Knt_type kn_qpost(Que_id que_id,void *p)
 {
 Knt_queue *qp;
 Knt_type err=KN_OK;

    if ( (int)que_id >= (int)num_queues)
        return kn_error(KN_OUT_OF_RANGE,KNC_QPOST);
    qp = (Knt_queue *) (queue_mngrp + que_id*sizeof(Knt_queue));

    disable(); /* Lock to prevent access */
    do { /* Use do..while to allow break; in the middle */
       if (qp->number >= qp->size) {
          err  = KN_OUT_OF_RANGE;
          break;
       }
       *(qp->in) = p;
       qp->number++;
       if (qp->in < qp->top){
          qp->in++;
       } else {
          qp->in = qp->bottom;
       }
    } while (0);
    enable();
    return err;

 } /*end kn_qpost*/

/*;*<*>********************************************************
  * kn_qaccept
  *
  * This fn() returns the pointer p from the top of queue que_id,
  * or if nothing is there returns a NULL pointer.
  *
  * The overview of this process is described at the begining of
  * this module under the heading "Conventions"
  *
  **start*/
 void *kn_qaccept(Que_id que_id, Knt_type *err)
 {
 register void *p;
 register Knt_queue *qp;

    if ( (int)que_id >= (int)num_queues) {
       *err = kn_error(KN_OUT_OF_RANGE,KNC_QACCEPT);
       return NULL;
    }
    qp = (Knt_queue *) (queue_mngrp + que_id*sizeof(Knt_queue));
    *err=KN_OK;
    disable(); /* Lock to prevent access */
    do { /* Use do..while to all break; in the middle */
       if (qp->number <= 0) {
          *err = KN_NOT_AVAILABLE;
          p = NULL;
          break;
       }
       p = *(qp->out);
       qp->number--;
       if (qp->out < qp->top){
          qp->out++;
       } else {
          qp->out = qp->bottom;
       }
    } while (0);
    enable();

    return p;
 } /*end kn_qaccept*/

/*;*<*>********************************************************
  * 
  **start*/
/* ()
 {

 } /*end*/

