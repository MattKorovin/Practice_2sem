#include "stm32f10x.h"
#include "segms.h"

/*
Необходимо подключить перемычки

Программа использует USART для связи с компьютером
Вход USART - состояние ячейки индикатора (что вывести на сегментах)

На вход поочерёдно подаётся состояние каждой отдельной ячейки
Сначала 0 (крайняя левая), потом 1, потом 2, потом 3 (крайняя правая), потом снова 0
Таймер поочерёдно переключает ячейки с периодом 1 мс для всей панели
На каждой ячейке отображается своя цифра

Как переключаются ячейки:
- Сначала отключается ячейка K
- Потом последовательно задаём состояние (0/1) каждого сегмента
- Потом ++К %= 4 (переключаем номер ячейки на следующую)
- Потом включаем ячейку К (следующая по счёту)
*/



volatile uint8_t NUM[4] = {0x06,0x6F,0x7F,0x66};     /* Состояние каждой ячейки (0 -> 3 СЛЕВА НАПРАВО)
Переменная ячейки:
0bTGFEDCBA
Где буквы отражают состояние соответвтующего сегмента (ВКЛ/ВЫКЛ)
*/

volatile uint8_t TIMER_NUM = 0;
volatile uint8_t USART_NUM = 0;





// Таймер ставим на 4 кГц - 4 раза в мс, по 1 разу на ячейку.
void setTimerConf(void) {
	RCC -> APB2ENR |= RCC_APB2ENR_TIM1EN;        // Включаем таймер 1
	TIM1 -> CR1 &= ~TIM_CR1_CEN;                 // Но пока отключаем счётчик
	
	TIM1 -> CR1 &= ~TIM_CR1_DIR;                 // Счётчик на увеличение
	TIM1 -> CR1 |= TIM_CR1_ARPE;                 // Включаем лимит
	TIM1 -> CR1 |= TIM_CR1_URS;                  // Прерывание UPDATE только от перезаполнения
	
	TIM1 -> PSC = 3599;                          // Деление на 3600 (частота обновления 20 кГц)
	TIM1 -> ARR = 4;                             // Лимит счётчика 5 (частота апдейта 4 кГц)
	
	NVIC_SetPriority(TIM1_UP_IRQn, 0);
	NVIC_EnableIRQ(TIM1_UP_IRQn);                // Разрешаем прерывания
	TIM1 -> DIER |= TIM_DIER_UIE;                // Убираем маску
	TIM1 -> CR1 |= TIM_CR1_CEN;                  // Запускаем счётчик
}



