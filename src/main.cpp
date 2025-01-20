/**
 * @file main.cpp
 * @brief Zephyr application for GPIO input and output handling using classes.
 * 
 * This application demonstrates the use of GPIO input and output with a
 * class-based approach in Zephyr RTOS. The ReadClass monitors a GPIO input,
 * while the ReactClass controls an LED based on the input state.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <inttypes.h>
#include <zephyr/logging/log.h>

// Module for logging
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

// GPIO Definitions
#define GPIO_INPUT_NODE DT_NODELABEL(gpio_input) /**< Device Tree node for GPIO input */
#define GPIO_OUTPUT_NODE DT_NODELABEL(gpio_output) /**< Device Tree node for GPIO output */

#if !DT_NODE_HAS_STATUS(GPIO_INPUT_NODE, okay) || !DT_NODE_HAS_STATUS(GPIO_OUTPUT_NODE, okay)
#error "GPIO input or output not properly defined in device tree"
#endif

// GPIO Specifications
static struct gpio_dt_spec gpio_input = GPIO_DT_SPEC_GET_OR(GPIO_INPUT_NODE, gpios, {0}); /**< GPIO input device */
static struct gpio_dt_spec gpio_output = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0}); /**< GPIO output device */

// Timing Constants
#define DEBOUNCE_DELAY_MS 50 /**< Debounce delay in milliseconds */
#define LED_BLINK_DELAY_MS 100 /**< LED blink delay in milliseconds */
#define LED_ON_TIME_MS 500 /**< LED ON time in milliseconds */

// Mutex to protect GPIO state
static struct k_mutex gpio_state_mutex;

/** @brief Global state of GPIO input. */
bool gpio_state = false;

// Thread stacks
K_THREAD_STACK_DEFINE(thread_1_stack, 1024); /**< Stack for ReadClass thread */
K_THREAD_STACK_DEFINE(thread_2_stack, 1024); /**< Stack for ReactClass thread */

/**
 * @brief Interface for GPIO operations.
 */
class GPIO {
public:
    virtual ~GPIO() = default;

    /**
     * @brief Configure the GPIO pin.
     */
    virtual int configure() = 0;

    /**
     * @brief Set the GPIO pin.
     * 
     * @param value Boolean value to set the pin state.
     */
    virtual void set(bool value) = 0;

    /**
     * @brief Get the GPIO pin state.
     * 
     * @return true if the pin is high, false otherwise.
     */
    virtual bool get() = 0;
};

class ZephyrGPIO : public GPIO {
private:
    struct gpio_dt_spec spec;

public:
    explicit ZephyrGPIO(const struct gpio_dt_spec& spec) : spec(spec) {}

    int configure() override {
        if (!gpio_is_ready_dt(&spec)) {
            LOG_ERR("GPIO device %s is not ready", spec.port->name);
            return -ENODEV;
        }
        return gpio_pin_configure_dt(&spec, GPIO_OUTPUT);
    }

    void set(bool value) override {
        gpio_pin_set_dt(&spec, value);
    }

    bool get() override {
        return gpio_pin_get_dt(&spec);
    }
};

// Class Definitions
class ReadClass {
private:
    struct gpio_callback gpio_cb;
    k_tid_t thread_id;

    static void gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
        auto *instance = reinterpret_cast<ReadClass *>(CONTAINER_OF(cb, ReadClass, gpio_cb));
        k_wakeup(instance->thread_id);
    }

    static void thread_handler(void *arg1, void *arg2, void *arg3) {
        auto *instance = static_cast<ReadClass *>(arg1);
        instance->process_gpio_state();
    }

    void process_gpio_state() {
        while (true) {
            k_sleep(K_FOREVER);
			k_msleep(50);
			
            bool current_state = 1; //gpio_pin_get(gpio_dev, GPIO_INPUT_PIN);
            k_msleep(DEBOUNCE_DELAY_MS); // Debounce
            bool stable_state = 1;  //gpio_pin_get(gpio_dev, GPIO_INPUT_PIN);
			LOG_INF("%s: gpio on!\n", __FUNCTION__);

            if (current_state == stable_state) {
                k_mutex_lock(&gpio_state_mutex, K_FOREVER);
                gpio_state = current_state;
                k_mutex_unlock(&gpio_state_mutex);
            }

			k_msleep(50);

			current_state = 0; //gpio_pin_get(gpio_dev, GPIO_INPUT_PIN);
            k_msleep(DEBOUNCE_DELAY_MS); // Debounce
            stable_state = 0;  //gpio_pin_get(gpio_dev, GPIO_INPUT_PIN);
			LOG_INF("%s: gpio off!\n", __FUNCTION__);

            if (current_state == stable_state) {
                k_mutex_lock(&gpio_state_mutex, K_FOREVER);
                gpio_state = current_state;
                k_mutex_unlock(&gpio_state_mutex);
            }

        }
    }

