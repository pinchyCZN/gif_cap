#include <windows.h>
#include <stdio.h>
#include "resource.h"

HINSTANCE ghinstance=0;
HINSTANCE ghprevinstance=0;
HWND ghdlg=0;
char gif_fname[MAX_PATH]={0};
int auto_inc_fname=FALSE;
int start_key=VK_RSHIFT;
//int stop_key=VK_LCONTROL;
int stop_key=VK_RSHIFT;
int gif_delay=5;
int cap_delay=33;
int frame_limit=0;
int old_frame_limit=0;
int do_dither=TRUE;

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
	RECT wrect={0};
	GetClientRect(hwnd,&rect);
	GetWindowRect(hwnd,&wrect);
	wrect.left+=GetSystemMetrics(SM_CXDLGFRAME);
	wrect.top+=GetSystemMetrics(SM_CYDLGFRAME)+GetSystemMetrics(SM_CYCAPTION);
	sprintf(title,"gif cap w=%i h=%i x=%i y=%i",rect.right,rect.bottom,wrect.left,wrect.top);
	SetWindowText(hwnd,title);
	return TRUE;
}
int get_main_region(HWND hwnd,HRGN *hregion)
{
	HRGN hr=*hregion;
	RECT rect;
	//if(hr!=0)
	//	DeleteObject(hr);
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
	if(htmp!=0){
		if(hr!=0){
			CombineRgn(hr, hr, htmp, RGN_DIFF);
		}
		DeleteObject(htmp);
	}
	set_title(hwnd);
}

