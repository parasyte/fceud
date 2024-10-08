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

typedef struct {
                char ID[5]; /*NESM^Z*/
                uint8 Version;
                uint8 TotalSongs;
                uint8 StartingSong;
                uint8 LoadAddressLow;
                uint8 LoadAddressHigh;
                uint8 InitAddressLow;
                uint8 InitAddressHigh;
                uint8 PlayAddressLow;
                uint8 PlayAddressHigh;
                uint8 SongName[32];
                uint8 Artist[32];
                uint8 Copyright[32];
                uint8 NTSCspeed[2];              // Unused
                uint8 BankSwitch[8];
                uint8 PALspeed[2];               // Unused
                uint8 VideoSystem;
                uint8 SoundChip;
                uint8 Expansion[4];
                uint8 reserve[8];
        } NSF_HEADER;
int NSFLoad(int fp);
DECLFW(NSF_write);
DECLFR(NSF_read);
void NSF_init(void);
extern uint8 CurrentSong;
extern uint8 SongReload;
void DrawNSF(uint8 *XBuf);
void NSFControl(int z);
extern NSF_HEADER NSFHeader;
void NSFDealloc(void);
void NSFDodo(void);
