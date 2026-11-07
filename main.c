/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "i2c_utils.h"
#include "i2c_clcd.h"
#include "dht11.h"
#define DATA_PIN 	P9_7 //Pin DHT11
#define BUZZER_PIN 	P9_1 // Pin de buzzer
#define BOMBA_PIN 	P9_4 // Pin del rele de la bomba
#define SUELO_PIN 	P9_6 // Salida digital DO del sensor de humedad de suelo
#define NIVEL_AGUA_PIN P10_0 // Entrada ADC para la salida analogica S del sensor de nivel
#define BOMBA_ON 	0    // Modulo relay Arduino normalmente activo en LOW
#define BOMBA_OFF 	1
#define SUELO_SECO_LEVEL 1 // En la mayoria de los modulos LM393, HIGH indica suelo seco
#define LECTURAS_SECO_PARA_REGAR 2 // Filtra cambios breves del comparador
#define TEMP_MIN_RIEGO 20.0f // El DHT11 habilita el riego por encima de esta temperatura
#define NIVEL_AGUA_BAJO_UV 900000UL // 0.9 V: corta la bomba antes de vaciar el deposito
#define NIVEL_AGUA_OK_UV 1200000UL // 1.2 V: nivel necesario para volver a habilitarla
#define MUESTRAS_NIVEL_AGUA 8U // Promedio para reducir ruido en la lectura
#define TEST_PIN_DHT 0 // Test temporal: deja el pin del DHT en alto para medir con tester
#define TEST_RELE 0 // Test temporal: prende y apaga el rele de la bomba
#define TEST_RELE_DELAY_MS 5000
#define DHT_READ_DELAY_MS 2000
#define BOMBA_TIEMPO_ON_MS 3000U // Pulso de riego
#define BOMBA_ESPERA_MS 5000U // Tiempo para que el agua penetre antes de volver a medir
#define BOMBA_NIVEL_CHECK_MS 100U // Control de seguridad mientras la bomba esta encendida
#define LCD_RECOVERY_DELAY_MS 150U // Espera a que desaparezca el ruido del rele y la bomba
#define PWM_FREQUENCY 3000 // frecuencia del PWM
#define BEEP_DURATION 500  // Duración del pitido en milisegundos
/*******************************************************************************
* Macros
*******************************************************************************/
/* Delay of 1000ms between commands */
#define CMD_TO_CMD_DELAY        (1000UL)

/* Packet positions */
#define PACKET_SOP_POS          (0UL)
#define PACKET_CMD_POS          (1UL)
#define PACKET_EOP_POS          (2UL)

/* Start and end of packet markers */
#define PACKET_SOP              (0x01UL)
#define PACKET_EOP              (0x17UL)

/* I2C slave address to communicate with */
#define I2C_SLAVE_ADDR          (0x3FUL)

/* I2C bus frequency */
#define I2C_FREQ                (100000UL)

/* Command valid status */
#define STATUS_CMD_DONE         (0x00UL)

/* Packet size */
#define PACKET_SIZE             (3UL)

/* Read sensor every 1000 ms */
#define READ_FREQ 1000

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* This enables RTOS aware debugging. */
volatile int uxTopUsedPriority;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/


/*******************************************************************************
* Function Definitions
*******************************************************************************/



void handle_error(uint32_t status)
{
    if (status != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }
}

static uint32_t leer_nivel_agua_uv(cyhal_adc_channel_t *canal)
{
    uint32_t suma = 0;

    for (uint8_t muestra = 0; muestra < MUESTRAS_NIVEL_AGUA; muestra++)
    {
        int32_t lectura_uv = cyhal_adc_read_uv(canal);
        if (lectura_uv > 0)
        {
            suma += (uint32_t)lectura_uv;
        }
    }

    return suma / MUESTRAS_NIVEL_AGUA;
}


/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function.
*   1. I2C Master sends command packet to the slave
*   2. I2C Master reads the response packet to generate the next command
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/



