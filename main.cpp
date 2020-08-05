#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl2.h"
#include "imgui/imgui_impl_sdl.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_thread.h>

#include <stdio.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "CTexture.h"
#include "Chip8Sound.h"
#include "chip8.h"

namespace fs = std::filesystem;

enum MachineState { UNDEFINED, INIT, RUNNING, PAUSED, STEP, WAIT, FINISHED };
Chip8Sound soundPlayer;
int emulation_speed = 600;
MachineState state = UNDEFINED;

//The window size of this program.
constexpr int SCREEN_WIDTH = 605;
constexpr int SCREEN_HEIGHT = 415;

//This is the size of the actual chip8 display.
constexpr int DISPLAY_WIDTH = 64;
constexpr int DISPLAY_HEIGHT = 32;

constexpr int DISPLAY_X = 0;
constexpr int DISPLAY_Y = 0;

constexpr int MAX_FPS = 60; //Unused 

//Default boot ROM to use for initial boot.
//Simpy prints the word READY to screen.
unsigned char boot_rom[] = 
{
  0x00, //0x200 CLS
  0xe0, 
  0xa2, //0x202 LOAD I, 0x21a - Sprite data address
  0x1a, 
  0x64, //0x204 LOAD r4, 5 - 5 bytes per sprite
  0x05, 
  0x61, //0x206 LOAD r1, 1 - the sprite counter
  0x01, 
  0x62, //0x208 LOAD r2, $10 - X Position
  0x12, 
  0x63, //0x20a LOAD r3, $10 - Y position
  0x0c, 
  0xd2, //0x20c DRAW r2, r3, $5 - Draw sprite
  0x35, 
  0x72, //0x20e ADD r2, 5 - Add 5 to X Position
  0x05, 
  0xf4, //0x210 ADD I, r4 - Move I to next sprite 
  0x1e, 
  0x71, //0x212 ADD r1, 1 - Increment sprite counter
  0x01, 
  0x31, //0x214 SE 1, 6 - Skip next instruction if r1 = 6
  0x06, 
  0x12, //0x216 JUMP to 0x20c
  0x0c, 
  0x12, //0x218 JUMP here - infinite loop
  0x18, 

  //Sprite data - starts at 0x21a
  //R
  0xe0,   //11100000
  0x90,   //10010000
  0xe0,   //11100000
  0x90,   //10010000
  0x90,   //10010000
  //E
  0xf0,   //11110000
  0x80,   //10000000
  0xf0,   //11110000
  0x80,   //10000000
  0xf0,   //11110000
  //A
  0xf0,   //11110000
  0x90,   //10010000
  0xf0,   //11110000
  0x90,   //10010000
  0x90,   //10010000
  //D
  0xe0,   //11100000
  0x90,   //10010000
  0x90,   //10010000
  0x90,   //10010000
  0xe0,   //11100000
  //Y
  0x90,   //10010000
  0x90,   //100100000
  0x60,   //01100000
  0x20,   //00100000
  0x20,   //001000000
};

//Reads a chip8 binary ROm and returns the raw byte data.
//NOTE: Caller must free the memory.
char* read_rom(fs::path p, std::ifstream::pos_type& size) 
{
  std::ifstream rom_file(p, std::ios::in | std::ios::binary | std::ios::ate);
  char* memblock;

  if (rom_file.is_open()) {
    size = rom_file.tellg();
    memblock = new char[size];
    rom_file.seekg(0, std::ios::beg);
    rom_file.read(memblock, size);
    rom_file.close();
  } else {
    std::cout << "Unable to read file: " << p;
    return 0;
  }

  return memblock;
}

