/* $Id$ */
#include "bbs.h"

/* help & menu processring */
static int      refscreen = NA;
extern char    *boardprefix;
extern struct utmpfile_t *utmpshm;
extern char    *fn_board;
extern char     board_hidden_status;

void
showtitle(char *title, char *mid)
{
    char            buf[40], buf1[40], numreg[50];
#ifndef DEBUG
    int             nreg;
#endif
    int             spc = 0, pad, bid;
    static char     lastboard[16] = {0};

    spc = strlen(mid);
    if (title[0] == 0)
	title++;
#ifdef DEBUG
    else {
	snprintf(numreg, sizeof(numreg),
		 "\033[41;5m  current pid: %6d  " TITLE_COLOR,
		 getpid());
	mid = numreg;
	spc = 22;
    }
#else
    else if (currutmp->mailalert) {
    snprintf(buf1, sizeof(buf1), "%s"TITLE_COLOR, I18N[1380]);
	mid = buf1;
	spc = 22;
    } else if (HAS_PERM(PERM_SYSOP) && (nreg = dashs(fn_register) / 163) > 10) {
	snprintf(numreg, sizeof(numreg),
		 I18N[1381],
		 nreg, (int)dashs("register.new.tmp") / 163, TITLE_COLOR);
	mid = numreg;
	spc = 22;
    }
#endif
    spc = 66 - strlen(title) - spc - strlen(currboard);
    if (spc < 0)
	spc = 0;
    pad = 1 - (spc & 1);
    memset(buf, ' ', spc >>= 1);
    buf[spc] = '\0';

    clear();
    prints(I18N[1382], TITLE_COLOR,
	   title, buf, mid, buf, " " + pad,
	currmode & MODE_SELECT ? I18N[1383] : currmode & MODE_ETC ? I18N[1384] :
	   currmode & MODE_DIGEST ? I18N[1385] : I18N[1386]);

    if (strcmp(currboard, lastboard)) {	/* change board */
	if (currboard[0] != 0 &&
	    (bid = getbnum(currboard)) > 0) {
	    // XXX: bid starts from 1
	    board_hidden_status = ((bcache[bid - 1].brdattr & BRD_HIDE) &&
				   (bcache[bid - 1].brdattr & BRD_POSTMASK));
	    strncpy(lastboard, currboard, sizeof(lastboard));
	}
    }
    if (board_hidden_status)
	prints("\033[32m%s", currboard);
    else
	prints("%s", currboard);
    prints(I18N[1387], currmode & MODE_SELECT ? 6 :
	   currmode & MODE_ETC ? 5 : currmode & MODE_DIGEST ? 2 : 7);
}

/* 動畫處理 */
#define FILMROW 11
static unsigned char menu_row = 12;
static unsigned char menu_column = 20;

static void
show_status(void)
{
    int i;
    struct tm      *ptime = localtime(&now);
    char            mystatus[160];
    char           *myweek = I18N[1388];
    const char     *msgs[] = {I18N[1389], I18N[1390], I18N[1391], I18N[1392], I18N[1393]};

    i = ptime->tm_wday << 1;
    snprintf(mystatus, sizeof(mystatus),
	     I18N[1394],
	     ptime->tm_mon + 1, ptime->tm_mday, myweek[i], myweek[i + 1],
	     ptime->tm_hour, ptime->tm_min, currutmp->birth ?
	     I18N[1395] : SHM->today_is,
	     SHM->UTMPnumber, cuser.userid, msgs[currutmp->pager]);
    outmsg(mystatus);
}

static int
u_movie()
{
    cuser.uflag ^= MOVIE_FLAG;
    return 0;
}

void
movie(int i)
{
    static short    history[MAX_HISTORY];
    int             j;

    if ((currstat != CLASS) && (cuser.uflag & MOVIE_FLAG) &&
	!SHM->Pbusystate && SHM->max_film > 0) {
	if (currstat == PSALE) {
	    i = PSALE;
	    reload_money();
	} else {
	    do {
		if (!i)
		    i = 1 + (int)(((float)SHM->max_film *
				   rand()) / (RAND_MAX + 1.0));

		for (j = SHM->max_history; j >= 0; j--)
		    if (i == history[j]) {
			i = 0;
			break;
		    }
	    } while (i == 0);
	}

	memmove(history, &history[1], SHM->max_history * sizeof(short));
	history[SHM->max_history] = j = i;

	if (i == 999)		/* Goodbye my friend */
	    i = 0;

	move(1, 0);
	clrtoline(1 + FILMROW);	/* 清掉上次的 */
	out_lines(SHM->notes[i], 11);	/* 只印11行就好 */
	outs(reset_color);
    }
    show_status();
    refresh();
}

