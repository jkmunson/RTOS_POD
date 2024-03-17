#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#include <FreeRTOS.h>
#include <semphr.h>

#include "console.h"
#include "main.h"

/*
void user_colors(char buf) {
	//Buf is guaranteed to be null-terminated, as it comes from vsnprintf
	//if(strcmp(buf, "Jeremy:", 7)) HAL_UART_Transmit(&CONSOLE_UART_HANDLE, "\e[1;32m]", strlen("\e[1;32m"), CONSOLE_MAX_WAIT);
}*/

//To simplify printf buffer logic, we must always have enough space to print the maximum size regardless of where we are in the buffer
#define CONSOLE_MAX_PRINTF_LENGTH 256
#define CONSOLE_BUFFER_SIZE 2048
#define CONSOLE_MAX_QUEUE_TRANSFERS 127
#define CONSOLE_MAX_QUEUE_TRANSFER_INDEX (CONSOLE_MAX_QUEUE_TRANSFERS-1)


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

void console_main(void *ignore __attribute__((unused))){
	while(true) {
		while(console.current_transfer != console.last_transfer) {
			if(console.transfers[console.current_transfer].type == transfer_IN_PLACE) {
				while(!console.transfers[console.current_transfer].ready){};
				HAL_UART_Transmit(&CONSOLE_UART_HANDLE, console.transfers[console.current_transfer].location.buf_in_place, console.transfers[console.current_transfer].len , 0xFFFF);
				console.current_transfer++;
				console.current_transfer %= CONSOLE_MAX_QUEUE_TRANSFERS;
			}
		}
		vTaskDelay(10);
	}
	vTaskSuspend(xTaskGetCurrentTaskHandle()); //LEAVE AT THE END
	vTaskDelete(NULL);
}


//Write serial data to console directly. Returns true on success
bool console_write(const uint8_t * const buf, size_t len){
	size_t transfer_idx;
	{	//critical section
		UBaseType_t nvic_state = taskENTER_CRITICAL_FROM_ISR();

		//Check if we can queue another transfer
		if( (console.last_transfer == (console.current_transfer-1)) ||
			((console.current_transfer == CONSOLE_MAX_QUEUE_TRANSFER_INDEX) && (console.last_transfer == 0) )) {
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

	//TODO: wake the transfer thread
	return true;
}

bool console_print(const char * const string){
	size_t len = strlen(string);
	return console_write((const uint8_t * const)string, len);
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
	{	//critical section. Determine if we will fit into the buffer, and it is available. Looks like a lot, but should be <30 clocks typically
		UBaseType_t nvic_state = taskENTER_CRITICAL_FROM_ISR();

		//Check if we can queue another transfer
		if( (console.last_transfer == (console.current_transfer-1)) ||
			((console.current_transfer == CONSOLE_MAX_QUEUE_TRANSFER_INDEX) && (console.last_transfer == 0) )) {
			console.failed_transfers++;
			taskEXIT_CRITICAL_FROM_ISR(nvic_state);
			return false;
		}

		//Get the index that we can use, update console's index
		transfer_idx = (console.last_transfer)%CONSOLE_MAX_QUEUE_TRANSFERS;

		//Check if there is enough buffer space for this message.
		//We are "before" the start index
		if( (console.buf_end_idx < console.buf_start_idx) && ((console.buf_start_idx - console.buf_end_idx) < len) ) {
			console.failed_transfers++;
			taskEXIT_CRITICAL_FROM_ISR(nvic_state);
			return false;
		}

		//We are "after" the start index
		if( (console.buf_end_idx + len) >= (CONSOLE_BUFFER_SIZE + CONSOLE_MAX_PRINTF_LENGTH) ) {
			console.failed_transfers++;
			taskEXIT_CRITICAL_FROM_ISR(nvic_state);
			return false;
		}

		buf_idx = console.buf_end_idx;

		console.buf_end_idx += len;
		//If we went off the end of the buffer (safe) then wrap
		console.buf_end_idx = console.buf_end_idx > CONSOLE_BUFFER_SIZE ? 0 : console.buf_end_idx;

		console.last_transfer = transfer_idx+1;
		console.transfers[transfer_idx].ready = false;
		taskEXIT_CRITICAL_FROM_ISR(nvic_state);
	}

	strncpy(console.buf+buf_idx, buffer, len);
	console.transfers[transfer_idx].type = transfer_BUFFERED;
	console.transfers[transfer_idx].len = len;
	console.transfers[transfer_idx].location.buf_index = buf_idx;
	console.transfers[transfer_idx].ready = true;

	//Wake the transfer thread
	return true;
}



