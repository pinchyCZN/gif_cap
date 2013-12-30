#include <windows.h>
#include "resource.h"
extern HINSTANCE ghinstance;

int get_bmp_pixel(BITMAP *bmp,int x,int y)
{
	int pixel=0;
	if(bmp!=0 && bmp->bmBits!=0){
		if(x>=0 && y>=0 && x<bmp->bmWidth && y<bmp->bmHeight){
			int *data=0;
			int offset=x*(bmp->bmBitsPixel/8)+(bmp->bmHeight-y-1)*bmp->bmWidthBytes;
			int max=bmp->bmWidthBytes*bmp->bmHeight;
			if(offset>=max || offset<0)
				return pixel;
			data=(char*)bmp->bmBits+offset;
			pixel=data[0];
		}
	}
	return pixel;
}
int center_window_to_desk(HWND hwnd,int width,int height)
{
	if(hwnd==0)
		return 0;
	else{
		RECT deskrect={0};
		int cx,cy;
		GetWindowRect(GetDesktopWindow(),&deskrect);
		cx=(deskrect.right-deskrect.left)/2;
		cy=(deskrect.bottom-deskrect.top)/2;
		MoveWindow(hwnd,cx-width/2,cy-height/2,width,height,TRUE);
	}
	return 0;
}
int set_icon(HWND hwnd,int IDCON_ID)
{
	HICON hIcon = LoadIcon(ghinstance,MAKEINTRESOURCE(IDCON_ID));
    if(hIcon){
		SendMessage(hwnd,WM_SETICON,ICON_SMALL,(LPARAM)hIcon);
		SendMessage(hwnd,WM_SETICON,ICON_BIG,(LPARAM)hIcon);
		return TRUE;
	}
}
BOOL CALLBACK splashproc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static BITMAP *bmp=0;
	static HBRUSH hbrush=0;
	static HRGN hregion=0;
	static int timer=0,mousex,mousey;
	if(FALSE)
	//if(msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY)
	//if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch (msg){
	case WM_INITDIALOG:
		{
			int cx,cy;
			if(lparam!=0){
				void **ptr=lparam;
				bmp=ptr[0];
				hbrush=ptr[1];
				if(bmp==0 || hbrush==0){
					EndDialog(hwnd,0);
				}
			}
			else
				EndDialog(hwnd,0);

			SetWindowText(hwnd,"win installer");
			{
				RECT deskrect={0},winrect={0};
				int xframe,yframe,ycaption;
				xframe=GetSystemMetrics(SM_CXDLGFRAME);
				yframe=GetSystemMetrics(SM_CYDLGFRAME);
				ycaption=GetSystemMetrics(SM_CYCAPTION);
				cx=xframe;
				cy=ycaption+xframe;
				GetWindowRect(GetDesktopWindow(),&deskrect);
				GetWindowRect(GetDesktopWindow(),&winrect);
				center_window_to_desk(hwnd,bmp->bmWidth+cx*2,bmp->bmHeight+cy+cx);
				hregion=CreateRectRgn(cx,cy,cx+bmp->bmWidth,cy+bmp->bmHeight);
			}
			if(hregion==0)
				EndDialog(hwnd,0);
			//timer=SetTimer(hwnd,1337,2000,NULL);
			//if(timer==0)
			//	PostMessage(hwnd,WM_CLOSE,0,0);
			{
				int x,y;
				for (y=0; y!=bmp->bmHeight; y++){
					for (x=0; x!=bmp->bmWidth; x++){
						unsigned long dwColor;
						dwColor = get_bmp_pixel(bmp,x,y);
						dwColor = dwColor & 0x00FFFFFF;
						//if(FALSE)
						if (!dwColor){
							HRGN hRegionTemp;
							hRegionTemp = CreateRectRgn(cx+x,cy+y,cx+x+1,cy+y+1);
							CombineRgn(hregion, hregion, hRegionTemp, RGN_XOR);
							DeleteObject(hRegionTemp);
						}
					}
				}
			}
			SetWindowRgn(hwnd,hregion,TRUE);
			set_icon(hwnd,IDI_ICON1);
		}
		break;
	case WM_TIMER:
		KillTimer(hwnd,timer);
		PostMessage(hwnd,WM_CLOSE,0,0);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDCANCEL:
			PostMessage(hwnd,WM_CLOSE,0,0);
			break;
		}
		break;
	case WM_CTLCOLORDLG:
		return hbrush;
		break;
	case WM_LBUTTONDOWN:
		mousex=LOWORD(lparam);
		mousey=HIWORD(lparam);
		break;
	case WM_MOUSEFIRST:
		{
			int x,y;
			x=LOWORD(lparam);
			y=HIWORD(lparam);
			if(wparam==1){
				POINT p;
				int dx,dy;
				dx=x-mousex;
				dy=y-mousey;
				p.x=dx-GetSystemMetrics(SM_CXDLGFRAME);
				p.y=dy-GetSystemMetrics(SM_CYCAPTION)-GetSystemMetrics(SM_CYDLGFRAME); //-y-GetSystemMetrics(SM_CYDLGFRAME)-GetSystemMetrics(SM_CYCAPTION);
				MapWindowPoints(hwnd,NULL,&p,1);
				SetWindowPos(hwnd,NULL,p.x,p.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
			}
		}
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_CLOSE:
		if(hregion!=0)
			DeleteObject(hregion);
		EndDialog(hwnd, 0);
		break;
	}
	return 0;
}

