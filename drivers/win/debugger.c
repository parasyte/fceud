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
#include "..\..\debugger.h"
#include "..\..\x6502.h"
#include "..\..\fce.h"
#include "..\..\nsf.h"


extern readfunc ARead[0x10000];
int DbgPosX,DbgPosY;
int WP_edit=-1;
int ChangeWait=0,ChangeWait2=0;
uint8 debugger_open=0;
HWND hDebug;
static HFONT hFont,hNewFont;
SCROLLINFO si;

int iaPC;

watchpointinfo watchpoint[65]; //64 watchpoints, + 1 reserved for step over

int step,stepout,jsrcount;
int childwnd,numWPs;


uint8 GetMem(uint16 A) {
	if ((A >= 0x2000) && (A < 0x4000)) {
		switch (A&7) {
			case 0: return PPU[0];
			case 1: return PPU[1];
			case 2: return PPU[2]|(PPUGenLatch&0x1F);
			case 3: return PPU[3];
			case 4: return SPRAM[PPU[3]];
			case 5: return XOffset;
			case 6: return RefreshAddr&0xFF;
			case 7: return VRAMBuffer;
		}
	}
	else if ((A >= 0x4000) && (A < 0x5FFF)) return 0xFF; //fix me
	return ARead[A](A);
}

uint8 GetPPUMem(uint8 A) {
	uint16 tmp=RefreshAddr&0x3FFF;

	if (tmp<0x2000) return VPage[tmp>>10][tmp];
	if (tmp>=0x3F00) return PALRAM[tmp&0x1F];
	return vnapage[(tmp>>10)&0x3][tmp&0x3FF];
}


BOOL CenterWindow(HWND hwndDlg) {
    HWND hwndParent;
    RECT rect, rectP;
    int width, height;
    int screenwidth, screenheight;
    int x, y;

    //move the window relative to its parent
    hwndParent = GetParent(hwndDlg);

    GetWindowRect(hwndDlg, &rect);
    GetWindowRect(hwndParent, &rectP);

    width  = rect.right  - rect.left;
    height = rect.bottom - rect.top;

    x = ((rectP.right-rectP.left) -  width) / 2 + rectP.left;
    y = ((rectP.bottom-rectP.top) - height) / 2 + rectP.top;

    screenwidth  = GetSystemMetrics(SM_CXSCREEN);
    screenheight = GetSystemMetrics(SM_CYSCREEN);

    //make sure that the dialog box never moves outside of the screen
    if(x < 0) x = 0;
    if(y < 0) y = 0;
    if(x + width  > screenwidth)  x = screenwidth  - width;
    if(y + height > screenheight) y = screenheight - height;

    MoveWindow(hwndDlg, x, y, width, height, FALSE);

    return TRUE;
}

