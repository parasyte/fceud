/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Ben Parnell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mapinc.h"

static void DoNovel(void)
{
 setprg32(0x8000,GameMemBlock[0]&3);
 setchr8(GameMemBlock[0]&7);
}

static DECLFW(NovelWrite)
{
 GameMemBlock[0]=A&0xFF;
 DoNovel();
}

static void NovelReset(void)
{
  SetWriteHandler(0x8000,0xFFFF,NovelWrite);
  SetReadHandler(0x8000,0xFFFF,CartBR);  
  setprg32(0x8000,0);
  setchr8(0);
}

static void NovelRestore(int version)
{
 DoNovel();
}

void Novel_Init(void)
{
  AddExState(&GameMemBlock[0], 1, 0,"L1");
  BoardPower=NovelReset;
  GameStateRestore=NovelRestore;
}
