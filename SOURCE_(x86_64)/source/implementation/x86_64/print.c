//ILLUSION OS FILE
#include "print.h"
#include "comandos.h"

const static size_t NUM_COLS = 80;
const static size_t NUM_ROWS = 25;

struct Char {
    uint8_t character;
    uint8_t color;
};

struct Char* buffer = (struct Char*) 0xb8000;
size_t col = 0;
size_t row = 0;
uint8_t color = PRINT_COLOR_WHITE | PRINT_COLOR_BLACK << 4;

void clear_row(size_t row) {
    struct Char empty = (struct Char) {
        character: ' ',
        color: color,
    };

    for (size_t col = 0; col < NUM_COLS; col++) {
        buffer[col + NUM_COLS * row] = empty;
    }
}

void print_clear() {
    for (size_t i = 0; i < NUM_ROWS; i++) {
        clear_row(i);
    }
}

void print_newline() {
    col = 0;

    if (row < NUM_ROWS - 1) {
        row++;
        return;
    }

    for (size_t row = 1; row < NUM_ROWS; row++) {
        for (size_t col = 0; col < NUM_COLS; col++) {
            struct Char character = buffer[col + NUM_COLS * row];
            buffer[col + NUM_COLS * (row - 1)] = character;
        }
    }

    clear_row(NUM_ROWS - 1);
}

void print_char(char character) {
    if (character == '\n') {
        print_newline();
        return;
    }

    if (col > NUM_COLS) {
        print_newline();
    }

    buffer[col + NUM_COLS * row] = (struct Char) {
        character: (uint8_t) character,
        color: color,
    };

    col++;
}

void print_str(char* str) {
    for (size_t i = 0; 1; i++) {
        char character = (uint8_t) str[i];

        if (character == '\0') {
            return;
        }

        print_char(character);
    }
}

/*
void get_str(char comando) { //verifica os comandos na variável "comando"
    if (comando == "ver") {
        print_char("versao 1.0");
        return;
    }
}
*/

void print_set_color(uint8_t foreground, uint8_t background) {
    color = foreground + (background << 4);
}

void print_uint64_dec(uint64_t value) {
    if (value == 0) {
        print_char('0');
        return;
    }
    
    char buffer[20];
    int i = 0;
    
    while (value > 0) {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    }
    
    while (i-- > 0) {
        print_char(buffer[i]);
    }
}

void print_uint64_hex(uint64_t value) {
    if (value == 0) {
        print_char('0');
        return;
    }
    
    char buffer[16];
    int i = 0;
    
    while (value > 0) {
        uint8_t digit = value & 0xF;
        
        if (digit < 10) {
            buffer[i++] = digit + '0';
        } else {
            buffer[i++] = digit - 10 + 'A';
        }
        
        value >>= 4;
    }
    
    while (i-- > 0) {
        print_char(buffer[i]);
    }
}

void print_uint64_bin(uint64_t value) {
    char buffer[64];
    
    for (size_t i = 0; i < 64; i++) {
        buffer[i] = (value & 1) + '0';
        value >>= 1;
    }
    
    for (size_t i = 64; i > 0; i--) {
        print_char(buffer[i - 1]);
    }
}

/*
void print_backspace() {
    // 1. Volta uma coluna
    col--;


    // 2. LÓGICA DE QUEBRA DE LINHA INVERSA (Wraparound Inverso)
    if (col < 0) {
        // Se a coluna é negativa, move para o final da linha anterior
        col = NUM_COLS - 1; 

        // Sobe uma linha
        row--; 

        // Proteção contra scroll up (não deixe a linha ir abaixo de zero)
        if (row < 0) {
            row = 0;
            col = 0;
            return;
        }
    }  
    buffer[col + NUM_COLS * row] = (struct Char) {
        character: ' ',
        color: color, // Usa a cor de fundo atual para "apagar"
    };
}
*/


void backspace() {
    // 1. Verificar se o cursor está no início (coluna 0). 
    //    Se for 0, não fazemos nada (não voltamos para a linha anterior por simplicidade).
    if (col > 0) {
        // 2. Mover o cursor para trás (lógica)
        col--;
        
        // 3. Sobrescrever o caractere anterior com um espaço em branco
        //    A cor do espaço é a cor de fundo atual, o que faz parecer que a letra "sumiu".
        buffer[col + NUM_COLS * row] = (struct Char) {
            character: ' ',
            color: color,
        };
        
        // **IMPORTANTE**: Como você não tem uma função para atualizar o cursor físico 
        // (aquele que pisca) usando as portas de I/O, o cursor pisca o caractere de trás. 
        // Seu `print_char` também não faz isso, mas em um OS real, você precisaria 
        // adicionar uma função que atualiza o cursor de hardware aqui e em `print_char`.
    } 
    // Note: Não há um "else if (col == 0) { row--; col = NUM_COLS - 1; }"
    // para simplificar e evitar lógica de rolagem complexa no backspace.
}

/*
void shell_disable_cursor() {
    outportb(0x3D4, 10);
    outportb(0x3D5, 32);
}
*/

