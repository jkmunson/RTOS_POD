#include "console.h"
#include "main.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
/*
void user_colors(char buf) {
	//Buf is guarenteed to be null-terminated, as it comes from vsnprintf
	if(strcmp(buf, "Jeremy:", 7)) HAL_UART_Transmit(&CONSOLE_UART_HANDLE, "\e[1;32m]", strlen("\e[1;32m"), CONSOLE_MAX_WAIT);


}*/


void console_printf(const char *fmt, ...) {
	char buffer[CONSOLE_MAX_STRING_LEN];

	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	//user_colors(buf);

	int len = strlen(buffer);
	HAL_UART_Transmit(&CONSOLE_UART_HANDLE, (uint8_t *)buffer, len, -1);
}

//void print_string(const char const *str) { HAL_UART_Transmit(&huart2, str, strlen(str), 99999); }
