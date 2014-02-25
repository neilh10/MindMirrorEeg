/*  uif_demo.c
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
Algorithms

Internal Data

*/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
/*#include <stdio.h>*/
/*#include <math.h>*/
/*#include <stdlib.h>*/

#include "general.h"
#include "iir.h"
#include "hw.h"
#include "uif.h"
#include "mm_coms.h"
#include "dim.h"
#include "proto.h"
#include "mfd.h"
#include "mmstatus.h"
/*#include "dbg.h"*/

/**************************************************************
 * Constants 
 */
 typedef struct {
    char *titlep;
    Dplt_data pattern;
 } Uift_display_demo;

/* Change the define to reflect number of displays defined in
 *   *demo_display[]
 */
#define NUMBER_DEMO_DISPLAY 10


/**************************************************************
 * Externally defined 
 */

/**************************************************************
 * Internal storage to this module 
 */
 char *uif_demo_statusp;
 /* Patterns to be demonstrated
  * First fill in title and then expected length according to following
  *     0x3f is 25% FSD
  *     0x80 is 50% FSD
  *     0xbf is 75% FSD - calibration mark
  * Then add case statement to pattern()
  * finally increment NUMBER_DEMO_DISPLAY
  * (can't use a table as IC96 doesn't like it - see bug7.txt or below
  */

 const Uift_display_demo trial_pattern = {
    /*0123456789012345*/
    " Trial Pattern  ", /* 16 Characters long only*/
    /* Starts with EMG data and then 0.75Hz to 38Hz */
    {0x03, /* Left EMG */
     0x03,0x03,0x03, /* Delta */
     0x03,0x03,      /* Theta 4.5, 6 */
     0x03,0x03,0x03,0x03,0x03, /* Alpha */
     0x03,0x03,0x03,0x03,      /* Beta  */
     0, /* Not Used*/
     0x03, /* Right EMG */
     0x03,0x03,0x03, /* R Delta */
     0x03,0x03,      /* R Theta */
     0x03,0x03,0x03,0x03,0x03, /* R Alpha */
     0x03,0x03,0x03,0x03,      /* R Beta  */
     0,  /* Not Used */
     0,MMS_DEMO_ACTIVE /* Two hijacked bytes */
    }
 };

