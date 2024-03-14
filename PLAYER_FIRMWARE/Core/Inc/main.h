/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

//Defines Needed for SD card
#define SD_SPI_HANDLE hspi4
#define SD_CS_GPIO_Port GPIOE
#define SD_CS_Pin GPIO_PIN_4

//Defines needed for Audio out on green
extern DAC_HandleTypeDef hdac1;

#define AUD_GREEN_DAC hdac1

extern TIM_HandleTypeDef htim6;

#define AUDIO_44_1_KHZ_TIMER htim6

extern uint8_t audio_buffer[49152];

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TFT_NRST_Pin GPIO_PIN_6
#define TFT_NRST_GPIO_Port GPIOB
#define BRIDGE_CLK_Pin GPIO_PIN_1
#define BRIDGE_CLK_GPIO_Port GPIOD
#define AUD_ORANGE_S_UNBUFFERED_IN_Pin GPIO_PIN_9
#define AUD_ORANGE_S_UNBUFFERED_IN_GPIO_Port GPIOA
#define AUD_ORANGE_R_OUTPUT_EN_Pin GPIO_PIN_9
#define AUD_ORANGE_R_OUTPUT_EN_GPIO_Port GPIOF
#define MICRO_SD_CS_Pin GPIO_PIN_9
#define MICRO_SD_CS_GPIO_Port GPIOC
#define ROT_A_Pin GPIO_PIN_2
#define ROT_A_GPIO_Port GPIOC
#define AUD_GREEN_L_OUTPUT_EN_Pin GPIO_PIN_0
#define AUD_GREEN_L_OUTPUT_EN_GPIO_Port GPIOC
#define AUD_GREEN_R_OUTPUT_EN_Pin GPIO_PIN_1
#define AUD_GREEN_R_OUTPUT_EN_GPIO_Port GPIOC
#define AUD_BLACK_T_UNBUFFERED_IN_Pin GPIO_PIN_14
#define AUD_BLACK_T_UNBUFFERED_IN_GPIO_Port GPIOD
#define ROT_S2_Pin GPIO_PIN_6
#define ROT_S2_GPIO_Port GPIOC
#define ROT_B_Pin GPIO_PIN_3
#define ROT_B_GPIO_Port GPIOC
#define AUD_GRN_MIC_POSTAMP_Pin GPIO_PIN_1
#define AUD_GRN_MIC_POSTAMP_GPIO_Port GPIOA
#define AUD_ORANGE_L_OUTPUT_EN_Pin GPIO_PIN_2
#define AUD_ORANGE_L_OUTPUT_EN_GPIO_Port GPIOF
#define TFT_SPI_DC_Pin GPIO_PIN_0
#define TFT_SPI_DC_GPIO_Port GPIOA
#define AUD_BLACK_R1_UNBUFFERED_IN_Pin GPIO_PIN_13
#define AUD_BLACK_R1_UNBUFFERED_IN_GPIO_Port GPIOD
#define AUD_ORANGE_L_PREAMP_AUDIO_Pin GPIO_PIN_2
#define AUD_ORANGE_L_PREAMP_AUDIO_GPIO_Port GPIOA
#define AUD_GREEN_L_PREAMP_AUDIO_Pin GPIO_PIN_4
#define AUD_GREEN_L_PREAMP_AUDIO_GPIO_Port GPIOA
#define AUD_BLACK_S_UNBUFFERED_IN_Pin GPIO_PIN_8
#define AUD_BLACK_S_UNBUFFERED_IN_GPIO_Port GPIOE
#define PINK_BUFFERED_ANALOG_IN_R1_Pin GPIO_PIN_11
#define PINK_BUFFERED_ANALOG_IN_R1_GPIO_Port GPIOD
#define AUD_GREEN_R_PREAMP_AUDIO_Pin GPIO_PIN_5
#define AUD_GREEN_R_PREAMP_AUDIO_GPIO_Port GPIOA
#define TFT_LED_LVL_Pin GPIO_PIN_6
#define TFT_LED_LVL_GPIO_Port GPIOA
#define ROT_S1_Pin GPIO_PIN_5
#define ROT_S1_GPIO_Port GPIOC
#define PINK_BUFFERED_ANALOG_IN_T_Pin GPIO_PIN_12
#define PINK_BUFFERED_ANALOG_IN_T_GPIO_Port GPIOD
#define PINK_BUFFERED_ANALOG_IN_S_Pin GPIO_PIN_7
#define PINK_BUFFERED_ANALOG_IN_S_GPIO_Port GPIOA
#define ROT_C_Pin GPIO_PIN_4
#define ROT_C_GPIO_Port GPIOC
#define AUD_ORANGE_PREAMP_AUDIO_Pin GPIO_PIN_1
#define AUD_ORANGE_PREAMP_AUDIO_GPIO_Port GPIOB
#define VOLUME_ANALOG_IN_Pin GPIO_PIN_8
#define VOLUME_ANALOG_IN_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
