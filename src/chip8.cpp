#include "chip8.h"

using namespace std;



int main()
{
    sf::RenderWindow window(sf::VideoMode(64 * 10, 32 * 10), "Chip8");

    const char* filepath = "D:/Personal Projects/Projects/CHIP8Emulator/games/Chip-8 Programs/IBM Logo.ch8";

    Chip8 chip8 = Chip8();
    // Due to my limited amount of C++ experience I do not know why pc won't be initialized to 512 in the constructor
    chip8.pc = 512;
    chip8.loadROM(filepath);

    while (window.isOpen())
    {
        chip8.cycle(window);

        sf::Event event;

        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        window.clear();
        chip8.draw(window);
        window.display();
    }

    return 0;
}

void Chip8::draw(sf::RenderWindow& window) {
    for (int i = 0; i < 64 * 32; i++) {
        if (display[i] == 1)
            window.draw(pixels[i]);
    }
}

Chip8::~Chip8() {}

Chip8::Chip8() {
    pc = 512;
    opcode = 0;
    I = 0;
    sptr = 0;
    soundTimer = 0;
    delayTimer = 0;

    // Clear Display
    for (int i = 0; i < 64 * 32; i++) {
        display[i] = 0;
        pixels[i] = sf::RectangleShape(sf::Vector2f(10, 10));
        pixels[i].setPosition(10 * (i % 64), 10 * (i / 64));
    }

    // Clear Stack
    for (int i = 0; i < sizeof(stack); i++) { stack[i] = 0; }

    // Clear Registers 
    for (int i = 0; i < sizeof(registers); i++) { registers[i] = 0; }

    // Clear Memory
    for (int i = 0; i < sizeof(memory); i++) { memory[i] = 0; }

    // Set first 80 places of memory to font information
    for (int i = 0; i < 80; ++i) { memory[i] = chip8_fontset[i]; }
}

void Chip8::cycle(sf::RenderWindow& window){
    // Get opcode
    opcode = memory[pc] << 8 | memory[pc + 1];
    pc += 2;


    int x = (opcode & 0x0F00) >> 8;
    int y = (opcode & 0x00F0) >> 4;
    int N = opcode & 0x000F;
    int NN = (opcode & 0x00FF);

    // Decode opcode
    switch (opcode & 0xF000) { // Decipher opcode from first byte
        case 0xA000:
        {
            I = opcode & 0x0FFF;
        }
        break;

        case 0xB000: 
        {
            pc = registers[0x0] + (opcode & 0x0FFF);
        }
        break;

        case 0xC000:
        {
            srand(time(nullptr));
            int random = rand() & 256;
            registers[x] = NN & random;
        }
        break;

        case 0xD000:
        {
            // Bruh Moment

            uint8_t sprite;
            x = registers[x] % 64;
            y = registers[y] % 32;
            registers[0xF] = 0;

            for (int yOffset = 0; yOffset < N; yOffset++){
                sprite = memory[I + yOffset];
                for (int xOffset = 0; xOffset < 8; xOffset++){
                    if ((sprite & (0x80 >> xOffset)) != 0) {
                        if (display[(x + xOffset + ((y + yOffset) * 64))] == 1) {
                            registers[0xF] = 1;
                        }
                        display[(x + xOffset + ((y + yOffset) * 64))] ^= 1;
                    }
                }
            }
        }

        case 0xE000:
        {
            switch (opcode & 0xF00F) {
            case 0xE00E:
            {
                break;
            }
            }
            break;
        }

        case 0x0000:
        {
            switch (opcode) {
            case 0x00E0:
            {
                for (int i = 0; i < 64 * 32; i++) { display[i] = 0; }
                break;
            }

            case 0x00EE:
            {
                pc = stack[sptr];
                stack[sptr] = 0;
                sptr--;
                break;
            }
            }
            break;
        }

        case 0x1000:
        {
            pc = opcode & 0x0FFF;  
        }
        break;

        case 0x2000:
        {
            stack[sptr] = pc;
            sptr++;
            pc = opcode & 0x0FFF;
        }
        break;

        case 0x3000:
        {
            if (registers[x] == NN){
                pc += 2;
            }
        }
        break;

        case 0x4000:
        {
            if (registers[x] != NN){
                pc += 2;
            }
        }
        break;

        case 0x5000:
        {
            if (registers[x] == registers[y]){
                pc += 2;
            }
        }
        break;

        case 0x6000:
        {
            registers[x] = NN;
        }
        break;

        case 0x7000:
        {
            if (x != 0xF){
                registers[x] += NN;
            }
        }
        break;

        case 0x8000:
        {
            // Multiple opcodes start with 8, deciphered with the last 4 bits 
            switch (opcode & 0xF00F) {
            
            case 0x8000:
            {
                registers[(opcode & 0x0F00) >> 8] = registers[(opcode & 0x00F0) >> 4];
            }
            break;

            case 0x8001:
            {
                registers[x] = registers[x] | registers[y];
            }
            break;

            case 0x8002:
            {
                registers[x] = registers[x] & registers[y]; 
            }
            break;

            case 0x8003:
            {
                registers[x] = registers[x] ^ registers[y]; 
            }
            break;

            case 0x8004:
            {
                int sum = registers[x] + registers[y];
                if (sum > 255){
                    registers[0xF] = 1;
                } else {
                    registers[0xF] = 0;
                }
                registers[x] = sum;
            }
            break;

            case 0x8005:
            {
                int diff = registers[x] - registers[y];

                if (diff >= 0){
                    registers[0xF] = 1;
                } else {
                    registers[0xF] = 0;
                }
                registers[x] = diff;
            }
            break;           

            case 0x8006:
            // this and 0x8008  0x800E are ambigious
            {
                registers[x] = registers[y]; // Optional

                if (registers[x] % 2 == 1) {    // Check if we're right shifting out a 1 
                    registers[0xF] = 1;
                } else {
                    registers[0xF] = 0;
                }
                registers[x] = registers[x] >> 1;
            }
            break;

            case 0x8007:
            {
                int diff = registers[y] - registers[x];

                if (diff >= 0){
                    registers[0xF] = 1;
                } else {
                    registers[0xF] = 0;
                }
                registers[x] = diff;
            }
            break;

            case 0x800E:
            {
                registers[x] = registers[y]; // Optional

                if (registers[x] >> 7 == 1){
                    registers[0xF] = 1;
                } else {
                    registers[0xF] = 0;
                }

                registers[x] = registers[x] << 1;
            }

            }
        }
        break;

        case 0x9000:
        {
            if (registers[x] != registers[y]){
                pc += 2;
            }
        }
        break;

        default:
        {
            //printf("Cannot decipher: 0x%X \n", opcode);
        }
        break;
    }
}

void Chip8::loadROM(char const* file_path) {
        // Open ROM file
        FILE* rom = fopen(file_path, "rb");
        // Get file size
        fseek(rom, 0, SEEK_END);
        long rom_size = ftell(rom);
        rewind(rom);
        // Allocate memory to store rom
        char* rom_buffer = (char*)malloc(sizeof(char) * rom_size);
        // Copy ROM into buffer
        size_t result = fread(rom_buffer, sizeof(char), (size_t)rom_size, rom);

        // Copy buffer to memory
        if ((4096 - 512) > rom_size) {
            for (int i = 0; i < rom_size; ++i) {
                memory[i + 512] = (uint8_t)rom_buffer[i];   // Load into memory starting
                                                            // at 0x200 (=512)
            }
        }
        // Clean up
        fclose(rom);
        free(rom_buffer);
    }
