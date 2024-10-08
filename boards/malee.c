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

DECLFW(MWrite)
{
 (GameMemBlock-0x7000)[A]=V;
}

static void MALEEReset(void)
{
  setprg2r(0x10,0x7000,0);
  SetReadHandler(0x8000,0xFFFF,CartBR);
  SetReadHandler(0x6000,0x67ff,CartBR);
  SetReadHandler(0x7000,0x77FF,CartBR);
  SetWriteHandler(0x7000,0x77FF,MWrite);
  setprg2r(1,0x6000,0);
  setprg32(0x8000,0);
  setchr8(0);
}

void MALEE_Init(void)
{
  AddExState(GameMemBlock, 2048, 0,"RAM");
  SetupCartPRGMapping(0x10,GameMemBlock,2048,1);
  BoardPower=MALEEReset;
}
