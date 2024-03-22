#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

#include "console.h"
#include "main.h"

//To simplify printf buffer logic, we must always have enough space to print the maximum size regardless of where we are in the buffer
#define CONSOLE_MAX_PRINTF_LENGTH 255
#define CONSOLE_BUFFER_SIZE 24576
#define CONSOLE_MAX_QUEUE_TRANSFERS 96
#define CONSOLE_BLOCKING_DELAY_TICKS 3


TaskHandle_t console_thread;

struct transfer_info {
	union {
		size_t buf_index;
		const uint8_t *buf_in_place;
	} location;
	size_t len;
	enum {transfer_IN_PLACE, transfer_BUFFERED} type;
	bool ready;
};


static struct {
	volatile struct transfer_info transfers[CONSOLE_MAX_QUEUE_TRANSFERS];
	volatile size_t current_transfer; //Where we will transmit if able
	volatile size_t last_transfer; //next empty spot

	volatile uint32_t failed_transfers;
	uint8_t buf[CONSOLE_BUFFER_SIZE+CONSOLE_MAX_PRINTF_LENGTH]; //Extra space
	volatile size_t buf_start_idx, buf_end_idx; //Start = start of valid data, end = end of valid data
} console __attribute__((section(".ccm_sram_all")));

void empty_console_memory(void){
	for(size_t i; i < sizeof(console); i++)((uint8_t *)&console)[i] = 0;
	for(int i = 0; i < CONSOLE_BUFFER_SIZE+CONSOLE_MAX_PRINTF_LENGTH; i++)console.buf[i]='F';
	console.current_transfer = 0;
	console.last_transfer = 0;
	for(int i = 0; i < CONSOLE_MAX_QUEUE_TRANSFERS; i++) console.transfers[i].ready=false;
}

void console_main(void *ignore __attribute__((unused))){
	console_thread = xTaskGetHandle("console_main_thread");

	while(true) {
		while(console.current_transfer != console.last_transfer) {

			while(!console.transfers[console.current_transfer].ready || UART_CheckIdleState(&CONSOLE_UART_HANDLE) != HAL_OK){
				vTaskSuspend(NULL); //We will be woken by the uart ISR or other console functions when there is something to do
			};

			switch(console.transfers[console.current_transfer].type){
			case transfer_IN_PLACE:
				while(HAL_UART_Transmit_DMA(&CONSOLE_UART_HANDLE, console.transfers[console.current_transfer].location.buf_in_place, console.transfers[console.current_transfer].len) != HAL_OK){
					vTaskSuspend(NULL);
				}
			break;

			case transfer_BUFFERED:
				while(HAL_UART_Transmit_DMA(&CONSOLE_UART_HANDLE, &console.buf[console.transfers[console.current_transfer].location.buf_index], console.transfers[console.current_transfer].len) != HAL_OK) {
					vTaskSuspend(NULL);
				}
				taskENTER_CRITICAL();
				console.buf_start_idx += console.transfers[console.current_transfer].len;
				console.buf_start_idx = console.buf_start_idx > CONSOLE_BUFFER_SIZE ? 0 : console.buf_start_idx;
				taskEXIT_CRITICAL();

			break;
			}
			taskENTER_CRITICAL();
			console.current_transfer++;
			console.current_transfer %= CONSOLE_MAX_QUEUE_TRANSFERS;
			taskEXIT_CRITICAL();
		}
		if(console.failed_transfers) {
			console_print_time(), console_printf("Console: \e[1;31m%d Failed Transfers\n\e[0m", console.failed_transfers);
			taskENTER_CRITICAL();
			console.failed_transfers = 0;
			taskEXIT_CRITICAL();
			continue;
		}
		vTaskSuspend(NULL);
	}
	vTaskDelete(NULL); //LEAVE AT THE END
}


//Write serial data to console directly. Returns true on success. Blocks until success or timeout
bool console_write_blocked(const uint8_t *const buf, size_t len, TickType_t timeout_ticks){
	TickType_t start_time = xTaskGetTickCount();
	bool result;
	while(!(result = console_write(buf, len)) && xTaskGetTickCount()-start_time < timeout_ticks) {
		//Attempting to add something to the uart queue requires an extremely high impact critical section.
		//This critical section must not be repeated at a high rate -> delay for at least a few ticks to allow other tasks to run
		vTaskDelay(CONSOLE_BLOCKING_DELAY_TICKS);
	}
	return result;
}

