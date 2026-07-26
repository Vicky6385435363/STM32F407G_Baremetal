STM32 Bare-Metal PWM & Hardware Delay: Line-by-Line Breakdown

This document breaks down your C code line-by-line. The code configures the STM32F407 hardware to generate a Pulse Width Modulation (PWM) signal on Pin D12 using Timer 4, and uses Timer 5 (a 32-bit hardware timer) to create a mathematically precise delay, creating a smooth fading "breathing" effect.

Includes and RCC Registers

#include <stdint.h>

Imports standard integer types like uint32_t, guaranteeing variables are exactly 32 bits wide, which is mandatory for 32-bit hardware registers.

#define RCC_AHB1ENR  *((volatile uint32_t*)0x40023830)

Defines a macro pointing to the Reset and Clock Control (RCC) register for the AHB1 bus. This bus powers the GPIO ports. volatile prevents the compiler from optimizing away the writes.

#define RCC_APB1ENR  *((volatile uint32_t*)0x40023840)

Points to the RCC register for the APB1 bus, which powers peripheral devices like Timer 4 and Timer 5.

GPIO Registers

#define GPIOD_MODER  *((volatile uint32_t*)0x40020C00)

Points to the Mode Register for GPIO Port D. This dictates whether pins are Inputs, Outputs, Analog, or Alternate Functions.

#define GPIOD_AFRH   *((volatile uint32_t*)0x40020C24)

Points to the Alternate Function High register for Port D. This connects pins 8-15 to internal peripherals instead of basic GPIO logic.

TIM4 Registers (PWM Generation)

#define TIM4_CR1     *((volatile uint32_t*)0x40000800)

Points to Timer 4 Control Register 1. Contains the main "ON" switch for the timer.

#define TIM4_CCMR1   *((volatile uint32_t*)0x40000818)

Points to Timer 4 Capture/Compare Mode Register 1. Configures Channel 1 behavior (e.g., setting it to PWM output mode).

#define TIM4_CCER    *((volatile uint32_t*)0x40000820)

Points to Timer 4 Capture/Compare Enable Register. Physically connects the timer's internal signal to the outside pin.

#define TIM4_PSC     *((volatile uint32_t*)0x40000828)

Points to Timer 4 Prescaler. Slows down the incoming clock frequency before it reaches the timer.

#define TIM4_ARR     *((volatile uint32_t*)0x4000082C)

Points to Timer 4 Auto-Reload Register. Sets the "top" value the timer counts to, defining the frequency (period) of the PWM wave.

#define TIM4_CCR1    *((volatile uint32_t*)0x40000834)

Points to Timer 4 Capture/Compare Register 1. Controls the duty cycle (brightness).

TIM5 Registers (Hardware Delay)

#define TIM5_CR1     *((volatile uint32_t*)0x40000C00)

Points to Timer 5 Control Register 1. Contains the "ON" switch for Timer 5.

#define TIM5_SR      *((volatile uint32_t*)0x40000C10)

Points to Timer 5 Status Register. The hardware sets a flag here when the timer finishes counting.

#define TIM5_CNT     *((volatile uint32_t*)0x40000C24)

Points to Timer 5 Counter Register. This holds the actual live number as the timer counts up.

#define TIM5_PSC     *((volatile uint32_t*)0x40000C28)

Points to Timer 5 Prescaler. Slows the clock down to 1 millisecond ticks.

#define TIM5_ARR     *((volatile uint32_t*)0x40000C2C)

Points to Timer 5 Auto-Reload Register. The target number of milliseconds we want to wait.

Hardware Delay Functions

void init_delay_timer(void) {

Begins the initialization for TIM5.

RCC_APB1ENR |= (1 << 3);

Turns on the clock for Timer 5 by setting bit 3 to 1. Without this, the timer is dead silicon.

TIM5_PSC = 16000 - 1;

Sets the Prescaler. The default APB1 clock is 16MHz (16,000,000 ticks per second). Dividing by 16,000 means the timer counts exactly 1,000 times a second. Therefore, 1 tick = 1 millisecond. We subtract 1 because hardware counts starting at 0.

TIM5_CR1 &= ~(1 << 0);

Ensures the timer is turned OFF (clearing bit 0, the CEN bit) until we actually need to use it.

}

Ends init_delay_timer.