//Handles keyboard events.
MachineState handle_event(SDL_Event event, Chip8* chip8_machine) 
{
  //MachineState state = RUNNING;

  if (event.type == SDL_QUIT)
  {
    state = FINISHED;
  }
  else if (event.type == SDL_KEYDOWN) 
  {
    switch (event.key.keysym.sym) 
    {
      case SDLK_0: {
        chip8_machine->keyPressed = 0;
        break;
      }
      case SDLK_1: {
        chip8_machine->keyPressed = 1;
        break;
      }
      case SDLK_2: {
        chip8_machine->keyPressed = 2;
        break;
      }
      case SDLK_3: {
        chip8_machine->keyPressed = 3;
        break;
      }
      case SDLK_4: {
        chip8_machine->keyPressed = 4;
        break;
      }
      case SDLK_5: {
        chip8_machine->keyPressed = 5;
        break;
      }
      case SDLK_6: {
        chip8_machine->keyPressed = 6;
        break;
      }
      case SDLK_7: {
        chip8_machine->keyPressed = 7;
        break;
      }
      case SDLK_8: {
        chip8_machine->keyPressed = 8;
        break;
      }
      case SDLK_9: {
        chip8_machine->keyPressed = 9;
        break;
      }
      case SDLK_a: {
        chip8_machine->keyPressed = 0xa;
        break;
      }
      case SDLK_b: {
        chip8_machine->keyPressed = 0xb;
        break;
      }
      case SDLK_c: {
        chip8_machine->keyPressed = 0xc;
        break;
      }
      case SDLK_d: {
        chip8_machine->keyPressed = 0xd;
        break;
      }
      case SDLK_e: {
        chip8_machine->keyPressed = 0xe;
        break;
      }
      case SDLK_f: {
        chip8_machine->keyPressed = 0xf;
        break;
      }
    }
  } 
  else if (event.type == SDL_KEYUP) 
  {
    switch (event.key.keysym.sym) 
    {
      case SDLK_F2: {
        state = INIT;
        break;
      }
      case SDLK_F6: {
        state = STEP;
        break;
      }
      case SDLK_F5: {
        if (state == WAIT)
        {
          state = RUNNING;
        }
      }
      case SDLK_0: {
        chip8_machine->keyPressed = 0xff;
        break;
      }
      case SDLK_1: {
        chip8_machine->keyPressed = 0xff;
        break;
      }
      case SDLK_2: {
        chip8_machine->keyPressed = 0xff;
        break;
      }
      case SDLK_3: {
        chip8_machine->keyPressed = 0xff;
        break;
      }
      case SDLK_4: {
        chip8_machine->keyPressed = 0xff;
        break;
      }
      case SDLK_5: {
        chip8_machine->keyPressed = 0xff;
        break;
      }
      case SDLK_6: {
        chip8_machine->keyPressed = 0xff;
        break;
      }
      case SDLK_7: {
        chip8_machine->keyPressed = 0xff;
        break;
      }
      case SDLK_8: {
        chip8_machine->keyPressed = 0xff;
        break;
      }
      case SDLK_9: {
        chip8_machine->keyPressed = 0xff;
        break;
      }
      case SDLK_a: {
        chip8_machine->keyPressed = 0xff;
        break;
      }
      case SDLK_b: {
        chip8_machine->keyPressed = 0xff;
        break;
      }
      case SDLK_c: {
        chip8_machine->keyPressed = 0xff;
        break;
      }
      case SDLK_d: {
        chip8_machine->keyPressed = 0xff;
        break;
      }
      case SDLK_e: {
        chip8_machine->keyPressed = 0xff;
        break;
      }
      case SDLK_f: {
        chip8_machine->keyPressed = 0xff;
        break;
      }
      case SDLK_ESCAPE: {
        if (state != PAUSED) {
          std::cout << "Emulation paused...\n";
          state = PAUSED;
        }
        else 
        {
          std::cout << "Emulation resumed.\n";
          state = RUNNING;
        }
      }
    }
  }
  return state;
}

