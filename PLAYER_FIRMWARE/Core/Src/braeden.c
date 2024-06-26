#include <FreeRTOS.h>
#include <task.h>
#include <main.h>
#include <console.h>
#include "app_fatfs.h"

#include "wyatt.h"
#include "braeden.h"

#define BUF_SIZE 256
uint16_t adcBuffer[BUF_SIZE*2];
uint16_t *main_buf, *isr_buf;


//Swaps the contents of the isr_buf and main_buf

bool recording_complete;


void swap_buffers(void){
	uint16_t *temp = main_buf;
	main_buf = isr_buf;
	isr_buf = temp;
}

FRESULT res;

TaskHandle_t task_handle = NULL;

bool aud_buf_ready = false;
bool ready_to_start = false;
bool write_complete = false;
uint32_t num_writes = 0;

//Gets the converted value from the ADC and puts it in a buffer. Repeats until buffer is filled.
void get_audio_sample(void){
	static int entry = -1;
	uint32_t bytes_written;

	if ((stop_recording == true) && (write_complete == true)){
		ready_to_start = false;
		bytes_written = num_writes * 256 * 2;
	}

	if (start_recording == true){
		write_complete = false;
		ready_to_start = true;
	}

	if(!ready_to_start) return;

	entry = (entry+1)%BUF_SIZE;

	isr_buf[entry] = HAL_ADC_GetValue(&hadc1);

	HAL_ADC_Start(&hadc1);

	if (entry == BUF_SIZE-1){
		swap_buffers();
		aud_buf_ready = true;
		//xTaskResumeFromISR(task_handle);
	}
}

void braeden_main(void *ignore __attribute__((unused))) {
	task_handle = xTaskGetCurrentTaskHandle();
	HAL_ADC_Start(&hadc1);
	ready_to_start = true;
	isr_buf = adcBuffer;
	main_buf = adcBuffer+BUF_SIZE;

	static size_t chars_written = 0;
	while(1) {
		//vTaskSuspend(NULL);
		if(!aud_buf_ready) continue;
		aud_buf_ready = false;
		if(!start_recording) {chars_written = 0; continue;};
		chars_written += BUF_SIZE;
		f_write(write_file_handle, main_buf, BUF_SIZE, NULL);
		if(chars_written >= 882000) {
			recording_complete = true;
			start_recording = false;
		}

	}
	vTaskSuspend(xTaskGetCurrentTaskHandle()); //LEAVE AT THE END
	vTaskDelete(NULL);
}
