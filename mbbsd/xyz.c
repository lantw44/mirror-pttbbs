/* $Id$ */
#include "bbs.h"

#if 0
/* 各種統計及相關資訊列表 */
/* Ptt90年度大學聯招查榜系統  */
int
x_90()
{
    use_dict(I18N[2431], "etc/90");
    return 0;
}

/* Ptt89年度大學聯招查榜系統  */
int
x_89()
{
    use_dict(I18N[2432], "etc/89");
    return 0;
}
/* Ptt88年度大學聯招查榜系統  */
int
x_88()
{
    use_dict(I18N[2433], "etc/88");
    return 0;
}
/* Ptt87年度大學聯招查榜系統  */
int
x_87()
{
    use_dict(I18N[2434], "etc/87");
    return 0;
}

/* Ptt86年度大學聯招查榜系統  */
int
x_86()
{
    use_dict(I18N[2435], "etc/86");
    return 0;
}

#endif
int
x_boardman()
{
    more("etc/topboardman", YEA);
    return 0;
}

int
x_user100()
{
    more("etc/topusr100", YEA);
    return 0;
}

int
x_history()
{
    more("etc/history", YEA);
    return 0;
}

#ifdef HAVE_X_BOARDS
static int
x_boards()
{
    more("etc/topboard.tmp", YEA);
    return 0;
}
#endif

int
x_birth()
{
    more("etc/birth.today", YEA);
    return 0;
}

int
x_weather()
{
    more("etc/weather.tmp", YEA);
    return 0;
}

int
x_mrtmap()
{
    more("etc/MRT.map", YEA);
	return 0;
}

int
x_stock()
{
    more("etc/stock.tmp", YEA);
    return 0;
}

int
x_note()
{
    more(fn_note_ans, YEA);
    return 0;
}

int
x_issue()
{
    more("etc/day", YEA);
    return 0;
}

int
x_week()
{
    more("etc/week", YEA);
    return 0;
}

int
x_today()
{
    more("etc/today", YEA);
    return 0;
}

int
x_yesterday()
{
    more("etc/yesterday", YEA);
    return 0;
}

int
x_login()
{
    more("etc/Welcome_login.0", YEA);
    return 0;
}

#ifdef HAVE_INFO
static int
x_program()
{
    more("etc/version", YEA);
    return 0;
}
#endif

#ifdef HAVE_LICENSE
static int
x_gpl()
{
    more("etc/GPL", YEA);
    return 0;
}
#endif

int
note()
{
    char    *fn_note_tmp = "note.tmp";
    char    *fn_note_dat = "note.dat";
    int             total = 0, i, collect, len;
    struct stat     st;
    char            buf[256], buf2[80];
    int             fd, fx;
    FILE           *fp, *foo;

    typedef struct notedata_t {
	time_t          date;
	char            userid[IDLEN + 1];
	char            username[19];
	char            buf[3][80];
    }               notedata_t;
    notedata_t      myitem;

    if (cuser.money < 5) {
	vmsg(I18N[2436]);
	return 0;
    }
    setutmpmode(EDNOTE);
    do {
	myitem.buf[0][0] = myitem.buf[1][0] = myitem.buf[2][0] = '\0';
	move(12, 0);
	clrtobot();
	outs(I18N[2437]);
	for (i = 0; (i < 3) && getdata(16 + i, 0, I18N[2438], myitem.buf[i],
				       sizeof(myitem.buf[i]) - 5, DOECHO)
	     && *myitem.buf[i]; i++);
	getdata(b_lines - 1, 0, I18N[2439],
		buf, 3, LCECHO);

	if (buf[0] == 'q' || (i == 0 && *buf != 'e'))
	    return 0;
    } while (buf[0] == 'e');
    demoney(-5);
    strcpy(myitem.userid, cuser.userid);
    strncpy(myitem.username, cuser.username, 18);
    myitem.username[18] = '\0';
    myitem.date = now;

    /* begin load file */
    if ((foo = fopen(".note", "a")) == NULL)
	return 0;

    if ((fp = fopen(fn_note_ans, "w")) == NULL) {
	fclose(fp);
	return 0;
    }

    if ((fx = open(fn_note_tmp, O_WRONLY | O_CREAT, 0644)) <= 0) {
	fclose(foo);
	fclose(fp);
	return 0;
    }

    if ((fd = open(fn_note_dat, O_RDONLY)) == -1)
	total = 1;
    else if (fstat(fd, &st) != -1) {
	total = st.st_size / sizeof(notedata_t) + 1;
	if (total > MAX_NOTE)
	    total = MAX_NOTE;
    }
    fputs(I18N[2440], fp);
    collect = 1;

    while (total) {
	snprintf(buf, sizeof(buf), I18N[2441],
		myitem.userid, myitem.username);
	len = strlen(buf);

	for (i = len; i < 71; i++)
	    strcat(buf, " ");
	snprintf(buf2, sizeof(buf2), I18N[2442],
		Cdate(&(myitem.date)));
	strcat(buf, buf2);
	fputs(buf, fp);
	if (collect)
	    fputs(buf, foo);
	for (i = 0; i < 3 && *myitem.buf[i]; i++) {
	    fprintf(fp, I18N[2443],
		    myitem.buf[i]);
	    if (collect)
		fprintf(foo, I18N[2444],
			myitem.buf[i]);
	}
	fputs(I18N[2445], fp);

	if (collect) {
	    fputs(I18N[2446], foo);
	    fclose(foo);
	    collect = 0;
	}
	write(fx, &myitem, sizeof(myitem));

	if (--total)
	    read(fd, (char *)&myitem, sizeof(myitem));
    }
    fputs(I18N[2447], fp);
    fclose(fp);
    close(fd);
    close(fx);
    Rename(fn_note_tmp, fn_note_dat);
    more(fn_note_ans, YEA);
    return 0;
}

