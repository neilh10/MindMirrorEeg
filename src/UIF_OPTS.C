/* uif_opts.c  uif OPTIONS/ACTION processing
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
 

 This file contains the uif module OPTIONS/ACTIONS processing. It uses 
 events to drive it through a series of menus. Each menu is implemented
 in tabular form, and to change the menus the tables are updated.

 The first event of any sequence is to switch the display into menu mode,
 and draw the top level menu.

 The last event MUST  be to restore the system to the previous state
 it was in before the module was invoked.

 In between the above events the following events are recognised
      ACTION
      DOWN one menu item
      UP one menu item

 This module looks after all boundary conditions with the top and bottom
 of menus.

 This file contains menu interface descriptions, and initialisation 
 of the data structures for those menus.

 Note: The IC96 compiler supports initialisation of
            constant objects at file scope
            constant and automatic objects at block scope.

 Algorithims
     uif_opts_initialsation  - first
     uif_opts - as many times as needed afterwards
     uif_opts_entry - as many times as needed
 */
 /* #define _Debug_ */
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
#include <string.h>
#include "general.h"
#include "hw.h"
#include "uif.h"
#include "ldd.h"
#include "uif_opts.h"
#include "mm_coms.h"
#include "mfd.h"
#include "mmstatus.h"

/**************************************************************
 * Constants 
 *
 * Perform all file scope initialization with the constant
 * prefix. These structures can't be modified by the program.
 */
#define CURSOR_POS 0 /* Pos of cursor on screen */
 enum MENU_TYPES {
  MENU_NEW,   /* Has a new menu */
  MENU_POP,   /* Pops an old menu off the stack */
  MENU_DRAW   /* Draws current menu    */
 };

/**************************************************************
 * Externally defined 
 */
 extern const char version[], ver_time[], ver_date[];

 extern char SchTxData;/* Data rate on serial output
	 //!0 if transmit at 8x/sec otherwise 0 for 2x/sec
	 // Initialize to !0 */


 
/**************************************************************
 * Internal storage to this module 
 */
 void (*menu_routine_entry)(void); /* Pointer to function if any menu
                        * item needs to be called
                        */

 
 int menu_cursor_line; /* Current position of the active menu item.
                        * 0 every time a new menu is drawn
                        */
#define BOTTOM_MENU 0
#define BOTTOM_MENU_OFFSET 1

 enum UIFE_OPTS_RET uifv_opts_ret;
 /*
  * Struct to handle menu's  - maintained by menu_scrn() routine
  *     - initialised for each new menu in uif_opts(UIFE_opts_NEW)
  */
#define MAX_NUM_MENUS 5
 static struct {
    int active_menu;
    menu *p[MAX_NUM_MENUS]; 
 } menu_manager;
#define active_menup menu_manager.p[menu_manager.active_menu]

/* Active montage */
/* enum UIFE_MONTAGE uifv_montage; */

/**************************************************************
 * Internal prototypes 
 */
 void menu_key(enum UIFE_OPTS action);
 void uif_opts_line(unsigned char cursor,char *description,int line_num);
 void menu_action(void);
 void menu_scrn(enum MENU_TYPES type, menu *mp);
 void montage_red_green(void);
 void montage_black_grey(void);
 void montage_lateral(void);
 void DataComp(void);

 void TxData2xSecond(void);
 void TxData8xSecond(void);
 void TxDataRaw(void);
 
#define MenuBufferSize 18
#ifdef _DataComp_ 
  char CompCommsState[MenuBufferSize];
  const char CompDataFiltered[] = " FILTERED DATA";
  const char CompDataRaw[]      = " RAW DATA";
#endif /* _DataComp_ */
 
/**************************************************************
 * Menu Definitions
 *
 * Perform all file scope initialization with the constant
 * prefix. These structures can't be modified by the program.
 */

/***Menu Element return ***************************/
 const tMenuElement menu_elem_return = {
   MENU_ELEMENT_MENU,
   NULL,   /* NULL So it exits */
   " RETURN"
 };

/*** Menu **************************
 *
 */
/*
const menu menu_ = {
   MENU_LIST,
   0,      /* Number of elements *!/
   " ",   /* Menu Heading - 16 Chars max *!/
   NULL /* Last element *!/
 };
*/
/*** Menu **************************
 *
 */
/*
const menu menu_ = {
   MENU_LIST,
   0,      /* Number of elements *!/
   " ",   /* Menu Heading - 16 Chars max *!/
   NULL /* Last element *!/
 };
*/
/*** Menu **************************
 *
 */
