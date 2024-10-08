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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ZLIB
 #include <zlib.h>
 #include "zlib/unzip.h"
#endif

#include "types.h"
#include "file.h"
#include "endian.h"
#include "memory.h"
#include "driver.h"

static void *desctable[8]={0,0,0,0,0,0,0,0};
static int x;

#ifdef ZLIB

typedef struct {
           uint8 *data;
           uint32 size;
           uint32 location;
} ZIPWRAP;

void *MakeZipWrap(void *tz)
{
 unz_file_info ufo;
 ZIPWRAP *tmp;

 if(!(tmp=FCEU_malloc(sizeof(ZIPWRAP))))
  goto doret;
 
 unzGetCurrentFileInfo(tz,&ufo,0,0,0,0,0,0);  

 tmp->location=0;
 tmp->size=ufo.uncompressed_size;
 if(!(tmp->data=FCEU_malloc(ufo.uncompressed_size)))
 {
  tmp=0;
  goto doret;
 }

 unzReadCurrentFile(tz,tmp->data,ufo.uncompressed_size);

 doret:

 unzCloseCurrentFile(tz);
 unzClose(tz);
 
 return tmp;
}
#endif

#ifndef __GNUC__
 #define strcasecmp strcmp
#endif

int FASTAPASS(2) FCEU_fopen(char *path, char *mode)
{
 void *t;

 #ifdef ZLIB
  unzFile tz;
  if((tz=unzOpen(path)))  // If it's not a zip file, use regular file handlers.
			  // Assuming file type by extension usually works,
			  // but I don't like it. :)
  {
   if(unzGoToFirstFile(tz)==UNZ_OK)
   {
    for(;;)
    {
     char tempu[512];	// Longer filenames might be possible, but I don't
		 	// think people would name files that long in zip files...
     unzGetCurrentFileInfo(tz,0,tempu,512,0,0,0,0);
     tempu[511]=0;
     if(strlen(tempu)>=4)
     {
      char *za=tempu+strlen(tempu)-4;
      if(!strcasecmp(za,".nes") || !strcasecmp(za,".fds") ||
         !strcasecmp(za,".nsf") || !strcasecmp(za,".unf") ||
         !strcasecmp(za,".nez"))
       break;
     }
     if(strlen(tempu)>=5)
     {
      if(!strcasecmp(tempu+strlen(tempu)-5,".unif"))
       break;
     }
     if(unzGoToNextFile(tz)!=UNZ_OK)
     { 
      if(unzGoToFirstFile(tz)!=UNZ_OK) goto zpfail;
      break;     
     }
    }
    if(unzOpenCurrentFile(tz)!=UNZ_OK)
     goto zpfail;       
   }
   else
   {
    zpfail:
    unzClose(tz);
    return 0;
   }

   for(x=0;x<8;x++)
    if(!desctable[x])
    {
     if(!(desctable[x]=MakeZipWrap(tz)))
      return(0);
     return((x+1)|0x8000);
    }
  }
#endif

 #ifdef ZLIB
 if((t=fopen(path,"rb")))
 {
  uint32 magic;

  magic=fgetc(t);
  magic|=fgetc(t)<<8;
  magic|=fgetc(t)<<16;

  fclose(t);

  if(magic==0x088b1f)
  {
   if((t=gzopen(path,mode)))
    for(x=0;x<8;x++)
     if(!desctable[x])
     {
      desctable[x]=t;
      return((x+1)|0x4000);
     }
  }
 }
 #endif

  if((t=fopen(path,mode)))
  {
   fseek(t,0,SEEK_SET);
   for(x=0;x<8;x++)
    if(!desctable[x])
    {
     desctable[x]=t;
     return(x+1);
    }
  }
 return 0;
}

int FASTAPASS(1) FCEU_fclose(int stream)
{
 #ifdef ZLIB
 if(stream&0x4000)
 {
  gzclose(desctable[(stream&255)-1]);
  desctable[(stream&255)-1]=0;
 }
 else if(stream&0x8000)
 {
  free(((ZIPWRAP*)desctable[(stream&255)-1])->data);
  free(desctable[(stream&255)-1]);
  desctable[(stream&255)-1]=0;
 }
 else // close zip file
 {
 #endif
  fclose(desctable[stream-1]);
  desctable[stream-1]=0;
 #ifdef ZLIB
 }
 #endif
 return 1;
}

size_t FASTAPASS(3) FCEU_fread(void *ptr, size_t size, size_t nmemb, int stream)
{
 #ifdef ZLIB
 if(stream&0x4000)
 {
  return gzread(desctable[(stream&255)-1],ptr,size*nmemb);
 }
 else if(stream&0x8000)
 { 
  ZIPWRAP *wz;
  uint32 total=size*nmemb;

  wz=(ZIPWRAP*)desctable[(stream&255)-1];
  if(wz->location>=wz->size) return 0;

  if((wz->location+total)>wz->size)
  {
   int ak=wz->size-wz->location;
   memcpy((uint8*)ptr,wz->data+wz->location,ak);
   wz->location=wz->size;
   return(ak/size);
  }
  else
  {
   memcpy((uint8*)ptr,wz->data+wz->location,total);
   wz->location+=total;
   return nmemb;
  }
 }
 else
 {
 #endif
 return fread(ptr,size,nmemb,desctable[stream-1]);
 #ifdef ZLIB
 }
 #endif
}

