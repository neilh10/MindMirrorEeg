/*ldd_tlx.c
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
 LCD Device Driver Module for Toshibas TLX range of screens

 This module drives the display for
    TOSHIBA TLX-1013
            TLX-1391

 Note this file is used in MMIR and DVSTY and must be kept in sync
 Version Date
  0.1    22/July/92   Orginal


 This module supports access to a physical display of type
    graphics            text
   <----XG---->       <---XT---->
   ^                  ^
   |YG                |YT
   V                  V

  +-----+-------+-------+-----+
  |     | TLX-1391      | 1013|
  |     |6*8 dot|8*8 dot|     |
  |     |fonts  | fonts |     |
  +-----+-------+-------+-----+
  |  XG | 126   |  128  | 160 |
  |  YG | 128   |  128  | 128 |
  |  XT |  21   |   16  | 20  |
  |  YT |  16   |   16  | 16  |
  +-----+-------+-------+-----+

  The routines used to support this are
     ldd_determine_scrn()    - not used at present
     ldd_init_device()
     ldd_init_cg_ram(flag_char, *data)
     ldd_graphics_set(type, xg, yg, req)
     ldd_graphics_clr(type, xg, yg, req)
     ldd_line(line_num, string) -writes line on line number specified
     ldd_putchar(char)
     ldd_putchar_set_cur(txt_window_start, txt_window_size)
     ldd_wrchar(x,y,char)
     ldd_logo()
 This physical display has two settings that are set by the FS pin
 in hardware. They are (Pg 11 & 12)

  FS = GND or 0V
     8 * 8 dot fonts giving; 16 character wide lines by 16 lines
                             128 (width) by 128 (height) graphics display
  FS = VDD or +5V
     6 * 8 dot fonts giving; 21 character wide lines by 16 lines
                             126 (width) by 128 (height) graphics display

 There is no way of detecting from software which strapping option
 has been selected in hardware.


 Implementation Details
 Toshiba defines displays as 
      1 screen driver modules
   or 2 screen driver modules

 The 1 screen driver modules is as expected - all memory is linear from
 the starting address. 
 The 2 screen driver module consists of (TLX-1013)
            <-----------------160 dots--------------->
 GH+  0 +0:0+----------------------------------------+ GH+ 0 +19:7
 GH+ GA +0:0|                                    /|\ | GH+ GA+19:7
            |                                 63  |  |
 GH+62GA+0:0|                                 dots|  | GH+62GA+19:7
 GH+63GA+0:0|                                    \|/ | GH+63GA+19:7
            +----------------------------------------+ 
GH1+  0 +0:0+----------------------------------------+ GH1+ 0 +19:7
GH1+ GA +0:0|                                    /|\ | GH1+ GA+19:7
            |                                 63  |  |
GH1+62GA+0:0|                                 dots|  | GH1+62GA+19:7
GH1+63GA+0:0|                                    \|/ | GH1+63GA+19:7
            +----------------------------------------+

Where GH (Graphic Home address) = 0
      GH1  = GH + 0x8000        = 0x8000

NOTE 1: 
When TLX_2_SCREEN is defined, then all modules compensate for 
the above problem, except for the primitives

      read_com_0
      write_com_1
      write_com_2
      write_data()

NOTE 2: The TLX displays support text and graphics. There is an attribute
function defined by the TLX modules, however to use this the graphics
function must be turned off. See algorithm sect 5.5.2
    "Procedure of setting attribute" in particular the mode set
    command is for text only
*/
/*#define WR_DEBUG1*/
#ifdef IC96
#include <h\cntl196.h>
#pragma nolistinclude
#endif
#include <80c196.h>

#include "general.h"
#include "ldd.h"
#include "hw_mem.h"

#pragma optimize(0)  /* Level 0 is best for speed of vital routines*/
/**************************************************************
 * Constants 
 */

#ifdef TLX_1391 /************************/
#define CG_HOME_ADDRESS 0x1C00

/* Defines for LCD screen */
#define INIT_1_GRPH_CURSOR 0
#define INIT_1_TXT_CURSOR 0
#define TXT_1_LINE_WIDTH 0x10
#define TXT_1_SCRN_HGHT 0x10
/*#define TXT_STATUS (INIT_1_TXT_HOME_ADDRESS+15*LENGTH_TXT_LINE)
*/
#define TXT_LN_STATUS 15
#define LCD_MEMORY_SIZE 0x2000 /* 8Kbytes */
#endif /*TLX_1391 */

#ifdef TLX_1013 /************************/
#define CG_HOME_ADDRESS 0x0800 /* Cater for CG_ROM_MODE and limited storage*/

