#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>

uint16_t check_key() {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    memset(&timeout, 0, sizeof(struct timeval));
    return select(1, &readfds, NULL, NULL, &timeout);
}

namespace Lc3 {
    uint16_t sign_extend(uint16_t x, int bit_count) {
        if ((x >> (bit_count - 1)) & 1)
            x |= (0xFFFF << bit_count);
        return x;
    }

    uint16_t swap16(uint16_t x) { return (x << 8) | (x >> 8); }

    enum Reg { R0=0, R7=7, PC, CF, RC, KBSR=0xFE00, KBDR=0xFE02 };

    uint16_t reg[RC];
    uint16_t memory[UINT16_MAX];
    bool running;

    void update_flags(int r0) {
        if (reg[r0] == 0) {
            reg[CF] = 1 << 1; // ZERO
        } else if ((reg[r0] >> 15) & 0x1) {
            reg[CF] = 1 << 2; // NEG
        } else {
            reg[CF] = 1 << 0; // POS
        }  
    }

    void trap(uint16_t code) {
        switch (code) {
            case 0x20: // GETC
                reg[R0] = (uint16_t)getchar(); 
                break;
            case 0x21: // OUT
                putc((char)reg[R0], stdout);
                fflush(stdout);
                break;
            case 0x22: // PUTS
            {
                int address = reg[R0];
                while (memory[address]) {
                    putc((char)memory[address], stdout);
                    ++address; 
                }
                fflush(stdout);
                break;
            }
            case 0x23: // IN
                printf("Enter a character: ");
                reg[R0] = (uint16_t)getchar();
                break;
            case 0x24: // PUTSP
            {
                printf("%s", (const char*)(memory + reg[R0]));
                break;
            }
            case 0x25: // HALT
                puts("HALT");
                fflush(stdout);
                running = false;
                break;
        }
    }

    uint16_t read(uint16_t address) {
        if (address == KBSR) {
            if (check_key()) {
                memory[KBSR] = (1 << 15);
                memory[KBDR] = getchar();
            } else {
                memory[KBSR] = 0;
            }
        }
        return memory[address];
    }

    void write(uint16_t address, uint16_t x) { memory[address] = x; }

    void read_image(const char* image_path) {
        FILE* file = fopen(image_path, "rb");
        uint16_t origin;
        fread(&origin, sizeof(origin), 1, file);
        origin = swap16(origin);
        
        while (!feof(file)) {
            uint16_t temp;
            fread(&temp, sizeof(uint16_t), 1, file);
            memory[origin] = swap16(temp);
            ++origin;
        }
        fclose(file);
    }
    /*
      I got this method from Bisquit's NES emulator tutorial.
      You can see his work here: https://github.com/bisqwit
       */

    template <unsigned op>
    void ins(uint16_t instr) {
        uint16_t r0, r1, r2, imm, imm_flag;
        uint16_t pc_plus_off, base_plus_off;

        uint16_t opbit = (1 << op);
        if (0x4EEE & opbit) { r0 = (instr >> 9) & 0x7; }
        if (0x12E3 & opbit) { r1 = (instr >> 6) & 0x7; }
        if (0x0022 & opbit) { 
            r2 = instr & 0x7; 
            imm_flag = (instr >> 5) & 0x1;
            imm = sign_extend((instr) & 0x1F, 5);
        }
        if (0x00C0 & opbit) { // Base + offset
            base_plus_off = reg[r1] + sign_extend(instr & 0x3f, 6); 
        }
        if (0x4C0D & opbit) { // Indirect address
            pc_plus_off = reg[PC] + sign_extend(instr & 0x1ff, 9); 
        }
        if (0x0001 & opbit) {  // BR
            uint16_t cond = (instr >> 9) & 0x7; 
            if (cond & reg[CF]) { reg[PC] = pc_plus_off; }
        } 
        if (0x0002 & opbit) {  // ADD
            if (imm_flag) {
                reg[r0] = reg[r1] + imm;
            } else {
                reg[r0] = reg[r1] + reg[r2]; 
            }
        }
        if (0x0020 & opbit) {  // AND
            if (imm_flag) {
                reg[r0] = reg[r1] & imm;
            } else {
                reg[r0] = reg[r1] & reg[r2];
            }
        }
        if (0x0200 & opbit) { reg[r0] = ~reg[r1]; } // NOT
        if (0x1000 & opbit) {  // JMP
            reg[PC] = reg[r1]; 
        }
        if (0x0010 & opbit) { // JSR
            uint16_t long_flag = (instr >> 11) & 1; 
            pc_plus_off = reg[PC] +  sign_extend(instr & 0x7ff, 11);
            reg[R7] = reg[PC]; 
            if (long_flag) {
                reg[PC] = pc_plus_off;
            } else {
                reg[PC] = reg[r1];
            }
        }

        if (0x0004 & opbit) { reg[r0] = read(pc_plus_off); } // LD
        if (0x0400 & opbit) { reg[r0] = read(read(pc_plus_off)); } // LDI
        if (0x0040 & opbit) { reg[r0] = read(base_plus_off); }  // LDR
        if (0x4000 & opbit) { reg[r0] = pc_plus_off; } // LEA
        if (0x0008 & opbit) { write(pc_plus_off, reg[r0]); } // ST
        if (0x0800 & opbit) { write(read(pc_plus_off), reg[r0]); } // STI
        if (0x0080 & opbit) { write(base_plus_off, reg[r0]); } // STR
        if (0x8000 & opbit) { trap(instr & 0xFF); }; // TRP
        //if (0x0100 & opbit) { } // RTI
        if (0x4666 & opbit) { update_flags(r0); }
    }

    static void (*op_table[16])(uint16_t) = {
        ins<0>, ins<1>, ins<2>, ins<3>, 
        ins<4>, ins<5>, ins<6>, ins<7>, 
        NULL, ins<9>, ins<10>, ins<11>, 
        ins<12>, NULL, ins<14>, ins<15>  
    };

    void run() {
        reg[PC] = 0x3000;
        running = true;
        while (running)
        {
            uint16_t instr = read(reg[PC]++);
            op_table[instr >> 12](instr);
        }  
    }
};

struct termios original_tio;

void handle_interrupt(int signal) {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio); // restore input buffering
    printf("\n");
    exit(-2);
}

int main(int argc, const char* argv[])
{
    signal(SIGINT, handle_interrupt);

    tcgetattr(STDIN_FILENO, &original_tio); // disable input buffering
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    if (argc > 1) Lc3::read_image(argv[1]);
    Lc3::run();
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio); // restore input buffering
}

