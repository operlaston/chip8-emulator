#include "chip8.h"

chip8 mychip8;

int main() {
  mychip8.initialize();
  while (1) {
    // fetch and execute next instruction
    mychip8.emulateCycle();

    // redraw screen if necessary
    if (mychip8.drawFlag) {
    }

    // update key state
    mychip8.setKeys();
  }
  return 0;
}
