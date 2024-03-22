#include <stdint.h>
#include <stdbool.h>

#include "stm32g4xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "buttons.h"

volatile bool up_pressed, down_pressed, left_pressed, right_pressed;
volatile bool up_held, down_held, left_held, right_held;

#define DEBOUNCE_MIN_WAIT_TICKS 15

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	static struct {
		TickType_t last_time;
		volatile bool *pressed, *held;
		bool has_released;
	} 	right_button 	= 	{0, &right_pressed,	&right_held, true},
		up_button 		= 	{0, &up_pressed, 	&up_held, 	 true},
		down_button 	= 	{0, &down_pressed, 	&down_held,  true},
		left_button 	= 	{0, &left_pressed, 	&left_held,  true},
		rot_enc_button  =	{0, &right_pressed,	&right_held, true},
		*current_button = NULL;

	const enum{FALLING, RISING} edge_type = (uint8_t)HAL_GPIO_ReadPin(GPIOA, GPIO_Pin);

	switch(GPIO_Pin) {
		case RIGHT_BUTTON_Pin:
			current_button = &right_button;
		break;

		case UP_BUTTON_Pin:
			current_button = &up_button;
		break;

		case DOWN_BUTTON_Pin:
			current_button = &down_button;
		break;

		case LEFT_BUTTON_Pin:
			current_button = &left_button;
		break;
		case ROT_S1_Pin:
			current_button = &rot_enc_button;
		break;
	}

	TickType_t current_time = xTaskGetTickCountFromISR();

	if( (current_time - current_button->last_time) < DEBOUNCE_MIN_WAIT_TICKS ) return;

	switch(edge_type){
		case FALLING:
			*current_button->pressed = true;
			*current_button->held = true;
			current_button->last_time = current_time;
			current_button->has_released = false;
		break;

		case RISING:
			*current_button->held = false;
			current_button->last_time = current_time;
			current_button->has_released = true;
		break;
	}
}

