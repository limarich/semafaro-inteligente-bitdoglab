#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOSConfig.h"
#include "lib/buzzer.h"

#define ledR 13          // pino do led vermelho
#define ledG 11          // pino do led verde
#define signalDelay 1000 // tempo que o led fica aceso
#define BUTTON_A 5
#define DEBOUNCE_MS 200 // intervalo minimo de 200ms para o debounce
#define BUZZER_A 10
#define BUZZER_B 21
#define BUZZER_FREQUENCY 850

volatile int last_interrupt_a = 0; // variavel para controlar o debounce
volatile bool night_mode = false;

// gerenciador de interrupcoes
void gpio_irq_handler(uint gpio, uint32_t events)
{
    // variavel para o controle do debounce
    uint current_time = to_ms_since_boot(get_absolute_time());

    if (gpio == BUTTON_A)
    {
        if (current_time - last_interrupt_a > DEBOUNCE_MS)
        {
            last_interrupt_a = current_time;
            night_mode = !night_mode; // muda do modo normal para noturno e vice-versa
        }
    }
}

void vSemafaroTask(void *pvParameters)
{

    // Inicialização dos pinos
    gpio_init(ledR);
    gpio_init(ledG);
    gpio_set_dir(ledR, GPIO_OUT);
    gpio_set_dir(ledG, GPIO_OUT);

    while (1)
    {

        if (!night_mode)
        {

            // sinal verde
            gpio_put(ledR, false);
            gpio_put(ledG, true);
            vTaskDelay(pdMS_TO_TICKS(signalDelay * 2));

            // sinal amarelo
            gpio_put(ledR, true);
            gpio_put(ledG, true);
            vTaskDelay(pdMS_TO_TICKS(signalDelay));

            // sinal vermelho
            gpio_put(ledG, false);
            gpio_put(ledR, true);
            vTaskDelay(pdMS_TO_TICKS(signalDelay * 3));
        }
        else
        {
            // sinal verde
            gpio_put(ledR, true);
            gpio_put(ledG, true);
            vTaskDelay(pdMS_TO_TICKS(signalDelay));

            // sinal amarelo
            gpio_put(ledR, false);
            gpio_put(ledG, false);
            vTaskDelay(pdMS_TO_TICKS(signalDelay));
        }
    }
}

void vBuzzerTask(void *pvParameters)
{
    initialization_buzzers(BUZZER_A, BUZZER_B);

    while (1)
    {
        if (!night_mode)
        {
            // Verde: beep curto no início do sinal verde
            buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 1000);
            vTaskDelay(pdMS_TO_TICKS(signalDelay));

            // Amarelo: dois beeps curtos no início do sinal amarelo
            buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 100);
            vTaskDelay(pdMS_TO_TICKS(200));
            buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 100);
            vTaskDelay(pdMS_TO_TICKS(signalDelay - 300));

            // Vermelho: beep longo no início do sinal vermelho
            buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 500);
            vTaskDelay(pdMS_TO_TICKS((signalDelay * 3) - 500));
        }
        else
        {
            // Modo noturno: beep curto a cada 2s
            buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 200);
            vTaskDelay(pdMS_TO_TICKS(1800));
        }
    }
}

int main()
{
    stdio_init_all();

    // configuração do butão A
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    // habilita interrupção com o botão
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    xTaskCreate(vSemafaroTask, "Task de Semafaro", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vBuzzerTask, "Task de Buzzer", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    vTaskStartScheduler();
    panic_unsupported();
}
