#include "chip8.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <csignal>

chip8 mychip8;

void cleanup(int sig) {
  (void)sig;
  mychip8.isRunning = false;
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    printf("Please pass in a ROM to load.\n");
    printf("Usage: ./chip8 path/to/file.chip8\n");
    return 0;
  }

  struct sigaction action;
  action.sa_handler = cleanup;
  action.sa_flags = SA_RESTART;
  sigemptyset(&action.sa_mask);
  int err = sigaction(SIGINT, &action, NULL);
  if (err < 0) {
    perror("sigaction");
    exit(-1);
  }

  if (mychip8.initialize(argv[1]) < 0) {
    mychip8.isRunning = false;
  }
  mychip8.clearScreen();

  while (mychip8.isRunning) {
    // fetch and execute next instruction
    if (mychip8.handleInput() < 0) {
      cleanup(0);
      break;
    }

    const uint64_t start_time = SDL_GetPerformanceCounter();
    // run 8 instructions per frame
    for (int i = 0; i < 8; i++) {
      if (mychip8.emulateCycle() < 0) {
        cleanup(0);
        break;
      }
      // redraw screen if necessary
      if (mychip8.drawFlag) {
        mychip8.drawGraphics();
        mychip8.drawFlag = 0;
      }
    }
    const uint64_t end_time = SDL_GetPerformanceCounter();

    const uint64_t time_spent = (double)((end_time - start_time) * 1000) /
                                SDL_GetPerformanceFrequency();
    SDL_Delay(16.67f > time_spent ? 16.67f - time_spent : 16.67f);
    mychip8.updateTimers();
  }

  mychip8.cleanup();
  return 0;
}
