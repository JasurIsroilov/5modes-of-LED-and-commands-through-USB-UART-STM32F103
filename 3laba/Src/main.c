/**
  ******************************************************************************
  * @file    main.c
  * @author  Vladimir Leonidov
  * @version V1.0.0
  * @date    15.11.2021
  * @brief   Лабораторная работа №3 - "USART, DMA"
  *			 Отладочная плата: STM32F103 Nucleo
  *
  *			 Реализованы отправка и приём данных по USART.
  *			 Разработан простейший обработчик команд.
  *
  ******************************************************************************
  */

#include "main.h"


char RxBuffer[RX_BUFF_SIZE];					//Буфер приёма USART
char TxBuffer[TX_BUFF_SIZE];					//Буфер передачи USART
bool ComReceived;								//Флаг приёма строки данных
int w=2;
bool led_flag=false;
bool blink = false;

/**
  * @brief  Обработчик прерывания от USART2
  * @param  None
  * @retval None
  */
void USART2_IRQHandler(void)
{
	if ((USART2->SR & USART_SR_RXNE)!=0)		//Прерывание по приёму данных?
	{
		uint8_t pos = strlen(RxBuffer);			//Вычисляем позицию свободной ячейки

		RxBuffer[pos] = USART2->DR;				//Считываем содержимое регистра данных

		if ((RxBuffer[pos]==0x0D)) //&& (RxBuffer[pos-1]== 0x0D))							//Если это символ конца строки
		{
			ComReceived = true;					//- выставляем флаг приёма строки
			return;								//- и выходим
		}
	}
}

/**
  * @brief  Подпрограмма обработчика прерывания
  *			по переполнению тайемера TIM2
  * @param  None
  * @retval None
  */
void TIM2_IRQHandler(void)
{
	TIM2->SR &= ~TIM_SR_UIF;			//Сброс флага переполнения
	if((w>0)&&(w<10))
	{
		if(led_flag)
		{
			LED_SWAP();
			TIM2->ARR = (10-w+1)-1;
			led_flag = false;
		}
		else
		{
			LED_SWAP();
			TIM2->ARR = (w+1)-1;
			led_flag = true;
		}
	}
	else if(w==10)
	{
		LED_ON();
		led_flag = true;
	}
	else
	{
		LED_OFF();
		led_flag = false;
	}
	//LED_SWAP();
}


void TIM3_IRQHandler(void)
{
	TIM3->SR &= ~TIM_SR_UIF;			//Сброс флага переполнения
	TIM3->CNT=0;
	if(blink)
	{
		blink = false;
		TIM2->CNT = 0;
		TIM2->CR1 |= TIM_CR1_CEN;
		//LED_OFF();
		//led_flag = false;
		//TIM2->CR1 &= ~TIM_CR1_CEN;
		//TIM2->CNT = 0;
	}
	else
	{
		blink = true;
		TIM2->CR1 &= ~TIM_CR1_CEN;
		TIM2->CNT = 0;
		LED_OFF();
		led_flag = false;
	}
	//LED_SWAP();
}
/**
  * @brief  Подпрограмма обработчика прерывания
  *			по внешнему импульсу на выводах (EXTI15-EXTI10)
  *			Нас интересует EXTI13, там висит кнопка.
  * @param  None
  * @retval None
  */
void EXTI0_IRQHandler(void)
{
	// Т.к. этот обработчик вызывается, если произшло одно из прерываний EXTI15-EXI10,
	// нужно проверить, кто из них его вызвал.
	if (EXTI->PR & EXTI_PR_PR0) 		// нас интересует EXTI0
	{
		EXTI->PR |= EXTI_PR_PR0;
		delay(10000);					//Задержка для защиты от дребезга контактов
		w+=2;
		if(w>10)
		{
			w-=2;
		}
		TIM2->CNT=0;
		//TIM2->CR1 ^= TIM_CR1_CEN;		//Инвертируем состояние таймера
	}
}

