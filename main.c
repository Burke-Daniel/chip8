// TODO feature list:
// - Would be cool to have a graphical menu
//   that you can load programs from


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "raylib.h"

#define CHIP8_MEMORY_SIZE 4096U

#define CLEAR_SCREEN             (0x00E0)
#define RETURN_SUBROUTINE        (0x00EE)
#define JUMP_ADDR                (0x1000)
#define CALL                     (0x2000)
#define SKIP_IF_EQ_IMM           (0x3000)
#define SKIP_IF_NEQ_IMM          (0x4000)
#define SKIP_IF_EQ               (0x5000)
#define ASSIGN_VX_IMM            (0x6000)
#define ADD_VX_IMM               (0x7000)
#define ASSIGN_VX_VY             (0x8000)
#define OR_VX_VY                 (0x8001)
#define AND_VX_VY                (0x8002)
#define XOR_VX_VY                (0x8003)
#define ADD_VX_VY                (0x8004)
#define SUB_VX_VY                (0x8005)
#define RIGHT_SHIFT_VX_VY        (0x8006)
#define VX_SUB_VY                (0x8007)
#define LEFT_SHIFT_VX_VY         (0x800E)
#define SET_I_ADDR               (0xA000)
#define JUMP_PLUS_V0             (0xB000)
#define RAND                     (0xC000)
#define DRAW_SPRITE              (0xD000)
#define SKIP_IF_KEY_PRESSED      (0xE09E)
#define SKIP_IF_KEY_NOT_PRESSED  (0xE0A1)
#define SET_VX_TIMER             (0xF007)
#define KEY_AWAIT_STORE          (0xF00A)
#define SET_DELAY_TIMER          (0xF015)
#define SET_SOUND_TIMER          (0xF018)
#define ADD_I_VX                 (0xF01E)
#define SET_I_SPRITE_LOCATION    (0xF029)
#define SET_BCD_VX               (0xF033)
#define REG_DUMP                 (0xF055)
#define REG_LOAD                 (0xF065)

bool is_instruction(uint16_t opcode, uint16_t instruction)
{
    return (opcode & instruction);
}

uint8_t memory[CHIP8_MEMORY_SIZE];

typedef struct
{
    uint8_t V0;
    uint8_t V1;
    uint8_t V2;
    uint8_t V3;
    uint8_t V4;
    uint8_t V5;
    uint8_t V6;
    uint8_t V7;
    uint8_t V8;
    uint8_t V9;
    uint8_t VA;
    uint8_t VB;
    uint8_t VC;
    uint8_t VD;
    uint8_t VE;
    uint8_t VF;
} Registers;

// TODO stack

void dump_program(const char *program_name)
{
    FILE *program = fopen(program_name, "r");
    if (program == NULL)
    {
        fprintf(stderr, "Could not open program %s\n", program_name);
        return;
    }

    int i = 0;
    int c;
    while ((c = fgetc(program)) != EOF)
    {
        printf("0x%x ", c);
    }
    printf("\n");

}

