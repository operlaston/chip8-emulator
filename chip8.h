// chip8.h

#ifndef CHIP8_H
#define CHIP8_H

#define MEM_SIZE (4096)
#define SCREEN_WIDTH (64)
#define SCREEN_HEIGHT (32)

class chip8 {
  unsigned char memory[MEM_SIZE]; // 4096 bytes of memory total
  unsigned short opcode;          // current instruction
  unsigned char V[16]; // 16 registers V0-VE + 16th register carry flag
  unsigned short I;  // index register used for pointing to operands 0x000-0xFFF
  unsigned short pc; // program counter 0x000-0xFFF
  unsigned char gfx[SCREEN_HEIGHT][SCREEN_WIDTH]; // 64 x 32 screen
  unsigned char delay_timer; // counts at 60hz (60 cycles/sec)
  unsigned char sound_timer; // counts at 60hz as well
  unsigned short stack[16];
  unsigned short sp;     // stack pointer
  unsigned char key[16]; // keep track of state of each key (0x0-0xF)

public:
  void initialize();
  void emulateCycle();
  void setKeys();
  char drawFlag;
};

#endif
