#include <SFML/Graphics.hpp>
#include <SFML/Main.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <fstream>
#include <optional>
#include <string>
#include <ctime>
#include <random>

//skalirana rezolucija x10
const int Screen_width = 640;
const int Screen_height = 320;
const char* ROM = "C:\\Users\\Leo\\Downloads\\Pong (1 player).ch8";
//C:\\Users\\Leo\\Downloads\\Maze (alt) [David Winter, 199x](1).ch8
//C:\\Users\\Leo\\Downloads\\IBM_Logo.ch8
//"C:\\Users\\Leo\\Downloads\\test_opcode.ch8"
//"C:\\Users\\Leo\\Downloads\\Pong (1 player).ch8"

enum class State {
    Running,
    Paused,
    Quit
};

class Chip8 {
public:
    State state;
    unsigned short opcode; //ili opcode [35] jer ih je tolko
    unsigned char memory[4096];
    //registri
    unsigned char R[15];
    unsigned char RF; //zastavica
    unsigned short I;
    unsigned short PC = 0x200;
    //IO ureðaji
    unsigned char pixel_state[32][64];
    unsigned char keypad[16];
    //tajmeri
    unsigned char delay_timer;
    unsigned char sound_timer;
    //stog
    unsigned short stack[16];
    unsigned short sp;

    void initialize() {
        memset(memory, 0, sizeof(memory));
        memset(R, 0, sizeof(R));
        memset(pixel_state, 0, sizeof(pixel_state));
        memset(stack, 0, sizeof(stack));
        //memset(opcode, 0, sizeof(opcode));

        RF = 0;
        PC = 0x200;  // Program counter starts at 0x200
        I = 0;      // Reset index register
        sp = 0;      // Reset stack pointer
        
        // Load fontset
        const unsigned int fontset_starting_adress = 0x50; //ne pocinje na 0 jer je povijesno tam bil mali interpreter
        static const unsigned char chip8_fontset[80] = {
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
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
        };
        for (int i = 0; i < 80; i++) {
            memory[fontset_starting_adress + i] = chip8_fontset[i];
        }

        // Reset timers
        delay_timer = 0;
        sound_timer = 0;
    }

    void loadROM(const char * file){
        std::fstream dat;
        dat.open(file, std::ios::in | std::ios::ate | std::ios::binary);
        if (!dat.is_open()) {
            std::cerr << "\ncant open file " << file;
            return;
        }
        std::streampos size = dat.tellg();
        char* buffer = new char[size];
        dat.seekg(0, std::ios::beg);
        dat.read(buffer, size);

        dat.close();
        dat.clear();

        for (int i = 0; i < size; i++) {
            memory[0x200 + i] = static_cast<unsigned char>(buffer[i]);
            printf("%02X", memory[0x200 + i]);
            if (i % 2 != 0) printf(" ");
            if ((i + 1) % 16 == 0) printf("\n");
        }

        delete[] buffer;

        /*
        datoteèni pokazivaè na kraj da dobiš velièinu datoteke
        alociraj buffer za sve podatke
        datoteèni pokazivaè na poèetak
        uèitaj sadržaj u buffer
        */
    }

