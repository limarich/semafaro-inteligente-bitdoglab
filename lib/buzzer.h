#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stddef.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

// Inicializa os buzzers configurados para os pinos fornecidos
void initialization_buzzers(uint gpio_buzzer_a, uint gpio_buzzer_b);

// Configuração e PWM para os buzzers
void buzzer_pwm(uint gpio, uint16_t frequency, uint16_t duration_ms);

#endif // BUZZER_H