/*
const menu menu_ = {
   MENU_LIST,
   0,      /* Number of elements *!/
   " ",   /* Menu Heading - 16 Chars max *!/
   NULL /* Last element *!/
 };
*/

/*** Menu Montage **************************
 * Options to change which electrodes are looked at.
 */

 const tMenuElement menu_elem_pop_menu = {
   MENU_ELEMENT_MENU,
   NULL,   /* Go up one menu */
   " UP MENU"
 };

 const static tMenuElement menu_elem_lateral = {
   MENU_ELEMENT_ROUTINE_EXIT,
   (void *) montage_lateral,
   " LATERAL"
 };

 const static tMenuElement menu_elem_red_green = {
   MENU_ELEMENT_ROUTINE_EXIT,
   (void *) montage_red_green, /* fn to set monopolar montage */
   " RED/GREEN"
 };

 const static tMenuElement menu_elem_black_grey = {
   MENU_ELEMENT_ROUTINE_EXIT,
   (void *) montage_black_grey,
   " BLACK/GREY"
 };
		   
const menu menu_montage = {
   MENU_LIST,
   4,     /* Number of elements */
   "****MONTAGE****",   /* menu heading - 16 chars max */
   &menu_elem_pop_menu,
   &menu_elem_lateral,
   &menu_elem_red_green,
   &menu_elem_black_grey,
 };

 const tMenuElement menu_elem_montage = {
   MENU_ELEMENT_MENU,
   (void *) &menu_montage,
   " MONTAGE"
 };

 
/*** Menu Tx Data **************************
 * Options to change the transmited data 
 */

 const static tMenuElement menu_elem_txdata2x = {
	 MENU_ELEMENT_ROUTINE_EXIT,
	 (void *) TxData2xSecond,
	 " 2 UPDATES/sec"
 };

 const static tMenuElement menu_elem_txdata8x = {
	 MENU_ELEMENT_ROUTINE_EXIT,
	 (void *) TxData8xSecond, /*  */
	 " 8 UPDATES/sec"
 };

 const static tMenuElement menu_elem_txdataraw = {
	 MENU_ELEMENT_ROUTINE_EXIT,
	 (void *) TxDataRaw,
	 " RAW DATA"
 };

 const menu menu_txdata = {
	 MENU_LIST,
	 4,     /* Number of elements */
	 "Transmit Options",   /* menu heading - 16 chars max */
	 &menu_elem_pop_menu,
	 &menu_elem_txdata2x,
	 &menu_elem_txdata8x,
	 &menu_elem_txdataraw,
 };

 const tMenuElement menu_elem_txdata = {
	 MENU_ELEMENT_MENU,
	 (void *) &menu_txdata,
	 " TX OPTS"
 };

/***Menu Element upload ***************************/
 const tMenuElement menu_elem_upload = {
   MENU_ELEMENT_ROUTINE_EXIT,
   (void *) uif_upld,
   " UPLOAD"
 };


 /***Menu Element demonstration ***************************/
 const tMenuElement menu_elem_demo = {
   MENU_ELEMENT_ROUTINE,
   (void *) uif_set_mode_demo,
   " DEMO SCREENS"
 };

#ifdef _DataComp_
 /*** Menu Computer Data **************************
 * Options to change the type of data transferred
 */
 const static tMenuElement MenuElemComp = {
   MENU_ELEMENT_ROUTINE_EXIT,
   (void *) DataComp,
   &CompCommsState
 };
 
const menu MenuComputerData = {
   MENU_LIST,
   2,     /* Number of elements */
   "**DATA TO COMP**",   /* menu heading - 16 chars max */
   &menu_elem_pop_menu,
   &MenuElemComp,
 };
 const tMenuElement MenuElemCompData = {
   MENU_ELEMENT_MENU,
   (void *) &MenuComputerData,
   " COMP DATA"
 };
#endif /* _DataComp_ */

/*** Menu **************************
 * About Mind Mirror III information
 */
 const static tMenuElement menu_elem_version = {
   MENU_ELEMENT_MENU,
   NULL,  /* Go op one menu */
   &version
 };

  const static tMenuElement menu_elem_ver_date = {
   MENU_ELEMENT_MENU,
   NULL,  /* Go op one menu */
   &ver_date
 };