static void
mail_sysop()
{
    FILE           *fp;
    char            genbuf[200];

    if ((fp = fopen("etc/sysop", "r"))) {
	int             i, j;
	char           *ptr;

	typedef struct sysoplist_t {
	    char            userid[IDLEN + 1];
	    char            duty[40];
	}               sysoplist_t;
	sysoplist_t     sysoplist[9];

	j = 0;
	while (fgets(genbuf, 128, fp)) {
	    if ((ptr = strchr(genbuf, '\n'))) {
		*ptr = '\0';
		if ((ptr = strchr(genbuf, ':'))) {
		    *ptr = '\0';
		    do {
			i = *++ptr;
		    } while (i == ' ' || i == '\t');
		    if (i) {
			strcpy(sysoplist[j].userid, genbuf);
			strcpy(sysoplist[j++].duty, ptr);
		    }
		}
	    }
	}
	fclose(fp);

	move(12, 0);
	clrtobot();
	outs(I18N[2448]);
	outs(I18N[2449]);
	outs(I18N[2450]);

	for (i = 0; i < j; i++)
	    prints("%15d.   \033[1;%dm%-16s%s\033[0m\n",
		 i + 1, 31 + i % 7, sysoplist[i].userid, sysoplist[i].duty);
	prints(I18N[2451], "", 31 + j % 7);
	getdata(b_lines - 1, 0, I18N[2452],
		genbuf, 4, DOECHO);
	i = genbuf[0] - '0' - 1;
	if (i >= 0 && i < j) {
	    clear();
	    do_send(sysoplist[i].userid, NULL);
	}
    }
}

int
m_sysop()
{
    setutmpmode(MSYSOP);
    mail_sysop();
    return 0;
}

void 
log_memoryusage(void)
{
  int use=((int)sbrk(0)-0x8048000)/1024;
  if(use<500)
    use=499;
  if(use>1000)
    use=1000;
  GLOBALVAR[use/100-4]++; // use [0]~[6]
}

int
Goodbye()
{
    char            genbuf[100];
    char			genbuf1[100];

	snprintf(genbuf1, sizeof(genbuf1), "%s%s%s", I18N[2453], BBSNAME, I18N[2454]);
    getdata(b_lines - 1, 0, genbuf1,
	    genbuf, 3, LCECHO);

    if (*genbuf != 'y')
	return 0;

    movie(999);
    if (cuser.userlevel) {
	getdata(b_lines - 1, 0,
		I18N[2455],
		genbuf, 3, LCECHO);
	if (genbuf[0] == 'm')
	    mail_sysop();
	else if (genbuf[0] == 'n')
	    note();
    }
    log_memoryusage();
    clear();
    prints(I18N[2456],
	   cuser.userid, cuser.username, BBSName);
    user_display(&cuser, 0);
    pressanykey();

    more("etc/Logout", NA);
    pressanykey();
    u_exit("EXIT ");
    return QUIT;
}