void EXTI1_IRQHandler(void)
{
	// Т.к. этот обработчик вызывается, если произшло одно из прерываний EXTI15-EXI10,
	// нужно проверить, кто из них его вызвал.
	if (EXTI->PR & EXTI_PR_PR1) 		// нас интересует EXTI13
	{
		EXTI->PR |= EXTI_PR_PR1;
		delay(10000);					//Задержка для защиты от дребезга контактов
		w-=2;
		if(w<0)
		{
			w+=2;
		}
		TIM2->CNT=0;
		//TIM2->CR1 ^= TIM_CR1_CEN;		//Инвертируем состояние таймера
	}
}

/**
  * @brief  Инициализация портов ввода-вывода
  * @param  None
  * @retval None
  */
void initPorts(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;	//включить тактирование GPIOB
	//очистка полей
	GPIOB->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);
	//и конфигурация
	GPIOB->CRL |= GPIO_CRL_MODE0_1;		//PA5, выход 2МГц
}

/**
  * @brief  Инициализация прерывания от кнопки (PC13)
  * @param  None
  * @retval None
  */
void initButton(void)
{
	//Включить тактирование порта GPIOC и альтернативных функций
		RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
		/* Настраиваем PC13 на вход, альтернативная функция */
		// Сбрасываем биты конфигурации порта...
		//GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
		//...и выставляем так, как нам нужно
		//GPIOC->CRH |= GPIO_CRH_CNF13_1;			//Вход с подтяжкой
		//GPIOC->BSRR |= GPIO_BSRR_BS13;			//Подтяжка к Vdd

		/* Настройка самого прерывания */

		// Настройка альтернативных фукнций портов.
		// Настройки портов с 12 по 15 хранятся в регистре AFIO_EXTICR4.
		// Регистры объединены в массив AFIO->EXTICR, нумерация массива начинается с нулевого элемента.
		// Поэтому настройки AFIO_EXTICR4 хранятся в AFIO->EXTICR[3]
		/*AFIO->EXTICR[3] |= AFIO_EXTICR4_EXTI13_PC;

		EXTI->FTSR |= EXTI_FTSR_TR13;			//Прерывание по спаду импульса (при нажатии на кнопку)
		EXTI->IMR |= EXTI_IMR_MR13;				//Выставляем маску - EXTI13

		NVIC_EnableIRQ(EXTI15_10_IRQn);			//Разрешаем прерывание
		NVIC_SetPriority(EXTI15_10_IRQn, 0);/*/	//Выставляем приоритет
		//PA0
		GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0);
		GPIOA->CRL |= GPIO_CRL_CNF0_1;
		GPIOA->BSRR |= GPIO_BSRR_BS0;
		//PA1
		GPIOA->CRL &= ~(GPIO_CRL_MODE1 | GPIO_CRL_CNF1);
		GPIOA->CRL |= GPIO_CRL_CNF1_1;
		GPIOA->BSRR |= GPIO_BSRR_BS1;
		//AFIO
		AFIO->EXTICR[0] |= (AFIO_EXTICR1_EXTI0_PA | AFIO_EXTICR1_EXTI1_PA);
		EXTI->FTSR |= (EXTI_FTSR_TR0 | EXTI_FTSR_TR1);
		EXTI->IMR |= (EXTI_IMR_MR0 | EXTI_IMR_MR1);
		//NVIC PA0
		NVIC_EnableIRQ(EXTI0_IRQn);
		NVIC_SetPriority(EXTI0_IRQn, 0);
		//NVIC PA1
		NVIC_EnableIRQ(EXTI1_IRQn);
		NVIC_SetPriority(EXTI1_IRQn, 0);
}

/**
  * @brief  Инициализация таймера TIM6
  * @param  None
  * @retval None
  */
