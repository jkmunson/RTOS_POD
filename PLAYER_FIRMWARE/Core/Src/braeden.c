#include <FreeRTOS.h>
#include <task.h>
#include <main.h>
#include <console.h>
#include "app_fatfs.h"

uint8_t entry = 0;
uint16_t adcBuffer[255];
FIL microphone;
FRESULT res;
void get_audio_sample(void){
	adcBuffer[entry++] = HAL_ADC_GetValue(&hadc5);
	HAL_ADC_Start(&hadc5);
	if (entry == 255){
		vTaskResume((TaskHandle_t)threads[3]);
	}
}

void braeden_main(void *ignore __attribute__((unused))) {
	vTaskSuspend(xTaskGetCurrentTaskHandle());
	res = f_open(&microphone, "recording", FA_WRITE | FA_CREATE_ALWAYS);
	if (res != FR_OK){
		while(1);
	}
	f_write(&microphone, &adcBuffer, entry, NULL);
	entry = 0;
	console_print_time(), console_print("Hi from braeden main\n");
	console_print_time(), console_printf("Message %d\n", 2);
	vTaskSuspend(xTaskGetCurrentTaskHandle()); //LEAVE AT THE END
	vTaskDelete(NULL);
}
