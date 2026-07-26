#include "deviceInit.h"
#include "USBDFramework.h"

int main() {
    deviceInit();
    USBInit();

    for(;;);
    
    return 0;
}