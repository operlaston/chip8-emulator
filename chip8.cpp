#include "chip8.h"
#include <cstring>
#include <stdio.h>
#include <random>

unsigned char generateRandom();

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
  bool jump = false;
  switch (opcode & 0xF000) {
    case 0x0000: {
      switch (opcode) {
        // clears the screen
        case 0x00E0: {
          memset(gfx, 0, sizeof(gfx));
          break;
        }
        // returns from subroutine
        case 0x00EE: {
          pc = stack[--sp];
          break;
        }
        default: {
          printf("unknown opcode 0x%X\n", opcode);
        }
      }
      break;
    }

    case 0x1000: {
      // jump to address NNN (0x1NNN)
      pc = opcode & 0x0FFF;
      jump = true;
      break;
    }

    case 0x2000: {
      // jump to subroutine NNN (0x2NNN)
      if (sp >= sizeof(stack)) {
        printf("stack overflow. last opcode: 0x%X\n", opcode);
      } else {
        stack[sp++] = pc;
        pc = opcode & 0x0FFF;
        jump = true;
      }
      break;

    }

    case 0x3000: {
      unsigned char x = (opcode & 0x0F00) >> 8;
      unsigned char operand1 = V[x];
      unsigned char operand2 = opcode & 0x00FF;
      if (operand1 == operand2) {
        pc += 2; // skip the next instruction
      }
      break;
    }

    case 0x4000: {
      unsigned char x = (opcode & 0x0F00) >> 8;
      unsigned char operand1 = V[x];
      unsigned char operand2 = opcode & 0x00FF;
      if (operand1 != operand2) {
        pc += 2; // skip the next instruction
      }
      break;
    }

    case 0x5000: {
      unsigned char x = (opcode & 0x0F00) >> 8;
      unsigned char y = (opcode & 0x00F0) >> 4;
      if (V[x] == V[y]) {
        pc += 2; // skip the next instruction
      }
      break;
    }

    case 0x6000: {
      // sets Vx to NN [0x6XNN]
      unsigned char x = (opcode & 0x0F00) >> 8;
      V[x] = opcode & 0x00FF;
      break;
    }

    case 0x7000: {
      // adds NN to Vx [0x7XNN]
      unsigned char x = (opcode & 0x0F00) >> 8;
      V[x] += opcode & 0x00FF;
      break;
    }

    case 0x8000: {
      unsigned char lastDigit = opcode & 0x000F;
      unsigned char x = (opcode & 0x0F00) >> 8;
      unsigned char y = (opcode & 0x00F0) >> 4;
      if (lastDigit == 0) {
        V[x] = V[y];
      }
      else if (lastDigit == 1) {
        V[x] = V[x] | V[y];
      }
      else if (lastDigit == 2) {
        V[x] = V[x] & V[y];
      }
      else if (lastDigit == 3) {
        V[x] = V[x] ^ V[y];
      }
      else if (lastDigit == 4) {
        int result = V[x] + V[y];
        // set carry flag
        if (result > 0xFF) {
          V[0xF] = 1;
        }
        else {
          V[0xF] = 0;
        }
        V[x] = V[x] + V[y];
      }
      else if (lastDigit == 5) {
        if (V[x] >= V[y]) {
          V[0xF] = 1;
        }
        else {
          V[0xF] = 0; // underflow
        }
        V[x] = V[x] - V[y];
      }
      else if (lastDigit == 6) {
        char lastBit = V[x] & 0x1; 
        V[x] = V[x] >> 1;
        V[0xF] = lastBit;
      }
      else if (lastDigit == 7) {
        if (V[y] >= V[x]) {
          V[0xF] = 1;
        }
        else {
          V[0xF] = 0; // underflow
        }
        V[x] = V[y] - V[x];
      }
      else if (lastDigit == 0xE) {
        char firstBit = (V[x] & 0x80) >> 7;
        V[x] = V[x] << 1;
        V[0xF] = firstBit;
      }
      else {
        printf("opcode doesn't exist 0x%X\n", opcode);
      }
      break;
    }

    case 0x9000: {
      unsigned char x = (opcode & 0x0F00) >> 8;
      unsigned char y = (opcode & 0x00F0) >> 4;
      if (V[x] != V[y]) {
        pc += 2; // skip the next instruction
      }
      break;
    }

    case 0xA000: {
      short addr = opcode & 0x0FFF;
      I = addr;
      break;
    }

    case 0xB000: {
      short addr = opcode & 0x0FFF;
      pc = V[0] + addr;
      jump = true;
      break;
    }

    case 0xC000: {
      // RAND
      unsigned char random_number = generateRandom();  
      unsigned char nn = opcode & 0x00FF;
      unsigned char x = (opcode & 0x0F00) >> 8;
      V[x] = random_number & nn;
      break;
    }

    case 0xD000: {
      break;
    }

    case 0xE000: {
      break;
    }

    case 0xF000: {
      break;
    }
  }

  if (!jump) {
    pc += 2;
  }
  // decrement timers
}

void chip8::setKeys() { return; }


unsigned char generateRandom() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, 255);
  int random_number = dist(gen);
  unsigned char rand = random_number;
  return rand;
}
