/*
 * stm32-rng.h: STM32F RNG defines.
 *
 * Author: Adrian Huang <adrianhuang0701@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _STM32_RNG_H
#define _STM32_RNG_H

/*
 * STM32 RNG: Fields of Control Register
 */
#define STM32_RNG_CR_EN (1 << 2)	/* RNG Enable		*/
#define STM32_RNG_CR_IE (1 << 3)	/* RNG Interrupt Enable	*/

/*
 * STM32 RNG: Fields of Status Register
 */
#define STM32_RNG_SR_DRDY (1 << 0)	/* Data Ready			*/
#define STM32_RNG_SR_CECS (1 << 1)	/* Clock Error Current Status	*/
#define STM32_RNG_SR_SECS (1 << 2)	/* Seed Error Current Status	*/
#define STM32_RNG_SR_CEIS (1 << 5)	/* Clock Error Interrupt Status */
#define STM32_RNG_SR_SEIS (1 << 6)	/* Seed Error Interrupt Status	*/

#define STM32_RCC_AHB2ENR_RNG (1 << 6)		/* RNG clock enable	*/

/* The clock of PLL48CLK is less than 48 MHz. */
#define STM32_RNG_PLL48CLK	48

/*
 * It requires 40 periods of the PLL48CLK clock signal between two
 * consecutive random numbers accroding to the chapter 24
 * "Random Number Generator" of RM0090 Reference Manual.
 * The unit of this macro is nanosecond.
 */
#define STM32_RNG_DELAY_NS (40 * 1000 / STM32_RNG_PLL48CLK)

int stm32_rng_read_number(unsigned long, u32 *);

#endif