const static tMenuElement menu_elem_ver_time = {
   MENU_ELEMENT_MENU,
   NULL,  /* Go op one menu */
   &ver_time
 };


 const menu menu_about = {
   MENU_INFORMATION,
   3,      /* Number of elements */
   " MMIR VERSION ",   /* Menu Heading - 16 Chars max */
   &menu_elem_version,
   &menu_elem_ver_date,
   &menu_elem_ver_time,
   NULL      /* Last element */
 };
 const static tMenuElement menu_elem_about = {
   MENU_ELEMENT_MENU,
   (void *) &menu_about,
   " ABOUT MMIR"
 };

/***Top level Menu **************************
 * The following menu items are supported
 */
const menu menu_top = {
   MENU_LIST,
   6,    /* Number of menu elements */
   "***TOP LEVEL***",  /*Menu heading*/
   &menu_elem_return,
   &menu_elem_txdata,
   &menu_elem_upload,
   &menu_elem_montage,
   &menu_elem_demo,
#ifdef _Debug_ /* Not complete - MenuElemCompData */
   &MenuElemCompData,
#endif
   &menu_elem_about
 };

 /*;*<*>********************************************************
 * uif_opts_initialisation
 * 
 * Initialisation for this file
 **start*/
 void uif_opts_initialisation(Initt_system_state *ssp)
{

    menu_routine_entry = NULL;
    menu_manager.active_menu = 0;
    /*uifv_montage = MMS_MONTAGE_LATERAL; */
    
#ifdef _DataComp_
	strcpy(CompCommsState,CompDataRaw);
#endif /* _DataComp_ */

 } /*end uif_opts_initialisation*/

 /*;*<*>********************************************************
 * uif_opts
 *
 * This procedure is the normal entry point to this module. 
 *
 * The events that are accepted are
 *
 *     UIFE_OPTS_NEW  is invoked to setup the menu screen
 *     UIFE_OPTS_END  clearup for end of menu processing
 *     UIFE_OPTS_UP_ITEM    to take action for menu processing
 *     UIFE_OPTS_DOWN_ITEM  "   "
 *     UIFE_OPTS_ACTION     "   "
 *
 *
 **start*/
 enum UIFE_OPTS_RET uif_opts(enum UIFE_OPTS action)
 {
   uifv_opts_ret = UIFC_NO_ACTION;

   switch (action) {
   case UIFE_OPTS_NEW:
      menu_manager.active_menu = 0;
      menu_manager.p[0] = (menu *) &menu_top;
      menu_scrn(MENU_DRAW,NULL);
      break;

   case UIFE_OPTS_END:
      /* Screen cleared/initialised dpl_initialisation() */
      /* ldd_clr(LDDE_CLR_TEXT); */
      /*active_menu = NULL; */
      menu_routine_entry = NULL;
      break;

   default:
/* The following are also catered for
 *   case UIFE_OPTS_UP_ITEM:
 *   case UIFE_OPTS_DOWN_ITEM:
 *   case UIFE_OPTS_ACTION:
 */
      menu_key(action);
      break;
   }

   return uifv_opts_ret;

 } /*end uif_opts*/

/*;*<*>********************************************************
  *  uif_opts_entry
  * 
  * This function is called when uife_mode == UIFE_OPTS
  *   - when KEY_LEFT_ARROW, KEY_RIGHT_ARROW, KEY_RESTART
  *   - or else periodically when no key has been pressed
  *
  * If a menu item has registered the need to be polled periodically while
  *  active then it is called.
  *
  **start*/
 enum UIFE_OPTS_RET uif_opts_entry(enum key_pressed_enum key)
 {
   uifv_opts_ret = UIFC_NO_ACTION;
/*   if ( menu_routine_entry != NULL) {
     (*menu_routine_entry)();
   }
*/
   return uifv_opts_ret;
 } /*end uif_opts_entry*/

