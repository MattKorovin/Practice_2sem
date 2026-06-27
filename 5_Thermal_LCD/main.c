#include "stm32f10x.h"

#define BUFFERSIZE 9                    // Размер буфера температур
#define HEATER_ON  0x01                 // Маска включения/выключения нагревателя
#define HEATER_USE 0x02                 // Маска разрешения изменения состояния нагревателя
#define USART_ENAB 0x01                 // Разрешить USART принимать температуру
#define USART_BYTE 0x02                 // В какой байт записывать температуру
#define USART_FLAG 0xAA                 // Команда для приёма данных

volatile uint16_t temp = 0;             // Текущая температура
volatile uint16_t tempB;                // Буфер температуры для PendSV
volatile uint16_t buffer[BUFFERSIZE];   // Последние BUFFERSIZE значений температуры
volatile uint16_t tempMedian;           // Медианное значение температуры
volatile uint16_t tempPrevious;         // Прерыдущее медианное значение температуры

volatile uint16_t tempHeat = 0x01;      // Температура удержания (для нагревателя)
volatile uint8_t 	heaterStatus = 0;     // Статус нагревателя (вкл/выкл, используется/нет)
volatile uint8_t  usartStatus = 0;      // Статус входа USART
volatile uint8_t  usartBuffer = 0;      // Буфер выхода USART
volatile uint16_t tempCelsius;          // Температура в градусах Цельсия для USART

// Вспомогательная функция для сортировки буфера и возврата медианы - работать будем с ней
uint16_t GetMedian(const volatile uint16_t values[BUFFERSIZE])
{
    uint16_t sorted[BUFFERSIZE];
    uint16_t value;
    uint8_t i;
    uint8_t j;

    /* Копирование исходного массива */
    for (i = 0U; i < BUFFERSIZE; i++)
    {
        sorted[i] = values[i];
    }

    /* Сортировка вставками */
    for (i = 1U; i < BUFFERSIZE; i++)
    {
        value = sorted[i];
        j = i;

        while ((j > 0U) && (sorted[j - 1U] > value))
        {
            sorted[j] = sorted[j - 1U];
            j--;
        }

        sorted[j] = value;
    }

    /* После сортировки медиана находится посередине */
    return sorted[(BUFFERSIZE - 1)/2];
}

int16_t toCelsius(uint16_t pulses) {
    int32_t result;
    result = ((int32_t)pulses - 801L) * 5L;

    if (result >= 0L)
        result += 4L;
    else
        result -= 4L;

    return (uint16_t)(int16_t)(result / 8L);
}







// Конфигурация USART2
void setConf_USART(void) {
	RCC -> APB1ENR |= RCC_APB1ENR_USART2EN;
	RCC -> APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;
	RCC -> APB2ENR |= RCC_APB2ENR_AFIOEN;
	
	
	// PA2 = Alternative Output Push-Pull
	GPIOA -> CRL |= GPIO_CRL_MODE2_0;
	GPIOA -> CRL &= ~GPIO_CRL_MODE2_1;
	GPIOA -> CRL &= ~GPIO_CRL_CNF2_0;
	GPIOA -> CRL |= GPIO_CRL_CNF2_1;
	
	// PA3 = Input Floating
	GPIOA -> CRL &= ~GPIO_CRL_MODE3_0;
	GPIOA -> CRL &= ~GPIO_CRL_MODE3_1;
	GPIOA -> CRL |= GPIO_CRL_CNF3_0;
	GPIOA -> CRL &= ~GPIO_CRL_CNF3_1;
	
	
	USART2 -> BRR = 0xEA60;                      // Скорость передачи 600
	
	USART2 -> CR1 |= USART_CR1_UE;               // Включаем USART2
	USART2 -> CR1 |= USART_CR1_RE;               // Включает RX (приём)
	USART2 -> CR1 |= USART_CR1_TE;               // Включаем TX (передача)
	
	USART2 -> CR1 |= USART_CR1_RXNEIE;           // Разрешаем прерывания по принятии слова
	NVIC_SetPriority(USART2_IRQn, 2);            // Приоритет - после таймера
	NVIC_EnableIRQ(USART2_IRQn);                 // Инициализируем прерывания
}



