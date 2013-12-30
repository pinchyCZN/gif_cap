#include <windows.h>
#include <stdio.h>
#include "resource.h"

HINSTANCE ghinstance=0;
HINSTANCE ghprevinstance=0;
HWND ghdlg=0;
int assert(int a)
{
	if(!a){
		MessageBox(NULL,"ASSERT","ERROR",MB_OK|MB_SYSTEMMODAL);
	}
	return a;
}
void open_console()
{
#define _O_TEXT         0x4000  /* file mode is text (translated) */
	char title[MAX_PATH]={0}; 
	HWND hcon; 
	FILE *hf;
	static BYTE consolecreated=FALSE;
	static int hcrt=0;
	
	if(consolecreated==TRUE)
	{
		GetConsoleTitle(title,sizeof(title));
		if(title[0]!=0){
			hcon=FindWindow(NULL,title);
			ShowWindow(hcon,SW_SHOW);
		}
		hcon=(HWND)GetStdHandle(STD_INPUT_HANDLE);
		FlushConsoleInputBuffer(hcon);
		return;
	}
	AllocConsole(); 
	hcrt=_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE),_O_TEXT);

	fflush(stdin);
	hf=_fdopen(hcrt,"w"); 
	*stdout=*hf; 
	setvbuf(stdout,NULL,_IONBF,0);
	GetConsoleTitle(title,sizeof(title));
	if(title[0]!=0){
		hcon=FindWindow(NULL,title);
		ShowWindow(hcon,SW_SHOW); 
		SetForegroundWindow(hcon);
	}
	consolecreated=TRUE;
}

int center_window_to_desk(HWND hwnd)
{
	if(hwnd==0)
		return 0;
	else{
		RECT deskrect={0};
		RECT winrect={0};
		int cx,cy;
		int width,height;
		GetWindowRect(GetDesktopWindow(),&deskrect);
		GetWindowRect(hwnd,&winrect);
		cx=(deskrect.right-deskrect.left)/2;
		cy=(deskrect.bottom-deskrect.top)/2;
		width=winrect.right-winrect.left;
		height=winrect.bottom-winrect.top;
		MoveWindow(hwnd,cx-width/2,cy-height/2,width,height,TRUE);
	}
	return 0;
}
int set_title(HWND hwnd)
{
	char title[255]={0};
	RECT rect={0};
	GetClientRect(hwnd,&rect);
	sprintf(title,"gif cap w=%i h=%i x=%i y=%i",rect.right,rect.bottom,0,0);
	SetWindowText(hwnd,title);
	return TRUE;
}
int get_main_region(HWND hwnd,HRGN *hregion)
{
	HRGN hr=*hregion;
	RECT rect;
	if(hr!=0)
		DeleteObject(hr);
	GetWindowRect(hwnd,&rect);
	hr=CreateRectRgn(0,0,rect.right-rect.left,rect.bottom-rect.top);
	*hregion=hr;
	return TRUE;
}
int clear_window(HWND hwnd,HRGN *hregion)
{
	int cx,cy;
	RECT crect={0};
	HRGN htmp,hr;
	int xframe,yframe,ycaption;
	xframe=GetSystemMetrics(SM_CXDLGFRAME);
	yframe=GetSystemMetrics(SM_CYDLGFRAME);
	ycaption=GetSystemMetrics(SM_CYCAPTION);
	cx=xframe;
	cy=ycaption+xframe;
	GetClientRect(hwnd,&crect);
	get_main_region(hwnd,hregion);
	hr=*hregion;
	//GetWindowRgn(hwnd,hr);
	htmp=CreateRectRgn(cx,cy,cx+crect.right,cy+crect.bottom);
	if(htmp!=0 && hr!=0){
		//FillRgn(
		CombineRgn(hr, hr, htmp, RGN_DIFF);
		//NULLREGION
		DeleteObject(htmp);
	}
	set_title(hwnd);
}