void setPinConf(void) {
	// Настройка GPIO
	RCC -> APB2ENR |= RCC_APB2ENR_IOPBEN;
	RCC -> APB2ENR |= RCC_APB2ENR_IOPAEN;
	
	
	// Сегменты
	
	// A
	GPIOB -> CRH &= ~GPIO_CRH_MODE12_0;    // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH |= GPIO_CRH_MODE12_1;     // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH &= ~GPIO_CRH_CNF12_0;     // CNF0 = 0 (CNF = 00 - Output Push-Pull)
	GPIOB -> CRH &= ~GPIO_CRH_CNF12_1;     // CNF1 = 0 (CNF = 00 - Output Push-Pull)
	GPIOB -> BSRR |= GPIO_BSRR_BR12;       // GND
	
	// B
	GPIOB -> CRH &= ~GPIO_CRH_MODE13_0;    // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH |= GPIO_CRH_MODE13_1;     // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH &= ~GPIO_CRH_CNF13_0;     // CNF0 = 0 (CNF = 00 - Output Push-Pull)
	GPIOB -> CRH &= ~GPIO_CRH_CNF13_1;     // CNF1 = 0 (CNF = 00 - Output Push-Pull)
	GPIOB -> BSRR |= GPIO_BSRR_BR13;       // GND
	
	// C
	GPIOB -> CRH &= ~GPIO_CRH_MODE14_0;    // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH |= GPIO_CRH_MODE14_1;     // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH &= ~GPIO_CRH_CNF14_0;     // CNF0 = 0 (CNF = 00 - Output Push-Pull)
	GPIOB -> CRH &= ~GPIO_CRH_CNF14_1;     // CNF1 = 0 (CNF = 00 - Output Push-Pull)
	GPIOB -> BSRR |= GPIO_BSRR_BR14;       // GND
	
	// D
	GPIOB -> CRH &= ~GPIO_CRH_MODE15_0;    // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH |= GPIO_CRH_MODE15_1;     // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH &= ~GPIO_CRH_CNF15_0;     // CNF0 = 0 (CNF = 00 - Output Push-Pull)
	GPIOB -> CRH &= ~GPIO_CRH_CNF15_1;     // CNF1 = 0 (CNF = 00 - Output Push-Pull)
	GPIOB -> BSRR |= GPIO_BSRR_BR15;       // GND
	
	// E
	GPIOA -> CRH &= ~GPIO_CRH_MODE8_0;     // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOA -> CRH |= GPIO_CRH_MODE8_1;      // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOA -> CRH &= ~GPIO_CRH_CNF8_0;      // CNF0 = 0 (CNF = 00 - Output Push-Pull)
	GPIOA -> CRH &= ~GPIO_CRH_CNF8_1;      // CNF1 = 0 (CNF = 00 - Output Push-Pull)
	GPIOA -> BSRR |= GPIO_BSRR_BR8;        // GND
	
	// F
	GPIOA -> CRH &= ~GPIO_CRH_MODE9_0;     // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOA -> CRH |= GPIO_CRH_MODE9_1;      // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOA -> CRH &= ~GPIO_CRH_CNF9_0;      // CNF0 = 0 (CNF = 00 - Output Push-Pull)
	GPIOA -> CRH &= ~GPIO_CRH_CNF9_1;      // CNF1 = 0 (CNF = 00 - Output Push-Pull)
	GPIOA -> BSRR |= GPIO_BSRR_BR9;        // GND
	
	// G
	GPIOA -> CRH &= ~GPIO_CRH_MODE10_0;    // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOA -> CRH |= GPIO_CRH_MODE10_1;     // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOA -> CRH &= ~GPIO_CRH_CNF10_0;     // CNF0 = 0 (CNF = 00 - Output Push-Pull)
	GPIOA -> CRH &= ~GPIO_CRH_CNF10_1;     // CNF1 = 0 (CNF = 00 - Output Push-Pull)
	GPIOA -> BSRR |= GPIO_BSRR_BR10;       // GND
	
	// T (DP)
	GPIOA -> CRH &= ~GPIO_CRH_MODE11_0;    // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOA -> CRH |= GPIO_CRH_MODE11_1;     // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOA -> CRH &= ~GPIO_CRH_CNF11_0;     // CNF0 = 0 (CNF = 00 - Output Push-Pull)
	GPIOA -> CRH &= ~GPIO_CRH_CNF11_1;     // CNF1 = 0 (CNF = 00 - Output Push-Pull)
	GPIOA -> BSRR |= GPIO_BSRR_BR11;       // GND
	
	
	// Ячейки
	
	//Выключаем JTAG
	RCC -> APB2ENR |= RCC_APB2ENR_AFIOEN;
	AFIO -> MAPR &= ~AFIO_MAPR_SWJ_CFG;
	AFIO -> MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE; /*
	JTAG - это для отладки
	Он использует выводы PA15, PB4
	Без его отключения ячейки посередине работать не будут
	Но, его отключение никак не вредит работе и программированию */
	
	// NUM0 = Output Open-Drain
	GPIOA -> CRH &= ~GPIO_CRH_MODE12_0;    // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOA -> CRH |= GPIO_CRH_MODE12_1;     // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOA -> CRH |= GPIO_CRH_CNF12_0;      // CNF0 = 1 (CNF = 01 - Output Open-Drain)
	GPIOA -> CRH &= ~GPIO_CRH_CNF12_1;     // CNF1 = 0 (CNF = 01 - Output Open-Drain)
	GPIOA -> BSRR |= GPIO_BSRR_BR12;       // GND (ВКЛ)
	
	// NUM1 = Output Open-Drain
	GPIOA -> CRH &= ~GPIO_CRH_MODE15_0;    // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOA -> CRH |= GPIO_CRH_MODE15_1;     // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOA -> CRH |= GPIO_CRH_CNF15_0;      // CNF0 = 1 (CNF = 01 - Output Open-Drain)
	GPIOA -> CRH &= ~GPIO_CRH_CNF15_1;     // CNF1 = 0 (CNF = 01 - Output Open-Drain)
	GPIOA -> BSRR |= GPIO_BSRR_BS15;       // Открытый коллектор (ВЫКЛ)
	
	// NUM2 = Output Open-Drain
	GPIOB -> CRL &= ~GPIO_CRL_MODE4_0;     // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOB -> CRL |= GPIO_CRL_MODE4_1;      // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOB -> CRL |= GPIO_CRL_CNF4_0;       // CNF0 = 1 (CNF = 01 - Output Open-Drain)
	GPIOB -> CRL &= ~GPIO_CRL_CNF4_1;      // CNF1 = 0 (CNF = 01 - Output Open-Drain)
	GPIOB -> BSRR |= GPIO_BSRR_BS4;        // Открытый коллектор (ВЫКЛ)
	
	// NUM3 = Output Open-Drain
	GPIOB -> CRL &= ~GPIO_CRL_MODE5_0;     // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOB -> CRL |= GPIO_CRL_MODE5_1;      // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOB -> CRL |= GPIO_CRL_CNF5_0;       // CNF0 = 1 (CNF = 01 - Output Open-Drain)
	GPIOB -> CRL &= ~GPIO_CRL_CNF5_1;      // CNF1 = 0 (CNF = 01 - Output Open-Drain)
	GPIOB -> BSRR |= GPIO_BSRR_BS5;        // Открытый коллектор (ВЫКЛ)
	
	
	// USART2 - Поскольку USART1 занят
	
	/* PA2 = Alternative Output Push-Pull
	GPIOA -> CRL |= GPIO_CRL_MODE2_0;
	GPIOA -> CRL &= ~GPIO_CRL_MODE2_1;
	GPIOA -> CRL &= ~GPIO_CRL_CNF2_0;
	GPIOA -> CRL |= GPIO_CRL_CNF2_1; */
	
	// PA3 = Input Floating
	GPIOA -> CRL &= ~GPIO_CRL_MODE3_0;
	GPIOA -> CRL &= ~GPIO_CRL_MODE3_1;
	GPIOA -> CRL |= GPIO_CRL_CNF3_0;
	GPIOA -> CRL &= ~GPIO_CRL_CNF3_1;
}



