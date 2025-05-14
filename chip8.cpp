#include "chip8.h"
#include <cstring>
#include <stdio.h>

void chip8::initialize() {
  pc = 0x200; // programs start at address 512 (0x200)
  opcode = 0;
  I = 0;
  sp = 0;

  memset(gfx, 0, sizeof(gfx));
  memset(stack, 0, sizeof(stack));
  memset(memory, 0, sizeof(memory));
  memset(V, 0, sizeof(V));

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

  for (unsigned long i = 0; i < sizeof(chip8_fontset); i++) {
    memory[i + sizeof(chip8_fontset)] = chip8_fontset[i];
  }

  delay_timer = 0;
  sound_timer = 0;
}

void chip8::emulateCycle() {
  // the program needs to be loaded into memory starting at 512 or 0x200 before
  // this fetch opcode
  opcode = memory[pc] << 8 | memory[pc + 1];

  // decode and execute
  switch (opcode & 0xF000) {
  case 0x0000:
    switch (opcode) {
    // clears the screen
    case 0x00E0:
      memset(gfx, 0, sizeof(gfx));
      break;
    // returns from subroutine
    case 0x00EE:
      pc = stack[--sp];
      break;
    default:
      printf("unknown opcode 0x%X\n", opcode);
    }
    break;

  case 0x1000:
    pc = opcode & 0x0FFF;
    break;

  case 0x2000:
    if (sp >= sizeof(stack)) {
      printf("stack overflow. last opcode: 0x%X\n", opcode);
    } else {
      stack[sp++] = pc;
      pc = opcode & 0x0FFF;
    }
    break;

  case 0x3000:
    break;

  case 0x4000:
    break;

  case 0x5000:
    break;

  case 0x6000:
    break;

  case 0x7000:
    break;

  case 0x8000:
    break;

  case 0x9000:
    break;

  case 0xA000:
    break;

  case 0xB000:
    break;

  case 0xC000:
    break;

  case 0xD000:
    break;

  case 0xE000:
    break;

  case 0xF000:
    break;
  }

  // decrement timers
}

void chip8::setKeys() { return; }