public:
    ReadClass (): thread_id(nullptr) {}

    void init() {
        int ret;
        if (!gpio_is_ready_dt(&gpio_input)) {
            LOG_ERR("Error: GPIO input device is not ready.\n");
            return;
        }
        else {LOG_INF("GPIO input device is ready.\n");}
        

        ret = gpio_pin_configure_dt(&gpio_input, GPIO_INPUT | GPIO_PULL_UP);
        if (ret != 0) {
            LOG_ERR("Error %d: failed to configure %s pin %d\n",
                ret, gpio_input.port->name, gpio_input.pin);
            return;
        }
        else {LOG_INF("GPIO input device is conf.\n");}

        ret = gpio_pin_interrupt_configure_dt(&gpio_input, GPIO_INT_EDGE_BOTH);
        if (ret != 0) {
            LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n",
                ret, gpio_input.port->name, gpio_input.pin);
            return;
        }

        gpio_init_callback(&gpio_cb, gpio_callback, BIT(gpio_input.pin));
        gpio_add_callback(gpio_input.port, &gpio_cb);
        LOG_INF("Set up button at %s pin %d\n", gpio_input.port->name, gpio_input.pin);

        thread_id = k_thread_create(&thread_data, thread_1_stack, K_THREAD_STACK_SIZEOF(thread_1_stack),
                                    thread_handler, this, nullptr, nullptr, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
    }

private:
    struct k_thread thread_data;
};

class ReactClass {
private:
    static void thread_handler(void *arg1, void *arg2, void *arg3) {
        auto *instance = static_cast<ReactClass *>(arg1);
        instance->control_led();
    }

    void control_led() {
		static bool previous_state = 1;
        while (true) {
            k_msleep(100); // Periodic check
            k_mutex_lock(&gpio_state_mutex, K_FOREVER);
            bool state = gpio_state;
            k_mutex_unlock(&gpio_state_mutex);

			if (state != previous_state) {
				if (state) {
					for (int i = 0; i < 3; ++i) {
                        gpio_pin_set_dt(&gpio_output, 1);
						LOG_INF("%s: LED is ON!\n", __FUNCTION__);

						k_msleep(LED_BLINK_DELAY_MS);
                        gpio_pin_set_dt(&gpio_output, 0);
						LOG_INF("%s: LED is OFF!\n", __FUNCTION__);

						k_msleep(LED_BLINK_DELAY_MS);
					}
				} else {

					LOG_INF("%s: LED is ON!\n", __FUNCTION__);
                    gpio_pin_set_dt(&gpio_output, 1);
					k_msleep(LED_ON_TIME_MS);
					LOG_INF("%s: LED is OFF!\n", __FUNCTION__);
                    gpio_pin_set_dt(&gpio_output, 0);
				}
				previous_state = state;
			}
        }
    }

public:
    ReactClass()   {}

    void init() {
        int ret = 0;

        if (gpio_output.port && !gpio_is_ready_dt(&gpio_output)) {
            LOG_ERR("Error %d: Output device is not ready; ignoring it\n",
                ret);
            gpio_output.port = NULL;
        }

        if (gpio_output.port) {
            ret = gpio_pin_configure_dt(&gpio_output, GPIO_OUTPUT);
            if (ret != 0) {
                LOG_ERR("Error %d: failed to configure LED device %s pin %d\n",
                    ret, gpio_output.port->name, gpio_output.pin);
                gpio_output.port = NULL;
            } else {
                LOG_INF("Set up LED at %s pin %d\n", gpio_output.port->name, gpio_output.pin);
            }
        }
        
        k_thread_create(&thread_data, thread_2_stack, K_THREAD_STACK_SIZEOF(thread_2_stack),
                        thread_handler, this, nullptr, nullptr, K_PRIO_PREEMPT(2), 0, K_NO_WAIT);
    }

private:
    struct k_thread thread_data;
};

int main(void) {
    k_mutex_init(&gpio_state_mutex);

    static ReadClass read_class;
    static ReactClass react_class;

    read_class.init();
    react_class.init();
	return 0;
}