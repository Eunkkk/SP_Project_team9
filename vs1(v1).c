#include <ncurses.h>
#include <menu.h>
#include <panel.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <dirent.h>
#include <sys/stat.h>

chtype volatile main_ipt_ch; //getch 반환값을 받


void EntityListWinDisplay(WINDOW *entity_list_win);
void DirectoryWinDisplay(WINDOW * directory_dispaly_win, const char * absDIRstr);
void getCurrDir(char **absDIRstr, long * maxpath);
void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color);
//디렉토리 생성 윈도
void Dlg_Mkdir(WINDOW *dlg_win_mkdir, const char *curDirName, char *usrIptName);
//단축키 표시 윈도우
void ShortCutWinDisplay(WINDOW *shortcut_display_win);


int main() {
	//한글을 입력받기 위한 설정.
	setlocale(LC_ALL, "ko_KR.utf8");
	setenv("NCURSES_NO_UTF8_ACS", "1", 0);

	PANEL *top;

	WINDOW *entity_list_win, *shortcut_display_win, *entity_detail_display_win,
		*directory_display_win, *dlg_win_mkdir, *dlg_win_rename, *dlg_win_chmod;//윈도우생성변수

	char * absDIRstr;
	long maxpath;

	entity_list_win = (WINDOW *)NULL;					//
	shortcut_display_win = (WINDOW *)NULL;			//하단부 명령줄 생성
	directory_display_win = (WINDOW *)NULL;			//
	entity_detail_display_win = (WINDOW *)NULL;	//
	dlg_win_mkdir = (WINDOW *)NULL;
	absDIRstr = (char *)NULL;

	char usrIptName[100];  	//사용자가 입력한 파일명 또는 디렉토리명 저장

	initscr();
	start_color();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	ESCDELAY = 0;

	getCurrDir(&absDIRstr, &maxpath);

	refresh();

	directory_display_win = newwin(3, COLS, 0, 0);
	entity_list_win = newwin(LINES - 7, COLS, 3, 0);
	entity_detail_display_win = newwin(1, COLS, LINES - 4, 0);
	shortcut_display_win = newwin(3, COLS, LINES - 3, 0);

	DirectoryWinDisplay(directory_display_win, absDIRstr);
	EntityListWinDisplay(entity_list_win);
	ShortCutWinDisplay(shortcut_display_win);

	while (1) {
		if (main_ipt_ch != 0x03)
			if (main_ipt_ch != KEY_F(12)) {
				main_ipt_ch = wgetch(entity_list_win);
			}
		if (main_ipt_ch == KEY_F(12)) {
			break;
		}

		switch (main_ipt_ch)
		{
		case 0x6b: 	//mkdir 명령
			Dlg_Mkdir(dlg_win_mkdir, absDIRstr, &usrIptName[0]);
			top = (PANEL *)panel_userptr(top);
			top_panel(top);

			update_panels();
			mvwaddch(shortcut_display_win, 1, COLS - 2, 'K' | A_BOLD | A_BLINK | COLOR_PAIR(4));
			break;
		}

		wrefresh(entity_list_win);
		wrefresh(entity_detail_display_win);
		wrefresh(shortcut_display_win);
	}


	//메모리 해제
	delwin(directory_display_win);
	delwin(entity_list_win);
	delwin(entity_detail_display_win);
	delwin(shortcut_display_win);
	delwin(dlg_win_mkdir);
	delwin(dlg_win_rename);
	delwin(dlg_win_chmod);

	endwin();

	unsetenv("NCURSES_NO_UTF8_ACS");

	return 0;
}

