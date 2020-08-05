#include "chip8.h"
#include <cstdlib>
#include <ctime>
#include <iostream>

Chip8::Chip8()
{
  for (int32_t i = 0; i < 80; i++)
  {
    Memory[i] = Charset[i];
  }

  for (int32_t i = 80; i < 4096; i++)
  {
    Memory[i] = 0;
  }
}

Chip8::~Chip8() { delete[] display; }

void Chip8::boot(char program[], int32_t len)
{
  for (int32_t i = 0; i < len; i++)
  {
    Memory[ROMTOP + i] = program[i];
  }

  keyPressed = 0xff;
  SP = 0;
  I = 0;
  PC = ROMTOP;
  DT = ST = 0;

  for (int32_t i = 0; i < 16; i++)
  {
    Stack[i] = 0;
    V[i] = 0;
  }

  srand((int32_t)time(0));
  display = new uint32_t[64 * 32];

  for (int32_t i = 0; i < 64 * 32; i++)
  {
    display[i] = PIXEL_OFF;
  }

  std::cout << "Chip 8 initialized\n";
}

void Chip8::step()
{
  int16_t opcode = (Memory[PC] << 8) | Memory[PC + 1]; // Big-endian order
  PC += 2;
  // display[rand() % 200] = rand() % 16384;
  // cache common operations
  int16_t nnn = mask_nnn(opcode);
  uint8_t xh = mask_xh(opcode);
  uint8_t x = mask_xl(opcode);
  uint8_t y = mask_yh(opcode);
  uint8_t kk = mask_low(opcode);
  uint8_t n = mask_n(opcode);

  switch (xh)
  {
    case 0x0:
    {
      switch (opcode)
      {
        case 0x00E0: // CLS
        {
          // clear display
          for (int32_t i = 0; i < 64 * 32; i++)
            display[i] = PIXEL_OFF;
          break;
        }
        case 0x00EE: // RET
        {
          PC = Stack[SP];
          SP--;
          break;
        }
        default:
        {
          //std::cout << "Unknown instruction:" << opcode;
          break;
        }
      }
      break;
    }
    case 0x1: // JP addr
    {
      PC = nnn;
      break;
    }
    case 0x2: // Call addr
    {
      SP++;
      Stack[SP] = PC;
      PC = nnn;
      break;
    }
    case 0x3: // SE Vx, byte
    {
      if (V[x] == kk)
        PC += 2;

      break;
    }
    case 0x4: // SNE Vx, byte
    {
      if (V[x] != kk)
        PC += 2;

      break;
    }
    case 0x5: // SE Vx, Vy
    {
      if (n != 0)
        break;

      if (V[x] == V[y])
        PC += 2;

      break;
    }
    case 0x6: // LD Vx, byte
    {
      V[x] = kk;
      break;
    }
    case 0x7: // ADD Vx, byte
    {
      V[x] += kk;
      break;
    }
    case 0x8:
    {
      switch (n)
      {
        case 0x0: // LD Vx, Vy
        {
          V[x] = V[y];
          break;
        }
        case 0x1: // OR Vx, Vy
        {
          V[x] |= V[y];
          break;
        }
        case 0x2: // AND Vx, Vy
        {
          V[x] &= V[y];
          break;
        }
        case 0x3: // XOR Vx, Vy
        {
          V[x] ^= V[y];
          break;
        }
        case 0x4: // ADD Vx, Vy
        {
          int16_t add = V[x] + V[y];

          if (add > 255)
            V[F] = 1;
          else
            V[F] = 0;

          V[x] = add & 0xff;
          break;
        }
        case 0x5: // SUB Vx, Vy
        {
          if (V[y] > V[x])
            V[F] = 0;
          else
            V[F] = 1;

          V[x] = V[x] - V[y];
          break;
        }
        case 0x6: // SHR Vx {, Vy}
        {
          if (!shiftUsingVY)
          {
            V[F] = V[x] & 0x01;
            V[x] >>= 1;
          }
          else
          {
            V[F] = V[y] & 0x01;
            V[x] = V[y] >> 1;
          }

          break;
        }
        case 0x7: // SUBN Vx, Vy
        {
          if (V[x] > V[y])
            V[F] = 0;
          else
            V[F] = 1;

          V[x] = V[y] - V[x];
          break;
        }
        case 0xE: // SHL Vx {,Vy}
        {
          if (!shiftUsingVY)
          {
            V[F] = ((V[x] & 0x80) >> 7);
            V[x] <<= 1;
          }
          else
          {
            V[F] = ((V[y] & 0x80) >> 7);
            V[x] = V[y] << 1;
          }
          break;
        }
        default:
        {
          //std::cout << "Unknown instruction:" << opcode;
          break;
        }
      }
      break;
    }

    case 0x9: // SNE Vx, Vy
    {
      if (n != 0)
        break;

      if (V[x] != V[y])
        PC += 2;

      break;
    }
    case 0xa: // LD I, addr
    {
      I = nnn;
      break;
    }
    case 0xb: // JP V0 + addr
    {
      PC = (nnn + V[0]) & 0xfff;
      break;
    }
    case 0xc: // RND Vx, byte
    {
      int32_t r = (rand() % 255);
      V[x] = r & kk;
      break;
    }
    case 0xd: // DRW Vx, Vy, nibble
    {
      V[F] = 0;
      for (int32_t i = 0; i < n; i++)
      {
        uint8_t sprite = Memory[I + i];
        int32_t row = (V[y] + i) % 32;

        for (int32_t f = 0; f < 8; f++)
        {
          int32_t b = (sprite & 0x80) >> 7;
          int32_t col = (V[x] + f) % 64;
          int32_t offset = row * 64 + col;

          if (b == 1)
          {
            if (display[offset] != PIXEL_OFF)
            {
              display[offset] = PIXEL_OFF;
              V[F] = 1;
            }
            else
              display[offset] = PIXEL_ON;
          }

          sprite <<= 1;
        }
      }
      break;
    }
    case 0xe:
    {
      switch (kk)
      {
        case 0x9e: // SKP Vx
        {
          if (keyPressed == V[x])
            PC += 2;

          break;
        }
        case 0xA1: // SKNP Vx
        {
          if (keyPressed != V[x])
            PC += 2;

          break;
        }
        default:
        {
          //std::cout << "Unknown instruction:" << opcode;
          break;
        }
      }
      break;
    }
    case 0xf:
    {
      switch (kk)
      {
        case 0x07: // LD Vx, DT
        {
          V[x] = DT;
          break;
        }
        case 0x0a: // LD Vx, K
        {
          if (keyPressed != 0xff)
            V[x] = keyPressed;
          else
            PC -= 2;
          break;
        }
        case 0x15: // LD DT, Vx
        {
          DT = V[x];
          break;
        }
        case 0x18: // LD ST, Vx
        {
          ST = V[x];
          break;
        }
        case 0x1e: // ADD I, Vx
        {
          // From Wikipedia:
          // VF is set to 1 when there is a range overflow (I+VX>0xFFF), and to
          // 0 when there isn't. This is an undocumented feature of the CHIP - 8
          // and used by the Spacefight 2091!game
          if ((I + V[x]) > 0xfff)
            V[F] = 1;
          else
            V[F] = 0;

          I += V[x];
          I &= 0xfff;

          break;
        }
        case 0x29: // LD F, Vx
        {
          I = V[x] * 5;
          I &= 0xfff;
          break;
        }
        case 0x33: // LD B, Vx
        {
          uint8_t bcd = V[x];
          uint8_t unit = bcd % 10;
          bcd = bcd / 10;
          uint8_t tens = bcd % 10;
          bcd = bcd / 10;
          uint8_t hundreds = bcd % 10;
          Memory[I] = hundreds;
          Memory[I + 1] = tens;
          Memory[I + 2] = unit;
          break;
        }
        case 0x55: // LD [I], Vx
        {
          for (int32_t i = 0; i <= x; i++)
            Memory[I + i] = V[i];

          if (incrementIOnLD)
          {
            I += x + 1;
          }
          break;
        }
        case 0x65: // LD Vx, [I]
        {
          for (int32_t i = 0; i <= x; i++)
            V[i] = Memory[I + i];

          if (incrementIOnLD)
          {
            I += x + 1;
          }
          break;
        }
        default:
        {
          //std::cout << "Not implemented: " << opcode;
          break;
        }
      }
      break;
    }
  }
}