int main(void)
{

	char linea1[17];
	char linea2[17];
	/*inicio del pwm */
	cyhal_pwm_t pwm;
	cyhal_adc_t adc_agua;
	cyhal_adc_channel_t adc_agua_channel;
	const cyhal_adc_config_t adc_agua_config =
	{
		.continuous_scanning = false,
		.resolution = 12,
		.average_count = 1,
		.average_mode_flags = CYHAL_ADC_AVG_MODE_AVERAGE,
		.ext_vref_mv = 0,
		.vneg = CYHAL_ADC_VNEG_VSSA,
		.vref = CYHAL_ADC_REF_VDDA,
		.ext_vref = NC,
		.is_bypassed = false,
		.bypass_pin = NC
	};

    cy_rslt_t result;
    /* uint8_t cmd = CYBSP_LED_STATE_ON; */
    /* uint8_t buffer[PACKET_SIZE]; */

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    /* Board init failed. Stop program execution */
    handle_error(result);

    result = cyhal_pwm_init(&pwm, BUZZER_PIN, NULL);
    handle_error(result);
    result = cyhal_pwm_set_duty_cycle(&pwm, 50 ,PWM_FREQUENCY );
    handle_error(result);

    /* Enable interrupts */
    __enable_irq();

    /* Initialize the retarget-io middleware */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);
    if (result != CY_RSLT_SUCCESS)
	{
		CY_ASSERT(0);
	}

    /* Initialize the DATA pin. The pin used can be changed in main.h file */
    result = cyhal_gpio_init(DATA_PIN, CYHAL_GPIO_DIR_BIDIRECTIONAL, CYHAL_GPIO_DRIVE_OPENDRAINDRIVESLOW, 1);
    if (result != CY_RSLT_SUCCESS)
    {
		CY_ASSERT(0);
	}

#if TEST_PIN_DHT
    cyhal_gpio_configure(DATA_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG);
    cyhal_gpio_write(DATA_PIN, 1);
    I2C_CLCD_Clear();
    I2C_CLCD_PutString("TEST DHT ALTO");
    for (;;)
    {
        CyDelay(1000);
    }
#endif

    /* Inicializa el rele de la bomba apagado. */
    result = cyhal_gpio_init(BOMBA_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, BOMBA_OFF);
    if (result != CY_RSLT_SUCCESS)
	{
		CY_ASSERT(0);
	}

    /* Lee la salida digital DO del modulo comparador de humedad de suelo. */
    result = cyhal_gpio_init(SUELO_PIN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);
    if (result != CY_RSLT_SUCCESS)
	{
		CY_ASSERT(0);
	}

    /* Inicializa P10_0 como entrada analogica para el sensor de nivel de agua. */
    result = cyhal_adc_init(&adc_agua, NIVEL_AGUA_PIN, NULL);
    handle_error(result);
    /* Escala la conversion entre 0 V y VDDA (3.3 V). */
    result = cyhal_adc_configure(&adc_agua, &adc_agua_config);
    handle_error(result);
    result = cyhal_adc_channel_init_diff(&adc_agua_channel, &adc_agua,
                                         NIVEL_AGUA_PIN, CYHAL_ADC_VNEG, NULL);
    handle_error(result);

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
        printf("\x1b[2J\x1b[;H");

        printf("****************** "
               "HAL: I2C Master CharLCD "
               "****************** \r\n\n");

    I2C_CLCD_Init() ;
    I2C_CLCD_PutString("Inicio sistema") ;
    I2C_CLCD_Pos(2 , 1);
    I2C_CLCD_PutString("de riego") ;
    CyDelay(100);
    I2C_CLCD_Clear();

#if TEST_RELE
    for (;;)
    {
        printf("Rele ON\r\n");
        I2C_CLCD_Clear();
        I2C_CLCD_PutString("TEST RELE");
        I2C_CLCD_Pos(2 , 1);
        I2C_CLCD_PutString("ON");
        cyhal_gpio_write(BOMBA_PIN, BOMBA_ON);
        CyDelay(TEST_RELE_DELAY_MS);

        printf("Rele OFF\r\n");
        I2C_CLCD_Clear();
        I2C_CLCD_PutString("TEST RELE");
        I2C_CLCD_Pos(2 , 1);
        I2C_CLCD_PutString("OFF");
        cyhal_gpio_write(BOMBA_PIN, BOMBA_OFF);
        CyDelay(TEST_RELE_DELAY_MS);
    }
