#include "USBDDriver.h"

void USBInit(void) {
    USBDPinInit();
    USBDCoreInit();
    USBDConnect(1);
}