#include <FreeRTOS.h>
#include <task.h>

extern uint8_t audio_buffer[49152];
extern DAC_HandleTypeDef hdac1;
#define AUD_GREEN_DAC hdac1;

DAC_DHR12LD

void jeremy_main(void *ignore) {


	vTaskSuspend(xTaskGetCurrentTaskHandle()); //LEAVE AT THE END
	vTaskDelete(NULL);
}
