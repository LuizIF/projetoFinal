#include <stdio.h>             // Biblioteca padrão para entrada e saída, utilizada para printf.
#include "pico/stdlib.h"       // Biblioteca padrão para funções básicas do Pico, como GPIO e temporização.
#include "hardware/adc.h"      // Biblioteca para controle do ADC (Conversor Analógico-Digital).
#include "hardware/spi.h"      // Biblioteca para comunicação SPI (Serial Peripheral Interface).
#include "hardware/i2c.h"      // Biblioteca para comunicação I2C (Inter-Integrated Circuit).
#include "hardware/dma.h"      // Biblioteca para uso do controlador DMA (Direct Memory Access).
#include "hardware/pio.h"      // Biblioteca para controle de periféricos programáveis (PIO).
#include "hardware/interp.h"   // Biblioteca para uso dos interpoladores de hardware.
#include "hardware/timer.h"    // Biblioteca para controle de temporizadores e delays.
#include "hardware/watchdog.h" // Biblioteca para uso do watchdog timer, garantindo reinicialização em caso de falha.
#include "hardware/uart.h"     // Biblioteca para comunicação serial UART (Universal Asynchronous Receiver-Transmitter).

// Definições dos pinos para o joystick e botão
#define VRX_PIN 26    // Define o pino GP26 para o eixo X do joystick (Canal ADC0).
#define VRY_PIN 27    // Define o pino GP27 para o eixo Y do joystick (Canal ADC1).
#define SW_PIN 22     // Define o pino GP22 para o botão do joystick (entrada digital).

int main() {
    // Inicializa a comunicação serial para permitir o uso de printf.
    // Isso permite enviar mensagens para o console via USB, facilitando a depuração.
    stdio_init_all();

    // Inicializa o módulo ADC do Raspberry Pi Pico.
    // Isso prepara o ADC para ler valores dos pinos analógicos.
    adc_init();

    // Configura o pino GP26 para leitura analógica do ADC.
    // O pino GP26 está mapeado para o canal 0 do ADC, que será usado para ler o eixo X do joystick.
    adc_gpio_init(VRX_PIN);

    // Configura o pino GP27 para leitura analógica do ADC.
    // O pino GP27 está mapeado para o canal 1 do ADC, que será usado para ler o eixo Y do joystick.
    adc_gpio_init(VRY_PIN);

    // Configura o pino do botão como entrada digital com pull-up interno.
    // O pull-up garante que o pino leia "alto" quando o botão não está pressionado, evitando leituras instáveis.
    gpio_init(SW_PIN);
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_pull_up(SW_PIN);

    // Loop infinito para ler continuamente os valores do joystick e do botão.
    while (true) {
        // Seleciona o canal 0 do ADC (pino GP26) para leitura.
        // Esse canal corresponde ao eixo X do joystick (VRX).
        adc_select_input(0);
        uint16_t vrx_value = adc_read(); // Lê o valor do eixo X, de 0 a 4095.

        // Seleciona o canal 1 do ADC (pino GP27) para leitura.
        // Esse canal corresponde ao eixo Y do joystick (VRY).
        adc_select_input(1);
        uint16_t vry_value = adc_read(); // Lê o valor do eixo Y, de 0 a 4095.

        // Lê o estado do botão do joystick (SW).
        // O valor lido será 0 se o botão estiver pressionado e 1 se não estiver.
        bool sw_value = gpio_get(SW_PIN) == 0; // 0 indica que o botão está pressionado.

        // Imprime os valores lidos na comunicação serial.
        // VRX e VRY mostram a posição do joystick, enquanto SW mostra o estado do botão.
        printf("VRX: %u, VRY: %u, SW: %d\n", vrx_value, vry_value, sw_value);

        // Introduz um atraso de 500 milissegundos antes de repetir a leitura.
        // Isso evita que as leituras e impressões sejam feitas muito rapidamente.
        sleep_ms(500);
    }

    // Retorna 0 indicando que o programa terminou com sucesso.
    // Esse ponto nunca será alcançado, pois o loop é infinito.
    return 0;
}