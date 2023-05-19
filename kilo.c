/*** includes ***/

#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>

/*** defines **/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

/*** structure to store configs for the editor **/
struct editorConfig {
	int screenrows;
	int screencols;
	struct termios orig_termios;
};

struct editorConfig E;

/*** terminal **/

/*** displays error messages ***/

void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	perror(s);
	exit(1);
}

/*** disable terminal raw mode on editor exit ***/

void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
		die("tcsetattr");
}

/*** enable terminal raw mode ***/

void enableRawMode()
{
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
		die ("tcgetattr");

	atexit(disableRawMode);

	struct termios raw = E.orig_termios;
	raw.c_iflag &= ~(BRKINT | INPCK | ISTRIP | IXON | ICRNL);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
		die("tcsetattr");

}

/*** read a character input (low level input) **/

char editorReadKey() {
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN)
			die("read");
	}
	return c;
}

/*** get the current position of the cursor ***/

int getCursorPosition(int *rows, int *cols) {
	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

	printf("\r\n");
	char c;
	while (read(STDIN_FILENO, &c, 1) == 1) {
		if (iscntrl(c)) {
			printf("%d\r\n", c);
		}
		else {
			printf("%d ('%c')\r\n", c, c);
		}
	}

	editorReadKey();

	return -1;
}

/*** get the window size of the terminal ***/

int getWindowSize(int *rows, int *cols) {
	struct winsize ws;

	if (1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
			return -1;
		return getCursorPosition(rows, cols);
	}
	else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

/*** input **/

/*** processes the character inputs from editorReadKey() ***/
void editorProcessKeypress() {
	char c = editorReadKey();

	switch(c) {
		case CTRL_KEY('q'):
			exit(0);
			break;
	}
}

/*** output ***/

/*** draw a column of rows like Vim ***/

void editorDrawRows() {
	int y;
	for (y=0; y < E.screenrows; y++)
	{
		write(STDOUT_FILENO, "~\r\n", 3);
	}
}

/*** clears the screen and resets the cursor ***/
void editorRefreshScreen() {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	editorDrawRows();

	write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** init ***/

void initEditor() {
	if (getWindowSize(&E.screenrows, &E.screencols) == -1)
		die("getWindowSize");
}

int main()
{
	enableRawMode();
	initEditor();

	while (1) {
		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}
