#include "stm32f10x.h"

#define LED1_ON() GPIOB -> BSRR |= GPIO_BSRR_BS10 // Периметр
#define LED1_OFF() GPIOB -> BSRR |= GPIO_BSRR_BR10
#define LED1_TOGGLE() GPIOB -> ODR ^= GPIO_ODR_ODR10

#define LED2_ON() GPIOB -> BSRR |= GPIO_BSRR_BS11 // Центр
#define LED2_OFF() GPIOB -> BSRR |= GPIO_BSRR_BR11
#define LED2_TOGGLE() GPIOB -> ODR ^= GPIO_ODR_ODR11

volatile uint8_t stateLED = 0; // Режим работы
/*
0b0000BbAa
B - режим работы группы центра (1 - мигать, 0 - не мигать)
b - состояние подсветки группы центра (1 - ВКЛ, 0 - ВЫКЛ)
A, a - аналогично для периметра
*/

#define LED1_STATE ((uint8_t)0x01) // a
#define LED1_MODE ((uint8_t)0x02)  // A
#define LED2_STATE ((uint8_t)0x04) // b
#define LED2_MODE ((uint8_t)0x08)  // B



/*
Необходимо поставить перемычки: PA8, PA9_2, PA10, PA11, PB12, PB15

Программа использует 5 кнопок:
  USER1 (PA8) - включить подсветку периметра без мигания
	USER2 (PA9) - выклчить подсветку периметра
	USER3 (PA10) - включить подсветку центра без мигания
	USER4 (PA11) - выключить подсветку центра
	Джойстик (PB12) - ВКЛ мигание включенных светодиодов

Используется 3 прерывания:
  Внешнее прерывание (9_5) по кнопке - обработка нажатий кнопок
  Внешнее прерывание (15_10) по кнопке - обработка нажатий кнопок
	Прерывание по таймеру - если режим = 2 то мигание соответствующими светодиодами
	
ВНИМАНИЕ! Нажатие кнопки на джойстике
отключает прерывания по ней вплоть
до следующего прерывания таймера.
Это для борьбы с дребезгом контактов
*/



