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

#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "version.h"
#include "memory.h"
#include "general.h"
#include "svga.h"

void *FCEU_malloc(uint32 size)
{
 void *ret;
 ret=malloc(size);
 if(!ret)
  FCEU_PrintError(MSG_ERRAM);
 return ret;
}

void FCEU_free(void *ptr)		// Might do something with this and FCEU_malloc later...
{
 free(ptr);
}

void FASTAPASS(3) FCEU_memmove(void *d, void *s, uint32 l)
{
 memmove(d, s, l);
}
