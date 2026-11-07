/*
 * i2c_utils.h
 *
 *  Created on: 2024/02/17
 *      Author: 700650
 */

#ifndef I2C_UTILS_H_
#define I2C_UTILS_H_

#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

/* I2C slave address to communicate with */


extern cyhal_i2c_t     mI2C;
extern cyhal_i2c_cfg_t mI2C_cfg;
extern uint8_t         i2c_slave_address ;

cy_rslt_t i2c_init(void) ;

void i2c_set_slave_address(uint8_t address) ;

cy_rslt_t i2c_write_byte(uint8_t data) ;

cy_rslt_t i2c_write_bytes(uint8_t *data, uint16_t len) ;

cy_rslt_t i2c_read_byte(uint8_t *data) ;

cy_rslt_t i2c_read_bytes(uint8_t *data, uint16_t len) ;


#endif /* I2C_UTILS_H_ */