// Конфигурация GPIO И внешних прерываний
void setPinConf(void) {
	// Настройка GPIO
	
	RCC -> APB2ENR |= RCC_APB2ENR_IOPBEN;
	RCC -> APB2ENR |= RCC_APB2ENR_IOPAEN;
	
	
	// PB10 = Output Push-Pull (LED периметр)
	GPIOB -> CRH &= ~GPIO_CRH_MODE10_0;    // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH |= GPIO_CRH_MODE10_1;     // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH &= ~GPIO_CRH_CNF10_0;     // CNF0 = 0 (CNF = 00 - Output Push-Pull)
	GPIOB -> CRH &= ~GPIO_CRH_CNF10_1;     // CNF1 = 0 (CNF = 00 - Output Push-Pull)
	
	// PB11 = Output Push-Pull (LED центр)
	GPIOB -> CRH &= ~GPIO_CRH_MODE11_0;    // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH |= GPIO_CRH_MODE11_1;     // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH &= ~GPIO_CRH_CNF11_0;     // CNF0 = 0 (CNF = 00 - Output Push-Pull)
	GPIOB -> CRH &= ~GPIO_CRH_CNF11_1;     // CNF1 = 0 (CNF = 00 - Output Push-Pull)
	
	// PB15 = Output Open-Drain ("GND")   // Общий для кнопок джойстика
	GPIOB -> CRH &= ~GPIO_CRH_MODE15_0;   // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH |= GPIO_CRH_MODE15_1;    // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH |= GPIO_CRH_CNF15_0;     // CNF0 = 1 (CNF = 01 - Output Open-Drain)
	GPIOB -> CRH &= ~GPIO_CRH_CNF15_1;    // CNF1 = 0 (CNF = 01 - Output Open-Drain)
	GPIOB -> BSRR |= GPIO_BSRR_BR15;      // PB15 -> GND
	
	
	//PA8 = Input Floating
	GPIOA -> CRH &= ~GPIO_CRH_MODE8_0;     // MODE0 = 0 (MODE = 00 - Input)
	GPIOA -> CRH &= ~GPIO_CRH_MODE8_1;     // MODE1 = 0 (MODE = 00 - Input)
	GPIOA -> CRH |= GPIO_CRH_CNF8_0;       // CNF0 = 1 (CNF = 01 - Floating)
	GPIOA -> CRH &= ~GPIO_CRH_CNF8_1;      // CNF1 = 0 (CNF = 01 - Floating)
	
	//PA9 = Input Floating
	GPIOA -> CRH &= ~GPIO_CRH_MODE9_0;     // MODE0 = 0 (MODE = 00 - Input)
	GPIOA -> CRH &= ~GPIO_CRH_MODE9_1;     // MODE1 = 0 (MODE = 00 - Input)
	GPIOA -> CRH |= GPIO_CRH_CNF9_0;       // CNF0 = 1 (CNF = 01 - Floating)
	GPIOA -> CRH &= ~GPIO_CRH_CNF9_1;      // CNF1 = 0 (CNF = 01 - Floating)
	
	//PA10 = Input Floating
	GPIOA -> CRH &= ~GPIO_CRH_MODE10_0;     // MODE0 = 0 (MODE = 00 - Input)
	GPIOA -> CRH &= ~GPIO_CRH_MODE10_1;     // MODE1 = 0 (MODE = 00 - Input)
	GPIOA -> CRH |= GPIO_CRH_CNF10_0;       // CNF0 = 1 (CNF = 01 - Floating)
	GPIOA -> CRH &= ~GPIO_CRH_CNF10_1;      // CNF1 = 0 (CNF = 01 - Floating)
	
	//PA11 = Input Floating
	GPIOA -> CRH &= ~GPIO_CRH_MODE11_0;     // MODE0 = 0 (MODE = 00 - Input)
	GPIOA -> CRH &= ~GPIO_CRH_MODE11_1;     // MODE1 = 0 (MODE = 00 - Input)
	GPIOA -> CRH |= GPIO_CRH_CNF11_0;       // CNF0 = 1 (CNF = 01 - Floating)
	GPIOA -> CRH &= ~GPIO_CRH_CNF11_1;      // CNF1 = 0 (CNF = 01 - Floating)
	
	// PB12 = Input Pull-Up
	GPIOB -> CRH &= ~GPIO_CRH_MODE12_0;   // MODE0 = 0 (MODE = 00 - Input)
	GPIOB -> CRH &= ~GPIO_CRH_MODE12_1;   // MODE1 = 0 (MODE = 00 - Input)
	GPIOB -> CRH &= ~GPIO_CRH_CNF12_0;    // CNF0 = 0 (CNF = 10 - Pull-Down / Pull-Up)
	GPIOB -> CRH |= GPIO_CRH_CNF12_1;     // CNF1 = 1 (CNF = 10 - Pull-Down / Pull-Up)
	GPIOB -> BSRR |= GPIO_BSRR_BS12;      // PB12 = Pull-Up
	
	
	// Настройка прерываний
	RCC -> APB2ENR |= RCC_APB2ENR_AFIOEN;        // Interrupt ON
	
	// Init PA8
	EXTI -> PR |= EXTI_PR_PR8;
	EXTI -> IMR |= EXTI_IMR_MR8;
	AFIO -> EXTICR[2] &= ~AFIO_EXTICR3_EXTI8;
	AFIO -> EXTICR[2] |= AFIO_EXTICR3_EXTI8_PA;  // port 8 = A
	
	// Init PA9
	EXTI -> PR |= EXTI_PR_PR9;
	EXTI -> IMR |= EXTI_IMR_MR9;
	AFIO -> EXTICR[2] &= ~AFIO_EXTICR3_EXTI9;
	AFIO -> EXTICR[2] |= AFIO_EXTICR3_EXTI9_PA;  // port 9 = A	
	
	// Setup PA8, PA9
	EXTI -> FTSR |= EXTI_FTSR_TR8;                // Interrupt 1 -> 0
	EXTI -> FTSR |= EXTI_FTSR_TR9;                // Interrupt 1 -> 0
	NVIC_SetPriority(EXTI9_5_IRQn, 1);            // Set Priority EXTI_9_5 -> 1
	NVIC_EnableIRQ(EXTI9_5_IRQn);                 // Enable Interrupt 1
	
	// Init PA10
	EXTI -> PR |= EXTI_PR_PR10;
	EXTI -> IMR |= EXTI_IMR_MR10;
	AFIO -> EXTICR[2] &= ~AFIO_EXTICR3_EXTI10;
	AFIO -> EXTICR[2] |= AFIO_EXTICR3_EXTI10_PA;  // port 10 = A
	
	// Init PA11
	EXTI -> PR |= EXTI_PR_PR11;
	EXTI -> IMR |= EXTI_IMR_MR11;
	AFIO -> EXTICR[2] &= ~AFIO_EXTICR3_EXTI11;
	AFIO -> EXTICR[2] |= AFIO_EXTICR3_EXTI11_PA;  // port 11 = A	
	
	// Init PB12
	EXTI -> PR |= EXTI_PR_PR12;
	EXTI -> IMR |= EXTI_IMR_MR12;
	AFIO -> EXTICR[3] &= ~AFIO_EXTICR4_EXTI12;
	AFIO -> EXTICR[3] |= AFIO_EXTICR4_EXTI12_PB;  // port 12 = B
	
	// Setup PA10, PA11, PB12
	EXTI -> FTSR |= EXTI_FTSR_TR10;                // Interrupt 1 -> 0
	EXTI -> FTSR |= EXTI_FTSR_TR11;                // Interrupt 1 -> 0
	EXTI -> FTSR |= EXTI_FTSR_TR12;                // Interrupt 1 -> 0
	NVIC_SetPriority(EXTI15_10_IRQn, 2);           // Set Priority EXTI_9_5 -> 1
	NVIC_EnableIRQ(EXTI15_10_IRQn);                // Enable Interrupt 1
}



