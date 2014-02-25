#define HW_DIG_REV4
/*
  hw_mem.h

  Defines the hardware memory map
*/
#ifndef hw_map_h
#define hw_map_h

/*
 *
 * Address of LCD port
 */
#define TLX_COMMAND_PORT 0x8002 /* rev2,rev3, & rev4 digital HW */
#define TLX_DATA_PORT    0x8000 /* rev2,rev3, & rev4 digital HW */

/*
 * Expected address of Analog Board I/O registers 
 * If changing then linker file needs changing
 */
#if defined(HW_DIG_REV3)
#define AG_BRD_REG 0x08800 /* rev2,rev3 digital HW */
/* Note: the locate command uses these doesn't like to add */
#define  LAT_DAC  0x8800   /*AG_BRD_REG+0  U611/rev2 */
#define  LAT_COM  0x8808   /*AG_BRD_REG+4   */
#define  LAT_L    0x8810   /*AG_BRD_REG+0xC */
#define  LAT_R    0x8818   /*AG_BRD_REG+8   */
#define  CLK_COM  0x8820   /*AG_BRD_REG+0x10*/
#define  LAT_TEST 0x8830   /*AG_BRD_REG+0x20*/
#define  CLK_TEST 0x8838   /*AG_BRD_REG+0x30*/

#elif defined(HW_DIG_REV4)

#define AG_BRD_REG 0x0840 /* rev4 digital HW */
/* A7=0 A6=1 */
/* Note: the locate command uses these doesn't like to add */
#define  LAT_DAC  0x8040   /*AG_BRD_REG+0  U611/rev2 */
#define  LAT_COM  0x8048   /*AG_BRD_REG+4   */
#define  LAT_L    0x8050   /*AG_BRD_REG+0xC */
#define  LAT_R    0x8058   /*AG_BRD_REG+8   */
#define  CLK_COM  0x8060   /*AG_BRD_REG+0x10*/
#define  ANLG_NC1 0x8068   /* not used */
#define  LAT_TEST 0x8070   /*AG_BRD_REG+0x20*/
#define  CLK_TEST 0x8078   /*AG_BRD_REG+0x30*/
#endif  /* HW_DIG_REV4 */

/*
 * old registers for Analog rev1 hardware 
 *
!#define LAT_DAC  (AG_BRD_REG+0) 
!#define LAT_COM  (AG_BRD_REG+4)
!#define LAT_L    (AG_BRD_REG+8)
!#define LAT_R    (AG_BRD_REG+0xc0)
!#define CLK_COM  (AG_BRD_REG+0x10)
!#define CLK_TEST (AG_BRD_REG+0x20)
!#define LAT_TEST (AG_BRD_REG+0x30) 
*/

/*
 * Addresses of RAM
 *
 */
#define NB_RAM_SIZE 0x2000       /* 8K NB Ram */
#define LOWER_NB_RAM_ADDR 0xa000 /* rev2 HW */
#define UPPER_NB_RAM_ADDR (LOWER_NB_RAM_ADDR+NB_RAM_SIZE-1) /* rev2 HW */

#define BANKED_RAM_SIZE 0x4000       /* 16K Banked Ram */
#define LOWER_BANKED_RAM_ADDR 0xC000 /* rev2 HW */
#define UPPER_BANKED_RAM_ADDR (LOWER_BANKED_RAM_ADDR+BANKED_RAM_SIZE-1)
/* If LOWER_BANKED_RAM_ADDR is changed then it must be modified
 * in drm_ram.c as well
 */
/*************************************************
 * Switched Bank Numbers
 */
 enum ram_bank_number {RAM_BANK_0,RAM_BANK_1,RAM_BANK_2,RAM_BANK_3,
                       RAM_BANK_4,RAM_BANK_5,RAM_BANK_6,RAM_BANK_7,
                       RAM_BANK_END};

#endif /* hw_mem_h */