/*****NEW PATTERNS ***/
 const Uift_display_demo dreamless_sleep_pattern = {
    /* Designed by Peter Staples & Isobel Cade 13/Nov/93 */
    /*0123456789012345*/
    " Dreamless Sleep", /* 16 Characters long only*/
    /* Starts with EMG data and then 0.75Hz to 38Hz */
    {0x04, /* Left EMG */
     0xff,0xf1,0xaa, /* Delta */
     0x19,0x03,      /* Theta 4.5, 6 */
     0x03,0x03,0x03,0x03,0x03, /* Alpha */
     0x03,0x03,0x03,0x03,      /* Beta  */
     0, /* Not Used*/
     0x03, /* Right EMG */
     0xff,0xf1,0xaa, /* R Delta */
     0x19,0x03,      /* R Theta */
     0x03,0x03,0x03,0x03,0x03, /* R Alpha */
     0x03,0x03,0x03,0x03,      /* R Beta  */
     0,  /* Not Used */
     0,MMS_DEMO_ACTIVE /* Two hijacked bytes */
    }
 };
 const Uift_display_demo theta_dreaming_pattern = {
    /* Designed by Peter Staples & Isobel Cade 13/Nov/93 */
    /*0123456789012345*/
    "Theta - Dreaming", /* 16 Characters long only*/
    /* Starts with EMG data and then 0.75Hz to 38Hz */
    {0x03, /* Left EMG */
     0x03,0x03,0x19, /* Delta */
     0x68,0x7e,      /* Theta 4.5, 6 */
     0x4b,0x10,0x03,0x03,0x03, /* Alpha */
     0x03,0x03,0x03,0x03,      /* Beta  */
     0, /* Not Used*/
     0x03, /* Right EMG */
     0x03,0x03,0x19, /* R Delta */
     0x68,0x7e,      /* R Theta */
     0x4b,0x10,0x03,0x03,0x03, /* R Alpha */
     0x03,0x03,0x03,0x03,      /* R Beta  */
     0,  /* Not Used */
     0,MMS_DEMO_ACTIVE /* Two hijacked bytes */
    }
 };
 const Uift_display_demo alpha_hypnagogic_pattern = {
    /* Designed by Peter Staples & Isobel Cade 13/Nov/93 */
    /*0123456789012345*/
    "Alpha-Hypnagogic", /* 16 Characters long only*/
    /* Starts with EMG data and then 0.75Hz to 38Hz */
    {0x03, /* Left EMG */
     0x03,0x03,0x03, /* Delta */
     0x03,0x19,      /* Theta 4.5, 6 */
     0x81,0xcc,0xcc,0x8e,0x19, /* Alpha */
     0x03,0x03,0x03,0x03,      /* Beta  */
     0, /* Not Used*/
     0x03, /* Right EMG */
     0x03,0x03,0x03, /* R Delta */
     0x03,0x19,      /* R Theta */
     0x81,0xcc,0xcc,0x8e,0x19, /* R Alpha */
     0x03,0x03,0x03,0x03,      /* R Beta  */
     0,  /* Not Used */
     0,MMS_DEMO_ACTIVE /* Two hijacked bytes */
    }
 };
 const Uift_display_demo beta_pattern = {
    /* Designed by Peter Staples & Isobel Cade 13/Nov/93 */
    /*0123456789012345*/
    "      Beta      ", /* 16 Characters long only*/
    /* Starts with EMG data and then 0.75Hz to 38Hz */
    {0x03, /* Left EMG */
     0x03,0x03,0x03, /* Delta */
     0x03,0x03,      /* Theta 4.5, 6 */
     0x03,0x03,0x03,0x03,0x32, /* Alpha */
     0x99,0xe8,0xff,0xff,      /* Beta  */
     0, /* Not Used*/
     0x03, /* Right EMG */
     0x03,0x03,0x03, /* R Delta */
     0x03,0x03,      /* R Theta */
     0x03,0x03,0x03,0x03,0x32, /* R Alpha */
     0x99,0xe8,0xff,0xff,      /* R Beta  */
     0,  /* Not Used */
     0,MMS_DEMO_ACTIVE /* Two hijacked bytes */
    }
 };
 const Uift_display_demo alpha_blocking_pattern = {
    /* Designed by Peter Staples & Isobel Cade 13/Nov/93 */
    /*0123456789012345*/
    " Alpha Blocking ", /* 16 Characters long only*/
    /* Starts with EMG data and then 0.75Hz to 38Hz */
    {0x03, /* Left EMG */
     0xff,0x9a,0x68, /* Delta */
     0x32,0x1b,      /* Theta 4.5, 6 */
     0x03,0x0c,0x1b,0x50,0x88, /* Alpha */
     0xc9,0xe5,0xf1,0xff,      /* Beta  */
     0, /* Not Used*/
     0x03, /* Right EMG */
     0xff,0x99,0x68, /* R Delta */
     0x32,0x1b,      /* R Theta */
     0x03,0x0c,0x1b,0x50,0x88, /* R Alpha */
     0xc9,0xe5,0xf1,0xff,      /* R Beta  */
     0,  /* Not Used */
     0,MMS_DEMO_ACTIVE /* Two hijacked bytes */
    }
 };
 const Uift_display_demo state4_pattern = {
    /* Designed by Peter Staples & Isobel Cade 13/Nov/93 */
    /*0123456789012345*/
    "     State 4    ", /* 16 Characters long only*/
    /* Starts with EMG data and then 0.75Hz to 38Hz */
    {0x03, /* Left EMG */
     0x03,0x03,0x2e, /* Delta */
     0x68,0x60,      /* Theta 4.5, 6 */
     0x2e,0xb3,0xe5,0xd9,0x68, /* Alpha */
     0x0c,0x03,0x03,0x03,      /* Beta  */
     0, /* Not Used*/
     0x03, /* Right EMG */
     0x03,0x03,0x2e, /* R Delta */
     0x68,0x60,      /* R Theta */
     0x2e,0xb3,0xe5,0xd9,0x68, /* R Alpha */
     0x0c,0x03,0x03,0x03,      /* R Beta  */
     0,  /* Not Used */
     0,MMS_DEMO_ACTIVE /* Two hijacked bytes */
    }
 };
 const Uift_display_demo awakened_mind_pattern = {
    /* Designed by Peter Staples & Isobel Cade 13/Nov/93 */
    /*0123456789012345*/
    " Awakened  Mind ", /* 16 Characters long only*/
    /* Starts with EMG data and then 0.75Hz to 38Hz */
    {0x00, /* Left EMG */
     0x9a,0x4b,0x10, /* Delta */
     0x54,0x21,      /* Theta 4.5, 6 */
     0x81,0xd4,0xcc,0x94,0x2a, /* Alpha */
     0x68,0x85,0x79,0x2a,      /* Beta  */
     0, /* Not Used*/
     0x03, /* Right EMG */
     0x9a,0x4b,0x10, /* R Delta */
     0x54,0x21,      /* R Theta */
     0x81,0xd4,0xcc,0x94,0x2a, /* R Alpha */
     0x68,0x85,0x79,0x2a,      /* R Beta  */
     0,  /* Not Used */
     0,MMS_DEMO_ACTIVE /* Two hijacked bytes */
    }
 };
 const Uift_display_demo creativity_pattern = {
    /* Designed by Peter Staples & Isobel Cade 13/Nov/93 */
    /*0123456789012345*/
    "   Creativity   ", /* 16 Characters long only*/
    /* Starts with EMG data and then 0.75Hz to 38Hz */
    {0x03, /* Left EMG */
     0x1c,0x60,0x9a, /* Delta */
     0xb3,0xcc,      /* Theta 4.5, 6 */
     0xe9,0xff,0xff,0xf1,0xd8, /* Alpha */
     0xbf,0x9a,0x60,0x19,      /* Beta  */
     0, /* Not Used*/
     0x03, /* Right EMG */
     0x1c,0x60,0x9a, /* R Delta */
     0xb3,0xcc,      /* R Theta */
     0xe9,0xff,0xff,0xf1,0xd8, /* R Alpha */
     0xbf,0x9a,0x60,0x19,      /* R Beta  */
     0,  /* Not Used */
     0,MMS_DEMO_ACTIVE /* Two hijacked bytes */
    }
 };
 const Uift_display_demo lateral_asymmetry_pattern = {
    /* Designed by Peter Staples & Isobel Cade 13/Nov/93 */
    /*0123456789012345*/
    "Latral Asymmetry", /* 16 Characters long only*/
    /* Starts with EMG data and then 0.75Hz to 38Hz */
    {0x03, /* Left EMG */
     0xa6,0x45,0x08, /* L Delta */
     0x4b,0x19,      /* L Theta */
     0x81,0xcc,0xcc,0x9a,0x25, /* L Alpha */
     0x53,0x81,0x74,0x21,      /* L Beta  */
     0, /* Not Used*/
     0x03, /* Right EMG */
     0xff,0xbf,0x4b, /* Delta */
     0x0c,0x03,      /* Theta 4.5, 6 */
     0x03,0x03,0x03,0x03,0x03, /* Alpha */
     0x03,0x03,0x03,0x03,      /* Beta  */
     0,  /* Not Used */
     0,MMS_DEMO_ACTIVE /* Two hijacked bytes */
    }
 };

 const Uift_display_demo emg_pattern = {
    /*0123456789012345*/
    "       EMG      ", /* 16 Characters long only*/
    /* Starts with EMG data and then 0.75Hz to 38Hz */
    {0x66, /* Left EMG */
     0x03,0x03,0x03, /* Delta */
     0x03,0x03,      /* Theta 4.5, 6 */
     0x03,0x03,0x03,0x03,0x03, /* Alpha */
     0x03,0x03,0x03,0x03,      /* Beta  */
     0, /* Not Used*/
     0x73, /* Right EMG */
     0x03,0x03,0x03, /* R Delta */
     0x03,0x03,      /* R Theta */
     0x03,0x03,0x03,0x03,0x03, /* R Alpha */
     0x03,0x03,0x03,0x03,      /* R Beta  */
     0,  /* Not Used */
     0,MMS_DEMO_ACTIVE /* Two hijacked bytes */
    }
 };