// Конфигурация таймера И его прерываний
void setTimerConf(void) {
	RCC -> APB2ENR |= RCC_APB2ENR_TIM1EN;        // Включаем таймер 1
	TIM1 -> CR1 &= ~TIM_CR1_CEN;                 // Но пока отключаем счётчик
	
	TIM1 -> CR1 &= ~TIM_CR1_DIR;                 // Счётчик на увеличение
	TIM1 -> CR1 |= TIM_CR1_ARPE;                 // Включаем лимит
	TIM1 -> CR1 |= TIM_CR1_URS;                  // Прерывание UPDATE только от перезаполнения
	
	TIM1 -> PSC = 7199;                          // Деление на 7200 (частота обновления 10 кГц)
	TIM1 -> ARR = 4999;                          // Лимит счётчика 5000 (частота обновления 2 Гц)
	
	NVIC_SetPriority(TIM1_UP_IRQn, 0);
	NVIC_EnableIRQ(TIM1_UP_IRQn);                // Разрешаем прерывания
	TIM1 -> DIER |= TIM_DIER_UIE;                // Убираем маску
	TIM1 -> CR1 |= TIM_CR1_CEN;                  // Запускаем счётчик
}



void setStart(void) {
	stateLED &= ~LED1_STATE;
	stateLED |= LED2_STATE;
	stateLED |= LED2_MODE;
	EXTI -> IMR |= EXTI_IMR_MR8;
	EXTI -> IMR &= ~EXTI_IMR_MR9;
	EXTI -> IMR |= EXTI_IMR_MR11;
	EXTI -> IMR |= EXTI_IMR_MR10;
	LED2_ON();
	LED1_OFF();
}



