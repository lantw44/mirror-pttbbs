/* $Id$ */
#include "bbs.h"
#include <termios.h>

/* ----------------------------------------------------- */
/* basic tty control                                     */
/* ----------------------------------------------------- */
void
init_tty(void)
{
    struct termios tty_state, tty_new;

    if (tcgetattr(1, &tty_state) < 0) {
	syslog(LOG_ERR, "tcgetattr(): %m");
	return;
    }
    memcpy(&tty_new, &tty_state, sizeof(tty_new));
    tty_new.c_lflag &= ~(ICANON | ECHO | ISIG);
    /*
     * tty_new.c_cc[VTIME] = 0; tty_new.c_cc[VMIN] = 1;
     */
#if 1
    cfmakeraw(&tty_new);
    tty_new.c_cflag &= ~(CSIZE|PARENB);
    tty_new.c_cflag |= CS8;
    tcsetattr(1, TCSANOW, &tty_new);
#else
    tcsetattr(1, TCSANOW, &tty_new);
    system("stty raw -echo");
#endif
}

/* ----------------------------------------------------- */
/* init tty control code                                 */
/* ----------------------------------------------------- */


#define TERMCOMSIZE (40)

static void
sig_term_resize(int sig GCC_UNUSED)
{
    struct winsize  newsize;
    Signal(SIGWINCH, SIG_IGN);	/* Don't bother me! */
    ioctl(0, TIOCGWINSZ, &newsize);
    term_resize(newsize.ws_col, newsize.ws_row);
}

void term_resize(int w, int h)
{
    int dorefresh = 0;
    Signal(SIGWINCH, SIG_IGN);	/* Don't bother me! */


    /* make sure reasonable size */
    h = MAX(24, MIN(100, h));
    w = MAX(80, MIN(200, w));

    if (w != t_columns || h != t_lines)
    {
	// invoke terminal system resize
	resizeterm(h, w);

	t_lines = h;
	t_columns = w;
	dorefresh = 1;
    }
    b_lines = t_lines - 1;
    p_lines = t_lines - 4;

    Signal(SIGWINCH, sig_term_resize);
    if (dorefresh)
    {
	redrawwin();
	refresh();
    }
}

int
term_init(void)
{
    Signal(SIGWINCH, sig_term_resize);
    return YEA;
}

void
bell(void)
{
    const char c = Ctrl('G');
    write(1, &c, 1);
}