void DirectoryWinDisplay(WINDOW *directory_display_win, const char *absDIRstr) {
	box(directory_display_win, 0, 0);

	wmove(directory_display_win, 1, 2);
	wprintw(directory_display_win, "현재 디렉토리 : ");

	wclrtoeol(directory_display_win);
	wattron(directory_display_win, A_BOLD | COLOR_PAIR(2));
	wprintw(directory_display_win, "%s", absDIRstr);
	wattroff(directory_display_win, A_BOLD | COLOR_PAIR(2));
	mvwaddch(directory_display_win, 1, COLS - 1, ACS_VLINE);

	wrefresh(directory_display_win);
}
void EntityListWinDisplay(WINDOW *entity_list_win)
{
	int max_x, max_y;

	// 개체 리스트 메뉴를 붙일 윈도우 생성.

	getmaxyx(entity_list_win, max_y, max_x);    // 윈도우가 취할 수 있는 최대 좌표
												// 결국 우측끝 최하단 좌표.
	box(entity_list_win, 0, 0);
	print_in_middle(entity_list_win, 1, 0, max_x, "개체 이름 / 크기", COLOR_PAIR(1) | A_BOLD);

	// 목록 윈도우 ESC키 문자 구분
	mvwaddch(entity_list_win, 0, max_x - 14, ACS_TTEE);    // ㅜ 문자 삽입
	mvwaddch(entity_list_win, 1, max_x - 14, ACS_VLINE);
	mvwprintw(entity_list_win, 1, max_x - 12, "종료:");

	wattron(entity_list_win, A_BOLD | COLOR_PAIR(4));
	wprintw(entity_list_win, "[F12]");
	wattroff(entity_list_win, A_BOLD | COLOR_PAIR(4));

	// window안에 ㅏ-----ㅓ 삽입
	mvwaddch(entity_list_win, 2, 0, ACS_LTEE);
	mvwhline(entity_list_win, 2, 1, ACS_HLINE, max_x - 2);
	mvwaddch(entity_list_win, 2, max_x - 1, ACS_RTEE);

	mvwaddch(entity_list_win, 2, max_x - 14, ACS_BTEE);   // ESC구분자 ㅗ  문자 삽입

	wrefresh(entity_list_win);
} // EntityListWinDisplay() end
void ShortCutWinDisplay(WINDOW *shortcut_display_win)
{
	int win_x, win_y;

	char botLabelCtrl[] = { "Ctrl+" };
	char botLabelCopy[] = { "Copy( )" };
	char botLabelMove[] = { "Move( )" };
	char botLabelPaste[] = { "Paste( )" };
	char botLabelRename[] = { "Rename( )" };
	char botLabelChmod[] = { "Chmod( )" };
	char botLabelMkdir[] = { "Mkdir( )" };
	char botLabelDel[] = { "Delete( )" };
	char botLabelFMove[] = { "[ ]" };

	char botLabelKey[] = { 'c','o','v','n','h','k','d','/' };

	char *botLabel[9] = { &botLabelCtrl[0],&botLabelCopy[0],&botLabelMove[0],
		&botLabelPaste[0],&botLabelRename[0],&botLabelChmod[0],&botLabelMkdir[0],
		&botLabelDel[0],&botLabelFMove[0] };

	int label_x;
	int i;

	getbegyx(shortcut_display_win, win_y, win_x);

	label_x = win_x + 2;
	box(shortcut_display_win, 0, 0);

	wattron(shortcut_display_win, COLOR_PAIR(3) | A_BOLD);

	mvwaddstr(shortcut_display_win, 1, label_x, botLabel[0]);
	for (i = 1; i<9; i++) {

		if (i == 8)continue;
		label_x = (label_x + strlen(botLabel[i - 1]) + 1);

		mvwaddstr(shortcut_display_win, 1, label_x, botLabel[i]);
		mvwaddch(shortcut_display_win, 1, label_x + strlen(botLabel[i]) - 2, botLabelKey[i - 1] | COLOR_PAIR(4) | A_BOLD | A_UNDERLINE);
	}

	wattroff(shortcut_display_win, COLOR_PAIR(3) | A_BOLD);
	wrefresh(shortcut_display_win);
}

void getCurrDir(char **absDIRstr, long *maxpath)
{
	if ((*maxpath = pathconf(".", _PC_PATH_MAX)) == -1)
	{
		endwin();
		fprintf(stderr, "경로 이름 길이를 결정하는데 실패\n");
		perror(" ");
		exit(-13);
	}
	if ((*absDIRstr = (char *)malloc(*maxpath)) == NULL)
	{
		endwin();
		perror("경로이름을 위한 공간을 할당하는데 실패하였습니다.");
		exit(-14);
	}


	if (getcwd(*absDIRstr, *maxpath) == NULL)
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

	if (win == NULL)
		win = stdscr;
	getyx(win, y, x);
	if (startx != 0)
		x = startx;
	if (starty != 0)
		y = starty;
	if (width == 0)
		width = COLS;

	length = strlen(string);
	temp = (width - length) / 2;
	x = startx + (int)temp;
	wattron(win, color);
	mvwprintw(win, y, x, "%s", string);
	wattroff(win, color);

}