int NewBreak(HWND hwndDlg, int num, int enable) {
	unsigned int brk=0,ppu=0,sprite=0;
	char str[5];

	GetDlgItemText(hwndDlg,200,str,5);
	if (IsDlgButtonChecked(hwndDlg,106) == BST_CHECKED) ppu = 1;
	if (IsDlgButtonChecked(hwndDlg,107) == BST_CHECKED) sprite = 1;
	if ((!ppu) && (!sprite)) {
		if (GI->type == GIT_NSF) { //NSF Breakpoint keywords
			if (strcmp(str,"LOAD") == 0) brk = (NSFHeader.LoadAddressLow | (NSFHeader.LoadAddressHigh<<8));
			if (strcmp(str,"INIT") == 0) brk = (NSFHeader.InitAddressLow | (NSFHeader.InitAddressHigh<<8));
			if (strcmp(str,"PLAY") == 0) brk = (NSFHeader.PlayAddressLow | (NSFHeader.PlayAddressHigh<<8));
		}
		else if (GI->type == GIT_FDS) { //FDS Breakpoint keywords
			if (strcmp(str,"NMI1") == 0) brk = (GetMem(0xDFF6) | (GetMem(0xDFF7)<<8));
			if (strcmp(str,"NMI2") == 0) brk = (GetMem(0xDFF8) | (GetMem(0xDFF9)<<8));
			if (strcmp(str,"NMI3") == 0) brk = (GetMem(0xDFFA) | (GetMem(0xDFFB)<<8));
			if (strcmp(str,"RST") == 0) brk = (GetMem(0xDFFC) | (GetMem(0xDFFD)<<8));
			if ((strcmp(str,"IRQ") == 0) || (strcmp(str,"BRK") == 0)) brk = (GetMem(0xDFFE) | (GetMem(0xDFFF)<<8));
		}
		else { //NES Breakpoint keywords
			if ((strcmp(str,"NMI") == 0) || (strcmp(str,"VBL") == 0)) brk = (GetMem(0xFFFA) | (GetMem(0xFFFB)<<8));
			if (strcmp(str,"RST") == 0) brk = (GetMem(0xFFFC) | (GetMem(0xFFFD)<<8));
			if ((strcmp(str,"IRQ") == 0) || (strcmp(str,"BRK") == 0)) brk = (GetMem(0xFFFE) | (GetMem(0xFFFF)<<8));
		}
	}
	if ((brk == 0) && (sscanf(str,"%04x",&brk) == EOF)) return 1;
	if ((ppu) && (brk > 0x3FFF)) brk &= 0x3FFF;
	if ((sprite) && (brk > 0x00FF)) brk &= 0x00FF;
	watchpoint[num].address = brk;

	watchpoint[num].endaddress = 0;
	GetDlgItemText(hwndDlg,201,str,5);
	sscanf(str,"%04x",&brk);
	if ((ppu) && (brk > 0x3FFF)) brk &= 0x3FFF;
	if ((sprite) && (brk > 0x00FF)) brk &= 0x00FF;
	if ((brk != 0) && (watchpoint[num].address < brk)) watchpoint[num].endaddress = brk;

	watchpoint[num].flags = 0;
	if (enable) watchpoint[num].flags|=WP_E;
	if (IsDlgButtonChecked(hwndDlg,102) == BST_CHECKED) watchpoint[num].flags|=WP_R;
	if (IsDlgButtonChecked(hwndDlg,103) == BST_CHECKED) watchpoint[num].flags|=WP_W;
	if (IsDlgButtonChecked(hwndDlg,104) == BST_CHECKED) watchpoint[num].flags|=WP_X;
	if (ppu) {
		watchpoint[num].flags|=BT_P;
		watchpoint[num].flags&=~WP_X; //disable execute flag!
	}
	if (sprite) {
		watchpoint[num].flags|=BT_S;
		watchpoint[num].flags&=~WP_X; //disable execute flag!
	}

	return 0;
}

int AddBreak(HWND hwndDlg) {
	if (numWPs == 64) return 1;
	if (NewBreak(hwndDlg,numWPs,1)) return 2;
	numWPs++;
	return 0;
}

BOOL CALLBACK AddbpCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char str[8]={0};
	int tmp;

	switch(uMsg) {
		case WM_INITDIALOG:
			CenterWindow(hwndDlg);
			SendDlgItemMessage(hwndDlg,200,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,201,EM_SETLIMITTEXT,4,0);
			if (WP_edit >= 0) {
				SetWindowText(hwndDlg,"Edit Breakpoint...");

				sprintf(str,"%04X",watchpoint[WP_edit].address);
				SetDlgItemText(hwndDlg,200,str);
				sprintf(str,"%04X",watchpoint[WP_edit].endaddress);
				if (strcmp(str,"0000") != 0) SetDlgItemText(hwndDlg,201,str);
				if (watchpoint[WP_edit].flags&WP_R) CheckDlgButton(hwndDlg, 102, BST_CHECKED);
				if (watchpoint[WP_edit].flags&WP_W) CheckDlgButton(hwndDlg, 103, BST_CHECKED);
				if (watchpoint[WP_edit].flags&WP_X) CheckDlgButton(hwndDlg, 104, BST_CHECKED);

				if (watchpoint[WP_edit].flags&BT_P) {
					CheckDlgButton(hwndDlg, 106, BST_CHECKED);
					EnableWindow(GetDlgItem(hwndDlg,104),FALSE);
				}
				else if (watchpoint[WP_edit].flags&BT_S) {
					CheckDlgButton(hwndDlg, 107, BST_CHECKED);
					EnableWindow(GetDlgItem(hwndDlg,104),FALSE);
				}
				else CheckDlgButton(hwndDlg, 105, BST_CHECKED);
			}
			else CheckDlgButton(hwndDlg, 105, BST_CHECKED);
			break;
		case WM_CLOSE:
		case WM_QUIT:
			break;
		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch(LOWORD(wParam)) {
						case 100:
							if (WP_edit >= 0) {
								NewBreak(hwndDlg,WP_edit,(BOOL)(watchpoint[WP_edit].flags&WP_E));
								EndDialog(hwndDlg,1);
								break;
							}
							if ((tmp=AddBreak(hwndDlg)) == 1) {
								MessageBox(hwndDlg, "Too many breakpoints, please delete one and try again", "Breakpoint Error", MB_OK);
								goto endaddbrk;
							}
							if (tmp == 2) goto endaddbrk;
							EndDialog(hwndDlg,1);
							break;
						case 101:
							endaddbrk:
							EndDialog(hwndDlg,0);
							break;
						case 105: //CPU Mem
							EnableWindow(GetDlgItem(hwndDlg,104),TRUE);
							break;
						case 106: //PPU Mem
						case 107: //Sprtie Mem
							EnableWindow(GetDlgItem(hwndDlg,104),FALSE);
							break;
					}
					break;
			}
        	break;
	}
	return FALSE; //TRUE;
}