//Write serial data to console directly. Returns true on success
bool console_write(const uint8_t * const buf, size_t len){
	size_t transfer_idx;
	{	//critical section
		UBaseType_t nvic_state = taskENTER_CRITICAL_FROM_ISR();

		//Check if we can queue another transfer
		if( console.last_transfer == (console.current_transfer+CONSOLE_MAX_QUEUE_TRANSFERS-1)%CONSOLE_MAX_QUEUE_TRANSFERS){
			console.failed_transfers++;
			taskEXIT_CRITICAL_FROM_ISR(nvic_state);
			return false;
		}

		//Get the index that we can use, update console's index
		transfer_idx = console.last_transfer;
		console.last_transfer = (console.last_transfer+1)%CONSOLE_MAX_QUEUE_TRANSFERS;
		console.transfers[transfer_idx].ready = false;

		taskEXIT_CRITICAL_FROM_ISR(nvic_state);
	}

	console.transfers[transfer_idx].type = transfer_IN_PLACE;
	console.transfers[transfer_idx].len = len;
	console.transfers[transfer_idx].location.buf_in_place = buf;
	console.transfers[transfer_idx].ready = true;

	//Wake up the console writer thread
	xTaskResumeFromISR(console_thread);
	return true;
}

bool console_print_blocked(const char * const str, TickType_t timeout_ticks){
	TickType_t start_time = xTaskGetTickCount();
	bool result;
	while(!(result = console_print(str)) && xTaskGetTickCount()-start_time < timeout_ticks) {
		//Attempting to add something to the uart queue requires an extremely high impact critical section.
		//This critical section must not be repeated at a high rate -> delay for at least a few ticks to allow other tasks to run
		vTaskDelay(CONSOLE_BLOCKING_DELAY_TICKS);
	}
	return result;
}

bool console_print(const char * const str){
	size_t len = strlen(str);
	return console_write((const uint8_t * const)str, len);
}

bool attempt_reserve_buffer(size_t *transfer_idx, size_t *buf_idx, size_t len) {
	//critical section. Determine if we will fit into the buffer, and it is available
	UBaseType_t nvic_state = taskENTER_CRITICAL_FROM_ISR();

	//Check if we can queue another transfer
	if( console.last_transfer == (console.current_transfer+CONSOLE_MAX_QUEUE_TRANSFERS-1)%CONSOLE_MAX_QUEUE_TRANSFERS) {
		taskEXIT_CRITICAL_FROM_ISR(nvic_state);
		return false;
	}

	//Check if there is enough buffer space for this message.
	//We are "before" the start index
	if( (console.buf_end_idx < console.buf_start_idx) && ((console.buf_start_idx - console.buf_end_idx) < len) ) {
		taskEXIT_CRITICAL_FROM_ISR(nvic_state);
		return false;
	}

	//Get the index that we can use
	*transfer_idx = console.last_transfer;
	console.last_transfer = (console.last_transfer+1)%CONSOLE_MAX_QUEUE_TRANSFERS;
	console.transfers[*transfer_idx].ready = false;

	*buf_idx = console.buf_end_idx;

	console.buf_end_idx += len;
	//If we went off the end of the buffer (safe) then wrap
	console.buf_end_idx = console.buf_end_idx > (CONSOLE_BUFFER_SIZE-1) ? 0 : console.buf_end_idx;

	taskEXIT_CRITICAL_FROM_ISR(nvic_state);
	return true;
}

bool console_printf_blocked(const char * const fmt, TickType_t timeout_ticks, ...){
	TickType_t start_time = xTaskGetTickCount();
	char buffer[CONSOLE_MAX_PRINTF_LENGTH];
	va_list args;
	va_start(args, timeout_ticks);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	size_t len = strlen(buffer);

	size_t transfer_idx;
	size_t buf_idx;
	bool result;

	while( (!( result = attempt_reserve_buffer(&transfer_idx, &buf_idx, len))) && ( (xTaskGetTickCount()-start_time) < timeout_ticks) ){
		vTaskDelay(CONSOLE_BLOCKING_DELAY_TICKS);
	}

	strncpy(console.buf+buf_idx, buffer, len);
	console.transfers[transfer_idx].type = transfer_BUFFERED;
	console.transfers[transfer_idx].len = len;
	console.transfers[transfer_idx].location.buf_index = buf_idx;
	console.transfers[transfer_idx].ready = true;

	xTaskResumeFromISR(console_thread);

	return result;
}


bool console_printf(const char * const fmt, ...) {
	char buffer[CONSOLE_MAX_PRINTF_LENGTH];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	size_t len = strlen(buffer);

	size_t transfer_idx;
	size_t buf_idx;

	if (!attempt_reserve_buffer(&transfer_idx, &buf_idx, len)) {
		console.failed_transfers++;
		return false;
	}

	strncpy(console.buf+buf_idx, buffer, len);
	console.transfers[transfer_idx].type = transfer_BUFFERED;
	console.transfers[transfer_idx].len = len;
	console.transfers[transfer_idx].location.buf_index = buf_idx;
	console.transfers[transfer_idx].ready = true;

	xTaskResumeFromISR(console_thread);
	return true;
}


bool console_print_time(void){
	return console_printf("[\e[1;32m%7lu.%03d\e[0m] ", (unsigned long)((xTaskGetTickCountFromISR())/1000), (int)((xTaskGetTickCountFromISR())%1000) );
}

bool console_print_time_blocked(void){
	return console_printf_blocked("[\e[1;32m%7lu.%03d\e[0m] ", 900, (unsigned long)((xTaskGetTickCount())/1000), (int)((xTaskGetTickCount())%1000) );
}
