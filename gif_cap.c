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
static int colortable[256*3+1];
int downsample(unsigned char *data,int w,int h,int row_width,unsigned char *pixels)
{
	if(pixels!=0){
		int i,j,poffset=0;
		for(i=h-1;i>=0;i--){
			for(j=0;j<w;j++){
				int x=(i*row_width)+(j*4);
				unsigned char index=0;
				unsigned char r,g,b;
				b=data[x+0]&0xE0;
				g=data[x+1]&0xE0;
				r=data[x+2]&0xC0;
				index=r|(g>>2)|(b>>6);
				pixels[poffset++]=index;
			}
		}
	}
}
int create_ctable()
{
	int i,index;
	static int created=FALSE;
	if(created)
		return TRUE;
	index=0;
	for(i=0;i<256;i++){
		unsigned char r,g,b;
		r=i>>6;
		g=(i>>3)&0x7;
		b=i&3;
		colortable[index*3]=r<<6;
		colortable[index*3+1]=g<<5;
		colortable[index*3+2]=b<<5;
		index++;
	}
	colortable[index*3]=-1;
	created=TRUE;
	return TRUE;
}
int grab_pixels(HDC hdc,HBITMAP hbmp,BITMAP *bmp,unsigned char **pixels,int w,int h)
{
	int result=FALSE;
	unsigned char *data;
	BITMAPINFO bmi={0};

	data=GlobalAlloc(GMEM_FIXED,bmp->bmWidthBytes*bmp->bmHeight);
	
	if (!data) 
		return result;
	bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biBitCount=32;
	bmi.bmiHeader.biWidth=bmp->bmWidth;
	bmi.bmiHeader.biHeight=bmp->bmHeight;
	bmi.bmiHeader.biPlanes=bmp->bmPlanes;
	bmi.bmiHeader.biCompression=BI_RGB;
	bmi.bmiHeader.biClrImportant=0;
	bmi.bmiHeader.biClrUsed=0;
	bmi.bmiHeader.biSizeImage = ((bmi.bmiHeader.biWidth * 32 +31)& ~31) /8 * bmi.bmiHeader.biHeight;

	if(GetDIBits(hdc,hbmp,0,(WORD)bmp->bmHeight,data,&bmi,DIB_RGB_COLORS)){
			*pixels=malloc(w*h);
			if(*pixels!=0){
				int row_width;
				row_width=bmp->bmWidthBytes;
				downsample(data,w,h,row_width,*pixels);
				result=TRUE;
			}
	}
	if(data)
		GlobalFree((HGLOBAL)data);
	return result;
}
int take_snap(HWND hwnd,unsigned char **pixels,int w,int h)
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
			hbmp=CreateCompatibleBitmap(hscreen,w,h);
			if(hbmp!=0){
				//PBITMAPINFO pBitmapInfo;
				BITMAP bmp;
				HBITMAP hold;
				hold=SelectObject(htarget,hbmp);
				BitBlt(htarget, 0, 0, w, h, hscreen, 0, 0, SRCCOPY);
				SelectObject(htarget, hold);
				if(GetObject(hbmp,sizeof(BITMAP),&bmp)){
				//pBitmapInfo=CreateBitmapInfoStruct(hbmp);
					if(grab_pixels(htarget,hbmp,&bmp,pixels,w,h))
						result=TRUE;
				}
				//CreateBMPFile(("b:\\picture.bmp"), pBitmapInfo, hbmp, htarget);
				DeleteObject(hbmp);
			}
			DeleteDC(htarget);
		}
		ReleaseDC(hwin,hscreen);
	}
	return result;
}
int animate(HWND hwnd,int step)
{
	static unsigned char *gifimage=0;
	static void *gsdata=0;
	static int w=0,h=0;
	if(step==0){
		RECT rect={0};
		GetClientRect(hwnd,&rect);
		w=rect.right-rect.left-1;
		h=rect.bottom-rect.top-1;

		gsdata=newgif(&gifimage,w,h,colortable,0);
		if(gsdata!=0){
			animategif(gsdata,0,5,0,2);
		}
	}
	else if(step==1){
		unsigned char *pixels=0;
		if(gifimage!=0){
			take_snap(hwnd,&pixels,w,h);
			if(pixels!=0){
				int len;
				char str[80];
				len=putgif(gsdata,pixels);
				_snprintf(str,sizeof(str),"%i (0x%08X)",len,len);
				str[sizeof(str)-1]=0;
				SetWindowText(hwnd,str);
				free(pixels);
			}
		}
	}
	else if(step==2){
		if(gsdata!=0){
			char str[80];
			int glen=0;
			glen=endgif(gsdata);
			_snprintf(str,sizeof(str),"DONE %i (0x%08X)",glen,glen);
			SetWindowText(hwnd,str);
			{
				FILE *f;
				f=fopen("b:\\test.gif","wb");
				if(f!=0){
					fwrite(gifimage,1,glen,f);
					fclose(f);
				}
			}

		}
		if(gifimage!=0)
			free(gifimage);
	}

}
BOOL CALLBACK gifcap(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HRGN hregion=0;
	{
		static DWORD tick=0;
		//print_msg(msg,lparam,wparam,hwnd);
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
			create_ctable();
		}
		break;
	case WM_APP:
		switch(wparam){
		case '1':
			animate(hwnd,lparam);
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
		if(GetAsyncKeyState(VK_SHIFT)&0x8001){
			int i;
			Beep(1000,100);
			PostMessage(ghdlg,WM_APP,'1',0);
			Sleep(33);
			for(i=0;i<5000;i++){
				PostMessage(ghdlg,WM_APP,'1',1);
				Sleep(33);
				if(GetAsyncKeyState(VK_CONTROL)&0x8001)
					break;
			}
			PostMessage(ghdlg,WM_APP,'1',2);
			Beep(800,100);
			printf("1 presed\n");
		}
	}
}
int CALLBACK WinMain(HINSTANCE hinstance,HINSTANCE hprevinstance,char *cmdline,int showcmd)
{
	//open_console();
	_beginthread(key_thread,0,0);
	return DialogBoxParam(hinstance,MAKEINTRESOURCE(IDD_DIALOG1),NULL,gifcap,NULL);
}