#include "chip8.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_timer.h>
#include <cstring>
#include <iostream>
#include <portaudio.h>
#include <random>
#include <stdio.h>
#include <stdlib.h>

unsigned char generateRandom();
// void audio_callback(void *, uint8_t, int); // SDL audio
int audio_callback(const void *, void *, unsigned long,
                   const PaStreamCallbackTimeInfo *, PaStreamCallbackFlags,
                   void *);

int audio_callback(const void *input, void *output, unsigned long frameCount,
                   const PaStreamCallbackTimeInfo *timeInfo,
                   PaStreamCallbackFlags statusFlags, void *userData) {
  (void)input;
  (void)timeInfo;
  (void)userData;
  (void)statusFlags;

  int16_t *out = (int16_t *)output;
  static uint32_t running_sample_index = 0;
  const int32_t square_wave_period = SAMPLE_RATE / FREQUENCY;
  const int32_t half_square_wave_period = square_wave_period / 2;

  for (unsigned long i = 0; i < frameCount; i++) {
    out[i] = ((running_sample_index++ / half_square_wave_period) % 2)
                 ? AMPLITUDE
                 : -AMPLITUDE;
  }
  return paContinue;
  /*

  float *out = (float *)output;
  static unsigned long phase = 0;
  unsigned long phaseIncrement =
      (FREQUENCY << 16) / SAMPLE_RATE; // Fixed-point math

  for (unsigned int i = 0; i < frameCount; i++) {
    out[i] = (phase & 0xFFFF) < 0x8000 ? AMPLITUDE : -AMPLITUDE;
    phase += phaseIncrement;
  }
  return paContinue;
  */
}

// callback needs to take in a void *userdata, Uint8 *stream, int len and return
// void
/*void audio_callback(void *userdata, uint8_t *stream, int len) {
  (void)userdata;
  int16_t *audio_data = (int16_t *)stream;
  static uint32_t running_sample_index = 0;
  const int32_t square_wave_period =
      44100 / 440; // sampling rate / square wave freq (hz)
  const int32_t half_square_wave_period = square_wave_period / 2;

  // We are filling out 2 bytes at a time (int16_t), len is in bytes,
  //   so divide by 2
  // If the current chunk of audio for the square wave is the crest of the wave,
  //   this will add the volume, otherwise it is the trough of the wave, and
  //   will add "negative" volume
  for (int i = 0; i < len / 2; i++)
    audio_data[i] =
        ((running_sample_index++ / half_square_wave_period) % 2) ? 3000 : 0;
}*/

