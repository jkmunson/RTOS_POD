#pragma once
#include <stdbool.h>
bool console_write(const uint8_t *const buf, size_t len);
bool console_print(const char * const string);
bool console_printf(const char * const fmt, ...);
void console_main(void * ignore);
