# Proyecto de riego automatico con PSoC 6

## Objetivo del proyecto

El proyecto implementa un sistema de riego automatico usando una placa PSoC 6, un sensor digital de humedad de suelo con comparador, un sensor DHT11, un sensor analogico de nivel de agua, una pantalla LCD I2C, un buzzer y un modulo de rele para controlar una bomba.

La logica principal es:

- Detectar si el suelo esta seco o humedo mediante la salida digital del comparador.
- Leer temperatura y humedad ambiente con el DHT11 como segundo sensor.
- Medir el nivel de agua disponible mediante una entrada analogica.
- Mostrar los valores en una pantalla LCD.
- Activar la bomba durante 3 segundos cuando el suelo esta seco y la temperatura ambiente permite el riego.
- Esperar 5 segundos con la bomba apagada, volver a medir y repetir el pulso solo si el suelo sigue seco.
- Activar el buzzer si el nivel de agua del deposito es bajo.
- Bloquear o detener inmediatamente la bomba si el deposito queda con nivel bajo.

## Componentes utilizados

- Placa PSoC 6.
- Sensor de humedad de suelo con modulo comparador LM393 y salida digital `DO`.
- Sensor DHT11.
- Sensor analogico de nivel de agua de 3 pines.
- Pantalla LCD con interfaz I2C.
- Modulo de rele de 5V.
- Bomba de agua de 12V.
- Fuente externa de 12V para la bomba.
- Alimentacion USB para la placa PSoC.

## Conexion de hardware

### Sensor DHT11

El sensor DHT11 se conecta al pin `P9_7`.

```text
DHT11 VCC  -> 3.3V
DHT11 GND  -> GND
DHT11 DATA -> P9_7
```

Si se usa el DHT11 suelto de 4 pines, se agrega una resistencia pull-up entre `DATA` y `3.3V`, por ejemplo `5.6 kOhm`.

Si se usa un modulo DHT11 de 3 pines, normalmente ya trae esa resistencia incluida.

### Sensor de humedad de suelo con comparador

Se utiliza la salida digital `DO` del modulo comparador y se conecta a `P9_6`.

```text
Sensor suelo VCC -> 3.3V
Sensor suelo GND -> GND
Sensor suelo DO  -> P9_6
Sensor suelo AO  -> sin conectar
```

El potenciometro del modulo ajusta el punto en el cual la salida digital cambia entre suelo seco y suelo humedo. En la configuracion mas comun con LM393, `DO` entrega nivel alto cuando el suelo esta seco y nivel bajo cuando esta humedo. El codigo permite invertir esta interpretacion cambiando `SUELO_SECO_LEVEL` de `1` a `0`.

El modulo debe alimentarse con `3.3V` cuando `DO` se conecta directamente al PSoC. Si se alimenta con `5V`, la salida digital puede alcanzar un nivel no seguro para el pin del microcontrolador.

### Sensor de nivel de agua

El sensor de nivel de agua de tres pines entrega una tension analogica por el terminal `S`. Se conecta a la entrada ADC `P10_0`.

```text
Sensor nivel S -> P10_0
Sensor nivel + -> 3.3V
Sensor nivel - -> GND
```

La salida aumenta a medida que una mayor parte de las pistas queda cubierta por agua. Como la relacion depende del agua, la profundidad y el propio sensor, el programa muestra la tension medida en milivoltios por consola para poder calibrar el umbral.

El sensor se alimenta con `3.3V`, no con `5V`, para que la tension aplicada a `P10_0` permanezca dentro del rango seguro del PSoC.

`P9_2` no se utiliza para este sensor porque, en el PSoC 6 `CY8C624ABZI-S2D44`, no esta conectado al convertidor ADC. Intentar inicializar el ADC en ese pin detiene el programa antes de iniciar la pantalla LCD.

### Pantalla LCD I2C

La pantalla usa el bus I2C de la placa.

```text
LCD SDA -> CYBSP_I2C_SDA
LCD SCL -> CYBSP_I2C_SCL
LCD VCC -> 5V o 3.3V segun el modulo
LCD GND -> GND
```

