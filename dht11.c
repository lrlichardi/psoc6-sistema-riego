

#include "dht11.h"
#include "cy_retarget_io.h"
#include "cyhal.h"
#include "cybsp.h"

#define DATA_PIN 	P9_7
#define DATA_PORT   P9_7_PORT
#define DATA_NUM    P9_7_PIN
#define SUCCESS 				0x00
#define DHT_NO_RESPONSE_LOW 	0x11
#define DHT_NO_RESPONSE_HIGH 	0x12
#define DHT_RESPONSE_STUCK_HIGH	0x13
#define DHT_BIT_STUCK_LOW 	    0x14
#define DHT_BIT_STUCK_HIGH 	    0x15
#define DHT_INCORRECT_VALUE		0x02


/* Structure to store temperature and humidity readings */

struct readings DHT_reading = {0, 0};

/*******************************************************************************
* Function Name: Fraction_Convert
****************************************************************************//**
*
* Converts a 8 bit binary number into a fractional decimal value
*
* \param num
* 8 bit binary value
*
* \return
* Fractional decimal value of the binary number
*
*******************************************************************************/
float Fraction_Convert(uint8_t num)
{
    float fraction = 0;
    int unit = 0;
    for( int i = 0; i<8; i++)
    {
        unit = num & 1;
        num = num>>1;
        fraction = fraction + unit * pow(2, -(1+i));
    }
    return fraction;
}

static uint16_t DHT_WaitForLevel(uint8_t level, uint16_t timeout)
{
    uint16_t count = 0;

    while (Cy_GPIO_Read(DATA_PORT, DATA_NUM) != level)
    {
        cyhal_system_delay_us(1);
        count++;
        if (count >= timeout)
        {
            break;
        }
    }

    return count;
}

static uint16_t DHT_WaitWhileLevel(uint8_t level, uint16_t timeout)
{
    uint16_t count = 0;

    while (Cy_GPIO_Read(DATA_PORT, DATA_NUM) == level)
    {
        cyhal_system_delay_us(1);
        count++;
        if (count >= timeout)
        {
            break;
        }
    }

    return count;
}

/*******************************************************************************
* Function Name: DHT_Start
****************************************************************************//**
*
* Initiates the communication with the sensor.
* Function sends 18ms low signal after 1s delay.
*
*******************************************************************************/

void DHT_Start(void)
{
    Cy_GPIO_SetDrivemode(DATA_PORT, DATA_NUM, CY_GPIO_DM_OD_DRIVESLOW);
    Cy_GPIO_Write(DATA_PORT, DATA_NUM, 1);
    CyDelay(1000);
    Cy_GPIO_Write(DATA_PORT, DATA_NUM, 0);
    cyhal_system_delay_ms(20);
    Cy_GPIO_Write(DATA_PORT, DATA_NUM, 1);
}

/*******************************************************************************
* Function Name: DHT_Read
****************************************************************************//**
*
* Reads the temperature and humidity values if the sensor read is successful
*
* \param humidity
* Pointer to the float variable which stores humidity value
*
* \param temperature
* Pointer to the float variable which stores temperature value
*
* \return
* Status of sensor read
*
*******************************************************************************/

uint8 DHT_Read(float* humidity, float* temperature)
{
    uint16_t ack_low_time = 0, high_time = 0;
    uint8_t temp = 0, index = 0, bit_count = 7;
    uint8_t result = SUCCESS;
    uint8_t byteval[5] = {0,0,0,0,0};   /* Array to store the 5 byte values received from the sensor */


    DHT_Start();
    __disable_irq();
    /****************************************************************************
    *   Response to the start condition is a 54us low signal and 80us high signal.
    ****************************************************************************/

    if (DHT_WaitForLevel(0, timeout_duration) >= timeout_duration)
    {
        result = DHT_NO_RESPONSE_LOW;
        goto cleanup;
    }

    ack_low_time = DHT_WaitWhileLevel(0, timeout_duration);
    if (ack_low_time >= timeout_duration)
    {
        result = DHT_NO_RESPONSE_HIGH;
        goto cleanup;
    }

    if (DHT_WaitWhileLevel(1, timeout_duration) >= timeout_duration)
    {
        result = DHT_RESPONSE_STUCK_HIGH;
        goto cleanup;
    }

    for (int i = 0; i < 40; i++)
	{
        if (DHT_WaitForLevel(1, timeout_duration) >= timeout_duration)
        {
            result = DHT_BIT_STUCK_LOW;
            goto cleanup;
        }

        high_time = DHT_WaitWhileLevel(1, timeout_duration);
        if (high_time >= timeout_duration)
        {
            result = DHT_BIT_STUCK_HIGH;
            goto cleanup;
        }

        /****************************************************************************
        *  The sensor MCU outputs a low signal for 54us and high signal for 24us if
        *  bit value is 0 and a low signal for 54us and high signal for 70us if bit
        *  value is 1.
        ****************************************************************************/

        /* DHT11: compara el alto del bit contra la mitad del ACK bajo. */
        if (high_time > (ack_low_time / 2))
            byteval[index] |= (1 << bit_count);
        if (bit_count == 0)   /* Next Byte */
   	    {
   		    bit_count = 7;    /* Reset bit_count to 7 */
   		    index++;          /* Increment index so that next byte is chosen */
   	    }
   	    else bit_count--;
    }


    /* Checksum is calculated by adding all 4 bytes */
    temp = (byteval[0] + byteval[1] + byteval[2] + byteval[3]);
    if((temp == byteval[4]) && (byteval[4] != 0))
    {
        /* Pass the temperature and humidity data only when checksum is matched and it is not equal to 0 */
        *humidity = (int)byteval[0] + Fraction_Convert(byteval[1]);
        *temperature = (int)byteval[2] + Fraction_Convert(byteval[3]);

        result = SUCCESS;
    }
    else
    {

    	/* Incorrect value */
        result = DHT_INCORRECT_VALUE;
    }

cleanup:
    __enable_irq();
    return result;
}

/*******************************************************************************
* Function Name: DHT_Task
****************************************************************************//**
*
* Task function to read the sensor values and pass the values into a queue
*
* \param pvParameters
* Void pointer which points to the queue handle
*
*******************************************************************************/

void DHT_Task()
{

		DHT_reading.result_code = DHT_Read(&DHT_reading.humidity, &DHT_reading.temperature);

}




