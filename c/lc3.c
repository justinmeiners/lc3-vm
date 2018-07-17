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

// OP codes
enum 
{
    OP_BR = 0,
    OP_ADD = 1,
    OP_LD = 2,
    OP_ST = 3,
    OP_JSR = 4,
    OP_AND = 5,
    OP_LDR = 6,
    OP_STR = 7,
    OP_RTI = 8,
    OP_NOT = 9,
    OP_LDI = 10,
    OP_STI = 11,
    OP_JMP = 12,
    OP_LEA = 14,
    OP_TRAP = 15
};

// Condition Flags
enum
{
    FL_POS = 1 << 0, // P
    FL_ZRO = 1 << 1, // Z
    FL_NEG = 1 << 2, // N
};

// Registers
enum
{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,
    R_COND,
    R_COUNT
};

// Memory mapped registers
enum
{
    MR_KBSR = 0xFE00,
    MR_KBDR = 0xFE02
};

// Trap Codes (OS Calls)
enum
{
    TRAP_GETC = 0x20,
    TRAP_OUT = 0x21,
    TRAP_PUTS = 0x22,
    TRAP_IN = 0x23,
    TRAP_PUTSP = 0x24,
    TRAP_HALT = 0x25
};

#define PC_START 0x3000
#define ADDRESS_SPACE UINT16_MAX

uint16_t memory[ADDRESS_SPACE];
uint16_t reg[R_COUNT];
int running = 1;

void update_flags(int r0)
{
    if (reg[r0] == 0)
    {
        reg[R_COND] = FL_ZRO;
    }
    else if ((reg[r0] >> 15) & 0x1)
    {
        reg[R_COND] = FL_NEG;
    }
    else
    {
        reg[R_COND] = FL_POS;
    }  
}

void trap(uint16_t code)
{
    switch (code)
    {
        case TRAP_GETC:
            // reads a single ASCII char
            reg[R_R0] = (uint16_t)getchar(); 
            break;
        case TRAP_OUT:
            putc((char)reg[R_R0], stdout);
            fflush(stdout);
            break;
        case TRAP_PUTS:
        {
            int address = reg[R_R0];

            while (memory[address])
            {
                putc((char)memory[address], stdout);
                ++address; 
            }
            fflush(stdout);
            break;
        }
        case TRAP_IN:
            printf("Enter a character: ");
            reg[R_R0] = (uint16_t)getchar();
            break;
        case TRAP_PUTSP:
        {
            const char* str = (const char*)(memory + reg[R_R0]);
            printf("%s", str);
            break;
        }
        case TRAP_HALT:
            puts("HALT");
            fflush(stdout);
            running = 0; 
            break;
    }
}

uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) && 1) {
        // extend 1s
        for (int i = bit_count; i < 16; ++i)
            x |= (1 << i);
    }
    return x;
}

uint16_t swap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}

void read_image_file(FILE* file)
{
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);
    
    while (!feof(file))
    {
        uint16_t temp;
        fread(&temp, sizeof(uint16_t), 1, file);
        memory[origin] = swap16(temp);
        ++origin;
    }
}

void read_image(const char* image_path)
{
    FILE* file = fopen(image_path, "rb");
    read_image_file(file);
    fclose(file);
}

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

uint16_t check_key() {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    if (select(1, &readfds, NULL, NULL, &timeout)) {
        return 1;
    }
    else 
    {
        return 0;
    }
}

void mem_write(uint16_t address, uint16_t val)
{
    memory[address] = val;
}

uint16_t mem_read(uint16_t address)
{
    if (address == MR_KBSR)
    {
        uint16_t has_key = check_key();
        
        if (has_key) {
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        }
        else
        {
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
}

// input a character from the keyboard (0x23)
// output a character to the monitor (0x21)
// halt the program (0x25)

int main(int argc, const char* argv[])
{
    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

    memset(memory, 0, sizeof(memory));

    reg[R_PC] = PC_START;
    if (argc > 1) read_image(argv[1]);
    
    while (running)
    {
        // FETCH
        uint16_t instr = mem_read(reg[R_PC]);
        ++reg[R_PC];

        // DECODE

        // registers
        uint16_t r2 = instr & 0x7;
        uint16_t r1 = (instr >> 6) & 0x7;
        uint16_t r0 = (instr >> 9) & 0x7;
        uint16_t op = instr >> 12;

        // immediate mode [0-5)
        uint16_t imm_flag = (instr >> 5) & 0x1;
        uint16_t imm = sign_extend((instr) & 0x1F, 5);

        // pc offset [0-9)
        uint16_t pc_offset = sign_extend((instr) & 0x1ff, 9);

        // condition flags [9-12)
        uint16_t cond_flag = (instr >> 9) & 0x7;

        switch (op)
        {
            case OP_ADD:
                if (imm_flag)
                {
                    reg[r0] = reg[r1] + imm;
                }
                else
                {
                    reg[r0] = reg[r1] + reg[r2];
                }

                update_flags(r0);
                break;
            case OP_AND:
                if (imm_flag)
                {
                    reg[r0] = reg[r1] & imm;
                }
                else
                {
                    reg[r0] = reg[r1] & reg[r2];
                }
                update_flags(r0);
                break;
            case OP_NOT:
                reg[r0] = ~reg[r1];
                update_flags(r0);
                break;
            case OP_BR:
                if (cond_flag & reg[R_COND])
                {
                    reg[R_PC] += pc_offset;
                }
                break;
            case OP_JMP:
                // ret handled implicity when r1 = 7
                reg[R_PC] = reg[r1];
                break;
            case OP_JSR:
            {
                // special decode for unique instruction
                uint16_t long_pc_offset = sign_extend(instr & 0x7ff, 11);
                uint16_t long_flag = (instr >> 11) & 1;

                reg[R_R7] = reg[R_PC];
                if (long_flag)
                {
                    // JSR
                    reg[R_PC] += long_pc_offset;  
                }
                else
                {
                    // JSRR
                    reg[R_PC] = reg[r1];
                }
                break;
            }
            case OP_LD:
                reg[r0] = mem_read(reg[R_PC] + pc_offset);
                update_flags(r0);
                break;
            case OP_LDI:
                reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
                update_flags(r0);
                break;
            case OP_LDR:
                reg[r0] = mem_read(reg[r1] + imm);
                update_flags(r0);
                break;
            case OP_LEA:
                reg[r0] = reg[R_PC] + pc_offset;
                update_flags(r0);
                break;
            case OP_ST:
                mem_write(reg[R_PC] + pc_offset, reg[r0]);
                break;
            case OP_STI:
                mem_write(mem_read(reg[R_PC] + pc_offset), reg[r0]);
                break;
            case OP_STR:
            {
                mem_write(reg[r1] + imm, reg[r0]);
                break;
            }
            case OP_TRAP:
            {
                // special decode for unique instruction
                trap(instr & 0xFF);
                break;
            }
            case OP_RTI:
                // BLEH?
                break;
            default:
                break;
        }
    }

    restore_input_buffering();
}

