# STM32F407 Bare-Metal Architecture

A collection of bare-metal C programs for the STM32F407VG Discovery Board. This repository bypasses the STM32 Hardware Abstraction Layer (HAL) and Standard Peripheral Libraries to interact directly with the microcontroller's memory-mapped registers. 

This project demonstrates foundational embedded systems architecture, hardware-level timing, and direct peripheral manipulation.

## Features

*   **Direct Register Access:** All peripherals are configured using bitwise operations on raw memory addresses based on the Cortex-M4 Programming Manual and STM32F4 Reference Manual.
*   **Hardware PWM Generation (TIM4):** Configured Alternate Function (AF2) multiplexing to route Timer 4 directly to the onboard LEDs for variable duty-cycle brightness control without CPU intervention.
*   **Precise Hardware Delays (TIM5):** Utilizes the 32-bit TIM5 peripheral to create deterministic, mathematically precise blocking delays, escaping the compiler-dependent inaccuracy of `for()` loop delays.
*   **Analog-to-Digital Conversion (ADC1):** Configured 12-bit ADC in single-conversion polling mode to read analog voltages (e.g., from an MQ-2 sensor) via pin `PA1`.
*   **ITM Debugging:** Implemented `_write` syscall overrides to redirect standard C `printf()` output to the Serial Wire Viewer (SWV) ITM Data Console for real-time serial monitoring over the ST-LINK debug interface.

## Hardware Requirements

*   **Board:** STM32F407G-DISC1 (Discovery Board)
*   **Core:** ARM Cortex-M4 with FPU
*   **External Components:** Jumper wires, optional analog sensors (e.g., MQ-2 Gas Sensor, Potentiometer) connected to `PA1`.

## Development Environment Setup

This code is built using **STM32CubeIDE**. To run the project successfully, ensure the following configurations are set:

1.  **Floating-Point Unit (FPU):**
    Ensure the FPU is initialized if performing float calculations (like converting raw ADC integer steps to a physical voltage).
2.  **Serial Wire Viewer (SWV) for `printf`:**
    *   In the Debug Configuration, go to the **Debugger** tab.
    *   Enable **Serial Wire Viewer (SWV)** and ensure the Core Clock matches your system clock (default is usually `16.0 MHz`).
    *   Open `Window > Show View > SWV > SWV ITM Data Console`.
    *   Configure trace to enable **Port 0**.

## Why Bare-Metal?

While frameworks like FreeRTOS and the STM32 HAL accelerate development time, true system optimization requires understanding the underlying silicon architecture. This repository serves as a technical showcase of:
*   Understanding bus clock gating (AHB1/APB1/APB2).
*   Bitwise logic masking and register manipulation.
*   Translating hardware datasheets (RM0090) into executable C code.
*   Managing floating/high-impedance analog states and understanding signal quantization.

## Author

**Vignesh K.** 
*M.E. Embedded System Technologies | College of Engineering, Guindy*

Focusing on RTOS, embedded networking (CAN/LIN), and hardware-level microcontroller programming.

---
