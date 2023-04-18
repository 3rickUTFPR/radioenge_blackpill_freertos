#include "cmsis_os.h"
// #include "cmsis_os2.h"
#include "stm32f4xx_hal.h"
#include "radioenge_modem.h"
#include "uartRingBufDMA.h"
#include "main.h"
#include "stdint.h"
#include "stdio.h"

extern osTimerId_t PeriodicSendTimerHandle;
extern osThreadId_t AppSendTaskHandle;
extern ADC_HandleTypeDef hadc1;
extern osEventFlagsId_t ModemStatusFlagsHandle;
extern osMessageQueueId_t TemperatureQueueHandle;
char msg[256];

extern TIM_HandleTypeDef htim3;


void LoRaWAN_RxEventCallback(uint8_t *data, uint32_t length, uint32_t port, int32_t rssi, int32_t snr)
{
}

void PeriodicSendTimerCallback(void *argument)
{
}

void AppSendTaskCode(void *argument)
{
uint32_t read;
TEMPERATURE_OBJ_t temp;
temp.seq_no = 0;
while (1)
{
HAL_ADC_Start(&hadc1);
HAL_ADC_PollForConversion(&hadc1, 100);
read = HAL_ADC_GetValue(&hadc1);
temp.seq_no++;
temp.temp_oCx100 = (int32_t)(330 * ((float)read / 4096));
LoRaSendB(2, (uint8_t *)&temp, sizeof(TEMPERATURE_OBJ_t));
}
}

void ReadFromADCTaskCode(void *argument)
{
    uint32_t read;
    //int32_t temp_oCx100;
    TEMPERATURE_OBJ_t data;
    data.seq_no = 0;
    while (1)
    {
        // read LM35 Temperature
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 100);
        read = HAL_ADC_GetValue(&hadc1);
        data.seq_no = data.seq_no + 1;
        data.temp_oCx100 = (int32_t)(33000 * ((float)read / 4096));
        // Send Message
        osMessageQueuePut(TemperatureQueueHandle, &data, 0U, osWaitForever);
        osDelay(200);
    }
}

void PeriodicSendTimerCallback(void *argument)
{
}


