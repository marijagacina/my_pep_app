#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

// #define GPIO_OUTPUT_NODE DT_ALIAS(led0)

// GPIO Input and Output Definitions
#define GPIO_INPUT_NODE DT_NODELABEL(gpio_input)
#define GPIO_OUTPUT_NODE DT_NODELABEL(gpio_output)

#if !DT_NODE_HAS_STATUS(GPIO_INPUT_NODE, okay) || !DT_NODE_HAS_STATUS(GPIO_OUTPUT_NODE, okay)
#error "GPIO input or output not properly defined in device tree"
#endif

#define GPIO_INPUT_PIN DT_GPIO_PIN(GPIO_INPUT_NODE, gpios)

static const struct gpio_dt_spec gpio_input = GPIO_DT_SPEC_GET_OR(GPIO_INPUT_NODE, gpios, {0});;
static const struct gpio_dt_spec gpio_output = GPIO_DT_SPEC_GET_OR(GPIO_OUTPUT_NODE, gpios, {0});

// Debounce delay and LED timing
#define DEBOUNCE_DELAY_MS 50
#define LED_BLINK_DELAY_MS 100
#define LED_ON_TIME_MS 500

static struct k_mutex gpio_state_mutex;
bool gpio_state = false;

K_THREAD_STACK_DEFINE(thread_1_stack, 1024);
K_THREAD_STACK_DEFINE(thread_2_stack, 1024);

// Class Definitions
class ReadClass {
private:
   // const struct device *gpio_dev;
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
						printk("%s: gpio on!\n", __FUNCTION__);

            if (current_state == stable_state) {
       //         k_mutex_lock(&gpio_state_mutex, K_FOREVER);
                gpio_state = current_state;
        //        k_mutex_unlock(&gpio_state_mutex);
            }

			k_msleep(50);

			current_state = 0; //gpio_pin_get(gpio_dev, GPIO_INPUT_PIN);
            k_msleep(DEBOUNCE_DELAY_MS); // Debounce
            stable_state = 0;  //gpio_pin_get(gpio_dev, GPIO_INPUT_PIN);
			printk("%s: gpio off!\n", __FUNCTION__);

            if (current_state == stable_state) {
         //       k_mutex_lock(&gpio_state_mutex, K_FOREVER);
                gpio_state = current_state;
          //      k_mutex_unlock(&gpio_state_mutex);

            }

        }
    }

public:
    ReadClass (): thread_id(nullptr) {}

    void init() {
        int ret;
        //gpio_input = GPIO_DT_SPEC_GET_OR(GPIO_INPUT_NODE, gpios, {0});
        	const char *name = "foo";
        printk("GPIO input: port %s, pin %d\n", name, 1);

        if (!gpio_is_ready_dt(&gpio_input)) {
            printk("Error: GPIO input device is not ready.\n");
            return;
        }
        else {printk("GPIO input device is ready.\n");}
        

        ret = gpio_pin_configure_dt(&gpio_input, GPIO_INPUT | GPIO_PULL_UP);
        if (ret != 0) {
            printk("Error %d: failed to configure %s pin %d\n",
                ret, gpio_input.port->name, gpio_input.pin);
            return;
        }
        else {printk("GPIO input device is conf.\n");}

        ret = gpio_pin_interrupt_configure_dt(&gpio_input, GPIO_INT_EDGE_TO_ACTIVE);
        if (ret != 0) {
            printk("Error %d: failed to configure interrupt on %s pin %d\n",
                ret, gpio_input.port->name, gpio_input.pin);
            return;
        }

        gpio_init_callback(&gpio_cb, gpio_callback, BIT(gpio_input.pin));
        gpio_add_callback(gpio_input.port, &gpio_cb);
        printk("Set up button at %s pin %d\n", gpio_input.port->name, gpio_input.pin);

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
           // k_mutex_lock(&gpio_state_mutex, K_FOREVER);
            bool state = gpio_state;
           // k_mutex_unlock(&gpio_state_mutex);

			if (state != previous_state) {
				if (state) {
					for (int i = 0; i < 3; ++i) {
						//gpio_pin_set(gpio_dev, GPIO_OUTPUT_PIN, 1);
						printk("%s: LED is ON!\n", __FUNCTION__);

						k_msleep(LED_BLINK_DELAY_MS);
						//gpio_pin_set(gpio_dev, GPIO_OUTPUT_PIN, 0);
						printk("%s: LED is OFF!\n", __FUNCTION__);

						k_msleep(LED_BLINK_DELAY_MS);
					}
				} else {

					printk("%s: LED is ON!\n", __FUNCTION__);

					//gpio_pin_set(gpio_dev, GPIO_OUTPUT_PIN, 1);
					k_msleep(LED_ON_TIME_MS);
					printk("%s: LED is OFF!\n", __FUNCTION__);

					//gpio_pin_set(gpio_dev, GPIO_OUTPUT_PIN, 0);
				}
				previous_state = state;
			}
        }
    }

public:
    ReactClass()   {}

    void init() {
        int ret;
       // gpio_output = GPIO_DT_SPEC_GET_OR(GPIO_OUTPUT_NODE, gpios, {0});
        printk("GPIO output: port %s, pin %d\n", "fds", 2);
	    const char *name = "fd";


        if (!gpio_is_ready_dt(&gpio_output)) {
            printk("Error: GPIO output device is not ready.\n");
            return;
        }

        if (gpio_output.port && !gpio_is_ready_dt(&gpio_output)) {
            printk("Error %d: LED device %s is not ready; ignoring it\n",
                ret, name);
        }

        if (gpio_output.port) {
            ret = gpio_pin_configure_dt(&gpio_output, GPIO_OUTPUT);
            if (ret != 0) {
                printk("Error %d: failed to configure LED device %s pin %d\n",
                    ret, name, gpio_output.pin);
            } else {
                printk("Set up LED at %s pin %d\n", gpio_output.port->name, gpio_output.pin);
            }
        }

        k_thread_create(&thread_data, thread_2_stack, K_THREAD_STACK_SIZEOF(thread_2_stack),
                        thread_handler, this, nullptr, nullptr, K_PRIO_PREEMPT(2), 0, K_NO_WAIT);
    }

private:
    struct k_thread thread_data;
};


// Main Function
int main(void) {
    k_mutex_init(&gpio_state_mutex);

    static ReadClass read_class;
    static ReactClass react_class;

    read_class.init();
    react_class.init();
	return 0;
}