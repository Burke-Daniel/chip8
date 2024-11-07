// TODO feature list:
// - Work on input
// - Would be cool to have a graphical menu
//   that you can load programs from


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "raylib.h"

/* #define DEBUG */

// TODO error logging as well
#ifdef DEBUG
#define DEBUG_PRINT printf
#else
#define DEBUG_PRINT
#endif

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

#define CHIP8_MEMORY_SIZE 4096U
#define CHIP8_STACK_SIZE 16U
#define MAX_PROGRAM_SIZE 2 * 1028

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0') 

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


const uint8_t hex_sprites[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};

typedef struct
{
    uint16_t stack_arr[CHIP8_STACK_SIZE];
    uint8_t stack_pointer;
} Stack;

void stack_push(Stack* stack, uint16_t val)
{
    /* if (stack->stack_pointer + 1 > CHIP8_STACK_SIZE) */
    /* { */
    /*     DEBUG_PRINT("WARNING: STACK OVERFLOW\n"); */
    /*     return; */
    /* } */

    stack->stack_arr[stack->stack_pointer] = val;
    stack->stack_pointer++;
}

uint16_t stack_pop(Stack* stack)
{
    /* if (stack->stack_pointer == 0) */
    /* { */
    /*     DEBUG_PRINT("WARNING: popping from empty stack\n"); */
    /*     return 0; */
    /* } */

    stack->stack_pointer--;
    uint16_t val = stack->stack_arr[stack->stack_pointer];

    return val;
}

bool is_instruction(uint16_t opcode, uint16_t instruction)
{
    return (opcode & instruction);
}

typedef union
{
    uint8_t V[16];
    struct
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
    };
} Registers;

uint8_t chip8_key_to_keyboard_key[] = {
    [0x0] = KEY_X,
    [0x1] = KEY_ONE,
    [0x2] = KEY_TWO,
    [0x3] = KEY_THREE,
    [0x4] = KEY_Q,
    [0x5] = KEY_W,
    [0x6] = KEY_E,
    [0x7] = KEY_A,
    [0x8] = KEY_S,
    [0x9] = KEY_D,
    [0xa] = KEY_Z,
    [0xb] = KEY_C,
    [0xc] = KEY_FOUR,
    [0xd] = KEY_R,
    [0xe] = KEY_F,
    [0xf] = KEY_V,
};

uint8_t keyboard_key_to_chip8_key[] = {
    [KEY_X] = 0x0,
    [KEY_ONE] = 0x1,
    [KEY_TWO] = 0x2,
    [KEY_THREE] = 0x3,
    [KEY_Q] = 0x4,
    [KEY_W] = 0x5,
    [KEY_E] = 0x6,
    [KEY_A] = 0x7,
    [KEY_S] = 0x8,
    [KEY_D] = 0x9,
    [KEY_Z] = 0xa,
    [KEY_C] = 0xb,
    [KEY_FOUR] = 0xc,
    [KEY_R] = 0xd,
    [KEY_F] = 0xe,
    [KEY_V] = 0xf,
};

bool key_state[] = {
    [KEY_ONE] = false,
    [KEY_TWO] = false,
    [KEY_THREE] = false,
    [KEY_FOUR] = false,
    [KEY_Q] = false,
    [KEY_W] = false,
    [KEY_E] = false,
    [KEY_R] = false,
    [KEY_A] = false,
    [KEY_S] = false,
    [KEY_D] = false,
    [KEY_F] = false,
    [KEY_Z] = false,
    [KEY_X] = false,
    [KEY_C] = false,
    [KEY_V] = false,
};

#define INSTRUCTION_SIZE (2)

uint8_t memory[CHIP8_MEMORY_SIZE];
Registers registers;
uint16_t I;
Stack stack;
static uint16_t program_counter = 0;
static uint16_t delay_timer = 0;
static uint16_t sound_timer = 0;

#define WIDTH (64U)
#define HEIGHT (32U)
#define SCALE_FACTOR (16U)

bool display[WIDTH][HEIGHT];

