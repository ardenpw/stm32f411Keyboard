#ifndef USBCoreDefs_h
#define USBCoreDefs_h

#include <stm32f411xe.h>

typedef enum USBEPType {
    USB_ENDPOINT_TYPE_CONTROL,
    USB_ENDPOINT_TYPE_ISO,
    USB_ENDPOINT_TYPE_BULK,
    USB_ENDPOINT_TYPE_INT
} USBEPType;

typedef struct {
    void (*onUSBResetRecieved)();
    void (*onSetupDataReceived)(uint8_t epNum, uint16_t bcnt);
    void (*onOutDataReceived)(uint8_t epNum, uint16_t bcnt);
    void (*onInTransferCompleted)(uint8_t epNum);
    void (*onOutTransferCompleted)(uint8_t epNum);
    void (*onUSBPolled)();
} USBevents;

typedef enum {
    USB_DEVICE_STATE_DEFAULT,
    USB_DEVICE_STATE_ADDRESSED,
    USB_DEVICE_STATE_CONFIGURED,
    USB_DEVICE_STATE_SUSPENDED
} USBDeviceState;

typedef enum {
    USB_CONTROL_STAGE_SETUP, // also USB_CONTROL_STAGE_IDLE
    USB_CONTROL_STAGE_DATA_OUT,
    USB_CONTROL_STAGE_DATA_IN,
    USB_CONTROL_STAGE_DATA_IN_IDLE,
    USB_CONTROL_STAGE_DATA_IN_ZERO,
    USB_CONTROL_STAGE_STATUS_OUT,
    USB_CONTROL_STAGE_STATUS_IN
} USBControlTransferStage;

#define OTG ((USB_OTG_GlobalTypeDef *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_GLOBAL_BASE))
#define OTGD ((USB_OTG_DeviceTypeDef *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_DEVICE_BASE))
#define PCGCCTL ((uint32_t *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_PCGCCTL_BASE))


#endif // #ifndef USBCoreDefs_h