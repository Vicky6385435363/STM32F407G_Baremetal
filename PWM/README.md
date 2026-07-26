# STM32 Bare-Metal PWM & Hardware Delay 

> **Overview:** A complete bare-metal C implementation for the STM32F407 that generates a smooth, mathematically precise "breathing" LED effect. It avoids standard libraries, writing directly to the hardware registers to use Timer 4 for Pulse Width Modulation (PWM) on Pin D12, and Timer 5 for a non-blocking, accurate hardware delay.

---

## Hardware Requirements

*   **Microcontroller:** STM32F407 (or compatible STM32F4 series)
*   **Output:** Port D, Pin 12 (connected to an onboard or external LED)
*   **Clock Speed:** Defaults to 16MHz

---

## Register Map & Peripherals

The project interacts directly with the following hardware registers to control the CPU, GPIO, and Timers without overhead.

| Peripheral | Register | Description |
| :--- | :--- | :--- |
| **RCC** | `RCC_AHB1ENR` | Enables the clock for GPIO Port D |
| **RCC** | `RCC_APB1ENR` | Enables the clock for Timer 4 and Timer 5 |
| **GPIO** | `GPIOD_MODER` | Sets Pin 12 to Alternate Function mode |
| **GPIO** | `GPIOD_AFRH` | Connects Pin 12 to the internal TIM4 peripheral |
| **TIM4 (PWM)** | `TIM4_CR1` | The main "ON" switch for Timer 4 |
| **TIM4 (PWM)** | `TIM4_ARR` | Auto-Reload Register: Sets the PWM frequency |
| **TIM4 (PWM)** | `TIM4_CCR1` | Capture/Compare Register: Controls duty cycle (brightness) |
| **TIM5 (Delay)** | `TIM5_SR` | Status Register: Flags when the delay finishes |

---

## Code Walkthrough

### 1. Initialization & Memory Management
Before any peripheral works, it needs power. Standard integer types are used to perfectly match the 32-bit hardware architecture.
*   **`#include <stdint.h>`:** Guarantees variables are exactly 32 bits wide.
*   **`volatile` keyword:** Prevents the compiler from optimizing away our direct memory writes to the hardware.

### 2. Precise Hardware Delay (`TIM5`)
Instead of using an unpredictable software `for` loop, this code configures Timer 5 (a 32-bit hardware timer) to create an exact delay.
*   **`TIM5_PSC = 16000 - 1`:** Slows the default 16MHz clock down to exactly 1,000 ticks per second (1 tick = 1 millisecond).
*   **`TIM5_ARR = ms`:** We write our target delay here. If we pass 15, the timer counts to 15.
*   **`TIM5_SR &= ~(1 << 0)`:** Clears the Update Interrupt Flag (UIF) from previous runs.
*   **`while(!(TIM5_SR & (1 << 0))) {}`:** The CPU blocks here. It watches the Status Register until the background hardware finishes counting and physically flips the bit to 1.

### 3. PWM Generation (`TIM4`)
Timer 4 is configured to generate a 1kHz PWM signal directly out of Pin D12.
*   **`GPIOD_MODER |= (2 << 24)`:** Configures Pin 12 as an Alternate Function rather than standard Input/Output.
*   **`GPIOD_AFRH |= (2 << 16)`:** Specifically routes the TIM4 internal signal to Pin D12 (Alternate Function 2).
*   **`TIM4_PSC = 16 - 1`:** Divides the 16MHz clock to 1MHz (1 microsecond per tick).
*   **`TIM4_ARR = 1000 - 1`:** Sets the top count. At 1MHz, counting to 1000 takes exactly 1 millisecond, establishing a 1kHz PWM frequency.
*   **`TIM4_CCMR1 |= (6 << 4)`:** Activates PWM Mode 1 on Channel 1.
*   **`TIM4_CCMR1 |= (1 << 3)`:** Enables the "Preload" register, ensuring brightness changes happen smoothly on the next cycle without glitching.
*   **`TIM4_CCER |= (1 << 0)`:** Opens the final gate, allowing the PWM wave to exit the timer and reach the physical pin.

---

## The Breathing Effect

The `main()` loop coordinates the PWM and delay functions to create the visual effect. Embedded systems must never exit, so everything runs inside an infinite `while(1)` loop.

*   **The Fade UP Loop:** `for(int i = 0; i < 1000; i += 10)`
    Writes the loop value (`i`) into `TIM4_CCR1`. As `i` increases, the duty cycle widens, making the LED brighter. The CPU waits `15ms` between each step.
*   **The Fade DOWN Loop:** `for(int i = 1000; i > 0; i -= 10)`
    Immediately after reaching max brightness, this loop reverses the process, smoothly dimming the LED back to zero.