/* Defines for LCD screen */
#define INIT_1_GRPH_CURSOR 0
#define INIT_1_TXT_CURSOR 0
#define TXT_1_LINE_WIDTH 0x14
#define TXT_1_SCRN_HGHT  0x10
#define TLX_GS2_OFFSET (TLX_GA_SCREEN2_OFFSET-TLX_GRPH_LN_LNGTH*64)
#define TLX_GRPH_END_SCRN_1_LN_OFFSET 63
#define TLX_GRPH_END_SCRN_1_TXT_OFFSET (TLX_GRPH_END_SCRN_1_LN_OFFSET*TLX_1_LINE_WIDTH)
#define TLX_TXT_END_SCRN_1_LN_OFFSET  7
#define TLX_TXT_END_SCRN_1_TXT_OFFSET (TLX_TXT_END_SCRN_1_LN_OFFSET*TLX_1_LINE_WIDTH)
/*#define TXT_STATUS (INIT_1_TXT_HOME_ADDRESS+TLX_GA_SCREEN2_OFFSET+7*LENGTH_TXT_LINE)
*/
#define TXT_LN_STATUS 15
/*#define LCD_MEMORY_SIZE 0x2000 /* 8Kbytes */

#endif /*TLX_1013 */

/* Common Defines for all LCDs */
#define LENGTH_TXT_LINE TLX_GRPH_LN_LNGTH
#define LDD_MAX_STATUS_LENGTH TLX_GRPH_LN_LNGTH
#define LCD_GRAPHICS_MEMORY_SIZE (TLX_GRPH_LN_LNGTH * TLX_SCREEN_Y)
#define TLX_TXT_SCRN_SIZE TLX_GRPH_LN_LNGTH*TXT_1_SCRN_HGHT

/* See section 4.4.2  for description of below */
#define INIT_CG_HOME_ADDRESS (CG_HOME_ADDRESS>>11)
/*#define CG_RAM_OFFSET (CG_HOME_ADDRESS | 0x80)*/


/**************************************************************
 * Internal enums/typedefs 
 */

/**************************************************************
 *   Internal storage 
 */
 /*Word txt_home_addr,/* The offset into the display ram that
                     * is the starting position of the text area 
                     * Range: 0 to 0x7fff
                     */
static Word txt_x_cursor, txt_y_cursor; /* Must be out of ext ram */
                    /* The cursor position into the display area 
                     * This is the offset address into the display ram.
                     * ie it needs to be added to INIT_1_TXT_HOME_ADDRESS
                     * Range: txt_x_cursor 0 to LENGTH_TXT_LINE-1
                     *        txt_y_cursor 0 to TXT_1_SCRN_HGHT-1
                     */
/*   grph_home_addr,  /* The offset into the display ram that is the
                     * starting position of the graphics area
                     * Range:
                     */
/*   grph_cur_pos,    /* The cursor position into the display area
                     * This is the physical address into the display ram.
                     */
Word grph_end,      /* This is the last location in the display ram
                     * that will be displayed as text.
                     */
   grphb_ln_lngth;  /* This is the length of the graphics line in
                     * bytes.
                     * It may be 16 or 22 (dec)
                     */
/* Location of the LCD registers */
 char command, data;
#ifdef REAL_TARGET
#pragma locate(data=TLX_DATA_PORT,command=TLX_COMMAND_PORT)
#endif /*REAL_TARGET*/
#ifdef EVAL_TARGET
#pragma locate(data=TLX_DATA_PORT,command=TLX_COMMAND_PORT)
#endif /* EVAL_TARGET */

/* Shared variables */

/* Externally defined */

/**************************************************************
 * Internal prototypes 
 */
 void write_txt(int data1);
 void auto_reset(void);
 int gen_mask(int bit_num,int *num_bitsp,int num_req);
#define LCD_READY  0x3 /* See pg 16 TLX-1391-E0*/
#define LCD_DAV_READY 0x4  /*STA2 DAV      */
#define LCD_RDY_READY 0x8  /*STA3 RDY Pg 16*/
#define LCD_CLR_READY 0x20 /*STA5 CLR      */
/*#define chk_status() while ((command&LCD_READY)!=LCD_READY) {delay(2);}*/
#define chk_status() while ((command&LCD_READY)!=LCD_READY) {}
#define chk_dav_status() while ((command&LCD_DAV_READY)!=LCD_DAV_READY) {}
#define chk_clr_status() while ((command&LCD_CLR_READY)!=LCD_CLR_READY) {}