void initTIM2(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;		//Включить тактирование TIM6

	//Частота APB1 для таймеров = APB1Clk * 2 = 32МГц * 2 = 64МГц
	TIM2->PSC = 64000-1;					//Предделитель частоты (64МГц/64000 = 1кГц)
	TIM2->ARR = (500)-1;				//Модуль счёта таймера (1кГц/1000 = 1с)
	TIM2->DIER |= TIM_DIER_UIE;				//Разрешить прерывание по переполнению таймера
	TIM2->CR1 |= TIM_CR1_CEN;				//Включить таймер
	if((w>0)&&(w<10))
	{
		TIM2->CNT=0;
		TIM2->ARR = (w+1)-1;				//Модуль счёта таймера (1кГц/1000 = 1с)
		LED_ON();
		led_flag = true;
	}
	else if(w==10)
	{
		LED_ON();
		led_flag=true;
	}
	else
	{
		LED_OFF();
		led_flag=false;
	}

	NVIC_EnableIRQ(TIM2_IRQn);				//Рарзрешить прерывание от TIM2
	NVIC_SetPriority(TIM2_IRQn, 2);			//Выставляем приоритет
}


void initTIM3(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;		//Включить тактирование TIM6

	//Частота APB1 для таймеров = APB1Clk * 2 = 32МГц * 2 = 64МГц
	TIM3->PSC = 64000-1;					//Предделитель частоты (64МГц/64000 = 1кГц)
	TIM3->ARR = (1000)-1;				//Модуль счёта таймера (1кГц/1000 = 1с)
	TIM3->DIER |= TIM_DIER_UIE;				//Разрешить прерывание по переполнению таймера
	//TIM3->CR1 |= TIM_CR1_CEN;				//Включить таймер

	NVIC_EnableIRQ(TIM3_IRQn);				//Рарзрешить прерывание от TIM2
	NVIC_SetPriority(TIM3_IRQn, 1);			//Выставляем приоритет
}

/**
  * @brief  Инициализация систем тактирования:
  * 		Источник тактирования: HSI
  * 		Частота: 64МГц
  * @param  None
  * @retval None
  */
void initClk(void)
{
	// Enable HSI
	RCC->CR |= RCC_CR_HSION;
	while(!(RCC->CR & RCC_CR_HSIRDY)){};

	// Enable Prefetch Buffer
	FLASH->ACR |= FLASH_ACR_PRFTBE;

	// Flash 2 wait state
	FLASH->ACR &= ~FLASH_ACR_LATENCY;
	FLASH->ACR |= FLASH_ACR_LATENCY_2;

	// HCLK = SYSCLK
	RCC->CFGR |= RCC_CFGR_HPRE_DIV1;

	// PCLK2 = HCLK
	RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;

	// PCLK1 = HCLK/2
	RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

	// PLL configuration: PLLCLK = HSI/2 * 16 = 64 MHz
	RCC->CFGR &= ~RCC_CFGR_PLLSRC;
	RCC->CFGR |= RCC_CFGR_PLLMULL16;

	// Enable PLL
	RCC->CR |= RCC_CR_PLLON;

	// Wait till PLL is ready
	while((RCC->CR & RCC_CR_PLLRDY) == 0) {};

	// Select PLL as system clock source
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_PLL;

	// Wait till PLL is used as system clock source
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL){};
}

/**
  * @brief  Инициализация USART2
  * @param  None
  * @retval None
  */
void initUSART2(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;						//включить тактирование альтернативных ф-ций портов
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;					//включить тактирование UART2

	GPIOA->CRL &= ~(GPIO_CRL_MODE2 | GPIO_CRL_CNF2);		//PA2 на выход
	GPIOA->CRL |= (GPIO_CRL_MODE2_1 | GPIO_CRL_CNF2_1);

	GPIOA->CRL &= ~(GPIO_CRL_MODE3 | GPIO_CRL_CNF3);		//PA3 - вход
	GPIOA->CRL |= GPIO_CRL_CNF3_0;

	/*****************************************
	Скорость передачи данных - 115200
	Частота шины APB1 - 32МГц

	1. USARTDIV = 32'000'000/(16*115200) = 17.4
	2. 17 = 0x11
	3. 16*0.4 = 6
	4. Итого 0x116
	*****************************************/
	USART2->BRR = 0xD05;

	USART2->CR1 |= USART_CR1_RE | USART_CR1_TE | USART_CR1_UE;
	USART2->CR1 |= USART_CR1_RXNEIE;						//разрешить прерывание по приему байта данных

	NVIC_EnableIRQ(USART2_IRQn);
}


