#ifndef USBDevice_h
#define USBDevice_h

#include "USBCoreDefs.h"

typedef struct {
    USBDeviceState deviceState;
    USBControlTransferStage controlTransferStage;
    uint8_t cfgVal;

    void const * ptrOutBuffer;
    uint32_t outDataSize;
    void const *ptrInBuffer;
    uint32_t inDataSize;
} USBdevice;

#endif // #ifndef USBDevice_h