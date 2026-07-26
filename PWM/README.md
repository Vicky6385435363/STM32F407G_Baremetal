# STM32F4 Bare-Metal PWM LED Brightness Control

A minimal, register-level (no HAL, no CMSIS abstraction) example that configures **TIM4 Channel 1** on an STM32F4 microcontroller to drive an LED on **PD12** with hardware PWM, smoothly fading its brightness up in software.

This project is intentionally written using **direct memory-mapped register access** (`#define` + pointer dereference) instead of ST's HAL/LL libraries, so you can see exactly what's happening at the hardware level.

---

## Hardware Assumptions

| Item | Value |
|---|---|
| MCU | STM32F4 series (e.g. STM32F407) |
| LED pin | **PD12** (on the Discovery board this is the green LED) |
| Timer | **TIM4**, Channel 1 |
| Timer clock | Assumed **16 MHz** (APB1 timer clock) |
| PWM frequency | **1 kHz** (1000 steps, 1 µs per tick) |

> Note: On many STM32F4 boards APB1 timer clock is actually 42/84 MHz depending on `RCC_CFGR` prescalers, not 16 MHz. The register values here (`PSC`/`ARR`) assume a 16 MHz timer clock - adjust `TIM4_PSC` if your actual clock differs, or the PWM frequency will not be 1 kHz.

---

---

## Line-by-Line Explanation

### Register Definitions

```c
#define RCC_AHB1ENR  *((volatile uint32_t*)0x40023830)
```
This defines a macro that lets you write `RCC_AHB1ENR` in code and have it act as if it were a variable, but it actually points to the fixed memory address `0x40023830` - the **RCC AHB1 peripheral clock enable register**. Peripherals like GPIOD are "gated off" (unpowered/unclocked) by default to save power, so you must enable their clock via this register before using them. `volatile` tells the compiler not to optimize away reads/writes, since the value can change due to hardware, not just code.

```c
#define RCC_APB1ENR  *((volatile uint32_t*)0x40023840)
```
Same idea, but this is the **APB1 peripheral clock enable register**, which controls the clock for peripherals like TIM4 (Timers 2-7 live on the APB1 bus).

```c
#define GPIOD_MODER  *((volatile uint32_t*)0x40020C00)
```
The **GPIOD mode register**. Each GPIO pin has 2 bits here that select its mode: `00` = input, `01` = general-purpose output, `10` = alternate function, `11` = analog.

