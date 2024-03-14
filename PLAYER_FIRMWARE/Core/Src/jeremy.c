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

size_t audio_dma_current_index;

void update_green_DMA_addr(TIM_HandleTypeDef *htim){
	audio_dma_current_index = (audio_dma_current_index+4)%AUD_BUFFER_SIZE;
}


void jeremy_main(void *ignore) {
	HAL_TIM_RegisterCallback(&AUDIO_44_1_KHZ_TIMER, HAL_TIM_PERIOD_ELAPSED_CB_ID, update_green_DMA_addr);

	HAL_TIM_Base_Start(&AUDIO_44_1_KHZ_TIMER);

	HAL_DAC_Start(&AUD_GREEN_DAC, DAC_CHANNEL_1);
	HAL_DAC_Start(&AUD_GREEN_DAC, DAC_CHANNEL_2);

	HAL_DAC_Start_DMA(&AUD_GREEN_DAC, DAC_CHANNEL_1, (uint32_t*)audio_buffer, (AUD_BUFFER_SIZE>>2)-1 , DAC_ALIGN_12B_L);
	HAL_DAC_Start_DMA(&AUD_GREEN_DAC, DAC_CHANNEL_2, (uint32_t*)(audio_buffer+2), (AUD_BUFFER_SIZE>>2)-1 , DAC_ALIGN_12B_L);

	audio_dma_current_index = 0; //Reset the index now, in case it had counted up before we started DMA. its okay to be a ways behind the dma.

	vTaskSuspend(xTaskGetCurrentTaskHandle()); //LEAVE AT THE END
	vTaskDelete(NULL);
}