// Конфигурация термодатчика
void setConf_Thermal(void){
	RCC -> APB1ENR |= RCC_APB1ENR_TIM3EN;  // Включаем тактирование таймера 3
	RCC -> APB1ENR |= RCC_APB1ENR_TIM2EN;  // Включаем тактирование таймера 2
	
	
	// TIMER 3 - Счётчик импульсов датчика
	
	// PB5 - Input Floating
	GPIOB -> CRL &= ~GPIO_CRL_MODE5_0;
	GPIOB -> CRL &= ~GPIO_CRL_MODE5_1;
	GPIOB -> CRL |= GPIO_CRL_CNF5_0;
	GPIOB -> CRL &= ~GPIO_CRL_CNF5_1;
	
	// Перезапуск таймера
  RCC->APB1RSTR |= RCC_APB1RSTR_TIM3RST;
  RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM3RST;
	
	// Перенастройка TIM3 на 2 канал
  AFIO->MAPR &= ~AFIO_MAPR_TIM3_REMAP;   
  AFIO->MAPR |= AFIO_MAPR_TIM3_REMAP_1;
	
  TIM3->CR1 = 0U;
  TIM3->DIER = 0U;
  TIM3->CCER = 0U;
	
  TIM3->PSC = 72U - 1U;                  // Ставим делитель на 72 МГц - частота 1 МГц
  TIM3->ARR = 0xFFFFU;                   // Заполнение нам не нужно
  TIM3->CNT = 0U;                        // Значение счётчика тоже
	
	// Настройка 2 канала как входа PB5
  TIM3->CCMR1 &=
        ~(TIM_CCMR1_CC2S |
          TIM_CCMR1_IC2PSC |
          TIM_CCMR1_IC2F);

  TIM3->CCMR1 |= TIM_CCMR1_CC2S_0;
  TIM3->CCMR1 |= (0x5U << 12U);
	
  TIM3->CCER |= TIM_CCER_CC2P;           // Захват по нисходящему фронту (1 -> 0)
  TIM3->CCER |= TIM_CCER_CC2E;           // Разрешаем захват по 2 каналу
	
  TIM3->EGR = TIM_EGR_UG;
  TIM3->SR = 0U;

  NVIC_ClearPendingIRQ(TIM3_IRQn);       
	NVIC_SetPriority(TIM3_IRQn, 0);        // Первый приоритет
	TIM3->DIER |= TIM_DIER_CC2IE;          // Разрешаем прерывания на 2 канал TIM3
  NVIC_EnableIRQ(TIM3_IRQn);             // Включаем прерывания
  TIM3->CR1 |= TIM_CR1_CEN;              // Запускаем таймер
	
	
	// TIMER 2 - Счётчик импульсов
	
  RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;
  RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;

 
  TIM2->CR1 = 0U;
  TIM2->DIER = 0U;
    
	TIM2->PSC = 72U - 1U;                  // Делитель 72 - частота 1 МГц

  TIM2->ARR = 50U - 1U;                  // Размер счётчика 50 - период заполнения 50 мкс
  TIM2->CNT = 0U;                        // Сбрасываем счётчик таймера 2

  TIM2->CR1 |= TIM_CR1_OPM;              // После перезаполнения сбрасываем таймер и останавливаемся
  TIM2->CR1 |= TIM_CR1_URS;

  TIM2->EGR = TIM_EGR_UG;
  TIM2->SR = 0U;

  TIM2->DIER |= TIM_DIER_UIE;            // Прерывание по переполнению

  NVIC_ClearPendingIRQ(TIM2_IRQn);
	NVIC_SetPriority(TIM2_IRQn, 1);        // Второй приоритет
  NVIC_EnableIRQ(TIM2_IRQn);
}



// Обработка нагревателя
void heater(void) {
	if(heaterStatus & HEATER_USE) {
		if((heaterStatus & HEATER_ON) && (tempMedian >= tempHeat)) {
			heaterStatus &= ~HEATER_ON;
			// Выключаем нагреватель
		}
		else if (!(heaterStatus & HEATER_ON) && (tempMedian < tempHeat - 8) && (tempHeat > 8)){
			heaterStatus |= HEATER_ON;
			// Включаем нагреватель
		}
	}
}



