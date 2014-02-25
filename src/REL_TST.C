/* rel_test.c
 * This file is named after the release directory name
 *
 * Its main function is to appear at the begining of the
 * release build and thus define the build name
 */
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
#include <80C196.h>
#include "general.h"
#include "iir.h"
#include "mm_coms.h"
#include "dim.h"
#include "proto.h"
 main()
 {
 int on=FALSE,loop=0;

    initialisation();

    /*
     * Flash LEDS if ever reach here 
     */
    do {
       if (on) {
          ioport1 = 0xff; /* turn leds on */
          on = FALSE;
       } else {
          ioport1 = 0;   /* turn leds off */
          on = TRUE;
       }
       do {} while (++loop); /* delay */

    } while (1); /* Loop forever if it ever returns */
 }