static int
show_menu(commands_t * p)
{
    register int    n = 0;
    register char  *s;
    char            buf[80];

    movie(currstat);

    move(menu_row, 0);
    while (p[n].desc > 0 && p[n].desc < MAX_STRING) {
	if (HAS_PERM(p[n].level)) {
		s = I18N[p[n].desc];
	    snprintf(buf, sizeof(buf), s + 2, I18N[1396 + cuser.proverb % 4]);
	    prints("%*s  (\033[1;36m%c\033[0m)%s\n", menu_column, "", s[1],
		   buf);
	}
	n++;
    }
    return n - 1;
}

void
domenu(int cmdmode, char *cmdtitle, int cmd, commands_t cmdtable[])
{
    int             lastcmdptr;
    int             n, pos, total, i;
    int             err;
    static char     cmd0[LOGIN];

    /* XXX: 傳進來的 cmd 若權限不足, 則不知 cursor 在哪, 導致 crash */
    if (cmd0[cmdmode])
	cmd = cmd0[cmdmode];

    setutmpmode(cmdmode);

    showtitle(cmdtitle, BBSName);

    total = show_menu(cmdtable);

    show_status();
    lastcmdptr = pos = 0;

    do {
	i = -1;
	switch (cmd) {
	case Ctrl('I'):
	    t_idle();
	    refscreen = YEA;
	    i = lastcmdptr;
	    break;
	case Ctrl('N'):
	    New();
	    refscreen = YEA;
	    i = lastcmdptr;
	    break;
	case Ctrl('A'):
	    if (mail_man() == FULLUPDATE)
		refscreen = YEA;
	    i = lastcmdptr;
	    break;
	case KEY_DOWN:
	    i = lastcmdptr;
	case KEY_HOME:
	case KEY_PGUP:
	    do {
		if (++i > total)
		    i = 0;
	    } while (!HAS_PERM(cmdtable[i].level));
	    break;
	case KEY_END:
	case KEY_PGDN:
	    i = total;
	    break;
	case KEY_UP:
	    i = lastcmdptr;
	    do {
		if (--i < 0)
		    i = total;
	    } while (!HAS_PERM(cmdtable[i].level));
	    break;
	case KEY_LEFT:
	case 'e':
	case 'E':
	    if (cmdmode == MMENU)
		cmd = 'G';
	    else if ((cmdmode == MAIL) && chkmailbox())
		cmd = 'R';
	    else
		return;
	default:
	    if ((cmd == 's' || cmd == 'r') &&
	    (currstat == MMENU || currstat == TMENU || currstat == XMENU)) {
		if (cmd == 's')
		    ReadSelect();
		else
		    Read();
		refscreen = YEA;
		i = lastcmdptr;
		break;
	    }
	    if (cmd == '\n' || cmd == '\r' || cmd == KEY_RIGHT) {
		move(b_lines, 0);
		clrtoeol();

		currstat = XMODE;

		if ((err = (*cmdtable[lastcmdptr].cmdfunc) ()) == QUIT)
		    return;
		currutmp->mode = currstat = cmdmode;

		if (err == XEASY) {
		    refresh();
		    safe_sleep(1);
		} else if (err != XEASY + 1 || err == FULLUPDATE)
		    refscreen = YEA;

		if (err != -1)
		    cmd = I18N[cmdtable[lastcmdptr].desc][0];
		else
		    cmd = I18N[cmdtable[lastcmdptr].desc][1];
		cmd0[cmdmode] = I18N[cmdtable[lastcmdptr].desc][0];
	    }
	    if (cmd >= 'a' && cmd <= 'z')
		cmd &= ~0x20;
	    while (++i <= total)
		if (I18N[cmdtable[i].desc][1] == cmd)
		    break;
	}

	if (i > total || !HAS_PERM(cmdtable[i].level))
	    continue;

	if (refscreen) {
	    showtitle(cmdtitle, BBSName);

	    show_menu(cmdtable);

	    show_status();
	    refscreen = NA;
	}
	cursor_clear(menu_row + pos, menu_column);
	n = pos = -1;
	while (++n <= (lastcmdptr = i))
	    if (HAS_PERM(cmdtable[n].level))
		pos++;

	cursor_show(menu_row + pos, menu_column);
    } while (((cmd = igetch()) != EOF) || refscreen);

    abort_bbs(0);
}
/* INDENT OFF */

