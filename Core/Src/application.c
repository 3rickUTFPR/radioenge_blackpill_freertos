#include "cmsis_os.h"
// #include "cmsis_os2.h"
#include "stm32f4xx_hal.h"
#include "radioenge_modem.h"
#include "uartRingBufDMA.h"
#include "main.h"
#include <stdint.h>
#include <stdio.h>

extern osTimerId_t PeriodicSendTimerHandle;
extern osThreadId_t AppSendTaskHandle;
extern ADC_HandleTypeDef hadc1;
extern osEventFlagsId_t ModemStatusFlagsHandle;
extern osMessageQueueId_t TemperatureQueueHandle;
extern osMessageQueueId_t AccontrollerQueueHandle;
char msg[256];

extern TIM_HandleTypeDef htim3;

void LoRaWAN_RxEventCallback(uint8_t *data, uint32_t length, uint32_t port, int32_t rssi, int32_t snr)
{
    // uint16_t *I;
    //ACCONTROLLER_OBJ_t rcv_data;
    //rcv_data = *((ACCONTROLLER_OBJ_t *)data);
    // rcv_data.compressor_power;
    osMessageQueuePut(AccontrollerQueueHandle, data, 0U, osWaitForever);
}
void DutyCycleCode(void *data)
{
    ACCONTROLLER_OBJ_t rcv_data;
    while(1)
    {
        osMessageQueueGet(AccontrollerQueueHandle, &rcv_data, NULL, osWaitForever); // wait for message
        htim3.Instance->CCR2 = (htim3.Instance->ARR * (rcv_data.compressor_power)) / 100;
    }
}
void PeriodicSendTimerCallback(void *argument)
{
}

void AppSendTaskCode(void *argument)
{
    uint32_t read;
    TEMPERATURE_OBJ_t temp;
    temp.seq_no = 0;

    float R1 = 10000, logR2;
    float tempThermistor;
    const float c1 = 0.001129148;
    const float c2 = 0.000234125;
    const float c3 = 0.0000000876741;
    uint16_t mV;               // read voltage
    const uint16_t mV_KY_013 = 3300; // KY-013 power voltage

    while (1)
    {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 100);
        read = HAL_ADC_GetValue(&hadc1);
        temp.seq_no++;
        //temp.temp_oCx100 = (int32_t)(330 * ((float)read / 4096));
        mV = 3300 * read / 4095.0;
        logR2 = log(R1 * mV / (mV_KY_013 - mV));  // calculate resistance on thermistor               
        tempThermistor = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2)); // temperature in Kelvin
        tempThermistor = tempThermistor - 273.15; // convert Kelvin to Celsius
        temp.temp_oCx100 = (int32_t)(tempThermistor*100);
       // temp.compressor_power = gLastPWMStatus.compressor_power;
        LoRaSendB(2, (uint8_t *)&temp, sizeof(TEMPERATURE_OBJ_t));
    }
}

void ReadFromADCTaskCode(void *argument)
{
    uint32_t read;
    // int32_t temp_oCx100;
    TEMPERATURE_OBJ_t data;
    data.seq_no = 0;

    float R1 = 10000, logR2;
    float tempThermistor;
    const float c1 = 0.001129148;
    const float c2 = 0.000234125;
    const float c3 = 0.0000000876741;
    uint16_t mV;               // read voltage
    const uint16_t mV_KY_013 = 3300; // KY-013 power voltage
      
    while (1)
    {
        // read LM35 Temperature
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 100);
        read = HAL_ADC_GetValue(&hadc1);
        data.seq_no = data.seq_no + 1;
        //data.temp_oCx100 = (int32_t)(3300 * ((float)read / 4096));
        mV = 3300 * read / 4095.0;
        logR2 = log(R1 * mV / (mV_KY_013 - mV));  // calculate resistance on thermistor               
        tempThermistor = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2)); // temperature in Kelvin
        tempThermistor = tempThermistor - 273.15; // convert Kelvin to Celsius
        data.temp_oCx100 = (int32_t)(tempThermistor*100);
        //temp.compressor_power = gLastPWMStatus.compressor_power;
   
        // Send Message
        osMessageQueuePut(TemperatureQueueHandle, &data, 0U, osWaitForever);
        osDelay(30000);
    }
}
