#include <stm32f411xe.h>

static void GPIOInit(void) {
    // GPIOC
    // LED
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    GPIOC->MODER |= (1 << (13 * 2)); // MODER_13 27:26 = 0b01 = output

    GPIOC->ODR |= (1 << 13); // ODR13, led on (on is low)

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
}    

static void clockInit(void) {
    // 3 ws
    FLASH->ACR &= ~(0x07 << FLASH_ACR_LATENCY_Pos);
    FLASH->ACR |= (FLASH_ACR_LATENCY_3WS << FLASH_ACR_LATENCY_Pos);

    // HSE
    RCC->CR |= RCC_CR_HSEON;
    while ((RCC->CR & RCC_CR_HSERDY) == 0) {
        GPIOC->ODR &= ~(1 << 13);
    }
    GPIOC->ODR |= (1 << 13);

    // PLL
    RCC->PLLCFGR = 0;
    RCC->PLLCFGR |= (RCC_PLLCFGR_PLLSRC_HSE | 
                    (4 << RCC_PLLCFGR_PLLQ_Pos) |
                    (0 << RCC_PLLCFGR_PLLP_Pos) | 
                    (192 << RCC_PLLCFGR_PLLN_Pos) | 
                    (25 << RCC_PLLCFGR_PLLM_Pos));
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0) {
        GPIOC->ODR &= ~(1 << 13);
    }
    GPIOC->ODR |= (1 << 13);

    // prescalers & clock switch
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_PLL) != RCC_CFGR_SWS_PLL) {
        GPIOC->ODR &= ~(1 << 13);
    }
    GPIOC->ODR |= (1 << 13);

    RCC->CFGR |= (0x04 << RCC_CFGR_PPRE1_Pos) | 
                 (0x04 << RCC_CFGR_PPRE2_Pos);

    // dissable HSI
    RCC->CR &= ~RCC_CR_HSION;    
}

static void MCO1Init(void) {
    GPIOA->MODER |= (0x02 << (8 * 2));
    GPIOA->OSPEEDR |= (0x03 << (8 * 2));

    RCC->CFGR |= (4 << RCC_CFGR_MCO1PRE_Pos) | (3 << RCC_CFGR_MCO1_Pos);

    //GPIOC->ODR &= ~(1 << 13);
}

void deviceInit(void) {
    clockInit();
    GPIOInit();
    MCO1Init();
}