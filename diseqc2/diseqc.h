// main functions
void DiseqcInit();
void DiseqcSend(volatile uint8_t *data, uint8_t size); // data must be volatile

// events (weak)
void DiseqcOnRx(uint8_t data, uint8_t parityError);
void DiseqcOnPause(uint8_t srNotEmpty);
void DiseqcOnTxc();
void DiseqcOnNoise(uint16_t duration);

// echp control
void DiseqcEchoOn();
void DiseqcEchoOff();

// comparators control
#define DISEQC_COMP_MAX		1 // comparator number for upper windows border
#define DISEQC_COMP_MIN		2 // comparator number for lower windows border

void DiseqcSetCompValue(uint8_t comparator, uint16_t value);
uint16_t DiseqcGetCompValue(uint8_t comparator);
uint8_t DiseqcGetCompOutput(uint8_t comparator);
