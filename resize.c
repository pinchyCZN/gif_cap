#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "resource.h"


enum{
	CONTROL_ID,
	XPOS,YPOS,
	WIDTH,HEIGHT,
	HUG_L,
	HUG_R,
	HUG_T,
	HUG_B,
	HUG_CTRL_X,
	HUG_CTRL_Y,
	HUG_HEIGHT,
	HUG_WIDTH,
	HUG_CTRL_TXT_X,
	HUG_CTRL_TXT_Y,
	HUG_CTRL_TXT_X_,
	HUG_CTRL_TXT_Y_,
	SIZE_HEIGHT_OFF,
	SIZE_WIDTH_OFF,
	SIZE_HEIGHT_PER,
	SIZE_WIDTH_PER,
	SIZE_TEXT_CTRL,
	CONTROL_FINISH,
	RESIZE_FINISH
};

int process_anchor_list(HWND hwnd,short *list)
{
	int limit=9999;
	int i=0,j,x,y,width,height;
	HWND dlg_item;
	HDC	 hdc;
	RECT crect;
	SIZE text_size;
	char str[255];
	double f;
	int done=FALSE;
	int last_text=0;

	memset(&crect,0,sizeof(crect));
	hdc=GetDC(hwnd);
	GetClientRect(hwnd, &crect);
	do{
		switch(list[i]){
		case CONTROL_ID:
			x=y=width=height=0;
			dlg_item=GetDlgItem(hwnd,list[i+1]);
			if(dlg_item==NULL)
				done=TRUE;
			break;
		case XPOS:
			x+=list[i+1];
			break;
		case YPOS:
			y+=list[i+1];
			break;
		case WIDTH:
			width+=list[i+1];
			break;
		case HEIGHT:
			height+=list[i+1];
			break;
		case HUG_L:
			x+=crect.left+list[i+1];
			break;
		case HUG_R:
			x+=crect.right+list[i+1];
			break;
		case HUG_T:
			y+=crect.top+list[i+1];
			break;
		case HUG_B:
			y+=crect.bottom+list[i+1];
			break;
		case HUG_CTRL_X:
			break;
		case HUG_CTRL_Y:
			break;
		case HUG_HEIGHT:
			j=crect.bottom-crect.top;
			f=(double)list[i+1]/1000.0;
			y+=j*f;
			break;
		case HUG_WIDTH:
			j=crect.right-crect.left;
			f=(double)list[i+1]/1000.0;
			x+=j*f;
			break;
		case HUG_CTRL_TXT_X:
			if(last_text!=list[i+1]){
				GetDlgItemText(hwnd,list[i+1],str,sizeof(str)-1);
				GetTextExtentPoint32(hdc,str,strlen(str),&text_size);
				last_text=list[i+1];
			}
			x+=text_size.cx;
			break;
		case HUG_CTRL_TXT_X_:
			if(last_text!=list[i+1]){
				GetDlgItemText(hwnd,list[i+1],str,sizeof(str)-1);
				GetTextExtentPoint32(hdc,str,strlen(str),&text_size);
				last_text=list[i+1];
			}
			x-=text_size.cx;
			break;
		case HUG_CTRL_TXT_Y:
			if(last_text!=list[i+1]){
				GetDlgItemText(hwnd,list[i+1],str,sizeof(str)-1);
				GetTextExtentPoint32(hdc,str,strlen(str),&text_size);
				last_text=list[i+1];
			}
			y+=text_size.cy;
			break;
		case HUG_CTRL_TXT_Y_:
			if(last_text!=list[i+1]){
				GetDlgItemText(hwnd,list[i+1],str,sizeof(str)-1);
				GetTextExtentPoint32(hdc,str,strlen(str),&text_size);
				last_text=list[i+1];
			}
			y-=text_size.cy;
			break;
		case SIZE_HEIGHT_OFF:
			height+=crect.bottom-crect.top+list[i+1];
			break;
		case SIZE_WIDTH_OFF:
			width+=crect.right-crect.left+list[i+1];
			break;
		case SIZE_HEIGHT_PER:
			j=crect.bottom-crect.top;
			f=(double)list[i+1]/1000.0;
			height+=f*j;
			break;
		case SIZE_WIDTH_PER:
			j=crect.right-crect.left;
			f=(double)list[i+1]/1000.0;
			width+=f*j;
			break;
		case SIZE_TEXT_CTRL:
			if(last_text!=list[i+1]){
				GetDlgItemText(hwnd,list[i+1],str,sizeof(str)-1);
				GetTextExtentPoint32(hdc,str,strlen(str),&text_size);
				last_text=list[i+1];
			}
			width+=text_size.cx;
			height+=text_size.cy;
			break;
		case CONTROL_FINISH:
			SetWindowPos(dlg_item,NULL,x,y,width,height,SWP_NOZORDER);
			break;
		case RESIZE_FINISH:
			done=TRUE;
			break;
		default:
			printf("bad command %i\n",list[i]);
			break;
		}
		i+=2;
		if(i>limit)
			done=TRUE;
	}while(!done);
	ReleaseDC(hwnd,hdc);
	return TRUE;
}
/*
short install_dlg_anchors[]={
	CONTROL_ID,IDC_INSTALL_INFO,
			XPOS,0,YPOS,10,
			WIDTH,0,HEIGHT,23,
			SIZE_WIDTH_OFF,0,
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_TXT_LOCAL,
			XPOS,92,YPOS,49,
			WIDTH,0,HEIGHT,23,
			SIZE_WIDTH_OFF,-92,
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_TXT_APPDATA,
			XPOS,92,YPOS,85,
			WIDTH,0,HEIGHT,23,
			SIZE_WIDTH_OFF,-92,
			CONTROL_FINISH,-1,
	RESIZE_FINISH
};
*/
int get_word(unsigned char *str,int num,char *out,int olen)
{
	int i,len,count=0,index=0;
	int start_found=FALSE;
	len=strlen(str);
	for(i=0;i<len;i++){
		if(str[i]==','){
			count++;
			if(count>num){
				out[index++]=0;
				break;
			}
		}
		if(!start_found){
			if(count==num && str[i]>' ' && str[i]!=',')
				start_found=TRUE;
		}
		if(start_found){
			if(str[i]<' ' || str[i]=='/'){
				out[index++]=0;
				break;
			}
			if(str[i]>' ')
				out[index++]=str[i];
			if(i==len-1){
				out[index++]=0;
				break;
			}
		}
		if(index>=olen-2){
			out[index++]=0;
			break;
		}
	}
	return TRUE;
}
int modify_list(short *list)
{
	FILE *f;
	char str[1024];
	char *tags[]={
	"CONTROL_ID",
	"XPOS","YPOS",
	"WIDTH","HEIGHT",
	"HUG_L",
	"HUG_R",
	"HUG_T",
	"HUG_B",
	"HUG_CTRL_X",
	"HUG_CTRL_Y",
	"HUG_HEIGHT",
	"HUG_WIDTH",
	"HUG_CTRL_TXT_X",
	"HUG_CTRL_TXT_Y",
	"HUG_CTRL_TXT_X_",
	"HUG_CTRL_TXT_Y_",
	"SIZE_HEIGHT_OFF",
	"SIZE_WIDTH_OFF",
	"SIZE_HEIGHT_PER",
	"SIZE_WIDTH_PER",
	"SIZE_TEXT_CTRL",
	"CONTROL_FINISH",
	"RESIZE_FINISH",
	0
	};
	if(list==0)
		return 0;
	f=fopen("rc.txt","rb");
	if(f!=0){
		int result=FALSE;
		int index=0;
		int done=FALSE;
		printf("-------start---------\n");
		do{
			char *s;
			int i,j;
			str[0]=0;
			result=fgets(str,sizeof(str),f);
			s=strstr(str,"/");
			if(s!=0)
				s[0]=0;
			for(i=0;i<10;i+=2){
				char word[40]={0};
				if(done)
					break;
				get_word(str,i,word,sizeof(word));
				if(strlen(word)==0)
					break;
				if(strlen(word)>0){
					for(j=0;j<100;j++){
						if(tags[j]==0)
							break;
						if(strcmp(word,tags[j])==0){
							int val;
							if(strcmp(word,"CONTROL_ID")==0){
								char w2[40]={0};
								get_word(str,i+1,w2,sizeof(w2));
								printf("%s,%s\n",word,w2);
								index+=2;
								break;
							}
							if(strcmp(word,"CONTROL_FINISH")==0){
								index+=2;
								break;
							}
							if(strcmp(word,"RESIZE_FINISH")==0){
								done=TRUE;
								break;
							}
							if(list[index]==RESIZE_FINISH){
								done=TRUE;
								break;
							}
							list[index++]=j;
							printf("%s,",word);
							word[0]=0;
							get_word(str,i+1,word,sizeof(word));
							printf("%s\n",word);
							if(strlen(word)>0){
								val=atoi(word);
								list[index++]=val;
							}
							else
								index=index;
						}
					}
				}
			}
			if(done)
				break;
		}while(result);
		fclose(f);
	}
	return TRUE;
}

