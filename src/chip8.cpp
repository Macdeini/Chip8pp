#include "chip8.h"
#include <chrono>
#include <thread>
using namespace std;



int main()
{
    sf::RenderWindow window(sf::VideoMode(64 * 10, 32 * 10), "Chip8");

    const char* filepath = "D:/Personal Projects/Projects/CHIP8Emulator/games/roms/PONG2";

    Chip8 chip8 = Chip8();
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
        chip8.draw(window);
        chip8.drawFlag = false;
        window.display();
    }
    return 0;
}

void Chip8::draw(sf::RenderWindow& window) {
    for (int i = 0; i < 64 * 32; i++) {
        if (display[i] == 1) {
            pixels[i].setFillColor(sf::Color(255, 255, 255));
        }
        else {
            pixels[i].setFillColor(sf::Color(0, 0, 0));
        }
        window.draw(pixels[i]);
    }
}

Chip8::~Chip8() {}

Chip8::Chip8() : pc(512), opcode(0), I(0), sptr(0), soundTimer(0), delayTimer(0), drawFlag(false) {
    // Clear Display
    for (int i = 0; i < 64 * 32; i++) {
        display[i] = 0;
        pixels[i] = sf::RectangleShape(sf::Vector2f(10, 10));
        pixels[i].setPosition(10 * (i % 64), 10 * (i / 64));
    }

    // Clear Stack
    for (int i = 0; i < 16; i++) { stack[i] = 0; }

    // Clear Registers 
    for (int i = 0; i < 16; i++) { registers[i] = 0; }

    // Clear Memory
    for (int i = 0; i < 4096; i++) { memory[i] = 0; }

    // Set first 80 places of memory to font information
    for (int i = 0; i < 80; ++i) { memory[i] = chip8_fontset[i]; }
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

int getKey() {
    // Keyboard Inputs corresponding to hex values 0x0 - 0xF
    // 1 2 3 4 
    // Q W E R 
    // A S F G 
    // Z X C V 
    // Converting from QWERTY to the Chip8 keypad 
    map<sf::Keyboard::Key, int> keyConvert =
    { {sf::Keyboard::Num1, 0x1}, {sf::Keyboard::Num2, 0x2}, {sf::Keyboard::Num3, 0x3}, {sf::Keyboard::Num4, 0xC},
    {sf::Keyboard::Q, 0x4},     {sf::Keyboard::W, 0x5},    {sf::Keyboard::E, 0x6},    {sf::Keyboard::R, 0xD},
    {sf::Keyboard::A, 0x7},     {sf::Keyboard::S, 0x8},    {sf::Keyboard::D, 0x9},    {sf::Keyboard::F, 0xE},
    {sf::Keyboard::Z, 0xA},     {sf::Keyboard::X, 0x0},    {sf::Keyboard::C, 0xB},    {sf::Keyboard::V, 0xF} };
    for (auto& kv : keyConvert) {
        if (sf::Keyboard::isKeyPressed(kv.first)) {
            return kv.second;
        }
    }
    return 17;
}

void Chip8::cycle(sf::RenderWindow& window) {
    // Get opcode
    opcode = memory[pc] << 8 | memory[pc + 1];
    printf("0x%X %d \n", opcode, pc);
    pc += 2;
    //

    int x = (opcode & 0x0F00) >> 8;
    int y = (opcode & 0x00F0) >> 4;
    int N = opcode & 0x000F;
    int NN = (opcode & 0x00FF);
    int key = 17;

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
        uint8_t sprite;
        x = registers[x] % 64;
        y = registers[y] % 32;
        registers[0xF] = 0;
        for (int yOffset = 0; yOffset < N; yOffset++) {
            sprite = memory[I + yOffset];
            for (int xOffset = 0; xOffset < 8; xOffset++) {
                if ((sprite & (0x80 >> xOffset)) != 0) {
                    if (display[(x + xOffset + ((y + yOffset) * 64))] == 1) {
                        registers[0xF] = 1;
                    }
                    display[(x + xOffset + ((y + yOffset) * 64))] ^= 1;
                }
            }
        }
        drawFlag = true;
    }
    break;

    case 0xE000:
    {
        switch (opcode & 0xF0FF) {
            case 0xE09E:
            {
                key = getKey();
                if (registers[x] == key) {
                    pc += 2;
                }
            }
            break;
            case 0xE0A1:
            {
                key = getKey();
               // printf("key %d and register %d \n", key, registers[x]);
                if (registers[x] != key) {
                    pc += 2;
                }
            }
            break;
        }
    }

    break;

    case 0xF000:
    {
        switch (opcode & 0xF0FF) {
        case 0xF007:
        {
            registers[x] = delayTimer;
        }
        break;
        case 0xF00A:
        {
            // Waiting until key records a valid key press
            key = getKey();
            if (key == 17) {
                pc -= 2;
                return;
            }
            registers[x] = key;
        }
        break;
        case 0xF015:
        {
            delayTimer = registers[x];
        }
        break;
        case 0xF018:
        {
            soundTimer = registers[x];
        }
        break;
        case 0xF01E:
        {
            I += registers[x];
        }
        break;
        case 0xF029:
        {
            I = registers[x] * 5;
        }
        break;
        case 0xF033:
        {
            uint8_t num = registers[x];
            memory[I] = num / 100;
            memory[I + 1] = (num / 10) % 10;
            memory[I + 2] = num % 10;
        }
        break;
        case 0xF055:
        {
            for (int i = 0; i <= x; i++) {
                memory[I + i] = registers[i];
            }
        }
        break;
        case 0xF065:
        {
            for (int i = 0; i <= x; i++) {
                registers[i] = memory[I + i];

            }
        }
        break;
        }
        break;
    }
    case 0x0000:
    {
        switch (opcode) {
        case 0x00E0:
        {
            drawFlag = true;
            for (int i = 0; i < 64 * 32; i++) { display[i] = 0; }
        }
        break;
        case 0x00EE:
        {
            sptr--;
            pc = stack[sptr];
            stack[sptr] = 0;
        }
        break;

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
        if (registers[x] == NN) {
            pc += 2;
        }
    }
    break;

    case 0x4000:
    {
        if (registers[x] != NN) {
            pc += 2;
        }
    }
    break;

    case 0x5000:
    {
        if (registers[x] == registers[y]) {
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
        if (x != 0xF) {
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
            if (sum > 255) {
                registers[0xF] = 1;
            }
            else {
                registers[0xF] = 0;
            }
            registers[x] = sum;
        }
        break;

        case 0x8005:
        {
            int diff = registers[x] - registers[y];

            if (diff >= 0) {
                registers[0xF] = 1;
            }
            else {
                registers[0xF] = 0;
            }
            registers[x] = diff;
        }
        break;

        case 0x8006:
            // this and 0x8008  0x800E are ambigious
        {

            if (registers[x] % 2 == 1) {    // Check if we're right shifting out a 1 
                registers[0xF] = 1;
            }
            else {
                registers[0xF] = 0;
            }
            registers[x] = registers[x] >> 1;
        }
        break;

        case 0x8007:
        {
            int diff = registers[y] - registers[x];

            if (diff >= 0) {
                registers[0xF] = 1;
            }
            else {
                registers[0xF] = 0;
            }
            registers[x] = diff;
        }
        break;

        case 0x800E:
        {

            if (registers[x] >> 7 == 1) {
                registers[0xF] = 1;
            }
            else {
                registers[0xF] = 0;
            }

            registers[x] = registers[x] << 1;
        }
        break;
        }
    }
    break;

    case 0x9000:
    {
        if (registers[x] != registers[y]) {
            pc += 2;
        }
    }
    break;

    default:
    {
        printf("Cannot decipher: 0x%X \n", opcode);
    }
    break;
    }

    if (delayTimer > 0) {
        delayTimer--;
    }
    if (soundTimer > 0) {
        if (soundTimer == 1)
            cout << "Beep" << endl;
        soundTimer--;
    }
}