char *BinToASM(int addr, uint8 *opcode) {
	static char str[64]={0},chr[5]={0};
	uint16 tmp,tmp2;

	switch (opcode[0]) {
		#include "dasm.h"
	}
	return str;
}

void Disassemble(unsigned int addr) {
	char str[1024]={0},chr[20]={0};
	int size,i,j;
	uint8 opcode[3];

	si.nPos = addr;
	SetScrollInfo(GetDlgItem(hDebug,301),SB_CTL,&si,TRUE);

	for (i = 0; i < 20; i++) {
		if (addr > 0xFFFF) break;
		sprintf(chr, "$%04X:", addr);
		strcat(str,chr);
		if ((size = opsize[GetMem(addr)]) == 0) {
			sprintf(chr, "%02X        UNDEFINED", GetMem(addr++));
			strcat(str,chr);
		}
		else {
			if ((addr+size) > 0xFFFF) {
				while (addr <= 0xFFFF) {
					sprintf(chr, "%02X        OVERFLOW", GetMem(addr++));
					strcat(str,chr);
				}
				break;
			}
			for (j = 0; j < size; j++) {
				sprintf(chr, "%02X ", opcode[j] = GetMem(addr++));
				strcat(str,chr);
			}
			while (size < 3) {
				strcat(str,"   "); //pad output to align ASM
				size++;
			}
			strcat(strcat(str," "),BinToASM(addr,opcode));
		}
		strcat(str,"\r\n");
	}
	SetDlgItemText(hDebug, 300, str);
}


int GetEditHex(HWND hwndDlg, int id) {
	char str[5];
	int tmp;
	GetDlgItemText(hwndDlg,id,str,5);
	sscanf(str,"%04x",&tmp);
	return tmp;
}

/*
int GetEditStack(HWND hwndDlg) {
	char str[85];
	int tmp;
	GetDlgItemText(hwndDlg,308,str,85);
	sscanf(str,"%02x,%02x,%02x,%02x,\r\n",&tmp);
	return tmp;
}
*/

void UpdateRegs(HWND hwndDlg) {
	X.A = GetEditHex(hwndDlg,304);
	X.X = GetEditHex(hwndDlg,305);
	X.Y = GetEditHex(hwndDlg,306);
	X.PC = GetEditHex(hwndDlg,307);
}

