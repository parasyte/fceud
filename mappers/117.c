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



DECLFW(Mapper117_write)
{
 switch(A)
 {
  case 0xc003:IRQCount=V;break;
  case 0xc001:IRQa=V;break;
  case 0xa000:VROM_BANK1(0x0000,V);break;
  case 0xa001:VROM_BANK1(0x0400,V);break;
  case 0xa002:VROM_BANK1(0x0800,V);break;
  case 0xa003:VROM_BANK1(0x0c00,V);break;
  case 0xa004:VROM_BANK1(0x1000,V);break;
  case 0xa005:VROM_BANK1(0x1400,V);break;
  case 0xa006:VROM_BANK1(0x1800,V);break;
  case 0xa007:VROM_BANK1(0x1c00,V);break;
  case 0x8000:ROM_BANK8(0x8000,V);break;
  case 0x8001:ROM_BANK8(0xa000,V);break;
  case 0x8002:ROM_BANK8(0xc000,V);break;
  case 0x8003:ROM_BANK8(0xe000,V);break;
 }
}

static void Mapper117_hb(void)
{
 if(IRQa)
 {
        if(IRQCount<=0)
        {
         IRQa=0;
         TriggerIRQ();
        }
        else
        {
         IRQCount--;
        }
 }
}

void Mapper117_init(void)
{
  GameHBIRQHook=Mapper117_hb;
  SetWriteHandler(0x8000,0xffff,Mapper117_write);
}

