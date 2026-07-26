#include <stm32f411xe.h>
#include "USBDDriver.h"
#include "USBCoreDefs.h"

void USBDPinInit(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIOA->AFR[1] &= ~(GPIO_AFRH_AFSEL11 | GPIO_AFRH_AFSEL12);
    GPIOA->AFR[1] |= ((10 << GPIO_AFRH_AFSEL11_Pos) | (10 << GPIO_AFRH_AFSEL12_Pos));

    GPIOA->MODER &= ~(GPIO_MODER_MODER11_Msk | GPIO_MODER_MODER12_Msk);
    GPIOA->MODER |= ((10 << GPIO_MODER_MODER11_Pos) | (10 << GPIO_MODER_MODER12_Pos));
}

void USBDCoreInit(void) {
    // enable otg clock
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;

    // device mode and stuff
    OTG->GUSBCFG &= ~(USB_OTG_GUSBCFG_FHMOD_Msk | USB_OTG_GUSBCFG_PHYSEL_Msk | USB_OTG_GUSBCFG_TRDT_Msk);
    OTG->GUSBCFG |= (USB_OTG_GUSBCFG_FDMOD | USB_OTG_GUSBCFG_PHYLPCS | (0x06 << USB_OTG_GUSBCFG_TRDT_Pos));

    // speed
    OTGD->DCFG &= ~USB_OTG_DCFG_DSPD_Msk;
    OTGD->DCFG |= (3 << USB_OTG_DCFG_DSPD_Pos);

    // vbus sensing (IMPLEMENT LATER)
    OTG->GCCFG |= USB_OTG_GCCFG_NOVBUSSENS;

    // interrupts
    OTG->GINTMSK = 0;
    OTG->GINTMSK |= (USB_OTG_GINTMSK_USBRST | USB_OTG_GINTMSK_ENUMDNEM | USB_OTG_GINTMSK_SOFM |
                    USB_OTG_GINTMSK_USBSUSPM | USB_OTG_GINTMSK_WUIM | USB_OTG_GINTMSK_IEPINT |
                    USB_OTG_GINTMSK_OEPINT | USB_OTG_GINTMSK_RXFLVLM);
    
    // clear interrupts
    OTG->GINTSTS = 0xFFFFFFFF;

    // global interrupt enable
    OTG->GAHBCFG |= USB_OTG_GAHBCFG_GINT;
}

void USBDConnect(uint8_t enable) {
    if (enable >= 1) {
        OTG->GCCFG |= USB_OTG_GCCFG_PWRDWN;
        OTGD->DCTL &= ~USB_OTG_DCTL_SDIS;
    } else {
        OTG->GCCFG &= ~USB_OTG_GCCFG_PWRDWN;
        OTGD->DCTL |= USB_OTG_DCTL_SDIS;
    }
}

static void recalculateFIFOStarts(void) {
    uint16_t startADD = OTG->GRXFSIZ & USB_OTG_GRXFSIZ_RXFD;

    OTG->DIEPTXF0_HNPTXFSIZ &= ~USB_OTG_NPTXFSA;
    OTG->DIEPTXF0_HNPTXFSIZ |= (startADD << USB_OTG_NPTXFSA_Pos);

    startADD += (OTG->DIEPTXF0_HNPTXFSIZ & USB_OTG_TX0FD) >> USB_OTG_TX0FD_Pos;

    for (uint8_t i = 0; i < EPCOUNT - 1; i++) {
        OTG->DIEPTXF[i] &= ~USB_OTG_NPTXFSA;
        OTG->DIEPTXF[i] |= startADD;

        startADD += ((OTG->DIEPTXF[i] & USB_OTG_NPTXFD) >> USB_OTG_NPTXFD_Pos) * 4;
    }
}