```c
#define GPIOD_AFRH   *((volatile uint32_t*)0x40020C24)
```
The **GPIOD Alternate Function High register** (AFR[1] in ST's terminology). GPIO pins 8-15 use this register (pins 0-7 use `AFRL`/AFR[0] instead) to select *which* internal peripheral (Timer, USART, I2C, etc.) is connected to the pin when it's in Alternate Function mode. Each pin gets 4 bits (16 possible AF values, AF0–AF15).

```c
#define TIM4_CR1     *((volatile uint32_t*)0x40000800)
```
TIM4's **Control Register 1**. Controls basic timer behavior - most importantly bit 0 (`CEN`), which starts/stops the counter.

```c
#define TIM4_CCMR1   *((volatile uint32_t*)0x40000818)
```
**Capture/Compare Mode Register 1**. Configures channels 1 and 2 - specifically, whether each channel is used for input capture or output compare, and (in this case) which PWM mode to use.

```c
#define TIM4_CCER    *((volatile uint32_t*)0x40000820)
```
**Capture/Compare Enable Register**. Turns each channel's actual output pin drive on or off, and controls output polarity.

```c
#define TIM4_PSC     *((volatile uint32_t*)0x40000828)
```
**Prescaler register**. Divides the timer's input clock down before it reaches the counter. Formula: `timer_tick_freq = timer_clock / (PSC + 1)`.

```c
#define TIM4_ARR     *((volatile uint32_t*)0x4000082C)
```
**Auto-Reload Register**. Defines the counter's "top" value - once the counter reaches this, it resets to 0 (this sets the PWM period).

```c
#define TIM4_CCR1    *((volatile uint32_t*)0x40000834)
```
**Capture/Compare Register 1**. In PWM mode, this is the *compare value* - it sets the duty cycle: the output stays high (or low, depending on polarity) until the counter reaches this value within the period.

---

### `delay()` function

```c
void delay(volatile uint32_t count) {
    while(count--) {}
}
```
A crude busy-wait / "spin loop" delay. It counts down from `count` to 0, doing nothing each iteration. `count` is marked `volatile` so the compiler doesn't optimize the empty loop away entirely. This is **not** a precise, calibrated delay (it depends on CPU clock speed and compiler optimization level) - it's just enough to make brightness changes visible to the human eye.

---

### `init_pwm_led()` function

#### Step 1 — Enable Peripheral Clocks

```c
RCC_AHB1ENR |= (1 << 3);  // GPIOD clock
```
Sets bit 3 of `RCC_AHB1ENR`, which is the **GPIOD enable bit**, turning on the clock signal that powers GPIO port D's logic. Without this, writes to `GPIOD_MODER`/`GPIOD_AFRH` would have no effect.

```c
RCC_APB1ENR |= (1 << 2);  // TIM4 clock
```
Sets bit 2 of `RCC_APB1ENR`, the **TIM4 enable bit**, so the timer peripheral actually receives a clock and can count.

> Both lines use `|=` (read-modify-write with OR) rather than a plain assignment, so that enabling one peripheral's clock doesn't accidentally disable other peripherals whose enable bits live in the same register.

#### Step 2 — Configure PD12 as Alternate Function, routed to TIM4

```c
GPIOD_MODER &= ~(3 << 24);   // Clear bits 24, 25
GPIOD_MODER |= (2 << 24);    // Set to 10 (Alternate Function)
```
Each GPIO pin occupies 2 bits in `MODER`, at position `pin_number * 2`. For pin 12, that's bits 24–25. First the two bits are **cleared** (masked with `~(3<<24)`, since `3` = `0b11`), then set to binary `10` (`2 << 24`), which is the "Alternate Function" mode code. This tells the pin: "don't act as plain GPIO - hand control to an internal peripheral instead."

```c
GPIOD_AFRH &= ~(0xF << 16);
GPIOD_AFRH |= (2 << 16);     // Set AF2 (0010)
```
`AFRH` (AFR[1]) covers pins 8-15, with each pin getting a 4-bit field. Pin 12 is the **third** pin in this register (pins 8, 9, 10, 11, then 12), occupying bits 16-19 (`(12-8)*4 = 16`). The code clears those 4 bits, then writes `2` (binary `0010`), selecting **AF2**, which on the STM32F407's pin table is the alternate function mapped to **TIM4** on this pin. This is what actually connects the timer's internal PWM signal to the physical PD12 pin.

#### Step 3 - Set PWM Frequency

```c
TIM4_PSC = 16 - 1;           // Timer ticks at 1MHz (1us per tick)
```
Assuming a 16 MHz timer input clock, dividing by `(15+1) = 16` gives a **1 MHz** counting frequency - i.e., the counter increments once every 1 µs.

```c
TIM4_ARR = 1000 - 1;         // Period = 1000 ticks (1ms = 1kHz PWM frequency)
```
The counter counts from 0 up to 999 (1000 steps) before resetting, at 1 µs/tick that's a **1 ms period**, i.e. a **1 kHz PWM frequency**. This also defines the resolution: duty cycle can be set anywhere from 0 to 1000 (0.1% steps).

#### Step 4 - Configure PWM Mode 1 on Channel 1

```c
TIM4_CCMR1 |= (6 << 4);
```
Bits 6:4 of `CCMR1` are the `OC1M` (Output Compare 1 Mode) bits for channel 1. Writing `110` (`6`) selects **PWM Mode 1**: the output is high while the counter is less than `CCR1`, and low once the counter reaches/exceeds it (assuming default active-high polarity). This is the standard "duty-cycle" PWM behavior.

```c
TIM4_CCMR1 |= (1 << 3);
```
Sets the `OC1PE` (Output Compare 1 Preload Enable) bit. This buffers writes to `CCR1` so that a new duty-cycle value only takes effect at the *next* update event (end of period) rather than immediately mid-cycle - preventing glitches/tearing in the PWM waveform when you change brightness on the fly.

```c
TIM4_CCER |= (1 << 0);
```
Sets `CC1E` (Capture/Compare 1 Output Enable), the bit that actually connects the internal channel-1 waveform to the physical output pin. Without this, the timer would compute the PWM logic internally but never drive it out onto PD12.

#### Step 5 - Start the Timer

```c
TIM4_CR1 |= (1 << 0);        // Enable Counter (CEN)
```
Sets the `CEN` (Counter Enable) bit in `CR1`. This is the "go" switch - until now the timer was fully configured but frozen at 0. Once set, the counter begins incrementing every 1 µs, and PWM output begins on PD12.

---

### `main()` function

```c
int main(void) {
    init_pwm_led();
```
Runs all the one-time setup above: clocks, pin routing, timer/PWM configuration, and starts the counter.

```c
    while(1) {
        for(int i=0;i<1000;i+=10){
            TIM4_CCR1 = i;  // Variable brightness
            delay(150000);
        }
    }
}
```
An infinite loop that repeatedly fades the LED from off to fully on:
- `i` ramps from `0` to `990` in steps of `10` (100 steps total, since `ARR` = 999).
- Each iteration, `TIM4_CCR1 = i` updates the **duty cycle**: `i / 1000` is the fraction of each 1 ms period the pin stays high, so `i = 0` → LED off, `i = 999` → LED (nearly) fully on.
- Because `OC1PE` was enabled earlier, this new value is latched safely at the next period boundary - no visual glitching.
- `delay(150000)` pauses long enough (empirically) for the brightness change to be visible to the eye before moving to the next step.
- Once `i` reaches 1000, the `for` loop ends and the outer `while(1)` restarts it from `i = 0`, creating a repeating **fade-in sawtooth** effect. (Note: there's no fade-*out* - brightness snaps back to 0 abruptly at the end of each cycle rather than ramping down.)

The key insight: **the CPU never toggles the LED pin directly.** It only writes a number (`CCR1`) into the timer once every ~150,000 loop iterations. The actual high-frequency (1 kHz) on/off switching that produces the dimming effect is done entirely by the TIM4 hardware peripheral, running independently of the CPU.

---

## How PWM Dimming Works (Conceptually)

```
Period (1 ms @ 1kHz, ARR=999)
|<------------------------->|
 ________
|        |___________________|   CCR1 = 200  (20% duty -> dim)
 ____________________
|                    |________|   CCR1 = 600  (60% duty -> brighter)
 ________________________________
|                                |  CCR1 = 999 (~100% duty -> full brightness)
```

The LED (and human eye) can't perceive the 1 kHz flicker - it just perceives the **average power** delivered, which is proportional to the duty cycle. That's the whole trick behind PWM brightness/speed/analog-like control on a digital pin.

---


## Known Caveats / Things to Verify for Your Board

- **Timer clock assumption**: The 16 MHz assumption for APB1 timer clock is uncommon on real STM32F407 boards after full clock-tree configuration (which typically yields 42 or 84 MHz on APB1 timers). If you haven't configured `RCC_CFGR` clock dividers elsewhere, verify your actual `SYSCLK`/APB1 timer clock before trusting the 1 kHz figure.
- **AF2 on PD12 → TIM4**: Confirmed correct for STM32F4 devices per the alternate function mapping table - PD12 supports `TIM4_CH1` on AF2.
- **`delay()` accuracy**: Not calibrated to real time; it's compiler/optimization-level and CPU-clock dependent.

---

