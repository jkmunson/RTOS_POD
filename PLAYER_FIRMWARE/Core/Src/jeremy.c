#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_dma.h"
#include "stm32g4xx_hal_tim.h"
#include "main.h"
#include "console.h"
#include "buttons.h"


#include <string.h>
#include <stdio.h>

void print_all_ports(void){
	console_print("PORT A\t\tPORT B\t\tPORT C\t\tPORT D\t\tPORT E\t\tPORT F\n");
	console_printf("0x%08x\t0x%08x\t0x%08x\t0x%08x\t0x%08x\t0x%08x\t\n",
			GPIOA->IDR & 0x1FFF,
			GPIOB->IDR & ~(1<<4),
			GPIOC->IDR & ~(1<<12),
			GPIOD->IDR & ~(1<<2),
			GPIOE->IDR,
			GPIOF->IDR);
}

void jeremy_main(void *ignore __attribute__ ((unused))) {
	vTaskDelay(100);
	console_print("Testing Port B\n");
	console_print("Initial Pin Values:\n");
	print_all_ports();
	/*
	// GPIO E TESTING
		for(int pin = 0; pin < 16; pin++){
					console_printf("PortE Pin %2d\n", pin);
					if(pin == 77) continue;
					char a = 0;
					while(a != 'x') {
						HAL_UART_Receive(&huart5, &a, 1, HAL_MAX_DELAY);
					}
					GPIOE->MODER = GPIOE->MODER&~(3<<(pin*2)) | (1<<(pin*2));
					for(int i = 0; i < 50; i++){
						GPIOE->ODR ^= GPIO_ODR_OD0 << (pin);
						print_all_ports();
						vTaskDelay(10);
					}
					GPIOE->MODER = GPIOE->MODER&~(3<<(pin*2));

			}*/

/*
	// GPIO D TESTING
		for(int pin = 0; pin < 16; pin++){
					console_printf("PortD Pin %2d\n", pin);
					if(pin == 2) continue;
					char a = 0;
					while(a != 'x') {
						HAL_UART_Receive(&huart5, &a, 1, HAL_MAX_DELAY);
					}
					GPIOD->MODER = GPIOD->MODER&~(3<<(pin*2)) | (1<<(pin*2));
					for(int i = 0; i < 50; i++){
						GPIOD->ODR ^= GPIO_ODR_OD0 << (pin);
						print_all_ports();
						vTaskDelay(10);
					}
					GPIOD->MODER = GPIOD->MODER&~(3<<(pin*2));

			}*/

/*
	// GPIO C TESTING
	for(int pin = 0; pin < 16; pin++){
				console_printf("PortC Pin %2d\n", pin);
				if(pin == 12) continue;
				char a = 0;
				while(a != 'x') {
					HAL_UART_Receive(&huart5, &a, 1, HAL_MAX_DELAY);
				}
				GPIOC->MODER = GPIOC->MODER&~(3<<(pin*2)) | (1<<(pin*2));
				for(int i = 0; i < 50; i++){
					GPIOC->ODR ^= GPIO_ODR_OD0 << (pin);
					print_all_ports();
					vTaskDelay(10);
				}
				GPIOC->MODER = GPIOC->MODER&~(3<<(pin*2));

		}*/


	// GPIO B TESTING
	for(int pin = 0; pin < 16; pin++){
			console_printf("PortB Pin %2d\n", pin);
			if(pin == 3 || pin == 4) continue;
			char a = 0;
			while(a != 'x') {
				HAL_UART_Receive(&huart5, &a, 1, HAL_MAX_DELAY);
			}
			GPIOB->MODER = GPIOB->MODER&~(3<<(pin*2)) | (1<<(pin*2));
			for(int i = 0; i < 50; i++){
				GPIOB->ODR ^= GPIO_ODR_OD0 << (pin);
				print_all_ports();
				vTaskDelay(10);
			}
			GPIOB->MODER = GPIOB->MODER&~(3<<(pin*2));

	}


	// GPIO A TESTING
	for(int pin = 0; pin < 13; pin++){
		console_printf("PortA Pin %2d\n", pin);
		char a = 0;
		while(a != 'x') {
			HAL_UART_Receive(&huart5, &a, 1, HAL_MAX_DELAY);
		}
		GPIOA->MODER = GPIOA->MODER&~(3<<(pin*2)) | (1<<(pin*2));
		for(int i = 0; i < 50; i++){
			GPIOA->ODR ^= GPIO_ODR_OD0 << (pin);
			print_all_ports();
			vTaskDelay(10);
		}
		GPIOA->MODER = GPIOA->MODER&~(3<<(pin*2));

	}

	vTaskSuspend(NULL); //LEAVE AT THE END
	vTaskDelete(NULL);
}
