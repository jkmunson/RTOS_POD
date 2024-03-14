#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_dac.h"
#include "stm32g4xx_hal_dma.h"
#include "stm32g4xx_hal_tim.h"
#include "main.h"

extern DMA_HandleTypeDef hdma_dac1_ch1;
#define AUD_GREEN_L_DMA hdma_dac1_ch1;

//DAC_DHR12LD

void jeremy_main(void *ignore) {
	HAL_TIM_Base_Start(&AUDIO_44_1_KHZ_TIMER);
	HAL_DAC_Start(&AUD_GREEN_DAC, DAC_CHANNEL_1);
	HAL_DAC_Start(&AUD_GREEN_DAC, DAC_CHANNEL_2);

	HAL_DAC_Start_DMA(&AUD_GREEN_DAC, DAC_CHANNEL_1, (uint32_t*)audio_buffer, 32, DAC_ALIGN_12B_R);

	vTaskSuspend(xTaskGetCurrentTaskHandle()); //LEAVE AT THE END
	vTaskDelete(NULL);
}
