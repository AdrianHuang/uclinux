/*
 * RNG driver for STM32F boards. The STM32F(s) are the series products from
 * STMicroelectronics.
 *
 * Hardware driver for the STM32Fxxx Random Number Generator (RNG)
 *
 * Author: Adrian Huang <adrianhuang0701@gmail.com>
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/hw_random.h>

#include <mach/stm32.h>
#include <mach/stm32-rng.h>

#define PFX	KBUILD_MODNAME ": "

struct stm32_rng_regs {
	u32	cr;	/* Control Register */
	u32	sr;	/* Status Register */
	u32	dr;	/* Data Register */
};

/*
 * RNG register map base
 */
#define STM32_RNG_BASE (STM32_AHB2PERITH_BASE + 0x60800)
#define STM32_RNG ((volatile struct stm32_rng_regs *) STM32_RNG_BASE)

int stm32_rng_read_number(unsigned long reg_addr, u32 *buf)
{
	volatile struct stm32_rng_regs *rng_regs = (void *) reg_addr;
	int data_len = 0;

	/* Delay a period of time to get the next random number. */
	ndelay(STM32_RNG_DELAY_NS);

	if (rng_regs->sr & STM32_RNG_SR_DRDY) {
		*buf = rng_regs->dr;
		data_len = sizeof(u32);
	}

	return data_len;
}

static int stm32_rng_data_read(struct hwrng *rng, u32 *buffer)
{
	return stm32_rng_read_number(rng->priv, buffer);
}

static struct hwrng stm32_rng_ops = {
	.name		= "stm32",
	.data_read	= stm32_rng_data_read,
};

static int __init stm32_rng_init(void)
{
	int err;

	/* Enable RNG clock. */
	STM32_RCC->ahb2enr |= STM32_RCC_AHB2ENR_RNG;

	/* Enable RNG. */
	STM32_RNG->cr |= STM32_RNG_CR_EN;

	stm32_rng_ops.priv = (unsigned long) STM32_RNG;

	err = hwrng_register(&stm32_rng_ops);

	if (err)
		printk(KERN_ERR PFX "RNG registering failed (%d)\n", err);

	printk(KERN_INFO "STM32 Hardware RNG initialized\n");

	return err;
}

static void __exit stm32_rng_exit(void)
{
	STM32_RCC->ahb2enr &= ~STM32_RCC_AHB2ENR_RNG;
}

module_init(stm32_rng_init);
module_exit(stm32_rng_exit);

MODULE_AUTHOR("Adrian Huang <adrianhuang0701@gmail.com>");
MODULE_DESCRIPTION("H/W Random Number Generator (RNG) driver for STM32F");
MODULE_LICENSE("GPL");