// Конфигурация программного прерывания
void setConf_PendSV(void) {
	NVIC_SetPriority(PendSV_IRQn, 4);
	SCB->ICSR = SCB_ICSR_PENDSVCLR_Msk;
}





// Считает импульсы
void TIM3_IRQHandler(void) {
  TIM2 -> CR1 &= ~TIM_CR1_CEN;             // Временно останавливаем TIM2
	TIM2 -> CNT = 0;                         // Обнуляем счётчик TIM2
	TIM2->SR &= ~TIM_SR_UIF;                 // Сбрасываем флаг прерывания TIM2 (на всякий)
  TIM2 -> CR1 |= TIM_CR1_CEN;              // Запускаем TIM2
	TIM3 -> SR &= ~TIM_SR_CC2IF;             // Сбрасываем флаг прерывания TIM3
	temp++;
}



// Фиксирует импульсы
void TIM2_IRQHandler(void) {
	if (!(TIM2->SR & TIM_SR_UIF))
		return;
	
	TIM2->SR &= ~TIM_SR_UIF;                 // Сбрасываем флаг прерывания TIM2
  TIM2->CNT = 0U;                          // Сбрасываем счётчик TIM2 (на всякий)
	tempB = temp;
	temp = 0;
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;      // Вызываем PendSV
}



void PendSV_Handler(void) {
	if(tempB) {
		for(int i=BUFFERSIZE-1; i>0; i--)
			buffer[i] = buffer[i-1];
		buffer[0] = tempB;
		tempMedian = GetMedian(buffer);
		tempB = 0;
	}
	heater();
}



void USART2_IRQHandler(void) {
	if(USART2 -> SR & USART_SR_RXNE) {
		uint8_t usart = USART2 -> DR;
		if(usartStatus & USART_ENAB) {                              // Проверка: разрешён приём байтов?
			if(usartStatus & USART_BYTE) {                            // Второй байт данных
				tempHeat |= (uint16_t)usart << 8;                       // Записываем второй байт
				heaterStatus |= HEATER_USE;                             // Разрешаем нагреватель
				usartStatus &= ~(USART_BYTE | USART_ENAB);              // Запрещаем принимать температуру и сбрасываем байт
				SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
			}
			else {                                                    // Первый байт данных
				heaterStatus &= ~HEATER_USE;                            // Запрещаем нагреватель
				tempHeat = 0;                                           // Сбрасываем температуру нагревателя
				tempHeat |= (uint16_t)usart;                            // Записываем первый байт
				usartStatus |= USART_BYTE;                              // Следующий приём - второй байт
			}
		}
		else if (usart == USART_FLAG)                               // Флаг для приёма
			usartStatus = USART_ENAB;                                 // Разрешаем принимать следующие байты
		else if (!usart) {                                          // Если поступило просто 0x00 без 0xAA, то
			tempCelsius = toCelsius(tempMedian);                      // Пересчитываем температуру в градусы Цельсия
			USART2 -> DR = (uint8_t)tempCelsius;                      // Выдаём компу первый байт температуры
			usartBuffer = (uint8_t)(tempCelsius >> 8);                // Делаем буфер для второго байта
			USART2 -> CR1 |= USART_CR1_TXEIE;                         // Разрешаем прерывания по отправке слова
		}
	}                                          
	if ((USART2 -> SR & USART_SR_TXE) && (USART2 -> CR1 & USART_CR1_TXEIE)) {  // Если первый байт передан
			USART2 -> DR = usartBuffer;                                            // То выдаём компу второй байт температуры из буфера
			USART2->CR1 &= ~USART_CR1_TXEIE;                                       // Запрещаем прерывания по отправке слова
	}
}





int main(void) {
	__disable_irq();
	setConf_USART();
	setConf_Thermal();
	setConf_PendSV();
	__enable_irq();
	while(1) {
		
	}
}
