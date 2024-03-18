#pragma once
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"


bool console_write(const uint8_t *const buf, size_t len);
bool console_print(const char * const str);
bool console_printf(const char * const fmt, ...);

bool console_write_blocked(const uint8_t *const buf, size_t len, TickType_t timeout_ticks);
bool console_print_blocked(const char * const str, TickType_t timeout_ticks);
bool console_printf_blocked(const char * const fmt, TickType_t timeout_ticks, ...);

bool console_print_time(void);

void console_main(void * ignore);
void empty_console_memory(void);

extern TaskHandle_t console_thread;
