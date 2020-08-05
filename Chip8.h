/******************************************************/
/** A Chip8 emulator in C++, using SDL and Imgui.    **/
/**             © Arjun Nair, 2019                   **/
/*************************************************** **/
/** Reference to the Chip8 architecture:             **/
/** http://devernay.free.fr/hacks/chip8/C8TECH10.HTM **/
/**                                                  **/
/** This emulator implements graphics, audio and     **/
/** keyboard input. There is a simple GUI to select  **/
/** a game from a pre-defined list, as well as to    **/
/** tweak some emulator settings.                    **/
/** The emulator should compile on most platforms,   **/
/** by tweaking the makefile or changing the args    **/
/** to the compile as givein the README.md           **/
/** ************************************************ **/

#pragma once

#include <cstdint>

///Some helper functions to do common bit operations in chip8
#define mask_nnn(o) (o & 0x0fff)         ///Masks the lower 3 nibbles
#define mask_n(o) (o & 0x0f)             ///Masks the lower nibble
#define mask_xh(o) ((o & 0xf000) >> 12)  ///Masks the top most nibble of high byte
#define mask_xl(o) ((o & 0x0f00) >> 8)   ///Masks the lower nibble of high byte
#define mask_yh(o) ((o & 0x00f0) >> 4)   ///Masks the higher nibble of low byte
#define mask_yl(o) (o & 0x000f)          ///Masks the lower nibble of low byte
#define mask_high(o) ((o & 0xff00) >> 8) ///Masks the high byte
#define mask_low(o) (o & 0x00ff)         ///Masks the lower byte

///Describes a Chip8 machine including its memory, registers, and display configuration.
class Chip8
{
public:
  ///The chip8 had just 4k of memory.
  uint8_t Memory[4096];

  ///The chip8 includes a hexadecimal charset in binary form where
  ///each character is of size 5x8 bits.
  uint8_t Charset[80] = {
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

  ///There are 16 general purpose 8-bit registers, which are used for most operations.
  ///The 16th register - V(F) - is a special 'Flag' register and shouldn't be used
  ///by programs directly as it's value is dependent on some instructions.
  uint8_t V[16];

  ///The chip8 has a stack space for 16 16-bit addresses.
  int16_t Stack[16];

  ///The 8-bit stack pointer is used to point to the top of the Stack space.
  int16_t SP;

  ///A 16 bit general purpose register used to store memory addresses. Only 12
  ///bits are actually used.
  int16_t I;

  ///The Program Counter is an internal register and can't be used by chip8 programs.
  int16_t PC;

  ///These 8 bit registers are used as timers. They are auto-decremented @ 60Hz,
  ///when they are non-zero. When ST is non-zero, the chip8 produces a 'tone'.
  ///NOTE: These registers are to be auto-decremented *external* to the chip8.
  uint8_t DT, ST;

  ///Helper variables that aren't part of chip8 definition:
  const int16_t F = 15; // Index to the 16th V register.
  const uint32_t PIXEL_OFF = 0xc8c8c8c8;
  const uint32_t PIXEL_ON = 0x0a0a0a0a;

  ///Defines the 'top' of ROM space. 0x000 to 0x1FF are reserved by the ROM.
  const int16_t ROMTOP = 512;

  ///The original Chip8 specs for the 'shift' instruction required the Vx register
  ///to be the shifted value of Vy, but someone didn't get the memo apparently and
  ///most implementations tend to use the shifted value of Vx instead.
  ///This flag allows for the possibility to use the original specs instead.
  ///TODO: Mention any programs that use this feature.
  bool shiftUsingVY = false;

  ///Another quirk is that some specs call for the I register to be incremented
  ///for LD [I], Vx instruction. When enabled, this flag emulates the behaviour.
  bool incrementIOnLD = false;

  ///Holds the value of the key currently being pressed.
  uint8_t keyPressed;

  ///The display memory of chip8.
  ///TODO: Convert to byte array.
  uint32_t *display;

  Chip8();
  ~Chip8();

  ///Executes one instruction and updates the chip8 state.
  void step();

  ///Loads the chip8 with a program.
  void boot(char program[], int32_t len);
};
