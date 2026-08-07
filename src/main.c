#include "deviceInit.h"
#include "USBDFramework.h"
#include "USBDevice.h"

USBdevice USBDevice;
uint32_t buf[8];

int main() {
    deviceInit();

    USBDevice.ptrOutBuffer = &buf;
    USBInit(&USBDevice);

    for(;;) {
        USBPoll();
    }
    
    return 0;
}