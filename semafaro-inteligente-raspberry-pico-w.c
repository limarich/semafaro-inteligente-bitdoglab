
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
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"

#define ledR 13          // pino do led vermelho
#define ledG 11          // pino do led verde
#define signalDelay 1000 // tempo que o led fica aceso
#define BUTTON_A 5
#define DEBOUNCE_MS 200 // intervalo minimo de 200ms para o debounce
#define BUZZER_A 10
#define BUZZER_B 21
#define BUZZER_FREQUENCY 1000
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

// controle do modo do semáfaro
typedef enum
{
    GREEN_LIGHT,
    YELLOW_LIGHT,
    RED_LIGHT,
    NIGHT_MODE
} traffic_light_state;

volatile traffic_light_state light_state = GREEN_LIGHT;
// controla se o buzzer já tocou após mudar
volatile bool buzzer_already_played = false;
// variaveis relacionadas a matriz de led
PIO pio;
uint sm;
// variavel do display
ssd1306_t ssd; // Inicializa a estrutura do display

// inicializacao da PIO
void PIO_setup(PIO *pio, uint *sm);
// inicializa o display
void ssd_setup();

// controla a cor do semáfaro
void vTrafficLightControllerTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        if (light_state == NIGHT_MODE)
        {
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
            continue;
        }

        light_state = GREEN_LIGHT;
        buzzer_already_played = false;

        for (int i = 0; i < 12 && light_state != NIGHT_MODE; i++)
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));

        if (light_state == NIGHT_MODE)
            continue;

        light_state = YELLOW_LIGHT;
        buzzer_already_played = false;

        for (int i = 0; i < 6 && light_state != NIGHT_MODE; i++)
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));

        if (light_state == NIGHT_MODE)
            continue;

        light_state = RED_LIGHT;
        buzzer_already_played = false;
        for (int i = 0; i < 16 && light_state != NIGHT_MODE; i++)
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
    }
}

// controla o led RGB
void vLedTask(void *pvParameters)
{
    gpio_init(ledR);
    gpio_init(ledG);
    gpio_set_dir(ledR, GPIO_OUT);
    gpio_set_dir(ledG, GPIO_OUT);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    static bool toggle = false;

    while (1)
    {
        switch (light_state)
        {
        case GREEN_LIGHT:
            gpio_put(ledR, false);
            gpio_put(ledG, true);
            break;

        case YELLOW_LIGHT:
            gpio_put(ledR, true);
            gpio_put(ledG, true);
            break;

        case RED_LIGHT:
            gpio_put(ledR, true);
            gpio_put(ledG, false);
            break;

        case NIGHT_MODE:
            toggle = !toggle;
            gpio_put(ledR, toggle);
            gpio_put(ledG, toggle);
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
            break;

        default:
            gpio_put(ledR, false);
            gpio_put(ledG, false);
            break;
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}

// desenho do semáfaro na matriz de leds
void vTrafficLightTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    static bool toggle = false;

    while (1)
    {
        switch (light_state)
        {
        case GREEN_LIGHT:
            draw_traffic_light(pio, sm, GREEN);
            break;

        case YELLOW_LIGHT:
            draw_traffic_light(pio, sm, YELLOW);
            break;

        case RED_LIGHT:
            draw_traffic_light(pio, sm, RED);
            break;

        case NIGHT_MODE:
            toggle = !toggle;
            draw_traffic_light(pio, sm, toggle ? YELLOW : BLACK);
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
            break;

        default:
            draw_traffic_light(pio, sm, BLACK);
            break;
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}

// task para o aviso sonoro
void vBuzzerTask(void *pvParameters)
{
    initialization_buzzers(BUZZER_A, BUZZER_B);

    while (1)
    {
        if (light_state == NIGHT_MODE)
        {
            buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 200);
            vTaskDelay(pdMS_TO_TICKS(2800)); // 3s total
            continue;
        }

        switch (light_state)
        {
        case GREEN_LIGHT:
            if (!buzzer_already_played)
            {
                buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 1000);
                buzzer_already_played = true;
            }
            break;

        case YELLOW_LIGHT:
            if (!buzzer_already_played)
            {
                // Aguarda 100ms para garantir que a luz amarela já esteja visível
                vTaskDelay(pdMS_TO_TICKS(100));
                buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 50);
                vTaskDelay(pdMS_TO_TICKS(100));
                buzzer_already_played = false;
            }
            break;

        case RED_LIGHT:
            if (!buzzer_already_played)
            {
                vTaskDelay(pdMS_TO_TICKS(100));

                // Toca beep de 500ms, espera 1.5s e repete enquanto estiver no vermelho
                buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 500);
                vTaskDelay(pdMS_TO_TICKS(1500));
                buzzer_already_played = true;
            }
            break;

        default:
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(250));
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
            if (light_state == NIGHT_MODE)
            {
                light_state = GREEN_LIGHT;
                printf("Modo alterado: Normal\n");
            }
            else
            {
                light_state = NIGHT_MODE;
                printf("Modo alterado: Noturno\n");
            }

            vTaskDelay(pdMS_TO_TICKS(200)); // debounce
        }

        last_button_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(10)); // polling rápido
    }
}

// Task para desenhar no display
void vDisplayAnimationTask(void *pvParameters)
{
    float arm_rotate = 0;
    float leg_rotate = 0;
    int arm_factor = 2;
    int leg_factor = 3;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1)
    {

        ssd1306_fill(&ssd, false);
        // cabeça
        ssd1306_rect(&ssd, 10, (WIDTH - 10) / 2, 10, 10, true, true);
        // tronco
        ssd1306_rect(&ssd, 20, (WIDTH - 10) / 2 + 2, 6, 20, true, true);
        // braço 1
        ssd1306_rotated_rect_angle(&ssd, (WIDTH - 10) / 2 + 10, 26, 10, 5, 45.0 + arm_rotate, true);
        // braço 2
        ssd1306_rotated_rect_angle(&ssd, (WIDTH - 10) / 2, 26, 10, 5, 135 - arm_rotate, true);
        // perna 1
        ssd1306_rotated_rect_angle(&ssd, (WIDTH - 10) / 2 + 3, 40, 15, 5, 90.0 + leg_rotate, true);
        // perna 2
        ssd1306_rotated_rect_angle(&ssd, (WIDTH - 10) / 2 + 6, 40, 15, 5, 90.0 - leg_rotate, true);

        ssd1306_draw_string(&ssd, "PODE SEGUIR", 0, 0);

        ssd1306_send_data(&ssd);

        arm_rotate += arm_factor;
        leg_rotate += leg_factor;
        if (arm_rotate < 0)
            arm_factor *= -1;
        if (arm_rotate > 45)
            arm_factor *= -1;
        if (leg_rotate < 0)
            leg_factor *= -1;
        if (leg_rotate > 45)
            leg_factor *= -1;
    }
}
int main()
{

    stdio_init_all();

    // inicializa a PIO
    PIO_setup(&pio, &sm);
    // inicializa o display
    ssd_setup();

    // configuração do butão A
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    xTaskCreate(vTrafficLightControllerTask, "Task de gerenciamento do estado global", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vLedTask, "Task do LED RGB", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vBuzzerTask, "Task de Buzzer", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vTrafficLightTask, "Task do Semafaro com a matrix de led", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vButtonTask, "Leitura Botão", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vDisplayAnimationTask, "Desenha no display", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

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

void ssd_setup()
{
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    ssd1306_send_data(&ssd);                                      // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}