/*;*<*>********************************************************
  * menu_scrn
  *
  * This manages the menu screen. When a new menu is requested,
  * it is pushed onto an internal stack and drawn on the screen.
  * When a MENU_POP is requested the current menu is discarded and
  * the previous one drawn. This continues until no more menus
  * are available - when a global variable is set to indicate that
  * the menu system has been exited
  *
  *   MENU_NEW   Has a new menu (If new menu is NULL then acts like MENU_POP)
  *   MENU_POP   Pops an old menu off the stack
  *   MENU_DRAW  Draws current menu - default
  *
  *  The Menu Screen consists of
  *      Title Line
  *       7 Possible option lines
  *  The 16 lines on the physical screen are allocated as follows
  *    Line 0 Title
  *         1 Empty
  *         2 Option 1
  *         3 Empty or overflow from option 1
  *         4 Option 2
  *         5 Empty or overflow from option 2
  *         : : :
  *        14 Option 7
  *        15 Empty or overflow from option 7
  *
  * Algorithim
  *   Clear text screen
  *   Set up pointer to current menu
  *   If MENU_NEW & (*addr != NULL)
  *      Push new menu on to stack
  *      Set up pointers to new menu
  *   IF MENU_POP  or (MENU_NEW &(*addr == NULL))
  *      if top of stack
  *          indicate exit menuing
  *      else
  *          set up pointers to next menu up from stack
  *   Draw new menu
  *
  * Assumptions
  *   It is assumed that initialisation happens elsewhere
  **start*/
 void menu_scrn(enum MENU_TYPES type, menu *mp)
 {
  int elem_loop=0;
  register tMenuElement *mep;
  register menu *menup=active_menup;
  unsigned char cursor;


 ldd_clr(LDDE_CLR_TEXT);

 if (type == MENU_NEW) {
    if (mp != NULL) {
       /* New Menu
        * Add new menu to stack - first check if space on stack
        */
       if ( ++menu_manager.active_menu < MAX_NUM_MENUS ) {
          menu_manager.p[menu_manager.active_menu] = menup = mp; /* New menu */
       } else {
         /* Exceeded allowed number of menus
          * don't push old menu - throw new menu away
          */
         --menu_manager.active_menu;
       }
    } else {
       /* Pop new menu off stack - get MENU_POP to do work
        */
       type = MENU_POP;
    }
 }
 if (type == MENU_POP) {
    if ( --menu_manager.active_menu >= 0) {
       /* Pop new menu */
       menup = menu_manager.p[menu_manager.active_menu]; /* New menu */
    } else {
       menu_manager.active_menu = 0;
       menup = NULL;
       uifv_opts_ret = UIFC_END_MENUS;
       return;
    }
 }

 ldd_line(BOTTOM_MENU,(uchar *) menup->title);

 /* Initialise for first line on screen*/
 menu_cursor_line = 0;
 cursor = '>';

 for (elem_loop=0; elem_loop < menup->num_elements;elem_loop++) {
    mep = (tMenuElement *) menup->elem[elem_loop];
    uif_opts_line(cursor,
                (char *) mep->description,
                (elem_loop+BOTTOM_MENU_OFFSET) );
    cursor = ' ';
 }

 } /*end menu_scrn*/

/*;*<*>********************************************************
  * menu_key
  * 
  * This function deals with any actions that need to be taken
  *
  * These are
  *    UP    - adjust cursor '>' if possible
  *   DOWN   - adjust cursor '>' if possible
  *   ACTION - takes action
  *
  **start*/
 void menu_key(enum UIFE_OPTS action)
 {
 register  menu *mp = active_menup;

    switch (action) {
    case UIFE_OPTS_DOWN_ITEM:
       ldd_wr_ascii_char( CURSOR_POS,
                   ((menu_cursor_line+BOTTOM_MENU_OFFSET) << 1),
                   ' ');
       if (++menu_cursor_line >= mp->num_elements) {
           menu_cursor_line = mp->num_elements-1;
       }
       ldd_wr_ascii_char( CURSOR_POS,
                   ((menu_cursor_line+BOTTOM_MENU_OFFSET) << 1),
                   '>');
       break;

    case UIFE_OPTS_UP_ITEM:
       ldd_wr_ascii_char( CURSOR_POS,
                   ((menu_cursor_line+BOTTOM_MENU_OFFSET) << 1),
                    ' ');
       if (--menu_cursor_line < BOTTOM_MENU) {
           menu_cursor_line = BOTTOM_MENU;
       }
       ldd_wr_ascii_char( CURSOR_POS,
                   ((menu_cursor_line+BOTTOM_MENU_OFFSET) << 1),
                    '>');
       break;

    case UIFE_OPTS_ACTION:
       menu_action();
       break;

    }

 } /*end menu_key*/

/*;*<*>********************************************************
  * uif_opts_line
  * 
  * Prints specified line on screen with cursor
  **start*/
 void uif_opts_line(unsigned char cursor,char *description,int line_no)
 {
  char buf[20];

     strcpy(&buf[1],description);
     buf[CURSOR_POS] = cursor;
     ldd_line( (line_no << 1),  /* Mult by two to provide spacing on scrn*/
               (uchar *) &buf[0]);

 } /*end uif_opts_line*/