    void Cycle() {
    //TODO: fetch -> decode -> execute
    //instrukcije se citaju po 2 bajta
    opcode = memory[PC] << 8 | memory[PC + 1]; //big endian pa prvo uzimamo prvi veci bajt i pomaknemo ga za 8 bitova
    //i onda dodamo low byte na njega s |
    if ((opcode & 0xF000) == 0x6000) {
            std::cout << "\n0x6000: ";
            R[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF);
            //printf("\n%02X ", (opcode & 0x0F00) >> 8);
            //printf("\nprvi registar: %02X , drugi registar: %02X\n", R[0], R[1]);
    }
    if(opcode == 0x00E0) {
            memset(pixel_state, 0, sizeof(pixel_state));
    }
    if ((opcode & 0xF000) == 0xA000) {
            I = (opcode & 0x0FFF); //1 samo ako su oba 1 i s tim prva 4 bita koja nisu adresa pretvori u 0
            //printf("\n%02X\n", I);
            //printf("\n->%02X\n", (opcode & 0xF000)); //PRETVORI U A000 I TAK BUMO RASPOZNAVALI SVE A INSTRUKCIJE BEZ OBZIRA NA ADRESU
    }
    if ((opcode & 0xF000) == 0x1000) {
            PC = opcode & 0x0FFF;
    }
    if((opcode & 0xF000) == 0xD000){
        // uzmemo vrijednosti iz registri
        uint8_t x_coord = R[(opcode & 0x0F00) >> 8] % 64; // pomaknem bitove za 8 mjesta (u mjesto jedinice), % 64 je da se wrapa
        //okolo nakon 64 natrag na 1 a ne na 65
        uint8_t y_coord = R[(opcode & 0x00F0) >> 4] % 32;
        uint8_t height = (opcode & 0x000F);

        RF = 0;

        for (int row = 0; row < height; row++) {
            // detalji o spriteu su na adresi I + row
            uint8_t spriteByte = memory[I + row];

            for (int col = 0; col < 8; col++) {
                //gledamo bit po bit
                if ((spriteByte & (0x80 >> col)) != 0) {

                    //provjeravamo granicu
                    if ((y_coord + row) < 32 && (x_coord + col) < 64) {

                        //provjerimo sudar
                        if (pixel_state[y_coord + row][x_coord + col] == 1) {
                            RF = 1;
                        }

                        //XOR
                        pixel_state[y_coord + row][x_coord + col] ^= 1;
                    }
                }
            }
        }
    }
    if ((opcode & 0xF000) == 0x7000) {
        //Vx += NN. Carry zastavica (VF) se ne mijenja èak i dok doðe do overflowa!!
        R[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
    }
    if ((opcode & 0xF000) == 0x1000) {
        PC = opcode & 0x0FFF;
        return;
    }
    if ((opcode & 0xF000) == 0xC000) {
        std::mt19937 mt(time(nullptr));
        unsigned int rand_num = mt() % 256; //do 255
        R[(opcode & 0x0F00) >> 8] = rand_num & (opcode & 0x00FF);
    }
    if ((opcode & 0xF000) == 0x3000) {
        unsigned int reg_x = (opcode & 0x0F00) >> 8;
        if (R[reg_x] == (opcode & 0x00FF)) {
            PC += 2;
        }
    }
    if ((opcode & 0xF000) == 0x4000) {
        unsigned int reg_x = (opcode & 0x0F00) >> 8;
        if (R[reg_x] != (opcode & 0x00FF)) {
            PC += 2;
        }
    }
    if ((opcode & 0xF000) == 0x8000){
        //8xyO --> x,y registri i O je operacija nad njima
        unsigned int reg_x = (opcode & 0x0F00) >> 8;
        unsigned int reg_y = (opcode & 0x00F0) >> 8;
        unsigned int res = R[reg_x] + R[reg_y];
        switch (opcode & 0x000F) {
        case 0x0000:
            R[reg_x] = R[reg_y];
            break;
        case 0x0001:
            R[reg_x] = R[reg_x] | R[reg_y];
            break;
        case 0x0002:
            R[reg_x] = R[reg_x] & R[reg_y];
            break;
        case 0x0003:
            R[reg_x] = R[reg_x] ^ R[reg_y];
            break;
        case 0x0004:
            if (res > 255) {
                RF = 1;
            }
            else {
                RF = 0;
            }
            R[reg_x] = res & 0xFF;
            break;
        case 0x0005:
            if(R[reg_x] > R[reg_y]){
                RF = 1;
                R[reg_x] = R[reg_x] - R[reg_y];
            }
            else{
                RF = 0;
                R[reg_x] = R[reg_y] - R[reg_x];
            }
            break;
        case 0x0006:
            RF = R[reg_x] & 0x01; //lsb ili 0 ili 1
            R[reg_x] = R[reg_x] >> 1; //podijeli s 2
            break;
        case 0x0007:
            if (R[reg_y] > R[reg_x]) {
                RF = 1;
            }
            else {
                RF = 0;
            }
            R[reg_x] = R[reg_x] - R[reg_y];
            break;
        case 0x000E:
            RF = (R[reg_x] & 0x80) >> 7; //msb ili 0 ili 1
            R[reg_x] = R[reg_x] << 1; //pomnozi s 2
            break;
        }
    }
    if ((opcode & 0xF000) == 0x9000) {
        unsigned int reg_x = (opcode & 0x0F00) >> 8;
        unsigned int reg_y = (opcode & 0x00F0) >> 8;
        if (R[reg_x] != R[reg_y]) {
            PC += 2;
        }
    }
    PC += 2;
    return;
    }
};

static void crtaj_ekran(sf::RenderWindow& window, Chip8& chip8, sf::RectangleShape& pixel) {
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 64; j++) {
            if (chip8.pixel_state[i][j] == 1) {
                pixel.setPosition(static_cast<float>(j) * 10.0f, static_cast<float>(i) * 10.0f);
                window.draw(pixel);
            }
        }
    }
}


int main() {
    bool rom_loaded = false;
    Chip8 chip8;
    chip8.state = State::Running;
    chip8.initialize();
    
    sf::RectangleShape pixel({10.0f, 10.0f});
    pixel.setFillColor(sf::Color::Yellow);

    sf::RenderWindow window(sf::VideoMode(Screen_width, Screen_height), "CHIP-8");
    window.setFramerateLimit(5);

    if (!window.isOpen()) {
        std::cerr << "Error: Could not create window." << std::endl;
        return 1;
    }

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (chip8.state == State::Quit) {
                return 0;
            }
            if(event.type == sf::Event::KeyPressed){
                switch (event.key.code) {
                    case sf::Keyboard::W:
                        std::cerr << "up\n";
                        break;
                    case sf::Keyboard::Q:
                        chip8.state = State::Quit;
                        std::cerr << "gasim\n";
                        break;
                    case sf::Keyboard::I:
                        std::cerr << "informacije" << std::endl;
                        for (int i = 0; i < 4096; i++) {
                            std::cerr << static_cast<unsigned char>(chip8.memory[i]) <<" ,";
                            if (i % 50 == 0) {
                                std::cerr << "|" << std::endl << "|";
                            }
                        }
                        std::cerr << std::endl << ROM << std::endl;
                        break;
                    case sf::Keyboard::L:
                        chip8.loadROM(ROM);
                        rom_loaded = true;
                        break;
                    case sf::Keyboard::R:
                        chip8.PC = 0x200;
                        std::cerr << "\nresetiram PC";
                        break;
                    default:
                        std::cerr << "- - - - - - - - - - - - - - - - - - - - - - - - - - Glavni izbornik - - - - - - - - - - - - - - - - - - - - - - - - - -" << std::endl;
                }
            }
        }
        if (rom_loaded) {
            for (int i = 0; i < 10; i++) {
                chip8.Cycle();
            }
        }
        window.clear(sf::Color::Blue);
        crtaj_ekran(window, chip8, pixel);
        window.display();

    }
    return 0;
}