	#include <stdint.h>

	#define RCC_AHB1ENR  *((volatile uint32_t*)0x40023830)
	#define RCC_APB1ENR  *((volatile uint32_t*)0x40023840)

	#define GPIOD_MODER  *((volatile uint32_t*)0x40020C00)
	// AFR[1] handles pins 8-15
	#define GPIOD_AFRH   *((volatile uint32_t*)0x40020C24)

	#define TIM4_CR1     *((volatile uint32_t*)0x40000800)
	#define TIM4_CCMR1   *((volatile uint32_t*)0x40000818)
	#define TIM4_CCER    *((volatile uint32_t*)0x40000820)
	#define TIM4_PSC     *((volatile uint32_t*)0x40000828)
	#define TIM4_ARR     *((volatile uint32_t*)0x4000082C)
	#define TIM4_CCR1    *((volatile uint32_t*)0x40000834)
	void delay(volatile uint32_t count) {
		while(count--) {}
	}
	void init_pwm_led(void) {
		// 1. Enable Clocks
		RCC_AHB1ENR |= (1 << 3);  // GPIOD clock
		RCC_APB1ENR |= (1 << 2);  // TIM4 clock

		// 2. Configure PD12 for Alternate Function
		GPIOD_MODER &= ~(3 << 24);   // Clear bits 24, 25
		GPIOD_MODER |= (2 << 24);    // Set to 10 (Alternate Function)

		// Connect PD12 to AF2 (TIM4)
		// PD12 is controlled by bits 16-19 in AFRH (AFR[1])
		GPIOD_AFRH &= ~(0xF << 16);
		GPIOD_AFRH |= (2 << 16);     // Set AF2 (0010)

		// 3. Set PWM Frequency (Assuming 16MHz APB1 clock)
		TIM4_PSC = 16 - 1;           // Timer ticks at 1MHz (1us per tick)
		TIM4_ARR = 1000 - 1;         // Period = 1000 ticks (1ms = 1kHz PWM frequency)

		// 4. Configure PWM Mode 1 on Channel 1
		// Set OC1M bits to 110 (PWM Mode 1) in CCMR1
		TIM4_CCMR1 |= (6 << 4);
		// Enable Preload register on CCR1
		TIM4_CCMR1 |= (1 << 3);

		// Enable the output for Channel 1
		TIM4_CCER |= (1 << 0);

		// 5. Start the Timer
		TIM4_CR1 |= (1 << 0);        // Enable Counter (CEN)
	}

	int main(void) {
		init_pwm_led();

		while(1) {
			// You now control the LED brightness entirely in hardware
			// 0 = completely off, 1000 = completely on
			for(int i=0;i<1000;i+=10){
			TIM4_CCR1 = i;  // Variable brightness
			delay(150000);
		}
		}
	}