En este BSP, las senales I2C estan asociadas a:

```text
SCL -> P6_0
SDA -> P6_1
```

### Buzzer

El buzzer se controla con PWM desde el pin `P9_1`.

```text
Buzzer + -> P9_1
Buzzer - -> GND
```

### Modulo de rele y bomba

El modulo de rele se controla desde `P9_4`.

```text
P9_4      -> IN1 del modulo rele
5V/VBUS   -> VCC del modulo rele
GND PSoC  -> GND del modulo rele
```

La bomba no se alimenta desde la placa PSoC. La bomba usa una fuente externa de `12V`.

Conexion de potencia recomendada:

```text
+12V fuente -> COM del rele
NO del rele -> positivo de la bomba
negativo bomba -> negativo fuente 12V
```

El modulo de rele usado es de logica activa en bajo. Por eso en el codigo:

```c
#define BOMBA_ON  0
#define BOMBA_OFF 1
```

## Pines configurados en el codigo

```c
#define DATA_PIN   P9_7
#define BUZZER_PIN P9_1
#define BOMBA_PIN  P9_4
#define SUELO_PIN  P9_6
#define NIVEL_AGUA_PIN P10_0
```

- `DATA_PIN`: pin usado para comunicarse con el DHT11.
- `BUZZER_PIN`: pin de salida PWM para el buzzer.
- `BOMBA_PIN`: pin de salida digital que controla el rele de la bomba.
- `SUELO_PIN`: entrada digital conectada a `DO` del comparador de humedad de suelo.
- `NIVEL_AGUA_PIN`: entrada analogica conectada al pin `S` del sensor de nivel.

## Condiciones de riego

```c
#define SUELO_SECO_LEVEL 1
#define LECTURAS_SECO_PARA_REGAR 2
#define TEMP_MIN_RIEGO 20.0f
#define NIVEL_AGUA_BAJO_UV 900000UL
#define NIVEL_AGUA_OK_UV 1200000UL
#define MUESTRAS_NIVEL_AGUA 8U
#define BOMBA_TIEMPO_ON_MS 3000U
#define BOMBA_ESPERA_MS 5000U
```

- `SUELO_SECO_LEVEL` define el nivel logico que representa tierra seca.
- Se requieren dos lecturas secas consecutivas antes de encender la bomba.
- La bomba solo puede encenderse si el DHT11 mide `20 C` o mas.
- Cada riego dura 3 segundos. Luego la bomba se apaga durante 5 segundos para permitir que el agua se distribuya y se vuelve a medir el suelo.
- Si el suelo se humedece durante la espera, no se genera otro pulso de riego.
- Si el DHT11 falla o la temperatura baja de `20 C`, la bomba tambien se apaga.
- Si la tension del sensor baja a `0,9 V` o menos, se activa la alarma y la bomba queda bloqueada.
- La alarma se desactiva cuando la tension sube a `1,2 V` o mas.

Las dos lecturas consecutivas del sensor de suelo evitan que el rele se active por un cambio breve. El sensor de agua se promedia ocho veces y utiliza dos umbrales distintos para evitar que el buzzer cambie rapidamente de estado cerca del limite.

## Lectura del sensor

El DHT11 usa un protocolo de un solo cable. La comunicacion funciona asi:

1. El PSoC tira la linea `DATA` a bajo durante aproximadamente `20 ms`.
2. Luego libera la linea.
3. El DHT11 responde bajando y subiendo la linea.
4. El sensor envia 40 bits:
   - humedad entera,
   - humedad decimal,
   - temperatura entera,
   - temperatura decimal,
   - checksum.

En el archivo `dht11.c`, la funcion `DHT_Start()` inicia la comunicacion:

```c
void DHT_Start(void)
{
    Cy_GPIO_SetDrivemode(DATA_PORT, DATA_NUM, CY_GPIO_DM_OD_DRIVESLOW);
    Cy_GPIO_Write(DATA_PORT, DATA_NUM, 1);
    CyDelay(1000);
    Cy_GPIO_Write(DATA_PORT, DATA_NUM, 0);
    cyhal_system_delay_ms(20);
    Cy_GPIO_Write(DATA_PORT, DATA_NUM, 1);
}
```

