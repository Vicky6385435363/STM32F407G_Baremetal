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
