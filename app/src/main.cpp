#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include "our_driver.h"

#define SLEEP_TIME_MS 1000

/* The devicetree node identifier for the "led0" alias. */
#define LED_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

void test_our_driver(void)
{
        const struct device *our_dev = DEVICE_DT_GET(DT_NODELABEL(our_driver0));

        if (!device_is_ready(our_dev)) {
                LOG_ERR("our_driver not ready");
                return;
        }

        sensor_sample_fetch(our_dev);     /* LED ON */
        k_msleep(1000);

        struct sensor_value val;
        sensor_channel_get(our_dev, SENSOR_CHAN_ALL, &val);  /* LED OFF */
        LOG_INF("LED was: %d", val.val1);

	/* Task 2: custom extension API, independent of the sensor API */
	//our_driver_preset(our_dev, 3);
}


int main(void)
{
    bool led_state = true;

    if (!gpio_is_ready_dt(&led)) return 0;

    if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) return 0;

    /* Run the Task 1 driver test once before the blink loop starts */
    test_our_driver();

    while (1) {
        if (gpio_pin_toggle_dt(&led) < 0) return 0;

        led_state = !led_state;
        LOG_INF("LED state: %s", led_state ? "ON" : "OFF");
        k_msleep(SLEEP_TIME_MS);
    }
    return 0;
}
