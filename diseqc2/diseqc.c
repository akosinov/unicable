#include <stm32f0xx.h>
#include "diseqc.h"

// CONFIG
#define DISEQC_NOISE_MAX	(2085 + 40)	//default upper noise threshold
#define DISEQC_NOISE_MIN	(2085 - 40)	//default lower noise threshold

#define DISEQC_T0_MIN		800	//all times in microseconds
#define DISEQC_T0_MAX		1200
#define DISEQC_T1_MIN		400
#define DISEQC_T1_MAX		600
#define DISEQC_TPAUSE		1800

#define DISEQC_ANSWER_TIMEOUT	25	// diseqc answer timeout. ms
#define DISEQC_WAIT_TIMEOUT	50	// delay between diseqc transaction and TXC, ms

//TRANSMITTER
#define STATE_IDLE		0	// tx idle
#define STATE_SEND		1	// data tx in progress
#define STATE_LAST		2	// all data sent. awaiting one more cycle for complete rx
#define STATE_ANSWERWAIT	3	// awaiting answer
#define STATE_ANSWER		4	// answer rx in progress
#define STATE_WAIT		5	// delay between transaction and TXC

volatile uint8_t *ds_data, ds_parity, ds_state = STATE_IDLE;
volatile uint16_t ds_size, ds_sent;

void DiseqcSend(volatile uint8_t *data, uint8_t size) {
	if(ds_state == STATE_IDLE) {
		ds_data = data;
		ds_size = 9 * (uint16_t)size;
		ds_sent = 0;
		ds_parity = 0;
		ds_state = STATE_SEND;
	}
}

void TIM17_IRQHandler() {
	TIM17->SR = 0;
	if(ds_state == STATE_LAST) {
		ds_state = STATE_ANSWERWAIT;
		TIM7->CNT = 0;
		TIM7->ARR = DISEQC_ANSWER_TIMEOUT;
		TIM7->CR1 |= TIM_CR1_CEN;
	} else if(ds_state == STATE_SEND) {
		if(ds_sent >= ds_size) {
			TIM17->CCR1 = 0;
			ds_state = STATE_LAST;
		} else {
			if((ds_sent % 9) < 8 ) { //ordinary bit
				if((ds_data[ds_sent / 9] << (ds_sent % 9)) & 0x80) { //one
					TIM17->CCR1 = 1;
					ds_parity++;
				} else { //zero
					TIM17->CCR1 = 2;
				}
			} else { //parity
				TIM17->CCR1 = (ds_parity % 2) ? 2 : 1;
				ds_parity = 0;
			}
			ds_sent++;
		}
	}
}

void TIM7_IRQHandler() {
	TIM7->SR = 0;
	if(ds_state == STATE_ANSWERWAIT) {
		ds_state = STATE_WAIT;
		TIM7->CNT = 0;
		TIM7->ARR = DISEQC_WAIT_TIMEOUT;
		TIM7->CR1 |= TIM_CR1_CEN;
	} else {
		ds_state = STATE_IDLE;
		DiseqcOnTxc();
	}
}

//RECEIVER
volatile uint8_t dr_data = 0, dr_count = 0, dr_parity = 0, dr_echo = 1;

void DiseqcEchoOn() {
	dr_echo = 1;
}

void DiseqcEchoOff() {
	dr_echo = 0;
}

void DiseqcRcvReset() {
	dr_data = dr_count = dr_parity = 0;
}

void DiseqcDigitReceived( uint8_t digit ) {
	if(ds_state == STATE_ANSWERWAIT) { // answer reception started
		ds_state = STATE_ANSWER;
		TIM7->CR1 &= ~TIM_CR1_CEN;	// stop answer waiting timer
		TIM7->SR = 0;
	}
	dr_parity += digit;
	if(dr_count > 7) {
		if( dr_echo || (ds_state == STATE_IDLE) || (ds_state == STATE_ANSWER) || (ds_state == STATE_WAIT) )
			DiseqcOnRx(dr_data, !(dr_parity % 2));
		DiseqcRcvReset();
	}
	else {
		dr_data = (dr_data << 1) | digit;
		dr_count++;
	}
}

