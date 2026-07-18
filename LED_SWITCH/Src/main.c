#include <stdint.h>
#include <stdio.h>

/* Register Addresses for STM32F407 */
#define RCC_AHB1ENR    *((volatile uint32_t*)0x40023830)

#define GPIOD_MODER    *((volatile uint32_t*)0x40020C00)
#define GPIOD_ODR      *((volatile uint32_t*)0x40020C14)

#define GPIOA_MODER    *((volatile uint32_t*)0x40020000)
#define GPIOA_IDR      *((volatile uint32_t*)0x40020010)

uint32_t Count = 0;

void delay(volatile uint32_t count) {
    while(count--) {}
}

int main(void) {
    // 1. Enable clock for GPIOA (Bit 0) and GPIOD (Bit 3)
    RCC_AHB1ENR |= (1 << 3) | (1 << 0);

    // 2. Set PD12 (Green LED) to Output mode
    GPIOD_MODER &= ~(3 << 24);
    GPIOD_MODER |= (1 << 24);

    // Set PA0 to Input mode
    GPIOA_MODER &= ~(3 << 0);

    while(1) {
        // 3. Check for Button Press
        if (GPIOA_IDR & (1 << 0)) {
            delay(50000); // debounce delay

            if (GPIOA_IDR & (1 << 0)) { // Double-check press is stable
                Count++;
                printf("%lu\n", Count); // Fixed format string to prevent crash

                // CRITICAL: Wait here until button is completely released
                while (GPIOA_IDR & (1 << 0)) {}
            }
        }

        // 4. Handle State Actions
        if (Count % 2 == 0) {
            // Blink cycle
            GPIOD_ODR |= (1 << 12);
        }
        else {
            // Keep LED completely OFF
            GPIOD_ODR &= ~(1 << 12);
        }
    }
}
