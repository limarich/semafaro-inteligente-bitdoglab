#include "leds.h"

// Rotina para definição da intensidade de cores do LED
uint32_t matrix_rgb(uint r, uint g, uint b, float intensity)
{
    uint8_t R = (uint8_t)(r * intensity);
    uint8_t G = (uint8_t)(g * intensity);
    uint8_t B = (uint8_t)(b * intensity);
    return (G << 24) | (R << 16) | (B << 8);
}

// Rotina para acionar a matriz de LEDs - WS2812B
void draw_pio(pixel *draw, PIO pio, uint sm)
{
    for (int16_t i = 0; i < PIXELS; i++)
    {
        uint32_t valor_led = matrix_rgb(draw[i].red, draw[i].green, draw[i].blue, draw[i].intensity);
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}

void test_matrix(PIO pio, uint sm)
{
    frame test_frame, black_frame;

    pixel red = {255, 0, 0, 1}, black = {0, 0, 0, 1};

    for (int16_t i = 0; i < PIXELS; i++)
    {
        test_frame[i] = red;
        black_frame[i] = black;
        draw_pio(test_frame, pio, sm);
        sleep_ms(50);
    }
    draw_pio(black_frame, pio, sm);
    sleep_ms(50);
}

pixel handle_color(color_options color, float intensity)
{
    pixel pixel_color = {0, 0, 0};
    switch (color)
    {
    case BLACK:
        pixel_color.red = 0;
        pixel_color.green = 0;
        pixel_color.blue = 0;
        pixel_color.intensity = intensity;
        break;
    case BROWN:
        pixel_color.red = 165;
        pixel_color.green = 25;
        pixel_color.blue = 0;
        pixel_color.intensity = intensity;
        break;
    case RED:
        pixel_color.red = 255;
        pixel_color.green = 0;
        pixel_color.blue = 0;
        pixel_color.intensity = intensity;
        break;
    case ORANGE:
        pixel_color.red = 205;
        pixel_color.green = 43;
        pixel_color.blue = 0;
        pixel_color.intensity = intensity;
        break;
    case YELLOW:
        pixel_color.red = 255;
        pixel_color.green = 255;
        pixel_color.blue = 0;
        pixel_color.intensity = intensity;
        break;
    case GREEN:
        pixel_color.red = 0;
        pixel_color.green = 255;
        pixel_color.blue = 0;
        pixel_color.intensity = intensity;
        break;
    case BLUE:
        pixel_color.red = 0;
        pixel_color.green = 0;
        pixel_color.blue = 255;
        pixel_color.intensity = intensity;
        break;
    case VIOLET:
        pixel_color.red = 121;
        pixel_color.green = 8;
        pixel_color.blue = 205;
        pixel_color.intensity = intensity;
        break;
    case GRAY:
        pixel_color.red = 55;
        pixel_color.green = 55;
        pixel_color.blue = 55;
        pixel_color.intensity = intensity;
        break;
    case WHITE:
        pixel_color.red = 255;
        pixel_color.green = 255;
        pixel_color.blue = 255;
        pixel_color.intensity = intensity;
        break;
    default:
        break;
    }

    return pixel_color;
}

void draw_traffic_light(PIO pio, uint sm, color_options color, bool night_mode)
{

    pixel pixel_color = handle_color(color, 1);
    pixel black = handle_color(BLACK, 1);
    pixel gray = handle_color(GRAY, 0.05f);
    pixel red = handle_color(RED, 0.01f);
    pixel green = handle_color(GREEN, 0.01f);
    pixel yellow = handle_color(YELLOW, 0.01f);

    frame matrix = {
        black, gray, gray, gray, black,
        black, gray, green, gray, black,
        black, gray, yellow, gray, black,
        black, gray, red, gray, black,
        black, gray, gray, gray, black};

    if (night_mode)
    { // se estiver no modo noturno mantem o led vermelho e verde apagado
        matrix[17].intensity = 0;
        matrix[7].intensity = 0;
    }

    if (color == RED)
    {
        matrix[17].intensity = 1;
    }
    else if (color == GREEN)
    {
        matrix[7].intensity = 1;
    }
    else if (color == YELLOW)
    {
        matrix[12].intensity = 1;
    }
    else if (color == BLACK)
    {
        matrix[17] = black;
        matrix[7] = black;
        matrix[12] = black;
    }

    draw_pio(matrix, pio, sm);
}