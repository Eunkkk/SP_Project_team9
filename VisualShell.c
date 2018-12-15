#include <ncursesw/curses.h>
#include <menu.h>
#include <panel.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>	//struct dirent
#include <signal.h>

static volatile sig_atomic_t mark_val;
chtype volatile main_ipt_ch; //getch 반환값을 받음


void EntityListWinDisplay(WINDOW *entity_list_win);
void DirectoryWinDisplay(WINDOW * directory_dispaly_win, const char * absDIRstr);
void getCurrDir(char **absDIRstr, long * maxpath);
void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color);
int print_current_entity(const char *entity_name, const int pce_mode, WINDOW *entity_detail_display_win);
//디렉토리 생성 윈도
void Dlg_Mkdir(WINDOW *dlg_win_mkdir, const char *curDirName, char *usrIptName);
//이름 변경 윈도 
void Dlg_Rename(WINDOW *dlg_win_rename, const char *prevName, char *newName);
//단축키 표시 윈도
void ShortCutWinDisplay(WINDOW *shortcut_display_win);

int print_current_entity(const char *entity_name, const int pce_mode, WINDOW *entity_detail_display_win);
void execFunc(const char *filename);//Exec()함수들을 통해 실행파일 실행하기 위한 함수
void initMenu(int *n_choices, struct dirent ***d_ptrArray, ITEM ***my_items,
	char ***rtn_entity_size_chr, WINDOW **entity_list_win,
	MENU **my_menu, const char *absDIRstr);
void freeDynamicMem(MENU **my_menu, ITEM ***my_items, struct dirent ***d_ptrArray,
	char ***rtn_entity_size_chr, char **absDIRstr, const int *n_choices);

