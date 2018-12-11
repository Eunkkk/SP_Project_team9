#include <ncurses.h>
#include <menu.h>
#include <panel.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

void EntityListWinDisplay(WINDOW *entity_list_win);
void DirectoryWinDisplay(WINDOW * directory_dispaly_win,const char * absDIRstr);
void getCurrDir(char **absDIRstr, long * maxpath);
void print_in_middle(WINDOW *win,int starty,int startx, int width, char *string,chtype color);



int main(){

	WINDOW *entity_list_win, *directory_display_win;
	
	char * absDIRstr;
	long maxpath;

	entity_list_win=(WINDOW *)NULL;
	directory_display_win=(WINDOW *)NULL;

	absDIRstr=(char *)NULL;
	
	initscr();
	start_color();


	getCurrDir(&absDIRstr, &maxpath);

	refresh();

	directory_display_win=newwin(3,COLS,0,0);
	entity_list_win=newwin(LINES-7, COLS, 3, 0);

	DirectoryWinDisplay(directory_display_win,absDIRstr);
	EntityListWinDisplay(entity_list_win);

	wrefresh(entity_list_win);

	

	return 0;
}

void DirectoryWinDisplay(WINDOW *directory_display_win, const char *absDIRstr){
	box(directory_display_win,0,0);

	wmove(directory_display_win,1,2);
	wprintw(directory_display_win,"현재 디렉토리 = ");

 	wclrtoeol(directory_display_win);
	wattron(directory_display_win, A_BOLD | COLOR_PAIR(2));
	wprintw(directory_display_win, "%s",absDIRstr);
	wattroff(directory_display_win ,A_BOLD | COLOR_PAIR(2));
	mvwaddch(directory_display_win, 1, COLS-1, ACS_VLINE);

	wrefresh(directory_display_win);
}
void EntityListWinDisplay(WINDOW *entity_list_win)
{
	int max_x, max_y;

    // 개체 리스트 메뉴를 붙일 윈도우 생성.

	getmaxyx(entity_list_win, max_y, max_x);    // 윈도우가 취할 수 있는 최대 좌표
                            // 결국 우측끝 최하단 좌표.
	box(entity_list_win, 0, 0);
	print_in_middle(entity_list_win, 1, 0, max_x, "개체 이름 / 크기", COLOR_PAIR(1)|A_BOLD);

    // 목록 윈도우 ESC키 문자 구분
	mvwaddch(entity_list_win,0, max_x-14, ACS_TTEE);    // ㅜ 문자 삽입
	mvwaddch(entity_list_win, 1, max_x-14, ACS_VLINE);
	mvwprintw(entity_list_win, 1, max_x-12, "종료:");

	wattron(entity_list_win, A_BOLD | COLOR_PAIR(4));
	wprintw(entity_list_win, "[F12]");
	wattroff(entity_list_win, A_BOLD | COLOR_PAIR(4));

    // window안에 ㅏ-----ㅓ 삽입
	mvwaddch(entity_list_win, 2, 0, ACS_LTEE);
	mvwhline(entity_list_win, 2, 1, ACS_HLINE, max_x-2);
	mvwaddch(entity_list_win, 2, max_x-1, ACS_RTEE);

	mvwaddch(entity_list_win, 2, max_x-14, ACS_BTEE);   // ESC구분자 ㅗ  문자 삽입

	wrefresh(entity_list_win);
} // EntityListWinDisplay() end

void getCurrDir(char **absDIRstr, long *maxpath)
{
	if((*maxpath = pathconf(".",_PC_PATH_MAX))==-1)
	{
		endwin();
		fprintf(stderr,"경로 이름 길이를 결정하는데 실패\n");
		perror(" ");
		exit(-13);
	}
	if( (*absDIRstr = (char *)malloc(*maxpath)) == NULL )
	{
		endwin();
	        perror("경로이름을 위한 공간을 할당하는데 실패하였습니다.");
	        exit(-14);
	}


	if( getcwd(*absDIRstr, *maxpath) == NULL )
	{
        	endwin();
	        fprintf(stderr, "현재 작업 디렉토리를 열수 없습니다.\n");
        	perror(" ");
	        exit(-15);
   	}
}

void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color)
{
	int length, x, y;
	float temp;

	if(win == NULL)
		win = stdscr;
	getyx(win, y, x);
	if(startx != 0)
        	x = startx;
	if(starty != 0)
	        y = starty;
	if(width == 0)
	        width=COLS;

	length = strlen(string);
	temp = (width - length)/2;
	x = startx + (int)temp;
	wattron(win, color);
	mvwprintw(win, y, x, "%s", string);
	wattroff(win, color);

}