size_t FASTAPASS(3) FCEU_fwrite(void *ptr, size_t size, size_t nmemb, int stream)
{
 #ifdef ZLIB
 if(stream&0x4000)
 {
  return gzwrite(desctable[(stream&255)-1],ptr,size*nmemb);
 }
 else if(stream&0x8000)
 { 
  return 0;
 }
 else
 #endif
  return fwrite(ptr,size,nmemb,desctable[stream-1]);
}

int FASTAPASS(3) FCEU_fseek(int stream, long offset, int whence)
{
 #ifdef ZLIB
 if(stream&0x4000)
 {
  return gzseek(desctable[(stream&255)-1],offset,whence);
 } 
 else if(stream&0x8000)
 {
  ZIPWRAP *wz;
  wz=(ZIPWRAP*)desctable[(stream&255)-1];

  switch(whence)
  {
   case SEEK_SET:if(offset>=wz->size)
                  return(-1);
                 wz->location=offset;break;
   case SEEK_CUR:if(offset+wz->location>wz->size)
                  return (-1);
                 wz->location+=offset;
                 break;
  }    
  return 0;
 }
 else
 #endif
  return fseek(desctable[stream-1],offset,whence);
}

long FASTAPASS(1) FCEU_ftell(int stream)
{
 #ifdef ZLIB
 if(stream&0x4000)
 {
  return gztell(desctable[(stream&255)-1]);
 }
 else if(stream&0x8000)
 { 
  return (((ZIPWRAP *)desctable[(stream&255)-1])->location);  
 }
 else
 #endif
  return ftell(desctable[stream-1]);
}

void FASTAPASS(1)FCEU_rewind(int stream)
{
 #ifdef ZLIB
 if(stream&0x4000)
 {
  gzrewind(desctable[(stream&255)-1]);
 }
 else if(stream&0x8000)
 {
  ((ZIPWRAP *)desctable[(stream&255)-1])->location=0;
 }
 else
 #endif
 #ifdef _WIN32_WCE
  fseek(desctable[stream-1],0,SEEK_SET);
 #else
  rewind(desctable[stream-1]);
 #endif 
}

int FASTAPASS(2) FCEU_read32(void *Bufo, int stream)
{
 #ifdef ZLIB
 if(stream&0xC000)
 { 
  uint8 t[4];
  #ifndef LSB_FIRST
  uint8 x[4];
  #endif
  if(stream&0x8000)
  { 
   ZIPWRAP *wz;
   wz=(ZIPWRAP*)desctable[(stream&255)-1];
   if(wz->location+4>wz->size)
    {return 0;}
   *(uint32 *)t=*(uint32 *)(wz->data+wz->location);
   wz->location+=4;
  }
  else if(stream&0x4000)
   gzread(desctable[(stream&255)-1],&t,4);
  #ifndef LSB_FIRST
  x[0]=t[3];
  x[1]=t[2];
  x[2]=t[1];
  x[3]=t[0];
  *(uint32*)Bufo=*(uint32*)x;
  #else
  *(uint32*)Bufo=*(uint32*)t;
  #endif
  return 1;
 }
 else
 #endif
 {
  return read32(Bufo,desctable[stream-1]);
 }
}

int FASTAPASS(1) FCEU_fgetc(int stream)
{
 #ifdef ZLIB
 if(stream&0x4000)
  return gzgetc(desctable[(stream&255)-1]); 
 else if(stream&0x8000)
 {
  ZIPWRAP *wz;
  wz=(ZIPWRAP*)desctable[(stream&255)-1];
  if(wz->location<wz->size)
   return wz->data[wz->location++];
  return EOF;
 }
 else
#endif
  return fgetc(desctable[stream-1]);
}

long FASTAPASS(1) FCEU_fgetsize(int stream)
{
 #ifdef ZLIB
 if(stream&0x4000)
 {
  int x,t;
  t=gztell(desctable[(stream&255)-1]);
  gzrewind(desctable[(stream&255)-1]);
  for(x=0;gzgetc(desctable[(stream&255)-1]) != EOF; x++);
  gzseek(desctable[(stream&255)-1],t,SEEK_SET);
  return(x);
 }
 else if(stream&0x8000)
  return ((ZIPWRAP*)desctable[(stream&255)-1])->size;
 else
 #endif
 {
  long t,r;
  t=ftell(desctable[stream-1]);
  fseek(desctable[stream-1],0,SEEK_END);
  r=ftell(desctable[stream-1]);
  fseek(desctable[stream-1],t,SEEK_SET);
  return r;
 }
}

int FASTAPASS(1) FCEU_fisarchive(int stream)
{
 #ifdef ZLIB
 if(stream&0x8000)
  return 1;
 #endif
 return 0;
}
