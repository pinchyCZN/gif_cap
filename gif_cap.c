#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "resource.h"
#include "vga737.h"
HINSTANCE ghinstance=0;
HINSTANCE ghprevinstance=0;
HWND ghdlg=0;
int pause_thread=FALSE;
int state_control=0;
int thread_ack=0;
char gif_fname[MAX_PATH]={0};
int auto_inc_fname=FALSE;
int start_key=VK_SNAPSHOT;
int stop_key=VK_SNAPSHOT;
int gif_delay=3;
int cap_delay=29;
int frame_limit=0;
int old_frame_limit=0;
int frame_count=0;
int do_dither=TRUE;
int show_fps=FALSE;

int assert(int a)
{
	if(!a){
		MessageBox(NULL,"ASSERT","ERROR",MB_OK|MB_SYSTEMMODAL);
	}
	return a;
}
int strstri(const char *s1,const char *s2)
{
	int i,j,k;
	for(i=0;s1[i];i++)
		for(j=i,k=0;tolower(s1[j])==tolower(s2[k]);j++,k++)
			if(!s2[k+1])
				return (s1+i);
	return NULL;
}
int draw_char(unsigned char *pixels,int w,int h,unsigned char a,int x,int y,int cf,int cb)
{
	int i,j;
	unsigned char *p=vga737_bin+a*12;
	for(i=0;i<12;i++){
		for(j=0;j<8;j++){
			int c=0;
			int px,py;
			if(p[i]&(1<<(7-j)))
				c=cf;
			else
				c=cb;
			px=x+j;
			py=y+i;
			if(px<w && py<h){
				pixels[px+py*w]=c;
			}
		}
	}
	return 0;
}
int draw_string(char *pixels,int w,int h,char *str,int x,int y,int cf,int cb)
{
	int i,len;
	len=strlen(str);
	for(i=0;i<len;i++){
		if((x+i*8)>w)
			break;
		draw_char(pixels,w,h,str[i],x+i*8,y,cf,cb);
	}
	return 0;
}
int inc_fname(char *path,int pathlen)
{
	char drive[_MAX_DRIVE]={0},dir[_MAX_DIR]={0},fname[_MAX_FNAME]={0},ext[_MAX_EXT]={0};
	_splitpath(path,drive,dir,fname,ext);
	if(fname[0]!=0){
		int i,len,num=0;
		len=strlen(fname);
		for(i=len-1;i>=0;i--){
			char *s;
			if(isdigit(fname[i])){
				if(i==0){
					s=fname;
					goto INC;
				}
				continue;
			}
			else{
				s=fname+i+1;
INC:
				if(s[0]!=0){
					num=atoi(s);
					num++;
					s[0]=0;
				}
				break;
			}
		}
		_snprintf(path,pathlen,"%s%s%s%03i%s",drive,dir,fname,num,ext);
	}
	return TRUE;
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
//6 color steps in r,g,b direction
unsigned char colors[6]={0,51,102,153,204,255};
//used to help find closest color
unsigned char mid_colors[5]={26,76,128,178,230};
static int colortable[256*3+1];
//look up tables to speed things up
static unsigned char color_lut[256];
static unsigned char color_index_lut[256];
int closest_color_index(unsigned char c)
{
	int i;
	for(i=0;i<sizeof(mid_colors)/sizeof(char);i++){
		if(c<=mid_colors[i]){
			return i;
		}
	}
	return (sizeof(colors)/sizeof(unsigned char))-1;
}
int closest_color(unsigned char c)
{
	int i;
	for(i=0;i<sizeof(mid_colors)/sizeof(char);i++){
		if(c<=mid_colors[i]){
			return colors[i];
		}
	}
	return colors[(sizeof(colors)/sizeof(unsigned char))-1];
}
int create_ctable()
{
	int i,index;
	static int created=FALSE;
	if(created)
		return TRUE;

	for(i=0;i<256;i++){
		color_lut[i]=closest_color(i);
		color_index_lut[i]=closest_color_index(i);
	}

	index=0;
	{
		int x,y,z;
		for(x=0;x<6;x++){
			for(y=0;y<6;y++){
				for(z=0;z<6;z++){
					colortable[index*3]=colors[x];
					colortable[index*3+1]=colors[y];
					colortable[index*3+2]=colors[z];
					index++;
				}
			}
		}

	}
	//table must be size of power of 2, pad out rest of colors
	for(i=index;i<256;i++){
		colortable[index*3]=255;
		colortable[index*3+1]=255;
		colortable[index*3+2]=255;
		index++;
	}
	colortable[index*3]=-1;
	created=TRUE;
	return index;
}

int get_color_index(unsigned char r,unsigned char g,unsigned char b)
{
	int index=0;
	index+=color_index_lut[b];
	index+=color_index_lut[g]*6;
	index+=color_index_lut[r]*6*6;
	return index;
}
int dither(unsigned char *data,int w,int h,int row_width)
{
	int i,j;
	for(i=0;i<h;i++){
		for(j=0;j<w;j++){
			int x;
			unsigned char r,g,b;
			unsigned char nr,ng,nb;
			char quant[3];
			x=(i*row_width)+(j*4);
			b=data[x+0];
			g=data[x+1];
			r=data[x+2];
			nr=color_lut[r];
			nb=color_lut[b];
			ng=color_lut[g];
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
					int delta;
					if((j+1)<w){
						z=(i*row_width)+((j+1)*4);
						delta=data[z+k]+(7*quant[k]/16);
						if(delta>0xFF)
							delta=0xFF;
						else if(delta<0)
							delta=0;
						data[z+k]=delta;
					}
					if((i+1)<h){
						if((j-1)>=0){
							z=((i+1)*row_width)+((j-1)*4);
							delta=data[z+k]+(3*quant[k]/16);
							if(delta>0xFF)
								delta=0xFF;
							else if(delta<0)
								delta=0;
							data[z+k]=delta;
						}
						{
							z=((i+1)*row_width)+((j+0)*4);
							delta=data[z+k]+(5*quant[k]/16);
							if(delta>0xFF)
								delta=0xFF;
							else if(delta<0)
								delta=0;
							data[z+k]=delta;
						}
						if((j+1)<w){
							z=((i+1)*row_width)+((j+1)*4);
							delta=data[z+k]+(quant[k]/16);
							if(delta>0xFF)
								delta=0xFF;
							else if(delta<0)
								delta=0;
							data[z+k]=delta;
						}
					}
				}
			}
		}
	}
}



