#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOSConfig.h"

#define ledR 13          // pino do led vermelho
#define ledG 11          // pino do led verde
#define signalDelay 2000 // tempo que o led fica aceso
#define BUTTON_A 5
#define DEBOUNCE_MS 200 // intervalo minimo de 200ms para o debounce

volatile int last_interrupt_a = 0; // variavel para controlar o debounce
volatile bool night_mode = true;

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

void vSemafaroNormalTask(void *pvParameters)
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
            vTaskDelay(pdMS_TO_TICKS(signalDelay));

            // sinal amarelo
            gpio_put(ledR, true);
            gpio_put(ledG, true);
            vTaskDelay(pdMS_TO_TICKS(signalDelay / 2));

            // sinal vermelho
            gpio_put(ledG, false);
            gpio_put(ledR, true);
            vTaskDelay(pdMS_TO_TICKS(signalDelay));
        }
    }
}

void vSemafaroNoturnoTask(void *pvParameters)
{

    gpio_init(ledR);
    gpio_init(ledG);
    gpio_set_dir(ledR, GPIO_OUT);
    gpio_set_dir(ledG, GPIO_OUT);

    while (1)
    {
        if (night_mode)
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

int main()
{
    stdio_init_all();

    // configuração do butão A
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    // habilita interrupção com o botão
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    xTaskCreate(vSemafaroNormalTask, "Semafaro Normal", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vSemafaroNoturnoTask, "Semafaro Noturno", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    vTaskStartScheduler();
    panic_unsupported();
}