/*****END NEW PATTERNS ***/


 int demo_display_number;
/* IC96 V2.1 doesn't like the following - implement a switch()
 * const Uift_display_demo *demo_display[NUMBER_DEMO_DISPLAY] = {
 *    &am_pattern
 * };
 * See bug7.txt
 */
/**************************************************************
 * Internal prototypes 
 */
 void pattern(int number);
 void disp_pattern(const Uift_display_demo *demop);

/*;*<*>********************************************************
 * uif_display_pattern_initialisation
 **start*/
void uif_display_pattern_initialisation(Initt_system_state *ssp)
 {

 } /*end uif_display_pattern_initialisation*/

/*;*<*>********************************************************
  * uif_display_pattern
  *
  * This functions manages the changing of the demonstration
  * screen displays
  *
  * Input
  *    see UIFE_DEMO_OPTIONS
  **start*/
 void uif_display_pattern(enum UIFE_DEMO_OPTIONS options)
 { 
#define FIRST_DEMO_DISPLAY_NUMBER 0

   switch(options) {
   case UIFE_DEMO_START:
      demo_display_number = FIRST_DEMO_DISPLAY_NUMBER;
      break;

   case UIFE_DEMO_FINISH:
      /* Turn data supply back to filters */
      MfdSetInput(_ModuleDplFiltered,NULL);
      MfdSetInput(_ModuleSspFiltered,NULL);
      return;

   case UIFE_DEMO_FORWARD:
      demo_display_number++;
      break;

   case UIFE_DEMO_BACK:
      demo_display_number--;
      break;
   };

   /* Check range of demo_display */
   if ( demo_display_number < 0) {
      demo_display_number = (NUMBER_DEMO_DISPLAY-1);
   }
   if ( demo_display_number >= NUMBER_DEMO_DISPLAY) {
      demo_display_number = 0;
   }
   pattern(demo_display_number);

 }