El modo `CY_GPIO_DM_OD_DRIVESLOW` permite que el PSoC tire la linea a `0`, pero cuando escribe `1` la deja liberada. La resistencia pull-up se encarga de llevar la linea a `3.3V`. Esto es apropiado para sensores de un solo cable como el DHT11.

## Codigos de error del DHT11

El programa muestra un codigo de estado para diagnosticar problemas del sensor:

```text
DHT 0  -> lectura correcta
DHT 17 -> el sensor no bajo la linea para responder
DHT 18 -> el sensor bajo la linea, pero no subio como se esperaba
DHT 19 -> la linea quedo alta cuando debia cambiar
DHT 20 -> error leyendo un bit, linea baja trabada
DHT 21 -> error leyendo un bit, linea alta trabada
DHT 2  -> el sensor respondio, pero el checksum no coincide
```

Cuando la lectura es correcta, el programa guarda temperatura y humedad en la estructura global `DHT_reading`.

```c
struct readings DHT_reading;
```

## Funcionamiento del programa principal

El programa principal esta en `main.c`.

### Inicializacion general

Primero se inicializa la placa:

```c
result = cybsp_init();
handle_error(result);
```

Luego se inicializa el PWM para el buzzer:

```c
result = cyhal_pwm_init(&pwm, BUZZER_PIN, NULL);
handle_error(result);
result = cyhal_pwm_set_duty_cycle(&pwm, 50, PWM_FREQUENCY);
handle_error(result);
```

El buzzer usa una frecuencia de `3000 Hz`:

```c
#define PWM_FREQUENCY 3000
```

### Inicializacion del DHT11

El pin del DHT11 se configura como bidireccional open drain:

```c
result = cyhal_gpio_init(DATA_PIN,
                         CYHAL_GPIO_DIR_BIDIRECTIONAL,
                         CYHAL_GPIO_DRIVE_OPENDRAINDRIVESLOW,
                         1);
```

### Inicializacion del rele

El rele se inicializa apagado:

```c
result = cyhal_gpio_init(BOMBA_PIN,
                         CYHAL_GPIO_DIR_OUTPUT,
                         CYHAL_GPIO_DRIVE_STRONG,
                         BOMBA_OFF);
```

Esto evita que la bomba arranque apenas se enciende el sistema.

### Inicializacion del sensor de suelo

La salida `DO` se configura como entrada digital con pull-up interno:

```c
result = cyhal_gpio_init(SUELO_PIN,
                         CYHAL_GPIO_DIR_INPUT,
                         CYHAL_GPIO_DRIVE_PULLUP,
                         1);
```

El pull-up mantiene un nivel definido si la salida del comparador es de colector abierto o si el cable se desconecta.

### Inicializacion del sensor de nivel de agua

Primero se reserva el ADC asociado a `P10_0`, se configura para medir entre masa (`VSSA`) y la alimentacion analogica (`VDDA`) y luego se crea un canal de una sola entrada:

```c
result = cyhal_adc_init(&adc_agua, NIVEL_AGUA_PIN, NULL);
handle_error(result);
result = cyhal_adc_configure(&adc_agua, &adc_agua_config);
handle_error(result);
result = cyhal_adc_channel_init_diff(&adc_agua_channel,
                                     &adc_agua,
                                     NIVEL_AGUA_PIN,
                                     CYHAL_ADC_VNEG,
                                     NULL);
handle_error(result);
```

La funcion `cyhal_adc_read_uv()` entrega directamente la tension medida en microvoltios. El programa descarta valores negativos y promedia ocho mediciones para reducir el ruido. Esta forma de lectura evita el desplazamiento que introduce `cyhal_adc_read_u16()` en este PSoC: con aquella conversion, incluso una entrada conectada a GND podia aparecer cerca de la mitad de la escala y no activar la alarma.

### Inicializacion del LCD

La pantalla se inicializa con:

```c
I2C_CLCD_Init();
```

