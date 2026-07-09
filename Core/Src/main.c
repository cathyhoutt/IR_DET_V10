#include "main.h"
#include "adc.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"



// USART2 DMA????
uint8_t usart2_rx_buf[USART2_RX_BUF_LEN] = {0};
uint16_t usart2_rx_len = 0;
uint8_t usart2_frame_flag = 0;

// USART1 ???????
uint8_t usart1_rx_buf[USART1_RX_BUF_LEN] = {0};
uint16_t usart1_rx_cnt = 0;
uint8_t usart1_frame_flag = 0;

uint16_t adc_pd_val = 0;
float env_temp = 0.0f;

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;


// ADG719?????? PA0
#define ADG719_SW_PIN  GPIO_PIN_0
#define ADG719_SW_PORT GPIOA

// ????????
void ADG719_Enable(void);
void ADG719_Disable(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_DMA_Init();

    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_ADC1_Init();

    // USART2 ??DMA?? + ????
		HAL_NVIC_DisableIRQ(DMA1_Channel6_IRQn);
    HAL_UART_Receive_DMA(&huart2, usart2_rx_buf, USART2_RX_BUF_LEN);
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
	  __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
	  volatile uint16_t temp = 99;
	  dwin_send_vp_82(0x1000, temp);
		// ??DMA??
		//HAL_UART_Receive_DMA(&huart2, usart2_rx_buf, USART2_RX_BUF_LEN);

    // USART1 ???????
    HAL_UART_Receive_IT(&huart1, &usart1_rx_buf[0], 1);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);

    while (1)
    {
        // ??PD ADC??
        adc_pd_val = ADC_ReadPD();
        // ??PC9 DS18B20??
        env_temp = DS18B20_GetTemp();

        // ???DMA???
        if (usart2_frame_flag == 1)
        {
            usart2_frame_flag = 0;
            UART2_SendStr("DGUS DMA Recv OK\r\n");
            // ??????DMA
            memset(usart2_rx_buf, 0, USART2_RX_BUF_LEN);
            HAL_UART_Receive_DMA(&huart2, usart2_rx_buf, USART2_RX_BUF_LEN);
        }

        // ????1????
        if (usart1_frame_flag == 1)
        {
            usart1_frame_flag = 0;
            UART1_SendStr("IR Data Recv\r\n");
            memset(usart1_rx_buf, 0, USART1_RX_BUF_LEN);
            usart1_rx_cnt = 0;
            HAL_UART_Receive_IT(&huart1, &usart1_rx_buf[0], 1);
        }

        HAL_Delay(100);
    }
}

// ADC??PA1 PD???
uint16_t ADC_ReadPD(void)
{
    uint16_t val = 0;
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return val;
}

void UART2_SendStr(char *str)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)str, strlen(str), 100);
}
void UART1_SendStr(char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), 100);
}

void HAL_UART_IdleCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2)
    {
        HAL_DMA_Abort(&hdma_usart2_rx);
        usart2_rx_len = USART2_RX_BUF_LEN - __HAL_DMA_GET_COUNTER(&hdma_usart2_rx);
        usart2_frame_flag = 1;
        memset(usart2_rx_buf, 0, USART2_RX_BUF_LEN);
        HAL_UART_Receive_DMA(&huart2, usart2_rx_buf, USART2_RX_BUF_LEN);
    }
    if(huart->Instance == USART1)
    {
        usart1_frame_flag = 1;
    }
}

// ????????? USART1
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        usart1_rx_cnt++;
        if (usart1_rx_cnt >= USART1_RX_BUF_LEN)
        {
            usart1_rx_cnt = 0;
            memset(usart1_rx_buf, 0, USART1_RX_BUF_LEN);
        }
        HAL_UART_Receive_IT(&huart1, &usart1_rx_buf[usart1_rx_cnt], 1);
    }
		if(huart->Instance == USART2)
    {
        // ?????????,????????????
        UART1_SendStr("USART2 RX DATA IN\n");
    }
}

// ADG719????(PA0???)
void ADG719_Enable(void)
{
    HAL_GPIO_WritePin(ADG719_SW_PORT, ADG719_SW_PIN, GPIO_PIN_SET);
}
// ADG719????(PA0???)
void ADG719_Disable(void)
{
    HAL_GPIO_WritePin(ADG719_SW_PORT, ADG719_SW_PIN, GPIO_PIN_RESET);
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