/*;*<*>********************************************************
  * pattern
  *
  * Displays the data specified in the output stream of mfd_supply_data
  * and on the status line
  * 
  **start*/
 void pattern(int demo_number) {

  switch(demo_number) {

  case  0:
     disp_pattern(&creativity_pattern);
     break;

  case 1:
     disp_pattern(&dreamless_sleep_pattern);
     break;

  case 2:
     disp_pattern(&theta_dreaming_pattern);
     break;

  case 3:
     disp_pattern(&alpha_hypnagogic_pattern);
     break;

  case 4:
     disp_pattern(&beta_pattern);
     break;

  case 5:
     disp_pattern(&alpha_blocking_pattern);
     break;

  case 6:
     disp_pattern(&state4_pattern);
     break;

  case 7:
     disp_pattern(&awakened_mind_pattern);
     break;

  case 8:
     disp_pattern(&lateral_asymmetry_pattern);
     break;

  case 9:
     disp_pattern(&emg_pattern);
     break;

 default:
     break;
    
  }
 } /*end pattern*/

/*;*<*>********************************************************
 * disp_pattern
 **start*/
 void disp_pattern(const Uift_display_demo *demop)
 {

     MfdSetInput(_ModuleDplFiltered,&demop->pattern);
     MfdSetInput(_ModuleSspFiltered,&demop->pattern);
     uif_demo_statusp = demop->titlep;

 } /*end disp_pattern*/

/*;*<*>********************************************************
 * xyz
 **start*/
/* xyz()
 {

 } /*end xyz*/

/*;*<*>********************************************************
 * xyz
 **start*/
/* xyz()
 {

 } /*end xyz*/