Luego muestra un mensaje inicial:

```c
I2C_CLCD_PutString("Inicio sistema");
I2C_CLCD_Pos(2, 1);
I2C_CLCD_PutString("de riego");
```

La barra de progreso original quedo comentada para que el sistema arranque mas rapido.

## Bucle principal

El programa entra en un `for(;;)`, que es un bucle infinito.

En cada vuelta:

1. Lee la salida digital del sensor de suelo.
2. Confirma el estado seco durante dos lecturas consecutivas.
3. Promedia ocho lecturas analogicas del sensor de nivel de agua.
4. Actualiza el estado de nivel bajo usando los dos umbrales.
5. Lee el DHT11.
6. Imprime los estados por consola.
7. Evalua si corresponde encender o apagar la bomba.
8. Actualiza el LCD.
9. Hace sonar el buzzer si el nivel de agua es bajo.
10. Espera hasta completar el intervalo de 2 segundos.

La lectura se ejecuta con:

```c
DHT_Task();
```

La pantalla se actualiza cada:

```c
#define DHT_READ_DELAY_MS 2000
```

En funcionamiento sin riego, la lectura se repite aproximadamente cada `2 segundos`. Durante un ciclo de riego se aplica un pulso de 3 segundos y una espera de 5 segundos antes de volver a leer.

## Control de la bomba

La variable:

```c
uint8_t bomba_encendida = 0;
```

guarda si la bomba esta encendida o apagada durante la evaluacion actual.

La bomba se habilita cuando el suelo seco fue confirmado, el DHT11 entrega una lectura valida de al menos `20 C` y hay suficiente agua:

```c
if (suelo_seco_confirmado &&
    (DHT_reading.result_code == 0) &&
    (DHT_reading.temperature >= TEMP_MIN_RIEGO) &&
    !nivel_agua_bajo)
{
    cyhal_gpio_write(BOMBA_PIN, BOMBA_ON);
    bomba_encendida = 1;
}
```

Una vez habilitada, la bomba trabaja por pulsos:

```c
cyhal_gpio_write(BOMBA_PIN, BOMBA_ON);
CyDelay(BOMBA_TIEMPO_ON_MS);
cyhal_gpio_write(BOMBA_PIN, BOMBA_OFF);
CyDelay(BOMBA_ESPERA_MS);
```

Mientras la bomba esta encendida, el programa vuelve a leer el nivel cada `100 ms`. Si la lectura cae al umbral bajo, corta inmediatamente el rele, muestra `Bomba bloqueada` y hace sonar el buzzer. Al terminar una espera normal, el lazo vuelve a leer el sensor de suelo. Si no mejoro, genera otro pulso de 3 segundos; si ya esta humedo, la bomba permanece apagada.

## Manejo del LCD

Si los sensores leen bien, el LCD muestra:

```text
T:xx.xC H:xx%
Suelo:SECO B:ON
```

Cuando el suelo esta humedo, la segunda linea muestra `Suelo:HUM B:OFF`. Si el DHT11 falla, la primera linea muestra el codigo de error y la segunda conserva el estado del suelo y de la bomba:

```text
DHT Error xx
Suelo:HUM B:OFF
```

Si el deposito tiene nivel bajo, el aviso de agua tiene prioridad en la segunda linea:

```text
T:xx.xC H:xx%
AGUA BAJA B:OFF
```

De esta forma no se confunde una lectura fallida con una temperatura real de `0 C`.

Al cortar la bomba, el programa reinicializa el controlador del LCD. Esto recupera la sincronizacion si un pico electrico altero la comunicacion. El bus I2C funciona a `100 kHz` para aumentar la tolerancia al ruido.

## Manejo del buzzer

El buzzer ya no se activa por temperatura baja. Su unica funcion es avisar que el sensor conectado a `P10_0` detecta poco nivel de agua.

```c
if (nivel_agua_bajo)
{
    cyhal_pwm_start(&pwm);
    CyDelay(BEEP_DURATION);
    cyhal_pwm_stop(&pwm);
    CyDelay(DHT_READ_DELAY_MS - BEEP_DURATION);
}
else
{
    cyhal_pwm_stop(&pwm);
    CyDelay(DHT_READ_DELAY_MS);
}
```