int chip8::initialize(char *filename) {
  isRunning = true;
  awaiting_keypress = false;
  pc = PROGRAM_START; // programs start at address 512 (0x200)
  opcode = 0;
  I = 0;
  sp = 0;

  memset(gfx, 0, sizeof(gfx));
  memset(stack, 0, sizeof(stack));
  memset(memory, 0, sizeof(memory));
  memset(V, 0, sizeof(V));

  for (unsigned long i = 0; i < sizeof(chip8_fontset); i++) {
    memory[i + sizeof(chip8_fontset)] = chip8_fontset[i];
  }

  delay_timer = 0;
  sound_timer = 0;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
    SDL_Log("Could not initialize SDL: %s\n", SDL_GetError());
    return -1;
  }

  window = SDL_CreateWindow("Operlaston's CHIP-8 Emulator", 0, 0,
                            SCREEN_WIDTH * SCALE_FACTOR,
                            SCREEN_HEIGHT * SCALE_FACTOR, 0);

  if (window == NULL) {
    SDL_Log("Could not create SDL window: %s\n", SDL_GetError());
    return -1;
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL) {
    SDL_Log("Could not create SDL renderer: %s\n", SDL_GetError());
    return -1;
  }

  // initialize portaudio
  PaError err = Pa_Initialize();
  if (err != paNoError) {
    fprintf(stderr, "an error occurred while initializing portaudio: %s\n",
            Pa_GetErrorText(err));
    return -1;
  }

  // open an audio stream
  PaStreamParameters outputParams;
  // Configure output (mono, 16-bit int, 44100Hz)
  outputParams.device = Pa_GetDefaultOutputDevice();
  outputParams.channelCount = 1;         // Mono (1 channel)
  outputParams.sampleFormat = paInt16;   // 16-bit signed (AUDIO_S16LSB)
  outputParams.suggestedLatency = 0.012; // ~512 samples / 44100Hz = 0.0116s
  outputParams.hostApiSpecificStreamInfo = NULL;
  err = Pa_OpenStream(&stream, NULL, &outputParams, SAMPLE_RATE, 512, paNoFlag,
                      audio_callback, NULL);

  if (err != paNoError) {
    fprintf(stderr, "failed to open audio stream: %s\n", Pa_GetErrorText(err));
    return -1;
  }

  /*
  // Init SDL Audio stuff
  memset(&desired_audio_format, 0, sizeof(desired_audio_format));
  desired_audio_format.freq = 44100; // 44100hz "CD" quality
  desired_audio_format.format = AUDIO_S16LSB;
  desired_audio_format.channels = 1; // Mono, 1 channel
  desired_audio_format.samples = 512;
  desired_audio_format.callback = audio_callback;

  dev = SDL_OpenAudioDevice(NULL, 0, &desired_audio_format,
                            &obtained_audio_format, 0);
  if (dev == 0) {
    fprintf(stderr, "failed to open audio: %s\n", SDL_GetError());
    return -1;
  }

  if ((desired_audio_format.format != obtained_audio_format.format) ||
      (desired_audio_format.channels != obtained_audio_format.channels)) {
    SDL_Log("Could not get desire audio spec\n");
    printf("audio failed\n");
    return -1;
  }*/

  FILE *fp = NULL;
  fp = fopen(filename, "rb");
  if (fp == NULL) {
    std::cout << "failed to open file " << filename << std::endl;
    return -1;
  }

  fseek(fp, 0, SEEK_END);
  unsigned long rom_size = ftell(fp);
  rewind(fp);
  if (rom_size + PROGRAM_START > MEM_SIZE) {
    printf("ROM too large\n");
    fclose(fp);
    return -1;
  }

  unsigned char *rom_memory = memory + PROGRAM_START;
  size_t bytes_read = fread(rom_memory, 1, rom_size, fp);
  if (bytes_read != rom_size) {
    printf("error loading rom into memory\n");
    fclose(fp);
    return -1;
  }

  fclose(fp);
  return 0;
}