void EXTI9_5_IRQHandler(void) {
	if ((EXTI -> PR & EXTI_PR_PR8) && (EXTI -> IMR & EXTI_IMR_MR8)) {
		EXTI -> PR |= EXTI_PR_PR8;               // Снимаем флаг прерывания
		EXTI -> IMR &= ~EXTI_IMR_MR8;            // Ставим маску на группу 8 внешних прерываний
		stateLED |= LED1_STATE;                  // Переводим состояние группы в режим ВКЛ
		stateLED &= ~LED1_MODE;                  // Но не даём ей мигать
		LED1_ON();                               // Включаем группу
		EXTI -> IMR |= EXTI_IMR_MR9;             // Устанавливаем маску на группу 9 внешних прерываний
	}
	else if ((EXTI -> PR & EXTI_PR_PR9) && (EXTI -> IMR & EXTI_IMR_MR9)) {
		EXTI -> PR |= EXTI_PR_PR9;
		EXTI -> IMR &= ~EXTI_IMR_MR9;
		stateLED &= ~LED1_STATE;
		LED1_OFF();
		EXTI -> IMR |= EXTI_IMR_MR8;
	}
}



void EXTI15_10_IRQHandler(void) {
	if ((EXTI -> PR & EXTI_PR_PR10) && (EXTI -> IMR & EXTI_IMR_MR10)) {
		EXTI -> PR |= EXTI_PR_PR10;
		EXTI -> IMR &= ~EXTI_IMR_MR10;
		stateLED |= LED2_STATE;
		stateLED &= ~LED2_MODE;
		LED2_ON();
		EXTI -> IMR |= EXTI_IMR_MR11;
	}
	else if ((EXTI -> PR & EXTI_PR_PR11) && (EXTI -> IMR & EXTI_IMR_MR11)) {
		EXTI -> PR |= EXTI_PR_PR11;
		EXTI -> IMR &= ~EXTI_IMR_MR11;
		stateLED &= ~LED2_STATE;
		LED2_OFF();
		EXTI -> IMR |= EXTI_IMR_MR10;
	}
	if ((EXTI -> PR & EXTI_PR_PR12) && (EXTI -> IMR & EXTI_IMR_MR12)) {
		EXTI -> PR |= EXTI_PR_PR12;
		EXTI -> IMR &= ~EXTI_IMR_MR12;
		stateLED |= LED1_MODE;          // Обе группы будут иметь STATE "Мигать"
		stateLED |= LED2_MODE;          // Но по факту мигать будут только включенные
		EXTI -> IMR |= EXTI_IMR_MR8;
		EXTI -> IMR |= EXTI_IMR_MR10;
		// STATE выключенной группы сменится при нажатии на любую кнопку
	}
}



// Функция обработки вектора прерываний таймера
// Срабатывает, если таймер обновился
void TIM1_UP_IRQHandler(void) {
	TIM1 -> DIER &= ~TIM_DIER_UIE;           // Временно запрещаем прерывания
	TIM1 -> SR &= ~TIM_SR_UIF;               // Сбрасываем флаг прерывания
	
	if ((stateLED & LED1_MODE) && (stateLED & LED1_STATE)) {
		LED1_TOGGLE();
	}
	if ((stateLED & LED2_MODE) && (stateLED & LED2_STATE)) {
		LED2_TOGGLE();
	}
	
	TIM1 -> DIER |= TIM_DIER_UIE;            // Снова разрешаем прерывания таймера
	EXTI -> IMR |= EXTI_IMR_MR12;            // И кнопки джойстика тоже
}



int main(void) {
	setPinConf();
	setTimerConf();
	setStart();
	while(1) {
		
	}
}
