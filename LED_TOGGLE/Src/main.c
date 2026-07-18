#include <stdint.h>

/* Register Addresses for STM32F407 */
// RCC Base: 0x40023800, AHB1ENR Offset: 0x30
#define RCC_AHB1ENR    *((volatile uint32_t*)0x40023830)

// GPIOD Base: 0x40020C00
#define GPIOD_MODER    *((volatile uint32_t*)0x40020C00)
#define GPIOD_ODR      *((volatile uint32_t*)0x40020C14)

/*
 * Simple software delay
 * 'volatile' prevents the compiler from optimizing the empty loop away
 */
void delay(volatile uint32_t count) {
    while(count--) {}
}

int main(void) {
    // 1. Enable clock for GPIOD (Bit 3)
    RCC_AHB1ENR |= (1 << 3);

    // 2. Set PD12 (Green LED) to Output mode
    // Clear bits 24 and 25 for Pin 12
    GPIOD_MODER &= ~(3 << 24);
    // Set bit 24 to make it '01' (Output)
    GPIOD_MODER |= (1 << 24);

    while(1) {
        // 3. Toggle PD12 using bitwise XOR
        GPIOD_ODR ^= (1 << 12);

        // 4. Wait
        delay(1000000);
    }
}
