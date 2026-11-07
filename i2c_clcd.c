/*
 * i2c_clcd.c
 *
 *  Created on: 2024/02/17
 *      Author: 700650
 *  I2C LCD Controller assuming PCF85741
 */
/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "i2c_utils.h"
#include "i2c_clcd.h"

#define CLCD_INITIAL_WAIT 50

#define BIT_BL 0x08 // Backlight Control
#define BIT_EN 0x04 // Enable bit
#define BIT_RW 0x02 // Read/Write bit
#define BIT_RS 0x01 // Register Select bit


#define I2C_CLCD_I2C_ADDRESS 0x27

uint8_t back_light = BIT_BL ;
static bool i2c_initialized = false;

void I2C_CLCD_Init(void)
{
    CyDelay(CLCD_INITIAL_WAIT) ;

    if (!i2c_initialized) {
        if (CY_RSLT_SUCCESS != i2c_init()) {
            printf("I2C initialization failed!\n\r") ;
            return;
        }
        i2c_initialized = true;
    }


    CyDelay(100) ;

    i2c_set_slave_address(I2C_CLCD_I2C_ADDRESS) ;

    I2C_CLCD_WriteCommand4(0x30) ; /* Function mode 8bit mode */
    CyDelay(5) ; /* >= 4.1ms */

    I2C_CLCD_WriteCommand4(0x30) ; /* Function mode 8bit mode */
    CyDelayUs(100) ; /* >= 100us */

    I2C_CLCD_WriteCommand4(0x30) ; /* Funciton mode 8bit mode */

    I2C_CLCD_WriteCommand4(0x20) ; /* Function mode 4bit mode */

    I2C_CLCD_WriteCommand(0x28) ; /* 4 bits, 2 lineas, caracteres 5x8 */

    I2C_CLCD_WriteCommand(0x0C) ; /* Display ON, cursor y parpadeo OFF */

    I2C_CLCD_WriteCommand(0x01) ; /* Display Clear */

    I2C_CLCD_WriteCommand(0x02) ; /* Cursor Home */

    I2C_CLCD_WriteCommand(0x06) ; /* increment, no shift */

    I2C_CLCD_SetDDRAMAddr(0x00) ;

    I2C_CLCD_BackLight(0x01) ;    /* Backlight ON */
}

void I2C_CLCD_Start(void)
{
	 printf("aca");
}

void I2C_CLCD_BackLight(uint8_t mode)
{
	if (mode == 0) {
		back_light = 0 ;
	} else {
		back_light = BIT_BL ;
	}
}

void I2C_CLCD_EnablePulse(uint8_t data)
{
    i2c_write_byte(data | BIT_EN) ;
    CyDelayUs(1) ;
    i2c_write_byte(data & ~BIT_EN) ;
    CyDelayUs(50) ;
}

void I2C_CLCD_Write4(uint8_t data)
{
//    snprintf(str, STR_BUF_LEN, "%X\n", (data >> 4) & 0x0F) ;
//    print(str) ;
    i2c_set_slave_address(I2C_CLCD_I2C_ADDRESS) ;
    data |= back_light ;
    i2c_write_byte(data) ;
    I2C_CLCD_EnablePulse(data) ;
}

void I2C_CLCD_WriteCommand4(uint8_t cmd)
{
    uint8_t data ;
//    snprintf(str, STR_BUF_LEN, "command4: %02X\n", cmd) ;
//    print(str) ;

    data = (cmd & 0xF0) | BIT_EN ; /* EN = 1, RS = 0, RW = 0 */
    i2c_write_byte(data) ;
    CyDelayUs(450) ;

    data = (cmd & 0xF0) ; /* EN = 0, RS = 0, RW = 8 */
    i2c_write_byte(data) ;
    CyDelayUs(20) ;

    CyDelayUs(800) ;
}

