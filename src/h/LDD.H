/*
 * ldd.h
 *
 * Includes specific to logical device driver 
 *
 * Note: This file is used in the MMIR and DVSTY builds and must be
 * kept in sync
 *
 * Version Date
 *   0.1   22/7/92   Orginal
 *
 * Constants and enums
 */
/* enum ldde_physical_screen_type {PHY_SCRN_128BY128=1,PHY_SCRN_126BY128=1,
  PHY_SCRN_640BY200}; */
 enum ldde_clr {LDDE_CLR_ALL,LDDE_CLR_GRAPHICS,LDDE_CLR_TEXT};
/* enum ldde_type {LDDE_BIT=1,LDDE_2BIT,LDDE_3BIT,LDDE_4BIT,
		LDDE_5BIT,LDDE_6BIT,LDDE_7BIT,LDDE_BYTE};
*/
 enum ldde_type {LDDE_BIT=1,LDDE_2BIT,LDDE_3BIT,LDDE_4BIT,
		LDDE_5BIT,LDDE_6BIT,LDDE_7BIT,LDDE_BYTE,
                /*LDDE_H2BIT=129,LDDE_H3BIT,LDDE_H4BIT,
		LDDE_H5BIT,LDDE_H6BIT,LDDE_H7BIT,LDDE_HBYTE,
                */
                LDDE_TYPE_END};
enum ldde_scrn {LDDE_EDGES_ON,LDDE_EDGES_OFF};
/************************************************
 * Logo
 */
 typedef struct {
      int size;    /* Size in bytes of *datap */
      char *datap; /* Pointer to const char string of information to be sent*/
      int start;   /* Starting address of 1st byte
          * eg 0
          * For 12th line down this would be 12 * ( 8 * LINE_LENGTH)
          *  where LINE_LENGTH is number char on a line eg TLX1391=16,TLX1013=20
          */
 } Logo;

#ifdef TLX_1391 /************************/
#define TLX_DOTS_PER_CHAR 8
#define TLX_GRPH_LN_LNGTH 0x10
#define TLX_SCREEN_X 128
#define TLX_SCREEN_Y 128
          /*  Definition of RAM allocation in TLX_1391
           *
           *  0      - 0x13FF Graphic Ram Area (5120 bytes)
           *  0x1400 - 0x16ff Attribute Ram
           *  0x1700 - 0x1bff Text Ram 
           *  0x1c00 - 0x1fff CG RAM (1024 bytes)
           */
#define INIT_1_GRPH_HOME_ADDRESS 0
#define INIT_1_TXT_HOME_ADDRESS 0x1700
#define TLX_TEXT_RAM_SIZE 0x500
#endif /*TLX_1391 */

#ifdef TLX_1013 /************************/
#define TLX_DOTS_PER_CHAR 8
#define TLX_GRPH_LN_LNGTH 0x14 
#define TLX_SCREEN_X 160
#define TLX_SCREEN_Y 128
          /* Definition of RAM allocation in TLX_1013
           *  Upper Half Screen
           *
           *  0      -  0x7FF Graphic Ram Area (2048 bytes)
           *  0x800  -  0x9ff Attribute Ram
           *  0xA00  -  0xbff Text Ram
           *  0xc00  -  0xfff CG RAM (1024 bytes)
           *
           *  Lower Half Screen
           *  0x8000 - 0x87FF Graphic Ram Area (2048 bytes)
           *  0x8800 - 0x89ff Attribute Ram
           *  0x8A00 - 0x8bff Text Ram
           *  0x8c00 - 0x8fff  CG RAM (1024 bytes)
           */
#define INIT_1_GRPH_HOME_ADDRESS 0
#define INIT_1_TXT_HOME_ADDRESS 0x0a00
#define TLX_2_SCREEN   /* ENABLE TWO SCREENS */
#define LCD_MEMORY_SIZE_SCREEN1 0x1000
#define LCD_MEMORY_SIZE_SCREEN2 0x1000
#define LCD_MEMORY_SIZE         0x2000 /* 8Kbytes */
#define TLX_GA_SIZE_SCREEN1 (0x14*64)
#define TLX_GA_SCREEN2_OFFSET 0x8000
#define TLX_TEXT_RAM_SIZE 0x400 /* Initial calculation */

#endif /*TLX_1013 */

/* Command Descriptions for Toshiba TLX-LCDs pg 18 */
  /* Pointer Set Command - Sect 4.2.2.1 */
#define CURSOR_POINTER_SET_COM 0x21
#define OFFSET_REGISTER_SET_COM 0x22
#define ADDRESS_POINTER_SET_COM 0x24
  /* Control Word Set Command - Sect 4.2.2.2 */