void UpdateDebugger() {
	char str[256]={0},chr[8];
	int tmp,i;

	Disassemble(X.PC);

	sprintf(str, "%02X", X.A);
	SetDlgItemText(hDebug, 304, str);
	sprintf(str, "%02X", X.X);
	SetDlgItemText(hDebug, 305, str);
	sprintf(str, "%02X", X.Y);
	SetDlgItemText(hDebug, 306, str);
	sprintf(str, "%04X", (int)X.PC);
	SetDlgItemText(hDebug, 307, str);

	sprintf(str, "%04X", (int)RefreshAddr);
	SetDlgItemText(hDebug, 310, str);
	sprintf(str, "%02X", PPU[3]);
	SetDlgItemText(hDebug, 311, str);

	sprintf(str, "Scanline: %d", scanline);
	SetDlgItemText(hDebug, 501, str);

	tmp = X.S|0x0100;
	sprintf(str, "Stack $%04X", tmp);
	SetDlgItemText(hDebug, 403, str);
	tmp = ((tmp+1)|0x0100)&0x01FF;
	sprintf(str, "%02X", GetMem(tmp));
	for (i = 1; i < 20; i++) {
		tmp = ((tmp+1)|0x0100)&0x01FF;  //increment and fix pointer to $0100-$01FF range
		if ((i%4) == 0) sprintf(chr, ",\r\n%02X", GetMem(tmp));
		else sprintf(chr, ",%02X", GetMem(tmp));
		strcat(str,chr);
	}
	SetDlgItemText(hDebug, 308, str);

	GetDlgItemText(hDebug,309,str,5);
	sscanf(str,"%04X",&tmp);
	sprintf(str,"%04X",tmp);
	SetDlgItemText(hDebug,309,str);

	CheckDlgButton(hDebug, 200, BST_UNCHECKED);
	CheckDlgButton(hDebug, 201, BST_UNCHECKED);
	CheckDlgButton(hDebug, 202, BST_UNCHECKED);
	CheckDlgButton(hDebug, 203, BST_UNCHECKED);
	CheckDlgButton(hDebug, 204, BST_UNCHECKED);
	CheckDlgButton(hDebug, 205, BST_UNCHECKED);
	CheckDlgButton(hDebug, 206, BST_UNCHECKED);
	CheckDlgButton(hDebug, 207, BST_UNCHECKED);

	tmp = X.P;
	if (tmp & N_FLAG) CheckDlgButton(hDebug, 200, BST_CHECKED);
	if (tmp & V_FLAG) CheckDlgButton(hDebug, 201, BST_CHECKED);
	if (tmp & U_FLAG) CheckDlgButton(hDebug, 202, BST_CHECKED);
	if (tmp & B_FLAG) CheckDlgButton(hDebug, 203, BST_CHECKED);
	if (tmp & D_FLAG) CheckDlgButton(hDebug, 204, BST_CHECKED);
	if (tmp & I_FLAG) CheckDlgButton(hDebug, 205, BST_CHECKED);
	if (tmp & Z_FLAG) CheckDlgButton(hDebug, 206, BST_CHECKED);
	if (tmp & C_FLAG) CheckDlgButton(hDebug, 207, BST_CHECKED);
}

char *BreakToText(unsigned int num) {
	static char str[20],chr[8];

	sprintf(str, "$%04X", watchpoint[num].address);
	if (watchpoint[num].endaddress) {
		sprintf(chr, "-$%04X", watchpoint[num].endaddress);
		strcat(str,chr);
	}
	if (watchpoint[num].flags&WP_E) strcat(str,": E"); else strcat(str,": -");
	if (watchpoint[num].flags&BT_P) strcat(str,"P"); else if (watchpoint[num].flags&BT_S) strcat(str,"S"); else strcat(str,"C");
	if (watchpoint[num].flags&WP_R) strcat(str,"R"); else strcat(str,"-");
	if (watchpoint[num].flags&WP_W) strcat(str,"W"); else strcat(str,"-");
	if (watchpoint[num].flags&WP_X) strcat(str,"X"); else strcat(str,"-");

	return str;
}

void AddBreakList() {
	SendDlgItemMessage(hDebug,302,LB_INSERTSTRING,-1,(LPARAM)(LPSTR)BreakToText(numWPs-1));
}

void EditBreakList() {
	if (WP_edit >= 0) {
		SendDlgItemMessage(hDebug,302,LB_DELETESTRING,WP_edit,0);
		SendDlgItemMessage(hDebug,302,LB_INSERTSTRING,WP_edit,(LPARAM)(LPSTR)BreakToText(WP_edit));
		SendDlgItemMessage(hDebug,302,LB_SETCURSEL,WP_edit,0);
	}
}

