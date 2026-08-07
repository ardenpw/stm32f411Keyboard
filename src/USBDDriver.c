#include <stm32f411xe.h>
#include "USBDDriver.h"
#include "USBCoreDefs.h"

static void USBDPinInit(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIOA->AFR[1] &= ~(GPIO_AFRH_AFSEL11 | GPIO_AFRH_AFSEL12);
    GPIOA->AFR[1] |= ((10 << GPIO_AFRH_AFSEL11_Pos) | (10 << GPIO_AFRH_AFSEL12_Pos));

    GPIOA->MODER &= ~(GPIO_MODER_MODER11_Msk | GPIO_MODER_MODER12_Msk);
    GPIOA->MODER |= ((10 << GPIO_MODER_MODER11_Pos) | (10 << GPIO_MODER_MODER12_Pos));
}

static void USBDCoreInit(void) {
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

    OTGD->DOEPMSK |= USB_OTG_DOEPMSK_XFRCM;
    OTGD->DIEPMSK |= USB_OTG_DIEPMSK_XFRCM;
}

static void USBDSetDAD(uint8_t addr) {
    OTGD->DCFG &= ~USB_OTG_DCFG_DAD;
    OTGD->DCFG |= (addr << USB_OTG_DCFG_DAD_Pos);
}

static void USBDConnect(uint8_t enable) {
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
    // adding 3 guarantees word alignment
    txSize = (txSize + 3) / 4;

    if (epNum == 0) {
        OTG->DIEPTXF0_HNPTXFSIZ &= ~USB_OTG_TX0FD;
        OTG->DIEPTXF0_HNPTXFSIZ |= (txSize << USB_OTG_TX0FD_Pos);
    }
    else {
        OTG->DIEPTXF[epNum - 1] &= ~USB_OTG_NPTXFD;
        OTG->DIEPTXF[epNum - 1] |= (txSize << USB_OTG_NPTXFD_Pos);
    }

    recalculateFIFOStarts();
}

static void readPacket(void *buffer, uint16_t size) {
    uint32_t *fifo = FIFO(0);

    // scan in words
    for (; size >= 4; size -= 4, buffer += 4) {
        uint32_t data = *fifo;
        *((uint32_t *)buffer) = data; 
    }

    // scan with our last word, but turn it into bytes (not word aligned)
    if (size > 0) {
        uint32_t data = *fifo;

        for (; size > 0; size--, buffer++, data >>= 8) {
            *((uint8_t *)buffer) = 0xFF & data;
        }
    }
}

static void writePacket(uint8_t epNum, void const *buffer, uint16_t size) {
    uint32_t *fifo = FIFO(epNum);
    USB_OTG_INEndpointTypeDef *iep = INEP(epNum);

    iep->DIEPTSIZ = 0;
    iep->DIEPTSIZ |= ((1 << USB_OTG_HCTSIZ_PKTCNT_Pos) | (size << USB_OTG_DIEPTSIZ_XFRSIZ_Pos));

    iep->DIEPCTL &= USB_OTG_DIEPCTL_STALL;
    iep->DIEPCTL |= (USB_OTG_DIEPCTL_CNAK | USB_OTG_DIEPCTL_EPENA);

    size = (size + 3) / 4;

    for(; size > 0; size--, buffer += 4) {
        *fifo = *((uint32_t *)buffer);
    }
}

static void RXFIFOFlush(void) {
    OTG->GRSTCTL |= USB_OTG_GRSTCTL_RXFFLSH;
}

static void TXFIFOFlush(uint8_t epNum) {
    OTG->GRSTCTL &= ~USB_OTG_GRSTCTL_TXFNUM;
    OTG->GRSTCTL |= ((epNum << USB_OTG_GRSTCTL_TXFNUM_Pos) | USB_OTG_GRSTCTL_TXFFLSH);
}

static void deconfigEP(uint8_t epNum) {
    USB_OTG_INEndpointTypeDef *epi = INEP(epNum);
    USB_OTG_OUTEndpointTypeDef *epo = OUTEP(epNum);

    OTGD->DAINTMSK &= ~((1 << epNum) | (1 << 16 << epNum));

    epi->DIEPINT |= 0x287B;
    epo->DOEPINT |= 0x313B;

    if (epi->DIEPCTL & USB_OTG_DIEPCTL_EPENA) {
        epi->DIEPCTL |= USB_OTG_DIEPCTL_EPDIS;
    }

    epi->DIEPCTL &= ~USB_OTG_DIEPCTL_USBAEP;

    if (epNum != 0) {
        if (epo->DOEPCTL & USB_OTG_DOEPCTL_EPENA) {
            epo->DOEPCTL |= USB_OTG_DOEPCTL_EPDIS;
        }
        epo->DOEPCTL &= ~USB_OTG_DOEPCTL_USBAEP;
    }
}

