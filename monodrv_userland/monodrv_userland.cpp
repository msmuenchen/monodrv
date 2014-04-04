// monodrv_userland.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "stdafx.h"
#include <windows.h>
void ClearScreen();
void ClearScreen() {
	HANDLE hStdOut;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD count;
	DWORD cellCount;
	COORD homeCoords={0,0};
	hStdOut=GetStdHandle(STD_OUTPUT_HANDLE);
	if(hStdOut==INVALID_HANDLE_VALUE) return;
	if(!GetConsoleScreenBufferInfo(hStdOut,&csbi)) return;
	cellCount=csbi.dwSize.X*csbi.dwSize.Y;
	if(!FillConsoleOutputCharacter(hStdOut,(TCHAR)' ',cellCount,homeCoords,&count)) return;
	if(!FillConsoleOutputAttribute(hStdOut,csbi.wAttributes,cellCount,homeCoords,&count)) return;
	SetConsoleCursorPosition(hStdOut,homeCoords);
}


int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE hFile;
	DWORD dwReturn;
	char* msgs=(char*)malloc(256*256*80);
	char* cptr=NULL;
	unsigned int i,j;
	ZeroMemory(msgs,256*256*80);

	hFile=CreateFileA("\\\\.\\DARKMONO.VXD", 0, 0, 0, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, 0);
	printf("handle 0x%x, mem at %x\n",hFile,msgs);
	if((int)hFile==-1) {
		printf("no handle\n");
		scanf("%d",&dwReturn);
		return 1;
	}
	while(true) {
		ClearScreen();
		ZeroMemory(msgs,256*256*80);
		DeviceIoControl(hFile,0x10,NULL,0,msgs,256*256*80,NULL,NULL);
		
		for(i=0;i<256;i++) {
			for(j=0;j<256;j++) {
				cptr=msgs+(i*256*80)+(j*80);
				if(*cptr!=0)
					printf("%d/%d: %s\n",i,j,cptr);
			}
		}
		Sleep(100);
	}
	return 0;
}

