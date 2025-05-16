// chip8.h

#ifndef CHIP8_H
#define CHIP8_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>

#define MEM_SIZE (4096)
#define PROGRAM_START (0x200)
#define SCREEN_WIDTH (64)
#define SCREEN_HEIGHT (32)
#define SCALE_FACTOR (20)
#define SPRITE_WIDTH (8)
#define FONT_SET_START (80)

typedef enum { QUIT, RUNNING, PAUSED } emulator_state_t;

class chip8 {
  unsigned char memory[MEM_SIZE]; // 4096 bytes of memory total
  unsigned short opcode;          // current instruction
  unsigned char V[16]; // 16 registers V0-VE + 16th register carry flag
  unsigned short I;  // index register used for pointing to operands 0x000-0xFFF
  unsigned short pc; // program counter 0x000-0xFFF
  unsigned char gfx[SCREEN_HEIGHT * SCREEN_WIDTH]; // 64 x 32 screen
  unsigned short stack[16];
  unsigned short sp;         // stack pointer
  bool key[16];              // keep track of state of each key (0x0-0xF)
  unsigned char delay_timer; // counts at 60hz (60 cycles/sec)
  unsigned char sound_timer; // counts at 60hz as well
  bool awaiting_keypress;
  bool saved_key_state[16];
  unsigned char chip8_fontset[80] = {
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
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_AudioSpec desired_audio_format;
  SDL_AudioSpec obtained_audio_format;
  SDL_AudioDeviceID dev;

public:
  int initialize(char *);
  int emulateCycle();
  void setKeys();
  void clearScreen();
  void drawGraphics();
  void cleanup();
  int handleInput();
  void updateTimers();
  char drawFlag;
  bool isRunning;
};

#endif