int create_splash_screen(HWND hwnd)
{
	HBITMAP hbit;
	hbit=LoadImage(ghinstance,IDB_SPLASH1,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
	if(hbit!=0){
		BITMAP bmp;
		memset(&bmp,0,sizeof(bmp));
		if(GetObject(hbit,sizeof(bmp),&bmp)!=0){
			if(bmp.bmBitsPixel!=24 || bmp.bmPlanes!=1){
				MessageBox(NULL,"BMP format not supported","ERROR",MB_OK|MB_SYSTEMMODAL);
			}
			else{
				HBRUSH hbrush;
				hbrush=CreatePatternBrush(hbit);
				if(hbrush!=0){
					void *ptr[2];
					ptr[0]=&bmp;
					ptr[1]=hbrush;
					DialogBoxParam(ghinstance,IDD_SPLASH,0,&splashproc,ptr);
					DeleteObject(hbrush);
				}
			}
		}
		DeleteObject(hbit);
	}
	return 0;
}
int create_initial_window(HWND hwnd)
{
	RECT rect={0};
	HWND hdesk;
	int width=500,height=300;
	int x,y;
	hdesk=GetDesktopWindow();
	GetWindowRect(hdesk,&rect);
	x=(rect.right-rect.left)/2-(width/2);
	y=(rect.bottom-rect.top)/2-(height/2);
	SetWindowPos(hwnd,HWND_TOP,x,y,width,height,SWP_SHOWWINDOW); //SWP_NOMOVE
	return TRUE;
}

BOOL CALLBACK enum_clear_cb(HWND hwnd,LPARAM lparam)
{
	DestroyWindow(hwnd);
	return TRUE;
}

BOOL CALLBACK enum_set_font(HWND hwnd,LPARAM lparam)
{
	SendMessage(hwnd,WM_SETFONT,(WPARAM)lparam,0);
	return TRUE;
}

int get_sans_serif_font(HFONT *hfont)
{
	static HFONT hf=0;
	if(hfont==0)
		return FALSE;
	else if(hf!=0){
		*hfont=hf;
		return TRUE;
	}
	else{
		hf=CreateFont(8, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, 
			 OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
			DEFAULT_PITCH | FF_DONTCARE,TEXT("MS Sans Serif"));
		if(hf!=0){
			*hfont=hf;
			return TRUE;
		}
	}
	return FALSE;
}
int skin_step1(HWND hwnd)
{
	HFONT hfont=0;
	HWND hctrl_txt;
	EnumChildWindows(hwnd,enum_clear_cb,0);
	CreateWindow("BUTTON","Next >",WS_CHILD|WS_VISIBLE|WS_TABSTOP,0,0,75,23,hwnd,IDC_NEXT,ghinstance,0);
	CreateWindow("BUTTON","Cancel",WS_CHILD|WS_VISIBLE|WS_TABSTOP,80,0,75,23,hwnd,IDCANCEL,ghinstance,0);
	CreateWindow("BUTTON","Config",WS_CHILD|WS_VISIBLE|WS_TABSTOP,80*2,0,75,23,hwnd,IDC_CONFIGURE,ghinstance,0);
	hctrl_txt=CreateWindow("STATIC","",WS_CHILD|WS_VISIBLE,10,100,100,200,hwnd,IDC_TEXT,ghinstance,0);
	hfont=GetStockObject(DEFAULT_GUI_FONT);
	if(hfont!=0){
		EnumChildWindows(hwnd,enum_set_font,(LPARAM)hfont);
	}
	SetWindowText(hctrl_txt,"Demo install of cool app\r\nEnter password (123) to continue");

}