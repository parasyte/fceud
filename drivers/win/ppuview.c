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

#include "common.h"
#include "..\..\ppuview.h"
#include "..\..\debugger.h"
#include "..\..\svga.h"


HWND hPPUView;

int PPUViewPosX,PPUViewPosY;
uint8 chrcache0[0x1000],chrcache1[0x1000]; //cache CHR, fixes a refresh problem when right-clicking
uint8 *pattern0,*pattern1; //pattern table bitmap arrays
extern uint8 *palette;
static int pindex0=0,pindex1=0;
int PPUViewScanline=0,PPUViewer=0;
int PPUViewSkip=0,PPUViewRefresh;
int mouse_x,mouse_y;

#define PATTERNWIDTH	128
#define PATTERNHEIGHT	128
#define PATTERNBITWIDTH	PATTERNWIDTH*3
#define PATTERNDESTX	10
#define PATTERNDESTY	15
#define ZOOM			2

#define PALETTEWIDTH	32*4*4
#define PALETTEHEIGHT	32*2
#define PALETTEBITWIDTH	PALETTEWIDTH*3
#define PALETTEDESTX	10
#define PALETTEDESTY	327


BITMAPINFO bmInfo;
HDC pDC,TmpDC0,TmpDC1;
HBITMAP TmpBmp0,TmpBmp1;
HGDIOBJ TmpObj0,TmpObj1;

BITMAPINFO bmInfo2;
HDC TmpDC2,TmpDC3;
HBITMAP TmpBmp2,TmpBmp3;
HGDIOBJ TmpObj2,TmpObj3;


void PPUViewDoBlit() {
	int x,y;
	uint8 *pbitmap;

	if (!hPPUView) return;
	if (PPUViewSkip < PPUViewRefresh) {
		PPUViewSkip++;
		return;
	}
	PPUViewSkip=0;

	//draw line seperators on palette
	pbitmap = (palette+PALETTEBITWIDTH*31);
	for (x = 0; x < PALETTEWIDTH*2; x++) {
		*(uint8*)(pbitmap++) = 0;
		*(uint8*)(pbitmap++) = 0;
		*(uint8*)(pbitmap++) = 0;
	}
	pbitmap = (palette-3); //(palette+(32*4*3)-3);
	for (y = 0; y < 64*3; y++) {
		if (!(y%3)) pbitmap += (32*4*3);
		for (x = 0; x < 6; x++) {
			*(uint8*)(pbitmap++) = 0;
		}
		pbitmap += ((32*4*3)-6);
	}

	StretchBlt(pDC,PATTERNDESTX,PATTERNDESTY,PATTERNWIDTH*ZOOM,PATTERNHEIGHT*ZOOM,TmpDC0,0,PATTERNHEIGHT-1,PATTERNWIDTH,-PATTERNHEIGHT,SRCCOPY);
	StretchBlt(pDC,PATTERNDESTX+(PATTERNWIDTH*ZOOM)+1,PATTERNDESTY,PATTERNWIDTH*ZOOM,PATTERNHEIGHT*ZOOM,TmpDC1,0,PATTERNHEIGHT-1,PATTERNWIDTH,-PATTERNHEIGHT,SRCCOPY);

	StretchBlt(pDC,PALETTEDESTX,PALETTEDESTY,PALETTEWIDTH,PALETTEHEIGHT,TmpDC2,0,PALETTEHEIGHT-1,PALETTEWIDTH,-PALETTEHEIGHT,SRCCOPY);
}

void DrawPatternTable(uint8 *bitmap, uint8 *table, uint8 pal) {
	int i,j,x,y,index=0;
	int p=0,tmp;
	uint8 chr0,chr1;
	uint8 *pbitmap = bitmap;

	pal <<= 2;
	for (i = 0; i < 16; i++) {
		for (j = 0; j < 16; j++) {
			for (y = 0; y < 8; y++) {
				chr0 = table[index]; //VPage[table>>10][table];
				chr1 = table[index+8]; //VPage[table>>10][table+8];
				tmp=7;
				for (x = 0; x < 8; x++) {
					p = (chr0>>tmp)&1;
					p |= ((chr1>>tmp)&1)<<1;
					p = PALRAM[p|pal];
					tmp--;

					*(uint8*)(pbitmap++) = palo[p].b;
					*(uint8*)(pbitmap++) = palo[p].g;
					*(uint8*)(pbitmap++) = palo[p].r;
				}
				index++;
				pbitmap += ((PALETTEBITWIDTH>>2)-24);
			}
			index+=8;
			pbitmap -= (((PALETTEBITWIDTH>>2)<<3)-24);
		}
		pbitmap += ((PALETTEBITWIDTH>>2)*7);
	}
}

