/*
 * fb_moving_tux.h: moving tux defines.
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
#ifndef _FB_MOVING_TUX_H
#define _FB_MOVING_TUX_H

#define RNG_GET_NUMBER_RETRIES	1000

#define STM32_GPIO_PORT_PIN(port, pin)	(port * STM32_GPIO_PORT_PINS + pin)

/* PA0: User Button */
#define STM32_USER_BUTTON STM32_GPIO_PORT_PIN(0, 0)

enum {
	REACH_RANGE = 1,
	OUT_OF_RANGE
};

extern int fg_console;
extern int fb_get_logo_needs_truepalette(void);
extern void fbcon_clear_all(struct vc_data *);
extern void *fb_get_do_show_logo_fn(void);
extern void *fb_get_set_logo_truepalette_fn(void);
extern struct linux_logo *fb_get_logo(void);
extern struct class *fb_class;
extern signed char fbcon_get_con2fb_map(int);

#endif

