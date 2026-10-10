/*
 * Shell commands for our_driver (LED-backed sensor).
 *
 *   sensor fetch   -> sensor_sample_fetch()  (turns the LED on)
 *   sensor read    -> sensor_channel_get()   (turns the LED off, prints prior state)
 *   sensor info    -> prints device name and ready state
 *
 * Compiled only when CONFIG_SHELL=y (see drivers/our_driver/CMakeLists.txt).
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/shell/shell.h>

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

/*
 * SHELL_CMD_ARG(name, subcmds, help, handler, mandatory, optional):
 * "mandatory" counts the command word itself, so 1 means "no extra
 * arguments" -- the shell rejects e.g. `sensor fetch foo` on its own.
 */
SHELL_STATIC_SUBCMD_SET_CREATE(sensor_cmds,
	SHELL_CMD_ARG(fetch, NULL,
		      "Fetch a sample (turns the LED on)",
		      cmd_sensor_fetch, 1, 0),
	SHELL_CMD_ARG(read, NULL,
		      "Read the channel (turns the LED off, prints its prior state)",
		      cmd_sensor_read, 1, 0),
	SHELL_CMD_ARG(info, NULL,
		      "Print the device name and ready state",
		      cmd_sensor_info, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(sensor, &sensor_cmds, "LED sensor commands", NULL);