void get_input()
{
    key_state[KEY_ONE] = IsKeyDown(KEY_ONE);
    if (key_state[KEY_ONE])
        DEBUG_PRINT("keys.key_one_pressed is true\n");
    key_state[KEY_TWO] = IsKeyDown(KEY_TWO);
    if (key_state[KEY_TWO])
        DEBUG_PRINT("keys.key_two_pressed is true\n");
    key_state[KEY_THREE] = IsKeyDown(KEY_THREE);
    if (key_state[KEY_THREE])
        DEBUG_PRINT("keys.key_three_pressed is true\n");
    key_state[KEY_FOUR] = IsKeyDown(KEY_FOUR);
    if (key_state[KEY_FOUR])
        DEBUG_PRINT("keys.key_four_pressed is true\n");
    key_state[KEY_Q] = IsKeyDown(KEY_Q);
    if (key_state[KEY_Q])
        DEBUG_PRINT("keys.key_q_pressed is true\n");
    key_state[KEY_W] = IsKeyDown(KEY_W);
    if (key_state[KEY_W])
        DEBUG_PRINT("keys.key_w_pressed is true\n");
    key_state[KEY_E] = IsKeyDown(KEY_E);
    if (key_state[KEY_E])
        DEBUG_PRINT("keys.key_e_pressed is true\n");
    key_state[KEY_R] = IsKeyDown(KEY_R);
    if (key_state[KEY_R])
        DEBUG_PRINT("keys.key_r_pressed is true\n");
    key_state[KEY_A] = IsKeyDown(KEY_A);
    if (key_state[KEY_A])
        DEBUG_PRINT("keys.key_a_pressed is true\n");
    key_state[KEY_S] = IsKeyDown(KEY_S);
    if (key_state[KEY_S])
        DEBUG_PRINT("keys.key_s_pressed is true\n");
    key_state[KEY_D] = IsKeyDown(KEY_D);
    if (key_state[KEY_D])
        DEBUG_PRINT("keys.key_d_pressed is true\n");
    key_state[KEY_F] = IsKeyDown(KEY_F);
    if (key_state[KEY_F])
        DEBUG_PRINT("keys.key_f_pressed is true\n");
    key_state[KEY_Z] = IsKeyDown(KEY_Z);
    if (key_state[KEY_Z])
        DEBUG_PRINT("keys.key_z_pressed is true\n");
    key_state[KEY_X] = IsKeyDown(KEY_X);
    if (key_state[KEY_X])
        DEBUG_PRINT("keys.key_x_pressed is true\n");
    key_state[KEY_C] = IsKeyDown(KEY_C);
    if (key_state[KEY_C])
        DEBUG_PRINT("keys.key_c_pressed is true\n");
    key_state[KEY_V] = IsKeyDown(KEY_V);
    if (key_state[KEY_V])
        DEBUG_PRINT("keys.key_v_pressed is true\n");
}

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
        DEBUG_PRINT("0x%x ", c);
    }
    DEBUG_PRINT("\n");
    fclose(program);
}

void dump_display_memory()
{
    DEBUG_PRINT("Display Memory");
	for (int i = 0; i < WIDTH; i++)
    {
        for (int j = 0; j < HEIGHT; j++)
        {
            DEBUG_PRINT("0x%02x  ", display[i][j]);
        }
        DEBUG_PRINT("\n");
    }
}