static void RXFIFOConfig(uint16_t rxSize) {
    rxSize = 10 + (2 * ((rxSize / 4) + 1));

    OTG->GRXFSIZ &= ~USB_OTG_GRXFSIZ_RXFD;
    OTG->GRXFSIZ |= (rxSize << USB_OTG_GRXFSIZ_RXFD_Pos);

    recalculateFIFOStarts();
}

static void TXFIFOConfig(uint8_t epNum, uint16_t txSize) {
    txSize = (txSize + 3) / 4;

    if (epNum == 0) {
        OTG->DIEPTXF0_HNPTXFSIZ &= ~USB_OTG_NPTXFD_Msk;
        OTG->DIEPTXF0_HNPTXFSIZ |= (txSize << USB_OTG_NPTXFD_Pos);
    }
    else {
        OTG->DIEPTXF[epNum - 1] &= ~USB_OTG_NPTXFD;
        OTG->DIEPTXF[epNum - 1] |= (txSize << USB_OTG_NPTXFD_Pos);
    }

    recalculateFIFOStarts();
}

static void RXFIFOFlush(void) {
    OTG->GRSTCTL |= USB_OTG_GRSTCTL_RXFFLSH;
}

static void TXFIFOFlush(uint8_t epNum) {
    OTG->GRSTCTL &= ~USB_OTG_GRSTCTL_TXFNUM;
    OTG->GRSTCTL |= ((epNum << USB_OTG_GRSTCTL_TXFNUM_Pos) | USB_OTG_GRSTCTL_TXFFLSH);
}

static void ep0Config() {
    // unmask ep interrupt
    OTGD->DAINTMSK |= ((1 << 16) | (1 << 0));

    // 00 in MPSIZ = 64B max packet size
    INEP(0)->DIEPCTL &= (0x03 << USB_OTG_DIEPCTL_MPSIZ_Pos);
    INEP(0)->DIEPCTL |= (USB_OTG_DIEPCTL_USBAEP | USB_OTG_DIEPCTL_SNAK);

    // same thing
    OUTEP(0)->DOEPCTL &= (0x03 << USB_OTG_DOEPCTL_MPSIZ_Pos);
    OUTEP(0)->DOEPCTL |= (USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK);

    RXFIFOConfig(64);
    TXFIFOConfig(0, 64);
}

static void inEPConfig(uint8_t epNum, USBEPType epType, uint16_t epSize) {
    OTGD->DAINTMSK |= (1 << epNum);

    INEP(epNum)->DIEPCTL &= (USB_OTG_DIEPCTL_MPSIZ | USB_OTG_DIEPCTL_EPTYP);
    INEP(epNum)->DIEPCTL |= (USB_OTG_DIEPCTL_USBAEP | (epSize << USB_OTG_DIEPCTL_MPSIZ_Pos) | USB_OTG_DIEPCTL_SNAK |
                            (epType << USB_OTG_DIEPCTL_EPTYP_Pos) | (epNum << USB_OTG_DIEPCTL_TXFNUM_Pos) | USB_OTG_DIEPCTL_SD0PID_SEVNFRM);

    TXFIFOConifg(epNum, epSize);
}

static void resetHandler(void) {

}

void GINTSTSHandler(void) {
    volatile uint32_t a = OTG->GINTSTS;

    if (a & USB_OTG_GINTSTS_USBRST) {
        OTG->GINTSTS |= USB_OTG_GINTSTS_USBRST;

    }
    else if (a & USB_OTG_GINTSTS_ENUMDNE) {
        OTG->GINTSTS |= USB_OTG_GINTSTS_ENUMDNE;

    }
    else if (a & USB_OTG_GINTSTS_RXFLVL) {
        OTG->GINTSTS |= USB_OTG_GINTSTS_RXFLVL;

    }
    else if (a & USB_OTG_GINTSTS_IEPINT) {
        OTG->GINTSTS |= USB_OTG_GINTSTS_IEPINT;

    }
    else if (a & USB_OTG_GINTSTS_OEPINT) {
        OTG->GINTSTS |= USB_OTG_GINTSTS_OEPINT;

    }
}