int downsample(unsigned char *data,int w,int h,int row_width,unsigned char *pixels)
{
	if(pixels!=0){
		int i,j,poffset=0;
		for(i=h-1;i>=0;i--){
			for(j=0;j<w;j++){
				int x=(i*row_width)+(j*4);
				unsigned char index;
				unsigned char r,g,b;
				b=data[x+0];
				g=data[x+1];
				r=data[x+2];
				index=get_color_index(r,g,b);
				pixels[poffset++]=index;
			}
		}
	}
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
	BITMAPINFO _bmi={0};
	bmi=&_bmi;

	//bmi=malloc(sizeof(BITMAPINFO)+(256*sizeof(RGBQUAD)));
	//if(!bmi)
	//	return result;
	data=GlobalAlloc(GMEM_FIXED,bmp->bmWidthBytes*bmp->bmHeight);
	
	if(!data){
		//if(bmi)
		//	free(bmi);
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
	//rgb=&bmi->bmiColors[0];
	//rgb[0]=0xDEADBEEF;

	if(GetDIBits(hdc,hbmp,0,(WORD)bmp->bmHeight,data,bmi,DIB_RGB_COLORS)){
			*pixels=malloc(w*h);
			if(*pixels!=0){
				/*
				if(bmi->bmiHeader.biBitCount==8){
					copy_cmap(bmi->bmiColors);
					memcpy(*pixels,data,w*h);
					result=TRUE;
				}
				else
				*/
				{
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
	//if(bmi)
	//	free(bmi);

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

		gifimage=0;
		gsdata=newgif(&gifimage,w,h,colortable,0);
		if(gsdata!=0){
			animategif(gsdata,0,gif_delay,0,2);
		}
		frame_count=0;
	}
	else if(step==1){
		unsigned char *pixels=0;
		if(gsdata!=0){
			take_snap(hwnd,&pixels,w,h);
			if(pixels!=0){
				int len;
				char str[80];
				double fps=0;
				DWORD delta=0;
				if(show_fps){
					static DWORD tick=0,ctick;
					ctick=GetTickCount();
					if(frame_count==0)
						tick=ctick;
					delta=ctick-tick;
					if(delta!=0)
						fps=1000/delta;
					else
						fps=0;
					sprintf(str,"%.0f",fps);
					tick=ctick;
					draw_string(pixels,w,h,str,0,0,255,0);
				}
				len=putgif(gsdata,pixels);
				frame_count++;
				if(show_fps)
					_snprintf(str,sizeof(str),"%i FPS=%.0f delta=%u (%i)",frame_count,fps,delta,len);
				else
					_snprintf(str,sizeof(str),"%i (%i)",frame_count,len);
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
			_snprintf(str,sizeof(str),"DONE frames=%i size=%i",frame_count,glen);
			SetWindowText(hwnd,str);
			if(auto_inc_fname){
				inc_fname(gif_fname,sizeof(gif_fname));
				SetDlgItemText(hwnd,IDC_FNAME,gif_fname);
			}
			if(gif_fname[0]!=0){
				FILE *f;
				f=fopen(gif_fname,"wb");
				if(f!=0){
					if(gifimage!=0)
						fwrite(gifimage,1,glen,f);
					fclose(f);
				}
				else{
					char str[MAX_PATH]={0};
					_snprintf(str,sizeof(str),"invalid file:%s",gif_fname);
					str[sizeof(str)-1]=0;
					SetWindowText(hwnd,str);
					PostMessage(hwnd,WM_APP,'2',0);
				}
			}
		}
		if(gifimage!=0){
			free(gifimage);
			gifimage=0;
		}
	}
	thread_ack=1;
	return TRUE;
}
int sleep_exit(int delay,DWORD *tick,int key)
{
	DWORD tc,delta=MAXDWORD;
	do{
		Sleep(1);
		if(GetAsyncKeyState(key)&0x0001){
			return TRUE;
		}
		tc=GetTickCount();
		delta=tc-*tick;
	}while(delta<delay);
	*tick=tc;
	return FALSE;
}
int key_thread()
{
	int show_window_key=VK_F1;
	while(TRUE){
		Sleep(100);
		if(ghdlg==0)
			continue;
		if(pause_thread){
			while(pause_thread){
				Sleep(100);
			}
			GetAsyncKeyState(start_key);
			continue;
		}
		if(start_key==VK_F1)
			show_window_key=VK_F2;
		else
			show_window_key=VK_F1;
		if(GetAsyncKeyState(start_key)&0x8001){
			int i;
			int frame_done=FALSE;
			DWORD tick;
			WINDOWPLACEMENT wp={0};
			wp.length=sizeof(wp);
			if(GetWindowPlacement(ghdlg,&wp)){
				if(wp.showCmd==SW_SHOWMINIMIZED)
					continue;
			}
			if(!verify_path(gif_fname)){
				char str[MAX_PATH]={0};
				_snprintf(str,sizeof(str),"invalid path:%s",gif_fname);
				str[sizeof(str)-1]=0;
				SetWindowText(ghdlg,str);
				PostMessage(ghdlg,WM_APP,'2',0);
				continue;
			}
			state_control=1;
			Beep(1000,100);
			thread_ack=0;
			tick=GetTickCount();
			PostMessage(ghdlg,WM_APP,'1',0);
			while(TRUE){
				if(thread_ack){
					thread_ack=0;
					if(frame_limit!=0){
						if(frame_count>=frame_limit)
							break;
					}
					PostMessage(ghdlg,WM_APP,'1',1);
					frame_done=TRUE;
				}
				if(sleep_exit(cap_delay,&tick,stop_key))
					break;
				if(state_control!=1)
					break;
			}
			if(!frame_done)
				PostMessage(ghdlg,WM_APP,'1',1);
			PostMessage(ghdlg,WM_APP,'1',2);
			Beep(800,100);
			Sleep(250);
			GetAsyncKeyState(start_key);
			if(state_control!=1)
				PostMessage(ghdlg,WM_APP,'2',0);

			state_control=0;
		}
		else if(GetAsyncKeyState(show_window_key)&0x8001){
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
int verify_path(char *path)
{
	char tmp[MAX_PATH]={0};
	char drive[_MAX_DRIVE]={0},dir[_MAX_DIR]={0},fname[_MAX_FNAME]={0},ext[_MAX_EXT]={0};
	_splitpath(path,drive,dir,fname,ext);
	_snprintf(tmp,sizeof(tmp),"%s%s",drive,dir);
	tmp[sizeof(tmp)-1]=0;
	if(dir_exists(tmp))
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
int set_keybutton_text(HWND hwnd,int ctrl,int key)
{
extern char *virtual_keys[];
	int result=FALSE;
	char *s=virtual_keys[key&0xFF];
	HWND h;
	h=GetDlgItem(hwnd,ctrl);
	if(h){
		SetWindowText(h,s);
		result=TRUE;
	}
	return result;
}
int edit_changed(HWND hwnd,int ctrl,int *value)
{
	char str[80]={0};
	GetDlgItemText(hwnd,ctrl,str,sizeof(str));
	value[0]=atoi(str);
	return TRUE;
}
WNDPROC orig_edit=0;
static int last_key=0;
LRESULT APIENTRY subclass_edit(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	//print_msg(msg,lparam,wparam,hwnd);
	switch(msg){
	case WM_SYSKEYUP:
	case WM_KEYUP:
		{
			int code=(lparam>>16)&0xFF;
			int extended=(lparam>>24)&1;
			int vkey;
			vkey=MapVirtualKey(code,3);
			if(vkey!=0){
				if(wparam==VK_SNAPSHOT)
					last_key=VK_SNAPSHOT;
				else{
					if(vkey==VK_LMENU && extended)
						vkey=VK_RMENU;
					if(vkey==VK_LCONTROL && extended)
						vkey=VK_RCONTROL;
					last_key=vkey;
				}
			}
			else
				last_key=wparam;
			PostMessage(GetParent(hwnd),WM_APP,0,0);
		}
		break;
	}
	return CallWindowProc(orig_edit,hwnd,msg,wparam,lparam); 
}
BOOL CALLBACK get_hotkey(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static int *key=0;
	switch(msg){
	case WM_INITDIALOG:
		orig_edit=SetWindowLong(GetDlgItem(hwnd,IDC_EDIT1),GWL_WNDPROC,subclass_edit);
		last_key=0;
		key=lparam;
		if(key==0)
			EndDialog(hwnd,0);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		}
		break;
	case WM_APP:
		if(last_key!=VK_ESCAPE)
			key[0]=last_key;
		EndDialog(hwnd,0);
		break;
	case WM_CLOSE:
		EndDialog(hwnd,0);
		break;
	}
	return 0;
}
BOOL CALLBACK gifcap(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HRGN hregion=0;
	static HWND hgrippy=0;
#ifdef _DEBUG
	if(FALSE)
	if(msg!=WM_CTLCOLORSTATIC && msg!=WM_SETCURSOR && msg!=WM_NCHITTEST
		&& msg!=WM_MOUSEMOVE && msg!=WM_MOUSEFIRST)
	{
		static DWORD tick=0;
		print_msg(msg,lparam,wparam,hwnd);
	}
#endif
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
			set_keybutton_text(hwnd,IDC_START_KEY,start_key);
			set_keybutton_text(hwnd,IDC_STOP_KEY,stop_key);
			if(do_dither)
				CheckDlgButton(hwnd,IDC_DITHER,BST_CHECKED);
			create_ctable();
			hgrippy=create_grippy(hwnd);
			if(hgrippy)
				SetWindowPos(hgrippy,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
			_beginthread(key_thread,0,0);
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
			break;
		case '2':
			get_main_region(hwnd,&hregion);
			SetWindowRgn(hwnd,hregion,TRUE);
			break;
		case '3':
			SetFocus(GetDlgItem(hwnd,IDCANCEL));
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
		grippy_move(hwnd,hgrippy);
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
		case IDC_DITHER:
			if(BST_CHECKED==IsDlgButtonChecked(hwnd,IDC_DITHER))
				do_dither=TRUE;
			else
				do_dither=FALSE;
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
		case IDC_FPS:
			if(BST_CHECKED==IsDlgButtonChecked(hwnd,IDC_FPS))
				show_fps=TRUE;
			else
				show_fps=FALSE;
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
			pause_thread=TRUE;
			DialogBoxParam(ghinstance,IDD_GET_KEY,hwnd,get_hotkey,&start_key);
			stop_key=start_key;
			set_keybutton_text(hwnd,IDC_START_KEY,start_key);
			set_keybutton_text(hwnd,IDC_STOP_KEY,stop_key);
			pause_thread=FALSE;
			break;
		case IDC_STOP_KEY:
			pause_thread=TRUE;
			DialogBoxParam(ghinstance,IDD_GET_KEY,hwnd,get_hotkey,&stop_key);
			set_keybutton_text(hwnd,IDC_STOP_KEY,stop_key);
			pause_thread=FALSE;
			break;
		case IDCANCEL:
			if(state_control==0){
				get_main_region(hwnd,&hregion);
				SetWindowRgn(hwnd,hregion,TRUE);
				if(IDOK==MessageBox(hwnd,"OK to exit?","Exit?",MB_OKCANCEL|MB_SYSTEMMODAL))
					PostMessage(hwnd,WM_CLOSE,0,0);
			}
			else
				state_control=2;
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
#ifdef _DEBUG
	open_console();
#endif
	return DialogBoxParam(hinstance,MAKEINTRESOURCE(IDD_DIALOG1),NULL,gifcap,NULL);
}