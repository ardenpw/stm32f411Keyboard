#ifndef USBDDriver_h
#define USBDDriver_h

#include <stdint.h>
#include <stm32f411xe.h>

#define EPCOUNT 4

static inline USB_OTG_INEndpointTypeDef *INEP(uint8_t epNum) {
    return (USB_OTG_INEndpointTypeDef *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_IN_ENDPOINT_BASE + (0x20 * epNum));
}

static inline USB_OTG_OUTEndpointTypeDef *OUTEP(uint8_t epNum) {
    return (USB_OTG_OUTEndpointTypeDef *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_OUT_ENDPOINT_BASE + (0x20 * epNum));
}

void USBDPinInit(void);
void USBDCoreInit(void);
void USBDConnect(uint8_t enable);

#endif // #ifndef USBDDriver_h