int dither(unsigned char *data,int w,int h,int row_width)
{
	int i,j;
	for(i=0;i<h;i++){
		for(j=0;j<w;j++){
			int x;
			unsigned char r,g,b;
			unsigned char nr,ng,nb;
			unsigned char quant[3];
			x=(i*row_width)+(j*4);
			b=data[x+0];
			g=data[x+1];
			r=data[x+2];
			nr=r&0xC0;
			nb=b&0xE0;
			ng=g&0xE0;
			data[x+0]=nb;
			data[x+1]=ng;
			data[x+2]=nr;
			quant[2]=r-nr;
			quant[1]=g-ng;
			quant[0]=b-nb;
			{
				int k;
				for(k=0;k<3;k++){
					int z;
					unsigned int delta;
					if((j+1)<w){
						z=(i*row_width)+((j+1)*4);
						delta=data[z+k]+(7*quant[k]/16);
						if(delta>0xFF)
							delta=0xFF;
						data[z+k]=delta;
					}
					if((i+1)<h){
						if((j-1)>=0){
							z=((i+1)*row_width)+((j-1)*4);
							delta=data[z+k]+(3*quant[k]/16);
							if(delta>0xFF)
								delta=0xFF;
							data[z+k]=delta;
						}
						{
							z=((i+1)*row_width)+((j+0)*4);
							delta=data[z+k]+(5*quant[k]/16);
							if(delta>0xFF)
								delta=0xFF;
							data[z+k]=delta;
						}
						if((j+1)<w){
							z=((i+1)*row_width)+((j+1)*4);
							delta=data[z+k]+(quant[k]/16);
							if(delta>0xFF)
								delta=0xFF;
							data[z+k]=delta;
						}
					}
				}
			}
		}
	}
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
		colortable[index*3]=i&0xE0; //r<<6;
		colortable[index*3+1]=g<<5;
		colortable[index*3+2]=b<<5;
		index++;
	}
	colortable[index*3]=-1;
	created=TRUE;
	return TRUE;
}
int copy_cmap(RGBQUAD *rgb)
{
	int i;
	for(i=0;i<256;i++){
		colortable[i*3]=rgb[i].rgbRed;
		colortable[i*3+1]=rgb[i].rgbGreen;
		colortable[i*3+2]=rgb[i].rgbBlue;
	}
	colortable[i*3]=-1;
	return TRUE;
}
int grab_pixels(HDC hdc,HBITMAP hbmp,BITMAP *bmp,unsigned char **pixels,int w,int h)
{
	int result=FALSE;
	unsigned char *data;
	int *rgb;
	BITMAPINFO *bmi;

	bmi=malloc(sizeof(BITMAPINFO)+(256*sizeof(RGBQUAD)));
	if(!bmi)
		return result;
	data=GlobalAlloc(GMEM_FIXED,bmp->bmWidthBytes*bmp->bmHeight);
	
	if(!data){
		if(bmi)
			free(bmi);
		return result;
	}
	bmi->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biBitCount=32;
	bmi->bmiHeader.biWidth=bmp->bmWidth;
	bmi->bmiHeader.biHeight=bmp->bmHeight;
	bmi->bmiHeader.biPlanes=bmp->bmPlanes;
	bmi->bmiHeader.biCompression=BI_RGB;
	bmi->bmiHeader.biClrImportant=0;
	bmi->bmiHeader.biClrUsed=0;
	bmi->bmiHeader.biSizeImage = ((bmi->bmiHeader.biWidth * 32 +31)& ~31) /8 * bmi->bmiHeader.biHeight;
	rgb=&bmi->bmiColors[0];
	rgb[0]=0xDEADBEEF;

	if(GetDIBits(hdc,hbmp,0,(WORD)bmp->bmHeight,data,bmi,DIB_RGB_COLORS)){
			*pixels=malloc(w*h);
			if(*pixels!=0){
				if(bmi->bmiHeader.biBitCount==8){
					copy_cmap(bmi->bmiColors);
					memcpy(*pixels,data,w*h);
					result=TRUE;
				}
				else{
					int row_width;
					row_width=bmp->bmWidthBytes;
					if(do_dither)
						dither(data,w,h,row_width);
					downsample(data,w,h,row_width,*pixels);
					result=TRUE;
				}
			}
	}
	if(data)
		GlobalFree((HGLOBAL)data);
	if(bmi)
		free(bmi);

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
			animategif(gsdata,0,gif_delay,0,2);
		}
	}
	else if(step==1){
		unsigned char *pixels=0;
		if(gifimage!=0){
			take_snap(hwnd,&pixels,w,h);
			if(pixels!=0){
				int len;
				char str[80];
				//putgifcolortable(gsdata,colortable);
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
				if(gif_fname[0]!=0){
					FILE *f;
					f=fopen(gif_fname,"wb");
					if(f!=0){
						fwrite(gifimage,1,glen,f);
						fclose(f);
					}
				}
			}
		}
		if(gifimage!=0)
			free(gifimage);
	}

}
int sleep_exit(int t,int key)
{
	DWORD tick,delta=MAXDWORD;
	tick=GetTickCount();
	do{
		if(GetAsyncKeyState(key)&0x8001){
			return TRUE;
		}
		Sleep(1);
		delta=GetTickCount()-tick;
	}while(delta<t);
	return FALSE;
}
int key_thread()
{
	while(TRUE){
		Sleep(100);
		if(GetAsyncKeyState(start_key)&0x8001){
			int i;
			WINDOWPLACEMENT wp={0};
			wp.length=sizeof(wp);
			if(GetWindowPlacement(ghdlg,&wp)){
				if(wp.showCmd==SW_SHOWMINIMIZED)
					continue;
			}
			Beep(1000,100);
			PostMessage(ghdlg,WM_APP,'1',0);
			for(i=0;i<5000;i++){
				PostMessage(ghdlg,WM_APP,'1',1);
				if(sleep_exit(cap_delay,stop_key))
					break;
				if(GetAsyncKeyState(stop_key)&1)
					break;
				if(frame_limit!=0){
					if(frame_limit>i)
						break;
				}
			}
			PostMessage(ghdlg,WM_APP,'1',2);
			Beep(800,100);
			Sleep(250);
			GetAsyncKeyState(start_key);
		}
		else if(GetAsyncKeyState(VK_F1)&0x8001){
			PostMessage(ghdlg,WM_APP,'2',0);
		}
	}
}
int add_trailing_slash(char *fname,int size)
{
	if(fname!=0 && fname[0]!=0 && size>0){
		int len=strlen(fname);
		if((len<size-1) && (len>0)){
			if(fname[len-1]!='\\'){
				fname[len]='\\';
				fname[len+1]=0;
			}
		}
		fname[size-1]=0;
	}
	return TRUE;
}
int dir_exists(char *path)
{
	DWORD attrib=GetFileAttributes(path);
	if((attrib!=MAXDWORD) && (attrib&FILE_ATTRIBUTE_DIRECTORY))
		return TRUE;
	else
		return FALSE;
}
int set_file_path(HWND hwnd,int ctrl)
{
	char path[MAX_PATH]={0};
	const char *a[2]={"B:\\","C:\\TEMP\\"};
	int i;
	for(i=0;i<sizeof(a)/sizeof(char *);i++){
		if(dir_exists(a[i])){
			_snprintf(path,sizeof(path),"%s%s",a[i],"temp.gif");
			break;
		}
	}
	if(path[0]==0){
		GetTempPath(sizeof(path),path);
		if(dir_exists(path)){
			add_trailing_slash(path,sizeof(path));
			_snprintf(path,sizeof(path),"%s%s",path,"temp.gif");
		}
		else
			path[0]=0;
	}
	strncpy(gif_fname,path,sizeof(gif_fname));
	gif_fname[sizeof(gif_fname)-1]=0;
	SetDlgItemText(hwnd,ctrl,path);
	return TRUE;
}

