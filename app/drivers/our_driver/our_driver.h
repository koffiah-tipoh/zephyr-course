#ifndef OUR_DRIVER_H_
#define OUR_DRIVER_H_

#include <zephyr/device.h>
#include <stdint.h>

/**
 * @brief Preset the driver's blink-count parameter and immediately
 *        perform that many quick on/off blinks on the LED.
 *
 * Demonstrates a driver extension function that lives outside the
 * standard sensor API table, and that mutates the driver's runtime
 * (dynamic) data struct directly.
 *
 * @param dev    our_driver device instance
 * @param count  number of quick blinks to perform, and value stored
 *               into the driver's data struct for later reference
 *
 * @return 0 on success, negative errno on failure
 */

#ifdef __cplusplus
extern "C" {
#endif

int our_driver_preset(const struct device *dev, uint8_t count);

#ifdef __cplusplus
}
#endif

#endif /* OUR_DRIVER_H_ */

