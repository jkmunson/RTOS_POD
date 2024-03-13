#include <FreeRTOS.h>
#include <task.h>

void braeden_main(void *ignore) {

	vTaskSuspend(xTaskGetCurrentTaskHandle()); //LEAVE AT THE END
	vTaskDelete(NULL);
}