#ifndef WR_DEBUG1
#define chk_rdy_status() while ((command&LCD_RDY_READY)!=LCD_RDY_READY) {}
#else /* WR_DEBUG1*/
void chk_rdy_status(void) {
	register char mirror;
	register int cnt=0;

	mirror = command;

        printf(" %x,",0xff&mirror);

	while ((mirror&LCD_RDY_READY)!=LCD_RDY_READY) {
		delay (100);
		if (++cnt > 1000) {
			printf("\n\rldd_tlx: LCD RDY stuck %x",0xff&mirror);
			cnt=9;
		} 
		mirror = command;
	}
}
#endif /* WR_DEBUG1 */
#ifdef WR_DEBUG1
void read_command(void) {
	register char mirror;
/* Debug -read in command forever */
	while (1) {
		mirror = command;
		/*mirror = data;*/
/*
		delay (30000);
		delay (30000);
		delay (30000);
		delay (30000);
		printf("%x ",0xff&mirror);
*/
	}
}
#endif /* WR_DEBUG1 */

 /*;*<*>********************************************************
 * ldd_determine_scrn
 *
 * This is called before initialisation to determine what type
 * of screen is attached
 *
 **start*/
 void ldd_determine_scrn(Ldd_defs *defsp)
 {
   /*
    * Not used - user must know at compile time
    */

 } /*end ldd_determine_scrn*/

/*;*<*>********************************************************
 * ldd_init_device
 *
 *
 **start*/
 enum inite_return ldd_init_device(Ldd_defs *defsp)
{
      /* 
       * Init physical device
       */
      write_com_0(SET_MODE_ROM_OR_COM); /* Mode Set */

      /*
       * Tell the LCD controller where the graphics area
       * starts and how many bytes go on a displayed line
       */
      /*grph_home_addr = INIT_1_GRPH_HOME_ADDRESS;*/
      /*grph_cur_pos = INIT_1_GRPH_CURSOR;*/

      write_com_2(INIT_1_GRPH_HOME_ADDRESS/*grph_home_addr*/,   SET_GRAPHIC_HOME_ADDR_COM);
      write_com_2(TLX_GRPH_LN_LNGTH, SET_GRAPHIC_AREA_COM);

      /*
       * Tell the LCD controller where the text area
       * starts and how many characters go on a displayed line
       */
/*      txt_home_addr = INIT_1_TXT_HOME_ADDRESS;
      txt_cur_pos = INIT_1_TXT_CURSOR;
*/
      write_com_2(INIT_1_TXT_HOME_ADDRESS/*txt_home_addr*/,SET_TXT_HOME_ADDR_COM);
      write_com_2(LENGTH_TXT_LINE,SET_TXT_AREA_COM);

      /*
       * Initialise the LCD memory to zero
       */
      ldd_clr(LDDE_CLR_ALL);

      /*
       * Leave the address pointer pointing to text location
       */
      /*write_com_2(INIT_1_GRPH_HOME_ADDRESS/*grph_home_addr*!/, ADDRESS_POINTER_SET_COM);*/

      /*
       * Setup offset register to  point to Custom Graphics area
       */
      write_com_2(INIT_CG_HOME_ADDRESS, OFFSET_REGISTER_SET_COM);

      /*
       * Display Mode Set
       */
      write_com_0(DISPLAY_COM);

/*               |SET_CURSOL_BLINK_ON
               |SET_CURSOL_DISPLAY_ON
               |SET_TXT_DISPLAY_ON
               |SET_GRAPHIC_DISPLAY_ON) */

      /* Set up internal variables for possible text writes */
      ldd_putchar_set_cur(0,0);

