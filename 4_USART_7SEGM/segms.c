#include "segms.h"

// Контроль ячеек
void toggleNum_0(void) {
	GPIOA -> ODR ^= GPIO_ODR_ODR12;
}
void toggleNum_1(void) {
	GPIOA -> ODR ^= GPIO_ODR_ODR15;
}
void toggleNum_2(void) {
	GPIOB -> ODR ^= GPIO_ODR_ODR4;
}
void toggleNum_3(void) {
	GPIOB -> ODR ^= GPIO_ODR_ODR5;
}

const ToggleFunction toggleNum[4] = {
    toggleNum_0,
    toggleNum_1,
    toggleNum_2,
    toggleNum_3
};

/*
#define TOGGLE_NUM_0 GPIOA -> ODR ^= GPIO_ODR_ODR12
#define TOGGLE_NUM_1 GPIOA -> ODR ^= GPIO_ODR_ODR15
#define TOGGLE_NUM_2 GPIOB -> ODR ^= GPIO_ODR_ODR4
#define TOGGLE_NUM_3 GPIOB -> ODR ^= GPIO_ODR_ODR5*/
