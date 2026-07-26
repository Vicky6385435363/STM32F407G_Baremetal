#include <stdint.h>

/* --- RCC Registers --- */
#define RCC_AHB1ENR  *((volatile uint32_t*)0x40023830)
#define RCC_APB1ENR  *((volatile uint32_t*)0x40023840)

/* --- GPIO Registers --- */
#define GPIOD_MODER  *((volatile uint32_t*)0x40020C00)
#define GPIOD_AFRH   *((volatile uint32_t*)0x40020C24)

/* --- TIM4 Registers (PWM) --- */
#define TIM4_CR1     *((volatile uint32_t*)0x40000800)
#define TIM4_CCMR1   *((volatile uint32_t*)0x40000818)
#define TIM4_CCER    *((volatile uint32_t*)0x40000820)
#define TIM4_PSC     *((volatile uint32_t*)0x40000828)
#define TIM4_ARR     *((volatile uint32_t*)0x4000082C)
#define TIM4_CCR1    *((volatile uint32_t*)0x40000834)

/* --- TIM5 Registers (Hardware Delay) --- */
// Base Address for TIM5 is 0x40000C00
#define TIM5_CR1     *((volatile uint32_t*)0x40000C00)
#define TIM5_SR      *((volatile uint32_t*)0x40000C10)
#define TIM5_CNT     *((volatile uint32_t*)0x40000C24)
#define TIM5_PSC     *((volatile uint32_t*)0x40000C28)
#define TIM5_ARR     *((volatile uint32_t*)0x40000C2C)


void init_delay_timer(void) {
    // 1. Enable TIM5 clock (Bit 3 on APB1)
    RCC_APB1ENR |= (1 << 3);

    // 2. Set Prescaler for 1ms ticks (assuming 16MHz clock)
    TIM5_PSC = 16000 - 1;

    // Ensure timer is off initially
    TIM5_CR1 &= ~(1 << 0);
}

void delay_ms(uint32_t ms) {
    // 1. Set the target delay in milliseconds
    TIM5_ARR = ms;

    // 2. Reset the current counter to 0
    TIM5_CNT = 0;

    // 3. Clear the Update Interrupt Flag (UIF) in the Status Register (Bit 0)
    TIM5_SR &= ~(1 << 0);

    // 4. Start the timer (Enable Counter)
    TIM5_CR1 |= (1 << 0);

    // 5. Wait here until the hardware sets the UIF bit to 1
    while(!(TIM5_SR & (1 << 0))) {}

    // 6. Stop the timer
    TIM5_CR1 &= ~(1 << 0);
}

void init_pwm_led(void) {
    // Enable Clocks
    RCC_AHB1ENR |= (1 << 3);  // GPIOD clock
    RCC_APB1ENR |= (1 << 2);  // TIM4 clock

    // Configure PD12 for Alternate Function
    GPIOD_MODER &= ~(3 << 24);
    GPIOD_MODER |= (2 << 24);

    // Connect PD12 to AF2 (TIM4)
    GPIOD_AFRH &= ~(0xF << 16);
    GPIOD_AFRH |= (2 << 16);

    // Set PWM Frequency
    TIM4_PSC = 16 - 1;
    TIM4_ARR = 1000 - 1;

    // Configure PWM Mode 1 on Channel 1
    TIM4_CCMR1 |= (6 << 4);
    TIM4_CCMR1 |= (1 << 3);

    // Enable the output for Channel 1
    TIM4_CCER |= (1 << 0);

    // Start the Timer
    TIM4_CR1 |= (1 << 0);
}

int main(void) {
    init_pwm_led();
    init_delay_timer();  // Initialize our new hardware timer

    while(1) {
        // Fade UP
        for(int i = 0; i < 1000; i += 10) {
            TIM4_CCR1 = i;
            delay_ms(15);  // Mathematically precise 15ms hardware delay
        }

        // Fade DOWN
        for(int i = 1000; i > 0; i -= 10) {
            TIM4_CCR1 = i;
            delay_ms(15);
        }
    }
}
