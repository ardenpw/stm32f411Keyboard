#ifndef USBDDriver_h
#define USBDDriver_h

#include <stdint.h>
#include <stm32f411xe.h>
#include "USBCoreDefs.h"

#define EPCOUNT 4

static inline USB_OTG_INEndpointTypeDef *INEP(uint8_t epNum) {
    return (USB_OTG_INEndpointTypeDef *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_IN_ENDPOINT_BASE + (0x20 * epNum));
}

static inline USB_OTG_OUTEndpointTypeDef *OUTEP(uint8_t epNum) {
    return (USB_OTG_OUTEndpointTypeDef *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_OUT_ENDPOINT_BASE + (0x20 * epNum));
}

static inline __IO uint32_t *FIFO(uint8_t epNum) {
    return (__IO uint32_t *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_FIFO_BASE + (0x1000 * epNum));
}

typedef struct {
    void (*USBDCoreInit)();
    void (*setDAD)(uint8_t addr);
    void (*USBDPinInit)();
    void (*USBDConnect)(uint8_t enable);
    void (*RXFIFOFlush)();
    void (*TXFIFOFlush)(uint8_t epNum);
    void (*inEPConfig)(uint8_t epNum, USBEPType epType, uint16_t epSize);
    void (*readPacket)(void *buffer, uint16_t size);
    void (*writePacket)(uint8_t epNum, void const *buffer, uint16_t size);
    void (*poll)();
} USBdriver;

extern const USBdriver USBDriver;
extern USBevents USBEvents;

#endif // #ifndef USBDDriver_h