void delay_ms(uint32_t ms) {

A function that takes the number of milliseconds you want to wait as an argument.

TIM5_ARR = ms;

Writes your target delay into the Auto-Reload Register. If you pass 15, the timer will count to 15.

TIM5_CNT = 0;

Forces the live counter back down to zero to ensure we start counting from the beginning.

TIM5_SR &= ~(1 << 0);

Clears the Update Interrupt Flag (UIF). If the timer was used previously, this flag might still be at 1. We must force it to 0 so we know when it turns back to 1 later.

TIM5_CR1 |= (1 << 0);

Turns the timer ON. The silicon hardware now starts counting in the background, independently of the CPU.

while(!(TIM5_SR & (1 << 0))) {}

This is the blocking mechanism. The CPU enters this empty loop. It constantly checks bit 0 of the Status Register. When the background hardware hits your ARR value (e.g., 15), the hardware physically flips this bit to 1. The while condition becomes false, and the CPU escapes the loop.

TIM5_CR1 &= ~(1 << 0);

Turns the timer back OFF now that the delay is finished.

}

Ends delay_ms.

PWM Initialization Function

void init_pwm_led(void) {

Begins the initialization function for the hardware.

RCC_AHB1ENR |= (1 << 3);

Turns on the clock for GPIO Port D by setting bit 3 to 1.

RCC_APB1ENR |= (1 << 2);

Turns on the clock for Timer 4 by setting bit 2 to 1.

GPIOD_MODER &= ~(3 << 24);

Clears bits 24 and 25 (the bits controlling Pin 12) by ANDing them with a mask (~ inverts 0b11 to 0b00, clearing those specific spots).

GPIOD_MODER |= (2 << 24);

Writes 10 (binary for 2) into bits 24 and 25. 10 tells the hardware Pin 12 is in Alternate Function mode.

GPIOD_AFRH &= ~(0xF << 16);

Clears bits 16, 17, 18, and 19 (the 4-bit slot for Pin 12 in the Alternate Function High register).

GPIOD_AFRH |= (2 << 16);

Writes 0010 (binary for 2) into the Pin 12 slot. AF2 corresponds to routing TIM4 to this pin.

TIM4_PSC = 16 - 1;

Sets the Prescaler. Divides the 16MHz clock by 16, so the timer counts at 1MHz (1 microsecond per tick).

TIM4_ARR = 1000 - 1;

Sets the total period. The timer counts to 999 (1000 total ticks). At a 1MHz speed, 1000 ticks take 1 millisecond. Therefore, the PWM frequency is 1kHz.

TIM4_CCMR1 |= (6 << 4);

Configures Channel 1. Bits 4, 5, and 6 control the mode. We write 110 (binary for 6) into this location, which activates PWM Mode 1.

TIM4_CCMR1 |= (1 << 3);

Enables the "Preload" register for Channel 1. This ensures that if we change the brightness (CCR1), the change happens smoothly at the start of the next PWM cycle, preventing glitches.

TIM4_CCER |= (1 << 0);

Enables the output for Channel 1. This opens the gate allowing the PWM signal to exit the timer and reach the physical pin.

TIM4_CR1 |= (1 << 0);

Sets the Counter Enable (CEN) bit. The timer is now running and generating PWM in the background.

}

Ends init_pwm_led.

The Main Loop

int main(void) {

The entry point of the program.

init_pwm_led();

Calls our function to configure the PWM hardware.

init_delay_timer();

Calls our function to configure the TIM5 hardware for accurate millisecond delays.

while(1) {

An infinite loop. Embedded systems must never exit main.

for(int i = 0; i < 1000; i += 10) {

The "Fade UP" loop. Counts from 0 to 990 in steps of 10. i represents our desired brightness.

TIM4_CCR1 = i;

Writes the current loop value (i) into the PWM duty cycle register. As i increases, the LED gets physically brighter.

delay_ms(15);

Calls our new hardware-driven delay function. The CPU waits here for exactly 15 milliseconds before moving to the next brightness level.

}

Ends the "Fade UP" loop.

for(int i = 1000; i > 0; i -= 10) {

The "Fade DOWN" loop. Immediately after reaching maximum brightness, this loop counts backward from 1000 down to 10.

TIM4_CCR1 = i;

Writes the decreasing value to the hardware, dimming the LED.

delay_ms(15);

Waits exactly 15 milliseconds between each dimming step.

}

Ends the "Fade DOWN" loop. When finished, the while(1) loop restarts, seamlessly flowing back into the "Fade UP" loop.

}

Ends the while(1) loop.

}

Ends main.