//The chip8 emulation runs in its own thread at the prescribed emulation_speed;
int chip8_thread(void* data) 
{
  int fps = 0;

  unsigned int last_ticks = SDL_GetTicks();
  unsigned int target_ticks = 0;
  unsigned int current_ticks = 0;
  Chip8* chip8_machine = (Chip8*)data;
  //state = RUNNING;
  while (state != FINISHED) 
  {
    if (state != PAUSED) 
    {
      if (state != WAIT)
        chip8_machine->step();
      if (state == STEP)
        state = WAIT;
    }

    fps++;
    float max_frame_ticks = (1000.0 / (float)emulation_speed) + 0.00001;
    target_ticks = last_ticks + (unsigned int)(fps * max_frame_ticks);

    current_ticks = SDL_GetTicks();
    if (current_ticks < target_ticks) 
    {
      SDL_Delay(target_ticks - current_ticks);
      current_ticks = SDL_GetTicks();
    }

    if (current_ticks - last_ticks >= 1000) 
    {
      fps = 0;
      last_ticks = SDL_GetTicks();
    }
  }
  return 0;
}

//Initializes SDL and returns a window handle.
SDL_Window* initialize_sdl()
{
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) 
  {
    return NULL;
  }

  // Setup window
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_DisplayMode current;
  SDL_GetCurrentDisplayMode(0, &current);

  SDL_Window* window = SDL_CreateWindow(
      "Chimp Chip-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      SCREEN_WIDTH, SCREEN_HEIGHT, SDL_RENDERER_ACCELERATED);

  return window;
}

