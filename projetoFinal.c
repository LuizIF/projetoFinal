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

//Configurações da biblioteca do display

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/binary_info.h"
#include "raspberry26x32.h"
#include "ssd1306_font.h"


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



//Display//Display//Display//Display//Display//Display//Display//Display
//Abaixo: configurações do display

// Define the size of the display we have attached. This can vary, make sure you
// have the right size defined or the output will look rather odd!
// Code has been tested on 128x32 and 128x64 OLED displays
#define SSD1306_HEIGHT              32
#define SSD1306_WIDTH               128

#define SSD1306_I2C_ADDR            _u(0x3C)

// 400 is usual, but often these can be overclocked to improve display response.
// Tested at 1000 on both 32 and 84 pixel height devices and it worked.
#define SSD1306_I2C_CLK             400
//#define SSD1306_I2C_CLK             1000


// commands (see datasheet)
#define SSD1306_SET_MEM_MODE        _u(0x20)
#define SSD1306_SET_COL_ADDR        _u(0x21)
#define SSD1306_SET_PAGE_ADDR       _u(0x22)
#define SSD1306_SET_HORIZ_SCROLL    _u(0x26)
#define SSD1306_SET_SCROLL          _u(0x2E)

#define SSD1306_SET_DISP_START_LINE _u(0x40)

#define SSD1306_SET_CONTRAST        _u(0x81)
#define SSD1306_SET_CHARGE_PUMP     _u(0x8D)

#define SSD1306_SET_SEG_REMAP       _u(0xA0)
#define SSD1306_SET_ENTIRE_ON       _u(0xA4)
#define SSD1306_SET_ALL_ON          _u(0xA5)
#define SSD1306_SET_NORM_DISP       _u(0xA6)
#define SSD1306_SET_INV_DISP        _u(0xA7)
#define SSD1306_SET_MUX_RATIO       _u(0xA8)
#define SSD1306_SET_DISP            _u(0xAE)
#define SSD1306_SET_COM_OUT_DIR     _u(0xC0)
#define SSD1306_SET_COM_OUT_DIR_FLIP _u(0xC0)

#define SSD1306_SET_DISP_OFFSET     _u(0xD3)
#define SSD1306_SET_DISP_CLK_DIV    _u(0xD5)
#define SSD1306_SET_PRECHARGE       _u(0xD9)
#define SSD1306_SET_COM_PIN_CFG     _u(0xDA)
#define SSD1306_SET_VCOM_DESEL      _u(0xDB)

#define SSD1306_PAGE_HEIGHT         _u(8)
#define SSD1306_NUM_PAGES           (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN             (SSD1306_NUM_PAGES * SSD1306_WIDTH)

#define SSD1306_WRITE_MODE         _u(0xFE)
#define SSD1306_READ_MODE          _u(0xFF)

struct render_area {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;

    int buflen;
};