void execute_instruction(uint16_t opcode)
{
    DEBUG_PRINT("Opcode: 0x%04x\n", opcode);
    DEBUG_PRINT("Before program executed, program counter: 0x%04x\n", program_counter);

    if ((opcode & 0xF000) == 0x1000)
    {
        DEBUG_PRINT("Found JUMP_ADDR instruction\n");
        uint16_t value = opcode & 0x0FFF;
        DEBUG_PRINT("Setting program counter to %d\n", value);
        program_counter = value;
    }
    else if ((opcode & 0xF000) == 0x2000)
    {
        DEBUG_PRINT("Found CALL instruction\n");
        uint16_t value = opcode & 0x0FFF;
        DEBUG_PRINT("Calling function at address %d\n", value);
        DEBUG_PRINT("Pushing address %d to the stack\n", program_counter);
        stack_push(&stack, program_counter);
        program_counter = value;
    }
    else if ((opcode & 0xF000) == 0x3000)
    {
        DEBUG_PRINT("Found SE Vx, byte instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 8;
        uint8_t val = (opcode & 0x00FF);
        DEBUG_PRINT("Registers[%d] = %d\n", vx, registers.V[vx]);
        DEBUG_PRINT("val = %d\n", val);

        if (registers.V[vx] == val)
        {
            DEBUG_PRINT("register[%d] == %d, skipping next instruction\n", vx, val);
            program_counter += 2 * INSTRUCTION_SIZE;
        }
        else
        {
            DEBUG_PRINT("register[%d] != %d, not skipping next instruction\n", vx, val);
            program_counter += INSTRUCTION_SIZE;
        }
    }
    else if ((opcode & 0xF000) == 0x4000)
    {
        DEBUG_PRINT("Found SNE Vx, byte instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 8;
        uint8_t val = (opcode & 0x00FF);
        DEBUG_PRINT("Registers[%d] != %d\n", vx, val);

        if (registers.V[vx] != val)
        {
            program_counter += 2 * INSTRUCTION_SIZE;
        }
        else
        {
            program_counter += INSTRUCTION_SIZE;
        }
    }
    else if ((opcode & 0xF000) == 0x5000)
    {
        DEBUG_PRINT("Found SE Vx, Vy instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 8;
        uint8_t vy = (opcode & 0x00F0) >> 4;
        DEBUG_PRINT("Registers[%d] == Registers[%d]\n", vx, vy);

        if (registers.V[vx] == registers.V[vy])
        {
            program_counter += 2 * INSTRUCTION_SIZE;
        }
        else
        {
            program_counter += INSTRUCTION_SIZE;
        }
    }
    else if ((opcode & 0xF000) == 0x6000)
    {
        DEBUG_PRINT("Found LD Vx, byte instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 8;
        uint8_t val = (opcode & 0x00FF);
        DEBUG_PRINT("Registers[%d] = %d\n", vx, val);
        registers.V[vx] = val;
        program_counter += INSTRUCTION_SIZE;
    }
    else if ((opcode & 0xF000) == 0x7000)
    {
        DEBUG_PRINT("Found ADD Vx, byte instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 8;
        uint8_t val = (opcode & 0x00FF);
        /* DEBUG_PRINT("Registers[%d] += %d\n", vx, val); */
        DEBUG_PRINT("Before Register[%d] = %d\n", vx, registers.V[vx]);
        registers.V[vx] += val;
        DEBUG_PRINT("After Register[%d] = %d\n", vx, registers.V[vx]);
        program_counter += INSTRUCTION_SIZE;
    }
    else if ((opcode & 0xF000) == 0x8000)
    {
        uint16_t sub_code = opcode & 0x000F;
        switch (sub_code)
        {
        case 0x0:
        {
            DEBUG_PRINT("Found LD Vx, Vy instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            uint8_t vy = (opcode & 0x00F0) >> 4;
            DEBUG_PRINT("registers[%d] = registers[%d]\n", vx, vy);
            registers.V[vx] = registers.V[vy];
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x1:
        {
            DEBUG_PRINT("Found OR Vx, Vy instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            uint8_t vy = (opcode & 0x00F0) >> 4;
            DEBUG_PRINT("registers[%d] |= registers[%d]\n", vx, vy);
            registers.V[vx] |= registers.V[vy];
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x2:
        {
            DEBUG_PRINT("Found AND Vx, Vy instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            uint8_t vy = (opcode & 0x00F0) >> 4;
            DEBUG_PRINT("registers[%d] &= registers[%d]\n", vx, vy);
            registers.V[vx] &= registers.V[vy];
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x3:
        {
            DEBUG_PRINT("Found XOR Vx, Vy instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            uint8_t vy = (opcode & 0x00F0) >> 4;
            DEBUG_PRINT("registers[%d] ^= registers[%d]\n", vx, vy);
            registers.V[vx] ^= registers.V[vy];
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x4:
        {
            DEBUG_PRINT("Found ADD Vx, Vy instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            uint8_t vy = (opcode & 0x00F0) >> 4;
            DEBUG_PRINT("registers[%d] += registers[%d]\n", vx, vy);
            uint8_t val = registers.V[vx] + registers.V[vy];
            registers.VF = val > 255 ? 1 : 0;
            registers.V[vx] = val;
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x5:
        {
            DEBUG_PRINT("Found SUB Vx, Vy instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            uint8_t vy = (opcode & 0x00F0) >> 4;
            DEBUG_PRINT("registers[%d] -= registers[%d]\n", vx, vy);
            uint8_t val = registers.V[vx] - registers.V[vy];
            registers.VF = registers.V[vx] > registers.V[vy] ? 1 : 0;
            registers.V[vx] = val;
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x6:
        {
            DEBUG_PRINT("Found SHR Vx, { Vy } instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            DEBUG_PRINT("registers[%d] >> 1\n", vx);
            uint8_t val = registers.V[vx] >> 1;
            registers.VF = registers.V[vx] & 0x1 ? 1 : 0;
            registers.V[vx] = val;
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x7:
        {
            DEBUG_PRINT("Found SUBN Vx, Vy instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            uint8_t vy = (opcode & 0x00F0) >> 4;
            DEBUG_PRINT("registers[%d] -= registers[%d]\n", vy, vx);
            uint8_t val = registers.V[vy] - registers.V[vx];
            registers.VF = registers.V[vy] > registers.V[vx] ? 1 : 0;
            registers.V[vx] = val;
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0xE:
        {
            DEBUG_PRINT("Found SHL Vx, Vy instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            DEBUG_PRINT("registers[%d] << 1\n", vx);
            uint8_t val = registers.V[vx] << 1;
            registers.VF = registers.V[vx] & 0x80 ? 1 : 0;
            registers.V[vx] = val;
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        default:
            DEBUG_PRINT("Invalid instruction: 0x%04x\n", opcode);
            exit(1);
            break;
        }
    }
    else if ((opcode & 0xF00F) == 0x9000)
    {
        DEBUG_PRINT("Found SNE Vx, Vy instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 8;
        uint8_t vy = (opcode & 0x00F0) >> 4;
        DEBUG_PRINT("Skipping if registers[%d] != registers[%d]\n", vx, vy);
        if (registers.V[vx] != registers.V[vy])
        {
            program_counter += 2 * INSTRUCTION_SIZE;
        }
        else
        {
            program_counter += INSTRUCTION_SIZE;
        }
    }
    else if ((opcode & 0xF000) == 0xA000)
    {
        DEBUG_PRINT("Found LD I, addr instruction\n");
        uint16_t val = opcode & 0x0FFF;
        DEBUG_PRINT("I = 0x%x\n", val);
        I = val;
        program_counter += INSTRUCTION_SIZE;
    }
    else if ((opcode & 0xF000) == 0xB000)
    {
        DEBUG_PRINT("Found JP V0, addr instruction\n");
        uint16_t val = opcode & 0x0FFF;
        DEBUG_PRINT("program_counter = registers[0] + %d\n", val);
        program_counter = registers.V0 + val;
    }
    else if ((opcode & 0xF000) == 0xC000)
    {
        DEBUG_PRINT("Found RND Vx, byte instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 8;
        uint8_t val = (opcode & 0x00FF);
        DEBUG_PRINT("Registers[%d] = rand() & %d\n", vx, val);
        registers.V[vx] = (rand() % 255) & val;
        program_counter += INSTRUCTION_SIZE;
    }
    else if ((opcode & 0xF000) == 0xD000)
    {
        DEBUG_PRINT("Draw: Before doing draw, PC=0x%x\n", program_counter);

		uint8_t target_v_reg_x = (opcode & 0x0F00) >> 8;
		uint8_t target_v_reg_y = (opcode & 0x00F0) >> 4;
		uint8_t sprite_height = opcode & 0x000F;
		uint8_t x_location = registers.V[target_v_reg_x];
		uint8_t y_location = registers.V[target_v_reg_y];

        DEBUG_PRINT("Drawing at x=%d y=%d using memory starting at I=0x%x\n", x_location, y_location, I);

		registers.VF = 0;
        for (int32_t i = 0; i < sprite_height; i++)
        {
            uint8_t sprite = memory[I + i];
            DEBUG_PRINT("Sprite is located at address %d\n", I + i);
            DEBUG_PRINT("Sprite: 0x%x\n", sprite);

            for (size_t j = 0; j < 8; j++)
            {
                DEBUG_PRINT("Drawing X %d Y %d\n", y_location + i + 1, x_location + j);
                bool bit = sprite & (1 << (7-j));
                DEBUG_PRINT("Bit at %d is %d\n", j, bit);

                uint8_t x = (x_location + j) % WIDTH;
                uint8_t y = (y_location + i + 1) % HEIGHT;

                bool previous_display_value = display[x][y];

                display[x][y] ^= bit;
                
                bool new_display_value = display[x][y];

                if (previous_display_value == 1 && new_display_value == 0)
                {
                    registers.VF = 1;
                }
            }
        }

        DEBUG_PRINT("Draw: Adding two to program counter\n");
        DEBUG_PRINT("Before: 0x%x\n", program_counter);
        program_counter += INSTRUCTION_SIZE;
        DEBUG_PRINT("After: 0x%x\n", program_counter);
    }
    else if ((opcode & 0xF0FF) == 0xE09E)
    {
        DEBUG_PRINT("Found SKP Vx instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 0x8;
        DEBUG_PRINT("Skipping next instruction if key registers[%d] is pressed\n", vx);
        if (key_state[chip8_key_to_keyboard_key[registers.V[vx]]])
        {
            program_counter += 2 * INSTRUCTION_SIZE;
        }
        else
        {
            program_counter += INSTRUCTION_SIZE;
        }
    }
    else if ((opcode & 0xF0FF) == 0xE0A1)
    {
        DEBUG_PRINT("Found SKNP Vx instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 0x8;
        DEBUG_PRINT("Skipping next instruction if key registers[%d] is pressed\n", vx);
        if (!key_state[chip8_key_to_keyboard_key[registers.V[vx]]])
        {
            program_counter += 2 * INSTRUCTION_SIZE;
        }
        else
        {
            program_counter += INSTRUCTION_SIZE;
        }
    }
    else if ((opcode & 0xF000) == 0xF000)
    {
        DEBUG_PRINT("Found instruction that starts with F\n");
        uint16_t sub_word = opcode & 0x00FF;
        DEBUG_PRINT("Subword is 0x%x\n", sub_word);
        switch (sub_word)
        {
            case 0x07:
            {
                DEBUG_PRINT("Found LD Vx, DT instruction\n");
                uint8_t vx = (opcode & 0x0F00) >> 0x8;
                DEBUG_PRINT("Setting register[%d] == DT value\n", vx);
                registers.V[vx] = delay_timer;
                program_counter += INSTRUCTION_SIZE;
                break;
            }
            case 0x0A:
            {
                DEBUG_PRINT("Found LD Vx, K instruction\n");
                uint8_t vx = (opcode & 0x0F00) >> 0x8;
                DEBUG_PRINT("Waiting for keypress to store in registers[%d]\n", vx);

                // TODO need to make sure the key that is pressed is a chip8 key
                int key = 0;
                bool key_pressed = false;
                DEBUG_PRINT("Size of the key_state array is %d\n", ARRAY_SIZE(key_state));
                for (size_t i = 0; i < ARRAY_SIZE(key_state); i++)
                {
                    if (key_state[i])
                    {
                        DEBUG_PRINT("Key %d is pressed", i);
                        key = i;
                        key_pressed = true;
                        break;
                    }
                }

                if (key_pressed)
                {
                    registers.V[vx] = keyboard_key_to_chip8_key[key];
                    program_counter += INSTRUCTION_SIZE;
                }
                break;
            }
            case 0x15:
            {
                DEBUG_PRINT("Found LD DT, Vx instruction\n");
                uint8_t vx = (opcode & 0x0F00) >> 0x8;
                DEBUG_PRINT("Setting DT == register[%d]\n", vx);
                delay_timer = registers.V[vx];
                program_counter += INSTRUCTION_SIZE;
                break;
            }
            case 0x18:
            {
                DEBUG_PRINT("Found LD ST, Vx instruction\n");
                uint8_t vx = (opcode & 0x0F00) >> 0x8;
                DEBUG_PRINT("Setting ST == register[%d]\n", vx);
                sound_timer = registers.V[vx];
                program_counter += INSTRUCTION_SIZE;
                break;
            }
            case 0x1E:
            {
                DEBUG_PRINT("Found ADD I, Vx instruction\n");
                uint8_t vx = (opcode & 0x0F00) >> 0x8;
                DEBUG_PRINT("Setting I += register[%d]\n", vx);
                I += registers.V[vx];
                program_counter += INSTRUCTION_SIZE;
                break;
            }
            case 0x29:
            {
                DEBUG_PRINT("Found LD F, Vx instruction\n");
                uint8_t vx = (opcode & 0x0F00) >> 0x8;
                DEBUG_PRINT("Setting I hex sprite at register[%d]\n", vx);
                I = 5 * registers.V[vx];
                program_counter += INSTRUCTION_SIZE;
                break;
            }
            case 0x33:
            {
                DEBUG_PRINT("Found LD F, Vx instruction\n");
                uint8_t vx = (opcode & 0x0F00) >> 0x8;
                uint16_t val = registers.V[vx];
                uint16_t hundreds = val / 100;
                uint16_t tens = (val - (100 * hundreds)) / 10;
                uint16_t ones = (val - (100 * hundreds) - (10 * tens));
                DEBUG_PRINT("Putting %d at I, %d at I+1, %d at I+2\n", hundreds, tens, ones);

                memory[I] = hundreds;
                memory[I+1] = tens;
                memory[I+2] = ones;
                program_counter += INSTRUCTION_SIZE;
                break;
            }
            case 0x55:
            {
                DEBUG_PRINT("Found LD [I], Vx instruction\n");
                uint8_t vx = (opcode & 0x0F00) >> 0x8;
                DEBUG_PRINT("Storing V0 to V%d in memory\n", vx);
                memcpy(&memory[I], &registers.V[0], sizeof(registers.V[0]) * vx);
                program_counter += INSTRUCTION_SIZE;
                break;
            }
            case 0x65:
            {
                DEBUG_PRINT("Found LD Vx, [I] instruction\n");
                uint8_t vx = (opcode & 0x0F00) >> 0x8;
                DEBUG_PRINT("Loading V0 to V%d from memory\n", vx);
                memcpy(&registers.V[0], &memory[I], sizeof(registers.V[0]) * vx);
                program_counter += INSTRUCTION_SIZE;
                break;
            }
        }
    }
    else if ((opcode & 0x00F0) == 0x00E0)
    {
        switch(opcode)
        {
        case 0x00E0:
        {
            DEBUG_PRINT("Found CLEAR_SCREEN instruction\n");
            memset(display, 0, 32 * 64);
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x00EE:
        {
            DEBUG_PRINT("Found RETURN_SUBROUTINE instruction\n");
            program_counter = stack_pop(&stack);
            DEBUG_PRINT("Setting program_counter back to %d and incrementing\n", program_counter);
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        default:
            DEBUG_PRINT("Invalid instruction: 0x%04x\n", opcode);
            exit(1);
            break;
        }
    }
    else
    {
        DEBUG_PRINT("Invalid instruction 0x%x\n", opcode);
        exit(1);
    }
    DEBUG_PRINT("After instruction executed, program counter: 0x%04x\n", program_counter);
}

uint16_t create_draw_instruction(uint8_t vx, uint8_t vy, uint8_t n)
{
	uint16_t opcode = 0xD000;
	uint16_t X = (vx << 8) & 0x0F00;
	uint16_t Y = (vy << 4) & 0x00F0;
	uint16_t N = n & 0x000F;
	return opcode | X | Y | N;
}

int main(int argc, char** argv)
{
    InitAudioDevice();
    if (!IsAudioDeviceReady())
    {
        DEBUG_PRINT("ERROR: Cound not get audio device ready\n");
        exit(1);
    }

    bool sound_playing = false;
    Sound beep_timer_sound = LoadSound("/home/burkey/code/chip8/beep-02.wav");

    if (!IsSoundReady(beep_timer_sound))
    {
        DEBUG_PRINT("ERROR: Could not load beep timer sound\n");
        exit(1);
    }
    // 64 by 64
    InitWindow(WIDTH * SCALE_FACTOR, HEIGHT * SCALE_FACTOR, "raylib [core] example - basic window");

    char* program_name = "";
    if (argc > 1)
    {
        program_name = argv[1];
    }

    memcpy(memory, hex_sprites, sizeof(hex_sprites));

    FILE* program = fopen(program_name, "r");
    if (program == NULL)
    {
        fprintf(stderr, "Could not open program %s\n", program_name);
        return 1;
    }

    fseek(program, 0L, SEEK_END);
    size_t program_size = ftell(program);
    DEBUG_PRINT("The program is %lu bytes long\n", program_size);
    rewind(program);

    uint16_t program_opcodes[MAX_PROGRAM_SIZE];

    if (!program_opcodes)
    {
        fprintf(stderr, "Could not malloc memory");
        return false;
    }

    fread(program_opcodes, sizeof(uint16_t), program_size, program);
    size_t length = program_size / 2;

    DEBUG_PRINT("Length: %lu\n", length);
    for (int i = 0; i < length; i++)
    {
        DEBUG_PRINT("0x%04x ", program_opcodes[i]);
    }
    DEBUG_PRINT("\n");

    DEBUG_PRINT("Program length: %lu\n", length);

    memcpy(&memory[0x200], program_opcodes, sizeof(uint16_t) * length);
    program_counter = 0x200;

    stack.stack_pointer = 0;

    DEBUG_PRINT("After putting in memory:\n");
    for (int i = 0x200; i < 0x200 + (2 * length); i += 2)
    {
        DEBUG_PRINT("0x%02x%02x ", memory[i], memory[i+1]);
    }
    DEBUG_PRINT("\n");

    struct timespec start_time, current_time;
    timespec_get(&start_time, TIME_UTC);
    timespec_get(&current_time, TIME_UTC);

    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        // Get keyboard input
        get_input();

        uint16_t instruction = *(uint16_t*)(memory + program_counter);
        uint8_t inst_low = (uint8_t)(instruction & 0xff);
        uint8_t inst_high = (uint8_t)((instruction & 0xff00) >> 8);

        instruction = ((uint16_t)inst_low << 8) | (uint16_t)inst_high;
        execute_instruction(instruction);
        DEBUG_PRINT("\n");

        BeginDrawing();
        for (int i = 0; i < WIDTH; i++)
        {
            for (int j = 0; j < HEIGHT; j++)
            {
                Color color = display[i][j] ? BLACK : WHITE;
                DrawRectangle(SCALE_FACTOR * i, SCALE_FACTOR * j, SCALE_FACTOR * 8 , SCALE_FACTOR * 1, color);
            }
        }
        ClearBackground(RAYWHITE);
        EndDrawing();

        if (delay_timer > 0)
            delay_timer--;

        if (sound_timer > 0)
        {
            if (!sound_playing)
            {
                PlaySound(beep_timer_sound);
                sound_playing = true;
            }
            sound_timer--;
        }
        else
        {
            StopSound(beep_timer_sound);
            sound_playing = false;
        }
    }

    CloseWindow();
    
    return 0;
}