   return PASS;
 } /*end ldd_196_init */

 /*;*<*>********************************************************
 * ldd_init_cg_ram
 *
 * This function initialises the TLX CG ram data area
 * for the specified character.
 *
 * Input:
 *   flag_char - the character handle - bits 0 to 6, 7th ignored
 *               This is added to the base of the CG ram address
 *               and used to determine where to write the data
 *   *data     - pointer pointing to 8 bytes of data, that will
 *               be written at the address specifed by 'flag_char'
 *
 **start*/
 void ldd_init_cg_ram(uint flag_char, char *data)
 {
 register int loop;
#define CG_SIZE 8
    /*
     * Setup address pointer to the correct start address
     * of the Character Graphics ram.
     * Note if in rom mode bit 7 must be set
     */
    write_com_2((CG_HOME_ADDRESS | ((flag_char) << 3)),
                 ADDRESS_POINTER_SET_COM);

    write_com_0(DATA_AUTO_WRITE_COM);      /* Setup for a stream of bytes*/
    for (loop=0;loop < CG_SIZE; loop++) {
       write_data(*data++);                /* Write data */
    }
    auto_reset(); /* End of stream of bytes */

 } /*end init_cg_ram*/

 /*;*<*>********************************************************
 * ldd_graphics_set
 *
 * Sets the specified graphics bit or graphics byte depending on 'type'
 * If type != 1 then the bits that are set are those in the Y axis (or 
 * vertical to the viewer)
 * 
 * Input: type - in range from 1 to 8
 *        xg  - x position
 *        yg  - y position
 *        num_req - Number of bits to set, Range 1 to 8.
 **start*/
 int ldd_graphics_set(enum ldde_type type, int xg, int yg, int num_req) 
 {
 register int byte_num,start,loop,end_loop,bit_num, mask;
          int num_bits;
 static int char_loc;
    /* 
     * Determine bit number that needs set 
     */
#if TLX_DOTS_PER_CHAR == 8
   byte_num = xg>>3;
   num_bits = bit_num = (0x7 - (xg & 0x7));/* Invert 0 to 7 to make 7 to 0 */
#else /*TLX_DOTS_PER_CHAR */
   byte_num = xg/TLX_DOTS_PER_CHAR;
   bit_num = TLX_DOTS_PER_CHAR - 1 - ( xg - (byte_num*TLX_DOTS_PER_CHAR));
   num_bits = 1;
#endif /*TLX_DOTS_PER_CHAR */

   mask = gen_mask(bit_num, &num_bits, num_req);
   start = yg*TLX_GRPH_LN_LNGTH+byte_num;
   for(loop = start,end_loop=start+type*TLX_GRPH_LN_LNGTH;
       loop < end_loop;
       loop += TLX_GRPH_LN_LNGTH) {

#ifdef TLX_2_SCREEN
      /* Check to see if writing into screen 2 and adjust point
       * if true
       */
      if( yg >= 64) {
         write_com_2(((uint)loop+TLX_GS2_OFFSET),ADDRESS_POINTER_SET_COM);
      } else {
        write_com_2(loop, ADDRESS_POINTER_SET_COM);
      }
#endif /* TLX_2_SCREEN */
#ifndef TLX_2_SCREEN
      write_com_2(loop, ADDRESS_POINTER_SET_COM);
#endif
      /*chk_clr_status();*/
      write_com_1( (mask|read_com_0(DATA_BYTE_READ_COM)),
                   DATA_BYTE_WRITE_COM);

/*      write_com_0(BIT_SET_COM|bit_num); */
   }

   return num_bits;
 } /*end ldd_graphics_set */

 /*;*<*>********************************************************
 * ldd_graphics_clr
 *
 * Clears the specified graphics bit or graphics byte depending on 'type'
 *
 * input: as for ldd_graphics_set
 *
 **start*/
 int ldd_graphics_clr(enum ldde_type type,int xg,int yg, int num_req)
 {
 register int loop,end_loop,bit_num,byte_num,start, mask;
          int num_bits;

   /* 
    * Determine bit number that needs set 
    */
#if TLX_DOTS_PER_CHAR == 8
   byte_num = xg>>3;
   num_bits = bit_num = (0x7 - (xg & 0x7));/* Invert 0 to 7 to make 7 to 0 */
#else /*TLX_DOTS_PER_CHAR */
   byte_num = xg/TLX_DOTS_PER_CHAR;
   bit_num = TLX_DOTS_PER_CHAR - 1 - ( xg - (byte_num*TLX_DOTS_PER_CHAR));
   num_bits = 1;
#endif /*TLX_DOTS_PER_CHAR */

   mask = ~gen_mask(bit_num, &num_bits, num_req);
   start = yg*TLX_GRPH_LN_LNGTH+byte_num;
   for(loop = start,end_loop=start+type*TLX_GRPH_LN_LNGTH;
       loop < end_loop;
       loop += TLX_GRPH_LN_LNGTH) {

#ifdef TLX_2_SCREEN
      /* Check to see if writing into screen 2 and adjust point
       * if true
       */
      if( yg >= 64) {
         write_com_2(((uint)loop+TLX_GS2_OFFSET),ADDRESS_POINTER_SET_COM);
      } else {
        write_com_2(loop, ADDRESS_POINTER_SET_COM);
      }
#endif /* TLX_2_SCREEN */
#ifndef TLX_2_SCREEN
      write_com_2(loop, ADDRESS_POINTER_SET_COM);
#endif
      write_com_1( ( mask & read_com_0(DATA_BYTE_READ_COM)),
                   DATA_BYTE_WRITE_COM);
      /* write_com_0(BIT_SET_COM|bit_num); should be this */
   }

   return num_bits;
 } /*end ldd_graphics_clr */

 /*;*<*>********************************************************
 * ldd_line
 *
 * This function prints the first LDD_MAX_STATUS_LENGTH characters found in
 * string, on the line specified.
 *
 * Note: ASCII characters are expected.
 **start*/
 void ldd_line(uint line_num,uchar *string)
 {
 int txt_ascii,loop, txt_start;

     txt_start=INIT_1_TXT_HOME_ADDRESS/*txt_home_addr*/;
/*    write_com_2(INIT_1_TXT_HOME_ADDRESS/*txt_home_addr*!/,SET_TXT_HOME_ADDR_COM);
     write_com_2(LENGTH_TXT_LINE,SET_TXT_AREA_COM);
*/
#ifdef TLX_2_SCREEN
    if (line_num > TLX_TXT_END_SCRN_1_LN_OFFSET) {
       /* Force to write to lower half of screen */
       txt_start += TLX_GA_SCREEN2_OFFSET;
       line_num  -= (TLX_TXT_END_SCRN_1_LN_OFFSET+1);
    }
#endif /* TLX_2_SCREEN */
    txt_start += (line_num*LENGTH_TXT_LINE);
    write_com_2(txt_start,ADDRESS_POINTER_SET_COM);
    /* delay_ms(2); /* njh */
    write_com_0(DATA_AUTO_WRITE_COM);

    for (loop=0;loop < LDD_MAX_STATUS_LENGTH; loop++) {
       if((txt_ascii = *string++) == 0)
          break; /* Found End Of String */
       write_txt(txt_ascii);
    }
    auto_reset();
    /*write_com_0(SET_MODE_ROM_OR_COM); /* Mode Set */
    /*write_com_0(DISPLAY_COM);*/

 return;
 } /*end ldd_line */

 /*;*<*>********************************************************
 * ldd_aline - not debugged - cann't be used with graphics display
 *
 * This function prints the first LDD_MAX_STATUS_LENGTH characters found in
 * string, in the attributes area of the line specified.
 *
 * Input from the TLX manual
 *     * * * * N3 N2 N1 N0
 *             0  0  0  0  TLX_A_NORMAL       Normal display
 *             0  1  0  1  TLX_A_REV          Reverse display, text only
 *             0  0  1  1  TLX_A_INHIBIT      Inhibit Display
 *             1  0  0  0  TLX_A_BLINK        Blink of normal display
 *             1  1  0  1  TLX_A_BLINK_REV    Blink of reverse display
 *             1  0  1  1  also TLX_A_INHIBIT
 **start*/
