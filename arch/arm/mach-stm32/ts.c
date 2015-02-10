/*
 * The coordinate calibration of the touchscreen controller (stmpe811) for
 * stm32f4xx.
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

#define STM32F4_SCREEN_X_MAX	240
#define STM32F4_SCREEN_Y_MAX	320

static void stm32f4_ts_check_boundary(int *var, int boundary)
{
	if(*var < 0)
		*var = 0;
	else if (*var > boundary)
		*var = boundary;
	else
		;
}

static int stm32f4_ts_calibrate_x(int x)
{
	int tmp_x;

	x = (x <= 3000) ? (3870 - x) : (3800 - x);

	/* x value second correction */
	tmp_x = x / 15;

	stm32f4_ts_check_boundary(&tmp_x, STM32F4_SCREEN_X_MAX - 1);

	return tmp_x;
}

static int stm32f4_ts_calibrate_y(int y)
{
	int tmp_y;

	/* y value first correction */
	y -= 360;

	/* y value second correction */
	tmp_y = y / 11;

	stm32f4_ts_check_boundary(&tmp_y, STM32F4_SCREEN_Y_MAX - 1);

	return tmp_y;
}

void stm32f4_ts_calibration(int *x, int *y)
{
	static int cur_x, cur_y;
	int new_x, new_y, diff_x, diff_y;

	new_x = stm32f4_ts_calibrate_x(*x);
	new_y = stm32f4_ts_calibrate_y(*y);

	diff_x = abs(new_x - cur_x);
	diff_y = abs(new_y - cur_y);

	if (diff_x + diff_y > 5) {
		cur_x = new_x;
		cur_y = new_y;
	}

	*x = cur_x;
	*y = cur_y;
}