/*;*<*>********************************************************
  * menu_action
  *
  * A menu item needs to be acted on.
  * checks out menu_elementp->type and takes appropiate action
  *
  **start*/
 void menu_action(void)
 {
  register menu *mp = active_menup;
  register void (*fp)(void);
  register enum menu_element_type type = mp->elem[menu_cursor_line]->type;

  switch (type){
  case MENU_ELEMENT_ROUTINE: /* Address of routine in *addr      */
  case MENU_ELEMENT_ROUTINE_EXIT:
     fp = (void (*)) mp->elem[menu_cursor_line]->addr; /* Get supplied routine */
     if (fp != NULL) {
        (*fp)();  /* Execute supplied routine */
     }
     if (type == MENU_ELEMENT_ROUTINE_EXIT) {
        /* Need to exit at end */
        uifv_opts_ret = UIFC_END_MENUS;
     }
     break;

  case MENU_ELEMENT_MENU:    /* Address of another menu in *addr */
     /* Set up new menu */
     menu_scrn(MENU_NEW, (menu *) mp->elem[menu_cursor_line]->addr);
     break;
  }
 } /*end menu_action*/

/*;*<*>********************************************************
  * montage_red_green
  *
  * This function sets the front end electrodes to look at a 
  * monopolar montage .... with respect to GND.
  *
  **start*/
 void montage_red_green(void)
 {
    mfd_control(MFDC_MONTAGE,MMS_MONTAGE_RED_GREEN);
    hw_front_end_mode(HW_FE_FRONT_IN, HWE_ATT_BOTH, HWE_REFRESH);
 } /*end montage_read_green*/

/*;*<*>********************************************************
  * montage_black_grey
  *
  * This function sets the front end electrodes to look at a 
  * monopolar montage .... with respect to GND.
  *
  **start*/
 void montage_black_grey(void)
 {
    mfd_control(MFDC_MONTAGE,MMS_MONTAGE_BLACK_GREY);
    hw_front_end_mode(HW_FE_BACK_IN, HWE_ATT_BOTH, HWE_REFRESH);
 } /*end montage_black_grey*/

/*;*<*>********************************************************
  * montage_lateral
  *
  * This function sets the front end electrodes to look at a 
  * bipolar montage .... the Mind Mirror way.
  *
  **start*/
void montage_lateral(void)
 {
    mfd_control(MFDC_MONTAGE,MMS_MONTAGE_LATERAL);
    hw_front_end_mode(HW_FE_LATERAL_IN, HWE_ATT_BOTH, HWE_REFRESH);

 } /*end montage_lateral*/

/*;*<*>********************************************************
  * TxData2xSecond
  *
  * This function sets the Tx Data rate to be
  *
  **start*/
 void TxData2xSecond(void)
 {
	 SchTxData = 0;
	 MfdSetInput(_ModuleSspFiltered,NULL);
 } /*end TxData2xSecond*/
 
/*;*<*>********************************************************
  * TxData8xSecond
  *
  * This function sets the Tx Data rate to be
  *
  **start*/
 void TxData8xSecond(void)
 {
	 SchTxData = 1;
	 MfdSetInput(_ModuleSspFiltered,NULL);
 } /*end TxData8xSecond*/

/*;*<*>********************************************************
  * TxDataRaw
  *
  * This function sets the Tx Data rate to be
  *
  **start*/
 void TxDataRaw(void)
 {
	 SchTxData = 1;
	 MfdSetInput(_ModuleSspRaw,NULL);
	 
 } /*end TxDataRaw*/

#ifdef _DataComp_
/*;*<*>********************************************************
  * DataComp
  
    Menu item for changing the type of data going
    down the serial link.
    
  **start*/
 void DataComp (void)
 {

	  if (DataToSsp == _FilteredData_) /* Change to _RawData */
	  {
		  strcpy(CompCommsState,CompDataFiltered);
		  MfdSetInput(_ModuleSspRaw,NULL);
	  }
	  else /*Assume _RawData_ and change to FilteredData_ */
	  {
		  strcpy(CompCommsState,CompDataRaw);
		  MfdSetInput(_ModuleSspFiltered,NULL);

	  }
 } /*end DataCompFilter*/
#endif
 
/*;*<*>********************************************************
  * xxx
  **start*/
/* void xxx (void)
 {

 } /*end xxx*/


