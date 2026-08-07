#include "USBDFramework.h"
#include "USBDDriver.h"
#include "USBDevice.h"

static USBdevice *USBHandle;

void USBInit(USBdevice *usbDevice) {
    USBHandle = usbDevice;
    USBDriver.USBDPinInit();
    USBDriver.USBDCoreInit();
    USBDriver.USBDConnect(1);
}

void USBPoll() {
    USBDriver.poll();
}

static void usbRstHandler(void) {
    USBHandle->inDataSize = 0;
    USBHandle->outDataSize = 0;
    USBHandle->cfgVal = 0;
    USBHandle->deviceState = USB_DEVICE_STATE_DEFAULT;
    USBHandle->controlTransferStage = USB_CONTROL_STAGE_SETUP;
    USBDriver.setDAD(0);
}

USBevents USBEvents = {
    //.onUSBResetRecieved = &usbRstHandler
    // TODO: this
};