/* administrator's maintain menu */
static commands_t adminlist[] = {
    {m_user, PERM_ACCOUNTS,           1399},
    {search_user_bypwd, PERM_SYSOP,   1400},
    {search_user_bybakpwd,PERM_SYSOP, 1401},
    {m_board, PERM_SYSOP,             1402},
    {m_register, PERM_SYSOP,          1403},
    {cat_register, PERM_SYSOP,        1404},
    {x_file, PERM_SYSOP|PERM_VIEWSYSOP,     1405},
    {give_money, PERM_SYSOP|PERM_VIEWSYSOP, 1406},
    {m_loginmsg, PERM_SYSOP,          1407},
#ifdef  HAVE_MAILCLEAN
    {m_mclean, PERM_SYSOP,            1408},
#endif
#ifdef  HAVE_REPORT
    {m_trace, PERM_SYSOP,             1409},
#endif
    {NULL, 0, -1}
};

/* mail menu */
static commands_t maillist[] = {
    {m_new, PERM_READMAIL,      1410},
    {m_read, PERM_READMAIL,     1411},
    {m_send, PERM_BASIC,        1412},
    {x_love, PERM_LOGINOK,      1413},
    {mail_list, PERM_BASIC,     1414},
    {setforward, PERM_LOGINOK,1415},
    {m_sysop, 0,                1416},
    {m_internet, PERM_INTERNET, 1417},
    {mail_mbox, PERM_INTERNET,  1418},
    {built_mail_index, PERM_LOGINOK, 1419},
    {mail_all, PERM_SYSOP,      1420},
    {NULL, 0, -1}
};

/* Talk menu */
static commands_t talklist[] = {
    {t_users, 0,            1421},
    {t_pager, PERM_BASIC,   1422},
    {t_idle, 0,             1423},
    {t_query, 0,            1424},
    {t_qchicken, 0,         1425},
    {t_talk, PERM_PAGE,     1426},
    {t_chat, PERM_CHAT,     1427},
    {t_display, 0,          1428},
    {NULL, 0, -1}
};

/* name menu */
static int t_aloha() {
    friend_edit(FRIEND_ALOHA);
    return 0;
}

static int t_special() {
    friend_edit(FRIEND_SPECIAL);
    return 0;
}

static commands_t namelist[] = {
    {t_override, PERM_LOGINOK,1429},
    {t_reject, PERM_LOGINOK,  1430},
    {t_aloha,PERM_LOGINOK,    1431},
#ifdef POSTNOTIFY
    {t_post,PERM_LOGINOK,     1432},
#endif
    {t_special,PERM_LOGINOK,  1433},
    {NULL, 0, -1}
};

/* User menu */
static commands_t userlist[] = {
    {u_info, PERM_LOGINOK,          1434},
    {calendar, PERM_LOGINOK,          1435},
    {u_editcalendar, PERM_LOGINOK,    1436},
    {u_loginview, PERM_LOGINOK,     1437},
    {u_ansi, 0, 1438},
    {u_movie, 0,                    1439},
#ifdef  HAVE_SUICIDE
    {u_kill, PERM_BASIC,            1440},
#endif
    {u_editplan, PERM_LOGINOK,      1441},
    {u_editsig, PERM_LOGINOK,       1442},
#if HAVE_FREECLOAK
    {u_cloak, PERM_LOGINOK,           1443},
#else
    {u_cloak, PERM_CLOAK,           1444},
#endif
    {u_register, PERM_BASIC,        1445},
    {u_list, PERM_SYSOP,            1446},
    {NULL, 0, -1}
};

/* XYZ tool menu */
static commands_t xyzlist[] = {
#ifdef  HAVE_LICENSE
    {x_gpl, 0,       1447},
#endif
#ifdef HAVE_INFO
    {x_program, 0,   1448},
#endif
    {x_boardman,0,   1449},
//  {x_boards,0,     1450},
    {x_history, 0,   1451},
    {x_note, 0,      1452},
    {x_login,0,      1453},
    {x_week, 0,      1454},
    {x_issue, 0,     1455},
    {x_today, 0,     1456},
    {x_yesterday, 0, 1457},
    {x_user100 ,0,   1458},
    {x_birth, 0,     1459},
    {p_sysinfo, 0,   1460},
    {NULL, 0, -1}
};

