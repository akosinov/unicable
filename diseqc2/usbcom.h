#include "usbd_desc.h"

// main functions
void UsbComInit();
void UsbComSend(uint8_t data);
void UsbComConfirmRx();

// events (weak)
uint8_t UsbComOnRx(uint8_t data); // return 0 if NACK, then use UsbComConfirmRx
