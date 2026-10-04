/*
 * Copyright (c) 2017 Linaro Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>

static int said_board_init(void)
{
	printk("Board Initialized\n");
	return 0;
}

SYS_INIT(said_board_init, POST_KERNEL, 0);