void TIM15_IRQHandler() {
	TIM15->SR = 0;
	if( dr_echo || (ds_state == STATE_IDLE) || (ds_state == STATE_ANSWER) || (ds_state == STATE_WAIT) ) {
		DiseqcOnPause(dr_count);
		DiseqcRcvReset();
	}
	if(ds_state == STATE_ANSWER) { // answer reception ended
		ds_state = STATE_WAIT;
		TIM7->CNT = 0;
		TIM7->ARR = DISEQC_WAIT_TIMEOUT;
		TIM7->CR1 |= TIM_CR1_CEN;
	}
}

void TIM3_IRQHandler() {
	TIM3->SR = 0;
	uint16_t siglen = TIM15->CNT;
	if( (siglen >= DISEQC_T1_MIN) && (siglen <= DISEQC_T1_MAX) )
		DiseqcDigitReceived(1);
	else if( (siglen >= DISEQC_T0_MIN) && (siglen <= DISEQC_T0_MAX) )
		DiseqcDigitReceived(0);
	else
		DiseqcOnNoise(siglen);
}

//INIT
volatile uint16_t dr_zerosource = 0;

void DiseqcInit() {
// TRANSMITTER
// gpio PA8-output fast speed pull down AF2 (Tim1_Ch1)
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	GPIOA->AFR[1] &= ~0xF;
	GPIOA->AFR[1] |= 0x2;
	GPIOA->MODER |= GPIO_MODER_MODER8_1; //output AF
	GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEEDR8_0 | GPIO_OSPEEDR_OSPEEDR8_1; //fast speed
	GPIOA->PUPDR |= GPIO_PUPDR_PUPDR8_1; // pull down
// tim1 22 kHz 50% duty PWM mode1 on ch1 GATED by TIM17
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	TIM1->PSC = 1091 - 1; // pure magic 48000000 / 220000
	TIM1->ARR = 1;
	TIM1->CCR1 = 1; // duty = 50%
	TIM1->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1PE; // pwm mode 2
	TIM1->EGR |= TIM_EGR_UG;
	TIM1->CCER = TIM_CCER_CC1E;
	TIM1->BDTR = TIM_BDTR_MOE;
	TIM1->SMCR = TIM_SMCR_TS_1 | TIM_SMCR_TS_0 | TIM_SMCR_SMS_2 | TIM_SMCR_SMS_0;
	TIM1->CR1 = TIM_CR1_CEN;
// TIM17 TRGO is always OC1 (not OCref)
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM17, ENABLE);
	NVIC_EnableIRQ(TIM17_IRQn);
	TIM17->PSC = 1091 * 22 - 1; //pure magic
	TIM17->ARR = 2;
	TIM17->CCR1 = 0;
	TIM17->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1  | TIM_CCMR1_OC1PE; // pwm mode 1
	TIM17->EGR |= TIM_EGR_UG;
	TIM17->CCER = TIM_CCER_CC1E;
	TIM17->BDTR = TIM_BDTR_MOE;
	TIM17->DIER = TIM_DIER_UIE;
	TIM17->CR1 = TIM_CR1_CEN;
// tim6. tx delays counter
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);
	NVIC_EnableIRQ(TIM7_IRQn);
	TIM7->PSC = 48000 - 1; // 1ms clock
	TIM7->EGR |= TIM_EGR_UG;
	TIM7->SR = 0;
	TIM7->DIER = TIM_DIER_UIE;
	TIM7->CR1 = TIM_CR1_OPM; //one pulse
// RECEIVER
// dac
	RCC_APB1PeriphClockCmd(RCC_APB1ENR_DACEN, ENABLE);
	DAC->DHR12R1 = DISEQC_NOISE_MAX;
	DAC->DHR12R2 = DISEQC_NOISE_MIN;
	DAC->CR = DAC_CR_EN1 | DAC_CR_EN2;
// comparators 1 & 2 +input from PA1 -input from DAC 1 & 2. No hysteresis out to TIM3 CH1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); //comparators
	COMP->CSR = COMP_CSR_COMP1OUTSEL_2 | COMP_CSR_COMP1OUTSEL_1 | COMP_CSR_COMP1INSEL_2 | \
				COMP_CSR_COMP1HYST_1 | COMP_CSR_COMP1HYST_0 | \
				COMP_CSR_COMP2OUTSEL_2 | COMP_CSR_COMP2OUTSEL_1 | COMP_CSR_COMP2INSEL_2 | COMP_CSR_COMP2INSEL_0 |  \
				COMP_CSR_COMP2HYST_1 | COMP_CSR_COMP2HYST_0 | \
				COMP_CSR_WNDWEN | COMP_CSR_COMP2POL;
	COMP->CSR |= COMP_CSR_COMP1EN | COMP_CSR_COMP2EN;