El sonido queda intermitente:

```text
500 ms encendido
1500 ms apagado aproximadamente
```

Esto se define con:

```c
#define BEEP_DURATION 500
#define DHT_READ_DELAY_MS 2000
```

El nivel bajo activa la alarma sonora, muestra el mensaje del LCD y fuerza `BOMBA_OFF`. La bomba no vuelve a habilitarse hasta que el nivel alcanza `NIVEL_AGUA_OK_UV` (`1,2 V`).

### Calibracion del nivel de agua

Con el programa ejecutandose se observa en la consola la linea `Nivel agua: ... mV`. Se anota el valor con el deposito vacio y luego con el nivel minimo que se desea aceptar. Los valores `NIVEL_AGUA_BAJO_UV` y `NIVEL_AGUA_OK_UV` se expresan en microvoltios y se ajustan entre esas mediciones, dejando una separacion para evitar oscilaciones. Con las mediciones obtenidas en el prototipo (`16 mV` seco, `250 mV` con una gota y `2 V` sumergido), se adoptaron `0,9 V` para declarar nivel bajo y `1,2 V` para recuperar el estado normal.

Para comprobar la proteccion, se puede unir momentaneamente `P10_0` a GND. La consola debe indicar aproximadamente `Nivel agua: 0 mV (BAJO)`, el LCD debe mostrar `AGUA BAJA`, el buzzer debe sonar de forma intermitente y la bomba debe permanecer apagada. Si se desconecta solamente el cable `S`, la entrada analogica queda flotante y su estado no es predecible; para detectar tambien un cable desconectado se recomienda una resistencia de `100 kOhm` entre `P10_0` y GND.

## Modos de prueba

El codigo conserva dos modos de prueba temporales:

```c
#define TEST_PIN_DHT 0
#define TEST_RELE 0
```

Con `TEST_PIN_DHT` en `1`, el pin del DHT queda fijo en alto para medir con tester.

Con `TEST_RELE` en `1`, el rele prende y apaga cada 5 segundos:

```c
#define TEST_RELE_DELAY_MS 5000
```

Para el funcionamiento normal ambos deben quedar en `0`.

## Bloque I2C comentado

El proyecto original tenia un bloque de ejemplo I2C que usaba un buffer de 3 bytes:

```c
buffer[0] = PACKET_SOP;
buffer[1] = cmd;
buffer[2] = PACKET_EOP;
```

Ese bloque no forma parte del sistema de riego y quedo comentado. Se mantiene como referencia, pero no interviene en la lectura del sensor, el LCD, el buzzer ni la bomba.

## Resumen del funcionamiento final

El sistema final realiza un control automatico simple:

- Lee el sensor de suelo y el DHT11 aproximadamente cada 2 segundos cuando no esta regando.
- Lee el sensor analogico de nivel de agua en `P10_0` y promedia ocho muestras.
- Muestra temperatura, humedad ambiente, estado del suelo y estado de la bomba en el LCD.
- Si el suelo permanece seco durante dos lecturas y la temperatura es de `20 C` o mas, activa el rele durante 3 segundos.
- Luego apaga la bomba durante 5 segundos y vuelve a medir; repite el pulso solo si el suelo continua seco.
- Si el suelo se humedece, la temperatura baja de `20 C` o el DHT11 falla, mantiene el rele apagado.
- Si el nivel de agua es bajo, muestra `AGUA BAJA`, activa el buzzer y bloquea la bomba.
- Durante el riego controla el nivel cada `100 ms` y corta el rele si el deposito se vacia.
- La temperatura baja ya no activa el buzzer.
- Si el DHT11 falla, informa el codigo de error en pantalla y mantiene la bomba apagada.

Antes de la prueba final se debe calibrar el potenciometro del comparador con una muestra de tierra seca y otra humeda. Si el modulo entrega la logica invertida, se cambia `SUELO_SECO_LEVEL` a `0`.