void UpdatePPUView(int refreshchr) {
	int x,y,i;
	uint8 *pbitmap = palette;

	if (!hPPUView) return;

	if (refreshchr) {
		for (i = 0, x=0x1000; i < 0x1000; i++, x++) {
			chrcache0[i] = VPage[i>>10][i];
			chrcache1[i] = VPage[x>>10][x];
		}
	}

	//draw palettes
	for (y = 0; y < PALETTEHEIGHT; y++) {
		for (x = 0; x < PALETTEWIDTH; x++) {
			i = (((y>>5)<<4)+(x>>5));
			*(uint8*)(pbitmap++) = palo[PALRAM[i]].b;
			*(uint8*)(pbitmap++) = palo[PALRAM[i]].g;
			*(uint8*)(pbitmap++) = palo[PALRAM[i]].r;
		}
	}

	DrawPatternTable(pattern0,chrcache0,pindex0);
	DrawPatternTable(pattern1,chrcache1,pindex1);

	//PPUViewDoBlit();
}

void KillPPUView() {
	//GDI cleanup
	DeleteObject(TmpBmp0);
	SelectObject(TmpDC0,TmpObj0);
	DeleteDC(TmpDC0);
	DeleteObject(TmpBmp1);
	SelectObject(TmpDC1,TmpObj1);
	DeleteDC(TmpDC1);
	DeleteObject(TmpBmp2);
	SelectObject(TmpDC2,TmpObj2);
	DeleteDC(TmpDC2);
	ReleaseDC(hPPUView,pDC);

	DestroyWindow(hPPUView);
	hPPUView=NULL;
	PPUViewer=0;
	PPUViewSkip=0;
}

extern void StopSound(void);

