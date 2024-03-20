#include <FreeRTOS.h>
#include <task.h>

#include <console.h>

void get_audio_sample(void){

}

void braeden_main(void *ignore __attribute__((unused))) {
	console_print_time(), console_print("Hi from braeden main\n");
	console_print_time(), console_printf("Message %d\n", 2);
	vTaskSuspend(xTaskGetCurrentTaskHandle()); //LEAVE AT THE END
	vTaskDelete(NULL);
}