int chip8::emulateCycle() {
  // the program needs to be loaded into memory starting at 512 or 0x200 before
  // this fetch opcode
  if (pc < PROGRAM_START || pc >= sizeof(memory)) {
    printf("invalid memory access 0x%X Decimal: %d\n", pc, pc);
    printf("shutting down system\n");
    return -1;
  }

  // drawFlag = 0;
  opcode = memory[pc] << 8 | memory[pc + 1];

  // decode and execute
  bool jump = false;
  switch (opcode & 0xF000) {
  case 0x0000: {
    switch (opcode) {
    // clears the screen
    case 0x00E0: {
      memset(gfx, 0, sizeof(gfx));
      drawFlag = 1;
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
    if (sp >= sizeof(stack) / sizeof(unsigned short)) {
      fprintf(stderr, "stack overflow. last opcode: 0x%X\n", opcode);
      return -1;
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
    } else if (lastDigit == 1) {
      V[x] = V[x] | V[y];
      V[0xf] = 0;
    } else if (lastDigit == 2) {
      V[x] = V[x] & V[y];
      V[0xf] = 0;
    } else if (lastDigit == 3) {
      V[x] = V[x] ^ V[y];
      V[0xf] = 0;
    } else if (lastDigit == 4) {
      int result = V[x] + V[y];
      V[x] = V[x] + V[y];

      // set carry flag
      if (result > 0xFF) {
        V[0xF] = 1;
      } else {
        V[0xF] = 0;
      }
    } else if (lastDigit == 5) {
      int carryflag = 0;
      if (V[x] >= V[y]) {
        carryflag = 1;
      }
      V[x] = V[x] - V[y];
      V[0xF] = carryflag;
    } else if (lastDigit == 6) {
      char lastBit = V[y] & 0x1;
      V[x] = V[y] >> 1;
      V[0xF] = lastBit;
    } else if (lastDigit == 7) {
      int carryflag = 0;
      if (V[y] >= V[x]) {
        carryflag = 1;
      }
      V[x] = V[y] - V[x];
      V[0xF] = carryflag;
    } else if (lastDigit == 0xE) {
      char firstBit = (V[y] & 0x80) >> 7;
      V[x] = V[y] << 1;
      V[0xF] = firstBit;
    } else {
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
    unsigned char base_x = V[(opcode & 0x0F00) >> 8] % SCREEN_WIDTH;
    unsigned char base_y = V[(opcode & 0x00F0) >> 4] % SCREEN_HEIGHT;
    V[0xf] = 0;

    unsigned char n = opcode & 0x000F;

    for (int i = 0; i < n; i++) {
      unsigned char currBits = memory[i + I];
      // sprites drawn have a width of 8 pixels
      unsigned char bitmask = 0x80;
      unsigned char y = (base_y + i);
      if (y >= SCREEN_HEIGHT) {
        break;
      }
      for (int j = 0; j < SPRITE_WIDTH; j++) {
        // wrap out of bounds pixels
        unsigned char x = (base_x + j);
        if (x >= SCREEN_WIDTH) {
          break;
        }

        int position = (y * SCREEN_WIDTH) + x;
        unsigned char currPixel = gfx[position];
        unsigned char currBit = (currBits & bitmask) >> (SPRITE_WIDTH - j - 1);
        gfx[position] = currPixel ^ currBit;
        if (currPixel == 1 && gfx[position] == 0) {
          // set VF register if pixel is flipped from set to unset
          V[0xF] = 1;
        }
        bitmask = bitmask >> 1;
      }
    }

    drawFlag = 1;
    break;
  }

  case 0xE000: {
    unsigned char x = (opcode & 0x0F00) >> 8;
    switch (opcode & 0x000F) {
    case 0x000E: {
      if (key[V[x]] == 1) {
        pc += 2;
      }
      break;
    }

    case 0x0001: {
      if (key[V[x]] == 0) {
        pc += 2;
      }
      break;
    }

    default: {
      printf("not a valid instruction 0x%X\n", opcode);
    }
    }
    break;
  }

  case 0xF000: {
    unsigned char x = (opcode & 0x0F00) >> 8;
    switch (opcode & 0x00FF) {
    case 0x0007: {
      V[x] = delay_timer;
      break;
    }
    case 0x000A: {
      if (!awaiting_keypress) {
        awaiting_keypress = true;
        for (int i = 0; i < 16; i++) {
          saved_key_state[i] = key[i];
        }
      }
      unsigned char keyPressed = 0xff;
      for (int i = 0; i < 16; i++) {
        if (key[i] && !saved_key_state[i]) {
          keyPressed = i;
          awaiting_keypress = false;
          break;
        }
      }

      if (keyPressed ==
          0xff) { // don't advance program counter if no keys pressed
        pc -= 2;
      } else {
        V[x] = keyPressed;
      }
      break;
    }
    case 0x0015: {
      delay_timer = V[x];
      break;
    }
    case 0x0018: {
      sound_timer = V[x];
      break;
    }
    case 0x001E: {
      I += V[x];
      break;
    }
    case 0x0029: {
      unsigned char charSprite = V[x];
      I = FONT_SET_START + (charSprite * 5);
      break;
    }
    case 0x0033: {
      const char num = V[x];
      memory[I] = num / 100;
      memory[I + 1] = (num / 10) % 10;
      memory[I + 2] = num % 10;
      break;
    }
    case 0x0055: {
      // register dump
      for (uint8_t i = 0; i <= x; i++) {
        memory[I++] = V[i];
      }
      break;
    }
    case 0x0065: {
      // register load
      for (uint8_t i = 0; i <= x; i++) {
        V[i] = memory[I++];
      }
      break;
    }
    default: {
      printf("invalid instruction 0x%X\n", opcode);
    }
    }
    break;
  }
  }

  if (!jump) {
    pc += 2;
  }

  // printf("stack pointer is %d\n", sp);

  return 0;
}

void chip8::updateTimers() {
  if (delay_timer > 0) {
    delay_timer--;
  }
  if (sound_timer > 0) {
    sound_timer--;
    Pa_StartStream(stream);
  } else {
    Pa_StopStream(stream);
  }
}

void chip8::setKeys() { return; }

void chip8::clearScreen() {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black full opaque
  if (SDL_RenderClear(renderer) < 0) {
    SDL_Log("Failed to clear screen: %s\n", SDL_GetError());
  }
}

void chip8::drawGraphics() {

  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      SDL_Rect pixel = {
          x * SCALE_FACTOR, // x
          y * SCALE_FACTOR, // y
          SCALE_FACTOR,     // width
          SCALE_FACTOR,     // height
      };
      if (gfx[(y * SCREEN_WIDTH) + x] == 1) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255,
                               255); // white full opaque
      } else {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black full opaque
      }
      SDL_RenderFillRect(renderer, &pixel);
    }
  }

  SDL_RenderPresent(renderer); // update the screen with any renders
}

