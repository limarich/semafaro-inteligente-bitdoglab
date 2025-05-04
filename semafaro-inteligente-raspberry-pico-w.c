
#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOSConfig.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "lib/buzzer.h"
#include "pio_matrix.pio.h"
#include "lib/leds.h"

#define ledR 13          // pino do led vermelho
#define ledG 11          // pino do led verde
#define signalDelay 1000 // tempo que o led fica aceso
#define BUTTON_A 5
#define DEBOUNCE_MS 200 // intervalo minimo de 200ms para o debounce
#define BUZZER_A 10
#define BUZZER_B 21
#define BUZZER_FREQUENCY 850

// controle do modo do semáfaro
volatile bool night_mode = false;
// variaveis relacionadas a matriz de led
PIO pio;
uint sm;

// inicializacao da PIO
void PIO_setup(PIO *pio, uint *sm);

// controla o led RGB
void vLedTask(void *pvParameters)
{
    gpio_init(ledR);
    gpio_init(ledG);
    gpio_set_dir(ledR, GPIO_OUT);
    gpio_set_dir(ledG, GPIO_OUT);

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        if (!night_mode)
        {
            // Verde por 3 segundos, em etapas de 250ms
            gpio_put(ledR, false);
            gpio_put(ledG, true);
            for (int i = 0; i < 12 && !night_mode; i++)
                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));

            // Amarelo por 1.5 segundos
            gpio_put(ledR, true);
            gpio_put(ledG, true);
            for (int i = 0; i < 6 && !night_mode; i++)
                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));

            // Vermelho por 4 segundos
            gpio_put(ledG, false);
            gpio_put(ledR, true);
            for (int i = 0; i < 16 && !night_mode; i++)
                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
        }
        else
        {
            // Amarelo aceso por 500ms
            gpio_put(ledR, true);
            gpio_put(ledG, true);
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));

            // Amarelo apagado por 500ms
            gpio_put(ledR, false);
            gpio_put(ledG, false);
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
        }
    }
}

// desenho do semáfaro na matriz de leds
void vTrafficLightTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        if (!night_mode)
        {
            // Verde por 3 segundos, checando a cada 250ms
            for (int i = 0; i < 12 && !night_mode; i++)
            {
                draw_traffic_light(pio, sm, GREEN);
                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
            }

            // Amarelo por 1.5 segundos
            for (int i = 0; i < 6 && !night_mode; i++)
            {
                draw_traffic_light(pio, sm, YELLOW);
                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
            }

            // Vermelho por 4 segundos
            for (int i = 0; i < 16 && !night_mode; i++)
            {
                draw_traffic_light(pio, sm, RED);
                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
            }
        }
        else
        {
            draw_traffic_light(pio, sm, YELLOW);
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));

            draw_traffic_light(pio, sm, BLACK);
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
        }
    }
}

// task para o aviso sonoro
void vBuzzerTask(void *pvParameters)
{
    initialization_buzzers(BUZZER_A, BUZZER_B);

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        if (!night_mode)
        {
            // Verde: beep curto (1s)
            buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 1000);
            for (int i = 0; i < 8 && !night_mode; i++) // 2s / 250ms
                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));

            // Amarelo: dois beeps curtos com pausa
            buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 100);
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200));

            if (night_mode)
                continue; // checa antes do segundo beep

            buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 100);
            for (int i = 0; i < 5 && !night_mode; i++) // restante do ciclo ~1.25s
                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));

            // Vermelho: beep longo (500ms) e silêncio por 3.5s
            buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 500);
            for (int i = 0; i < 14 && !night_mode; i++) // 3.5s / 250ms
                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
        }
        else
        {
            // Modo noturno: beep curto a cada 2s
            buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 200);
            for (int i = 0; i < 8 && night_mode; i++) // 2s
                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
        }
    }
}

// task em pooling para leitura do botão
void vButtonTask(void *pvParameters)
{
    bool last_button_state = true;

    while (1)
    {
        bool current_state = gpio_get(BUTTON_A);

        if (!current_state && last_button_state) // botão foi pressionado
        {
            night_mode = !night_mode;
            printf("Modo alterado: %d\n", night_mode);
            vTaskDelay(pdMS_TO_TICKS(200)); // debounce
        }

        last_button_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(10)); // frequência de varredura
    }
}

int main()
{

    stdio_init_all();

    // inicializa a PIO
    PIO_setup(&pio, &sm);

    // configuração do butão A
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    xTaskCreate(vLedTask, "Task do LED RGB", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    // xTaskCreate(vBuzzerTask, "Task de Buzzer", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vTrafficLightTask, "Task do Semafaro com a matrix de led", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vButtonTask, "Leitura Botão", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

    vTaskStartScheduler();
    panic_unsupported();
}

void PIO_setup(PIO *pio, uint *sm)
{
    // configurações da PIO
    *pio = pio0;
    uint offset = pio_add_program(*pio, &pio_matrix_program);
    *sm = pio_claim_unused_sm(*pio, true);
    pio_matrix_program_init(*pio, *sm, offset, LED_PIN);
}