

#ifndef SOURCE_DHT_TASK_H_
#define SOURCE_DHT_TASK_H_
#include "cybsp.h"

/* Sensor timeout duration can be chnaged here */
#define timeout_duration 1000

float Fraction_Convert(uint8_t num);
void DHT_Start(void);
uint8 DHT_Read(float* humidity, float* temperature);

struct readings
{
	float humidity;
	float temperature;
	uint8 result_code;
};

void DHT_Task();

extern struct readings DHT_reading;


#endif /* SOURCE_DHT_TASK_H_ */
