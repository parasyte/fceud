/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO
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

static uint8 latche;

static DECLFW(Mapper2_write)
{
	latche=V;
        ROM_BANK16(0x8000,V);
}

void Mapper2_init(void)
{
  SetWriteHandler(0x8000,0xFFFF,Mapper2_write);
  AddExState(&latche, 1, 0, "LATC");
}

static DECLFW(Mapper3_write)
{
        VROM_BANK8(V);
	latche=V;
}

void Mapper3_init(void)
{
	SetWriteHandler(0x8000,0xFFFF,Mapper3_write);
	AddExState(&latche, 1, 0, "LATC");
}

DECLFW(Mapper7_write)
{
      ROM_BANK32(V&0xF);
      onemir((V>>4)&1);
      latche=V;
}

void Mapper7_init(void)
{
        onemir(0);
        SetWriteHandler(0x8000,0xFFFF,Mapper7_write);
	AddExState(&latche, 1, 0, "LATC");
}

DECLFW(Mapper11_write)
{
        ROM_BANK32(V);
        VROM_BANK8(V>>4);
	latche=V;
}

void Mapper11_init(void)
{
        ROM_BANK32(0);
	SetWriteHandler(0x8000,0xFFFF,Mapper11_write);
	AddExState(&latche, 1, 0, "LATC");
}

static DECLFW(Mapper13_write)
{
	setchr4r(0x10,0x1000,V&3);
	setprg32(0x8000,(V>>4)&3);
	latche=V;
}

static void Mapper13_StateRestore(int version)
{
	setchr4r(0x10,0x0000,0);
        setchr4r(0x10,0x1000,latche&3);
        setprg32(0x8000,(latche>>4)&3);
}

void Mapper13_init(void)
{
	SetWriteHandler(0x8000,0xFFFF,Mapper13_write);
	GameStateRestore=Mapper13_StateRestore;
	AddExState(&latche, 1, 0, "LATC");
	AddExState(MapperExRAM, 16384, 0, "CHRR");
	SetupCartCHRMapping(0x10, MapperExRAM, 16384, 1);

	latche=0;
        Mapper13_StateRestore(VERSION_NUMERIC);
}

DECLFW(Mapper34_write)
{
switch(A)
 {
 case 0x7FFD:ROM_BANK32(V);break;
 case 0x7FFE:VROM_BANK4(0x0000,V);break;
 case 0x7fff:VROM_BANK4(0x1000,V);break;
 }
if(A>=0x8000)
 ROM_BANK32(V);
}

void Mapper34_init(void)
{
  SetWriteHandler(0x7ffd,0xffff,Mapper34_write);
}

DECLFW(Mapper66_write)
{
 VROM_BANK8(V&0xF);
 ROM_BANK32((V>>4));
 latche=V;
}

void Mapper66_init(void)
{
 ROM_BANK32(0);
 SetWriteHandler(0x6000,0xffff,Mapper66_write);
 AddExState(&latche, 1, 0, "LATC");
}

DECLFW(Mapper152_write)
{
 ROM_BANK16(0x8000,(V>>4)&0x7);
 VROM_BANK8(V&0xF);
 onemir((V>>7)&1);	/* Saint Seiya...hmm. */
 latche=V;
}

void Mapper152_init(void)
{
 onemir(0);
 SetWriteHandler(0x6000,0xffff,Mapper152_write);
 AddExState(&latche, 1, 0, "LATC");
}

static DECLFW(Mapper70_write)
{
 ROM_BANK16(0x8000,V>>4);
 VROM_BANK8(V&0xF);
 latche=V;
}

void Mapper70_init(void)
{
 SetWriteHandler(0x6000,0xffff,Mapper70_write);
 AddExState(&latche, 1, 0, "LATC");
}
/* Should be two separate emulation functions for this "mapper".  Sigh.  URGE TO KILL RISING. */
static DECLFW(Mapper78_write)
{
 //printf("$%04x:$%02x\n",A,V);
 ROM_BANK16(0x8000,V&0x7);
 VROM_BANK8(V>>4);
 onemir((V>>3)&1);
 latche=V;
}

void Mapper78_init(void)
{
 SetWriteHandler(0x8000,0xffff,Mapper78_write);
 AddExState(&latche, 1, 0, "LATC");
}

DECLFW(Mapper87_write)
{
 VROM_BANK8(V>>1);
 latche=V;
}

void Mapper87_init(void)
{
 SetWriteHandler(0x6000,0xffff,Mapper87_write);
 AddExState(&latche, 1, 0, "LATC");
}

DECLFW(Mapper93_write)
{
  ROM_BANK16(0x8000,V>>4);
  MIRROR_SET(V&1);
  latche=V;
}

void Mapper93_init(void)
{
  SetWriteHandler(0x8000,0xffff,Mapper93_write);
  AddExState(&latche, 1, 0, "LATC");
}


DECLFW(Mapper94_write)
{
 ROM_BANK16(0x8000,V>>2);
 latche=V;
}

void Mapper94_init(void)
{
  SetWriteHandler(0x8000,0xffff,Mapper94_write);
  AddExState(&latche, 1, 0, "LATC");
}

/* I might want to add some code to the mapper 96 PPU hook function
   to not change CHR banks if the attribute table is being accessed,
   if I make emulation a little more accurate in the future.
*/

static uint8 M96LA;
static DECLFW(Mapper96_write)
{
 latche=V;
 setprg32(0x8000,V&3);
 setchr4r(0x10,0x0000,(latche&4)|M96LA);
 setchr4r(0x10,0x1000,(latche&4)|3);
}

static void FP_FASTAPASS(1) M96Hook(uint32 A)
{
 if(A<0x2000)
  return;
 M96LA=(A>>8)&3;
 setchr4r(0x10,0x0000,(latche&4)|M96LA);
}

static void M96Sync()
{
 setprg32(0x8000,latche&3);
 setchr4r(0x10,0x0000,(latche&4)|M96LA);
 setchr4r(0x10,0x1000,(latche&4)|3);
}

void Mapper96_init(void)
{
 SetWriteHandler(0x8000,0xffff,Mapper96_write);
 PPU_hook=M96Hook;
 AddExState(&latche, 1, 0, "LATC");
 AddExState(&M96LA, 1, 0, "LAVA");
 SetupCartCHRMapping(0x10, MapperExRAM, 32768, 1);
 latche=M96LA=0;
 M96Sync();
 GameStateRestore=M96Sync;
}

static DECLFW(Mapper140_write)
{
 VROM_BANK8(V&0xF);
 ROM_BANK32((V>>4)&0xF);
}

void Mapper140_init(void)
{
 ROM_BANK32(0);
 SetWriteHandler(0x6000,0x7FFF,Mapper140_write);
}

static void M185Sync()
{
 int x;

 if(!(mapbyte1[0]&3))
// if(!(mapbyte1[0]==0x21))
 {
  for(x=0;x<8;x++)
   setchr1r(0x10,x<<10,0);
  }
 else
  setchr8(0);
}

static DECLFW(Mapper185_write)
{
 mapbyte1[0]=V;
 M185Sync();
 // printf("Wr: $%04x:$%02x\n",A,V);
}

void Mapper185_init(void)
{
 memset(MapperExRAM,0xFF,1024);
 MapStateRestore=M185Sync;
 mapbyte1[0]=0;
 M185Sync();

 SetupCartCHRMapping(0x10,MapperExRAM,1024,0);
 SetWriteHandler(0x8000,0xFFFF,Mapper185_write);
}
