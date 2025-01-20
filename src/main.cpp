/**
 * @file main.cpp
 * @brief Zephyr application with decoupled hardware access for GPIO input and output.
 *
 * This application demonstrates GPIO handling using a class-based approach
 * with hardware access abstracted through an interface.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include "gpiohal.hpp"
#include "read_class.hpp"
#include "react_class.hpp"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

// Mutex to protect GPIO state
struct k_mutex gpio_state_mutex;

/** @brief Global state of GPIO input. */
bool gpio_state = false;

// Thread stacks
K_THREAD_STACK_DEFINE(thread_1_stack, 1024); /**< Stack for ReadClass thread */
K_THREAD_STACK_DEFINE(thread_2_stack, 1024); /**< Stack for ReactClass thread */

/**
 * @brief GPIO interrupt callback.
 * 
 * @param dev GPIO device pointer
 * @param cb GPIO callback structure
 * @param pins Triggered pins
 */
void ReadClass::gpio_interrupt_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    auto *instance = reinterpret_cast<ReadClass *>(CONTAINER_OF(cb, ReadClass, gpio_cb));
    k_wakeup(instance->thread_id);
}
/**
 * @brief Thread function to process GPIO state changes.
 * 
 * @param arg1 Instance of ReadClass
 * @param arg2 Unused
 * @param arg3 Unused
 */
void ReadClass::thread_handler(void *arg1, void *arg2, void *arg3) {
    auto *self = static_cast<ReadClass *>(arg1);
    self->process_gpio_state();
}

/**
 * @brief Process GPIO state changes with debouncing.
 */
void ReadClass::process_gpio_state() {
    while (true) {
        k_sleep(K_FOREVER);

        bool current_state = gpio_input.read();
        k_msleep(DEBOUNCE_DELAY_MS); // Debounce delay
        bool stable_state = gpio_input.read();

        if (current_state == stable_state && stable_state != last_state) {
            k_mutex_lock(&gpio_state_mutex, K_FOREVER);
            gpio_state = stable_state;
            k_mutex_unlock(&gpio_state_mutex);

            LOG_INF("GPIO state changed: %s\n", stable_state ? "HIGH" : "LOW");
            last_state = stable_state;
        }
    }
}

/**
 * @brief Constructor initializes the GPIO input.
 * 
 * @param gpio Reference to GPIO HAL instance
 */
ReadClass::ReadClass(GpioHal &gpio) : gpio_input(gpio), thread_id(nullptr), last_state(false) {}

/**
 * @brief Initialize the GPIO input and interrupt.
 * 
 * @return int 0 on success, error code otherwise
 */
int ReadClass::init() {
    int ret = gpio_input.init(GPIO_INPUT | GPIO_PULL_UP);
    if (ret) return ret;

    ret = gpio_input.configure_interrupt(GPIO_INT_EDGE_BOTH);
    if (ret) return ret;

    gpio_init_callback(&gpio_cb, gpio_interrupt_callback, BIT(gpio_input.get_spec().pin));
    gpio_input.add_callback(&gpio_cb);

    thread_id = k_thread_create(&thread_data, thread_1_stack, K_THREAD_STACK_SIZEOF(thread_1_stack),
                                thread_handler, this, nullptr, nullptr, READ_THREAD_PRIORITY, 0, K_NO_WAIT);
    return 0;
}

    /**
 * @brief Initialize the GPIO input and interrupt.
 * 
 * @return int 0 on success, error code otherwise
 */
void ReadClass::interrupt_change(bool state) {
    gpio_state = state;
}

/**
 * @brief Thread function to control LED output.
 * 
 * @param arg1 Instance of ReactClass
 * @param arg2 Unused
 * @param arg3 Unused
 */
void ReactClass::thread_handler(void *arg1, void *arg2, void *arg3) {
    auto *self = static_cast<ReactClass *>(arg1);
    while (true) {
        self->control_led();
    }
}

/**
 * @brief Control the LED based on GPIO state.
 */
void ReactClass::control_led() {
    k_msleep(100); // Periodic check

    k_mutex_lock(&gpio_state_mutex, K_FOREVER);
    bool state = gpio_state;
    k_mutex_unlock(&gpio_state_mutex);

    if (state != previous_state) {
        if (state) {
            for (int i = 0; i < 3; ++i) {
                gpio_output.set(true);
                LOG_INF("LED is ON!\n");
                k_msleep(LED_BLINK_DELAY_MS);
                gpio_output.set(false);
                LOG_INF("LED is OFF!\n");
                k_msleep(LED_BLINK_DELAY_MS);
            }
        } else {
            gpio_output.set(true);
            LOG_INF("LED is ON!\n");
            k_msleep(LED_ON_TIME_MS);
            gpio_output.set(false);
            LOG_INF("LED is OFF!\n");
        }
        previous_state = state;
    } 
}

/**
 * @brief Constructor initializes the GPIO output.
 * 
 * @param gpio Reference to GPIO HAL instance
 */
ReactClass::ReactClass(GpioHal &gpio) : gpio_output(gpio), thread_id(nullptr) {}

/**
 * @brief Initialize the GPIO output.
 * 
 * @return int 0 on success, error code otherwise
 */
int ReactClass::init() {
    int ret = gpio_output.init(GPIO_OUTPUT);
    if (ret) return ret;

    thread_id = k_thread_create(&thread_data, thread_2_stack, K_THREAD_STACK_SIZEOF(thread_2_stack),
                                thread_handler, this, nullptr, nullptr, REACT_THREAD_PRIORITY, 0, K_NO_WAIT);
    return 0;
}

void ReactClass::simulate_state_change(bool state) {
    gpio_state = state;
    control_led(); 
}


// Main Application
int main(void) {
    k_mutex_init(&gpio_state_mutex);

    GpioHal gpio_input(GPIO_DT_SPEC_GET_OR(GPIO_INPUT_NODE, gpios, {0}));
    GpioHal gpio_output(GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0}));

    ReadClass read_class(gpio_input);
    ReactClass react_class(gpio_output);

    if (read_class.init() != 0 || react_class.init() != 0) {
        LOG_ERR("Failed to initialize GPIO components\n");
        return -1;
    }
    return 0;
}
