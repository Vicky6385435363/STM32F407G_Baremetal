#include <stdint.h>
#include <stdio.h>
/* --- RCC Registers --- */
#define RCC_AHB1ENR  *((volatile uint32_t*)0x40023830)
#define RCC_APB2ENR  *((volatile uint32_t*)0x40023844)

/* --- GPIOA Registers --- */
#define GPIOA_MODER  *((volatile uint32_t*)0x40020000)

/* --- ADC1 Registers (Base: 0x40012000) --- */
#define ADC1_SR      *((volatile uint32_t*)0x40012000) // Status Register
#define ADC1_CR1     *((volatile uint32_t*)0x40012004) // Control 1
#define ADC1_CR2     *((volatile uint32_t*)0x40012008) // Control 2
#define ADC1_SQR3    *((volatile uint32_t*)0x40012034) // Sequence 3
#define ADC1_DR      *((volatile uint32_t*)0x4001204C) // Data Register

void init_adc(void) {
    // 1. Enable Clocks
    RCC_AHB1ENR |= (1 << 0);  // GPIOA Clock
    RCC_APB2ENR |= (1 << 8);  // ADC1 Clock

    // 2. Configure PA1 for Analog Mode
    // Pin 1 uses bits 2 and 3. Set them both to 1 (Binary 11)
    GPIOA_MODER |= (3 << 2);

    // 3. Configure the ADC Sequence
    // We only want 1 conversion, so we leave ADC_SQR1 length at 0
    // We want to read Channel 1 (PA1), so we write '1' to the first sequence slot
    ADC1_SQR3 = 1;

    // 4. Wake up the ADC
    ADC1_CR2 |= (1 << 0);     // ADON bit
}

uint16_t read_adc(void) {
    // 1. Start the conversion by setting SWSTART (Bit 30)
    ADC1_CR2 |= (1 << 30);

    // 2. Wait for the End Of Conversion (EOC) flag in the Status Register (Bit 1)
    // This is polling, just like the TIM5 delay!
    while (!(ADC1_SR & (1 << 1))) {}

    // 3. Read the 12-bit data (this automatically clears the EOC flag)
    return (uint16_t)(ADC1_DR & 0xFFF);
}

int main(void) {
    init_adc();
    uint16_t sensor_value = 0;

    while(1) {
        sensor_value = read_adc();
        printf("ADC Value: %u\n", sensor_value);

                // Add a small software delay so you don't flood the ITM console
                // (Printing millions of times per second will crash the debugger)
                for(volatile int i = 0; i < 500000; i++);
        // You now have a value from 0 to 4095 representing the voltage
    }
}
