/*
 * (C) Copyright (C) 2009
 * ARM Ltd.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H

#if defined(CONFIG_ARCH_STM32F1)
#define NR_IRQS		68	/* STM32F1 */

#elif defined(CONFIG_ARCH_STM32F2)
#define NR_IRQS		81	/* STM32F2 */

#elif defined(CONFIG_ARCH_STM32F3)  || \
      defined(CONFIG_ARCH_STM32F40) || \
      defined(CONFIG_ARCH_STM32F41)
#define NR_IRQS		82	/* STM32F3x, STM32F40xxx, and STM32F41xxx */

#else
#define NR_IRQS		91	/* STM32F42xxx, STM32F43xxx and others */
#endif

/*
 * Re-define NR_IRQS if the stmpe811 driver is compiled for
 * stm32f42/stm32f43 MCUs.
 */
#if defined(CONFIG_MFD_STMPE) && \
    (defined(CONFIG_ARCH_STM32F42) || defined(CONFIG_ARCH_STM32F43))
#undef NR_IRQS
#define NR_IRQS		99	/* STM32F42 + 8 internal interrupts (stmpe811)*/
#endif

#endif
