#include <FreeRTOS.h>
#include <task.h>
#include <main.h>
#include <console.h>
#include "app_fatfs.h"




#define BUF_SIZE 256
uint16_t adcBuffer[BUF_SIZE*2];
uint16_t *main_buf, *isr_buf;

void swap_buffers(void){
	uint16_t *temp = main_buf;
	main_buf = isr_buf;
	isr_buf = temp;
}

FIL microphone;
FRESULT res;

TaskHandle_t task_handle = NULL;

bool aud_buf_ready = false;
bool ready_to_start = false;

void get_audio_sample(void){
	console_print(".");
	static int entry = -1;

	if(!ready_to_start) return;

	entry = (entry+1)%BUF_SIZE;

	adcBuffer[entry] = HAL_ADC_GetValue(&hadc1);

	HAL_ADC_Start(&hadc1);

	if (entry == BUF_SIZE-1){
		swap_buffers();
		aud_buf_ready = true;
		vTaskResume(task_handle);
	}
}

void braeden_main(void *ignore __attribute__((unused))) {
	task_handle = xTaskGetCurrentTaskHandle();
	HAL_ADC_Start(&hadc1);
	ready_to_start = true;
	isr_buf = adcBuffer;
	main_buf = adcBuffer+BUF_SIZE;


	while(1) {
		console_print("Hello\n");
		vTaskSuspend(NULL);
		if(!aud_buf_ready) continue;
		static div = 0;
		if((++div) % 1000) continue;
		console_printf("Sample: %d", main_buf[0]);
		//res = f_open(&microphone, "recording", FA_WRITE | FA_CREATE_ALWAYS);

		//f_write(&microphone, &adcBuffer, BUF_SIZE, NULL);
		aud_buf_ready = false;
	}
	vTaskSuspend(xTaskGetCurrentTaskHandle()); //LEAVE AT THE END
	vTaskDelete(NULL);
}