//The main app setup and loop.
int main(int argc, char* argv[]) 
{
  //Initialize SDL first
  SDL_Window* window = initialize_sdl();
  if (window == NULL)
  {
    std::cout << "Error:" << SDL_GetError() << std::endl;
    return -1;
  }
  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  SDL_GL_SetSwapInterval(0);  // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard
  // Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsClassic();
  
  // Setup Platform/Renderer bindings
  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL2_Init();
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  //Initialize the emulator!
  Chip8* chipInstance = new Chip8();

  std::ifstream::pos_type romSize;
  std::string romPath = "./roms";
  const char* defaultRom = "./roms/BLITZ.ch8";
  char* memBlock = NULL;

  std::cout << "Booting..." << std::endl;
  chipInstance->boot((char*)boot_rom, sizeof(boot_rom));
  std::cout << "Ready. Select a ROM." << std::endl;

  soundPlayer.init();

  bool done = false;
  CTexture emuTexture;

  emuTexture.init(chipInstance->display, DISPLAY_WIDTH, DISPLAY_HEIGHT);

  std::string buttonText[16] = {"0", "1", "2", "3", "4", "5", "6", "7",
                                 "8", "9", "A", "B", "C", "D", "E", "F"};

  unsigned int lastTicks = SDL_GetTicks();
  unsigned int targetTicks = 0;
  unsigned int currentTicks = 0;

  struct rom_files 
  {
    fs::path romPath;
    std::string name;
  };

  std::vector<rom_files> romList;
  std::cout << "ROMS:\n";
  for (const auto& entry : fs::directory_iterator(romPath)) 
  {
    rom_files rf;
    rf.romPath = entry.path();
    rf.name = entry.path().filename().string();
    std::string ext = rf.name.substr(rf.name.length() - 4, rf.name.length());

    //ROMS must have .ch8 extension for it to be registered as a valid Chip8 ROM.
    if ((ext == ".ch8") || (ext == ".CH8")) 
    {
      std::cout << "   " <<  rf.name << std::endl;
      romList.push_back(rf);
    }
  }

  SDL_Thread* threadID =
      SDL_CreateThread(chip8_thread, "Chip8CoreThread", (void*)chipInstance);

  state = RUNNING;
  // Main loop
  while (!done) 
  {
    SDL_Event event;

    while (SDL_PollEvent(&event)) 
    {
      ImGui_ImplSDL2_ProcessEvent(&event);
      state = handle_event(event, chipInstance);

      if (state == FINISHED)
      {
        done = true;
      }
      else if (state == INIT)
      {
        state = RUNNING;
        chipInstance->boot(memBlock, romSize);
      }
    }

    // ST and DT are decremented at 60Hz
    if (chipInstance->DT > 0) chipInstance->DT--;

    if (chipInstance->ST > 0) 
    {
      chipInstance->ST--;
      soundPlayer.play_ring_buffer(true);
    } 
    else
      soundPlayer.play_ring_buffer(false);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    //Update the display contents
    emuTexture.update(chipInstance->display);
    emuTexture.render(DISPLAY_X, DISPLAY_Y);

    static float f = 0.0f;
    static int counter = 0;
    static int selected = 0;
    static bool disableMouseWheel = false;
    static bool disableMenu = true;
    static bool useOriginalShiftMethod = false;
    static bool incrementIonLDOperation = false;

    ImGui::SetNextWindowPos(ImVec2(0,0), ImGuiSetCond_Once);
    ImGui::SetNextWindowSize(ImVec2(SCREEN_WIDTH, SCREEN_HEIGHT), ImGuiSetCond_Once);
    ImGui::Begin("Chip-8", 0,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGuiWindowFlags window_flags =
        (disableMouseWheel ? ImGuiWindowFlags_NoScrollWithMouse : 0) |
        (disableMenu ? 0 : ImGuiWindowFlags_MenuBar);

    ImGui::BeginChild("Child1", ImVec2(DISPLAY_WIDTH * 6, 0), false,
                      window_flags);
      ImGui::Image((void*)(intptr_t)emuTexture.get_texture_id(),
                  ImVec2(DISPLAY_WIDTH * 6, DISPLAY_HEIGHT * 6));
      ImGui::Separator();
      if (ImGui::BeginCombo("ROM File", romList[selected].name.c_str())) 
      {
        for (int n = 0; n < romList.size(); n++)
          if (ImGui::Selectable(romList[n].name.c_str(), selected == n)) 
          {
            selected = n;
            delete[] memBlock;
            std::cout << "ROM selected: " << romList[n].name.c_str() << std::endl;
            memBlock = read_rom(romList[n].romPath, romSize);
            chipInstance->boot(memBlock, romSize);
          }
        ImGui::EndCombo();
      }

      ImGui::SliderInt("Emulation Speed", &emulation_speed, 10, 1000);

      if (ImGui::Checkbox("Use Vy for shift operations", &useOriginalShiftMethod))
        chipInstance->shiftUsingVY = useOriginalShiftMethod;

      if (ImGui::Checkbox("Increment I on LD Vx operations", &incrementIonLDOperation))
        chipInstance->incrementIOnLD = incrementIonLDOperation;

      //Hack to align text at bottom of the window.
      ImGui::NewLine();
      ImGui::NewLine();
      ImGui::NewLine();
      
      ImGui::Text("ESC = Pause/Resume.  F2 = Reset. F6 = Step Into.");
      ImGui::NewLine();
      if (state == PAUSED || state == WAIT) 
      {
        ImGui::Text("Emulation PAUSED...");
      }
      else 
      {
        ImGui::Text("Framerate: %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      }
  
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);

    ImGui::BeginChild("Child2", ImVec2(200, 0), true, window_flags);
      ImGui::Columns(2, "mycolumns");
      ImGui::Text("V[x]");
      ImGui::NextColumn();
      ImGui::Text("Stack");
      ImGui::Separator();
      ImGui::NextColumn();
  
      for (int i = 0; i < 16; i++)
        ImGui::Text("V%s: %d", buttonText[i].c_str(), chipInstance->V[i]);

      ImGui::NextColumn();

      for (int i = 0; i < 16; i++) ImGui::Text("%d", chipInstance->Stack[i]);
      ImGui::Columns(1);
      ImGui::Separator();


      ImGui::Text("PC: %d", chipInstance->PC);
      ImGui::Text("SP: %d", chipInstance->SP);
      ImGui::Text("DT: %d", chipInstance->DT);
      ImGui::Text("ST: %d", chipInstance->ST);
      ImGui::Text("I : %d", chipInstance->I);

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::End();

    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui::Render();

    // glUseProgram(0); // You may want this if using this code in an OpenGL 3+
                        // context where shaders may be bound
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);

    // The sound is synchronized at 60Hz, so we will use this to limit the frame
    // rate as well.
    while (soundPlayer.AudioRingBuffer.Length > 0) 
    {
      SDL_Delay(1);
    }
  }

  SDL_WaitThread(threadID, NULL);

  // Cleanup
  delete chipInstance;
  delete[] memBlock;

  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
