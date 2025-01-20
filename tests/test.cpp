#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "gpiohal.hpp"
#include "read_class.hpp"
#include 'react_class.hpp'


class MockGpioHal {
public:
    MOCK_METHOD(int, init, (gpio_flags_t flags), ());
    MOCK_METHOD(bool, read, (), (const));
    MOCK_METHOD(int, set, (bool state), (const));
    MOCK_METHOD(int, add_callback, (struct gpio_callback *callback), ());
    MOCK_METHOD(int, configure_interrupt, (gpio_flags_t flags), ());
    MOCK_METHOD(const struct gpio_dt_spec &, get_spec, (), (const));
};

class ReadClassTest : public ::testing::Test {
protected:
    MockGpioHal mock_gpio;
    ReadClass *read_class;

    virtual void SetUp() {
        read_class = new ReadClass(mock_gpio);
    }

    virtual void TearDown() {
        delete read_class;
    }
};

TEST_F(ReadClassTest, InitializationSuccess) {
    EXPECT_CALL(mock_gpio, init(GPIO_INPUT | GPIO_PULL_UP))
        .Times(1)
        .WillOnce(Return(0));
    EXPECT_CALL(mock_gpio, configure_interrupt(GPIO_INT_EDGE_BOTH))
        .Times(1)
        .WillOnce(Return(0));
    EXPECT_CALL(mock_gpio, add_callback(_))
        .Times(1);

    int ret = read_class->init();
    EXPECT_EQ(ret, 0);
}

TEST_F(ReadClassTest, InitializationFailureOnInit) {
    EXPECT_CALL(mock_gpio, init(GPIO_INPUT | GPIO_PULL_UP))
        .Times(1)
        .WillOnce(Return(-1));

    int ret = read_class->init();
    EXPECT_EQ(ret, -1);
}

TEST_F(ReadClassTest, HandlesGPIOInterrupt) {
    EXPECT_CALL(mock_gpio, init(GPIO_INPUT | GPIO_PULL_UP)).Times(1).WillOnce(Return(0));
    EXPECT_CALL(mock_gpio, configure_interrupt(GPIO_INT_EDGE_BOTH)).Times(1).WillOnce(Return(0));
    EXPECT_CALL(mock_gpio, add_callback(_)).Times(1);

    ASSERT_EQ(read_class->init(), 0);

    EXPECT_CALL(mock_gpio, read())
        .Times(2) 
        .WillOnce(Return(1))
        .WillOnce(Return(1));

    read_class->interrupt_change(true);

    k_sleep(K_MSEC(100));

    // Verify that the state has been updated
    EXPECT_TRUE(gpio_state);
}

/**
 * @brief Main function to run tests.
 */
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
