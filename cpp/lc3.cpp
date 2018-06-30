#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
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
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    if (select(1, &readfds, NULL, NULL, &timeout)) {
        return 1;
    } else {
        return 0;
    }
}

namespace Lc3 {
    uint16_t sign_extend(uint16_t x, int bit_count) {
        if ((x >> (bit_count - 1)) & 1) {
            // extend 1s
            for (int i = bit_count; i < 16; ++i)
                x |= (1 << i);
        }
        return x;
    }

    uint16_t swap16(uint16_t x) {
        return (x << 8) | (x >> 8);
    }

    enum Op {
         BR, ADD,  LD,  ST, JSR,
        AND, LDR, STR, RTI, NOT, 
        LDI, STI, JMP, REV, LEA, TRAP
    };

    enum Flags {
        POS = 1 << 0, ZRO = 1 << 1, NEG = 1 << 2,
    };

    enum Reg {
        R0, R1, R2, R3, R4, R5, R6, R7,
        PC, COND, COUNT
    };

    enum MemReg {
        KBSR = 0xFE00, KBDR = 0xFE02
    };

    enum Trap {
        GETC = 0x20, OUT = 0x21, PUTS=0x22,
        IN = 0x23, PUTSP = 0x24, HALT = 0x25
    };

    uint16_t reg[COUNT];
    uint16_t memory[UINT16_MAX];
    bool running;

    void update_flags(int r0) {
        if (reg[r0] == 0) {
            reg[COND] = ZRO;
        } else if ((reg[r0] >> 15) & 0x1) {
            reg[COND] = NEG;
        } else {
            reg[COND] = POS;
        }  
    }

    void trap(uint16_t code) {
        switch (code) {
            case GETC:
                // reads a single ASCII char
                reg[R0] = (uint16_t)getchar(); 
                break;
            case OUT:
                putc((char)reg[R0], stdout);
                fflush(stdout);
                break;
            case PUTS: {
                int address = reg[R0];

                while (memory[address]) {
                    putc((char)memory[address], stdout);
                    ++address; 
                }
                fflush(stdout);
                break;
            }
            case IN:
                printf("Enter a character: ");
                reg[R0] = (uint16_t)getchar();
                break;
            case PUTSP: {
                const char* str = (const char*)(memory + reg[R0]);
                printf("%s", str);
                break;
            }
            case HALT:
                puts("HALT");
                fflush(stdout);
                running = 0; 
                break;
        }
    }

    uint16_t read(uint16_t address) {
        if (address == KBSR) {
            uint16_t has_key = check_key();
            
            if (has_key) {
                memory[KBSR] = (1 << 15);
                memory[KBDR] = getchar();
            } else {
                memory[KBSR] = 0;
            }
        }
        return memory[address];
    }

    void write(uint16_t address, uint16_t x) {
        memory[address] = x;
    }

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
            if (cond & reg[COND]) { reg[PC] = pc_plus_off; }
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

    static void (*op_table[])(uint16_t) = {
        ins<BR>, ins<ADD>, ins<LD>, ins<ST>, 
        ins<JSR>, ins<AND>, ins<LDR>, ins<STR>, 
        NULL, ins<NOT>, ins<LDI>, ins<STI>, 
        ins<JMP>, NULL, ins<LEA>, ins<TRAP>  
    };

    void init() {
        memset(memory, 0, sizeof(memory));
        reg[PC] = 0x3000;
    }

    void run() {
        running = true;
        while (running)
        {
            // FETCH
            uint16_t instr = read(reg[PC]);
            ++reg[PC];
            Op op = (Op)((instr >> 12) & 0xF);
            op_table[op](instr);
        }  
    }
};

struct termios original_tio;

void disable_input_buffering() 
{
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering() 
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

void handle_interrupt(int signal) {
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

int main(int argc, const char* argv[])
{
    signal(SIGINT, handle_interrupt);
    disable_input_buffering();
    Lc3::init();
    if (argc > 1) Lc3::read_image(argv[1]);
    Lc3::run();
    restore_input_buffering();
}

