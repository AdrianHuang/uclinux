/*
 * A kernel module is used to move tux (Linux mascot) on LCD of STM32F
 * series products.
 *
 * Author: Adrian Huang <adrianhuang0701@gmail.com>
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/errno.h>
#include <linux/tty.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/vt_kern.h>
#include <linux/selection.h>
#include <linux/kbd_kern.h>
#include <linux/console.h>
#include <linux/device.h>
#include <linux/smp_lock.h>
#include <linux/fb.h>
#include <linux/linux_logo.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>

#include <asm/byteorder.h>
#include <asm/unaligned.h>

#include <mach/stm32.h>
#include <mach/stm32-rng.h>
#include <mach/gpio.h>

#include "fb_moving_tux.h"

#define PFX	KBUILD_MODNAME ": "

#define STM32_RNG_BASE (STM32_AHB2PERITH_BASE + 0x60800)

static int move_tux;
static DECLARE_MUTEX(tux_mutex);
static struct device *moving_tux_device;
static struct task_struct *moving_tux_thread;

static void (*fb_show_logo_fn)(struct fb_info *, struct fb_image *,
			    int, unsigned int);
static void  (*fb_set_logo_truepalette_fn)(struct fb_info *,
					const struct linux_logo *, u32 *);

struct ts_info {
	int x;
	int y;
	int touch_detected;
};

static struct ts_info ts;

static int is_prime_number(unsigned int var)
{
	int n = var;

	if (var <= 1)
		return 0;

	if (var == 2 || var == 3)
		return 1;

	while (--n >= 2)
		if (!(var % n))
			return 0;
	return 1;
}


static int find_next_not_prime_number(int var)
{
	while (1)
		if (!is_prime_number(++var))
			return var;
}

static void setup_fb_image(struct fb_image *image, struct linux_logo *logo,
				int x, int y)
{

	image->depth = 8;
	image->data = logo->data;

	image->dx = x;
	image->dy = y;
	image->width = logo->width;
	image->height = logo->height;
}

static struct fb_info *fb_get_info(void)
{
	int idx;

	idx = fbcon_get_con2fb_map(fg_console);

	return idx == -1 ? NULL : registered_fb[idx];
}

static int check_range(struct fb_info *info, struct linux_logo *logo,
				int x, int y, int range)
{
	int ret = 0;

	switch (range) {
	case REACH_RANGE:
		if (x == 0 || x == (info->var.xres - logo->width) || y == 0 ||
				y == (info->var.yres - logo->height))
			ret = 1;
		break;
	case OUT_OF_RANGE:
		if (x < 0 || x > (info->var.xres - logo->width) || y < 0 ||
				y > (info->var.yres - logo->height))
			ret = 1;
		break;
	default:
		break;

	}

	return ret;
}

static void moving_tux(int x, int y)
{
	struct fb_info *info;

	struct fb_image image;
	struct linux_logo *logo;

	u32 *palette = NULL, *saved_pseudo_palette = NULL;

	/* Clear virtual console */
	fbcon_clear_all(vc_cons[fg_console].d);

	info = fb_get_info();

	if (!info) {
		printk(KERN_ERR PFX "info pointer is NULL\n");
		return;
	}

	logo = fb_get_logo();

	if (!logo) {
		printk(KERN_ERR PFX "logo pointer is NULL\n");
		return;
	}

	if (check_range(info, logo, x, y, OUT_OF_RANGE)) {
		printk(KERN_ERR PFX "x: %d y: %d is out of range\n", x, y);
		return;
	}

	setup_fb_image(&image, logo, x, y);

	if (fb_get_logo_needs_truepalette()) {
		palette = kmalloc(256 * 4, GFP_KERNEL);

		if (palette == NULL) {
			printk(KERN_ERR PFX "not enough memory\n");
			return;
		}

		if (fb_set_logo_truepalette_fn)
			fb_set_logo_truepalette_fn(info, logo, palette);

		saved_pseudo_palette = info->pseudo_palette;
		info->pseudo_palette = palette;
	}

	if (fb_show_logo_fn)
		fb_show_logo_fn(info, &image, 0, 1);

	kfree(palette);
	if (saved_pseudo_palette != NULL)
		info->pseudo_palette = saved_pseudo_palette;

	return;
}

/*
 * get_random_number - get a random number between 1-20.
 *
 * Get a random number between 1-20, The random number must be
 * the modulo n of the argument 'val', where 'n' is the random number.
 */
static int get_random_number(int val)
{
	u32 random_num;
	int tmp, data_len;
	int retries = 0;

	while (++retries <= RNG_GET_NUMBER_RETRIES) {
		retries++;

		data_len = stm32_rng_read_number(STM32_RNG_BASE, &random_num);

		if (!data_len)
			continue;

		/* Ignore 0. The range is 1-20. */
		tmp = (random_num % 20) + 1;

		if (!(val % tmp))
			break;
	}

	/* Just return 1 since we cannot get a random number correctly. */
	if (retries > RNG_GET_NUMBER_RETRIES)
		tmp = 1;

	return tmp;
}

