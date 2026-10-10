/*
 * Shell commands for our_driver (LED-backed sensor).
 *
 *   sensor fetch        -> sensor_sample_fetch()  (turns the LED on)
 *   sensor info         -> prints device name and ready state
 *   sensor read         -> sensor_channel_get()   (turns the LED off, prints prior state)
 *   sensor set <count>  -> our_driver_preset()    (L06 Task 2 extension API)
 *
 * Compiled only when CONFIG_SHELL=y (see drivers/our_driver/CMakeLists.txt).
 */

#include <errno.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/shell/shell.h>

#include "our_driver.h"

/*
 * Accepted range for "sensor set". our_driver_preset() sleeps ~200 ms per
 * blink on the calling (shell) thread, so the upper bound is kept small to
 * avoid blocking the shell for long: 10 blinks is roughly 2 seconds.
 */
#define PRESET_MIN 1
#define PRESET_MAX 10

/*
 * Look the device up by compatible rather than by node label, so this file
 * stays decoupled from the overlay. Evaluates to NULL if no enabled node
 * with compatible "our,driver" exists.
 */
#define OUR_DRIVER_DEV DEVICE_DT_GET_ANY(our_driver)

/* Returns the device if it exists and is ready, otherwise prints an error. */
static const struct device *get_ready_dev(const struct shell *sh)
{
	const struct device *dev = OUR_DRIVER_DEV;

	if (dev == NULL) {
		shell_error(sh, "No our_driver device found in the devicetree");
		return NULL;
	}

	if (!device_is_ready(dev)) {
		shell_error(sh, "Device %s is not ready", dev->name);
		return NULL;
	}

	return dev;
}

static int cmd_sensor_fetch(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *dev = get_ready_dev(sh);

	if (dev == NULL) {
		return -ENODEV;
	}

	int ret = sensor_sample_fetch(dev);

	if (ret < 0) {
		shell_error(sh, "sensor_sample_fetch failed (err %d)", ret);
		return ret;
	}

	shell_print(sh, "Sample fetched (LED on)");
	return 0;
}

static int cmd_sensor_info(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *dev = OUR_DRIVER_DEV;

	if (dev == NULL) {
		shell_error(sh, "No our_driver device found in the devicetree");
		return -ENODEV;
	}

	shell_print(sh, "Device name : %s", dev->name);
	shell_print(sh, "Ready       : %s", device_is_ready(dev) ? "yes" : "no");
	return 0;
}

static int cmd_sensor_read(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *dev = get_ready_dev(sh);

	if (dev == NULL) {
		return -ENODEV;
	}

	struct sensor_value val;
	int ret = sensor_channel_get(dev, SENSOR_CHAN_ALL, &val);

	if (ret < 0) {
		shell_error(sh, "sensor_channel_get failed (err %d)", ret);
		return ret;
	}

	shell_print(sh, "LED state before read: %d (LED now off)", val.val1);
	return 0;
}

static int cmd_sensor_set(const struct shell *sh, size_t argc, char **argv)
{
	/* Missing argument (the shell only guarantees argc <= 2 here) */
	if (argc < 2) {
		shell_error(sh, "Missing argument. Usage: sensor set <%d-%d>",
			    PRESET_MIN, PRESET_MAX);
		return -EINVAL;
	}

	/* Must be a clean base-10 integer, e.g. reject "abc" and "3x" */
	char *end;

	errno = 0;
	long value = strtol(argv[1], &end, 10);

	if (end == argv[1] || *end != '\0' || errno == ERANGE) {
		shell_error(sh, "Invalid number: '%s'", argv[1]);
		return -EINVAL;
	}

	/* Range check */
	if (value < PRESET_MIN || value > PRESET_MAX) {
		shell_error(sh, "Value %ld out of range (%d-%d)",
			    value, PRESET_MIN, PRESET_MAX);
		return -EINVAL;
	}

	const struct device *dev = get_ready_dev(sh);

	if (dev == NULL) {
		return -ENODEV;
	}

	int ret = our_driver_preset(dev, (uint8_t)value);

	if (ret < 0) {
		shell_error(sh, "our_driver_preset failed (err %d)", ret);
		return ret;
	}

	shell_print(sh, "Preset applied: blink_count = %ld", value);
	return 0;
}

/*
 * SHELL_CMD_ARG(name, subcmds, help, handler, mandatory, optional):
 * "mandatory" counts the command word itself.
 *   - fetch/info/read: 1, 0 -> no extra arguments; the shell rejects
 *     e.g. `sensor fetch foo` on its own.
 *   - set: 1, 1 -> at most one argument; the shell rejects `sensor set 1 2`,
 *     and the handler reports a missing argument itself via shell_error.
 *     (Use 2, 0 instead if you want the shell core to reject a missing
 *     argument before the handler runs.)
 */
SHELL_STATIC_SUBCMD_SET_CREATE(sensor_cmds,
	SHELL_CMD_ARG(fetch, NULL,
		      "Fetch a sample (turns the LED on)",
		      cmd_sensor_fetch, 1, 0),
	SHELL_CMD_ARG(info, NULL,
		      "Print the device name and ready state",
		      cmd_sensor_info, 1, 0),
	SHELL_CMD_ARG(read, NULL,
		      "Read the channel (turns the LED off, prints its prior state)",
		      cmd_sensor_read, 1, 0),
	SHELL_CMD_ARG(set, NULL,
		      "Preset the blink count and blink the LED that many times: set <1-10>",
		      cmd_sensor_set, 1, 1),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(sensor, &sensor_cmds, "LED sensor commands", NULL);
