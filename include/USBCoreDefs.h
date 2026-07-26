#ifndef USBCoreDefs_h
#define USBCoreDefs_h

#include <stm32f411xe.h>

typedef enum USBEPType {
    USB_ENDPOINT_TYPE_CONTROL,
    USB_ENDPOINT_TYPE_ISO,
    USB_ENDPOINT_TYPE_BULK,
    USB_ENDPOINT_TYPE_INT
} USBEPType;

#define OTG ((USB_OTG_GlobalTypeDef *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_GLOBAL_BASE))
#define OTGD ((USB_OTG_DeviceTypeDef *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_DEVICE_BASE))
#define PCGCCTL ((uint32_t *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_PCGCCTL_BASE))


#endif // #ifndef USBCoreDefs_h