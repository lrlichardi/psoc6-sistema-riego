/*
 * i2c_utils.c
 *
 *  Created on: 2024/02/17
 *      Author: 700650
 */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "i2c_utils.h"

cyhal_i2c_t     mI2C;
cyhal_i2c_cfg_t mI2C_cfg;
uint8_t         i2c_slave_address ;

cy_rslt_t i2c_init(void) {
	cy_rslt_t result ;
	/* I2C Master configuration settings */
	printf(">> Configuring I2C Masterr..... ");
	mI2C_cfg.is_slave = false;
	mI2C_cfg.address = 0;
	/* 100 kHz mejora la inmunidad al ruido con cables y cargas inductivas. */
	mI2C_cfg.frequencyhal_hz = 100000UL;

	/* Init I2C master */
	result = cyhal_i2c_init(&mI2C, CYBSP_I2C_SDA, CYBSP_I2C_SCL, NULL);
    if (result != CY_RSLT_SUCCESS) {
    	return( result ) ;
    }

	/* Configure I2C Master */
	result = cyhal_i2c_configure(&mI2C, &mI2C_cfg);
    if (result != CY_RSLT_SUCCESS) {
    	return( result ) ;
    }

	printf("Done \r\n\n");
	return( result ) ;
}

void i2c_set_slave_address(uint8_t address)
{

	i2c_slave_address = address ;
}

cy_rslt_t i2c_write_byte(uint8_t data)
{
	cy_rslt_t result ;
	result = cyhal_i2c_master_write(
			&mI2C, i2c_slave_address,
	        &data, 1, 0, true);
	return( result ) ;
}

cy_rslt_t i2c_write_bytes(uint8_t data[], uint16_t len)
{
	cy_rslt_t result ;
	result = cyhal_i2c_master_write(
			&mI2C, i2c_slave_address,
	        data, len, 0, true);
	return( result ) ;
}

cy_rslt_t i2c_read_byte(uint8_t *data)
{
	cy_rslt_t result ;
	result = cyhal_i2c_master_read(
			&mI2C, i2c_slave_address,
			data, 1, 0, true)	;
	return( result ) ;
}

cy_rslt_t i2c_read_bytes(uint8_t data[], uint16_t len)
{
	cy_rslt_t result ;
	result = cyhal_i2c_master_read(
			&mI2C, i2c_slave_address,
			data, len, 0, true)	;
	return( result ) ;
}
