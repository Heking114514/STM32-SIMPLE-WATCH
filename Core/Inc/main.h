/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

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

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ADC_Pin GPIO_PIN_0
#define ADC_GPIO_Port GPIOA
#define KEY3_Pin GPIO_PIN_1
#define KEY3_GPIO_Port GPIOA
#define YX_RX_Pin GPIO_PIN_2
#define YX_RX_GPIO_Port GPIOA
#define YX_TX_Pin GPIO_PIN_3
#define YX_TX_GPIO_Port GPIOA
#define W25_CS_Pin GPIO_PIN_4
#define W25_CS_GPIO_Port GPIOA
#define W25_CLK_Pin GPIO_PIN_5
#define W25_CLK_GPIO_Port GPIOA
#define W25_DO_Pin GPIO_PIN_6
#define W25_DO_GPIO_Port GPIOA
#define W25_DI_Pin GPIO_PIN_7
#define W25_DI_GPIO_Port GPIOA
#define KEY2_Pin GPIO_PIN_0
#define KEY2_GPIO_Port GPIOB
#define KEY1_Pin GPIO_PIN_1
#define KEY1_GPIO_Port GPIOB
#define BAT_ADC_EN_Pin GPIO_PIN_12
#define BAT_ADC_EN_GPIO_Port GPIOB
#define MP3VCC_Pin GPIO_PIN_11
#define MP3VCC_GPIO_Port GPIOA
#define APK_OFF_Pin GPIO_PIN_12
#define APK_OFF_GPIO_Port GPIOA
#define LED_Pin GPIO_PIN_6
#define LED_GPIO_Port GPIOB
#define KEY4_Pin GPIO_PIN_7
#define KEY4_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