/* void ldd_aline(uint line_num,uchar *string)
 {
 int txt_ascii,loop, txt_start;

     txt_start=INIT_1_ATT_HOME_ADDRESS/*txt_home_addr*!/;
/*    write_com_2(INIT_1_TXT_HOME_ADDRESS/*txt_home_addr*!/,SET_TXT_HOME_ADDR_COM);
     write_com_2(LENGTH_TXT_LINE,SET_TXT_AREA_COM);
*!/
#ifdef TLX_2_SCREEN
    if (line_num > TLX_ATT_END_SCRN_1_LN_OFFSET) {
       /* Force to write to lower half of screen *!/
       txt_start += TLX_GA_SCREEN2_OFFSET;
       line_num  -= (TLX_ATT_END_SCRN_1_LN_OFFSET+1);
    }
#endif /* TLX_2_SCREEN *!/
    txt_start += (line_num*LENGTH_TXT_LINE);
    write_com_2(txt_start,ADDRESS_POINTER_SET_COM);
    write_com_0(DATA_AUTO_WRITE_COM);

    for (loop=0;loop < LDD_MAX_STATUS_LENGTH; loop++) {
       write_data(txt_ascii);
    }
    auto_reset();

 return;
 } /*end ldd_aline */

 /*;*<*>********************************************************
 * ldd_putchar
 *
 * Writes data to the TLX screen, at the cursor position previously
 * defined.
 *
 * The screen looks like
 * Line                      Line end Address
 * Num 
 *  0    +---------------+   TLX_GRPH_LN_LNGTH-1
 *  1    |               | 2*TLX_GRPH_LN_LNGTH-1
 *  2    |               | 3*TLX_GRPH_LN_LNGTH-1
 *
 *  x    |               | TLX_TXT_SCRN_SIZE-1
 *       +---------------+
 *  Where x is TXT_1_SCRN_HGHT
 *
 * Algorithm
 *   ldd_putchar_set_cur
 *   ldd_putchar           as many times as needed
 *   ldd_putchar_set_cur - to reset internal pointers at any time
 *
 * Unsupported functions:
 *   ASCII control codes such as CR & LF function
 **start*/
 int ldd_putchar(int data1)
 {
    if (data1 < ASCII_OFFSET) {
       if ( data1 == '\r' ){
          txt_x_cursor = 0;
       } else if( data1 == '\n'){
          txt_y_cursor++;
       } else {
          return -1;
       }
       return data1;
    }

    if (++txt_x_cursor >= LENGTH_TXT_LINE ) {
      txt_x_cursor = 0;
      txt_y_cursor++;
    }
    if (txt_y_cursor >= TXT_1_SCRN_HGHT) {
       txt_y_cursor = TXT_1_SCRN_HGHT-1;
    }
    ldd_wrchar(txt_x_cursor,txt_y_cursor,(data1-ASCII_OFFSET));
    return data1;

 } /*end ldd_putchar*/

 /*;*<*>********************************************************
 * ldd_putchar_set_cur
 *
 * Initialises the ldd_putchar variables
 *
 * See ldd_putchar for a description of how text is written to the
 * screen, and what the screen looks like to the caller.
 *
 * Input
 *   txt_x_pos - same as txt_x_cursor
 *   txt_y_pos - same as txt_y_cursor
 *
 **start*/
 void ldd_putchar_set_cur(int x_pos, int y_pos)
 {

    txt_x_cursor = x_pos;
    txt_y_cursor = y_pos;

 } /*end ldd_putchar_set_cur*/

 /*;*<*>********************************************************
 * ldd_wr_ascii_char
 *
 * Writes an ascii character to the specified character position. 
 *
 * Uses coordinates
 *    |-----> x_pos
 *    |
 *   \|/ y_pos
 *
 * Note:
 * Does not cater for two screen TLX-1013
 **start*/
 void ldd_wr_ascii_char(int x_pos, int y_pos,int data1)
 {
    ldd_wrchar(x_pos, y_pos, data1 - ASCII_OFFSET);
 } /*end ldd_wr_ascii_char*/

 /*;*<*>********************************************************
 * ldd_wrchar
 *
 * Writes a character to the specified character position. 
 *
 * Uses coordinates
 *    |-----> x_pos
 *    |
 *   \|/ y_pos
 *
 * Note:
 * Does not cater for two screen TLX-1013
 **start*/
 void ldd_wrchar(int x_pos, int y_pos,int data1)
 {

    write_com_2((INIT_1_TXT_HOME_ADDRESS+x_pos+(TXT_1_LINE_WIDTH*y_pos)),
                 ADDRESS_POINTER_SET_COM);

    write_com_1(data1,DATA_BYTE_WRITE_INC_COM);

 } /*end ldd_wrchar*/

 /*;*<*>********************************************************
 * ldd_logo
 *
 * This writes a logo to the screen.
 *
 * Input
 *    *logo_structp points to the logo, size and starting position
 *    starting position must take into account the split screen of
 *    TLX1013
 *
 **start*/
 void ldd_logo(Logo *logo_structp)
 {
  char *logo_datap = logo_structp->datap;
  int loop, end_loop = logo_structp->size;

/* The following has some bugs in it - leave out
*  long int start = (unsigned int) logo_structp->start;
*
*#ifdef TLX_2_SCREEN
*  if (start > TLX_GA_SIZE_SCREEN1) {
*     start += (TLX_GA_SCREEN2_OFFSET-TLX_GA_SIZE_SCREEN1);
*  }
*#endif /* TLX_2_SCREEN */

  write_com_2(logo_structp->start/*grph_home_addr*/, ADDRESS_POINTER_SET_COM);
  write_com_0(DATA_AUTO_WRITE_COM);

  for (loop=0; loop < end_loop; loop++) {
#ifdef TLX_2_SCREEN
     /* Check if switching TLX screens */
     if (loop == TLX_GA_SIZE_SCREEN1) {
        auto_reset();
        write_com_2((INIT_1_GRPH_HOME_ADDRESS/*grph_home_addr*/+TLX_GA_SCREEN2_OFFSET), ADDRESS_POINTER_SET_COM);
        write_com_0(DATA_AUTO_WRITE_COM);
     }
#endif /* TLX_2_SCREEN */
     write_data(*logo_datap++); /* Write Data */
  }
  auto_reset();

 } /*end ldd_logo*/

 /*;*<*>********************************************************
 * ldd_scrn_test
 *
 * Tests the edges of the screen
 **start*/
 void ldd_scrn_test(enum ldde_scrn test)
 {
 register int loop;
     /* Do top and bottom edges */
     for (loop=0; loop < TLX_SCREEN_X; loop++) {
        if (test == LDDE_EDGES_ON) {
           ldd_graphics_set(LDDE_BIT,loop,0,1);
           ldd_graphics_set(LDDE_BIT,loop,(TLX_SCREEN_Y-1),1);
        } else {
           ldd_graphics_clr(LDDE_BIT,loop,0,1);
           ldd_graphics_clr(LDDE_BIT,loop,(TLX_SCREEN_Y-1),1);
        }
     }

     /* Do left and right sides */
     for (loop=0; loop < TLX_SCREEN_Y; loop++) {
        if (test == LDDE_EDGES_ON) {
           ldd_graphics_set(LDDE_BIT,0,loop,1);
           ldd_graphics_set(LDDE_BIT,(TLX_SCREEN_X-1),loop,1);
        } else {
           ldd_graphics_clr(LDDE_BIT,0,loop,1);
           ldd_graphics_clr(LDDE_BIT,(TLX_SCREEN_X-1),loop,1);
        }
     }
 } /*end ldd_scrn_test*/

 /*;*<*>********************************************************
 *  read_com_0
 *
 * This routine writes the lower byte of 'cmd' to cmd as a command.
 * It then reads the data register and returns with the value.
 **start*/
 Byte read_com_0(int cmd)
 {

   chk_status();
   command = low_byte(cmd);

   chk_status();
   return data;

 } /*end read_com_0 */

 /*;*<*>********************************************************
 *  write_com_0
 *
 * This routine writes the lower byte of 'cmd' as a command to the 
 * LCD controller
 **start*/
 void write_com_0(int cmd)
 {
   chk_status();
   command = low_byte(cmd);
#ifdef WR_DEBUG1
	printf(" wc0: %x\n\r",(uchar)low_byte(cmd));
#endif /* WR_DEBUG */
 } /*end write_com_0 */

 /*;*<*>********************************************************
 *  write_com_1
 *
 * This routine writes the lower byte of 'data1' to the data
 * register followed by the lower byte of 'cmd' as a command
 * to the LCD controller
 **start*/
 void write_com_1(int data1, int cmd)
 {

   chk_status();
   data = low_byte(data1);

   chk_status();
   command = low_byte(cmd);

#ifdef WR_DEBUG1
	printf(" wc1: d=%x c=%x\n\r",
 	(uchar)low_byte(data1),(uchar)low_byte(cmd));
#endif /* WR_DEBUG */

 } /*end write_com_1 */

 /*;*<*>********************************************************
 *  write_com_2
 *
 * This routine writes
 *   the lower byte of data1 to the data register
 *   the upper byte of data1 to the data register
 *   the lower byte of cmd to the command register
 *    of the LCD controller
 **start*/
 void write_com_2(int data1, int cmd)
 {

   chk_status();
   data = low_byte(data1);

   chk_status();
   data = high_byte(data1);

   chk_status();
   command = low_byte(cmd);

#ifdef WR_DEBUG1
	printf("wc2: d=%x %x,c=%x\n\r",
 	(uchar)low_byte(data1),(uchar)high_byte(data1),(uchar)low_byte(cmd));
#endif /* WR_DEBUG */

 } /*end write_com_2 */

 /*;*<*>********************************************************
 *  write_txt
 *
 * This function writes the 'data1' ASCII value to the LCD screen.
 * To do this it translates 'data1' to a value the screen understands.
 *
 * It is assumed that the LCD has been put into a mode to accept
 * the data.
 *
 **start*/
 void write_txt(int data1)
 {
    write_data(data1-ASCII_OFFSET);
 } /*end write_txt */

 /*;*<*>********************************************************
 *  write_data
 *
 * This function writes the 'data1' value to the LCD screen.
 *
 * It is assumed that the LCD has been put into Data Auto Write mode.
 * 
 * The STA3 flag must be checked between writes
 **start*/
 void write_data(int data1)
 {
  chk_rdy_status();
  data = (char) data1;

#ifdef WR_DEBUG1
	printf(" wd:%x ",(uchar)low_byte(data1));
#endif /* WR_DEBUG */

 } /*end write_data */

 /*;*<*>********************************************************
 * auto_reset
 **start*/
 void auto_reset(void)
 {
   chk_rdy_status();
   write_com_0(DATA_AUTO_MODE_RESET_COM);

 } /*end auto_reset*/

 /*;*<*>********************************************************
 * ldd_clr
 *
 * Clears memory in the LCD
 *    LDDE_CLR_ALL - Default - works good
 *    LDDE_CLR_TEXT -  Coded for One screen
 *    LDDE_CLR_GRAPHICS - not tested
 * Needs work for LDDE_CLR_GRAPHICS and LDDE_CLR_TEXT
 **start*/
 void ldd_clr(enum ldde_clr type)
 {
   register int loop,
                start=INIT_1_GRPH_HOME_ADDRESS,
                stop=LCD_MEMORY_SIZE;

/*read_command();*/
/*   auto_reset(); /* Ensure LCD is not doing enything */

   switch(type){
   /*case LDDE_CLR_ALL:  /* Defaults are set up for this case: */
   case LDDE_CLR_GRAPHICS:
      stop=start+LCD_GRAPHICS_MEMORY_SIZE;
      break;
   case LDDE_CLR_TEXT:
      start = INIT_1_TXT_HOME_ADDRESS;
      stop  = (INIT_1_TXT_HOME_ADDRESS+TLX_TEXT_RAM_SIZE);
      break;
   }
/*   write_com_2(INIT_1_GRPH_HOME_ADDRESS/*grph_home_addr*!/,   SET_GRAPHIC_HOME_ADDR_COM);
*/
   write_com_2(start, ADDRESS_POINTER_SET_COM);
   write_com_0(DATA_AUTO_WRITE_COM);
#ifndef TLX_2_SCREEN
   /*
    * the following fills all memory with zeros
    * - this is assumed to clear graphics/text memory
    */
   for (loop=start;loop < stop; loop++) {
      write_data(0);
   }

#endif /* TLX_2_SCREEN */
#ifdef TLX_2_SCREEN
   for (loop=0;loop < LCD_MEMORY_SIZE_SCREEN1; loop++) {
      write_data(0);
   }

   /*
    * Deal with screen two
    */
   auto_reset();
   write_com_2(INIT_1_GRPH_HOME_ADDRESS/*grph_home_addr*/+TLX_GA_SCREEN2_OFFSET, ADDRESS_POINTER_SET_COM);
   write_com_0(DATA_AUTO_WRITE_COM);
   for (loop=0;loop < LCD_MEMORY_SIZE_SCREEN2; loop++) {
      write_data(0);
   }
#endif /* TLX_2_SCREEN */
   auto_reset();

 } /*end ldd_clr*/

 /*;*<*>********************************************************
 * gen_mask_pos
 *
 * Takes two numbers and generates a mask that has 
 *    num_bits set starting at bit bit_num and then has
 *    bits set in the nwgative direction.
 *
 *  Inputs:
 *      bit_num  Range:0->15
 *      num_bits Range:1->16
 *    limitations bit_num+num_bits should not be greater than 16
 **start*/
 int gen_mask_pos(int bit_num,int num_bits)
 {
  register int mask=0, temp, loop;

    temp = (1 << bit_num);
    for (loop = 0; loop < num_bits; loop++) {
       mask |= temp;
       temp >>= 1;
    }
    return mask;
 } /*end gen_mask_pos*/

 /*;*<*>********************************************************
 * gen_mask_neg
 *
 * Takes two numbers and generates a mask that has 
 *    num_bits set starting at bit bit_num, and then bits
 *   are set in the positive direction.
 *
 *  Inputs:
 *      bit_num  Range:0->15
 *      num_bits Range: 1 -> 16
 *    limitations bit_num+num_bits should not be greater than 16
 **start*/
 int gen_mask_neg(int bit_num,int num_bits)
 {
  register int mask=0, temp, loop;

    temp = (1 << bit_num);
    for (loop = 0; loop > num_bits; loop--) {
       mask |= temp;
       temp <<= 1;
    }
    return mask;
 } /*end gen_mask_neg*/

 /*;*<*>********************************************************
 * gen_mask
 **start*/
 int gen_mask(int bit_num, int *num_bitsp, int num_req)
 {
   if (num_req >= 0) {
      if (num_req < ++*num_bitsp) {
         *num_bitsp = num_req;
      }
      return gen_mask_pos(bit_num,*num_bitsp);
   } else {
      *num_bitsp -= 8;  /* Make -1 to -8 */
      if (num_req > *num_bitsp) {
         *num_bitsp = num_req;
      }
      return gen_mask_neg(bit_num,*num_bitsp);
   }

 } /*end gen_mask*/

 /*;*<*>********************************************************
 * 
 **start*/
/* ()
 {

 } /*end*/