// DMA ch4 for TIM3 TRG. TIM3->CNT = 0
	RCC_AHBPeriphClockCmd(RCC_AHBENR_DMAEN, ENABLE);
	DMA1_Channel4->CPAR = (uint32_t)(&(TIM3->CNT));
	DMA1_Channel4->CMAR = (uint32_t)(&dr_zerosource);
	DMA1_Channel4->CNDTR = 1;
	DMA1_Channel4->CCR = DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_CIRC | DMA_CCR_DIR;
	DMA1_Channel4->CCR |= DMA_CCR_EN;
// DMA ch5 for TIM`5 TRG. TIM15->CNT = 0
	DMA1_Channel5->CPAR = (uint32_t)(&(TIM15->CNT));
	DMA1_Channel5->CMAR = (uint32_t)(&dr_zerosource);
	DMA1_Channel5->CNDTR = 1;
	DMA1_Channel5->CCR = DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_CIRC | DMA_CCR_DIR;
	DMA1_Channel5->CCR |= DMA_CCR_EN;
// tim15. PURE MAGIC. TRIGGER & RESET on SAME event from TIM3
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM15, ENABLE);
	NVIC_EnableIRQ(TIM15_IRQn);
	TIM15->ARR = 1600; //def
	TIM15->CCR1 = 1600; //def
	TIM15->PSC = 47;
	TIM15->EGR |= TIM_EGR_UG;
	TIM15->CCMR1 = TIM_CCMR1_OC1M_0; // ch1 as output one-pulse
	TIM15->DIER = TIM_DIER_TDE | TIM_DIER_CC1IE; //DMA req on trigger, INT on CC1
	TIM15->CR1 = TIM_CR1_OPM; //one pulse
	TIM15->SMCR = TIM_SMCR_TS_0 | TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1; // TRIGGER mode from TIM3
// tim3. PURE MAGIC. TRIGGER & RESET on SAME event from comparators
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	NVIC_EnableIRQ(TIM3_IRQn);
	TIM3->ARR = 2200; //def
	TIM3->CCR2 = 2200; //def
	TIM3->CCMR1 = TIM_CCMR1_CC1S_0 | TIM_CCMR1_OC2M_0; // channel 1 as input from ic1, ch2 as output one-pulse
	TIM3->DIER = TIM_DIER_TDE | TIM_DIER_CC2IE; //DMA req on trigger, INT on CC2
	TIM3->CR2 = TIM_CR2_MMS_0; //TRGO = ENABLE
	TIM3->CR1 = TIM_CR1_OPM; //one pulse
	TIM3->SMCR = TIM_SMCR_TS_2 | TIM_SMCR_TS_0 | TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1; // TRIGGER mode self-slave on channel1
}

// comparators control
void DiseqcSetCompValue(uint8_t comparator, uint16_t value) {
	if( comparator == DISEQC_COMP_MAX)
		DAC->DHR12R1 = value;
	else if( comparator == DISEQC_COMP_MIN)
		DAC->DHR12R2 = value;
}

uint16_t DiseqcGetCompValue(uint8_t comparator) {
	if( comparator == DISEQC_COMP_MAX)
		return DAC->DHR12R1;
	else if( comparator == DISEQC_COMP_MIN)
		return DAC->DHR12R2;
	return 0;
}

uint8_t DiseqcGetCompOutput(uint8_t comparator) {
	uint16_t mask = 0;
	if( comparator == DISEQC_COMP_MAX)
		mask = COMP_CSR_COMP1OUT;
	else if( comparator == DISEQC_COMP_MIN)
		mask = COMP_CSR_COMP2OUT;
	return (COMP->CSR & mask) ? 1 : 0;
}

// event weak stubs
void __attribute__((weak)) DiseqcOnRx(uint8_t data, uint8_t parityError) {}
void __attribute__((weak)) DiseqcOnPause(uint8_t srNotEmpty) {}
void __attribute__((weak)) DiseqcOnTxc() {}
void __attribute__((weak)) DiseqcOnNoise(uint16_t duration) {}