/**
  * @brief  Передача строки по USART2 без DMA
  * @param  *str - указатель на строку
  * @param  crlf - если true, перед отправкой добавить строке символы конца строки
  * @retval None
  */
void txStr(char *str, bool crlf)
{
	uint16_t i;

	if (crlf)												//если просят,
		strcat(str,"\n");									//добавляем символ конца строки

	for (i = 0; i < strlen(str); i++)
	{
		USART2->DR = str[i];								//передаём байт данных
		while ((USART2->SR & USART_SR_TC)==0) {};			//ждём окончания передачи
	}
}

/**
  * @brief  Обработчик команд
  * @param  None
  * @retval None
  */
void ExecuteCommand(void)
{
//	txStr(RxBuffer, false);
	memset(TxBuffer,0,sizeof(TxBuffer));					//Очистка буфера передачи

	/* Обработчик команд */
	if (strncmp(RxBuffer,"*IDN?",5) == 0)					//Это команда "*IDN?"
	{
		strcpy(TxBuffer,"Isroilov IU4-73B");					//Она самая, возвращаем строку идентификации
	}
	else if (strncmp(RxBuffer,"BLINK ON",8) == 0)				//Команда запуска таймера?
	{
		blink = true;
		TIM3->CR1 |= TIM_CR1_CEN;
		//TIM2->CR1 &= ~TIM_CR1_CEN;
		//TIM2->CNT = 0;
		strcpy(TxBuffer, "OK");
	}
	else if (strncmp(RxBuffer,"BLINK OFF",9) == 0)				//Команда остановки таймера?
	{
		TIM3->CR1 &= ~TIM_CR1_CEN;
		TIM3->CNT = 0;
		TIM2->CNT = 0;
		TIM2->CR1 |= TIM_CR1_CEN;
		strcpy(TxBuffer, "OK");
	}
	else if (strncmp(RxBuffer, "BRIGHTNESS?", 11))
	{
		sprintf(TxBuffer, "%d", w*10);
	}
	else if (strncmp(RxBuffer,"BRIGHTNESS",10) == 0)				//Команда изменения периода таймера?
	{
		uint16_t tim_value;
		sscanf(RxBuffer,"%*s %hu", &tim_value);				//преобразуем строку в целое число

		if ((0 <= tim_value) && (tim_value <= 100))		//параметр должен быть в заданных пределах!
		{
			w=tim_value/10;
			TIM2->CNT = 0;

			if((w>0)&&(w<10))
				{
					TIM2->CNT=0;
					TIM2->ARR = (w+1)-1;				//Модуль счёта таймера (1кГц/1000 = 1с)
					//LED_ON();
					//led_flag = true;
				}
				else if(w==10)
				{
					LED_ON();
					led_flag=true;
				}
				else
				{
					LED_OFF();
					led_flag=false;
				}

			strcpy(TxBuffer, "OK");
		}
		else
			strcpy(TxBuffer, "Parameter is out of range");	//ругаемся
	}
	else
		strcpy(TxBuffer,"Invalid Command");					//Если мы не знаем, чего от нас хотят, ругаемся в ответ

	txStr(TxBuffer,true);									//Отправляем буефер передачи с символами конца строки

	memset(RxBuffer,0,RX_BUFF_SIZE);						//Очистка буфера приёма
	ComReceived = false;									//Сбрасываем флаг приёма строки
}

/**
  * @brief  Основная программа
  * @param  None
  * @retval None
  */
int main(void)
{
	/*Инициализации всякие*/
	initClk();
	initPorts();
	initButton();
	initTIM2();
	initTIM3();
	initUSART2();

	/*Основной цикл*/
	while(true)
	{
		if (ComReceived)				//Ждём приема строки
			ExecuteCommand();
	};
}

/**
  * @brief  Подпрограмма задержки
  * @param  takts - Кол-во тактов
  * @retval None
  */
void delay(uint32_t takts)
{
	for (uint32_t i = 0; i < takts; i++) {};
}
