#include <stm32f411xe.h>

static void initLed(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    GPIOC->MODER |= (1 << (13 * 2)); // MODER_13 27:26 = 0b01 = output

    
    GPIOC->ODR &= ~(1 << 13); // ODR13, led on (on is low)
}

static void a0InterruptInit(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // gpioA clock
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // syscfg clock
    
    // pin config/pullups
    GPIOA->MODER &= ~(3 << 0); // MODER0 clear = input    
    GPIOA->PUPDR |= (1 << 0); // pull up
    
    EXTI->IMR |= EXTI_IMR_MR0; // intererrupt on 0 is masked
    EXTI->RTSR |= EXTI_RTSR_TR0;
    EXTI->FTSR |= EXTI_FTSR_TR0;

    NVIC_EnableIRQ(EXTI0_IRQn);// ????
}

static void EXTI0_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR0) {
        EXTI->PR |= EXTI_PR_PR0;

        GPIOC->ODR ^= (1 << 13); 
    }
}

/*
int main(void) {
    initLed();
    a0InterruptInit();
    
    while (1);
    
    return 0;
}
*/