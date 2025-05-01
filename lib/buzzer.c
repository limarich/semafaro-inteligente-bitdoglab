#include "buzzer.h"

// Inicializa os buzzers a serem utilizados
void initialization_buzzers(uint gpio_buzzer_a, uint gpio_buzzer_b)
{
    gpio_init(gpio_buzzer_a);
    gpio_set_dir(gpio_buzzer_a, GPIO_OUT);
    gpio_put(gpio_buzzer_a, 0);

    gpio_init(gpio_buzzer_b);
    gpio_set_dir(gpio_buzzer_b, GPIO_OUT);
    gpio_put(gpio_buzzer_b, 0);
}

// Configura e ativa o PWM para controlar o buzzer
void buzzer_pwm(uint gpio, uint16_t frequency, uint16_t duration_ms)
{
    if (frequency == 0)
        return;

    gpio_set_function(gpio, GPIO_FUNC_PWM);

    uint slice = pwm_gpio_to_slice_num(gpio);
    uint channel = pwm_gpio_to_channel(gpio);

    float clock_div = 4.0f;
    pwm_set_clkdiv(slice, clock_div);

    uint32_t wrap_value = (125000000 / (clock_div * frequency)) - 1;
    pwm_set_wrap(slice, wrap_value);

    pwm_set_chan_level(slice, channel, wrap_value / 2);
    pwm_set_enabled(slice, true);

    sleep_ms(duration_ms);

    // zera o duty cycle
    pwm_set_chan_level(slice, channel, 0);

    pwm_set_enabled(slice, false);
    gpio_set_function(gpio, GPIO_FUNC_SIO);
    gpio_set_dir(gpio, GPIO_OUT);
    gpio_put(gpio, 0);
}