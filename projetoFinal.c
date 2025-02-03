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

#define LED_PIN_A 11      // Pino GPIO usado para o LED verde
#define LED_PIN_B 12      // Pino GPIO usado para o LED azul
#define LED_PIN_C 13      // Pino GPIO usado para o LED vermelho

// Define os pinos dos botões
#define BUTTON_A_PIN 5  // Pino GPIO usado para o botão A.
#define BUTTON_B_PIN 6  // Pino GPIO usado para o botão B.

float (temperatura)= 30; //Variável com valor inicial para a temperatura.
float (umidade) = 89; //Variável com valor inicial para a umidade.
float (destilado) = 0; //Variável com valor inicial para o nível de água destilada.
float (reservatorio) = 1000; //Variável com valor inicial para o nível de água do reservatório de água para destilar.

int count = 0; //Para contar quantas vezes a mensagem de atenção foi impressa.

bool limite = false; //Ativa desativa impressão dos dados das grandezas.
// Função de callback que será chamada a cada intervalo definido pelo temporizador.
// Esta função alterna o estado do LED e imprime uma mensagem na saída serial.
bool repeating_timer_callback(struct repeating_timer *t) {
    
    if(!limite){
        // Imprime na saída serial o valor atual de temperatura e umidade
        printf("\nTEMP: %.2f, UMID: %.2f, DEST: %.2f, RESE: %.2f\n", temperatura, umidade, destilado, reservatorio);
    }
    // Retorna true para manter o temporizador ativo
    return true;
}

// Função para gerar um número aleatório entre 0 e 1
//Para gerar incrementos e decrementos aleatórios para temperatura e umidade.
float random_float(){
    adc_select_input(4);  // Usa o canal 4 do ADC (pino flutuante)
    uint32_t raw = adc_read();  // Lê o valor do ADC
    uint32_t entropy = raw ^ time_us_32();  // Mistura com o tempo do sistema
    return (float)(entropy & 0xFFF) / 4095.0f;  // Normaliza para [0.0, 1.0]
};