/* Ptt money menu */
static commands_t moneylist[] = {
    {p_give, 0,         1461},
    {save_violatelaw, 0,1462},
#if !HAVE_FREECLOAK
    {p_cloak, 0,        1463},
#endif
    {p_from, 0,         1464},
    {ordersong,0,       1465},
    {p_exmail, 0,       1466},
    {NULL, 0, -1}
};

static int p_money() {
    domenu(PSALE, I18N[1467], '0', moneylist);
    return 0;
};

#if 0
static commands_t jceelist[] = {
    {x_90,PERM_LOGINOK,	     1468},
    {x_89,PERM_LOGINOK,	     1469},
    {x_88,PERM_LOGINOK,      1470},
    {x_87,PERM_LOGINOK,      1471},
    {x_86,PERM_LOGINOK,      1472},
    {NULL, 0, -1}
};

static int m_jcee() {
    domenu(JCEE, I18N[1473], '0', jceelist);
    return 0;
}
#endif

static int forsearch();
static int playground();
static int chessroom();

/* Ptt Play menu */
static commands_t playlist[] = {
#if 0
#if HAVE_JCEE
    {m_jcee, PERM_LOGINOK,   1474},
#endif
#endif
    {note, PERM_LOGINOK,     1475},
    {x_weather,0 ,           1476},
/* XXX 壞掉了 */
/*    {x_stock,0 ,             "SStock       【 股市行情 】"},*/
    {forsearch,PERM_LOGINOK, 1477},
    {topsong,PERM_LOGINOK,   1478},
    {p_money,PERM_LOGINOK,   1479},
    {chicken_main,PERM_LOGINOK, 1480},
    {playground,PERM_LOGINOK, 1481},
    {chessroom, PERM_LOGINOK, 1482},
    {NULL, 0, -1}
};

static commands_t chesslist[] = {
    {chc_main, PERM_LOGINOK, 1483},
    {chc_personal, PERM_LOGINOK, 1484},
    {chc_watch, PERM_LOGINOK, 1485},
    {NULL, 0, -1}
};

static int chessroom() {
    domenu(CHC, I18N[1486], '1', chesslist);
    return 0;
}

static commands_t plist[] = {

/*    {p_ticket_main, PERM_LOGINOK,"00Pre         【 總統機 】"},
      {alive, PERM_LOGINOK,        "00Alive       【  訂票雞  】"},
*/
    {ticket_main, PERM_LOGINOK,  1487},
    {guess_main, PERM_LOGINOK,   1488},
    {othello_main, PERM_LOGINOK, 1489},
//    {dice_main, PERM_LOGINOK,    1490},
    {vice_main, PERM_LOGINOK,    1491},
    {g_card_jack, PERM_LOGINOK,  1492},
    {g_ten_helf, PERM_LOGINOK,   1493},
    {card_99, PERM_LOGINOK,      1494},
    {NULL, 0, -1}
};

static int playground() {
    domenu(AMUSE, I18N[1495],'1',plist);
    return 0;
}

static commands_t slist[] = {
    {x_dict,0,                   1496},
    {x_mrtmap, 0,                1497},
    {main_railway, PERM_LOGINOK,  1498},
    {NULL, 0, -1}
};

static int forsearch() {
    domenu(SREG, I18N[1499], '1', slist);
    return 0;
}

/* main menu */

int admin()
{
    domenu(ADMIN, I18N[1500], 'X', adminlist);
    return 0;
}

int Mail()
{
    domenu(MAIL, I18N[1501], 'R', maillist);
    return 0;
}

int Talk()
{
    domenu(TMENU, I18N[1502], 'U', talklist);
    return 0;
}

int User()
{
    domenu(UMENU, I18N[1503], 'A', userlist);
    return 0;
}

int Xyz()
{
    domenu(XMENU, I18N[1504], 'M', xyzlist);
    return 0;
}

int Play_Play()
{
    domenu(PMENU, I18N[1505], 'A', playlist);
    return 0;
}

int Name_Menu()
{
    domenu(NMENU, I18N[1506], 'O', namelist);
    return 0;
}
 