static int update_steps(int *xy, int boundary)
{
	int random_num;
	int tmp = *xy;

	/*
	 * Adjust the value if it is a prime number because the x or y
	 * will be out of range ultimately. We need a number that is not
	 * a prime number.
	 */
	if (is_prime_number(tmp))  {
		tmp = find_next_not_prime_number(*xy);
		*xy = tmp;
	}

	if (tmp == boundary)
		return -get_random_number(boundary);

	if (tmp == 0)
		return get_random_number(boundary);

	/* Either the positive or the negative can be chosen. */
	stm32_rng_read_number(STM32_RNG_BASE, &random_num);
	return random_num % 2 ? get_random_number(boundary - tmp) :
				-get_random_number(tmp);

}

static void tux_change_move_tux_flag(void)
{
	down(&tux_mutex);
	move_tux = move_tux ? 0 : 1;
	up(&tux_mutex);
}

static void tux_get_display_xy(int ts_xy, int hw, int *xy, int boundary)
{
	int diff = ts_xy - hw;

	if (diff < 0) {
		*xy = 0;
	} else if (ts_xy > boundary)  {
		*xy = boundary;
	} else {
		*xy = diff;
	}

}

static int tux_thread(void *data)
{
	struct fb_info *info = fb_get_info();
	struct linux_logo *logo = fb_get_logo();
	int x_steps = 0, y_steps = 0;
	int *x, *y, x_boundary, y_boundary;

	if (!info || !logo)
		return -1;

	set_user_nice(current, -20);

	x = &info->var.xoffset;
	y = &info->var.yoffset;

	x_boundary = info->var.xres - logo->width;
	y_boundary = info->var.yres - logo->height;

	while (!kthread_should_stop()) {

		if (ts.touch_detected) {
			tux_get_display_xy(ts.x, logo->width / 2, x,
					x_boundary);
			tux_get_display_xy(ts.y, logo->height / 2, y,
					y_boundary);

			/*
			 * Display new position of Tux if the move_tux
			 * flag is not set.
			 */
			if (!move_tux)
				moving_tux(*x, *y);
		}

		if (gpio_get_value(STM32_USER_BUTTON)) {
			/* Wait for the button released by user. */
			while (gpio_get_value(STM32_USER_BUTTON))
				;

			tux_change_move_tux_flag();
		}

		if (!move_tux)
			goto reschedule_thread;

		if (ts.touch_detected || (!x_steps && !y_steps) ||
			check_range(info, logo, *x, *y,	REACH_RANGE)) {
			x_steps = update_steps(x, x_boundary);
			y_steps = update_steps(y, y_boundary);
		}

		moving_tux(*x, *y);
		*x += x_steps;
		*y += y_steps;

reschedule_thread:
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(msecs_to_jiffies(100)); /* wait 100ms */
	}

	return 0;
}

static ssize_t store_tux_ks(struct device *device,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	unsigned long value;

	strict_strtoul(buf, 0, &value);

	if (value != 0 && value != 1)
		return -EINVAL;

	if (move_tux != value)
		tux_change_move_tux_flag();

	return count;
}

static ssize_t show_tux_ks(struct device *device,
			   struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", move_tux);
}

static struct device_attribute device_attrs[] = {
	__ATTR(kickstart, S_IRUGO|S_IWUSR, show_tux_ks, store_tux_ks),
};

void notify_moving_tux(int x, int y, int touch_detected)
{
	if (!touch_detected) {
		ts.touch_detected = 0;
		return;
	}

	if (ts.x == x && ts.y == y)
		return;

	ts.x = x;
	ts.y = y;

	if (!ts.touch_detected)
		ts.touch_detected = 1;

	return;
}

static int __init moving_tux_init(void)
{
	int error = 0, i;

	moving_tux_device = device_create(fb_class, NULL, MKDEV(0, 0), NULL,
				     "moving_tux");

	if (IS_ERR(moving_tux_device)) {
		printk(KERN_WARNING "Unable to create device "
		       "for moving_tux; errno = %ld\n",
		       PTR_ERR(moving_tux_device));
		moving_tux_device = NULL;
	}

	for (i = 0; i < ARRAY_SIZE(device_attrs); i++) {
		error = device_create_file(moving_tux_device, &device_attrs[i]);

		if (error)
			break;
	}

	fb_show_logo_fn = fb_get_do_show_logo_fn();
	fb_set_logo_truepalette_fn = fb_get_set_logo_truepalette_fn();

	moving_tux_thread = kthread_create(tux_thread, NULL, "moving_tux");

	if (IS_ERR(moving_tux_thread)) {
		error = PTR_ERR(moving_tux_thread);
		printk(KERN_ERR PFX "cannot create a thread failed (%d)\n",
				error);
	}

	move_tux = 0;

	/* Start the thread. */
	wake_up_process(moving_tux_thread);

	return error;
}

static void __exit moving_tux_exit(void)
{
	kthread_stop(moving_tux_thread);
	return;
}

module_init(moving_tux_init);
module_exit(moving_tux_exit);

MODULE_AUTHOR("Adrian Huang <adrianhuang0701@gmail.com>");
MODULE_DESCRIPTION("Move tux (Linux mascot) on LCD of STM32F series products");
MODULE_LICENSE("GPL");
