#include <FreeRTOS.h>
#include <task.h>

extern uint8_t audio_buffer[49152]; //Can be recast to a more appropriate type.

void bryant_main(void *ignore) {


	vTaskSuspend(xTaskGetCurrentTaskHandle()); //LEAVE AT THE END
	vTaskDelete(NULL);
}
