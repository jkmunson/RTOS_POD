#include <FreeRTOS.h>
#include <task.h>
#include <main.h>
#include <console.h>
#include "app_fatfs.h"

uint8_t status;
FIL adcBuffer;
void get_audio_sample(void){
	HAL_ADC_Start(&hadc5);
	status = HAL_ADC_PollForConversion(&hadc5, 1);
	if(status == HAL_ERROR){
		Error_Handler();
	}else if(status == HAL_TIMEOUT){
		return;
	}else{
		HAL_ADC_Stop(&hadc5);
	}
	f_write(&adcBuffer, &hadc5, 2, NULL);
}

void braeden_main(void *ignore __attribute__((unused))) {
	console_print_time(), console_print("Hi from braeden main\n");
	console_print_time(), console_printf("Message %d\n", 2);
	vTaskSuspend(xTaskGetCurrentTaskHandle()); //LEAVE AT THE END
	vTaskDelete(NULL);
}
