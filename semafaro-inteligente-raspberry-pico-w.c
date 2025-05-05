
#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOSConfig.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"

#include "pio_matrix.pio.h"
#include "lib/buzzer.h"
#include "lib/leds.h"
#include "lib/ssd1306.h"
#include "lib/font.h"

#define ledR 13               // pino do led vermelho
#define ledG 11               // pino do led verde
#define signalDelay 1000      // tempo que o led fica aceso
#define BUTTON_A 5            // BOTÃO B para mudar o modo do semáfaro
#define BUTTON_B 6            // BOTÃO A para desligar o beep
#define DEBOUNCE_MS 200       // intervalo minimo de 200ms para o debounce
#define BUZZER_A 10           // PORTA DO BUZZER A
#define BUZZER_B 21           // PORTA DO BUZZER B
#define BUZZER_FREQUENCY 1500 // FREQUENCIA DO BUZZER
#define I2C_PORT i2c1         // PORTA DO i2C
#define I2C_SDA 14            // PINO DO SDA
#define I2C_SCL 15            // PINO DO SCL
#define endereco 0x3C         // ENDEREÇO

// controle do modo do semáfaro
typedef enum
{
    GREEN_LIGHT,
    YELLOW_LIGHT,
    RED_LIGHT,
    NIGHT_MODE
} traffic_light_state;
// estado do semáfaro
volatile traffic_light_state light_state = GREEN_LIGHT;
// controla se o buzzer já tocou após mudar
volatile bool buzzer_already_played = false;
// controla se o buzzer está habilitado
volatile bool buzzer_active = true;
// controla a luz amarela piscando
volatile bool night_toggle = false;

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
    // Marca o tempo atual para controle do atraso periódico
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        // Se estiver no modo noturno, alterna o estado da variável de piscar luzes
        if (light_state == NIGHT_MODE)
        {
            night_toggle = !night_toggle;

            // AGUARDA 1s para piscar o led
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
            continue;
        }

        // MUDA O VALOR DO ESTADO DO SEMÁFARO
        // SEMPRE QUE MUDAR O ESTADO O BUZZER É LIBERADO PARA TOCAR
        light_state = GREEN_LIGHT;
        buzzer_already_played = false;

        // SINAL VERDE POR 3s(APENAS DISPLAY, LED E MATRIZ DE LEDS)
        for (int i = 0; i < 12 && light_state != NIGHT_MODE; i++)
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));

        // CHECA SE O VALOR DO ESTADO MUDOU
        if (light_state == NIGHT_MODE)
            continue;

        // MUDA O VALOR DO ESTADO DO SEMÁFARO
        light_state = YELLOW_LIGHT;
        buzzer_already_played = false;

        // SINAL AMARELO POR 1.5s(APENAS DISPLAY, LED E MATRIZ DE LEDS)
        for (int i = 0; i < 6 && light_state != NIGHT_MODE; i++)
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));

        if (light_state == NIGHT_MODE)
            continue;

        // MUDA O VALOR DO ESTADO DO SEMÁFARO
        light_state = RED_LIGHT;
        buzzer_already_played = false;

        // SINAL VERMELHO POR 4s(APENAS DISPLAY, LED E MATRIZ DE LEDS)
        for (int i = 0; i < 16 && light_state != NIGHT_MODE; i++)
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
    }
}

// controla o led RGB
void vLedTask(void *pvParameters)
{
    // INICIALIZA OS PINOS DO LED RBG
    gpio_init(ledR);
    gpio_init(ledG);
    gpio_set_dir(ledR, GPIO_OUT);
    gpio_set_dir(ledG, GPIO_OUT);

    // Marca o tempo atual para controle do atraso periódico
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {

        // CHAVEAMENTO DA COR EXIBIDA DO LED BASEADO NO ESTADO DO SEMÁFARO
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
            gpio_put(ledR, night_toggle);
            gpio_put(ledG, night_toggle);
            break;

        default:
            gpio_put(ledR, false);
            gpio_put(ledG, false);
            break;
        }

        // EVITA USO EXCESSIVO DE CPU
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}

// desenho do semáfaro na matriz de leds
void vTrafficLightTask(void *pvParameters)
{
    // Marca o tempo atual para controle do atraso periódico
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        // CHAVEAMENTO DA ANIMACÃO DO SEMÁFARO EXIBIDA NA MATRIZ DE LEDS BASEADO NO ESTADO DO SEMÁFARO
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
            draw_traffic_light(pio, sm, night_toggle ? YELLOW : BLACK);
            break;

        default:
            draw_traffic_light(pio, sm, BLACK);
            break;
        }
        // EVITA USO EXCESSIVO DE CPU
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}