void FillBreakList(HWND hwndDlg) {
	unsigned int i;

	for (i = 0; i < numWPs; i++) {
		SendDlgItemMessage(hwndDlg,302,LB_INSERTSTRING,-1,(LPARAM)(LPSTR)BreakToText(i));
	}
}

void EnableBreak(int sel) {
	watchpoint[sel].flags^=WP_E;
	SendDlgItemMessage(hDebug,302,LB_DELETESTRING,sel,0);
	SendDlgItemMessage(hDebug,302,LB_INSERTSTRING,sel,(LPARAM)(LPSTR)BreakToText(sel));
	SendDlgItemMessage(hDebug,302,LB_SETCURSEL,sel,0);
}

void DeleteBreak(int sel) {
	int i;

	for (i = sel; i < numWPs; i++) {
		watchpoint[i].address = watchpoint[i+1].address;
		watchpoint[i].endaddress = watchpoint[i+1].endaddress;
		watchpoint[i].flags = watchpoint[i+1].flags;
	}
	numWPs--;
	SendDlgItemMessage(hDebug,302,LB_DELETESTRING,sel,0);
	EnableWindow(GetDlgItem(hDebug,102),FALSE);
	EnableWindow(GetDlgItem(hDebug,103),FALSE);
}

void KillDebugger() {
	SendDlgItemMessage(hDebug,302,LB_RESETCONTENT,0,0);
	numWPs = 0;
	step = 0;
	stepout = 0;
	jsrcount = 0;
	userpause = 0;
}

char *DisassembleLine(int addr) {
	static char str[64]={0},chr[20]={0};
	char *c;
	int size,j;
	uint8 opcode[3];

	sprintf(str, "$%04X:", addr);
	if ((size = opsize[GetMem(addr)]) == 0) {
		sprintf(chr, "%02X        UNDEFINED", GetMem(addr)); addr++;
		strcat(str,chr);
	}
	else {
		if ((addr+size) > 0xFFFF) {
			sprintf(chr, "%02X        OVERFLOW", GetMem(addr));
			strcat(str,chr);
			goto done;
		}
		for (j = 0; j < size; j++) {
			sprintf(chr, "%02X ", opcode[j] = GetMem(addr)); addr++;
			strcat(str,chr);
		}
		while (size < 3) {
			strcat(str,"   "); //pad output to align ASM
			size++;
		}
		strcat(strcat(str," "),BinToASM(addr,opcode));
	}
done:
	if ((c=strchr(str,'='))) *(c-1) = 0;
	if ((c=strchr(str,'@'))) *(c-1) = 0;
	return str;
}

BOOL CALLBACK AssemblerCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			CenterWindow(hwndDlg);

			//set font
			SendDlgItemMessage(hwndDlg,101,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,102,WM_SETFONT,(WPARAM)hNewFont,FALSE);

			SetDlgItemText(hwndDlg,101,DisassembleLine(iaPC));
			SetFocus(GetDlgItem(hwndDlg,100));
			break;
		case WM_CLOSE:
		case WM_QUIT:
			EndDialog(hwndDlg,0);
			break;
		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch(LOWORD(wParam)) {
						case 200:
							break;
						case 201:
							break;
						case 202:
							break;
					}
					break;
			}
			break;
	}
	return FALSE;
}

extern void StopSound();