BOOL CALLBACK PPUViewCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	RECT wrect;
	char str[20];

	switch(uMsg) {
		case WM_INITDIALOG:
			SetWindowPos(hwndDlg,0,PPUViewPosX,PPUViewPosY,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);

			//prepare the bitmap attributes
			//pattern tables
			memset(&bmInfo.bmiHeader,0,sizeof(BITMAPINFOHEADER));
			bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmInfo.bmiHeader.biWidth = PATTERNWIDTH;
			bmInfo.bmiHeader.biHeight = PATTERNHEIGHT;
			bmInfo.bmiHeader.biPlanes = 1;
			bmInfo.bmiHeader.biBitCount = 24;

			//palettes
			memset(&bmInfo2.bmiHeader,0,sizeof(BITMAPINFOHEADER));
			bmInfo2.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmInfo2.bmiHeader.biWidth = PALETTEWIDTH;
			bmInfo2.bmiHeader.biHeight = PALETTEHEIGHT;
			bmInfo2.bmiHeader.biPlanes = 1;
			bmInfo2.bmiHeader.biBitCount = 24;

			//create memory dcs
			pDC = GetDC(hwndDlg); // GetDC(GetDlgItem(hwndDlg,101));
			TmpDC0 = CreateCompatibleDC(pDC); //pattern table 0
			TmpDC1 = CreateCompatibleDC(pDC); //pattern table 1
			TmpDC2 = CreateCompatibleDC(pDC); //palettes

			//create bitmaps and select them into the memory dc's
			TmpBmp0 = CreateDIBSection(pDC,&bmInfo,DIB_RGB_COLORS,(void**)&pattern0,0,0);
			TmpObj0 = SelectObject(TmpDC0,TmpBmp0);
			TmpBmp1 = CreateDIBSection(pDC,&bmInfo,DIB_RGB_COLORS,(void**)&pattern1,0,0);
			TmpObj1 = SelectObject(TmpDC1,TmpBmp1);
			TmpBmp2 = CreateDIBSection(pDC,&bmInfo2,DIB_RGB_COLORS,(void**)&palette,0,0);
			TmpObj2 = SelectObject(TmpDC2,TmpBmp2);

			//Refresh Tackbar
			SendDlgItemMessage(hwndDlg,201,TBM_SETRANGE,0,(LPARAM)MAKELONG(0,25));
			SendDlgItemMessage(hwndDlg,201,TBM_SETPOS,1,PPUViewRefresh);

			//Set Text Limit
			SendDlgItemMessage(hwndDlg,102,EM_SETLIMITTEXT,3,0);

			UpdatePPUView(0);
			PPUViewDoBlit();
			PPUViewer=1;
			break;
		case WM_PAINT:
			PPUViewDoBlit();
			break;
		case WM_CLOSE:
		case WM_QUIT:
			KillPPUView();
			break;
		case WM_MOVING:
			StopSound();
			break;
		case WM_MOVE:
			GetWindowRect(hwndDlg,&wrect);
			PPUViewPosX = wrect.left;
			PPUViewPosY = wrect.top;
			break;
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
			mouse_x = GET_X_LPARAM(lParam);
			mouse_y = GET_Y_LPARAM(lParam);
			if (((mouse_x >= PATTERNDESTX) && (mouse_x < (PATTERNDESTX+(PATTERNWIDTH*ZOOM)))) && (mouse_y >= PATTERNDESTY) && (mouse_y < (PATTERNDESTY+(PATTERNHEIGHT*ZOOM)))) {
				if (pindex0 == 7) pindex0 = 0;
				else pindex0++;
			}
			else if (((mouse_x >= PATTERNDESTX+(PATTERNWIDTH*ZOOM)+1) && (mouse_x < (PATTERNDESTX+(PATTERNWIDTH*ZOOM)*2+1))) && (mouse_y >= PATTERNDESTY) && (mouse_y < (PATTERNDESTY+(PATTERNHEIGHT*ZOOM)))) {
				if (pindex1 == 7) pindex1 = 0;
				else pindex1++;
			}
			UpdatePPUView(0);
			PPUViewDoBlit();
			break;
		case WM_MOUSEMOVE:
			mouse_x = GET_X_LPARAM(lParam);
			mouse_y = GET_Y_LPARAM(lParam);
			if (((mouse_x >= PATTERNDESTX) && (mouse_x < (PATTERNDESTX+(PATTERNWIDTH*ZOOM)))) && (mouse_y >= PATTERNDESTY) && (mouse_y < (PATTERNDESTY+(PATTERNHEIGHT*ZOOM)))) {
				mouse_x = (mouse_x-PATTERNDESTX)/(8*ZOOM);
				mouse_y = (mouse_y-PATTERNDESTY)/(8*ZOOM);
				sprintf(str,"Tile: $%X%X",mouse_y,mouse_x);
				SetDlgItemText(hwndDlg,103,str);
				SetDlgItemText(hwndDlg,104,"Tile:");
				SetDlgItemText(hwndDlg,105,"Palettes");
			}
			else if (((mouse_x >= PATTERNDESTX+(PATTERNWIDTH*ZOOM)+1) && (mouse_x < (PATTERNDESTX+(PATTERNWIDTH*ZOOM)*2+1))) && (mouse_y >= PATTERNDESTY) && (mouse_y < (PATTERNDESTY+(PATTERNHEIGHT*ZOOM)))) {
				mouse_x = (mouse_x-(PATTERNDESTX+(PATTERNWIDTH*ZOOM)+1))/(8*ZOOM);
				mouse_y = (mouse_y-PATTERNDESTY)/(8*ZOOM);
				sprintf(str,"Tile: $%X%X",mouse_y,mouse_x);
				SetDlgItemText(hwndDlg,104,str);
				SetDlgItemText(hwndDlg,103,"Tile:");
				SetDlgItemText(hwndDlg,105,"Palettes");
			}
			else if (((mouse_x >= PALETTEDESTX) && (mouse_x < (PALETTEDESTX+PALETTEWIDTH))) && (mouse_y >= PALETTEDESTY) && (mouse_y < (PALETTEDESTY+PALETTEHEIGHT))) {
				mouse_x = (mouse_x-PALETTEDESTX)/32;
				mouse_y = (mouse_y-PALETTEDESTY)/32;
				sprintf(str,"Palette: $%02X",PALRAM[(mouse_y<<4)|mouse_x]);
				SetDlgItemText(hwndDlg,103,"Tile:");
				SetDlgItemText(hwndDlg,104,"Tile:");
				SetDlgItemText(hwndDlg,105,str);
			}
			else {
				SetDlgItemText(hwndDlg,103,"Tile:");
				SetDlgItemText(hwndDlg,104,"Tile:");
				SetDlgItemText(hwndDlg,105,"Palettes");
			}

			break;
		case WM_NCACTIVATE:
			sprintf(str,"%d",PPUViewScanline);
			SetDlgItemText(hwndDlg,102,str);
			break;
		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case EN_UPDATE:
					GetDlgItemText(hwndDlg,102,str,4);
					sscanf(str,"%d",&PPUViewScanline);
					if (PPUViewScanline > 239) PPUViewScanline = 239;
					break;
			}
			break;
		case WM_HSCROLL:
			if (lParam) { //refresh trackbar
				PPUViewRefresh = SendDlgItemMessage(hwndDlg,201,TBM_GETPOS,0,0);
			}
			break;
	}
	return FALSE; //TRUE;
}

void DoPPUView() {
	if (!GI) {
		FCEUD_PrintError("You must have a game loaded before you can use the PPU Viewer.");
		return;
	}
	if (GI->type==GIT_NSF) {
		FCEUD_PrintError("Sorry, you can't use the PPU Viewer with NSFs.");
		return;
	}

	if (!hPPUView) hPPUView = CreateDialog(fceu_hInstance,"PPUVIEW",NULL,PPUViewCallB);
	if (hPPUView) {
		SetWindowPos(hPPUView,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
		UpdatePPUView(0);
		PPUViewDoBlit();
	}
}
