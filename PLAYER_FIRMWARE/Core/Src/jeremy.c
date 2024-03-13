#include <FreeRTOS.h>
#include <task.h>

void jeremy_main(void *ignore) {


	vTaskSuspend(xTaskGetCurrentTaskHandle()); //LEAVE AT THE END
	vTaskDelete(NULL);
}
