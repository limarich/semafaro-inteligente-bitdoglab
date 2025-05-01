#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOSConfig.h"

#define ledR 13 // pino do led vermelho
#define ledG 11 // pino do led verde
#define signalDelay 2000

bool night_mode = true;

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
    xTaskCreate(vSemafaroNormalTask, "Semafaro Normal", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vSemafaroNoturnoTask, "Semafaro Noturno", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    vTaskStartScheduler();
    panic_unsupported();
}
