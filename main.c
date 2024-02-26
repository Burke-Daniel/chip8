// TODO feature list:
// - Would be cool to have a graphical menu
//   that you can load programs from


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "raylib.h"

#define CHIP8_MEMORY_SIZE 4096U
#define CHIP8_STACK_SIZE 16U

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
    if (stack->stack_pointer + 1 > 16)
    {
        printf("WARNING: STACK OVERFLOW\n");
        return;
    }
    
    memmove(stack->stack_arr + 1, stack->stack_arr, stack->stack_pointer);
    stack->stack_arr[0] = val;
    stack->stack_pointer++;
}

uint16_t stack_pop(Stack* stack)
{
    if (stack->stack_pointer == 0)
    {
        printf("WARNING: popping from empty stack\n");
        return 0;
    }

    uint16_t val = stack->stack_arr[0];
    memmove(stack->stack_arr, stack->stack_arr + 1, stack->stack_pointer);
    stack->stack_pointer--;

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

uint8_t keyboard_mapping[] = {
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

#define INSTRUCTION_SIZE (2)

uint8_t memory[CHIP8_MEMORY_SIZE];
Registers registers;
uint16_t I;
Stack stack;
static uint16_t stack_pointer = 0;
static uint16_t program_counter = 0;
static uint16_t delay_timer = 0;
static uint16_t sound_timer = 0;

#define WIDTH (64U)
#define HEIGHT (32U)
#define SCALE_FACTOR (16U)

bool display[WIDTH][HEIGHT];

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
    fclose(program);
}

void dump_display_memory()
{
    printf("Display Memory");
	for (int i = 0; i < WIDTH; i++)
    {
        for (int j = 0; j < HEIGHT; j++)
        {
            printf("0x%02x  ", display[i][j]);
        }
        printf("\n");
    }
}

void execute_instruction(uint16_t opcode)
{
    printf("Opcode: 0x%04x\n", opcode);
    if ((opcode & 0x00F0) == 0x00E0)
    {
        switch(opcode)
        {
        case 0x00E0:
        {
            printf("Found CLEAR_SCREEN instruction\n");
            memset(display, 0, 32 * 64);
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x00EE:
        {
            printf("Found RETURN_SUBROUTINE instruction\n");
            program_counter = stack_pop(&stack);
            break;
        }
        default:
            printf("Invalid instruction: 0x%04x\n", opcode);
            break;
        }
    }
    else if ((opcode & 0xF000) == 0x1000)
    {
        printf("Found JUMP_ADDR instruction\n");
        uint16_t value = opcode & 0x0FFF;
        printf("Setting program counter to %d\n", value);
        program_counter = value;
    }
    else if ((opcode & 0xF000) == 0x2000)
    {
        printf("Found CALL instruction\n");
        uint16_t value = opcode & 0x0FFF;
        printf("Calling function at address %d\n", value);
        stack_push(&stack, program_counter);
        program_counter = value;
    }
    else if ((opcode & 0xF000) == 0x3000)
    {
        printf("Found SE Vx, byte instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 8;
        uint8_t val = (opcode & 0x00FF);
        printf("Registers[%d] == %d\n", vx, val);

        if (registers.V[vx] == val)
        {
            program_counter += 2 * INSTRUCTION_SIZE;
        }
        else
        {
            program_counter += INSTRUCTION_SIZE;
        }
    }
    else if ((opcode & 0xF000) == 0x4000)
    {
        printf("Found SNE Vx, byte instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 8;
        uint8_t val = (opcode & 0x00FF);
        printf("Registers[%d] != %d\n", vx, val);

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
        printf("Found SE Vx, Vy instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 8;
        uint8_t vy = (opcode & 0x00F0) >> 4;
        printf("Registers[%d] == Registers[%d]\n", vx, vy);

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
        printf("Found LD Vx, byte instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 8;
        uint8_t val = (opcode & 0x00FF);
        printf("Registers[%d] = %d\n", vx, val);
        registers.V[vx] = val;
        program_counter += INSTRUCTION_SIZE;
    }
    else if ((opcode & 0xF000) == 0x7000)
    {
        printf("Found ADD Vx, byte instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 8;
        uint8_t val = (opcode & 0x00FF);
        printf("Registers[%d] += %d\n", vx, val);
        registers.V[vx] += val;
        program_counter += INSTRUCTION_SIZE;
    }
    else if ((opcode & 0xF000) == 0x8000)
    {
        uint16_t sub_code = opcode & 0x000F;
        switch (sub_code)
        {
        case 0x0:
        {
            printf("Found LD Vx, Vy instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            uint8_t vy = (opcode & 0x00F0) >> 4;
            printf("registers[%d] = registers[%d]\n", vx, vy);
            registers.V[vx] = registers.V[vy];
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x1:
        {
            printf("Found OR Vx, Vy instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            uint8_t vy = (opcode & 0x00F0) >> 4;
            printf("registers[%d] |= registers[%d]\n", vx, vy);
            registers.V[vx] |= registers.V[vy];
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x2:
        {
            printf("Found AND Vx, Vy instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            uint8_t vy = (opcode & 0x00F0) >> 4;
            printf("registers[%d] &= registers[%d]\n", vx, vy);
            registers.V[vx] &= registers.V[vy];
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x3:
        {
            printf("Found XOR Vx, Vy instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            uint8_t vy = (opcode & 0x00F0) >> 4;
            printf("registers[%d] ^= registers[%d]\n", vx, vy);
            registers.V[vx] ^= registers.V[vy];
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x4:
        {
            printf("Found ADD Vx, Vy instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            uint8_t vy = (opcode & 0x00F0) >> 4;
            printf("registers[%d] += registers[%d]\n", vx, vy);
            uint8_t val = registers.V[vx] + registers.V[vy];
            registers.VF = val > 255 ? 1 : 0;
            registers.V[vx] = val;
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x5:
        {
            printf("Found SUB Vx, Vy instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            uint8_t vy = (opcode & 0x00F0) >> 4;
            printf("registers[%d] -= registers[%d]\n", vx, vy);
            uint8_t val = registers.V[vx] - registers.V[vy];
            registers.VF = registers.V[vx] > registers.V[vy] ? 1 : 0;
            registers.V[vx] = val;
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x6:
        {
            printf("Found SHR Vx, { Vy } instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            printf("registers[%d] >> 1\n", vx);
            uint8_t val = registers.V[vx] >> 1;
            registers.VF = registers.V[vx] & 0x1 ? 1 : 0;
            registers.V[vx] = val;
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x7:
        {
            printf("Found SUBN Vx, Vy instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            uint8_t vy = (opcode & 0x00F0) >> 4;
            printf("registers[%d] -= registers[%d]\n", vy, vx);
            uint8_t val = registers.V[vy] - registers.V[vx];
            registers.VF = registers.V[vy] > registers.V[vx] ? 1 : 0;
            registers.V[vx] = val;
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0xE:
        {
            printf("Found SHL Vx, Vy instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 8;
            printf("registers[%d] << 1\n", vx);
            uint8_t val = registers.V[vx] << 1;
            registers.VF = registers.V[vx] & 0x80 ? 1 : 0;
            registers.V[vx] = val;
            break;
        }
        default:
            printf("Invalid instruction: 0x%04x\n", opcode);
            break;
        }
    }
    else if ((opcode & 0xF00F) == 0x9000)
    {
        printf("Found SNE Vx, Vy instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 8;
        uint8_t vy = (opcode & 0x00F0) >> 4;
        printf("Skipping if registers[%d] != registers[%d]\n", vx, vy);
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
        printf("Found LD I, addr instruction\n");
        uint16_t val = opcode & 0x0FFF;
        printf("I = 0x%x\n", val);
        I = val;
        program_counter += INSTRUCTION_SIZE;
    }
    else if ((opcode & 0xF000) == 0xB000)
    {
        printf("Found JP V0, addr instruction\n");
        uint16_t val = opcode & 0x0FFF;
        printf("program_counter = registers[0] + %d\n", val);
        program_counter = registers.V0 + val;
    }
    else if ((opcode & 0xF000) == 0xC000)
    {
        printf("Found RND Vx, byte instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 8;
        uint8_t val = (opcode & 0x00FF);
        printf("Registers[%d] = rand() & %d\n", vx, val);
        registers.V[vx] = (rand() % 255) & val;
        program_counter += INSTRUCTION_SIZE;
    }
    else if ((opcode & 0xF000) == 0xD000)
    {
		uint8_t target_v_reg_x = (opcode & 0x0F00) >> 8;
		uint8_t target_v_reg_y = (opcode & 0x00F0) >> 4;
		uint8_t sprite_height = opcode & 0x000F;
		uint8_t x_location = registers.V[target_v_reg_x];
		uint8_t y_location = registers.V[target_v_reg_y];
		uint8_t pixel;

		registers.VF = 0;
		for (int y_coordinate = 0; y_coordinate < sprite_height; y_coordinate++)
		{
			pixel = memory[I + y_coordinate];
			for (int x_coordinate = 0; x_coordinate < 8; x_coordinate++)
			{
				if ((pixel & (0x80 >> x_coordinate)) != 0)
				{
					if (display[y_location + y_coordinate][x_location + x_coordinate] == 1)
					{
						registers.VF = 1;
					}
					display[y_location + y_coordinate][x_location + x_coordinate] ^= 1;
				}
			}
		}

        program_counter += INSTRUCTION_SIZE;
    }
    else if ((opcode & 0xF0FF) == 0xE09E)
    {
        printf("Found SKP Vx instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 0x8;
        printf("Skipping next instruction if key registers[%d] is pressed\n", vx);
        if (IsKeyPressed(keyboard_mapping[registers.V[vx]]))
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
        printf("Found SKNP Vx instruction\n");
        uint8_t vx = (opcode & 0x0F00) >> 0x8;
        printf("Skipping next instruction if key registers[%d] is pressed\n", vx);
        if (!IsKeyPressed(keyboard_mapping[registers.V[vx]]))
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
        uint16_t sub_word = opcode & 0x00FF;
        switch (sub_word)
        {
        case 0x07:
        {
            printf("Found LD Vx, DT instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 0x8;
            printf("Setting register[%d] == DT value\n", vx);
            registers.V[vx] = delay_timer;
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x0A:
        {
            // TODO debug this
            printf("Found LD Vx, K instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 0x8;
            printf("Waiting for keypress to store in registers[%d]\n", vx);
            bool found_key = false;
            int key_pressed;
            while (!found_key)
            {
                key_pressed = GetKeyPressed();
                for (int i = 0; i < 16; i++)
                {
                    if (key_pressed == keyboard_mapping[i])
                    {
                        found_key = true;
                        break;
                    }
                }
                /* usleep(1000 * 50); */
            }
            registers.V[vx] = key_pressed;
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x15:
        {
            printf("Found LD DT, Vx instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 0x8;
            printf("Setting DT == register[%d]\n", vx);
            delay_timer = registers.V[vx];
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x18:
        {
            printf("Found LD ST, Vx instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 0x8;
            printf("Setting ST == register[%d]\n", vx);
            sound_timer = registers.V[vx];
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x1E:
        {
            printf("Found ADD I, Vx instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 0x8;
            printf("Setting I += register[%d]\n", vx);
            I += registers.V[vx];
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x29:
        {
            printf("Found LD F, Vx instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 0x8;
            printf("Setting I hex sprite at register[%d]\n", vx);
            I = 5 * registers.V[vx];
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x33:
        {
            printf("Found LD F, Vx instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 0x8;
            uint16_t val = registers.V[vx];
            uint16_t hundreds = val / 100;
            uint16_t tens = (val - (100 * hundreds)) / 10;
            uint16_t ones = (val - (100 * hundreds) - (10 * tens));
            printf("Putting %d at I, %d at I+1, %d at I+2\n", hundreds, tens, ones);

            memory[I] = hundreds;
            memory[I+1] = tens;
            memory[I+2] = ones;
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x55:
        {
            printf("Found LD [I], Vx instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 0x8;
            printf("Storing V0 to V%d in memory\n", vx);
            memcpy(&memory[I], &registers.V[0], sizeof(registers.V[0]) * vx);
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        case 0x65:
        {
            printf("Found LD Vx, [I] instruction\n");
            uint8_t vx = (opcode & 0x0F00) >> 0x8;
            printf("Loading V0 to V%d from memory\n", vx);
            memcpy(&registers.V[0], &memory[I], sizeof(registers.V[0]) * vx);
            program_counter += INSTRUCTION_SIZE;
            break;
        }
        }
    }
}

#ifdef TEST

int main(void)
{
    for (int i = 0; i < 64; i += 2)
    {
        for (int j = 0; j < 32; j += 4)
        {
            display[i][j] = true;
        }
    }

    printf("Before clearing screen:\n");
    for (int i = 0; i < 64; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            printf("%d ", display[i][j]);
        }
        printf("\n");
    }

    execute_instruction(0x00E0);
    printf("After clearing screen:\n");
    for (int i = 0; i < 64; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            printf("%d ", display[i][j]);
        }
        printf("\n");
    }
}

#else

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
    printf("The program is %lu bytes long\n", program_size);
    rewind(program);

// TODO this needs to be stored in memory
#define MAX_PROGRAM_SIZE 2 * 1028
    uint16_t program_opcodes[MAX_PROGRAM_SIZE];

    if (!program_opcodes)
    {
        fprintf(stderr, "Could not malloc memory");
        return false;
    }

    fread(program_opcodes, sizeof(uint16_t), program_size, program);
    size_t length = program_size / 2;

    printf("Length: %lu\n", length);
    for (int i = 0; i < length; i++)
    {
        printf("0x%04x ", program_opcodes[i]);
    }
    printf("\n");

    printf("\nAfter big->little endian conversion:\n");
    for (int i = 0; i < length; i++)
    {
        program_opcodes[i] = (program_opcodes[i] >> 8) | (program_opcodes[i] << 8);
        printf("0x%04x ", program_opcodes[i]);
    }
    printf("\n");


    printf("Program length: %lu\n", length);
    /* uint16_t program_counter = 0; */
    /* while (program_counter < length) */
    /* { */
    /*     execute_instruction(program_opcodes[program_counter]); */
    /*     program_counter++; */
    /* } */

    memcpy(&memory[0x200], program_opcodes, sizeof(uint16_t) * length);
    program_counter = 0x200;

    printf("After putting in memory:\n");
    for (int i = 0x200; i < 0x200 + (2 * length); i += 2)
    {
        printf("0x%02x%02x ", memory[i+1], memory[i]);
    }
    printf("\n");

    struct timespec start_time, current_time;
    timespec_get(&start_time, TIME_UTC);
    timespec_get(&current_time, TIME_UTC);

    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        /* timespec_get(&current_time, TIME_UTC); */
        /* if (current_time.tv_sec - start_time.tv_sec >= 1) */
		int i = 0;
        {
        /*     start_time = current_time; */
            /* srand(current_time.tv_sec); */
            /* int rx = rand() % 64; */
            /* int ry = rand() % 32; */

            /* memory[0] = 0xF0; */
            /* memory[1] = 0x90; */
            /* memory[2] = 0x90; */
            /* memory[3] = 0x90; */
            /* memory[4] = 0xF0; */

            /* registers.V[0] = rx; */
            /* registers.V[1] = rx; */
			/* uint16_t draw_instruction = create_draw_instruction(0, 1, 5); */
			/* printf("Draw instruction: 0x%04x\n", draw_instruction); */

            /* I = 0; */
			if (i++ < 39)
			{

            /* if (program_counter < 0x200 + (sizeof(uint16_t) * length)) */
            /* { */
                uint16_t instruction = *(uint16_t*)(memory + program_counter);
                execute_instruction(instruction);
                printf("\n");

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
                /* DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY); */
                EndDrawing();
            }
        }
    }

    CloseWindow();
    
    return 0;
}

#endif
