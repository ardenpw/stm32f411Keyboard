#ifndef USBDFramework_h
#define USBDFramework_h

#include "USBDevice.h"

void USBInit(USBdevice *usbDevice);
void USBPoll(void);

#endif // #ifndef USBDFramework_h