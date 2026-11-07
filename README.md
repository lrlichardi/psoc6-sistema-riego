# Sistema de riego automatico con PSoC 6

Proyecto final desarrollado con una placa PSoC 6 CY8CPROTO-062-4343W y ModusToolbox. El sistema mide temperatura, humedad ambiente, humedad del suelo y nivel de agua para controlar una bomba de riego de 12 V mediante un modulo de rele.

## Video de demostracion

[![Demostracion del sistema de riego con PSoC 6](https://img.youtube.com/vi/HMHHp0fm4Ac/hqdefault.jpg)](https://youtu.be/HMHHp0fm4Ac)

[Ver demostracion completa en YouTube](https://youtu.be/HMHHp0fm4Ac)

## Funciones principales

- Lectura de temperatura y humedad ambiente con un DHT11.
- Deteccion digital de suelo seco mediante un modulo con comparador.
- Medicion analogica del nivel de agua del deposito.
- Bloqueo de la bomba y alarma sonora cuando el nivel de agua es bajo.
- Riego por pulsos de 3 segundos con una espera de 5 segundos antes de volver a evaluar el suelo.
- Visualizacion de mediciones y estados en una pantalla LCD I2C 16x2.

## Conexiones

| Elemento | Conexion PSoC |
| --- | --- |
| DHT11 DATA | P9_7 |
| Sensor de suelo DO | P9_6 |
| Sensor de nivel S | P10_0 |
| Rele IN1 | P9_4 |
| Buzzer | P9_1 |
| LCD SCL | P6_0 |
| LCD SDA | P6_1 |

Los sensores conectados directamente a entradas del PSoC trabajan con 3,3 V. El modulo de rele utiliza 5 V y la bomba tiene una fuente externa de 12 V. La placa solamente controla la entrada del rele y no alimenta la bomba.

## Logica de funcionamiento

El sistema confirma dos lecturas consecutivas de suelo seco antes de regar. Si el DHT11 entrega una lectura valida, la temperatura es de al menos 20 grados y existe suficiente agua en el deposito, activa la bomba durante un maximo de 3 segundos. Luego espera 5 segundos para que el agua se distribuya y vuelve a medir.

El nivel del deposito se supervisa antes y durante el riego. Una lectura igual o menor a 0,9 V bloquea la bomba y activa el buzzer. El sistema vuelve a habilitar el riego cuando la tension alcanza 1,2 V.

## Compilacion

Requisitos:

- ModusToolbox 3.2 o posterior.
- BSP `CY8CPROTO-062-4343W`.
- Toolchain GCC ARM.

Desde una terminal de ModusToolbox:

```bash
make getlibs
make build
```

El archivo generado se guarda dentro de `build/APP_CY8CPROTO-062-4343W/Debug/`.

## Documentacion

- `DOCUMENTACION_PROYECTO.md`: explicacion tecnica del firmware y las conexiones.
- `Informe_Proyecto_Riego_PSoC.docx`: informe preparado para la presentacion del proyecto.

El proyecto parte del ejemplo HAL I2C master de Infineon y conserva su licencia original en `LICENSE`.
