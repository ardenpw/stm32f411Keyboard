#include <stm32f411xe.h>
#include "include.h"

// configure power and clock 
// UTMIFS?
// FDMOD in OTG_FS_GUSBCFG to force peripheral mode 

void interruptInit(void) {
    NVIC_EnableIRQ(OTG_FS_IRQn);
}

void GPIOInit(void) {
    // GPIOC
    // LED
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    GPIOC->MODER |= (1 << (13 * 2)); // MODER_13 27:26 = 0b01 = output

    GPIOC->ODR |= (1 << 13); // ODR13, led on (on is low)

    // GPIOA
    // MCO1
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER |= (2 << (8 * 2)); //  MODER_8 16:17 = 0b10 = alternate function
    GPIOA->OSPEEDR |= (3 << (8 * 2));

    // USB_OTG
    // PA11,12
    // we dont need PA9 for bus sensing, since the device is bus powered
    GPIOA->MODER |= (2 << (11 * 2)) | (2 << (12 * 2)); // alternate
    GPIOA->OSPEEDR |= (2 << (11 * 2)) | (2 << (12 * 2)); // fast
    GPIOA->AFR[1] |= (10 << 12) | (10 << 16); // AF10
}

void clockInit(void) {
    // flash waits
    FLASH->ACR |=  FLASH_ACR_LATENCY_3WS;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_3WS);

    //mco1 for PLL verification 
    // 96mhz / 2 = 48
    RCC->CFGR |= (4 << RCC_CFGR_MCO1PRE_Pos) | (3 << RCC_CFGR_MCO1_Pos); 

    // enable HSE
    RCC->CR |= RCC_CR_HSEON;
    while ((RCC->CR & RCC_CR_HSERDY_Msk) == 0) {
        GPIOC->ODR &= ~GPIO_ODR_ODR_13;
    }
    GPIOC->ODR |= GPIO_ODR_ODR_13;

    // PLL configuration
    RCC->PLLCFGR = 0;
    RCC->PLLCFGR |= (8 << RCC_PLLCFGR_PLLQ_Pos) | 
                    (1 << RCC_PLLCFGR_PLLP_Pos) | 
                    (384 << RCC_PLLCFGR_PLLN_Pos) | 
                    (25 << RCC_PLLCFGR_PLLM_Pos) | 
                    RCC_PLLCFGR_PLLSRC_HSE;
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0) {
        GPIOC->ODR &= ~GPIO_ODR_ODR_13;
    }
    GPIOC->ODR |= GPIO_ODR_ODR_13;

    // swtich cpu clock to pll
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_PLL) == 0) {
        GPIOC->ODR &= ~GPIO_ODR_ODR_13;
    }
    GPIOC->ODR |= GPIO_ODR_ODR_13;
}

void USBInit(void) {
    
    // USB_OTG clock
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
    while((OTG->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL) == 0);
    
    // soft core reset
    while ((OTG->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL) == 0);
    OTG->GRSTCTL |= USB_OTG_GRSTCTL_CSRST;
    while ((OTG->GRSTCTL & USB_OTG_GRSTCTL_CSRST) != 0);
    
    // Force device & TRDT
    // we dont have to do USB_OTG_GUSBCFG_PHYSEL, as we only have the FS PHY provided by Synopsis,
    // and is set to 1 on reset
    OTG->GUSBCFG |= USB_OTG_GUSBCFG_FDMOD | (0x06 << USB_OTG_GUSBCFG_TRDT_Pos);
    
    // enable clock (commonly written as OTG->PCGCCTL = 0; or *OTGPCTL = 0;)
    *(__IO uint32_t *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_PCGCCTL_BASE) = 0;
    
    OTG->GCCFG |= USB_OTG_GCCFG_PWRDWN | USB_OTG_GCCFG_NOVBUSSENS;

    // begin device config
    //soft disconnect
    OTGD->DCTL |= USB_OTG_DCTL_SDIS;
    
    OTGD->DCFG &= ~USB_OTG_DCFG_DAD;
    OTGD->DCFG |= (0x03 << USB_OTG_DCFG_DSPD_Pos) | USB_OTG_DCFG_NZLSOHSK;

    // RX FIFO size in 32b words (128B)
    OTG->GRXFSIZ |= (32 << USB_OTG_GRXFSIZ_RXFD_Pos);

    // fifo size 64B and offset of ep0 tx is 0x20
    OTG->DIEPTXF0_HNPTXFSIZ = (16 << USB_OTG_DCFG_DSPD_Pos) | 0x20;

    // USB interrupts
    OTG->GINTMSK = USB_OTG_GINTMSK_USBRST | 
                   USB_OTG_GINTMSK_ENUMDNEM | 
                   USB_OTG_GINTMSK_SOFM | 
                   USB_OTG_GINTMSK_USBSUSPM | 
                   USB_OTG_GINTMSK_WUIM | 
                   USB_OTG_GINTMSK_IEPINT | 
                   USB_OTG_GINTMSK_RXFLVLM;
    OTGD->DIEPMSK |= USB_OTG_DIEPMSK_XFRCM;

    // clear interrtups (rc_w1)
    OTG->GINTSTS = 0xFFFFFFFF;

    // unmaks global interrupts
    OTG->GAHBCFG |= USB_OTG_GAHBCFG_GINT;

}