int populate_vkeys(HWND hwnd,int ctrl)
{
extern char *virtual_keys[256];
	int i;
	for(i=0;i<256;i++){
		int index;
		index=SendDlgItemMessage(hwnd,ctrl,CB_ADDSTRING,0,virtual_keys[i]);
		if(index>=0)
			SendDlgItemMessage(hwnd,ctrl,CB_SETITEMDATA,index,i);	
	}
	return TRUE;
}
int set_list_index(HWND hwnd,int ctrl,int key)
{
	int i,count;
	count=SendDlgItemMessage(hwnd,ctrl,CB_GETCOUNT,0,0);
	if(count==CB_ERR)
		return FALSE;
	for(i=0;i<count;i++){
		int data;
		data=SendDlgItemMessage(hwnd,ctrl,CB_GETITEMDATA,i,0);
		if(data!=CB_ERR && data==key){
			SendDlgItemMessage(hwnd,ctrl,CB_SETCURSEL,i,0);
			break;
		}
	}
	return TRUE;
}
int key_changed(HWND hwnd,int ctrl,int *key)
{
	int index;
	index=SendDlgItemMessage(hwnd,ctrl,CB_GETCURSEL,0,0);
	if(index>=0){
		int data;
		data=SendDlgItemMessage(hwnd,ctrl,CB_GETITEMDATA,index,0);
		if(data!=CB_ERR){
			key[0]=data;
			return TRUE;
		}
	}
	return FALSE;
}
int edit_changed(HWND hwnd,int ctrl,int *value)
{
	char str[80]={0};
	GetDlgItemText(hwnd,ctrl,str,sizeof(str));
	value[0]=atoi(str);
	return TRUE;
}
BOOL CALLBACK gifcap(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HRGN hregion=0;
	if(msg!=WM_CTLCOLORSTATIC && msg!=WM_SETCURSOR && msg!=WM_NCHITTEST
		&& msg!=WM_MOUSEMOVE && msg!=WM_MOUSEFIRST)
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
			SetWindowRgn(hwnd,hregion,TRUE);
			SetWindowPos(hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
			SendDlgItemMessage(hwnd,IDC_FNAME,EM_LIMITTEXT,MAX_PATH,0);
			SendDlgItemMessage(hwnd,IDC_FRAME_COUNT,EM_LIMITTEXT,6,0);
			SendDlgItemMessage(hwnd,IDC_GIF_DELAY,EM_LIMITTEXT,6,0);
			SendDlgItemMessage(hwnd,IDC_CAP_DELAY,EM_LIMITTEXT,10,0);
			ShowWindow(GetDlgItem(hwnd,IDC_FRAME_COUNT),FALSE);
			set_file_path(hwnd,IDC_FNAME);
			{
				char str[80]={0};
				sprintf(str,"%i",gif_delay);
				SetDlgItemText(hwnd,IDC_GIF_DELAY,str);
				sprintf(str,"%i",cap_delay);
				SetDlgItemText(hwnd,IDC_CAP_DELAY,str);
			}
			populate_vkeys(hwnd,IDC_START_KEY);
			populate_vkeys(hwnd,IDC_STOP_KEY);
			set_list_index(hwnd,IDC_START_KEY,start_key);
			set_list_index(hwnd,IDC_STOP_KEY,stop_key);
			create_ctable();
		}
		break;
	case WM_APP:
		switch(wparam){
		case '1':
			if(lparam==0){
				clear_window(hwnd,&hregion);
				SetWindowRgn(hwnd,hregion,TRUE);
			}

			animate(hwnd,lparam);
			printf("made it\n");
			break;
		case '2':
			get_main_region(hwnd,&hregion);
			SetWindowRgn(hwnd,hregion,TRUE);
			break;

		}
		break;
	case WM_MOVE:
		set_title(hwnd);
		break;
	case WM_SIZE:
		get_main_region(hwnd,&hregion);
		//clear_window(hwnd,&hregion);
		SetWindowRgn(hwnd,hregion,TRUE);
		break;
	case WM_HELP:
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			{
				HWND hctrl=GetDlgItem(hwnd,IDOK);
				if(hctrl==GetFocus()){
					clear_window(hwnd,&hregion);
					SetWindowRgn(hwnd,hregion,TRUE);
				}
			}
			break;
		case IDC_FNAME:
			switch(HIWORD(wparam)){
			case EN_CHANGE:
				{
					char str[MAX_PATH]={0};
					GetDlgItemText(hwnd,IDC_FNAME,str,sizeof(str));
					strncpy(gif_fname,str,sizeof(gif_fname));
					gif_fname[sizeof(gif_fname)-1]=0;
				}
				printf("gif_fname=%s\n",gif_fname);
				break;
			}
			break;
		case IDC_AUTOINCREMENT:
			if(BST_CHECKED==IsDlgButtonChecked(hwnd,IDC_AUTOINCREMENT))
				auto_inc_fname=TRUE;
			else
				auto_inc_fname=FALSE;
			printf("auto_inc_fname=%i\n",auto_inc_fname);
			break;
		case IDC_FRAME_LIMIT:
			if(BST_CHECKED==IsDlgButtonChecked(hwnd,IDC_FRAME_LIMIT)){
				char str[80]={0};
				ShowWindow(GetDlgItem(hwnd,IDC_FRAME_COUNT),TRUE);
				frame_limit=old_frame_limit;
				sprintf(str,"%i",frame_limit);
				SetDlgItemText(hwnd,IDC_FRAME_COUNT,str);
			}
			else{
				old_frame_limit=frame_limit;
				frame_limit=0;
				printf("frame_limit=%i\n",frame_limit);
				ShowWindow(GetDlgItem(hwnd,IDC_FRAME_COUNT),FALSE);
			}
			break;
		case IDC_FRAME_COUNT:
			switch(HIWORD(wparam)){
			case EN_CHANGE:
				edit_changed(hwnd,IDC_FRAME_COUNT,&frame_limit);
				printf("frame_limit=%i\n",frame_limit);
				break;
			}
			break;
		case IDC_GIF_DELAY:
			switch(HIWORD(wparam)){
			case EN_CHANGE:
				edit_changed(hwnd,IDC_GIF_DELAY,&gif_delay);
				printf("gif_delay=%i\n",gif_delay);
				break;
			}
			break;
		case IDC_CAP_DELAY:
			switch(HIWORD(wparam)){
			case EN_CHANGE:
				edit_changed(hwnd,IDC_CAP_DELAY,&cap_delay);
				printf("cap_delay=%i\n",cap_delay);
				break;
			}
			break;
		case IDC_START_KEY:
			switch(HIWORD(wparam)){
			case CBN_SELCHANGE:
				if(key_changed(hwnd,IDC_START_KEY,&start_key))
					printf("start_key=%i\n",start_key);
				break;
			}
			break;
		case IDC_STOP_KEY:
			switch(HIWORD(wparam)){
			case CBN_SELCHANGE:
				if(key_changed(hwnd,IDC_STOP_KEY,&stop_key))
					printf("stop_key=%i\n",stop_key);
				break;
			}
			break;
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

int CALLBACK WinMain(HINSTANCE hinstance,HINSTANCE hprevinstance,char *cmdline,int showcmd)
{
//	open_console();
	_beginthread(key_thread,0,0);
	return DialogBoxParam(hinstance,MAKEINTRESOURCE(IDD_DIALOG1),NULL,gifcap,NULL);
}