#define SET_TXT_HOME_ADDR_COM    0x40
#define SET_TXT_AREA_COM         0x41
#define SET_GRAPHIC_HOME_ADDR_COM 0x42
#define SET_GRAPHIC_AREA_COM     0x43
  /* Mode Set Command - Sect 4.2.2.3 */
#define SET_MODE_ROM_OR_COM    0x80
#define SET_MODE_ROM_EXOR_COM  0x81
#define SET_MODE_ROM_AND_COM   0x83
#define SET_MODE_ROM_TXT_COM   0x84
#define SET_MODE_RAM_OR_COM    0x88
#define SET_MODE_RAM_EXOR_COM  0x89
#define SET_MODE_RAM_AND_COM   0x8B
#define SET_MODE_RAM_TXT_COM   0x8C
  /* Display Mode Set Command - Sect 4.2.2.4 */
  /* use in form (SET_DISPLAY_MODE_COM|SET_xxxx) */
#define SET_DISPLAY_MODE_COM     0x90
#define SET_CURSOL_BLINK_ON      0x1
#define SET_CURSOL_DISPLAY_ON    0x2
#define SET_TXT_DISPLAY_ON       0x4
#define SET_GRAPHIC_DISPLAY_ON   0x8
#define DISPLAY_COM (SET_DISPLAY_MODE_COM|SET_GRAPHIC_DISPLAY_ON|SET_TXT_DISPLAY_ON)
  /* Cursor Pattern Select Command - Sect 4.2.2.5 */
#define CURSOR_PATTERN_SELECT_COM 0xa0
#define INIT_CURSOR_PATTERN_SELECT_COM (CURSOR_PATTERN_SELECT_COM|4)
  /* Data Auto Write/Read - Sect 4.2.2.6 */
#define DATA_AUTO_WRITE_COM      0xB0
#define DATA_AUTO_READ_COM       0xB1
#define DATA_AUTO_MODE_RESET_COM 0xB2
  /* Data Write/Read - sect 4.2.2.7 */
#define DATA_BYTE_WRITE_INC_COM  0xC0
#define DATA_BYTE_READ_INC_COM   0xC1
#define DATA_BYTE_WRITE_DECR_COM 0xC2
#define DATA_BYTE_READ_DECR_COM  0xC3
#define DATA_BYTE_WRITE_COM      0xC4
#define DATA_BYTE_READ_COM       0xC5
   /* Screen Peek - sect 4.2.2.8 */
#define SCREEN_PEEK_COM 0xe0
   /* Screen Copy - sect 4.2.2.9 */
#define SCREEN_COPY_COM 0xe8
   /* Bit set/reset command - sect 4.2.2.10
   /* Use as (BIT_SET_RESET_COM|bit_no) */
#define BIT_SET_RESET_COM 0xf0
#define BIT_SET_COM       0xf1
#define BIT_RESET_COM     0xf0

/*
 *  Custom Character defines
 */
#define LDD_CHAR1 0x80
#define LDD_CHAR2 0x81
/*
 * Structures
 */
 typedef struct {
  /*  enum inite_virtual_screen_type virtual_screen_type; */
  /*  enum ldde_physical_screen_type physical_screen_type; */
   Pixels pixels;
 } Ldd_defs;


 /* ldd_tlx.c specific prototypes */
 void ldd_scrn_test(enum ldde_scrn test);
 void ldd_determine_scrn(Ldd_defs *defsp);
 enum inite_return ldd_init_device(Ldd_defs *defsp);
 int ldd_putchar(int data1);
 void ldd_putchar_set_cur(int txt_window_start, int txt_window_size);
 void ldd_wr_ascii_char(int x_pos, int y_pos,int data1);
 void ldd_wrchar(int x_pos, int y_pos,int data1);
 void ldd_line(uint line_num,uchar *string);
 /* void ldd_line_xy(uint line_num,uint char_num, uchar byte);*/
 void ldd_logo(Logo *logo_structp);
 void ldd_clr(enum ldde_clr type);

 Byte read_com_0(int cmd);
 void write_com_0(int cmd);
 void write_com_1(int data1, int cmd);
 void write_com_2(int data1, int cmd);
 void write_data(int data1);
 void dbg_wr(char *string);

 int ldd_graphics_bargraph(unsigned x_pos, unsigned y_pos,
      unsigned oldvalue,int diff);
 int ldd_graphics_write(
      enum ldde_type type, Pixels *cordp,Byte value);
 int  ldd_graphics_set(enum ldde_type type, int x,int y, int num_req);
 int ldd_graphics_clr(enum ldde_type type, int x,int y, int num_req);
/* void ldd_text_write(void);*/
 void ldd_status(char *string);