BOOL CALLBACK DebuggerCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LOGFONT lf;
	RECT wrect;
	char str[8]={0};
	int tmp,tmp2;
	int mouse_x,mouse_y;

	//these messages get handled at any time
	switch(uMsg) {
		case WM_INITDIALOG:
			SetWindowPos(hwndDlg,0,DbgPosX,DbgPosY,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);

			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = 0x10000;
			si.nPos = 0;
			si.nPage = 20;
			SetScrollInfo(GetDlgItem(hwndDlg,301),SB_CTL,&si,TRUE);

			//setup font
			hFont = (HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0);
			GetObject(hFont, sizeof(LOGFONT), &lf);
			strcpy(lf.lfFaceName,"Courier");
			hNewFont = CreateFontIndirect(&lf);

			SendDlgItemMessage(hwndDlg,300,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,304,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,305,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,306,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,307,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,308,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,309,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,310,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,311,WM_SETFONT,(WPARAM)hNewFont,FALSE);

			//text limits
			SendDlgItemMessage(hwndDlg,304,EM_SETLIMITTEXT,2,0);
			SendDlgItemMessage(hwndDlg,305,EM_SETLIMITTEXT,2,0);
			SendDlgItemMessage(hwndDlg,306,EM_SETLIMITTEXT,2,0);
			SendDlgItemMessage(hwndDlg,307,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,308,EM_SETLIMITTEXT,83,0);
			SendDlgItemMessage(hwndDlg,309,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,310,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,311,EM_SETLIMITTEXT,2,0);

			//I'm lazy, disable the controls which I can't mess with right now
			SendDlgItemMessage(hwndDlg,310,EM_SETREADONLY,TRUE,0);
			SendDlgItemMessage(hwndDlg,311,EM_SETREADONLY,TRUE,0);

			debugger_open = 1;
			FillBreakList(hwndDlg);
			break;
		case WM_CLOSE:
		case WM_QUIT:
			exitdebug:
			debugger_open = 0;
			DeleteObject(hNewFont);
			DestroyWindow(hwndDlg);
			break;
		case WM_MOVING:
			StopSound();
			break;
		case WM_MOVE:
			GetWindowRect(hwndDlg,&wrect);
			DbgPosX = wrect.left;
			DbgPosY = wrect.top;
			break;
		case WM_COMMAND:
			if ((HIWORD(wParam) == BN_CLICKED) && (LOWORD(wParam) == 100)) goto exitdebug;
			break;
	}

	//these messages only get handled when a game is loaded
	if (GI) {
		switch(uMsg) {
			case WM_VSCROLL:
				if (userpause) UpdateRegs(hwndDlg);
				if (lParam) {
					StopSound();
					GetScrollInfo((HWND)lParam,SB_CTL,&si);
					switch(LOWORD(wParam)) {
						case SB_ENDSCROLL:
						case SB_TOP:
						case SB_BOTTOM: break;
						case SB_LINEUP: si.nPos--; break;
						case SB_LINEDOWN:
							if ((tmp=opsize[GetMem(si.nPos)])) si.nPos+=tmp;
							else si.nPos++;
							break;
						case SB_PAGEUP: si.nPos-=si.nPage; break;
						case SB_PAGEDOWN: si.nPos+=si.nPage; break;
						case SB_THUMBPOSITION: //break;
						case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
					}
					if (si.nPos < si.nMin) si.nPos = si.nMin;
					if ((si.nPos+si.nPage) > si.nMax) si.nPos = si.nMax-si.nPage;
					SetScrollInfo((HWND)lParam,SB_CTL,&si,TRUE);
					Disassemble(si.nPos);
				}
				break;
			case WM_LBUTTONDOWN:
				mouse_x = GET_X_LPARAM(lParam);
				mouse_y = GET_Y_LPARAM(lParam);
				if ((userpause > 0) && (mouse_x > 8) && (mouse_x < 22) && (mouse_y > 10) && (mouse_y < 282)) {
					StopSound();
					if ((tmp=((mouse_y - 10) / 13)) > 19) tmp = 19;
					iaPC = si.nPos;
					while (tmp > 0) {
						if ((tmp2=opsize[GetMem(iaPC)]) == 0) tmp2++;
						if ((iaPC+=tmp2) > 0xFFFF) {
							iaPC = 0xFFFF;
							break;
						}
						tmp--;
					}
					DialogBox(fceu_hInstance,"ASSEMBLER",hwndDlg,AssemblerCallB);
					UpdateDebugger();
				}
				break;
			case WM_COMMAND:
				switch(HIWORD(wParam)) {
					case BN_CLICKED:
						switch(LOWORD(wParam)) {
							case 101: //Add
								StopSound();
								childwnd = 1;
								if (DialogBox(fceu_hInstance,"ADDBP",hwndDlg,AddbpCallB)) AddBreakList();
								childwnd = 0;
								UpdateDebugger();
								break;
							case 102: //Delete
								DeleteBreak(SendDlgItemMessage(hwndDlg,302,LB_GETCURSEL,0,0));
								break;
							case 103: //Edit
								StopSound();
								WP_edit = SendDlgItemMessage(hwndDlg,302,LB_GETCURSEL,0,0);
								if (DialogBox(fceu_hInstance,"ADDBP",hwndDlg,AddbpCallB)) EditBreakList();
								WP_edit = -1;
								UpdateDebugger();
								break;
							case 104: //Run
								if (userpause > 0) {
									UpdateRegs(hwndDlg);
									userpause = 0;
									UpdateDebugger();
								}
								break;
							case 105: //Step Into
								if (userpause > 0)
									UpdateRegs(hwndDlg);
								step = 1;
								userpause = 0;
								UpdateDebugger();
								break;
							case 106: //Step Out
								if (userpause > 0) {
									UpdateRegs(hwndDlg);
									if ((stepout) && (MessageBox(hwndDlg,"Step Out is currently in process. Cancel it and setup a new Step Out watch?","Step Out Already Active",MB_YESNO|MB_ICONINFORMATION) != IDYES)) break;
									if (GetMem(X.PC) == 0x20) jsrcount = 1;
									else jsrcount = 0;
									stepout = 1;
									userpause = 0;
									UpdateDebugger();
								}
								break;
							case 107: //Step Over
								if (userpause) {
									UpdateRegs(hwndDlg);
									if (GetMem(tmp=X.PC) == 0x20) {
										if ((watchpoint[64].flags) && (MessageBox(hwndDlg,"Step Over is currently in process. Cancel it and setup a new Step Over watch?","Step Over Already Active",MB_YESNO|MB_ICONINFORMATION) != IDYES)) break;
										watchpoint[64].address = (tmp+3);
										watchpoint[64].flags = WP_E|WP_X;
									}
									else step = 1;
									userpause = 0;
								}
								break;
							case 108: //Seek PC
								if (userpause) {
									UpdateRegs(hwndDlg);
									UpdateDebugger();
								}
								break;
							case 109: //Seek To:
								if (userpause) UpdateRegs(hwndDlg);
								GetDlgItemText(hwndDlg,309,str,5);
								sscanf(str,"%04X",&tmp);
								sprintf(str,"%04X",tmp);
								SetDlgItemText(hwndDlg,309,str);
								Disassemble(tmp);
								break;

							case 200: X.P^=N_FLAG; UpdateDebugger(); break;
							case 201: X.P^=V_FLAG; UpdateDebugger(); break;
							case 202: X.P^=U_FLAG; UpdateDebugger(); break;
							case 203: X.P^=B_FLAG; UpdateDebugger(); break;
							case 204: X.P^=D_FLAG; UpdateDebugger(); break;
							case 205: X.P^=I_FLAG; UpdateDebugger(); break;
							case 206: X.P^=Z_FLAG; UpdateDebugger(); break;
							case 207: X.P^=C_FLAG; UpdateDebugger(); break;
						}
						//UpdateDebugger();
						break;
					case LBN_DBLCLK:
						switch(LOWORD(wParam)) {
							case 302: EnableBreak(SendDlgItemMessage(hwndDlg,302,LB_GETCURSEL,0,0)); break;
						}
						break;
					case LBN_SELCANCEL:
						switch(LOWORD(wParam)) {
							case 302:
								EnableWindow(GetDlgItem(hwndDlg,102),FALSE);
								EnableWindow(GetDlgItem(hwndDlg,103),FALSE);
								break;
						}
						break;
					case LBN_SELCHANGE:
						switch(LOWORD(wParam)) {
							case 302:
								EnableWindow(GetDlgItem(hwndDlg,102),TRUE);
								EnableWindow(GetDlgItem(hwndDlg,103),TRUE);
								break;
						}
						break;
				}
				break;
		}
	}
	return FALSE; //TRUE;
}

void DoDebug(uint8 halt) {
	if (!debugger_open) hDebug = CreateDialog(fceu_hInstance,"DEBUGGER",NULL,DebuggerCallB);
	if (hDebug) {
		SetWindowPos(hDebug,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
		if (GI) UpdateDebugger();
	}
}