void Dlg_Mkdir(WINDOW *dlg_win_mkdir, const char *curDirName, char *usrIptName)
{
	int y, x, max_y, max_x;
	int i;
	int attr_style;

	struct stat statbuf; // 파일모드 정보들을 저장할 stat형의 구조체 선언.
	mode_t perm = 0000000;    // 권한 초기값

	chtype ipt;

	ipt = 0;
	usrIptName[0] = '\0';
	keypad(dlg_win_mkdir, TRUE); // initscr()이전의 이 함수 호출은 의미없음.

	getmaxyx(dlg_win_mkdir, max_y, max_x);

	box(dlg_win_mkdir, 0, 0);

	wattron(dlg_win_mkdir, A_BOLD);
	mvwprintw(dlg_win_mkdir, 0, 5, " 디렉토리 생성 (Mkdir)");
	wattroff(dlg_win_mkdir, A_BOLD);

	wattron(dlg_win_mkdir, A_BOLD | COLOR_PAIR(8));
	mvwprintw(dlg_win_mkdir, 2, 3, " 현재 디렉토리      ");

	mvwprintw(dlg_win_mkdir, 4, 3, " 만들 디렉터리 이름 ");

	mvwprintw(dlg_win_mkdir, 6, 24, "  확 인  "); // 버튼 1

	mvwprintw(dlg_win_mkdir, 6, 40, "  취 소  "); // 버튼 2
	wattroff(dlg_win_mkdir, A_BOLD | COLOR_PAIR(8));


	attr_style = A_BOLD | COLOR_PAIR(7);
	wmove(dlg_win_mkdir, 2, 23);

	wattron(dlg_win_mkdir, attr_style);

	for (i = 23; i<max_x - 2; i++)
	{
		mvwaddch(dlg_win_mkdir, 2, i, ' ');
		mvwaddch(dlg_win_mkdir, 4, i, ' ');
	}
	mvwprintw(dlg_win_mkdir, 2, 24, curDirName);

	echo(); // 사용자의 입력을 화면에 표시하기 위함.
	keypad(dlg_win_mkdir, FALSE);
	mvwgetnstr(dlg_win_mkdir, 4, 24, usrIptName, max_x - 28);
	keypad(dlg_win_mkdir, TRUE);
	noecho();

	wattroff(dlg_win_mkdir, attr_style);

	mvwchgat(dlg_win_mkdir, 6, 24, 9, A_BOLD | A_UNDERLINE, 8, NULL);
	mvwchgat(dlg_win_mkdir, 6, 40, 9, A_NORMAL, 8, NULL);
	wmove(dlg_win_mkdir, 6, 24);

	while ((ipt = wgetch(dlg_win_mkdir)) != '\n')
	{
		switch (ipt)
		{
		case KEY_LEFT:
			wmove(dlg_win_mkdir, 6, 24);
			wchgat(dlg_win_mkdir, 9, A_BOLD | A_UNDERLINE, 8, NULL);
			mvwchgat(dlg_win_mkdir, 6, 40, 9, A_NORMAL, 8, NULL);
			wmove(dlg_win_mkdir, 6, 24);
			break;

		case KEY_RIGHT:
			wmove(dlg_win_mkdir, 6, 40);
			wchgat(dlg_win_mkdir, 9, A_BOLD | A_UNDERLINE, 8, NULL);
			mvwchgat(dlg_win_mkdir, 6, 24, 9, A_NORMAL, 8, NULL);
			wmove(dlg_win_mkdir, 6, 40);
			break;

		case KEY_UP:
			mvwchgat(dlg_win_mkdir, 6, 24, 9, A_NORMAL, 8, NULL);
			mvwchgat(dlg_win_mkdir, 6, 40, 9, A_NORMAL, 8, NULL);

			wattron(dlg_win_mkdir, attr_style);
			for (i = 23; i<max_x - 2; i++)
			{
				mvwaddch(dlg_win_mkdir, 4, i, ' ');
			}
			echo(); // 사용자의 입력을 화면에 표시하기 위함.
			keypad(dlg_win_mkdir, FALSE);
			mvwgetnstr(dlg_win_mkdir, 4, 24, usrIptName, max_x - 28);
			keypad(dlg_win_mkdir, TRUE);

			noecho();
			wattroff(dlg_win_mkdir, attr_style);
			wmove(dlg_win_mkdir, 6, 24);
			wchgat(dlg_win_mkdir, 9, A_BOLD | A_UNDERLINE, 8, NULL);

			break;
		}
		wrefresh(dlg_win_mkdir);
	}

	getyx(dlg_win_mkdir, y, x); // 현재 커서의 좌표값얻어옴.
								// 사용자가 선택한 좌표의 위치로 [확인]인지 [취소]인지 선택하기로함.
	if ((x > 39 && x < 50) || usrIptName[0] == '\0') // 취소 상태.
	{
		usrIptName[0] = '\0';
	}
	else {
		// 확인 이므로 디렉토리 생성 코드 실행.

		if (lstat(curDirName, &statbuf) == -1)
		{
			perror(" ");
			perm = 0000700;
		}
		perm = statbuf.st_mode; // 바로 상위디렉토리의 권한을 그대로 이어받도록 하자.

		mkdir(usrIptName, perm); // 디렉토리 생성. 
	}

}   // Dlg_Mkdir() end
