#ifndef GPIOHAL_HPP
#define GPIOHAL_HPP

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>


/**
 * @class GpioHal
 * @brief Abstract GPIO access for both input and output devices.
 */
class GpioHal {
private:
    struct gpio_dt_spec gpio_spec; ///< GPIO device specification

public:
    explicit GpioHal(const struct gpio_dt_spec &spec) : gpio_spec(spec) {}

    /**
     * @brief Initialize the GPIO pin.
     * 
     * @param flags GPIO configuration flags
     * @return int 0 on success, error code otherwise
     */
    int init(gpio_flags_t flags) {
        if (!gpio_is_ready_dt(&gpio_spec)) {
            printk("Error: GPIO device %s not ready\n", gpio_spec.port->name);
            return -ENODEV;
        }
        return gpio_pin_configure_dt(&gpio_spec, flags);
    }

    /**
     * @brief Read the GPIO pin state.
     * 
     * @return true if pin is high, false otherwise
     */
    bool read() const { return gpio_pin_get_dt(&gpio_spec); }

    /**
     * @brief Set the GPIO pin state.
     * 
     * @param state true to set high, false to set low
     * @return int 0 on success, error code otherwise
     */
    int set(bool state) const { return gpio_pin_set_dt(&gpio_spec, state); }

    /**
     * @brief Add an interrupt callback to the GPIO pin.
     * 
     * @param callback GPIO callback
     * @return int 0 on success, error code otherwise
     */
    int add_callback(struct gpio_callback *callback) {
        gpio_add_callback(gpio_spec.port, callback);
        return 0;
    }

    /**
     * @brief Configure the GPIO pin interrupt.
     * 
     * @param flags Interrupt configuration flags
     * @return int 0 on success, error code otherwise
     */
    int configure_interrupt(gpio_flags_t flags) { return gpio_pin_interrupt_configure_dt(&gpio_spec, flags); }

    const struct gpio_dt_spec &get_spec() const { return gpio_spec; }
};

#endif // GPIOHAL_HPP