// Each opcode is two bytes long, big endian
// assume we are running on a little endian machine
// also caller is responsible for freeing memory
bool parse_program(FILE *program, uint16_t *program_opcodes, size_t *length)
{
    // TODO error_handling
    fseek(program, 0L, SEEK_END);
    size_t program_size = ftell(program);
    printf("The program is %lu bytes long\n", program_size);
    rewind(program);


    char *str = malloc(program_size + 1);
    if (!str)
    {
        fprintf(stderr, "Could not malloc memory");
        return false;
    }

    fread(str, program_size, sizeof(char), program);

    program_opcodes = (uint16_t*)str;
    *length = program_size / 2;

    for (int i = 0; i < *length; i++)
    {
        printf("0x%04x ", program_opcodes[i]);
    }

    printf("\nAfter big->little endian conversion:\n");
    for (int i = 0; i < *length; i++)
    {
        program_opcodes[i] = (program_opcodes[i] >> 8) | (program_opcodes[i] << 8);
        printf("0x%04x\n", program_opcodes[i]);
    }

    size_t program_counter = 0;
    while (program_counter < *length)
    {
        uint16_t opcode = program_opcodes[program_counter];
        if ((opcode & 0x00F0) == 0x00E0)
        {
            switch(opcode)
            {
            case 0x00E0:
                printf("Found CLEAR_SCREEN instruction\n");
                break;
            case 0x00EE:
                printf("Found RETURN_SUBROUTINE instruction\n");
                break;
            default:
                printf("Invalid instruction: 0x%04x\n", opcode);
                break;
            }
        }
        else if ((opcode & 0xF000) == 0x1000)
        {
            printf("Found JUMP_ADDR instruction\n");
        }
        else if ((opcode & 0xF000) == 0x2000)
        {
            printf("Found CALL instruction\n");
        }
        else if ((opcode & 0xF000) == 0x3000)
        {
            printf("Found SE Vx, byte instruction\n");
        }
        else if ((opcode & 0xF000) == 0x4000)
        {
            printf("Found SNE Vx, byte instruction\n");
        }
        else if ((opcode & 0xF000) == 0x5000)
        {
            printf("Found SE Vx, Vy instruction\n");
        }
        else if ((opcode & 0xF000) == 0x6000)
        {
            printf("Found LD Vx, byte instruction\n");
        }
        else if ((opcode & 0xF000) == 0x7000)
        {
            printf("Found ADD Vx, byte instruction\n");
        }
        else if ((opcode & 0xF000) == 0x8000)
        {
            uint16_t sub_code = opcode & 0x000F;
            switch(sub_code)
            {
            case 0x0:
                printf("Found LD Vx, Vy instruction\n");
                break;
            case 0x1:
                printf("Found OR Vx, Vy instruction\n");
                break;
            case 0x2:
                printf("Found AND Vx, Vy instruction\n");
                break;
            case 0x3:
                printf("Found XOR Vx, Vy instruction\n");
                break;
            case 0x4:
                printf("Found ADD Vx, Vy instruction\n");
                break;
            case 0x5:
                printf("Found SUB Vx, Vy instruction\n");
                break;
            case 0x6:
                printf("Found SHR Vx, Vy instruction\n");
                break;
            case 0x7:
                printf("Found SUBN Vx, Vy instruction\n");
                break;
            case 0xE:
                printf("Found SHL Vx, Vy instruction\n");
                break;
            default:
                printf("Invalid instruction: 0x%04x\n", opcode);
                break;
            }
        }
        else if ((opcode & 0xF00F) == 0x9000)
        {
            printf("Found SNE Vx, Vy instruction\n");
        }
        else if ((opcode & 0xF000) == 0xA000)
        {
            printf("Found LD I, addr instruction\n");
        }
        else if ((opcode & 0xF000) == 0xB000)
        {
            printf("Found JP V0, addr instruction\n");
        }
        else if ((opcode & 0xF000) == 0xC000)
        {
            printf("Found RND Vx, byte instruction\n");
        }
        else if ((opcode & 0xF000) == 0xD000)
        {
            printf("Found DRW Vx, Vy nibble instruction\n");
        }
        else if ((opcode & 0xF0FF) == 0xE09E)
        {
            printf("Found SKP Vx instruction\n");
        }
        else if ((opcode & 0xF0FF) == 0xE0A1)
        {
            printf("Found SKNP Vx instruction\n");
        }
        else if ((opcode & 0xF000) == 0xF000)
        {
            uint16_t sub_word = opcode & 0x00FF;
            switch (sub_word)
            {
            case 0x07:
                printf("Found LD Vx, DT instruction\n");
                break;
            case 0x0A:
                printf("Found LD Vx, K instruction\n");
                break;
            case 0x15:
                printf("Found LD DT, Vx instruction\n");
                break;
            case 0x18:
                printf("Found LD ST, Vx instruction\n");
                break;
            case 0x1E:
                printf("Found ADD I, Vx instruction\n");
                break;
            case 0x29:
                printf("Found LD F, Vx instruction\n");
                break;
            case 0x33:
                printf("Found LD F, Vx instruction\n");
                break;
            case 0x55:
                printf("Found LD [I], Vx instruction\n");
                break;
            case 0x65:
                printf("Found LD Vx, [I] instruction\n");
                break;
            }

        }
        program_counter++;
    }

    return true;
}

int main(void)
{
    // InitWindow(64, 32, "raylib [core] example - basic window");

    // while (!WindowShouldClose())
    // {
    //     BeginDrawing();
    //         ClearBackground(RAYWHITE);
    //         DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
    //     EndDrawing();
    // }

    // CloseWindow();
    
    // dump_program("random_number_test.ch8");

    const char* program_name = "random_number_test.ch8";
    FILE* program = fopen(program_name, "r");
    if (program == NULL)
    {
        fprintf(stderr, "Could not open program %s\n", program_name);
        return 1;
    }

    uint16_t *program_opcodes;
    size_t length;
    if (!parse_program(program, program_opcodes, &length))
    {
        fprintf(stderr, "Could not parse program\n");
        return 1;
    }

    //for (int i = 0; i < length; i++)
    //{
    //    printf("0x%x ", program_opcodes[i]);
    //}
    //printf("\n");

    return 0;
}

