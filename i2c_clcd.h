/*
 * i2c_clcd.h
 *
 *  Created on: 2024/02/17
 *      Author: 700650
 */

#ifndef I2C_CLCD_H_
#define I2C_CLCD_H_

#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "i2c_utils.h"
#include "i2c_clcd.h"

#define I2C_CLCD_NUM_ROWS 2
#define I2C_CLCD_NUM_COLS 16

void I2C_CLCD_Init(void) ;
void I2C_CLCD_Start(void) ;

void I2C_CLCD_Write4(uint8_t data) ;
void I2C_CLCD_WriteCommand4(uint8_t cmd) ;
void I2C_CLCD_WriteCommand(uint8_t cmd) ;
void I2C_CLCD_WriteData(uint8_t cmd) ;
int  I2C_CLCD_IsBusy(void) ;
void I2C_CLCD_Clear(void) ;
void I2C_CLCD_Home(void) ;
void I2C_CLCD_Display(uint8_t mode) ;
void I2C_CLCD_SetFunction(uint8_t mode) ;
void I2C_CLCD_SetCGRAMAddr(uint8_t addr) ;
void I2C_CLCD_SetDDRAMAddr(uint8_t addr) ;

void I2C_CLCD_BackLight(uint8_t mode) ;
void I2C_CLCD_PutChar(uint8_t c) ;
void I2C_CLCD_PutString(char *str) ;
void I2C_CLCD_Pos(int line, int col) ;

#endif /* I2C_CLCD_H_ */