int chip8::handleInput() {
  SDL_Event event;

  while (SDL_PollEvent(&event)) { // while there is still an event on the queue
    if (event.type == SDL_QUIT) {
      return -1;
    }

    else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
      bool isPressed = event.type == SDL_KEYDOWN;
      switch (event.key.keysym.sym) {
      case SDLK_ESCAPE:
        isRunning = false;
        break;
      case SDLK_1:
        key[0x1] = isPressed;
        break;
      case SDLK_2:
        key[0x2] = isPressed;
        break;
      case SDLK_3:
        key[0x3] = isPressed;
        break;
      case SDLK_q:
        key[0x4] = isPressed;
        break;
      case SDLK_w:
        key[0x5] = isPressed;
        break;
      case SDLK_e:
        key[0x6] = isPressed;
        break;
      case SDLK_a:
        key[0x7] = isPressed;
        break;
      case SDLK_s:
        key[0x8] = isPressed;
        break;
      case SDLK_d:
        key[0x9] = isPressed;
        break;
      case SDLK_x:
        key[0x0] = isPressed;
        break;
      case SDLK_z:
        key[0xa] = isPressed;
        break;
      case SDLK_c:
        key[0xb] = isPressed;
        break;
      case SDLK_4:
        key[0xc] = isPressed;
        break;
      case SDLK_r:
        key[0xd] = isPressed;
        break;
      case SDLK_f:
        key[0xe] = isPressed;
        break;
      case SDLK_v:
        key[0xf] = isPressed;
        break;
      default:
        break;
      }
    }
  }

  return 0;
}

void chip8::cleanup() {
  // SDL_CloseAudioDevice(dev);
  // SDL_QuitSubSystem(SDL_INIT_AUDIO);

  // cleanup port audio
  Pa_StopStream(stream);
  PaError err = Pa_CloseStream(stream);
  if (err != paNoError) {
    fprintf(stderr, "failed to close portaudio: %s\n", Pa_GetErrorText(err));
  }
  err = Pa_Terminate();
  if (err != paNoError) {
    fprintf(stderr, "failed to terminate portaudio: %s\n",
            Pa_GetErrorText(err));
  }

  // cleanup SDL
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

unsigned char generateRandom() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, 255);
  int random_number = dist(gen);
  unsigned char rand = random_number;
  return rand;
}