void calc_render_area_buflen(struct render_area *area) {
    // calculate how long the flattened buffer will be for a render area
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

#ifdef i2c_default

void SSD1306_send_cmd(uint8_t cmd) {
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command
    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(i2c_default, SSD1306_I2C_ADDR, buf, 2, false);
}

void SSD1306_send_cmd_list(uint8_t *buf, int num) {
    for (int i=0;i<num;i++)
        SSD1306_send_cmd(buf[i]);
}

void SSD1306_send_buf(uint8_t buf[], int buflen) {
    // in horizontal addressing mode, the column address pointer auto-increments
    // and then wraps around to the next page, so we can send the entire frame
    // buffer in one gooooooo!

    // copy our frame buffer into a new buffer because we need to add the control byte
    // to the beginning

    uint8_t *temp_buf = malloc(buflen + 1);

    temp_buf[0] = 0x40;
    memcpy(temp_buf+1, buf, buflen);

    i2c_write_blocking(i2c_default, SSD1306_I2C_ADDR, temp_buf, buflen + 1, false);

    free(temp_buf);
}

void SSD1306_init() {
    // Some of these commands are not strictly necessary as the reset
    // process defaults to some of these but they are shown here
    // to demonstrate what the initialization sequence looks like
    // Some configuration values are recommended by the board manufacturer

    uint8_t cmds[] = {
        SSD1306_SET_DISP,               // set display off
        /* memory mapping */
        SSD1306_SET_MEM_MODE,           // set memory address mode 0 = horizontal, 1 = vertical, 2 = page
        0x00,                           // horizontal addressing mode
        /* resolution and layout */
        SSD1306_SET_DISP_START_LINE,    // set display start line to 0
        SSD1306_SET_SEG_REMAP | 0x01,   // set segment re-map, column address 127 is mapped to SEG0
        SSD1306_SET_MUX_RATIO,          // set multiplex ratio
        SSD1306_HEIGHT - 1,             // Display height - 1
        SSD1306_SET_COM_OUT_DIR | 0x08, // set COM (common) output scan direction. Scan from bottom up, COM[N-1] to COM0
        SSD1306_SET_DISP_OFFSET,        // set display offset
        0x00,                           // no offset
        SSD1306_SET_COM_PIN_CFG,        // set COM (common) pins hardware configuration. Board specific magic number.
                                        // 0x02 Works for 128x32, 0x12 Possibly works for 128x64. Other options 0x22, 0x32
#if ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 32))
        0x02,
#elif ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 64))
        0x12,
#else
        0x02,
#endif
        /* timing and driving scheme */
        SSD1306_SET_DISP_CLK_DIV,       // set display clock divide ratio
        0x80,                           // div ratio of 1, standard freq
        SSD1306_SET_PRECHARGE,          // set pre-charge period
        0xF1,                           // Vcc internally generated on our board
        SSD1306_SET_VCOM_DESEL,         // set VCOMH deselect level
        0x30,                           // 0.83xVcc
        /* display */
        SSD1306_SET_CONTRAST,           // set contrast control
        0xFF,
        SSD1306_SET_ENTIRE_ON,          // set entire display on to follow RAM content
        SSD1306_SET_NORM_DISP,           // set normal (not inverted) display
        SSD1306_SET_CHARGE_PUMP,        // set charge pump
        0x14,                           // Vcc internally generated on our board
        SSD1306_SET_SCROLL | 0x00,      // deactivate horizontal scrolling if set. This is necessary as memory writes will corrupt if scrolling was enabled
        SSD1306_SET_DISP | 0x01, // turn display on
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void SSD1306_scroll(bool on) {
    // configure horizontal scrolling
    uint8_t cmds[] = {
        SSD1306_SET_HORIZ_SCROLL | 0x00,
        0x00, // dummy byte
        0x00, // start page 0
        0x00, // time interval
        0x03, // end page 3 SSD1306_NUM_PAGES ??
        0x00, // dummy byte
        0xFF, // dummy byte
        SSD1306_SET_SCROLL | (on ? 0x01 : 0) // Start/stop scrolling
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void render(uint8_t *buf, struct render_area *area) {
    // update a portion of the display with a render area
    uint8_t cmds[] = {
        SSD1306_SET_COL_ADDR,
        area->start_col,
        area->end_col,
        SSD1306_SET_PAGE_ADDR,
        area->start_page,
        area->end_page
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
    SSD1306_send_buf(buf, area->buflen);
}

static void SetPixel(uint8_t *buf, int x,int y, bool on) {
    assert(x >= 0 && x < SSD1306_WIDTH && y >=0 && y < SSD1306_HEIGHT);

    const int BytesPerRow = SSD1306_WIDTH ; // x pixels, 1bpp, but each row is 8 pixel high, so (x / 8) * 8

    int byte_idx = (y / 8) * BytesPerRow + x;
    uint8_t byte = buf[byte_idx];

    if (on)
        byte |=  1 << (y % 8);
    else
        byte &= ~(1 << (y % 8));

    buf[byte_idx] = byte;
}
// Basic Bresenhams.
static void DrawLine(uint8_t *buf, int x0, int y0, int x1, int y1, bool on) {

    int dx =  abs(x1-x0);
    int sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0);
    int sy = y0<y1 ? 1 : -1;
    int err = dx+dy;
    int e2;

    while (true) {
        SetPixel(buf, x0, y0, on);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2*err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static inline int GetFontIndex(uint8_t ch) {
    if (ch >= 'A' && ch <='Z') {
        return  ch - 'A' + 1;
    }
    else if (ch >= '0' && ch <='9') {
        return  ch - '0' + 27;
    }
    else return  0; // Not got that char so space.
}

static void WriteChar(uint8_t *buf, int16_t x, int16_t y, uint8_t ch) {
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8)
        return;

    // For the moment, only write on Y row boundaries (every 8 vertical pixels)
    y = y/8;

    ch = toupper(ch);
    int idx = GetFontIndex(ch);
    int fb_idx = y * 128 + x;

    for (int i=0;i<8;i++) {
        buf[fb_idx++] = font[idx * 8 + i];
    }
}

static void WriteString(uint8_t *buf, int16_t x, int16_t y, char *str) {
    // Cull out any string off the screen
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8)
        return;

    while (*str) {
        WriteChar(buf, x, y, *str++);
        x+=8;
    }
}

#endif


void MensagemDisplay(float temperatura, float umidade, float destilado, float reservatorio) { 
    char buffer[4][15];  // Array para armazenar as strings formatadas

    // Formatando os textos das variáveis para exibição
    snprintf(buffer[0], sizeof(buffer[0]), "TEMP: %.2f C", temperatura);
    snprintf(buffer[1], sizeof(buffer[1]), "UMID: %.2f ", umidade);
    snprintf(buffer[2], sizeof(buffer[2]), "DEST: %.2f L", destilado);
    snprintf(buffer[3], sizeof(buffer[3]), "RESE: %.2f L", reservatorio);

    // Define a área de renderização
    struct render_area frame_area = {
        .start_col = 0,
        .end_col = SSD1306_WIDTH - 1,
        .start_page = 0,
        .end_page = SSD1306_NUM_PAGES - 1
    };

    calc_render_area_buflen(&frame_area);

    // Zera o buffer da tela
    uint8_t buf[SSD1306_BUF_LEN];
    memset(buf, 0, SSD1306_BUF_LEN);
    render(buf, &frame_area);

    // Escreve as mensagens na tela, uma por linha (cada linha tem 8px de altura)
    int y = 0;
    for (int i = 0; i < 4; i++) {
        WriteString(buf, 5, y, buffer[i]);
        y += 8;  // Pula para a próxima linha
    }

    // Atualiza o display
    render(buf, &frame_area);
}

//Acima: configurações do display
//Display//Display//Display//Display//Display//Display//Display//Display

// Função de callback que será chamada a cada intervalo definido pelo temporizador.
// Esta função alterna o estado do LED e imprime uma mensagem na saída serial.
bool repeating_timer_callback(struct repeating_timer *t) {
    
    if(!limite){
        // Imprime na saída serial o valor atual de temperatura e umidade
        printf("\nTEMP: %.2f, UMID: %.2f, DEST: %.2f, RESE: %.2f\n", temperatura, umidade, destilado, reservatorio);
    }

    //exibeMensagemDisplay();
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
    stdio_init_all();
    
        //Definições da biblioteca
    #if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning i2c / SSD1306_i2d example requires a board with I2C pins
        puts("Default I2C pins were not defined");
    #else
    // useful information for picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    bi_decl(bi_program_description("SSD1306 OLED driver I2C example for the Raspberry Pi Pico"));

    printf("Hello, SSD1306 OLED display! Look at my raspberries..\n");

    // I2C is "open drain", pull ups to keep signal high when no data is being
    // sent
    
    i2c_init(i2c_default, SSD1306_I2C_CLK * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // run through the complete initialization process
    SSD1306_init();
   

    #endif

//////////////Fim da inicialização do display


//////////////Início do código de controle

    //Imprime no display os valores iniciais das grandezas
    MensagemDisplay(temperatura, umidade, destilado, reservatorio);

    // Inicializa a comunicação serial para permitir o uso de printf.
    // Isso permite enviar mensagens para o console via USB, facilitando a depuração.
    //stdio_init_all();    

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
            //printf("TEMP aumentou em: %.2f C\n", random_value);  // Imprime o valor aleatório para comparação
            temperatura = temperatura+random_value;//adiciona a temperatura + o valor randômico gerado pela float random_float()
            //printf("TEMP: %.2f, UMID: %.2f\n\n", temperatura, umidade); // Imprime os valores de temperatura e umidade          
            MensagemDisplay(temperatura, umidade, destilado, reservatorio);
        }
        else if(vrx_value<1000){
            float random_value = random_float(); //gera um valor aleatório para decrementar o valor da temperatura
            //printf("TEMP diminuiu em: %.2f C\n", random_value);  // Imprime o valor aleatório para comparação 
            temperatura = temperatura-random_value;//subtrai da temperatura o valor randômico gerado pela float random_float()
            //printf("TEMP: %.2f, UMID: %.2f\n\n", temperatura, umidade); // Imprime os valores de temperatura e umidade           
            MensagemDisplay(temperatura, umidade, destilado, reservatorio);
        }
        else if(vry_value>3000){
            float random_value = random_float(); //gera um valor aleatório para incrementar o valor da umidade
            //printf("UMID aumentou em: %.2f g/m³\n", random_value); // Imprime o valor aleatório para comparação  
            umidade = umidade+random_value;//adiciona a umidade o valor randômico gerado pela float random_float()
            //printf("TEMP: %.2f, UMID: %.2f\n\n", temperatura, umidade); // Imprime os valores de temperatura e umidade
            MensagemDisplay(temperatura, umidade, destilado, reservatorio);
        }
        else if(vry_value<1000){
            float random_value = random_float(); //gera um valor aleatório para incrementar o valor da umidade
            //printf("UMID diminuiu em: %.2f g/m³\n", random_value);  // Imprime o valor aleatório para comparação 
            umidade = umidade-random_value;//subtrai da umidade o valor randômico gerado pela float random_float()
            //printf("TEMP: %.2f, UMID: %.2f\n\n", temperatura, umidade); // Imprime os valores de temperatura e umidade
            MensagemDisplay(temperatura, umidade, destilado, reservatorio);
        }
        else if (sw_value) { // Verifica se o botão SW foi pressionado
            printf("\nBotão SW foi acionado. destilado e reservatorio resetados\n");  // Imprime botão SW 
            destilado = 0;
            reservatorio = 1000; 
            limite = false;
            count = 0; 
            MensagemDisplay(temperatura, umidade, destilado, reservatorio);          
        }
        else if (button_A_value) { // Verifica se o botão A foi pressionado
            float random_value = random_float(); //gera um valor aleatório para incrementar o valor da umidade
            //printf("\nBotão A foi acionado. Volume de destilado aumentou.\n");  // Imprime botão A
            destilado = destilado+random_value*195;
            MensagemDisplay(temperatura, umidade, destilado, reservatorio);                              
        }
        else if (button_B_value) { // Verifica se o botão B foi pressionado
            float random_value = random_float(); //gera um valor aleatório para incrementar o valor da umidade
            //printf("\nBotão B foi acionado. Volume de reservatorio diminuiu.\n");  // Imprime botão A
            reservatorio = reservatorio-random_value*195; 
            MensagemDisplay(temperatura, umidade, destilado, reservatorio);                                     
        }

        if ((destilado >= 1000) && (count<3)){
                printf("\nATENÇÃO! Esvaziar água destilada.\n");
                destilado = 1000;
                limite = true;
                count++;
                sleep_ms(1000);
                MensagemDisplay(temperatura, umidade, destilado, reservatorio);
        } 
        if ((reservatorio <= 0) && (count<3)){
                printf("\nATENÇÃO! Encher reservatório.\n");
                reservatorio = 0;
                limite = true;
                count++;
                sleep_ms(1000);
                MensagemDisplay(temperatura, umidade, destilado, reservatorio);
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
