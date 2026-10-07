#define DT_DRV_COMPAT our_driver

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "our_driver.h"

LOG_MODULE_REGISTER(our_driver, LOG_LEVEL_INF);

struct our_driver_config {
	struct gpio_dt_spec led;
};

struct our_driver_data {
	bool led_on;
	uint8_t blink_count;   /* NEW — set by our_driver_preset() */
};

static int our_driver_sample_fetch(const struct device *dev,
				    enum sensor_channel chan)
{
	const struct our_driver_config *config = dev->config;
	struct our_driver_data *data = dev->data;

	ARG_UNUSED(chan);

	int ret = gpio_pin_set_dt(&config->led, 1);
	if (ret < 0) {
		LOG_ERR("Failed to turn LED on (err %d)", ret);
		return ret;
	}

	data->led_on = true;
	LOG_INF("Sample fetch: LED turned ON");

	return 0;
}

static int our_driver_channel_get(const struct device *dev,
				   enum sensor_channel chan,
				   struct sensor_value *val)
{
	const struct our_driver_config *config = dev->config;
	struct our_driver_data *data = dev->data;

	ARG_UNUSED(chan);

	/* Report the LED's state *before* this call turns it off */
	val->val1 = data->led_on ? 1 : 0;
	val->val2 = 0;

	int ret = gpio_pin_set_dt(&config->led, 0);
	if (ret < 0) {
		LOG_ERR("Failed to turn LED off (err %d)", ret);
		return ret;
	}

	data->led_on = false;
	LOG_INF("Channel get: LED turned OFF (was: %d)", val->val1);

	return 0;
}

int our_driver_preset(const struct device *dev, uint8_t count)
{
	const struct our_driver_config *config = dev->config;
	struct our_driver_data *data = dev->data;

	data->blink_count = count;
	LOG_INF("Preset: blink_count set to %u", count);

	for (uint8_t i = 0; i < count; i++) {
		gpio_pin_set_dt(&config->led, 1);
		k_msleep(100);
		gpio_pin_set_dt(&config->led, 0);
		k_msleep(100);
	}

	data->led_on = false;

	return 0;
}

static DEVICE_API(sensor, our_driver_api) = {
	.sample_fetch = our_driver_sample_fetch,
	.channel_get = our_driver_channel_get,
};

static int our_driver_init(const struct device *dev)
{
	const struct our_driver_config *config = dev->config;

	if (!gpio_is_ready_dt(&config->led)) {
		LOG_ERR("LED GPIO device not ready");
		return -ENODEV;
	}

	int ret = gpio_pin_configure_dt(&config->led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure LED GPIO (err %d)", ret);
		return ret;
	}

	LOG_INF("our_driver initialized");
	return 0;
}

static struct our_driver_data our_driver_data_0;

static const struct our_driver_config our_driver_config_0 = {
	.led = GPIO_DT_SPEC_INST_GET(0, gpios),
};

DEVICE_DT_INST_DEFINE(0, our_driver_init, NULL,
		       &our_driver_data_0, &our_driver_config_0,
		       POST_KERNEL, 80, &our_driver_api);

