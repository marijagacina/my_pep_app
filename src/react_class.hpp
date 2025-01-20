#ifndef REACT_CLASS_HPP
#define REACT_CLASS_HPP

#include <zephyr/kernel.h>
#include "gpiohal.hpp"


/// LED blink delay in milliseconds
constexpr int LED_BLINK_DELAY_MS = 100;

/// LED on duration in milliseconds
constexpr int LED_ON_TIME_MS = 500;

/// Priority for the ReactClass thread
constexpr int REACT_THREAD_PRIORITY = 2;

/// Thread stack size for the ReactClass thread
constexpr size_t REACT_THREAD_STACK_SIZE = 1024;

extern k_mutex gpio_state_mutex; ///< Shared mutex for GPIO state
extern bool gpio_state;          ///< Shared GPIO state

/**
 * @class ReactClass
 * @brief Class responsible for reacting to GPIO state changes and controlling an LED.
 */
class ReactClass {
private:
    GpioHal &gpio_output; ///< GPIO output abstraction
    k_tid_t thread_id;    ///< Thread ID for the reacting thread
    bool previous_state;  ///< Last known GPIO state

    struct k_thread thread_data; ///< Thread data structure
    static K_THREAD_STACK_DEFINE(thread_stack, REACT_THREAD_STACK_SIZE);

    /**
     * @brief Thread function to control LED output.
     * 
     * @param arg1 Instance of ReactClass
     * @param arg2 Unused
     * @param arg3 Unused
     */
    static void thread_handler(void *arg1, void *arg2, void *arg3);

    /**
     * @brief Control the LED based on GPIO state.
     */
    void control_led();

public:
    /**
     * @brief Constructor initializes the GPIO output.
     * 
     * @param gpio Reference to GPIO HAL instance
     */
    ReactClass(GpioHal &gpio);

    /**
     * @brief Initialize the GPIO output.
     * 
     * @return int 0 on success, error code otherwise
     */
    int init();

    void simulate_state_change(bool state);

};

#endif // REACT_CLASS_HPP