// task para o aviso sonoro
void vBuzzerTask(void *pvParameters)
{
    // INCIALIZA OS BUZZERS
    initialization_buzzers(BUZZER_A, BUZZER_B);

    while (1)
    {
        if (buzzer_active)
        {

            // TOCA UM BEEP A CADA 2s
            if (light_state == NIGHT_MODE)
            {
                buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 200); // 200ms
                vTaskDelay(pdMS_TO_TICKS(1800));             // 2s total
                continue;
            }

            switch (light_state)
            {
            case GREEN_LIGHT:
                // TOCA UM BEEP DE 1s PARA INDICAR SINAL VERDE
                if (!buzzer_already_played)
                {
                    buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 1000); // 1s
                    buzzer_already_played = true;
                }
                break;

            case YELLOW_LIGHT:
                // TOCA BEEPS INTERMITENTES ATÉ MUDAR DE COR NO SEMÁFARO
                if (!buzzer_already_played)
                {
                    // Aguarda 100ms para garantir que a luz amarela já esteja visível
                    vTaskDelay(pdMS_TO_TICKS(200));
                    buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 50);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    buzzer_already_played = false;
                }
                break;

            case RED_LIGHT:
                // TOCA UM BEEP DE 500s PARA INDICAR SINAL VERMELHO
                if (!buzzer_already_played)
                {
                    vTaskDelay(pdMS_TO_TICKS(100));
                    buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY, 500);
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    buzzer_already_played = true;
                }
                break;

            default:
                break;
            }

            // ECONOMIA DE CPU
            vTaskDelay(pdMS_TO_TICKS(250));
        }
    }
}

// task em pooling para leitura do botão
void vButtonTask(void *pvParameters)
{
    // configuração do botão A
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    // configuração do botão B
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    bool last_button_a_state = true;
    bool last_button_b_state = true;

    while (1)
    {
        bool current_a_state = gpio_get(BUTTON_A);
        bool current_b_state = gpio_get(BUTTON_B);

        // Botão A PRESSIONADO MODIFICA O MODO DO SEMÁFARO
        if (!current_a_state && last_button_a_state)
        {
            if (light_state == NIGHT_MODE)
            {
                light_state = GREEN_LIGHT;
            }
            else
            {
                light_state = NIGHT_MODE;
            }

            vTaskDelay(pdMS_TO_TICKS(200)); // debounce
        }

        // Botão B pressionado
        if (!current_b_state && last_button_b_state)
        {
            buzzer_active = !buzzer_active;
            vTaskDelay(pdMS_TO_TICKS(200)); // debounce
        }

        last_button_a_state = current_a_state;
        last_button_b_state = current_b_state;

        vTaskDelay(pdMS_TO_TICKS(10)); // polling rápido
    }
}

// Task para desenhar no display
void vDisplayAnimationTask(void *pvParameters)
{
    float arm_rotate = 0; // CONTROLA A ROTAÇÃO DOS BRAÇOS NA ANIMAÇÃO
    float leg_rotate = 0; // CONTROLA A ROTAÇÃO DAS PERNAS NA ANIMAÇÃO
    int arm_factor = 4;   // FATOR QUE ALTERA A ROTAÇÃO
    int leg_factor = 6;   // FATOR QUE ALTERA A ROTAÇÃO

    // Marca o tempo atual para controle do atraso periódico
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        ssd1306_fill(&ssd, false); // LIMPA O DISPLAY

        // Desenha o boneco
        ssd1306_rect(&ssd, 10, (WIDTH - 10) / 2, 10, 10, true, true);                              // cabeça
        ssd1306_rect(&ssd, 20, (WIDTH - 10) / 2 + 2, 6, 20, true, true);                           // tronco
        ssd1306_rotated_rect_angle(&ssd, (WIDTH - 10) / 2 + 10, 26, 10, 5, 45 + arm_rotate, true); // braço 1
        ssd1306_rotated_rect_angle(&ssd, (WIDTH - 10) / 2, 26, 10, 5, 135 - arm_rotate, true);     // braço 2
        ssd1306_rotated_rect_angle(&ssd, (WIDTH - 10) / 2 + 3, 40, 15, 5, 90 + leg_rotate, true);  // perna 1
        ssd1306_rotated_rect_angle(&ssd, (WIDTH - 10) / 2 + 6, 40, 15, 5, 90 - leg_rotate, true);  // perna 2

        // NO MODO NORTURNO PISCA A TELA A CADA 1s
        if (light_state == NIGHT_MODE && night_toggle)
        {
            ssd1306_fill(&ssd, true);
        }

        // CONTROLA AS MENSAGENS DE AVISO NO DISPLAY
        switch (light_state)
        {
        case GREEN_LIGHT:
            ssd1306_draw_string(&ssd, "PODE SEGUIR!", 0, 0);
            break;

        case YELLOW_LIGHT:
            ssd1306_draw_string(&ssd, "ATENCAO!", 0, 0);
            break;

        case RED_LIGHT:
            ssd1306_draw_string(&ssd, "ESPERE!", 0, 0);
            break;

        case NIGHT_MODE:
            ssd1306_draw_string(&ssd, "ATENCAO!", 0, 0);
            break;

        default:
            ssd1306_draw_string(&ssd, "---", 0, 0);
            break;
        }

        ssd1306_send_data(&ssd); // ATUALIZA A TELA

        // Atualiza animação só no modo verde
        if (light_state == GREEN_LIGHT)
        {
            arm_rotate += arm_factor;
            leg_rotate += leg_factor;

            if (arm_rotate >= 45 || arm_rotate <= 0)
                arm_factor *= -1;
            if (leg_rotate >= 45 || leg_rotate <= 0)
                leg_factor *= -1;
        }

        // EVITA USO EXCESSIVO DA CPU
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
    }
}

int main()
{

    stdio_init_all();
    // inicializa a PIO
    PIO_setup(&pio, &sm);
    // inicializa o display
    ssd_setup();

    // REGISTRO DAS TASKS
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