int dump_sizes(HWND hwnd,short *IDC)
{
	int i;
	RECT client,win;
	GetClientRect(hwnd,&client);
	for(i=0;i<1000;i++){
		POINT p={0};
		int width,height;
		int cw,ch;
		if(IDC[i]==RESIZE_FINISH)
			break;
		if(IDC[i]==CONTROL_ID)
			i++;
		else{
			i++;
			continue;
		}
		printf("CONTROL_ID,%i,\n",IDC[i]);
		GetWindowRect(GetDlgItem(hwnd,IDC[i]),&win);
		p.x=win.left;
		p.y=win.top;
		ScreenToClient(hwnd,&p);
		width=win.right-win.left;
		height=win.bottom-win.top;
		cw=client.right-client.left;
		ch=client.bottom-client.top;
		printf("\tXPOS,%i,YPOS,%i, /*x.off=%i y.off=%i*/\n",
			p.x,p.y,
			-client.right+p.x,-client.bottom+p.y);
		printf("\tWIDTH,%i,HEIGHT,%i, /*w.off=%i h.off=%i*/\n\tCONTROL_FINISH,-1,\n",
			width,height,
			-cw+width,-ch+height);

	}
	printf("\n");return 0;
}
int reposition_controls(HWND hwnd, short *list)
{
	RECT	rect;
	GetClientRect(hwnd, &rect);
//modify_list(list);
	process_anchor_list(hwnd,list);
	InvalidateRect(hwnd,&rect,TRUE);
	return TRUE;
}
#define GRIPPIE_SQUARE_SIZE 15
int create_grippy(HWND hwnd)
{
	RECT client_rect;
	GetClientRect(hwnd,&client_rect);
	
	return CreateWindow("Scrollbar",NULL,WS_CHILD|WS_VISIBLE|SBS_SIZEGRIP,
		client_rect.right-GRIPPIE_SQUARE_SIZE,
		client_rect.bottom-GRIPPIE_SQUARE_SIZE,
		GRIPPIE_SQUARE_SIZE,GRIPPIE_SQUARE_SIZE,
		hwnd,NULL,NULL,NULL);
}

int grippy_move(HWND hwnd,HWND grippy)
{
	RECT client_rect;
	GetClientRect(hwnd,&client_rect);
	if(grippy!=0)
	{
		SetWindowPos(grippy,NULL,
			client_rect.right-GRIPPIE_SQUARE_SIZE,
			client_rect.bottom-GRIPPIE_SQUARE_SIZE,
			GRIPPIE_SQUARE_SIZE,GRIPPIE_SQUARE_SIZE,
			SWP_NOZORDER|SWP_SHOWWINDOW);
	}
	return 0;
}