int main() {
    
    // Inicializa a comunicação serial para permitir o uso de printf.
    // Isso permite enviar mensagens para o console via USB, facilitando a depuração.
    stdio_init_all();       

    // Declara uma estrutura para armazenar informações sobre o temporizador repetitivo.
    struct repeating_timer timer;

    // Configura um temporizador repetitivo que chama a função 'repeating_timer_callback' a cada 5 segundo (5000 ms).
    // Parâmetros:
    // 5000: Intervalo de tempo em milissegundos (5 segundos).
    // repeating_timer_callback: Função de callback que será chamada a cada intervalo.
    // NULL: Dados adicionais que podem ser passados para a função de callback (não utilizado aqui).
    // &timer: Ponteiro para a estrutura que armazenará informações sobre o temporizador.
    add_repeating_timer_ms(5000, repeating_timer_callback, NULL, &timer);
    

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

    // Configuração do botão A
    gpio_init(BUTTON_A_PIN); // Inicializa o pino do botão A
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN); // Configura o pino como entrada
    gpio_pull_up(BUTTON_A_PIN); // Habilita o resistor de pull-up interno

    // Configuração do botão B
    gpio_init(BUTTON_B_PIN); // Inicializa o pino do botão B
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN); // Configura o pino como entrada
    gpio_pull_up(BUTTON_B_PIN); // Habilita o resistor de pull-up interno   

    // Configuração do LED
    gpio_init(LED_PIN_A); // Inicializa o pino do LED
    gpio_set_dir(LED_PIN_A, GPIO_OUT); // Configura o pino como saída 

    gpio_init(LED_PIN_B); // Inicializa o pino do LED
    gpio_set_dir(LED_PIN_B, GPIO_OUT); // Configura o pino como saída   

    gpio_init(LED_PIN_C); // Inicializa o pino do LED
    gpio_set_dir(LED_PIN_C, GPIO_OUT); // Configura o pino como saída  

    // Loop principal do programa.
    // Como o temporizador está gerenciando o controle do LED, o loop principal fica livre para outras tarefas.
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
        
        // Lê o estado do botão A
        // O valor lido será 0 se o botão estiver pressionado e 1 se não estiver.
        bool button_A_value = gpio_get(BUTTON_A_PIN) == 0; // 0 indica que o botão está pressionado.
        // Lê o estado do botão B
        // O valor lido será 0 se o botão estiver pressionado e 1 se não estiver.
        bool button_B_value = gpio_get(BUTTON_B_PIN) == 0; // 0 indica que o botão está pressionado.

        
        // VRX e VRY mostram a posição do joystick, enquanto SW mostra o estado do botão.
        //printf("VRX: %u, VRY: %u, SW: %d\n", vrx_value, vry_value, sw_value);
              

        if(vrx_value>3000){
            float random_value = random_float(); //gera um valor aleatório para incrementar o valor da temperatura
            printf("TEMP aumentou em: %.2f C\n", random_value);  // Imprime o valor aleatório para comparação
            temperatura = temperatura+random_value;//adiciona a temperatura + o valor randômico gerado pela float random_float()
            //printf("TEMP: %.2f, UMID: %.2f\n\n", temperatura, umidade); // Imprime os valores de temperatura e umidade          
        }
        else if(vrx_value<1000){
            float random_value = random_float(); //gera um valor aleatório para decrementar o valor da temperatura
            printf("TEMP diminuiu em: %.2f C\n", random_value);  // Imprime o valor aleatório para comparação 
            temperatura = temperatura-random_value;//subtrai da temperatura o valor randômico gerado pela float random_float()
            //printf("TEMP: %.2f, UMID: %.2f\n\n", temperatura, umidade); // Imprime os valores de temperatura e umidade           
        }
        else if(vry_value>3000){
            float random_value = random_float(); //gera um valor aleatório para incrementar o valor da umidade
            printf("UMID aumentou em: %.2f g/m³\n", random_value); // Imprime o valor aleatório para comparação  
            umidade = umidade+random_value;//adiciona a umidade o valor randômico gerado pela float random_float()
            //printf("TEMP: %.2f, UMID: %.2f\n\n", temperatura, umidade); // Imprime os valores de temperatura e umidade
        }
        else if(vry_value<1000){
            float random_value = random_float(); //gera um valor aleatório para incrementar o valor da umidade
            printf("UMID diminuiu em: %.2f g/m³\n", random_value);  // Imprime o valor aleatório para comparação 
            umidade = umidade-random_value;//subtrai da umidade o valor randômico gerado pela float random_float()
            //printf("TEMP: %.2f, UMID: %.2f\n\n", temperatura, umidade); // Imprime os valores de temperatura e umidade
        }
        else if (sw_value) { // Verifica se o botão SW foi pressionado
            printf("\nBotão SW foi acionado. destilado e reservatorio resetados\n");  // Imprime botão SW 
            destilado = 0;
            reservatorio = 1000; 
            limite = false;
            count = 0;           
        }
        else if (button_A_value) { // Verifica se o botão A foi pressionado
            float random_value = random_float(); //gera um valor aleatório para incrementar o valor da umidade
            printf("\nBotão A foi acionado. Volume de destilado aumentou.\n");  // Imprime botão A
            destilado = destilado+random_value*195;                              
        }
        else if (button_B_value) { // Verifica se o botão B foi pressionado
            float random_value = random_float(); //gera um valor aleatório para incrementar o valor da umidade
            printf("\nBotão B foi acionado. Volume de reservatorio diminuiu.\n");  // Imprime botão A
            reservatorio = reservatorio-random_value*195;                                      
        }

        if ((destilado >= 1000) && (count<3)){
                printf("\nATENÇÃO! Esvaziar água destilada.\n");
                destilado = 1001;
                limite = true;
                count++;
                sleep_ms(1000);
        } 
        if ((reservatorio <= 0) && (count<3)){
                printf("\nATENÇÃO! Encher reservatório.\n");
                reservatorio = 0;
                limite = true;
                count++;
                sleep_ms(1000);
        } 

        if ((destilado <= 600) && (reservatorio >= 400)) {
            gpio_put(LED_PIN_A, 1); // LED verde aceso
            gpio_put(LED_PIN_B, 0);
            gpio_put(LED_PIN_C, 0); // LED vermelho apagado
        }
        else if ((destilado > 800) || (reservatorio <200)){// || ((float)reservatorio < 200)) {
            gpio_put(LED_PIN_A, 0); // LED verde apagado
            gpio_put(LED_PIN_B, 0);
            gpio_put(LED_PIN_C, 1); // LED vermelho aceso
        } 
        else { // Caso intermediário (600 < destilado ≤ 800 e 200 ≤ reservatorio < 400)
            gpio_put(LED_PIN_A, 1); // LED verde aceso
            gpio_put(LED_PIN_B, 0);
            gpio_put(LED_PIN_C, 1); // LED vermelho aceso (forma amarelo)
        }
        


        // Introduz um atraso de 300 milissegundos antes de repetir a leitura.
        // Isso evita que as leituras e impressões sejam feitas muito rapidamente.
        sleep_ms(300);
    }

    // Retorna 0 indicando que o programa terminou com sucesso.
    // Esse ponto nunca será alcançado, pois o loop é infinito.
    return 0;
}



