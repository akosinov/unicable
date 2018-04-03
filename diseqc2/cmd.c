#include <stdio.h>
#include "diseqc.h"
#include "usbcom.h"

// diseqc event handlers
volatile uint16_t rxcount = 0;

void DiseqcOnRx(uint8_t data, uint8_t parityError) {
	if(parityError)
		printf("##");
	if(rxcount)
		printf(" ");
	printf("%02X", data);
	rxcount++;
}

void DiseqcOnPause(uint8_t srNotEmpty) {
	if(srNotEmpty)
		printf("##");
	printf("\n");
	rxcount = 0;
}

void DiseqcOnTxc() {
	UsbComConfirmRx();
}

void DiseqcOnNoise(uint16_t duration) {
	printf("(%u)\n", duration);
}

// usb com event handler
volatile uint8_t txbuf[64], txbufp = 0, txbufs = 0;

void UsbComDigit(uint8_t byte) {
	if(!txbufs) //first digit
		txbufs = byte | 0xF0;
	else { //second digit
		txbuf[txbufp++] = ((txbufs & 0x0F) << 4) | byte;
		txbufs = 0;
		if(txbufp == sizeof(txbuf)) {
			txbufp = 0;
			printf(">>\n");
		}
	}
}

uint8_t UsbComOnRx(uint8_t data) {
	if((data == 0x0D) || (data == 0x0A)) {
		txbufs = 0;
		if(txbufp) {
			DiseqcSend(txbuf, txbufp);
			txbufp = 0;
			return 0;
		}
	} else if(data == 'Y')
		DiseqcEchoOn();
	else if(data == 'N')
		DiseqcEchoOff();
	else if(data == '.')
		printf("%c", 0x04); //EOT
	else if (data >= '0' && data <= '9')
		UsbComDigit(data - '0');
	else if (data >= 'a' && data <= 'f')
		UsbComDigit(data - 'a' + 10);
	else if (data >= 'A' && data <= 'F')
		UsbComDigit(data - 'A' + 10);
	return 1;
}

int main() {
	setbuf(stdout, NULL);
	UsbComInit(); // init sysclock too
	DiseqcInit();

	while (1) {
	}
	return 0;
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line) {
	while (1);
}
#endif