PBITMAPINFO CreateBitmapInfoStruct(HBITMAP hBmp)
{ 
	BITMAP bmp; 
	PBITMAPINFO pbmi; 
	WORD    cClrBits;
	// Retrieve the bitmap color format, width, and height. 
	if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp)) 
		return NULL;
	
	// Convert the color format to a count of bits. 
	cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel); 
	if (cClrBits == 1) 
		cClrBits = 1; 
	else if (cClrBits <= 4) 
		cClrBits = 4; 
	else if (cClrBits <= 8) 
		cClrBits = 8; 
	else if (cClrBits <= 16) 
		cClrBits = 16; 
	else if (cClrBits <= 24) 
		cClrBits = 24; 
	else cClrBits = 32; 
	
	// Allocate memory for the BITMAPINFO structure. (This structure 
	// contains a BITMAPINFOHEADER structure and an array of RGBQUAD 
	// data structures.) 
	
	if (cClrBits != 24) 
		pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
		sizeof(BITMAPINFOHEADER) + 
		sizeof(RGBQUAD) * (1<< cClrBits)); 
	
	// There is no RGBQUAD array for the 24-bit-per-pixel format. 
	
	else 
		pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
		sizeof(BITMAPINFOHEADER)); 
	
	// Initialize the fields in the BITMAPINFO structure. 
	
	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
	pbmi->bmiHeader.biWidth = bmp.bmWidth; 
	pbmi->bmiHeader.biHeight = bmp.bmHeight; 
	pbmi->bmiHeader.biPlanes = bmp.bmPlanes; 
	pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel; 
	if (cClrBits < 24) 
		pbmi->bmiHeader.biClrUsed = (1<<cClrBits); 
	
	// If the bitmap is not compressed, set the BI_RGB flag. 
	pbmi->bmiHeader.biCompression = BI_RGB; 
	
	// Compute the number of bytes in the array of color 
	// indices and store the result in biSizeImage. 
	// For Windows NT, the width must be DWORD aligned unless 
	// the bitmap is RLE compressed. This example shows this. 
	// For Windows 95/98/Me, the width must be WORD aligned unless the 
	// bitmap is RLE compressed.
	pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits +31)& ~31) /8
		* pbmi->bmiHeader.biHeight; 
	// Set biClrImportant to 0, indicating that all of the 
	// device colors are important. 
	pbmi->bmiHeader.biClrImportant = 0; 
	return pbmi; 
} 
void CreateBMPFile(LPTSTR pszFile, PBITMAPINFO pbi, 
				   HBITMAP hBMP, HDC hDC) 
{ 
	HANDLE hf;                 // file handle 
	BITMAPFILEHEADER hdr;       // bitmap file-header 
	PBITMAPINFOHEADER pbih;     // bitmap info-header 
	LPBYTE lpBits;              // memory pointer 
	DWORD dwTotal;              // total count of bytes 
	DWORD cb;                   // incremental count of bytes 
	BYTE *hp;                   // byte pointer 
	DWORD dwTmp; 
	
	pbih = (PBITMAPINFOHEADER) pbi; 
	lpBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);
	
	if (!lpBits) 
		return; 
	
	// Retrieve the color table (RGBQUAD array) and the bits 
	// (array of palette indices) from the DIB. 
	if (!GetDIBits(hDC, hBMP, 0, (WORD) pbih->biHeight, lpBits, pbi, 
		DIB_RGB_COLORS)) 
	{
		return;
	}
	
	// Create the .BMP file. 
	hf = CreateFile(pszFile, 
		GENERIC_READ | GENERIC_WRITE, 
		(DWORD) 0, 
		NULL, 
		CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, 
		(HANDLE) NULL); 
	if (hf == INVALID_HANDLE_VALUE) 
		return; 
	hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M" 
	// Compute the size of the entire file. 
	hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + 
		pbih->biSize + pbih->biClrUsed 
		* sizeof(RGBQUAD) + pbih->biSizeImage); 
	hdr.bfReserved1 = 0; 
	hdr.bfReserved2 = 0; 
	
	// Compute the offset to the array of color indices. 
	hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + 
		pbih->biSize + pbih->biClrUsed 
		* sizeof (RGBQUAD); 
	
	// Copy the BITMAPFILEHEADER into the .BMP file. 
	if (!WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER), 
		(LPDWORD)&dwTmp,  NULL)) 
	{
		return; 
	}
	
	// Copy the BITMAPINFOHEADER and RGBQUAD array into the file. 
	if (!WriteFile(hf, (LPVOID) pbih, sizeof(BITMAPINFOHEADER) 
		+ pbih->biClrUsed * sizeof (RGBQUAD), 
		(LPDWORD) &dwTmp, ( NULL))) 
		return; 
	
	// Copy the array of color indices into the .BMP file. 
	dwTotal = cb = pbih->biSizeImage; 
	hp = lpBits; 
	if (!WriteFile(hf, (LPSTR) hp, (int) cb, (LPDWORD) &dwTmp,NULL)) 
		return; 
	
	// Close the .BMP file. 
	if (!CloseHandle(hf)) 
		return; 
	
	// Free memory. 
	GlobalFree((HGLOBAL)lpBits);
}
int dither(unsigned char *data,int w,int h,int modulo,unsigned char *pixels)
{
	if(pixels!=0){
		int i,j;
		for(i=0;i<h;i++){
			for(j=0;j<w;j++){
				int c=data[i*modulo+j*3];
				int r,g,b;
				r=c&0xFF;
				g=(c>>8)&0xFF;
				b=(c>>16)&0xFF;
				r>>=5;
				b>>=5;
				g>>=5;
				pixels[i*j]=(r<<5)|(b<<3)|(g>>2);
			}
		}
	}
}
int create_ctable(int *table,int len)
{
	int i,index=0,c=0;
	//
	for(i=0;i<255;i++){
		int tmp;
		tmp=c*255/85;
		if(tmp>0xFF)
			tmp=0xFF;
		if(i<85){
			table[index++]=tmp;
			table[index++]=0;
			table[index++]=0;
		}else if(i<170){
			table[index++]=0;
			table[index++]=tmp;
			table[index++]=0;
		}else{
			table[index++]=0;
			table[index++]=0;
			table[index++]=tmp;
		}
		if(i==85)
			c=0;
		else if(i==170)
			c=0;
		else
			c++;
	}
	table[index++]=-1;
}
int save_gif(HDC hdc,HBITMAP hbmp,BITMAPINFO *bmp_info)
{
	int result=FALSE;
	unsigned char *data;
	BITMAPINFOHEADER *bmp_header;

	bmp_header=bmp_info;
	data=GlobalAlloc(GMEM_FIXED,bmp_header->biSizeImage);
	
	if (!data) 
		return result; 
	if(GetDIBits(hdc,hbmp,0,(WORD)bmp_header->biHeight,data,bmp_info,DIB_RGB_COLORS)){
		unsigned char *gifimage=0;
		unsigned char *pixels=0;
		void *gsdata=0;
		int bgindex=0;
		int *colortable;
		colortable=malloc(4*256*3);
		create_ctable(colortable,4*256*3);

		gsdata=newgif(&gifimage,bmp_header->biWidth,bmp_header->biHeight,colortable,bgindex);
		if(gifimage!=0){
			char *pixels=0;
			int glen=0;
			pixels=malloc(bmp_header->biWidth*bmp_header->biHeight);
			if(pixels!=0){
				dither(data,bmp_header->biWidth,bmp_header->biHeight,bmp_header->biWidth*3,pixels);
				putgif(gsdata,pixels);
				free(pixels);
			}
			glen=endgif(gsdata);
			{
				FILE *f;
				f=fopen("c:\\temp\\test.gif","wb");
				if(f!=0){
					fwrite(gifimage,1,glen,f);
					fclose(f);
				}
			}

			if(gifimage!=0)
				free(gifimage);

		}
		if(colortable!=0)
			free(colortable);
	}
	if(data)
		GlobalFree((HGLOBAL)data);
	return result;
}
int take_snap(HWND hwnd)
{
	int result=FALSE;
	HDC hscreen;
	HWND hwin=hwnd;
	//hwin=GetDesktopWindow();
	hscreen=GetDC(hwin);
	if(hscreen!=0){
		HDC htarget;
		htarget=CreateCompatibleDC(hscreen);
		if(htarget!=0){
			RECT rect;
			HBITMAP hbmp;
			int w,h;
			GetClientRect(hwin,&rect);
			w=rect.right-rect.left-1;
			h=rect.bottom-rect.top-1;
			hbmp=CreateCompatibleBitmap(hscreen,w,h);
			if(hbmp!=0){
				PBITMAPINFO pBitmapInfo;
				HBITMAP hold;
				hold=SelectObject(htarget,hbmp);
				BitBlt(htarget, 0, 0, w, h, hscreen, 0, 0, SRCCOPY);
				SelectObject(htarget, hold);
				pBitmapInfo=CreateBitmapInfoStruct(hbmp);
				save_gif(htarget,hbmp,pBitmapInfo);
				CreateBMPFile(("b:\\picture.bmp"), pBitmapInfo, hbmp, htarget);
				DeleteObject(hbmp);
				result=TRUE;
			}
			DeleteDC(htarget);
		}
		ReleaseDC(hwin,hscreen);
	}
	return result;
}
BOOL CALLBACK gifcap(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HRGN hregion=0;
	{
		static DWORD tick=0;
		print_msg(msg,lparam,wparam,hwnd);
	}
	switch(msg){
	case WM_INITDIALOG:
		{
			RECT rect={0};
			ghdlg=hwnd;
			center_window_to_desk(hwnd);
			GetWindowRect(hwnd,&rect);
			hregion=CreateRectRgn(0,0,rect.right-rect.left,rect.bottom-rect.top);
			clear_window(hwnd,&hregion);
			SetWindowRgn(hwnd,hregion,TRUE);
			SetWindowPos(hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		}
		break;
	case WM_APP:
		switch(wparam){
		case '1':
			take_snap(hwnd);
			printf("made it\n");
			break;
		}
		break;
	case WM_SIZE:
		clear_window(hwnd,&hregion);
		SetWindowRgn(hwnd,hregion,TRUE);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDCANCEL:
			PostMessage(hwnd,WM_CLOSE,0,0);
			break;
		}
		break;
	case WM_CLOSE:
		if(hregion!=0)
			DeleteObject(hregion);
		EndDialog(hwnd, 0);
		break;
	}
	return 0;
}
int key_thread()
{
	while(TRUE){
		Sleep(100);
		if(GetAsyncKeyState('1')&0x8001){
			PostMessage(ghdlg,WM_APP,'1',0);
			printf("1 presed\n");
		}
	}
}
int CALLBACK WinMain(HINSTANCE hinstance,HINSTANCE hprevinstance,char *cmdline,int showcmd)
{
	open_console();
	_beginthread(key_thread,0,0);
	return DialogBoxParam(hinstance,MAKEINTRESOURCE(IDD_DIALOG1),NULL,gifcap,NULL);
}