static void ep0Config() {
    // unmask ep interrupt
    OTGD->DAINTMSK |= ((1 << 16) | (1 << 0));

    // 00 in MPSIZ = 64B max packet size
    INEP(0)->DIEPCTL &= ~(0x03 << USB_OTG_DIEPCTL_MPSIZ_Pos);
    INEP(0)->DIEPCTL |= (USB_OTG_DIEPCTL_USBAEP | USB_OTG_DIEPCTL_SNAK);

    // same thing
    OUTEP(0)->DOEPCTL &= ~(0x03 << USB_OTG_DOEPCTL_MPSIZ_Pos); 
    OUTEP(0)->DOEPCTL |= (USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK);

    RXFIFOConfig(64);
    TXFIFOConfig(0, 64);
}

static void inEPConfig(uint8_t epNum, USBEPType epType, uint16_t epSize) {
    OTGD->DAINTMSK |= (1 << epNum);

    INEP(epNum)->DIEPCTL &= (USB_OTG_DIEPCTL_MPSIZ | USB_OTG_DIEPCTL_EPTYP);
    INEP(epNum)->DIEPCTL |= (USB_OTG_DIEPCTL_USBAEP | (epSize << USB_OTG_DIEPCTL_MPSIZ_Pos) | USB_OTG_DIEPCTL_SNAK |
                            (epType << USB_OTG_DIEPCTL_EPTYP_Pos) | (epNum << USB_OTG_DIEPCTL_TXFNUM_Pos) | USB_OTG_DIEPCTL_SD0PID_SEVNFRM);

    TXFIFOConfig(epNum, epSize);
}

static void resetHandler(void) {
    GPIOC->ODR &= ~(1 << 13);

    for (uint8_t i = 0; i < EPCOUNT; i++) {
        deconfigEP(i);
    }
}

static void rxflvlHandler() {
    uint32_t r = OTG->GRXSTSP;
    uint8_t epNum = ((r & USB_OTG_GRXSTSP_EPNUM) >> USB_OTG_GRXSTSP_EPNUM_Pos);
    uint16_t bcnt = ((r & USB_OTG_GRXSTSP_BCNT) >> USB_OTG_GRXSTSP_BCNT_Pos);
    uint8_t pktsts = ((r & USB_OTG_GRXSTSP_PKTSTS) >> USB_OTG_GRXSTSP_PKTSTS_Pos);

    switch (pktsts) {
        case (0x02): // out packet (data)
            break;
        case (0x06): // setup packet (data)
            break;
        case (0x04): // setup transaction complete
            OUTEP(epNum)->DOEPCTL |= (USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA);
            break;
        case (0x03): // out transfer complete
            OUTEP(epNum)->DOEPCTL |= (USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA);
            break;
    }
}

static void GINTSTSHandler(void) {
    volatile uint32_t a = OTG->GINTSTS;

    if (a & USB_OTG_GINTSTS_USBRST) {
        resetHandler();

        OTG->GINTSTS |= USB_OTG_GINTSTS_USBRST;
    }
    else if (a & USB_OTG_GINTSTS_ENUMDNE) {
        ep0Config();

        OTG->GINTSTS |= USB_OTG_GINTSTS_ENUMDNE;
    }
    else if (a & USB_OTG_GINTSTS_RXFLVL) {
        rxflvlHandler();

        OTG->GINTSTS |= USB_OTG_GINTSTS_RXFLVL;
    }
    else if (a & USB_OTG_GINTSTS_IEPINT) {
        OTG->GINTSTS |= USB_OTG_GINTSTS_IEPINT;
    }
    else if (a & USB_OTG_GINTSTS_OEPINT) {
        OTG->GINTSTS |= USB_OTG_GINTSTS_OEPINT;
    }
}

const USBdriver USBDriver = {
    .USBDCoreInit = &USBDCoreInit,
    .setDAD = &USBDSetDAD,
    .USBDPinInit = &USBDPinInit,
    .USBDConnect = &USBDConnect,
    .RXFIFOFlush = &RXFIFOFlush,
    .TXFIFOFlush = &TXFIFOFlush,
    .inEPConfig = &inEPConfig,
    .readPacket = &readPacket,
    .writePacket = &writePacket,
    .poll = &GINTSTSHandler
};