// USART2 тактируется от APB1 без множителей напрямую (36 МГц) -> BRR ставим другой
void setUsartConf(void) {
	RCC -> APB1ENR |= RCC_APB1ENR_USART2EN;      // Включаем тактирование USART2
	USART2 -> BRR = 0x0EA6;                      // Скорость передачи 9600
	
	USART2 -> CR1 |= USART_CR1_UE;               // Включаем USART2
	USART2 -> CR1 |= USART_CR1_RE;               // Включает RX (приём)
	//USART2 -> CR1 |= USART_CR1_TE;             // Включаем TX (передача) - не нужна
	
	USART2 -> CR1 |= USART_CR1_RXNEIE;           // Разрешаем прерывания по принятии слова
	NVIC_SetPriority(USART2_IRQn, 1);            // Приоритет - после таймера
	NVIC_EnableIRQ(USART2_IRQn);                 // Инициализируем прерывания
	__enable_irq();
}






void TIM1_UP_IRQHandler(void) {
	TIM1 -> DIER &= ~TIM_DIER_UIE;           // Временно запрещаем прерывания
	TIM1 -> SR &= ~TIM_SR_UIF;               // Сбрасываем флаг прерывания
	
	toggleNum[TIMER_NUM % 4]();              // Отключаем ячейку
	TIMER_NUM++;                             // Увеличиваем порядок ячейки
	
	// Последовательно задаём значения ячейкам
	CTRL_SEGM_A << (16 * ((NUM[TIMER_NUM % 4] & SEGM_A) == 0));
	CTRL_SEGM_B << (16 * ((NUM[TIMER_NUM % 4] & SEGM_B) == 0));
	CTRL_SEGM_C << (16 * ((NUM[TIMER_NUM % 4] & SEGM_C) == 0));
	CTRL_SEGM_D << (16 * ((NUM[TIMER_NUM % 4] & SEGM_D) == 0));
	CTRL_SEGM_E << (16 * ((NUM[TIMER_NUM % 4] & SEGM_E) == 0));
	CTRL_SEGM_F << (16 * ((NUM[TIMER_NUM % 4] & SEGM_F) == 0));
	CTRL_SEGM_G << (16 * ((NUM[TIMER_NUM % 4] & SEGM_G) == 0));
	CTRL_SEGM_T << (16 * ((NUM[TIMER_NUM % 4] & SEGM_T) == 0));
	
	toggleNum[TIMER_NUM % 4]();              // Включаем следующую ячейку
	
	TIM1 -> DIER |= TIM_DIER_UIE;            // Снова разрешаем прерывания таймера
}



void USART2_IRQHandler(void) {
	if(USART2 -> SR & USART_SR_RXNE) {
		NUM[USART_NUM % 4] = (uint8_t)USART2 -> DR;
		USART_NUM++;
	}
}





int main(void) {
	setPinConf();
	setTimerConf();
	setUsartConf();
	while(1) {
		
	}
}
