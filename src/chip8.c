#include "debug.h"

unsigned char fonts[80] =
{
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70,
  0xF0, 0x10, 0xF0, 0x80, 0xF0,
  0xF0, 0x10, 0xF0, 0x10, 0xF0,
  0x90, 0x90, 0xF0, 0x10, 0x10,
  0xF0, 0x80, 0xF0, 0x10, 0xF0,
  0xF0, 0x80, 0xF0, 0x90, 0xF0,
  0xF0, 0x10, 0x20, 0x40, 0x40,
  0xF0, 0x90, 0xF0, 0x90, 0xF0,
  0xF0, 0x90, 0xF0, 0x10, 0xF0,
  0xF0, 0x90, 0xF0, 0x90, 0x90,
  0xE0, 0x90, 0xE0, 0x90, 0xE0,
  0xF0, 0x80, 0x80, 0x80, 0xF0,
  0xE0, 0x90, 0x90, 0x90, 0xE0,
  0xF0, 0x80, 0xF0, 0x80, 0xF0,
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void executeOp(C8* c) {
  uint8_t *code = &c->memory[c->pc];
  uint16_t opcode = c->memory[c->pc] << 8 | c->memory[c->pc+1];
  uint8_t firstnib = code[0] >> 4;
  const Uint8 *keys;
  int regx;
  int regy;
  int x;
  int y;
  int i;
  unsigned height;
  unsigned pixel;

  printf("%04x %02x %02x \n", c->pc, code[0], code[1]);

  switch (firstnib) {

      case 0x0:
          switch (code[1]) {
            case 0xe0: //Clear the display.
              memset(c->screen, 0, sizeof(c->screen)); c->pc +=2;
              break;

            case 0xee: //Return from a subroutine.
              c->pc=c->stack[(--c->sp) & 0xf] + 2 ;
              break;

            default: instructionNotImplemented(opcode, c->pc); break;
          }
          break;

      case 0x01: //Jump to location nnn.
        c->pc = opcode & 0x0FFF;
        break;

      case 0x02: //Call subroutine at nnn.
        c->stack[(c->sp++)&0xF] = c->pc;
        c->pc = opcode & 0x0FFF;
        break;

      case 0x03: //Skip next instruction if Vx = kk.
        if(c->V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
          c->pc +=4;
        } else {
          c->pc +=2;
        } break;

      case 0x04: //Skip next instruction if Vx != kk.
        if(c->V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
          c->pc +=4;
        } else {
          c->pc +=2;
        } break;

      case 0x05: //Skip next instruction if Vx = Vy.
        if(c->V[(opcode & 0x0F00) >> 8] == c->V[(opcode & 0x00F0) >> 4]) {
          c->pc +=4;
        } else {
          c->pc +=2;
        }  break;

      case 0x06: //Set Vx = kk.
        c->V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
        c->pc += 2; break;

      case 0x07: //Set Vx = Vx + kk.
        c->V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
        c->pc += 2; break;

      case 0x08:{
        uint8_t lastnib = code[1] &0xf;
        switch(lastnib) {

          case 0x0: //Set Vx = Vy.
            c->V[(opcode & 0x0F00) >> 8] = c->V[(opcode & 0x00F0) >> 4];
            c->pc += 2; break;

          case 0x1: //Set Vx = Vx OR Vy
            c->V[(opcode & 0x0F00) >> 8] = c->V[(opcode & 0x0F00) >> 8] | c->V[(opcode & 0x00F0) >> 4];
            c->pc += 2; break;

          case 0x2: //Set Vx = Vx AND Vy.
            c->V[(opcode & 0x0F00) >> 8] = c->V[(opcode & 0x0F00) >> 8] & c->V[(opcode & 0x00F0) >> 4];
            c->pc += 2; break;

          case 0x3: //Set Vx = Vx XOR Vy
            c->V[(opcode & 0x0F00) >> 8] = c->V[(opcode & 0x0F00) >> 8] ^ c->V[(opcode & 0x00F0) >> 4];
            c->pc += 2; break;

          case 0x4: //Set Vx = Vx + Vy, set VF = carry.
            if(((int)c->V[(opcode & 0x0F00) >> 8] + (int) c->V[(opcode & 0x00F0) >> 4] < 256)) {
              c->V[0xf] &= 0;
            } else {
              c->V[0xf] = 1;
            }
            c->V[(opcode & 0x0F00) >> 8] += c->V[(opcode & 0x00F0) >> 4];
            c->pc += 2; break;

          case 0x5: //Set Vx = Vx - Vy, set VF = NOT borrow.
            if(((int)c->V[(opcode & 0x0F00) >> 8] - (int) c->V[(opcode & 0x00F0) >> 4] >= 0)) {
              c->V[0xf] = 1;
            } else {
              c->V[0xf] &= 0;
            }
            c->V[(opcode & 0x0F00) >> 8] -= c->V[(opcode & 0x00F0 >> 4)];
            c->pc +=2; break;

          case 0x6: //Set Vx = Vx SHR 1.
            c->V[0xf] = c->V[(opcode & 0x0F00) >> 8] & 0x1;
            c->V[(opcode & 0x0F00) >> 8] = c->V[(opcode & 0x0F00) >> 8] >> 1;
            c->pc += 2; break;

          case 0x7: //Set Vx = Vy - Vx, set VF = NOT borrow.
            if((int)c->V[(opcode & 0x0F00) >> 8] > (int)c->V[(opcode & 0x00F0) >> 4]){
              c->V[0xf] &= 0;
            } else {
              c->V[0xf] = 1;
            }
            c->V[(opcode & 0x0F00) >> 8] =  c->V[(opcode & 0x00F0) >> 4] - c->V[(opcode & 0x0F00) >> 8];
            c->pc += 2; break;

          case 0xe: //Set Vx = Vx SHL 1.
            c->V[0xf] = c->V[(opcode & 0x0F00) >> 8] >> 7;
            c->V[(opcode & 0x0F00) >> 8] = c->V[(opcode & 0x0F00) >> 8] << 1;
            c->pc += 2; break;
          default: instructionNotImplemented(opcode, c->pc); break;
        }
      }
      break;

      case 0x09: //Skip next instruction if Vx != Vy.
        if((c->V[opcode & 0x0F00] >> 8) != (c->V[opcode & 0x0F00] >> 4)){
          c->pc += 4;
        } else {
          c-> pc += 2;
        }
        break;

      case 0x0a: //Set I = nnn.
        c->I = opcode & 0x0FFF;
        c->pc +=2;
         break;

      case 0x0b: //Jump to location nnn + V0
        c->pc = (opcode & (0x0FFF)) + c->V[0];
        break;

      case 0x0c:
        c->V[(opcode & 0x0F00) >> 8] = rand()  & (opcode & 0x00FF);
        c->pc +=2;
        break;

      case 0x0d: //Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
        regx = c->V[(opcode & 0x0F00) >> 8];
        regy = c->V[(opcode & 0x00F0) >> 4];
        height = opcode & 0x000F;
        c->V[0xf] &= 0;

        for(y = 0; y < height; y++) {
          pixel = c->memory[c->I + y];
          for(x = 0; x < 8; x++){
            if(pixel & (0x80 >> x)){
              if(c->screen[x + regx + (y + regy) * 64]){
                c->V[0xf] = 1;
              }
              c->screen[x + regx + (y + regy) * 64] ^= 1;
            }
          }
        }
        c->pc += 2;
        break;

      case 0x0e: //Skip next instruction if key with the value of Vx is pressed.
      switch(code[1]){
        case 0x9E: //Skip next instruction if key with the value of Vx is pressed.
        SDL_PumpEvents();
        keys = SDL_GetKeyboardState(NULL);
          if(keys[key_map[c->V[(opcode & 0x0F00) >> 8]]]) {
            c->pc += 4;
          } else {
            c->pc += 2;
          }
          break;

        case 0xA1: //Skip next instruction if key with the value of Vx is not pressed.
          SDL_PumpEvents();
          keys = SDL_GetKeyboardState(NULL);
          if(!keys[key_map[c->V[(opcode & 0x0F00) >> 8]]]) {
            c->pc += 4;
          } else {
            c->pc += 2;
          }
          break;
        default: instructionNotImplemented(opcode, c->pc);break;
      }
      break;

      case 0x0f:
        switch(code[1]){
          case 0x07: //Set Vx = delay timer value.
            c->V[(opcode & 0x0F00) >> 8] = c->delay;
            c->pc += 2;
            break;

          case 0x0a://Wait for a key press, store the value of the key in Vx.
          SDL_PumpEvents();
          keys = SDL_GetKeyboardState(NULL);
            for(i=0; i <0x10; i++){
              if(keys[key_map[i]]){
                c->V[(opcode & 0x0F00) >> 8] = i;
                c->pc +=2;
              }
            }
           break;

          case 0x15: // Set delay timer = Vx.
            c->delay = c->V[(opcode & 0x0F00) >> 8];
            c->pc += 2;
           break;

          case 0x18: //Set sound timer = Vx.
            c->timer = c->V[(opcode & 0x0F00) >> 8];
            c->pc += 2;
          break;

          case 0x1e: //Set I = I + Vx.
            c->I += c->V[(opcode & 0x0F00) >> 8];
            c->pc += 2;
          break;

          case 0x29: // Set I = location of sprite for digit Vx.
            c->I = c->V[(opcode & 0x0F00) >> 8] * 0x5;
            c->pc += 2;
            break;

          case 0x33: //Store BCD representation of Vx in memory locations I, I+1, and I+2.
            c->memory[c->I] = c->V[(opcode & 0x0F00) >> 8] / 100;
            c->memory[c->I+1] = (c->V[(opcode & 0x0F00) >> 8] /10) % 10;
            c->memory[c->I+2] = c->V[(opcode & 0x0F00) >> 8] % 10;
            c->pc +=2;
            break;
          case 0x55: //Store registers V0 through Vx in memory starting at location I.

            for(i=0; i <= (opcode & 0x0F00) >> 8; i++){
                c->memory[c->I+i] = c->V[i];
            }
            c->pc +=2;
            break;

          case 0x65: //Read registers V0 through Vx from memory starting at location I.

            for(i=0; i <= (opcode & 0x0F00) >> 8; i++){
                c->V[i] = c->memory[c->I + i];
            }
            c->pc +=2;
            break;

          default: instructionNotImplemented(opcode, c->pc); break;
        }
        break;
  }

}

void init(FILE *f, C8 * c) {

  c-> memory = calloc(4096, 1);
  c-> sp &= 0;
  c-> pc = 0x200;

  //initialize memory values
  memset(c->screen, 0, sizeof(c->screen));
  memset(c->V, 0, sizeof(c->V));
  memset(c->stack, 0, sizeof(c->stack));

  //Place fonts in portion of memory that  would normally be occupied
  //by the CH8 Interpreter
  for(int i = 0; i < 80; i++){
    c->memory[i] = fonts[i];
  }

  fread(c->memory+0x200, 1, MEM_SIZE-0x200, f);

}

void sdl_draw(C8 *c, C8_display *display) {
  unsigned int pixels[W * H];

  for(int i = 0; i < (W * H); i++){
    unsigned short pixel = c->screen[i];
    pixels[i] = (0x00FFFFFF * pixel) | 0xFF000000;
  }

  SDL_UpdateTexture(display->texture, NULL, pixels, 64*sizeof(Uint32));
  SDL_RenderClear(display->renderer);
  SDL_RenderCopy(display->renderer, display->texture, NULL, NULL);
  SDL_RenderPresent(display->renderer);
  SDL_Delay(2);

}

void process_timers(C8 *c){
    if(c->delay > 0)
      c->delay--;
    if(c->timer > 0)
      c->timer--;
    if(c->timer != 0)
      printf("BEEP\n");
}

int process_keypress(SDL_Event *e){

  const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if(keys[SDL_SCANCODE_ESCAPE])
      return  0;
    if(keys[SDL_SCANCODE_P]) {
       while(1){
         if(SDL_PollEvent(e)){
           if(keys[SDL_SCANCODE_ESCAPE]){
             return 0;
           } else if(keys[SDL_SCANCODE_R]){
             break;
           }
         }

       }
    }
    return 1;
}

int main(int argc, char* argv[]) {

  int run = 1;
  short ticks = 0;
  int debug_flag = 0;
  FILE *f = NULL;
  SDL_Event event;

  //create memory space for Chip 8
  C8 * c = calloc(sizeof(C8), 1);
  C8_display *display = malloc(sizeof(C8_display));

  //OPEN FILE
  if(argc == 2){
      f = fopen(argv[1], "rb");
  } else if (argc == 3) {
      f = fopen(argv[2], "rb");
      if(strcmp(argv[1], "--debug") == 0){
        debug_flag = 1;
        printf("debug_flag set.");
      } else {
        printf("Flags include --debug.\n");
        exit(1);
      }
  }

  if (f == NULL) {
    printf("Error: Could not open %s\n", argv[1]);
    printf("Usage: chip8 <flag> <path/to/rom>");
    exit(1);
  }

  //initialize memory values
  init(f, c);

  if(SDL_Init(SDL_INIT_EVERYTHING) < 0){
      printf("SDL_Init failed: %s\n", SDL_GetError());
      exit(1);
  } else {
      display->window = SDL_CreateWindow("Chip 8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, SDL_WINDOW_OPENGL);
      display->renderer = SDL_CreateRenderer(display->window, -1, SDL_RENDERER_ACCELERATED);
      display->texture = SDL_CreateTexture(display->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, W, H);
  }

  while(run) {
    if(SDL_PollEvent(&event)){
      if(event.type == SDL_QUIT)
        exit(1);
    }

    if(debug_flag){
      dumpReg(c);
      //dumpMem(c);  //<--Remove comment and recompile to enable memory debug dump
    }

    executeOp(c);

    if(ticks == 10){
      process_timers(c);
      ticks = 0;
    }

    ticks++;
    sdl_draw(c, display);
    run = process_keypress(&event);
  }


  fclose(f); // close FILE
  free(c);
  free(display);
  return 0;
}