#endif
    /*
    int suma = 0;
    char value[10];
    // inicio de sistema
    for (int i = 1; i <= 100; i++) {
        suma += i;
        sprintf(value ,"%d" , i);
        I2C_CLCD_PutString("Cargando sistema");
        I2C_CLCD_Pos(2 , 1);
        I2C_CLCD_PutString("porcentaje:");
        I2C_CLCD_PutString(value);
        I2C_CLCD_PutString("%");
        CyDelay(100);
        I2C_CLCD_Clear();
    }

    I2C_CLCD_PutString("Terminado!");
    I2C_CLCD_Pos(2 , 1);
    I2C_CLCD_PutString("Iniciando!!");
    */

    uint8_t bomba_encendida = 0;
    uint8_t suelo_seco_confirmado = 0;
    uint8_t lecturas_suelo_seco = 0;
    uint8_t nivel_agua_bajo = 1;

    for (;;)
    {
        uint8_t suelo_seco = (cyhal_gpio_read(SUELO_PIN) == SUELO_SECO_LEVEL);
        uint32_t nivel_agua_uv = leer_nivel_agua_uv(&adc_agua_channel);

        if (nivel_agua_uv <= NIVEL_AGUA_BAJO_UV)
        {
            nivel_agua_bajo = 1;
        }
        else if (nivel_agua_uv >= NIVEL_AGUA_OK_UV)
        {
            nivel_agua_bajo = 0;
        }

        /* Se confirma suelo seco en dos lecturas; suelo humedo corta inmediatamente. */
        if (suelo_seco)
        {
            if (lecturas_suelo_seco < LECTURAS_SECO_PARA_REGAR)
            {
                lecturas_suelo_seco++;
            }
            if (lecturas_suelo_seco >= LECTURAS_SECO_PARA_REGAR)
            {
                suelo_seco_confirmado = 1;
            }
        }
        else
        {
            lecturas_suelo_seco = 0;
            suelo_seco_confirmado = 0;
        }

		DHT_Task();
		printf("\x1b[2J\x1b[;H");
		printf("DHT estado: %u\n", DHT_reading.result_code);
        printf("Suelo: %s%s\n", suelo_seco ? "SECO" : "HUMEDO",
               suelo_seco_confirmado ? " (confirmado)" : "");
        printf("Nivel agua: %lu mV (%s)\n", (unsigned long)(nivel_agua_uv / 1000UL),
               nivel_agua_bajo ? "BAJO" : "OK");

		if (DHT_reading.result_code == 0)
		{
		    printf("Humedad: %.2f, Temperatura: %.2f\n", DHT_reading.humidity, DHT_reading.temperature);
		}

        /* El suelo solicita riego, el DHT11 confirma y el nivel protege la bomba. */
        if (suelo_seco_confirmado &&
            (DHT_reading.result_code == 0) &&
            (DHT_reading.temperature >= TEMP_MIN_RIEGO) &&
            !nivel_agua_bajo)
        {
            cyhal_gpio_write(BOMBA_PIN, BOMBA_ON);
            bomba_encendida = 1;
        }
        else
        {
            cyhal_gpio_write(BOMBA_PIN, BOMBA_OFF);
            bomba_encendida = 0;
        }

		printf("Bomba: %s\n", bomba_encendida ? "ON" : "OFF");

		/* manejo lcd */
		I2C_CLCD_Clear();
		if (DHT_reading.result_code == 0)
		{
            snprintf(linea1, sizeof(linea1), "T:%.1fC H:%.0f%%",
                     DHT_reading.temperature, DHT_reading.humidity);
		}
		else
		{
            snprintf(linea1, sizeof(linea1), "DHT Error %u", DHT_reading.result_code);
		}
        if (nivel_agua_bajo)
        {
            snprintf(linea2, sizeof(linea2), "AGUA BAJA B:%s",
                     bomba_encendida ? "ON" : "OFF");
        }
        else
        {
            snprintf(linea2, sizeof(linea2), "Suelo:%s B:%s",
                     suelo_seco_confirmado ? "SECO" : "HUM",
                     bomba_encendida ? "ON" : "OFF");
        }
        I2C_CLCD_PutString(linea1);
        I2C_CLCD_Pos(2 , 1);
        I2C_CLCD_PutString(linea2);

        /* El buzzer avisa solamente cuando el nivel de agua es bajo. */
        if (nivel_agua_bajo)
        {
            cyhal_pwm_start(&pwm);
        }

        if (bomba_encendida)
        {
            uint32_t tiempo_bomba_ms = 0;

            /* Supervisa el deposito durante todo el pulso de riego. */
            while (tiempo_bomba_ms < BOMBA_TIEMPO_ON_MS)
            {
                CyDelay(BOMBA_NIVEL_CHECK_MS);
                tiempo_bomba_ms += BOMBA_NIVEL_CHECK_MS;
                nivel_agua_uv = leer_nivel_agua_uv(&adc_agua_channel);

                if (nivel_agua_uv <= NIVEL_AGUA_BAJO_UV)
                {
                    nivel_agua_bajo = 1;
                    printf("Seguridad: bomba detenida por nivel de agua bajo\n");
                    break;
                }
            }

            cyhal_gpio_write(BOMBA_PIN, BOMBA_OFF);
            bomba_encendida = 0;
            CyDelay(LCD_RECOVERY_DELAY_MS);

            /* Re-sincroniza el LCD despues del ruido producido al conmutar la bomba. */
            I2C_CLCD_Init();
            if (nivel_agua_bajo)
            {
                I2C_CLCD_PutString("AGUA BAJA");
                I2C_CLCD_Pos(2, 1);
                I2C_CLCD_PutString("Bomba bloqueada");
                cyhal_pwm_start(&pwm);
                CyDelay(BEEP_DURATION);
                cyhal_pwm_stop(&pwm);
                CyDelay(DHT_READ_DELAY_MS - BEEP_DURATION);
            }
            else
            {
                I2C_CLCD_PutString("Espera humedad");
                I2C_CLCD_Pos(2, 1);
                I2C_CLCD_PutString("Bomba OFF 5s");
                CyDelay(BOMBA_ESPERA_MS);
            }
        }
        else if (nivel_agua_bajo)
        {
            CyDelay(BEEP_DURATION);
            cyhal_pwm_stop(&pwm);
            CyDelay(DHT_READ_DELAY_MS - BEEP_DURATION);
        }
        else
        {
            cyhal_pwm_stop(&pwm);
            CyDelay(DHT_READ_DELAY_MS);
        }

        /*
        // Codigo de ejemplo I2C original. No se usa para el riego.
        buffer[PACKET_SOP_POS] = PACKET_SOP;
        buffer[PACKET_EOP_POS] = PACKET_EOP;
        buffer[PACKET_CMD_POS] = cmd;

        if (CY_RSLT_SUCCESS == cyhal_i2c_master_write(&mI2C, I2C_SLAVE_ADDR,
                                                  buffer, PACKET_SIZE, 0, true))
        {
            if (CY_RSLT_SUCCESS == cyhal_i2c_master_read(&mI2C, I2C_SLAVE_ADDR,
                                                 buffer, PACKET_SIZE, 0, true))
            {
                if ((PACKET_SOP == buffer[PACKET_SOP_POS]) &&
                   (PACKET_EOP == buffer[PACKET_EOP_POS]) &&
                   (STATUS_CMD_DONE == buffer[PACKET_CMD_POS]))
                {
                    cmd = (cmd == CYBSP_LED_STATE_ON) ?
                           CYBSP_LED_STATE_OFF : CYBSP_LED_STATE_ON;
                }
                else
                {
                    handle_error(1);
                }
            }
            cyhal_system_delay_ms(CMD_TO_CMD_DELAY);
        }
        */
    }
}