void USBConnect(void) {
    //OTG->GCCFG |= USB_OTG_GCCFG_PWRDWN;
    OTGD->DCTL &= ~USB_OTG_DCTL_SDIS;
}

void OTG_FS_IRQHandler(void) {
    uint32_t a = OTG->GINTSTS;
    if (a & USB_OTG_GINTSTS_USBRST) {
        // clear device address
        OTGD->DCFG &= ~(0x7F << USB_OTG_DCFG_DAD_Pos);

        // reset state machine 

        // set SNAK for ep0
        DOEPCTL0_reg |= USB_OTG_DOEPCTL_SNAK;

        // unmask interrupts
        // read DAINT to see which ep was interrupted
        OTGD->DAINTMSK = (1 << 16) | 1;

        OTGD->DOEPMSK = USB_OTG_DOEPMSK_STUPM | 
                        USB_OTG_DOEPMSK_XFRCM | 
                        USB_OTG_DOEPMSK_OTEPDM |
                        USB_OTG_DOEPMSK_OTEPSPRM;
        OTGD->DIEPMSK = USB_OTG_DIEPMSK_XFRCM;
        

        // set GRFXSIZ
        OTG->GRXFSIZ &= ~(0xFFFF << USB_OTG_GRXFSIZ_RXFD_Pos);
        OTG->GRXFSIZ |= (32 << USB_OTG_GRXFSIZ_RXFD_Pos);

        // EP0 tx fifo size
        OTG->DIEPTXF0_HNPTXFSIZ &= ~0xFFFFFFFF;
        OTG->DIEPTXF0_HNPTXFSIZ = (16 << 16) | 0x20;

        // num B2B setup packets max
        //DOEPTSIZ0_reg |= (2 << USB_OTG_DOEPTSIZ_STUPCNT_Pos);

        USB_OTG_INEndpointTypeDef* in0 = USBEPIn(0);
        USB_OTG_OUTEndpointTypeDef* out0 = USBEPOut(0);

        // ep 0 cfg
        in0->DIEPCTL |= (3 << USB_OTG_DIEPCTL_MPSIZ_Pos);

        out0->DOEPTSIZ = (1 << USB_OTG_DOEPTSIZ_STUPCNT_Pos) | (1 << USB_OTG_DOEPTSIZ_PKTCNT_Pos);
        out0->DOEPCTL |= USB_OTG_DOEPCTL_SNAK;
    } 
    if (a & USB_OTG_GINTSTS_ENUMDNE) {
        if (((OTGD->DSTS >> USB_OTG_DSTS_ENUMSPD) & 0x03) != 3) {
            GPIOC->ODR &= ~GPIO_ODR_ODR_13;
        }
        else {
            GPIOC->ODR |= GPIO_ODR_ODR_13;
        }
    }
}

int main() {
    interruptInit();
    GPIOInit();
    clockInit();
    USBInit();
    USBConnect();

    while (1);

    return 0;
}