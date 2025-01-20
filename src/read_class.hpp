#ifndef READ_CLASS_HPP
#define READ_CLASS_HPP

#include <zephyr/kernel.h>
#include "gpiohal.hpp"

/// Debounce delay in milliseconds
const int DEBOUNCE_DELAY_MS = 50;

/// Priority for the ReadClass thread
const int READ_THREAD_PRIORITY = 1;

/// Thread stack size for the ReadClass thread
const size_t READ_THREAD_STACK_SIZE = 1024;

extern k_mutex gpio_state_mutex; ///< Shared mutex for GPIO state
extern bool gpio_state;          ///< Shared GPIO state

/**
 * @class ReadClass
 * @brief Class responsible for reading GPIO input and handling its state.
 */
class ReadClass {
private:
    GpioHal &gpio_input;         ///< GPIO input abstraction
    struct gpio_callback gpio_cb; ///< GPIO callback structure
    k_tid_t thread_id;           ///< Thread ID for the reading thread
    bool last_state;             ///< Last stable GPIO state

    struct k_thread thread_data; ///< Thread data structure
    static K_THREAD_STACK_DEFINE(thread_stack, READ_THREAD_STACK_SIZE);

    /**
     * @brief GPIO interrupt callback.
     * 
     * @param dev GPIO device pointer
     * @param cb GPIO callback 
     * @param pins Triggered pins
     */
    static void gpio_interrupt_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

    /**
     * @brief Thread function to process GPIO state changes.
     * 
     * @param arg1 Instance of ReadClass
     * @param arg2 Unused
     * @param arg3 Unused
     */
    static void thread_handler(void *arg1, void *arg2, void *arg3);

    /**
     * @brief Process GPIO state changes with debouncing.
     */
    void process_gpio_state();

public:
    /**
     * @brief Constructor initializes the GPIO input.
     * 
     * @param gpio Reference to GPIO HAL instance
     */
    ReadClass(GpioHal &gpio);

    /**
     * @brief Initialize the GPIO input and interrupt.
     * 
     * @return int 0 on success, error code otherwise
     */
    int init();

    /**
     * @brief Initialize the GPIO input and interrupt.
     * 
     * @return int 0 on success, error code otherwise
     */
    void interrupt_change(bool state);

};

#endif // READ_CLASS_HPP