void I2C_CLCD_WriteCommand(uint8_t cmd)
{
    uint8_t data ;
//    snprintf(str, STR_BUF_LEN, "command8: %02X\n", cmd) ;
//    print(str) ;
    data = (cmd & 0xF0) | BIT_EN ; /* EN = 1, RS = 0, RW = 0 */
    i2c_write_byte(data) ;
    CyDelayUs(450) ;

    data = (cmd & 0xF0) ; /* EN = 0, RS = 0, RW = 0 */
    i2c_write_byte(data) ;
    CyDelayUs(20) ;

    CyDelayUs(800) ;

    data = ((cmd << 4) & 0xF0) | BIT_EN ; /* EN = 1, RS = 0, RW = 0 */
    i2c_write_byte(data) ;
    CyDelayUs(450) ;

    data = ((cmd << 4) & 0xF0) ; /* EN = 0, RS = 0, RW = 0 */
    i2c_write_byte(data) ;

    CyDelay(1) ;
}

void I2C_CLCD_WriteData(uint8_t value)
{
    uint8_t data ;
//    sprintf(str, STR_BUF_LEN, "data: %02X\n", data) ;
//    print(str) ;
    data = (value & 0xF0) | back_light | BIT_RS | BIT_EN ; /* EN = 1, RS = 1, RW = 0 */
    i2c_write_byte(data) ;
    CyDelayUs(450) ;

    data = (value & 0xF0) | back_light |  BIT_RS ; /* EN = 0, RS = 1, RW = 0 */
    i2c_write_byte(data) ;
    CyDelayUs(20) ;

    CyDelayUs(800) ;

    data = ((value << 4) & 0xF0) | back_light | BIT_RS | BIT_EN ; /* EN = 1, RS = 1, RW = 0 */
    i2c_write_byte(data) ;
    CyDelayUs(450) ;

    data = ((value << 4) & 0xF0) | back_light | BIT_RS ; /* EN = 0, RS = 1, RW = 0 */
    i2c_write_byte(data) ;

    CyDelay(1) ;
}

int  I2C_CLCD_IsBusy(void)
{
    int result = 0 ;
    /* read DB7... can we ? */
    return( result ) ;
}

void I2C_CLCD_Clear(void)
{
    I2C_CLCD_WriteCommand(0x01) ;
}

void I2C_CLCD_Home(void)
{
    I2C_CLCD_WriteCommand(0x03) ;
}

void I2C_CLCD_Display(uint8_t mode)
{
    I2C_CLCD_WriteCommand( 0x08 | (mode & 0x07)) ;
}

void I2C_CLCD_SetFunction(uint8_t mode)
{
    I2C_CLCD_WriteCommand(0x20 | (mode & 0x1C)) ;
}

void I2C_CLCD_SetCGRAMAddr(uint8_t addr)
{
    I2C_CLCD_WriteCommand(0x40 | (addr & 0x3F)) ;
}

void I2C_CLCD_SetDDRAMAddr(uint8_t addr)
{
    I2C_CLCD_WriteCommand(0x80 | (addr & 0x7F)) ;
}

void I2C_CLCD_PutChar(uint8_t c)
{
    I2C_CLCD_WriteData(c) ;
}

void I2C_CLCD_PutString(char *str)
{
    uint8_t *c = (uint8_t *)str ;

    while(c && *c) {
        I2C_CLCD_WriteData(*c++) ;
        CyDelay(2) ;
    }
}

void I2C_CLCD_Pos(int line, int col)
{
    int addr ;
    if (line < 1) {
        line = 1 ;
    }
    if (line > I2C_CLCD_NUM_ROWS) {
        line = I2C_CLCD_NUM_ROWS ;
    }
    if (col < 1) {
        col = 1 ;
    }
    if (col > I2C_CLCD_NUM_COLS) {
        col = I2C_CLCD_NUM_COLS ;
    }
    addr = 0x40 * (line-1) + (col-1) ;
    I2C_CLCD_SetDDRAMAddr(addr) ;
}