int main() {
	//한글을 입력받기 위한 설정.
	setlocale(LC_ALL, "ko_KR.utf8");
	setenv("NCURSES_NO_UTF8_ACS", "1", 0);

	PANEL *my_panels[4];
	PANEL *top;

	WINDOW *entity_list_win, *shortcut_display_win, *entity_detail_display_win,
		*directory_display_win, *dlg_win_mkdir, *dlg_win_rename, *dlg_win_chmod;//윈도우생성변수

																				//init argument
	int n_choices;
	struct dirent **d_ptrArray = (struct dirent **)NULL;
	ITEM **my_items = (ITEM **)NULL;
	char **rtn_entity_size_chr = (char**)NULL;
	MENU *my_menu = (MENU *)NULL;
	//absDIRstr,entity_list_win도 추가되어야한다 	


	char * absDIRstr;
	long maxpath;
	char *bakAbsDIRstr;

	entity_list_win = (WINDOW *)NULL;					//
	shortcut_display_win = (WINDOW *)NULL;			//하단부 명령줄 생성
	directory_display_win = (WINDOW *)NULL;			//
	entity_detail_display_win = (WINDOW *)NULL;	//
	my_items = (ITEM **)NULL;
	my_menu = (MENU *)NULL;
	absDIRstr = (char *)NULL;
	bakAbsDIRstr = (char *)NULL;
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
	entity_list_win = newwin(LINES - 7, COLS, 3, 0);//현재 디랙토리 내용을 보여주는 윈도우.
	keypad(entity_list_win, TRUE);
	entity_detail_display_win = newwin(1, COLS, LINES - 4, 0);
	shortcut_display_win = newwin(3, COLS, LINES - 3, 0);
	dlg_win_mkdir = newwin(8, COLS - 20, LINES / 2 - 8, 10);
	dlg_win_rename = newwin(8, COLS - 20, LINES / 2 - 8, 10);
	dlg_win_chmod = newwin(10, COLS - 20, LINES / 2 - 8, 10);

	//-------------패널 설정 부-------
	my_panels[0] = new_panel(entity_list_win);
	my_panels[1] = new_panel(dlg_win_mkdir);
	my_panels[2] = new_panel(dlg_win_rename);
	my_panels[3] = new_panel(dlg_win_chmod);

	set_panel_userptr(my_panels[0], my_panels[1]);
	set_panel_userptr(my_panels[1], my_panels[0]);
	set_panel_userptr(my_panels[0], my_panels[2]);
	set_panel_userptr(my_panels[2], my_panels[0]);
	set_panel_userptr(my_panels[0], my_panels[3]);
	set_panel_userptr(my_panels[3], my_panels[0]);

	top = my_panels[0];

	//--------------------------------


	//화면에 출력해주는 부분.
	DirectoryWinDisplay(directory_display_win, absDIRstr);
	EntityListWinDisplay(entity_list_win);
	ShortCutWinDisplay(shortcut_display_win);


	initMenu(&n_choices, &d_ptrArray, &my_items,
		&rtn_entity_size_chr, &entity_list_win,
		&my_menu, absDIRstr);

	wrefresh(entity_list_win);

	mark_val = 0;

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
		case KEY_DOWN:
			menu_driver(my_menu, REQ_DOWN_ITEM);
			break;

		case KEY_UP:
			menu_driver(my_menu, REQ_UP_ITEM);
			break;

		case KEY_LEFT:
			menu_driver(my_menu, REQ_LEFT_ITEM);
			break;

		case KEY_RIGHT:
			menu_driver(my_menu, REQ_RIGHT_ITEM);
			break;

		case 10:    // Enter 실행속성이 있는 파일에 한해서
					// exec()를 이용해 실행
					//
		{   // Enter 내부 블럭 시작
			ITEM *curItem;
			curItem = current_item(my_menu);
			int pce_rtn_val;
			char *entity_name = (char *)item_name(curItem);

			void(*p)(char *);   // 사용자 포인터와 연결해둔 함수를
								// 사용하기 위한 변수.

								// 디렉토리인지 실행파일인지 판별
			pce_rtn_val = print_current_entity(item_name(curItem), 1, NULL);
			switch (pce_rtn_val)
			{
			case 1:
				// 선택된 아이템이 디렉토리일 경우.
				// 디렉토리를 바꿔야함.
				chdir(entity_name);

				// 디렉토리가 바뀌는 이벤트 에서 먼저 메모리 해제.
				freeDynamicMem(&my_menu, &my_items, &d_ptrArray
					, &rtn_entity_size_chr
					, &absDIRstr
					, &n_choices);
				// 메모리 해제 끝..

				getCurrDir(&absDIRstr, &maxpath);
				DirectoryWinDisplay(directory_display_win, absDIRstr);

				// 현재 디렉터리 얻어오기.
				// absDIRstr에 동적 메모리가 할당되므로 나중에 해제 꼭하기~~

				// Initialize items
				initMenu(&n_choices, &d_ptrArray, &my_items
					, &rtn_entity_size_chr
					, &entity_list_win
					, &my_menu
					, absDIRstr);
				break;
			}
		}
		break;


		case 0x0b: 	//mkdir 명령
			Dlg_Mkdir(dlg_win_mkdir, absDIRstr, &usrIptName[0]);
			if (usrIptName[0] != '\0') //취소 버튼 눌렀을 시
			{
				// 디렉토리가 바뀌는 이벤트와 마찬가지로 메뉴 재생성.
				freeDynamicMem(&my_menu, &my_items, &d_ptrArray
					, &rtn_entity_size_chr
					, (char **)NULL //&absDIRstr
					, &n_choices);
				// 메모리 해제 끝..

				// 현재 디렉터리 얻어오기.

				// Initialize items
				initMenu(&n_choices, &d_ptrArray, &my_items, &rtn_entity_size_chr, &entity_list_win, &my_menu, absDIRstr);
				// 메뉴 재생성 끝
			}
			top = (PANEL *)panel_userptr(top);
			top_panel(top);

			update_panels();
			mvwaddch(shortcut_display_win, 1, COLS - 2, 'K' | A_BOLD | A_BLINK | COLOR_PAIR(4));
			break;

		case 0x0e:  // Rename 명령
		{
			ITEM *curItem;
			curItem = current_item(my_menu);
			char *entity_name = (char *)item_name(curItem);

			Dlg_Rename(dlg_win_rename, entity_name, &usrIptName[0]);

			if (usrIptName[0] != '\0')
			{
				// 디렉토리가 바뀌는 이벤트와 마
				// 찬가지로 메뉴 재생성.
				freeDynamicMem(&my_menu, &my_items, &d_ptrArray
					, &rtn_entity_size_chr
					, (char **)NULL //&absDIRstr
					, &n_choices);
				// 메모리 해제 끝..


				// Initialize items
				initMenu(&n_choices, &d_ptrArray, &my_items
					, &rtn_entity_size_chr
					, &entity_list_win
					, &my_menu, absDIRstr);
				// 메뉴 재생성 끝
			}
		}

		top = (PANEL *)panel_userptr(top);
		top_panel(top);
		update_panels();

		mvwaddch(shortcut_display_win, 1, COLS - 2, 'N'
			| A_BOLD | A_BLINK | COLOR_PAIR(4));
		break;
		{   // 내부 블럭 시작:/
			ITEM *curItem;
			curItem = current_item(my_menu); // 현재 막대가 가리키고 있는 아이템 불러옴.

			wmove(entity_detail_display_win, 0, 2);
			wclrtoeol(entity_detail_display_win);

			wprintw(entity_detail_display_win, "[%s] ", item_name(curItem));

			//print_current_entity(item_name(curItem), 0, entity_detail_display_win);

			wrefresh(entity_detail_display_win);

			wrefresh(shortcut_display_win);

			pos_menu_cursor(my_menu); // 개체 정보 출력으로 옮겨진 커서를 다시 메뉴로...

		}   // 내부 블럭 끝.   
			//-------------------------------------------------------------------------
			//wrefresh(entity_list_win);

		top = my_panels[0];  // entity_list_win가 맨 위로 표시되게                             
		top_panel(top);

		update_panels();
		doupdate();
		}

		wrefresh(entity_list_win);
		wrefresh(entity_detail_display_win);
		wrefresh(shortcut_display_win);

	}

	//메모리 헤제
	freeDynamicMem(&my_menu, &my_items, &d_ptrArray,
		&rtn_entity_size_chr, &absDIRstr, &n_choices);

	free(bakAbsDIRstr);
	bakAbsDIRstr = (char *)NULL;


	//메모리 해제
	delwin(directory_display_win);
	delwin(entity_list_win);
	delwin(entity_detail_display_win);
	delwin(shortcut_display_win);
	delwin(dlg_win_mkdir);
	delwin(dlg_win_rename);
	delwin(dlg_win_chmod);

	del_panel(my_panels[0]);
	del_panel(my_panels[1]);
	del_panel(my_panels[2]);
	del_panel(my_panels[3]);

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
	for (i = 1; i < 9; i++) {

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

int print_current_entity(const char *entity_name, const int pce_mode, WINDOW *entity_detail_display_win)
{
	int i;
	struct stat statbuf;
	char file_attribute;
	const char file_perm_templet[10] = "rwxrwxrwx";
	char view_perm[10];
	const short octperm[] = { S_IRUSR, S_IWUSR, S_IXUSR,
		S_IRGRP, S_IWGRP, S_IXGRP,
		S_IROTH, S_IWOTH, S_IXOTH };
	struct tm *tp_time;
	time_t timeval_time;

	struct group *grpPtr;
	struct passwd *usrPtr;

	if (lstat(entity_name, &statbuf) == -1)
	{
		if (errno == 75)
		{
			if (pce_mode == 0)
				printw("2G이상의 파일은 읽어올때 문제가 있습니다.");
			return 0;
		}

		endwin();
		fprintf(stderr, "오류번호 %d : %s", errno, entity_name);
		perror(" ");
		exit(-9);
	}

	if (S_ISREG(statbuf.st_mode) == 1) {
		file_attribute = '-';
	}
	else if (S_ISDIR(statbuf.st_mode)) {
		file_attribute = 'd';
	}
	else if (S_ISLNK(statbuf.st_mode)) {
		file_attribute = 'l';
	}
	else {
		file_attribute = '?';
	}

	for (i = 0; i < 9; i++)
	{
		if (statbuf.st_mode & octperm[i]) {
			view_perm[i] = file_perm_templet[i];
		}
		else {
			view_perm[i] = '-';
		}
	}

	view_perm[9] = '\0';

	if (pce_mode == 1)
	{
		if (file_attribute == 'd') {
			return 1;
		}
		else if (access(entity_name, X_OK) == 0) {
			return 2;
		}
		else {
			return 0;
		}
	}

	if ((usrPtr = getpwuid(statbuf.st_uid)) == NULL)
	{
		endwin();
		perror("uid error");
		exit(-10);
	}
	if ((grpPtr = getgrgid(statbuf.st_gid)) == NULL)
	{
		endwin();
		perror("gid error");
		exit(-11);
	}
	if (entity_detail_display_win == NULL)
	{
		return 0;
	}
	wprintw(entity_detail_display_win, "[%d Byte][%s.%s][%c%s]",
		statbuf.st_size, usrPtr->pw_name, grpPtr->gr_name,
		file_attribute, view_perm);

	return 0;
}

void entity_size_calc(const char *entity_name, char *rtn_entity_size_chr)
{
	const size_t Byte = 1 << 10;
	const size_t MegaB = 1 << 20;
	const size_t GigaB = 1 << 30;

	struct stat statbuf;

	size_t entity_size;
	char char_rlt;
	unsigned int ptsn_size[2];

	entity_size = statbuf.st_size;
	if (entity_size >= GigaB)
	{
		ptsn_size[0] = entity_size / GigaB;
		ptsn_size[1] = entity_size % GigaB;

		if (ptsn_size[1] > 0 && ptsn_size[1] < (GigaB / 10))
		{
			ptsn_size[1] = 1;
		}
		else {
			ptsn_size[1] = ptsn_size[1] / (GigaB / 10);
		}
		char_rlt = 'G';
		entity_size = ptsn_size[0];
	}
	else if (entity_size >= MegaB) {
		ptsn_size[0] = entity_size / MegaB;
		ptsn_size[1] = entity_size % MegaB;

		if (ptsn_size[1] > 0 && ptsn_size[1] < (MegaB / 10))
		{
			ptsn_size[1] = 1;
		}
		else {
			ptsn_size[1] = ptsn_size[1] / (MegaB / 10);
		}
		char_rlt = 'M';
		entity_size = ptsn_size[0];
	}
	else if (entity_size >= Byte) {
		ptsn_size[0] = entity_size / Byte;
		ptsn_size[1] = entity_size % Byte;

		if (ptsn_size[1] > 0 && ptsn_size[1] < (Byte / 10))
		{
			ptsn_size[1] = 1;
		}
		else {
			ptsn_size[1] = ptsn_size[1] / (Byte / 10);
		}
		char_rlt = 'K';
		entity_size = ptsn_size[0];
	}
	else {
		ptsn_size[0] = entity_size;
		ptsn_size[1] = 0;
		char_rlt = 'B';
	}
	if (ptsn_size[1] == 10)
	{
		--ptsn_size[1];
	}
	sprintf(rtn_entity_size_chr, "%d.%d.%c", ptsn_size[0], ptsn_size[1], char_rlt);
}

//실행파일 실행하기 위한 함수
void execFunc(const char *filename) {
	int status;
	pid_t rtn_fork;

	struct sigaction act_exec, oldact;

	char pause_msg[] = "프로그램 종료됨. enter키를 눌러주세요.";

	char *const cmdlist[2] = { (char *)filename, (char *)0 };//execvp에 맞추기위해

	def_prog_mode();//현재 터미널의 curses모드 상태를 저장

	endwin();//임시적으로 curses mode종료.

	act_exec.sa_handler = SIG_IGN;
	if (sigaction(SIGINT, &act_exec, &oldact) == -1)
	{
		perror("위치:EXECfUNC(), sigaction()오류");
		exit(-16);
	}

	rtn_fork = fork();
	switch (rtn_fork)
	{
	case -1://fork error
		perror("execFufnc() - fork() error");
		exit(-17);
		break;
	case 0://child process
		act_exec.sa_handler = SIG_DFL;
		//자식 프로세스가 sigint가 기본값으로
		if (sigaction(SIGINT, &act_exec, NULL) == -1)
		{
			perror("execFunc() - sigaction() error");
			exit(-18);
		}

		execvp(cmdlist[0], &cmdlist[0]);
		perror("execFunc() - execvp() error");
		exit(-19);
	}

	wait(&status);

	write(STDOUT_FILENO, filename, strlen(filename) + 1);
	write(STDOUT_FILENO, pause_msg, sizeof(pause_msg));

	getchar();

	act_exec = oldact;

	if (sigaction(SIGINT, &act_exec, NULL) == -1)
	{
		perror("execFunc() - sigaction() error");
		exit(-20);
	}

	reset_prog_mode();
}

//init함수
void initMenu(int *n_choices, struct dirent ***d_ptrArray, ITEM ***my_items,
	char ***rtn_entity_size_chr, WINDOW **entity_list_win,
	MENU **my_menu, const char *absDIRstr)
{

	int(*filter_func)(const struct dirent *);

	int i, par_x, par_y, max_x, max_y;
	int pce_rtn_val = 0;//pce_rtn값을 임시 저장할 변수


	filter_func = 0;

	if ((*n_choices = scandir(".", d_ptrArray, filter_func, alphasort)) < 0)
	{
		endwin();
		perror("scandir() 오류");
		exit(-1);
	}

	if ((*my_items = (ITEM **)calloc(*n_choices + 1, sizeof(ITEM *))) == NULL)
	{
		endwin();
		perror("calloc()오류 - 위치 init");
		exit(-2);
	}

	if ((*rtn_entity_size_chr = (char **)calloc(*n_choices, sizeof(char *))) == NULL)
	{
		endwin();
		perror("calloc()오류 - initMenu");
		exit(-3);
	}

	for (i = 0; i < *n_choices; i++)
	{
		if (((*rtn_entity_size_chr)[i] = (char *)malloc(sizeof(char)*(16))) == NULL)
		{
			endwin();
			perror("malloc()오류 - initMenu - for");
			exit(-4);
		}
	}

	for (i = 0; i < *n_choices; i++)
	{
		pce_rtn_val = print_current_entity((((*d_ptrArray)[i])->d_name), 1, NULL);

		switch (pce_rtn_val)
		{
		case 0:
			//byte 사이즈를 축약해주는 함수
			entity_size_calc(((*d_ptrArray)[i])->d_name, (*rtn_entity_size_chr)[i]);
			//[이름][크기]로 삽입
			(*my_items)[i] = new_item(((*d_ptrArray)[i])->d_name, (*rtn_entity_size_chr)[i]);
			break;

		case 1:
			(*my_items)[i] = new_item(((*d_ptrArray)[i])->d_name, "[디렉토리]");
			break;

		case 2:
			entity_size_calc(((*d_ptrArray)[i])->d_name, (*rtn_entity_size_chr)[i]);
			(*my_items)[i] = new_item(((*d_ptrArray)[i])->d_name, (*rtn_entity_size_chr)[i]);


			set_item_userptr((*my_items)[i], execFunc);
			break;

		}
	}

	getmaxyx(*entity_list_win, max_y, max_x);
	getparyx(*entity_list_win, par_y, par_x);

	(*my_items)[*n_choices] = (ITEM *)NULL;
	*my_menu = new_menu((ITEM **)(*my_items));

	set_menu_win((*my_menu), *entity_list_win);
	set_menu_sub((*my_menu), derwin(*entity_list_win, max_y - 4, max_x - 4, par_y + 4, par_x + 3));

	set_menu_mark(*my_menu, "#");

	if (max_y - 5 < *n_choices)
	{
		set_menu_format(*my_menu, max_y - 4, 2);
	}
	else {
		set_menu_format(*my_menu, max_y - 4, 1);
	}

	//메뉴 리스트와 설명의 간격, 아이탬열간격 공백넣기
	set_menu_spacing(*my_menu, 3, 0, 5);

	menu_opts_off(*my_menu, O_ONEVALUE);

	menu_opts_off(*my_menu, O_ROWMAJOR);

	if (strcmp(absDIRstr, "/") != 0)
	{
		item_opts_off((*my_items)[0], O_SELECTABLE);
	}

	post_menu(*my_menu);
}

void freeDynamicMem(MENU **my_menu, ITEM ***my_items, struct dirent ***d_ptrArray,
	char ***rtn_entity_size_chr, char **absDIRstr, const int *n_choices)
{
	int i;

	unpost_menu(*my_menu);
	free_menu(*my_menu);
	*my_menu = (MENU*)NULL;

	for (i = 0; i < *n_choices; i++) {
		if ((free_item((*my_items)[i])) != E_OK)
		{
			endwin();
			perror("free_item() - error");
			exit(-1);
		}
		(*my_items)[i] = (ITEM*)NULL;
	}
	(*my_items) = (ITEM **)NULL;

	for (i = 0; i < *n_choices; i++)
	{
		free((*d_ptrArray)[i]);
		(*d_ptrArray)[i] = (struct dirent *)NULL;
	}
	free(*d_ptrArray);
	(*d_ptrArray) = (struct dirent **)NULL;

	for (i = 0; i < *n_choices; i++) {
		free((*rtn_entity_size_chr)[i]);
		(*rtn_entity_size_chr)[i] = (char *)NULL;
	}
	free(*rtn_entity_size_chr);
	*rtn_entity_size_chr = (char **)NULL;

	if (absDIRstr != (char **)NULL)
	{
		free(*absDIRstr);
		*absDIRstr = (char *)NULL;
	}
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

	getmaxyx(dlg_win_mkdir, max_y, max_x);  //max_y와 max_x에 윈도우의 크기를 배치 

	box(dlg_win_mkdir, 0, 0);	//사각형을 그림 

	wattron(dlg_win_mkdir, A_BOLD); //특정 윈도우에 적용할 속성을 설정 
	mvwprintw(dlg_win_mkdir, 0, 5, " 디렉토리 생성 (Mkdir)"); //특정 위치에 문자열 출력 
	wattroff(dlg_win_mkdir, A_BOLD); //적용했던 속성을 해제 

	wattron(dlg_win_mkdir, A_BOLD | COLOR_PAIR(8)); //색상과 글씨체 
	mvwprintw(dlg_win_mkdir, 2, 3, " 현재 디렉토리      ");

	mvwprintw(dlg_win_mkdir, 4, 3, " 만들 디렉터리 이름 ");

	mvwprintw(dlg_win_mkdir, 6, 24, "  확 인  "); // 버튼 1

	mvwprintw(dlg_win_mkdir, 6, 40, "  취 소  "); // 버튼 2
	wattroff(dlg_win_mkdir, A_BOLD | COLOR_PAIR(8));


	attr_style = A_BOLD | COLOR_PAIR(7);
	wmove(dlg_win_mkdir, 2, 23);     //사용자 정의 윈도우와 연관된 논리적 커서 이동 

	wattron(dlg_win_mkdir, attr_style);  //

	for (i = 23; i < max_x - 2; i++)  	//주어진 행과 열 위치에 ' ' 출력 
	{
		mvwaddch(dlg_win_mkdir, 2, i, ' ');
		mvwaddch(dlg_win_mkdir, 4, i, ' ');
	}
	mvwprintw(dlg_win_mkdir, 2, 24, curDirName);  //현재 디렉토리 출력

	echo(); // 사용자의 입력을 화면에 표시하기 위함.
	keypad(dlg_win_mkdir, FALSE);
	mvwgetnstr(dlg_win_mkdir, 4, 24, usrIptName, max_x - 28);//정해진 크기만큼 입력받음 
	keypad(dlg_win_mkdir, TRUE); //키를 입력받음 
	noecho(); //사용자의 입력을 화면에 표시하지 않음. 

	wattroff(dlg_win_mkdir, attr_style);

	mvwchgat(dlg_win_mkdir, 6, 24, 9, A_BOLD | A_UNDERLINE, 8, NULL); //문자들의 속성 설정 
	mvwchgat(dlg_win_mkdir, 6, 40, 9, A_NORMAL, 8, NULL); //문자들의 속성 설정 
	wmove(dlg_win_mkdir, 6, 24);

	while ((ipt = wgetch(dlg_win_mkdir)) != '\n')	//입력된 문자가 엔터가 아닐경우
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
			for (i = 23; i < max_x - 2; i++)
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
		wrefresh(dlg_win_mkdir); //버퍼의 내용을 화면으로 갱신 
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


void Dlg_Rename(WINDOW *dlg_win_rename, const char *prevName, char *newName)
{
	int y, x, max_y, max_x;
	int i;
	int attr_style;
	chtype ipt;

	newName[0] = '\0';
	keypad(dlg_win_rename, TRUE);

	getmaxyx(dlg_win_rename, max_y, max_x);

	box(dlg_win_rename, 0, 0);

	wattron(dlg_win_rename, A_BOLD);
	mvwprintw(dlg_win_rename, 0, 5, " 이름 변경 (Rename)");
	wattroff(dlg_win_rename, A_BOLD);

	attr_style = A_BOLD | COLOR_PAIR(8);
	wattron(dlg_win_rename, attr_style);
	mvwprintw(dlg_win_rename, 2, 3, " 이전 이름   ");

	mvwprintw(dlg_win_rename, 4, 3, " 새로운 이름 ");

	mvwprintw(dlg_win_rename, 6, 24, "  확 인  ");

	mvwprintw(dlg_win_rename, 6, 40, "  취 소  ");
	wattroff(dlg_win_rename, attr_style);


	attr_style = A_BOLD | COLOR_PAIR(7);
	wmove(dlg_win_rename, 2, 16);

	wattron(dlg_win_rename, attr_style);

	for (i = 16; i < max_x - 2; i++)
	{
		mvwaddch(dlg_win_rename, 2, i, ' ');
		mvwaddch(dlg_win_rename, 4, i, ' ');
	}
	mvwprintw(dlg_win_rename, 2, 17, prevName);

	echo();





	ipt = 0, i = 0;
	wmove(dlg_win_rename, 4, 17);
	getyx(dlg_win_rename, y, x);
	while ((ipt = wgetch(dlg_win_rename)) != '\n' && i < max_x - 28)
	{
		if (ipt == KEY_BACKSPACE)
		{
			if (i > 0)
			{
				wmove(dlg_win_rename, y, --x);
				waddch(dlg_win_rename, ' ');
				newName[i--] = '\0';
			}

			wmove(dlg_win_rename, y, x);
			continue;
		}

		if (ipt < 33 || ipt>126)
		{
			wmove(dlg_win_rename, y, x);
			continue;
		}


		newName[i++] = ipt;
		getyx(dlg_win_rename, y, x);
	}
	newName[i] = '\0';


	noecho();

	wattroff(dlg_win_rename, attr_style);


	mvwchgat(dlg_win_rename, 6, 24, 9, A_BOLD | A_UNDERLINE, 8, NULL);
	mvwchgat(dlg_win_rename, 6, 40, 9, A_NORMAL, 8, NULL);

	wmove(dlg_win_rename, 6, 24);

	ipt = 0;
	while ((ipt = wgetch(dlg_win_rename)) != '\n')
	{
		switch (ipt)
		{
		case KEY_LEFT:
			wmove(dlg_win_rename, 6, 24);
			wchgat(dlg_win_rename, 9, A_BOLD | A_UNDERLINE, 8, NULL);
			mvwchgat(dlg_win_rename, 6, 40, 9, A_NORMAL, 8, NULL);
			wmove(dlg_win_rename, 6, 24);
			break;

		case KEY_RIGHT:
			wmove(dlg_win_rename, 6, 40);
			wchgat(dlg_win_rename, 9, A_BOLD | A_UNDERLINE, 8, NULL);
			mvwchgat(dlg_win_rename, 6, 24, 9, A_NORMAL, 8, NULL);
			wmove(dlg_win_rename, 6, 40);

			break;
		case KEY_UP:
			mvwchgat(dlg_win_rename, 6, 24, 9, A_NORMAL, 8, NULL);
			mvwchgat(dlg_win_rename, 6, 40, 9, A_NORMAL, 8, NULL);

			wattron(dlg_win_rename, attr_style);
			for (i = 16; i < max_x - 2; i++)
			{
				mvwaddch(dlg_win_rename, 4, i, ' ');
			}
			echo();

			keypad(dlg_win_rename, FALSE);
			mvwgetnstr(dlg_win_rename, 4, 17, newName, max_x - 28);
			keypad(dlg_win_rename, FALSE);

			noecho();
			wattroff(dlg_win_rename, attr_style);
			wmove(dlg_win_rename, 6, 24);
			wchgat(dlg_win_rename, 9, A_BOLD | A_UNDERLINE, 8, NULL);


		}

		wrefresh(dlg_win_rename);
	}

	getyx(dlg_win_rename, y, x);

	if ((x > 39 && x < 50) || newName[0] == '\0')
	{
		newName[0] = '\0';
	}
	else {

		rename(prevName, newName);
	}

}