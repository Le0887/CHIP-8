#include <SFML/Graphics.hpp>
#include <SFML/Main.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <fstream>
#include <optional>
#include <string>

//skalirana rezolucija x10
const int Screen_width = 640;
const int Screen_height = 320;
const char* ROM = "C:\\Users\\Leo\\Downloads\\IBM_Logo.ch8";

enum class State {
    Running,
    Paused,
    Quit
};

class Chip8 {
public:
    State state;
    unsigned short opcode[35];
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
        memset(opcode, 0, sizeof(opcode));

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
            printf("%02X ", static_cast<unsigned char>(buffer[i]));
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
};

static void test_ekrana(sf::RenderWindow& window, Chip8 chip8, sf::RectangleShape pixel){
    window.clear(sf::Color::Black);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 64; j++) {
            if (chip8.pixel_state[i][j] == 1) {
                pixel.setPosition(static_cast<float>(j) * 10.0f, static_cast<float>(i) * 10.0f);
                window.draw(pixel);
            }
        }
    }
    window.display();
}


int main() {
    Chip8 chip8;
    chip8.state = State::Running;
    chip8.initialize();
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 64; j++) {
            if(rand() % 2 == 0){
                chip8.pixel_state[i][j] = 1;
            }
            else {
                chip8.pixel_state[i][j] = 0;
            }
        }
    }
    
    sf::RectangleShape pixel({10.0f, 10.0f});
    pixel.setFillColor(sf::Color::White);

    sf::RenderWindow window(sf::VideoMode(Screen_width, Screen_height), "CHIP-8");
    window.setFramerateLimit(60);

    if (!window.isOpen()) {
        std::cerr << "Error: Could not create window." << std::endl;
        return 1;
    }

    while (window.isOpen()) {
        sf::Event event;
        test_ekrana(window, chip8, pixel);
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
                        break;
                    default:
                        std::cerr << "unknown keycode\n";
                }
            }
        }
    }
    return 0;
}