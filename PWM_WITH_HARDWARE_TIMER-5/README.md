# Concept & Code Breakdown

> **The Core Concept:** "Bare-metal" programming means writing software that interacts directly with the microcontroller's silicon hardware, bypassing heavy abstraction layers like the STM32 Hardware Abstraction Layer (HAL). In this project, we manually manipulate memory-mapped registers to configure the CPU clock, configure a GPIO pin, and drive two separate hardware timers to create a mathematically precise fading LED effect.

---

## 1. Includes and Register Definitions

To talk to the hardware, we define pointers to specific memory addresses where the STM32's control registers live. 

*   **`#include <stdint.h>`** 
    Imports standard integer types (like `uint32_t`). This guarantees our variables are exactly 32 bits wide, which is mandatory for 32-bit hardware registers.
*   **`#define RCC_AHB1ENR *((volatile uint32_t*)0x40023830)`** 
    Points to the Reset and Clock Control (RCC) register for the AHB1 bus, which powers the GPIO ports. The `volatile` keyword prevents the compiler from optimizing away our writes.
*   **`#define RCC_APB1ENR *((volatile uint32_t*)0x40023840)`** 
    Points to the RCC register for the APB1 bus, which powers peripheral devices like Timer 4 and Timer 5.

*(The code continues to define pointers for `GPIOD`, `TIM4`, and `TIM5` registers following this exact same logic, pointing to their respective memory addresses as defined in the STM32F407 datasheet.)*

---

## 2. Hardware Delay Functions (`TIM5`)

Instead of using a generic `for` loop that wastes CPU cycles unpredictably, we use Timer 5 as a dedicated hardware stopwatch.

### Initialization: `init_delay_timer()`
*   **`RCC_APB1ENR |= (1 << 3);`** 
    Turns on the clock for Timer 5. Without this, the timer is dead silicon.
*   **`TIM5_PSC = 16000 - 1;`** 
    Sets the Prescaler. The default APB1 clock is 16MHz. Dividing by 16,000 means the timer counts exactly 1,000 times a second. Therefore, 1 tick = 1 millisecond.
*   **`TIM5_CR1 &= ~(1 << 0);`** 
    Ensures the timer is turned OFF until we actually need to use it.

### Execution: `delay_ms(uint32_t ms)`
*   **`TIM5_ARR = ms;`** 
    Writes your target delay into the Auto-Reload Register. If you pass 15, the timer will count to 15.
*   **`TIM5_CNT = 0;`** 
    Forces the live counter back down to zero to ensure we start counting from the beginning.
*   **`TIM5_SR &= ~(1 << 0);`** 
    Clears the Update Interrupt Flag (UIF). We must force it to 0 so we know when the hardware turns it back to 1 later.
*   **`TIM5_CR1 |= (1 << 0);`** 
    Turns the timer ON. The silicon hardware now starts counting in the background, independently of the CPU.
*   **`while(!(TIM5_SR & (1 << 0))) {}`** 
    **The Blocking Loop:** The CPU waits here, constantly checking bit 0 of the Status Register. When the hardware hits your target value, it physically flips this bit to 1, and the CPU escapes the loop.
*   **`TIM5_CR1 &= ~(1 << 0);`** 
    Turns the timer back OFF.

---

## 3. PWM Initialization (`init_pwm_led`)

This configures Timer 4 to generate a Pulse Width Modulation (PWM) signal on Pin D12.

*   **`RCC_AHB1ENR |= (1 << 3);`** & **`RCC_APB1ENR |= (1 << 2);`**
    Turns on the clocks for GPIO Port D and Timer 4.
*   **`GPIOD_MODER &= ~(3 << 24);`** & **`GPIOD_MODER |= (2 << 24);`**
    Clears the bits for Pin 12, then writes `10` (binary for 2) to tell the hardware Pin 12 is in Alternate Function mode.
*   **`GPIOD_AFRH &= ~(0xF << 16);`** & **`GPIOD_AFRH |= (2 << 16);`**
    Writes `0010` into the Pin 12 slot of the Alternate Function register. This physically routes TIM4 to this pin.
*   **`TIM4_PSC = 16 - 1;`** 
    Sets the Prescaler to divide the 16MHz clock by 16, so the timer counts at 1MHz (1 microsecond per tick).
*   **`TIM4_ARR = 1000 - 1;`** 
    Sets the total period to 1000 ticks. At a 1MHz speed, 1000 ticks take 1 millisecond, establishing a 1kHz PWM frequency.
*   **`TIM4_CCMR1 |= (6 << 4);`** 
    Writes `110` (binary 6) to activate PWM Mode 1.
*   **`TIM4_CCMR1 |= (1 << 3);`** 
    Enables the "Preload" register. This ensures that brightness changes happen smoothly at the start of the next PWM cycle, preventing visual glitches.
*   **`TIM4_CCER |= (1 << 0);`** 
    Enables the output for Channel 1, opening the gate for the PWM signal to exit the timer.
*   **`TIM4_CR1 |= (1 << 0);`** 
    Sets the Counter Enable (CEN) bit. The timer is now generating PWM in the background.

---

## 4. The Main Loop (`main`)

Embedded systems must never exit `main`, so the logic lives in an infinite loop.

*   **`init_pwm_led();`** & **`init_delay_timer();`**
    Calls our hardware initialization functions.
*   **`while(1) { ... }`**
    The infinite application loop.
*   **`for(int i = 0; i < 1000; i += 10)`**
    **The Fade UP Loop:** Counts from 0 to 990.
*   **`TIM4_CCR1 = i;`** 
    Writes the loop value (`i`) into the PWM duty cycle register. As `i` increases, the LED gets brighter.
*   **`delay_ms(15);`** 
    Calls our hardware delay. The CPU waits exactly 15 milliseconds before moving to the next brightness level.
*   **`for(int i = 1000; i > 0; i -= 10)`**
    **The Fade DOWN Loop:** Immediately after reaching max brightness, this loop counts backward, smoothly dimming the LED back to zero.
