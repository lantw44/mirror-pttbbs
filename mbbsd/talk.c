/* $Id$ */
#include "bbs.h"

#define QCAST   int (*)(const void *, const void *)

//#define sig_des [(a)] (SHM->i18nstr[cuser.language][1896 + (a)])
#define sig_des(a) (SHM->i18nstr[cuser.language][1896 + (a)])
/*static char    *sig_des[] = {
    SHM->i18nstr[cuser.language][1896], SHM->i18nstr[cuser.language][1897], "", SHM->i18nstr[cuser.language][1898], SHM->i18nstr[cuser.language][1899], SHM->i18nstr[cuser.language][1900]
};*/

#define MAX_SHOW_MODE 4
#define M_INT 15		/* monitor mode update interval */
#define P_INT 20		/* interval to check for page req. in
				 * talk/chat */
#define BOARDFRI  1

typedef struct talkwin_t {
    int             curcol, curln;
    int             sline, eline;
}               talkwin_t;

typedef struct pickup_t {
    userinfo_t     *ui;
    int             friend, uoffset;
}               pickup_t;

/* 記錄 friend 的 user number */
//
#define PICKUP_WAYS     7

static char    *fcolor[11] = {
    "", "\033[36m", "\033[32m", "\033[1;32m",
    "\033[33m", "\033[1;33m", "\033[1;37m", "\033[1;37m",
    "\033[31m", "\033[1;35m", "\033[1;36m"
};
static char     save_page_requestor[40];
static char     page_requestor[40];

userinfo_t *uip;

int
iswritable_stat(userinfo_t * uentp, int fri_stat)
{
    if (uentp == currutmp)
	return 0;

    if (HAS_PERM(PERM_SYSOP))
	return 1;

    if (!HAS_PERM(PERM_LOGINOK))
	return 0;

    return (uentp->pager != 3 && (fri_stat & HFM || uentp->pager != 4));
}

int
isvisible_stat(userinfo_t * me, userinfo_t * uentp, int fri_stat)
{
    if (uentp->userid[0] == 0)
	return 0;

    if (PERM_HIDE(uentp) && !(PERM_HIDE(me)))	/* 對方紫色隱形而你沒有 */
	return 0;
    else if ((me->userlevel & PERM_SYSOP) ||
	     ((fri_stat & HRM) && (fri_stat & HFM)))
	/* 站長看的見任何人 */
	return 1;

    if (uentp->invisible && !(me->userlevel & PERM_SEECLOAK))
	return 0;

    return !(fri_stat & HRM);
}

char           *
modestring(userinfo_t * uentp, int simple)
{
    static char     modestr[40];
    char    *notonline = SHM->i18nstr[cuser.language][1901];
    register int    mode = uentp->mode;
    register char  *word;
    int             fri_stat;

    /* for debugging */
    if (mode >= MAX_MODES) {
	syslog(LOG_WARNING, "what!? mode = %d", mode);
	word = SHM->i18nstr[cuser.language][ModeTypeTable[mode % MAX_MODES]];
    } else
	word = SHM->i18nstr[cuser.language][ModeTypeTable[mode]];
    fri_stat = friend_stat(currutmp, uentp);
    if (!(HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_SEECLOAK)) &&
	((uentp->invisible || (fri_stat & HRM)) &&
	 !((fri_stat & HFM) && (fri_stat & HRM))))
	return notonline;
    else if (mode == EDITING) {
	snprintf(modestr, sizeof(modestr), "E:%s",
		SHM->i18nstr[cuser.language][ModeTypeTable[uentp->destuid < EDITING ? uentp->destuid :
			      EDITING]]);
	word = modestr;
    } else if (!mode && *uentp->chatid == 1) {
	if (!simple)
	    snprintf(modestr, sizeof(modestr),
		     SHM->i18nstr[cuser.language][1902], getuserid(uentp->destuid));
	else
	    snprintf(modestr, sizeof(modestr), SHM->i18nstr[cuser.language][1903]);
    } 
    else if (!mode && *uentp->chatid == 3)
	snprintf(modestr, sizeof(modestr), SHM->i18nstr[cuser.language][1904]);
    else if (
#ifdef NOKILLWATERBALL
	     uentp->msgcount > 0
#else
	     (!mode) && *uentp->chatid == 2
#endif
	     )
	if (uentp->msgcount < 10) {
	    char           *cnum[10] =
	    {"", SHM->i18nstr[cuser.language][1905], SHM->i18nstr[cuser.language][1906], SHM->i18nstr[cuser.language][1907], SHM->i18nstr[cuser.language][1908], SHM->i18nstr[cuser.language][1909], SHM->i18nstr[cuser.language][1910], SHM->i18nstr[cuser.language][1911],
	    SHM->i18nstr[cuser.language][1912], SHM->i18nstr[cuser.language][1913]};
	    snprintf(modestr, sizeof(modestr),
		     SHM->i18nstr[cuser.language][1914], cnum[(int)(uentp->msgcount)]);
	} else
	    snprintf(modestr, sizeof(modestr), SHM->i18nstr[cuser.language][1915]);
    else if (!mode)
	return (uentp->destuid == 6) ? uentp->chatid :
	    SHM->i18nstr[cuser.language][1890 + ((0 <= uentp->destuid && uentp->destuid < 6) ?
			  uentp->destuid : 0)];
    else if (simple)
	return word;
    else if (uentp->in_chat && mode == CHATING)
	snprintf(modestr, sizeof(modestr), "%s (%s)", word, uentp->chatid);
    else if (mode == TALK) {
	if (!isvisible_uid(uentp->destuid))	/* Leeym 對方(紫色)隱形 */
	    snprintf(modestr, sizeof(modestr), "%s", SHM->i18nstr[cuser.language][1916]);
	/* Leeym * 大家自己發揮吧！ */
	else
	    snprintf(modestr, sizeof(modestr),
		     "%s %s", word, getuserid(uentp->destuid));
    } else if (mode == M_FIVE) {
	if (!isvisible_uid(uentp->destuid))
	    snprintf(modestr, sizeof(modestr), "%s", SHM->i18nstr[cuser.language][1917]);
	else
	    snprintf(modestr, sizeof(modestr), "%s %s", word, getuserid(uentp->destuid));
    } else if (mode == CHESSWATCHING) {
	snprintf(modestr, sizeof(modestr), SHM->i18nstr[cuser.language][1918]);
    } else if (mode == CHC) {
	if (isvisible_uid(uentp->destuid))
	    snprintf(modestr, sizeof(modestr), "%s", SHM->i18nstr[cuser.language][1919]);
	else
	    snprintf(modestr, sizeof(modestr),
		     SHM->i18nstr[cuser.language][1920], getuserid(uentp->destuid));
    } else if (mode != PAGE && mode != TQUERY)
	return word;
    else
	snprintf(modestr, sizeof(modestr),
		 "%s %s", word, getuserid(uentp->destuid));

    return (modestr);
}

int
set_friend_bit(userinfo_t * me, userinfo_t * ui)
{
    int             unum, *myfriends, hit = 0;

    /* 判斷對方是否為我的朋友 ? */
    if( intbsearch(ui->uid, me->friend, me->nFriends) )
	hit = IFH;

    /* 判斷我是否為對方的朋友 ? */
    if( intbsearch(me->uid, ui->friend, ui->nFriends) )
	hit |= HFM;

    /* 判斷對方是否為我的仇人 ? */
    myfriends = me->reject;
    while ((unum = *myfriends++)) {
	if (unum == ui->uid) {
	    hit |= IRH;
	    break;
	}
    }

    /* 判斷我是否為對方的仇人 ? */
    myfriends = ui->reject;
    while ((unum = *myfriends++)) {
	if (unum == me->uid) {
	    hit |= HRM;
	    break;
	}
    }
    return hit;
}

int
reverse_friend_stat(int stat)
{
    int             stat1 = 0;
    if (stat & IFH)
	stat1 |= HFM;
    if (stat & IRH)
	stat1 |= HRM;
    if (stat & HFM)
	stat1 |= IFH;
    if (stat & HRM)
	stat1 |= IRH;
    if (stat & IBH)
	stat1 |= IBH;
    return stat1;
}

void login_friend_online(void)
{
    userinfo_t     *uentp;
    int             i, stat, stat1;
    int             offset = (int)(currutmp - &SHM->uinfo[0]);
#ifdef OUTTACACHE
    int             sfd;
    if( (sfd = toconnect(OUTTACACHEHOST, OUTTACACHEPORT)) > 0 ){
	if( towrite(sfd, &offset, sizeof(offset)) > 0                    &&
	    towrite(sfd, &currutmp->uid, sizeof(currutmp->uid)) > 0      &&
	    towrite(sfd, currutmp->friend, sizeof(currutmp->friend)) > 0 &&
	    towrite(sfd, currutmp->reject, sizeof(currutmp->reject)) > 0 ){
	    ocfs_t  fs;
	    while( toread(sfd, &fs, sizeof(fs)) > 0 )
		if( SHM->uinfo[fs.index].uid == fs.uid ){
		    currutmp->friend_online[currutmp->friendtotal++]
			= fs.friendstat;
		    SHM->uinfo[fs.index].friend_online[ SHM->uinfo[fs.index].friendtotal++ ] = fs.rfriendstat;
		}
	    close(sfd);
	    return;
	}
	close(sfd);
    }
#endif
    for (i = 0; i < SHM->UTMPnumber && currutmp->friendtotal < MAX_FRIEND; i++) {
	uentp = (SHM->sorted[SHM->currsorted][0][i]);
	if (uentp && uentp->uid && (stat = set_friend_bit(currutmp, uentp))) {
	    stat1 = reverse_friend_stat(stat);
	    stat <<= 24;
	    stat |= (int)(uentp - &SHM->uinfo[0]);
	    currutmp->friend_online[currutmp->friendtotal++] = stat;
	    if (uentp != currutmp && uentp->friendtotal < MAX_FRIEND) {
		stat1 <<= 24;
		stat1 |= offset;
		uentp->friend_online[uentp->friendtotal++] = stat1;
	    }
	}
    }
    return;
}

int
logout_friend_online(userinfo_t * utmp)
{
    int             i, j, k;
    int             offset = (int)(utmp - &SHM->uinfo[0]);
    userinfo_t     *ui;
    while (utmp->friendtotal > 0) {
	i = utmp->friendtotal - 1;
	j = (utmp->friend_online[i] & 0xFFFFFF);
	utmp->friend_online[i] = 0;
	ui = &SHM->uinfo[j];
	if (ui->pid && ui != utmp) {
	    for (k = 0; k < ui->friendtotal && k < MAX_FRIEND &&
		 (int)(ui->friend_online[k] & 0xFFFFFF) != offset; k++);
	    if (k < ui->friendtotal) {
		ui->friendtotal--;
		ui->friend_online[k] = ui->friend_online[ui->friendtotal];
		ui->friend_online[ui->friendtotal] = 0;
	    }
	}
	utmp->friendtotal--;
	utmp->friend_online[utmp->friendtotal] = 0;
    }
    return 0;
}


int
friend_stat(userinfo_t * me, userinfo_t * ui)
{
    int             i, j, hit = 0;
    /* 看板好友 */
    if (me->brc_id && ui->brc_id == me->brc_id) {
	hit = IBH;
    }
    for (i = 0; me->friend_online[i] && i < MAX_FRIEND; i++) {
	j = (me->friend_online[i] & 0xFFFFFF);
	if (0 <= j && j < MAX_ACTIVE && ui == &SHM->uinfo[j]) {
	    hit |= me->friend_online[i] >> 24;
	    break;
	}
    }
    if (PERM_HIDE(ui))
	return hit & ST_FRIEND;
    return hit;
}

int
isvisible_uid(int tuid)
{
    userinfo_t     *uentp;

    if (!tuid || !(uentp = search_ulist(tuid)))
	return 1;
    return isvisible(currutmp, uentp);
}

/* 真實動作 */
static void
my_kick(userinfo_t * uentp)
{
    char            genbuf[200];

    getdata(1, 0, msg_sure_ny, genbuf, 4, LCECHO);
    clrtoeol();
    if (genbuf[0] == 'y') {
	snprintf(genbuf, sizeof(genbuf),
		 "%s (%s)", uentp->userid, uentp->username);
	log_usies("KICK ", genbuf);
	if ((uentp->pid <= 0 || kill(uentp->pid, SIGHUP) == -1) && (errno == ESRCH))
	    purge_utmp(uentp);
	outs(SHM->i18nstr[cuser.language][1921]);
    } else
	outs(msg_cancel);
    pressanykey();
}

static void
chicken_query(char *userid)
{
    if (getuser(userid)) {
	if (xuser.mychicken.name[0]) {
	    time_diff(&(xuser.mychicken));
	    if (!isdeadth(&(xuser.mychicken))) {
		show_chicken_data(&(xuser.mychicken), NULL);
		prints(SHM->i18nstr[cuser.language][1922], userid);
	    }
	} else {
	    move(1, 0);
	    clrtobot();
	    prints(SHM->i18nstr[cuser.language][1923], userid);
	}
	pressanykey();
    }
}

int
my_query(char *uident)
{
    userec_t        muser;
    int             tuid, i, fri_stat = 0;
    unsigned long int j;
    userinfo_t     *uentp;
    const char *money[10] =
    {SHM->i18nstr[cuser.language][1924], SHM->i18nstr[cuser.language][1925], SHM->i18nstr[cuser.language][1926], SHM->i18nstr[cuser.language][1927], SHM->i18nstr[cuser.language][1928],
    SHM->i18nstr[cuser.language][1929], SHM->i18nstr[cuser.language][1930], SHM->i18nstr[cuser.language][1931], SHM->i18nstr[cuser.language][1932], SHM->i18nstr[cuser.language][1933]};
    const char *sex[8] =
    {MSG_BIG_BOY, MSG_BIG_GIRL,
	MSG_LITTLE_BOY, MSG_LITTLE_GIRL,
    MSG_MAN, MSG_WOMAN, MSG_PLANT, MSG_MIME};

    if ((tuid = getuser(uident))) {
	memcpy(&muser, &xuser, sizeof(muser));
	move(1, 0);
	clrtobot();
	move(1, 0);
	setutmpmode(TQUERY);
	currutmp->destuid = tuid;

	if ((uentp = (userinfo_t *) search_ulist(tuid)))
	    fri_stat = friend_stat(currutmp, uentp);

	j = muser.money;
	for (i = 0; i < 10 && j > 10; i++)
	    j /= 10;
	prints(SHM->i18nstr[cuser.language][1934],
	       muser.userid,
	       muser.username,
	       26 - strlen(muser.userid) - strlen(muser.username), "",
	       money[i]);
	if (uentp && ((fri_stat & HFM && !uentp->invisible) || strcmp(muser.userid,cuser.userid) == 0))
	    prints(" ($%d)", muser.money);
	prints("\n");

	prints(SHM->i18nstr[cuser.language][1935], muser.numlogins);
	move(2, 40);
#ifdef ASSESS
	prints(SHM->i18nstr[cuser.language][1936], muser.numposts, muser.goodpost, muser.badpost);
#else
	prints(SHM->i18nstr[cuser.language][1937], muser.numposts);
#endif

	prints(SHM->i18nstr[cuser.language][1938],
	       (uentp && isvisible_stat(currutmp, uentp, fri_stat)) ?
	       modestring(uentp, 0) : SHM->i18nstr[cuser.language][1939]);

	outs(((uentp && uentp->mailalert) || load_mailalert(muser.userid))
	     ? SHM->i18nstr[cuser.language][1940] :
	     SHM->i18nstr[cuser.language][1941]);
	prints(SHM->i18nstr[cuser.language][1942],
	       Cdate(&muser.lastlogin),
	       (muser.lasthost[0] ? muser.lasthost : SHM->i18nstr[cuser.language][1943]));
	prints(SHM->i18nstr[cuser.language][1944],
	       muser.five_win, muser.five_lose, muser.five_tie,
	       muser.chc_win, muser.chc_lose, muser.chc_tie);
#ifdef ASSESS
	prints(SHM->i18nstr[cuser.language][1945], muser.goodsale, muser.badsale);
	move(6, 40);
#endif
	if ((uentp && ((fri_stat & HFM) || strcmp(muser.userid,cuser.userid) == 0) && !uentp->invisible))
	    prints(SHM->i18nstr[cuser.language][1946], sex[muser.sex % 8]);

	showplans(uident);
	pressanykey();
	return FULLUPDATE;
    }
    return DONOTHING;
}

static char     t_last_write[80];

void
water_scr(water_t * tw, int which, char type)
{
    if (type == 1) {
	int             i;
	int             colors[] = {33, 37, 33, 37, 33};
	move(8 + which, 28);
	prints(" ");
	move(8 + which, 28);
	prints("\033[1;37;45m  %c %-14s \033[0m",
	       tw->uin ? ' ' : 'x',
	       tw->userid);
	for (i = 0; i < 5; ++i) {
	    move(16 + i, 4);
	    prints(" ");
	    move(16 + i, 4);
	    if (tw->msg[(tw->top - i + 4) % 5].last_call_in[0] != 0)
		prints(SHM->i18nstr[cuser.language][1947],
		       colors[i],
		       tw->msg[(tw->top - i + 4) % 5].last_call_in);
	    else
		prints(SHM->i18nstr[cuser.language][1948]);
	}

	move(21, 4);
	prints(" ");
	move(21, 4);
	prints("\033[0m   \033[1;37;46m%-66s\033[0m   \n",
	       tw->msg[5].last_call_in);

	move(0, 0);
	prints(" ");
	move(0, 0);
	prints(SHM->i18nstr[cuser.language][1949], tw->userid);
	clrtoeol();
	move(0, strlen(tw->userid) + 6);
    } else {
	move(8 + which, 28);
	prints("123456789012345678901234567890");
	move(8 + which, 28);
	prints(SHM->i18nstr[cuser.language][1950],
	       tw->uin ? ' ' : 'x',
	       tw->userid);
    }
}

void
my_write2(void)
{
    int             i, ch, currstat0;
    char            genbuf[256], msg[80], done = 0, c0, which;
    water_t        *tw;
    unsigned char   mode0;

    if (swater[0] == NULL)
	return;
    wmofo = 0;
    currstat0 = currstat;
    c0 = currutmp->chatid[0];
    mode0 = currutmp->mode;
    currutmp->mode = 0;
    currutmp->chatid[0] = 3;
    currstat = DBACK;

    //init screen
	move(7, 28);
    prints(SHM->i18nstr[cuser.language][1951]);
    for (i = 0; i < 5; ++i)
	if (swater[i] == NULL || swater[i]->pid == 0)
	    break;
	else {
	    if (swater[i]->uin &&
		(swater[i]->pid != swater[i]->uin->pid ||
		 swater[i]->userid[0] != swater[i]->uin->userid[0]))
		swater[i]->uin = (userinfo_t *) search_ulist_pid(swater[i]->pid);
	    water_scr(swater[i], i, 0);
	}
    move(15, 4);
    prints(SHM->i18nstr[cuser.language][1952]);
    move(22, 4);
    prints(SHM->i18nstr[cuser.language][1953]);
    water_scr(swater[0], 0, 1);
    refresh();

    which = 0;
    do {
	switch ((ch = igetch())) {
	case Ctrl('T'):
	case KEY_UP:
	    if (water_usies != 1) {
		water_scr(swater[(int)which], which, 0);
		which = (which - 1 + water_usies) % water_usies;
		water_scr(swater[(int)which], which, 1);
		refresh();
	    }
	    break;

	case KEY_DOWN:
	case Ctrl('R'):
	    if (water_usies != 1) {
		water_scr(swater[(int)which], which, 0);
		which = (which + 1 + water_usies) % water_usies;
		water_scr(swater[(int)which], which, 1);
		refresh();
	    }
	    break;

	case KEY_LEFT:
	    done = 1;
	    break;

	default:
	    done = 1;
	    tw = swater[(int)which];

	    if (!tw->uin)
		break;

	    if (ch != '\r' && ch != '\n') {
		msg[0] = ch, msg[1] = 0;
	    } else
		msg[0] = 0;
	    move(0, 0);
	    prints("\033[m");
	    clrtoeol();
	    snprintf(genbuf, sizeof(genbuf), SHM->i18nstr[cuser.language][1954], tw->userid);
	    if (!oldgetdata(0, 0, genbuf, msg,
			    80 - strlen(tw->userid) - 6, DOECHO))
		break;

	    if (my_write(tw->pid, msg, tw->userid, 4, tw->uin))
		strncpy(tw->msg[5].last_call_in, t_last_write,
			sizeof(tw->msg[5].last_call_in));
	    break;
	}
    } while (!done);

    currstat = currstat0;
    currutmp->chatid[0] = c0;
    currutmp->mode = mode0;
    if (wmofo == RECVINREPLYING){
	wmofo = NOTREPLYING;
	write_request(0);
    }
    wmofo = NOTREPLYING;
}

/*
 * 被呼叫的時機: 1. 丟群組水球 flag = 1 (pre-edit) 2. 回水球     flag = 0 3.
 * 上站aloha  flag = 2 (pre-edit) 4. 廣播       flag = 3 if SYSOP, otherwise
 * flag = 1 (pre-edit) 5. 丟水球     flag = 0 6. my_write2  flag = 4
 * (pre-edit) but confirm
 */
int
my_write(pid_t pid, char *prompt, char *id, int flag, userinfo_t * puin)
{
    int             len, currstat0 = currstat, fri_stat;
    char            msg[80], destid[IDLEN + 1];
    char            genbuf[200], buf[200], c0 = currutmp->chatid[0];
    unsigned char   mode0 = currutmp->mode;
    struct tm      *ptime;
    userinfo_t     *uin;
    uin = (puin != NULL) ? puin : (userinfo_t *) search_ulist_pid(pid);
    strlcpy(destid, id, sizeof(destid));

    if (!uin && !(flag == 0 && water_which->count > 0)) {
	vmsg(SHM->i18nstr[cuser.language][1955]);
	watermode = -1;
	return 0;
    }
    currutmp->mode = 0;
    currutmp->chatid[0] = 3;
    currstat = DBACK;

    ptime = localtime(&now);

    if (flag == 0) {
	/* 一般水球 */
	watermode = 0;
	if (!(len = getdata(0, 0, prompt, msg, 56, DOECHO))) {
	    outmsg(SHM->i18nstr[cuser.language][1956]);
	    clrtoeol();
	    refresh();
	    currutmp->chatid[0] = c0;
	    currutmp->mode = mode0;
	    currstat = currstat0;
	    watermode = -1;
	    return 0;
	}
	if (watermode > 0) {
	    int             i;

	    i = (water_which->top - watermode + MAX_REVIEW) % MAX_REVIEW;
	    uin = (userinfo_t *) search_ulist_pid(water_which->msg[i].pid);
	    strlcpy(destid, water_which->msg[i].userid, sizeof(destid));
	}
    } else {
	/* pre-edit 的水球 */
	strlcpy(msg, prompt, sizeof(msg));
	len = strlen(msg);
    }

    strip_ansi(msg, msg, STRIP_ALL);
    if (uin && *uin->userid && (flag == 0 || flag == 4)) {
	snprintf(buf, sizeof(buf), SHM->i18nstr[cuser.language][1957], uin->userid, msg);
	getdata(0, 0, buf, genbuf, 3, LCECHO);
	if (genbuf[0] == 'n') {
	    currutmp->chatid[0] = c0;
	    currutmp->mode = mode0;
	    currstat = currstat0;
	    watermode = -1;
	    return 0;
	}
    }
    watermode = -1;
    if (!uin || !*uin->userid || strcasecmp(destid, uin->userid)) {
	vmsg(SHM->i18nstr[cuser.language][1959]);
	currutmp->chatid[0] = c0;
	currutmp->mode = mode0;
	currstat = currstat0;
	return 0;
    }
    fri_stat = friend_stat(currutmp, uin);
    if (flag != 2) {		/* aloha 的水球不用存下來 */
	/* 存到自己的水球檔 */
	if (!fp_writelog) {
	    sethomefile(genbuf, cuser.userid, fn_writelog);
	    fp_writelog = fopen(genbuf, "a");
	}
	if (fp_writelog) {
	    fprintf(fp_writelog, "To %s: %s [%s]\n",
		    uin->userid, msg, Cdatelite(&now));
	    snprintf(t_last_write, 66, "To %s: %s", uin->userid, msg);
	}
    }
    if (flag == 3 && uin->msgcount) {
	/* 不懂 */
	uin->destuip = currutmp - &SHM->uinfo[0];
	uin->sig = 2;
	if (uin->pid > 0)
	    kill(uin->pid, SIGUSR1);
    } else if (flag != 2 &&
	       !HAS_PERM(PERM_SYSOP) &&
	       (uin->pager == 3 ||
		uin->pager == 2 ||
		(uin->pager == 4 &&
		 !(fri_stat & HFM))))
	outmsg(SHM->i18nstr[cuser.language][1960]);
    else {
	int     write_pos = uin->msgcount; /* try to avoid race */
	if ( write_pos < (MAX_MSGS - 1) ) { /* race here */
	    unsigned char   pager0 = uin->pager;

	    uin->msgcount = write_pos + 1;
	    uin->pager = 2;
	    uin->msgs[write_pos].pid = currpid;
	    strlcpy(uin->msgs[write_pos].userid, cuser.userid,
		    sizeof(uin->msgs[write_pos].userid));
	    strlcpy(uin->msgs[write_pos].last_call_in, msg,
		    sizeof(uin->msgs[write_pos].last_call_in));
	    uin->pager = pager0;
	} else if (flag != 2)
	    outmsg(SHM->i18nstr[cuser.language][1961]);

	if (uin->msgcount >= 1 &&
#ifdef NOKILLWATERBALL
	    !(uin->wbtime = now) /* race */
#else
	    (uin->pid <= 0 || kill(uin->pid, SIGUSR2) == -1) 
#endif
	    && flag != 2)
	    outmsg(SHM->i18nstr[cuser.language][1962]);
	else if (uin->msgcount == 1 && flag != 2)
	    outmsg(SHM->i18nstr[cuser.language][1963]);
	else if (uin->msgcount > 1 && uin->msgcount < MAX_MSGS && flag != 2)
	    outmsg(SHM->i18nstr[cuser.language][1964]);
    }

    clrtoeol();

    currutmp->chatid[0] = c0;
    currutmp->mode = mode0;
    currstat = currstat0;
    return 1;
}

void
getmessage(msgque_t msg)
{
    int     write_pos = currutmp->msgcount; 
    if ( write_pos < (MAX_MSGS - 1) ) { 
            unsigned char pager0 = currutmp->pager;
 	    currutmp->msgcount = write_pos+1;
            memcpy(&currutmp->msgs[write_pos], &msg, sizeof(msgque_t));
            currutmp->pager = pager0;
   	    write_request(SIGUSR1);
        }
}

void
t_display_new(void)
{
    static int      t_display_new_flag = 0;
    int             i, off = 2;
    if (t_display_new_flag)
	return;
    else
	t_display_new_flag = 1;

    if (WATERMODE(WATER_ORIG))
	water_which = &water[0];
    else
	off = 3;

    if (water[0].count && watermode > 0) {
	move(1, 0);
	outs(SHM->i18nstr[cuser.language][1965]);
	outs(WATERMODE(WATER_ORIG) ?
	     SHM->i18nstr[cuser.language][1966] :
	     SHM->i18nstr[cuser.language][1967]);
	if (WATERMODE(WATER_NEW)) {
	    move(2, 0);
	    clrtoeol();
	    for (i = 0; i < 6; i++) {
		if (i > 0)
		    if (swater[i - 1]) {

			if (swater[i - 1]->uin &&
			    (swater[i - 1]->pid != swater[i - 1]->uin->pid ||
			     swater[i - 1]->userid[0] != swater[i - 1]->uin->userid[0]))
			    swater[i - 1]->uin = (userinfo_t *) search_ulist_pid(swater[i - 1]->pid);
			prints("%s%c%-13.13s\033[m",
			       swater[i - 1] != water_which ? "" :
			       swater[i - 1]->uin ? "\033[1;33;47m" :
			       "\033[1;33;45m",
			       !swater[i - 1]->uin ? '#' : ' ',
			       swater[i - 1]->userid);
		    } else
			prints("              ");
		else
		    prints(SHM->i18nstr[cuser.language][1968],
			   water_which == &water[0] ? "\033[1;33;47m " :
			   " "
			);
	    }
	}
	for (i = 0; i < water_which->count; i++) {
	    int             a = (water_which->top - i - 1 + MAX_REVIEW) % MAX_REVIEW, len = 75 - strlen(water_which->msg[a].last_call_in)
	    - strlen(water_which->msg[a].userid);
	    if (len < 0)
		len = 0;

	    move(i + (WATERMODE(WATER_ORIG) ? 2 : 3), 0);
	    clrtoeol();
	    if (watermode - 1 != i)
		prints("\033[1;33;46m %s \033[37;45m %s \033[m%*s",
		       water_which->msg[a].userid,
		       water_which->msg[a].last_call_in, len,
		       "");
	    else
		prints("\033[1;44m>\033[1;33;47m%s \033[37;45m %s \033[m%*s",
		       water_which->msg[a].userid,
		       water_which->msg[a].last_call_in,
		       len, "");
	}

	if (t_last_write[0]) {
	    move(i + off, 0);
	    clrtoeol();
	    prints(t_last_write);
	    i++;
	}
	move(i + off, 0);
	outs(SHM->i18nstr[cuser.language][1969]);
	if (WATERMODE(WATER_NEW))
	    while (i++ <= water[0].count) {
		move(i + off, 0);
		clrtoeol();
	    }
    }
    t_display_new_flag = 0;
}

int
t_display(void)
{
    char            genbuf[200], ans[4];
    if (fp_writelog) {
	fclose(fp_writelog);
	fp_writelog = NULL;
    }
    setuserfile(genbuf, fn_writelog);
    if (more(genbuf, YEA) != -1) {
	move(b_lines - 4, 0);
	outs(SHM->i18nstr[cuser.language][1970]);
	getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][1971],
		ans, sizeof(ans), LCECHO);
	if (*ans == 'm') {
	    fileheader_t    mymail;
	    char            title[128], buf[80];

	    sethomepath(buf, cuser.userid);
	    stampfile(buf, &mymail);

	    mymail.filemode = FILE_READ ;
	    strlcpy(mymail.owner, SHM->i18nstr[cuser.language][1972], sizeof(mymail.owner));
	    strlcpy(mymail.title, SHM->i18nstr[cuser.language][1973], sizeof(mymail.title));
	    sethomedir(title, cuser.userid);
	    Rename(genbuf, buf);
	    append_record(title, &mymail, sizeof(mymail));
	} else if (*ans == 'c')
	    unlink(genbuf);
	return FULLUPDATE;
    }
    return DONOTHING;
}

static void
do_talk_nextline(talkwin_t * twin)
{
    twin->curcol = 0;
    if (twin->curln < twin->eline)
	++(twin->curln);
    else
	region_scroll_up(twin->sline, twin->eline);
    move(twin->curln, twin->curcol);
}

static void
do_talk_char(talkwin_t * twin, int ch, FILE *flog)
{
    screenline_t   *line;
    int             i;
    char            ch0, buf[81];

    if (isprint2(ch)) {
	ch0 = big_picture[twin->curln].data[twin->curcol];
	if (big_picture[twin->curln].len < 79)
	    move(twin->curln, twin->curcol);
	else
	    do_talk_nextline(twin);
	outc(ch);
	++(twin->curcol);
	line = big_picture + twin->curln;
	if (twin->curcol < line->len) {	/* insert */
	    ++(line->len);
	    memcpy(buf, line->data + twin->curcol, 80);
	    save_cursor();
	    do_move(twin->curcol, twin->curln);
	    ochar(line->data[twin->curcol] = ch0);
	    for (i = twin->curcol + 1; i < line->len; i++)
		ochar(line->data[i] = buf[i - twin->curcol - 1]);
	    restore_cursor();
	}
	line->data[line->len] = 0;
	return;
    }
    switch (ch) {
    case Ctrl('H'):
    case '\177':
	if (twin->curcol == 0)
	    return;
	line = big_picture + twin->curln;
	--(twin->curcol);
	if (twin->curcol < line->len) {
	    --(line->len);
	    save_cursor();
	    do_move(twin->curcol, twin->curln);
	    for (i = twin->curcol; i < line->len; i++)
		ochar(line->data[i] = line->data[i + 1]);
	    line->data[i] = 0;
	    ochar(' ');
	    restore_cursor();
	}
	move(twin->curln, twin->curcol);
	return;
    case Ctrl('D'):
	line = big_picture + twin->curln;
	if (twin->curcol < line->len) {
	    --(line->len);
	    save_cursor();
	    do_move(twin->curcol, twin->curln);
	    for (i = twin->curcol; i < line->len; i++)
		ochar(line->data[i] = line->data[i + 1]);
	    line->data[i] = 0;
	    ochar(' ');
	    restore_cursor();
	}
	return;
    case Ctrl('G'):
	bell();
	return;
    case Ctrl('B'):
	if (twin->curcol > 0) {
	    --(twin->curcol);
	    move(twin->curln, twin->curcol);
	}
	return;
    case Ctrl('F'):
	if (twin->curcol < 79) {
	    ++(twin->curcol);
	    move(twin->curln, twin->curcol);
	}
	return;
    case KEY_TAB:
	twin->curcol += 8;
	if (twin->curcol > 80)
	    twin->curcol = 80;
	move(twin->curln, twin->curcol);
	return;
    case Ctrl('A'):
	twin->curcol = 0;
	move(twin->curln, twin->curcol);
	return;
    case Ctrl('K'):
	clrtoeol();
	return;
    case Ctrl('Y'):
	twin->curcol = 0;
	move(twin->curln, twin->curcol);
	clrtoeol();
	return;
    case Ctrl('E'):
	twin->curcol = big_picture[twin->curln].len;
	move(twin->curln, twin->curcol);
	return;
    case Ctrl('M'):
    case Ctrl('J'):
	line = big_picture + twin->curln;
	strncpy(buf, (char *)line->data, line->len);
	buf[line->len] = 0;
	do_talk_nextline(twin);
	break;
    case Ctrl('P'):
	line = big_picture + twin->curln;
	strncpy(buf, (char *)line->data, line->len);
	buf[line->len] = 0;
	if (twin->curln > twin->sline) {
	    --(twin->curln);
	    move(twin->curln, twin->curcol);
	}
	break;
    case Ctrl('N'):
	line = big_picture + twin->curln;
	strncpy(buf, (char *)line->data, line->len);
	buf[line->len] = 0;
	if (twin->curln < twin->eline) {
	    ++(twin->curln);
	    move(twin->curln, twin->curcol);
	}
	break;
    }
    trim(buf);
    if (*buf)
	fprintf(flog, "%s%s: %s%s\n",
		(twin->eline == b_lines - 1) ? "\033[1;35m" : "",
		(twin->eline == b_lines - 1) ?
		getuserid(currutmp->destuid) : cuser.userid, buf,
		(ch == Ctrl('P')) ? "\033[37;45m(Up)\033[m" : "\033[m");
}

static void
do_talk(int fd)
{
    struct talkwin_t mywin, itswin;
    char            mid_line[128], data[200];
    int             i, datac, ch;
    int             im_leaving = 0;
    FILE           *log, *flog;
    struct tm      *ptime;
    char            genbuf[200], fpath[100];

    ptime = localtime(&now);

    setuserfile(fpath, "talk_XXXXXX");
    flog = fdopen(mkstemp(fpath), "w");

    setuserfile(genbuf, fn_talklog);

    if ((log = fopen(genbuf, "w")))
	fprintf(log, "[%d/%d %d:%02d] & %s\n",
		ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour,
		ptime->tm_min, save_page_requestor);
    setutmpmode(TALK);

    ch = 58 - strlen(save_page_requestor);
    snprintf(genbuf, sizeof(genbuf), SHM->i18nstr[cuser.language][1974], cuser.userid, cuser.username);
    i = ch - strlen(genbuf);
    if (i >= 0)
	i = (i >> 1) + 1;
    else {
	genbuf[ch] = '\0';
	i = 1;
    }
    memset(data, ' ', i);
    data[i] = '\0';

    snprintf(mid_line, sizeof(mid_line),
	     SHM->i18nstr[cuser.language][1975], data, genbuf, save_page_requestor, data);

    memset(&mywin, 0, sizeof(mywin));
    memset(&itswin, 0, sizeof(itswin));

    i = b_lines >> 1;
    mywin.eline = i - 1;
    itswin.curln = itswin.sline = i + 1;
    itswin.eline = b_lines - 1;

    clear();
    move(i, 0);
    outs(mid_line);
    move(0, 0);

    add_io(fd, 0);

    while (1) {
	ch = igetch();
	if (ch == I_OTHERDATA) {
	    datac = recv(fd, data, sizeof(data), 0);
	    if (datac <= 0)
		break;
	    for (i = 0; i < datac; i++)
		do_talk_char(&itswin, data[i], flog);
	} else {
	    if (ch == Ctrl('C')) {
		if (im_leaving)
		    break;
		move(b_lines, 0);
		clrtoeol();
		outs(SHM->i18nstr[cuser.language][1976]);
		im_leaving = 1;
		continue;
	    }
	    if (im_leaving) {
		move(b_lines, 0);
		clrtoeol();
		im_leaving = 0;
	    }
	    switch (ch) {
	    case KEY_LEFT:	/* 把2byte的鍵改為一byte */
		ch = Ctrl('B');
		break;
	    case KEY_RIGHT:
		ch = Ctrl('F');
		break;
	    case KEY_UP:
		ch = Ctrl('P');
		break;
	    case KEY_DOWN:
		ch = Ctrl('N');
		break;
	    }
	    data[0] = (char)ch;
	    if (send(fd, data, 1, 0) != 1)
		break;
	    if (log)
		fprintf(log, "%c", (ch == Ctrl('M')) ? '\n' : (char)*data);
	    do_talk_char(&mywin, *data, flog);
	}
    }
    if (log)
	fclose(log);

    add_io(0, 0);
    close(fd);

    if (flog) {
	char            ans[4];
	int             i;

	fprintf(flog, SHM->i18nstr[cuser.language][1977],
		Cdatelite(&now));
	for (i = 0; i < scr_lns; i++)
	    fprintf(flog, "%.*s\n", big_picture[i].len, big_picture[i].data);
	fclose(flog);
	more(fpath, NA);
	getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][1978],
		ans, sizeof(ans), LCECHO);
	if (*ans == 'm') {
	    fileheader_t    mymail;
	    char            title[128];

	    sethomepath(genbuf, cuser.userid);
	    stampfile(genbuf, &mymail);
	    mymail.filemode = FILE_READ ;
	    strlcpy(mymail.owner, SHM->i18nstr[cuser.language][1979], sizeof(mymail.owner));
	    snprintf(mymail.title, sizeof(mymail.title),
		     SHM->i18nstr[cuser.language][1980],
		     getuserid(currutmp->destuid));
	    sethomedir(title, cuser.userid);
	    Rename(fpath, genbuf);
	    append_record(title, &mymail, sizeof(mymail));
	} else
	    unlink(fpath);
	flog = 0;
    }
    setutmpmode(XINFO);
}

#define lockreturn(unmode, state) if(lockutmpmode(unmode, state)) return

int make_connection_to_somebody(userinfo_t *uin, int timeout){
    int sock, length, pid, ch;
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
	perror("sock err");
	unlockutmpmode();
	return -1;
    }
    server.sin_family = PF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = 0;
    if (bind(sock, (struct sockaddr *) & server, sizeof(server)) < 0) {
	close(sock);
	perror("bind err");
	unlockutmpmode();
	return -1;
    }
    length = sizeof(server);
    if (getsockname(sock, (struct sockaddr *) & server, (socklen_t *) & length) < 0) {
	close(sock);
	perror("sock name err");
	unlockutmpmode();
	return -1;
    }
    currutmp->sockactive = YEA;
    currutmp->sockaddr = server.sin_port;
    currutmp->destuid = uin->uid;
    setutmpmode(PAGE);
    uin->destuip = currutmp - &SHM->uinfo[0];
    pid = uin->pid;
    if (pid > 0)
	kill(pid, SIGUSR1);
    clear();
    prints(SHM->i18nstr[cuser.language][1981], uin->userid);

    listen(sock, 1);
    add_io(sock, timeout);

    while (1) {
	ch = igetch();
	if (ch == I_TIMEOUT) {
	    ch = uin->mode;
	    if (!ch && uin->chatid[0] == 1 &&
		    uin->destuip == currutmp - &SHM->uinfo[0]) {
		bell();
		outmsg(SHM->i18nstr[cuser.language][1982]);
		refresh();
	    } else if (ch == EDITING || ch == TALK || ch == CHATING ||
		    ch == PAGE || ch == MAILALL || ch == MONITOR ||
		    ch == M_FIVE || ch == CHC ||
		    (!ch && (uin->chatid[0] == 1 ||
			     uin->chatid[0] == 3))) {
		add_io(0, 0);
		close(sock);
		currutmp->sockactive = currutmp->destuid = 0;
		vmsg(SHM->i18nstr[cuser.language][1983]);
		unlockutmpmode();
		return -1;
	    } else {
#ifdef linux
		add_io(sock, 20);	/* added for linux... achen */
#endif
		move(0, 0);
		outs(SHM->i18nstr[cuser.language][1984]);
		bell();

		uin->destuip = currutmp - &SHM->uinfo[0];
		if (pid <= 0 || kill(pid, SIGUSR1) == -1) {
#ifdef linux
		    add_io(sock, 20);	/* added 4 linux... achen */
#endif
		    vmsg(msg_usr_left);
		    unlockutmpmode();
		    return -1;
		}
		continue;
	    }
	}
	if (ch == I_OTHERDATA)
	    break;

	if (ch == '\004') {
	    add_io(0, 0);
	    close(sock);
	    currutmp->sockactive = currutmp->destuid = 0;
	    unlockutmpmode();
	    return -1;
	}
    }
    return sock;
}

void
my_talk(userinfo_t * uin, int fri_stat, char defact)
{
    int             sock, msgsock, error = 0, ch;
    pid_t           pid;
    char            c;
    char            genbuf[4];

    unsigned char   mode0 = currutmp->mode;

    genbuf[0] = defact;
    ch = uin->mode;
    strlcpy(currauthor, uin->userid, sizeof(currauthor));

    if (ch == EDITING || ch == TALK || ch == CHATING || ch == PAGE ||
	ch == MAILALL || ch == MONITOR || ch == M_FIVE || ch == CHC ||
	(!ch && (uin->chatid[0] == 1 || uin->chatid[0] == 3)) ||
	uin->lockmode == M_FIVE || uin->lockmode == CHC) {
	if (ch == CHC) {
	    kill(uin->pid, SIGUSR1);
	    sock = make_connection_to_somebody(uin, 20);
	    if (sock < 0)
		vmsg(SHM->i18nstr[cuser.language][1985]);
	    else {
		strlcpy(currutmp->mateid, uin->userid, sizeof(currutmp->mateid));
		chc(sock, CHC_WATCH);
	    }
	}
	else
	    outs(SHM->i18nstr[cuser.language][1986]);
    } else if (!HAS_PERM(PERM_SYSOP) &&
	       (((fri_stat & HRM) && !(fri_stat & HFM)) ||
		((!uin->pager) && !(fri_stat & HFM)))) {
	outs(SHM->i18nstr[cuser.language][1987]);
    } else if (!HAS_PERM(PERM_SYSOP) &&
	     (((fri_stat & HRM) && !(fri_stat & HFM)) || uin->pager == 2)) {
	outs(SHM->i18nstr[cuser.language][1988]);
    } else if (!HAS_PERM(PERM_SYSOP) &&
	       !(fri_stat & HFM) && uin->pager == 4) {
	outs(SHM->i18nstr[cuser.language][1989]);
    } else if (!(pid = uin->pid) /* || (kill(pid, 0) == -1) */ ) {
	//resetutmpent();
	outs(msg_usr_left);
    } else {
	showplans(uin->userid);
	getdata(2, 0, SHM->i18nstr[cuser.language][1990], genbuf, 4, LCECHO);
	switch (*genbuf) {
	case 'y':
	case 't':
	    uin->sig = SIG_TALK;
	    break;
	case 'f':
	    lockreturn(M_FIVE, LOCK_THIS);
	    uin->sig = SIG_GOMO;
	    break;
	case 'c':
	    lockreturn(CHC, LOCK_THIS);
	    uin->sig = SIG_CHC;
	    break;
	case 'd':
	    uin->sig = SIG_DARK;
	    break;
	case 'p':
	    reload_chicken();
	    getuser(uin->userid);
	    if (uin->lockmode == CHICKEN || currutmp->lockmode == CHICKEN)
		error = 1;
	    if (!cuser.mychicken.name[0] || !xuser.mychicken.name[0])
		error = 2;
	    if (error) {
		vmsg(error == 2 ? SHM->i18nstr[cuser.language][1991] :
		       SHM->i18nstr[cuser.language][1992]);
		return;
	    }
	    uin->sig = SIG_PK;
	    break;
	default:
	    return;
	}

	uin->turn = 1;
	currutmp->turn = 0;
	strlcpy(uin->mateid, currutmp->userid, sizeof(uin->mateid));
	strlcpy(currutmp->mateid, uin->userid, sizeof(currutmp->mateid));

	sock = make_connection_to_somebody(uin, 5);

	msgsock = accept(sock, (struct sockaddr *) 0, (socklen_t *) 0);
	if (msgsock == -1) {
	    perror("accept");
	    unlockutmpmode();
	    return;
	}
	add_io(0, 0);
	close(sock);
	currutmp->sockactive = NA;
	read(msgsock, &c, sizeof c);

	if (c == 'y') {
	    snprintf(save_page_requestor, sizeof(save_page_requestor),
		     "%s (%s)", uin->userid, uin->username);
	    /* gomo */
	    switch (uin->sig) {
	    case SIG_DARK:
		main_dark(msgsock, uin);
		break;
	    case SIG_PK:
		chickenpk(msgsock);
		break;
	    case SIG_GOMO:
		gomoku(msgsock);
		break;
	    case SIG_CHC:
		chc(msgsock, CHC_VERSUS);
		break;
	    case SIG_TALK:
	    default:
		do_talk(msgsock);
	    }
	} else {
	    move(9, 9);
	    outs(SHM->i18nstr[cuser.language][1993]);
	    switch (c) {
	    case 'a':
		outs(SHM->i18nstr[cuser.language][1994]);
		break;
	    case 'b':
		prints(SHM->i18nstr[cuser.language][1995], sig_des(uin->sig));
		break;
	    case 'd':
		outs(SHM->i18nstr[cuser.language][1996]);
		break;
	    case 'c':
		outs(SHM->i18nstr[cuser.language][1997]);
		break;
	    case 'e':
		outs(SHM->i18nstr[cuser.language][1998]);
		break;
	    case 'f':
		{
		    char            msgbuf[60];

		    read(msgsock, msgbuf, 60);
		    prints(SHM->i18nstr[cuser.language][1999], sig_des(uin->sig));
		    move(10, 18);
		    outs(msgbuf);
		}
		break;
	    case '1':
		prints(SHM->i18nstr[cuser.language][2000], sig_des(uin->sig));
		break;
	    case '2':
		prints(SHM->i18nstr[cuser.language][2001], sig_des(uin->sig));
		break;
	    default:
		prints(SHM->i18nstr[cuser.language][2002], sig_des(uin->sig));
	    }
	    close(msgsock);
	}
    }
    currutmp->mode = mode0;
    currutmp->destuid = 0;
    unlockutmpmode();
    pressanykey();
}

/* 選單式聊天介面 */
#define US_PICKUP       1234
#define US_RESORT       1233
#define US_ACTION       1232
#define US_REDRAW       1231

static void
t_showhelp()
{
    clear();

    outs(SHM->i18nstr[cuser.language][2003]);

    if (HAS_PERM(PERM_PAGE)) {
	outs(SHM->i18nstr[cuser.language][2004]);
    }
    if (HAS_PERM(PERM_SYSOP)) {
	outs(SHM->i18nstr[cuser.language][2005]);
	outs(SHM->i18nstr[cuser.language][2006]);
	outs(SHM->i18nstr[cuser.language][2007]);
#if defined(SHOWBOARD) && defined(DEBUG)
	outs(SHM->i18nstr[cuser.language][2008]);
#endif
    }
    pressanykey();
}

/*
 * static int listcuent(userinfo_t * uentp) { if((!uentp->invisible ||
 * HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_SEECLOAK)))
 * AddNameList(uentp->userid); return 0; }
 * 
 * static void creat_list() { CreateNameList(); apply_ulist(listcuent); }
 */

/* Kaede show friend description */
static char    *
friend_descript(userinfo_t * uentp, char *desc_buf, int desc_buflen)
{
    char    *space_buf = "";
    char            fpath[80], name[IDLEN + 2], *desc, *ptr;
    int             len, flag;
    FILE           *fp;
    char            genbuf[STRLEN];

    if((set_friend_bit(currutmp,uentp)|IFH)==0)
	return space_buf;

    setuserfile(fpath, friend_file[0]);

    if ((fp = fopen(fpath, "r"))) {
	snprintf(name, sizeof(name), "%s ", uentp->userid);
	len = strlen(name);
	desc = genbuf + 13;

	/* TODO maybe none linear search, or fread, or cache */
	while ((flag = (int)fgets(genbuf, STRLEN, fp))) {
	    if (!memcmp(genbuf, name, len)) {
		if ((ptr = strchr(desc, '\n')))
		    ptr[0] = '\0';
		break;
	    }
	}
	fclose(fp);
	if (flag)
	    strlcpy(desc_buf, desc, desc_buflen);
	else
	    return space_buf;

	return desc_buf;
    } else
	return space_buf;
}

/* XXX 為什麼 diff 是 time_t */
static char    *
descript(int show_mode, userinfo_t * uentp, time_t diff)
{
    static char     description[30];
    switch (show_mode) {
    case 1:
	return friend_descript(uentp, description, sizeof(description));
    case 0:
	return (((uentp->pager != 2 && uentp->pager != 3 && diff) ||
		 HAS_PERM(PERM_SYSOP)) ?
#ifdef WHERE
		uentp->from_alias ? SHM->replace[uentp->from_alias] :
		uentp->from
#else
		uentp->from
#endif
		: "*");
    case 2:
	snprintf(description, sizeof(description),
		 "%3d/%3d/%3d", uentp->five_win,
		 uentp->five_lose, uentp->five_tie);
	return description;
    case 3:
	snprintf(description, sizeof(description),
		 "%3d/%3d/%3d", uentp->chc_win,
		 uentp->chc_lose, uentp->chc_tie);
	return description;
    default:
	syslog(LOG_WARNING, "damn!!! what's wrong?? show_mode = %d",
	       show_mode);
	return "";
    }
}

/*
 * userlist
 * 
 * 有別於其他大部份 bbs在實作使用者名單時, 都是將所有 online users 取一份到
 * local space 中, 按照所須要的方式 sort 好 (如按照 userid , 五子棋, 來源等
 * 等) . 這將造成大量的浪費: 為什麼每個人都要為了產生這一頁僅 20 個人的資料
 * 而去 sort 其他一萬人的資料?
 *
 * 一般來說, 一份完整的使用者名單可以分成「好友區」和「非好友區」. 不同人的
 * 「好友區」應該會長的不一樣, 不過「非好友區」應該是長的一樣的. 針對這項特
 * 性, 兩區有不同的實作方式.
 *
 * + 好友區
 *   好友區只有在排列方式為 [嗨! 朋友] 的時候「可能」會用到.
 *   每個 process可以透過 currutmp->friend_online[]得到互相間有好友關係的資
 *   料 (不包括板友, 板友是另外生出來的) 不過 friend_online[]是 unorder的.
 *   所以須要先把所有的人拿出來, 重新 sort 一次.
 *   好友區 (互相任一方有設好友+ 板友) 最多只會有 MAX_FRIENDS個
 *   因為產生好友區的 cost 相當高, "能不產生就不要產生"
 *
 * + 非好友區
 *   透過 shmctl utmpsortd , 定期 (通常一秒一次) 將全站的人按照各種不同的方
 *   式 sort 好, 放置在 SHM->sorted中.
 *
 * 接下來, 我們每次只從確定的起始位置拿, 特別是除非有須要, 不然不會去產生好
 * 友區. 
 *
 * 各個 function 摘要
 * sort_cmpfriend()   sort function, key: friend type
 * pickup_maxpages()  # pages of userlist
 * pickup_myfriend()  產生好友區
 * pickup_bfriend()   產生板友
 * pickup()           產生某一頁使用者名單
 * draw_pickup()      把畫面輸出
 * userlist()         主函式, 負責呼叫 pickup()/draw_pickup() 以及按鍵處理
 *
 * SEE ALSO
 *     include/pttstruct.h
 *
 * BUGS
 *     搜尋的時候沒有辦法移到該人上面
 *
 * AUTHOR
 *     in2 <in2@in2home.org>
 */
char    nPickups;

static int
sort_cmpfriend(const void *a, const void *b)
{
    if (((((pickup_t *) a)->friend) & ST_FRIEND) ==
	((((pickup_t *) b)->friend) & ST_FRIEND))
	return strcasecmp(((pickup_t *) a)->ui->userid,
			  ((pickup_t *) b)->ui->userid);
    else
	return (((pickup_t *) b)->friend & ST_FRIEND) -
	    (((pickup_t *) a)->friend & ST_FRIEND);
}

int
pickup_maxpages(int pickupway, int nfriends)
{
    int             number;
    if (cuser.uflag & FRIEND_FLAG)
	number = nfriends;
    else
	number = SHM->UTMPnumber +
	    (pickupway == 0 ? nfriends : 0);
    return (number - 1) / nPickups + 1;
}

static int
pickup_myfriend(pickup_t * friends,
		int *myfriend, int *friendme, int *badfriend)
{
    userinfo_t     *uentp;
    int             i, where, frstate, ngets = 0;

    *badfriend = 0;
    *myfriend = *friendme = 1;
    for (i = 0; currutmp->friend_online[i] && i < MAX_FRIEND; ++i) {
	where = currutmp->friend_online[i] & 0xFFFFFF;
	if (0 <= where && where < MAX_ACTIVE &&
	    (uentp = &SHM->uinfo[where]) && uentp->pid &&
	    uentp != currutmp &&
	    isvisible_stat(currutmp, uentp,
			   frstate =
			   currutmp->friend_online[i] >> 24)){
	    if( frstate & IRH )
		++*badfriend;
	    if( !(frstate & IRH) || ((frstate & IRH) && (frstate & IFH)) ){
		friends[ngets].ui = uentp;
		friends[ngets].uoffset = where;
		friends[ngets++].friend = frstate;
		if (frstate & IFH)
		    ++* myfriend;
		if (frstate & HFM)
		    ++* friendme;
	    }
	}
    }
    /* 把自己加入好友區 */
    friends[ngets].ui = currutmp;
    friends[ngets++].friend = (IFH | HFM);
    return ngets;
}

static int
pickup_bfriend(pickup_t * friends, int base)
{
    userinfo_t     *uentp;
    int             i, ngets = 0;
    int             currsorted = SHM->currsorted, number = SHM->UTMPnumber;
    friends = friends + base;
    for (i = 0; i < number && ngets < MAX_FRIEND - base; ++i) {
	uentp = SHM->sorted[currsorted][0][i];
	if (uentp && uentp->pid && uentp->brc_id == currutmp->brc_id &&
	    currutmp != uentp && isvisible(currutmp, uentp) &&
	    (base || !(friend_stat(currutmp, uentp) & (IFH | HFM)))) {
	    friends[ngets].ui = uentp;
	    friends[ngets++].friend = IBH;
	}
    }
    return ngets;
}

static void
pickup(pickup_t * currpickup, int pickup_way, int *page,
       int *nfriend, int *myfriend, int *friendme, int *bfriend, int *badfriend)
{
    /* avoid race condition */
    int             currsorted = SHM->currsorted;
    int             utmpnumber = SHM->UTMPnumber;
    int             friendtotal = currutmp->friendtotal;

    userinfo_t    **utmp;
    int             which, sorted_way, size = 0, friend;

    if (friendtotal == 0)
	*myfriend = *friendme = 1;

    /* 產生好友區 */
    which = *page * nPickups;
    if( (cuser.uflag & FRIEND_FLAG) || /* 只顯示好友模式 */
	((pickup_way == 0) &&          /* [嗨! 朋友] mode */
	 (
	  /* 含板友, 好友區最多只會有 (friendtotal + 板友) 個*/
	  (currutmp->brc_id && which < (friendtotal + 1 +
					bcache[currutmp->brc_id-1].nuser)) ||
	  
	  /* 不含板友, 最多只會有 friendtotal個 */
	  (!currutmp->brc_id && which < friendtotal + 1)
	  ))) {
	pickup_t        friends[MAX_FRIEND];

	*nfriend = pickup_myfriend(friends, myfriend, friendme, badfriend);

	if (pickup_way == 0 && currutmp->brc_id != 0){
	    *nfriend += pickup_bfriend(friends, *nfriend);
	    *bfriend = SHM->bcache[currutmp->brc_id - 1].nuser;
	}
	else
	    *bfriend = 0;
	if (*nfriend > which) {
	    /* 只有在要秀出才有必要 sort */
	    qsort(friends, *nfriend, sizeof(pickup_t), sort_cmpfriend);
	    size = *nfriend - which;
	    if (size > nPickups)
		size = nPickups;
	    memcpy(currpickup, friends + which, sizeof(pickup_t) * size);
	}
    } else
	*nfriend = 0;

    if (!(cuser.uflag & FRIEND_FLAG) && size < nPickups) {
	sorted_way = ((pickup_way == 0) ? 0 : (pickup_way - 1));
	utmp = SHM->sorted[currsorted][sorted_way];
	which = *page * nPickups - *nfriend;
	if (which < 0)
	    which = 0;
	for (; which < utmpnumber && size < nPickups; which++) {
	    friend = friend_stat(currutmp, utmp[which]);
	    if ((pickup_way ||
		 (currutmp != utmp[which] && !(friend & ST_FRIEND))) &&
		isvisible(currutmp, utmp[which])) {
		currpickup[size].ui = utmp[which];
		currpickup[size++].friend = friend;
	    }
	}
    }

    for (; size < nPickups; ++size)
	currpickup[size].ui = 0;
}

static void
draw_pickup(int drawall, pickup_t * pickup, int pickup_way,
	    int page, int show_mode, int show_uid, int show_board,
	    int show_pid, int myfriend, int friendme, int bfriend, int badfriend)
{
    char           *msg_pickup_way[PICKUP_WAYS] = {
	SHM->i18nstr[cuser.language][2009], SHM->i18nstr[cuser.language][2010], SHM->i18nstr[cuser.language][2011], SHM->i18nstr[cuser.language][2012], SHM->i18nstr[cuser.language][2013], SHM->i18nstr[cuser.language][2014], SHM->i18nstr[cuser.language][2015]
    };
    char           *MODE_STRING[MAX_SHOW_MODE] = {
	SHM->i18nstr[cuser.language][2016], SHM->i18nstr[cuser.language][2017], SHM->i18nstr[cuser.language][2018], SHM->i18nstr[cuser.language][2019]
    };
    char            pagerchar[5] = "* -Wf";

    userinfo_t     *uentp;
    int             i, ch, state, friend;
    char            mind[5];
#ifdef SHOW_IDLE_TIME
    char            idlestr[32];
    int             idletime;
#endif

    if (drawall) {
	showtitle((cuser.uflag & FRIEND_FLAG) ? SHM->i18nstr[cuser.language][2020] : SHM->i18nstr[cuser.language][2021],
		  BBSName);
	prints(SHM->i18nstr[cuser.language][2022],
	       show_uid ? "UID" : "No.",
	       (HAS_PERM(PERM_SEECLOAK) || HAS_PERM(PERM_SYSOP)) ? 'C' : ' ',
	       SHM->i18nstr[cuser.language][2023],
	       MODE_STRING[show_mode],
	       show_board ? "Board" : SHM->i18nstr[cuser.language][2024],
	       show_pid ? "       PID" : SHM->i18nstr[cuser.language][2025]
	    );
	move(b_lines, 0);
	outs(SHM->i18nstr[cuser.language][2026]);
    }
    move(1, 0);
    prints(SHM->i18nstr[cuser.language][2027],
	   msg_pickup_way[pickup_way], SHM->UTMPnumber,
	   myfriend, friendme, currutmp->brc_id ? (bfriend + 1) : 0, badfriend);

    for (i = 0, ch = page * nPickups + 1; i < nPickups; ++i, ++ch) {
	move(i + 3, 0);
	prints("a");
	move(i + 3, 0);
	uentp = pickup[i].ui;
	friend = pickup[i].friend;
	if (uentp == NULL) {
	    prints("\n");
	    continue;
	}
	if (!uentp->pid) {
	    prints(SHM->i18nstr[cuser.language][2028], ch);
	    continue;
	}
	if (PERM_HIDE(uentp))
	    state = 9;
	else if (currutmp == uentp)
	    state = 10;
	else if (friend & IRH && !(friend & IFH))
	    state = 8;
	else
	    state = (friend & ST_FRIEND) >> 2;

#ifdef SHOW_IDLE_TIME
	idletime = (now - uentp->lastact);
	if (idletime > 86400)
	    strlcpy(idlestr, " -----", sizeof(idlestr));
	else if (idletime >= 3600)
	    snprintf(idlestr, sizeof(idlestr), "%3dh%02d",
		     idletime / 3600, (idletime / 60) % 60);
	else if (idletime > 0)
	    snprintf(idlestr, sizeof(idlestr), "%3d'%02d",
		     idletime / 60, idletime % 60);
	else
	    strlcpy(idlestr, "      ", sizeof(idlestr));
#endif

	if ((uentp->userlevel & PERM_VIOLATELAW))
	    memcpy(mind, SHM->i18nstr[cuser.language][2029], 4);
	else if (uentp->birth)
	    memcpy(mind, SHM->i18nstr[cuser.language][2030], 4);
	else
	    memcpy(mind, uentp->mind, 4);
	mind[4] = 0;
	prints("%5d %c%c%s%-13s%-17.16s\033[m%-17.16s%-13.13s\33[33m%-4.4s\33[m%s\n",

	/* list number or uid */
#ifdef SHOWUID
	       show_uid ? uentp->uid :
#endif
	       ch,

	/* super friend or pager */
	       (friend & HRM) ? 'X' : pagerchar[uentp->pager % 5],

	/* visibility */
	       (uentp->invisible ? ')' : ' '),

	/* color of userid, userid */
	       fcolor[state], uentp->userid,

	/* nickname */
	       uentp->username,

	/* from */
	       descript(show_mode, uentp,
			uentp->pager & !(friend & HRM)),

	/* board or mode */
#if defined(SHOWBOARD) && defined(DEBUG)
	       show_board ? (uentp->brc_id == 0 ? "" :
			     bcache[uentp->brc_id - 1].brdname) :
#endif
	/* %-13.13s */
	       modestring(uentp, 0),

	/* memo */
	       mind,

	/* idle */
#ifdef SHOW_IDLE_TIME
	       idlestr
#else
	       ""
#endif
	    );

    }
}

int
call_in(userinfo_t * uentp, int fri_stat)
{
    if (iswritable_stat(uentp, fri_stat)) {
	char            genbuf[60];
	snprintf(genbuf, sizeof(genbuf), SHM->i18nstr[cuser.language][2031], uentp->userid);
	my_write(uentp->pid, genbuf, uentp->userid, 0, NULL);
	return 1;
    }
    return 0;
}

inline static void
userlist(void)
{
    pickup_t       *currpickup;
    userinfo_t     *uentp;
    static char     show_mode = 0;
    static char     show_uid = 0;
    static char     show_board = 0;
    static char     show_pid = 0;
    char            skippickup = 0, redraw, redrawall;
    int             page, offset, pickup_way, ch, leave, fri_stat;
    int             nfriend, myfriend, friendme, bfriend, badfriend, i;
    time_t          lastupdate;

    nPickups = b_lines - 3;
    currpickup = (pickup_t *)malloc(sizeof(pickup_t) * nPickups);
    page = offset = 0;
    pickup_way = 0;
    leave = 0;
    redrawall = 1;
    /*
     * 各個 flag :
     * redraw:    重新 pickup(), draw_pickup() (僅中間區, 不含標題列等等)
     * redrawall: 全部重畫 (含標題列等等, 須再指定 redraw 才會有效)
     * leave:     離開使用者名單
     */
    while (!leave) {
	if( !skippickup )
	    pickup(currpickup, pickup_way, &page,
		   &nfriend, &myfriend, &friendme, &bfriend, &badfriend);
	draw_pickup(redrawall, currpickup, pickup_way, page,
		    show_mode, show_uid, show_board, show_pid,
		    myfriend, friendme, bfriend, badfriend);

	/*
	 * 如果因為換頁的時候, 這一頁有的人數比較少,
	 * (通常都是最後一頁人數不滿的時候) 那要重新計算 offset
	 * 以免指到沒有人的地方
	 */
	if (offset == -1 || currpickup[offset].ui == NULL) {
	    for (offset = (offset == -1 ? nPickups - 1 : offset);
		 offset >= 0; --offset)
		if (currpickup[offset].ui != NULL)
		    break;
	    if (offset == -1) {
		if (--page < 0)
		    page = pickup_maxpages(pickup_way, nfriend) - 1;
		offset = 0;
		continue;
	    }
	}
	skippickup = redraw = redrawall = 0;
	lastupdate = now;
	while (!redraw) {
	    ch = cursor_key(offset + 3, 0);
	    uentp = currpickup[offset].ui;
	    fri_stat = currpickup[offset].friend;

	    if (ch == KEY_RIGHT || ch == '\n' || ch == '\r')
		ch = 't';

	    switch (ch) {
	    case KEY_LEFT:
	    case 'e':
	    case 'E':
		redraw = leave = 1;
		break;

	    case KEY_TAB:
		pickup_way = (pickup_way + 1) % PICKUP_WAYS;
		redraw = 1;
		redrawall = 1;
		break;

	    case KEY_DOWN:
	    case 'n':
	    case 'j':
		if (++offset == nPickups || currpickup[offset].ui == NULL) {
		    redraw = 1;
		    if (++page >= pickup_maxpages(pickup_way,
						  nfriend))
			offset = page = 0;
		    else
			offset = 0;
		}
		break;

	    case '0':
	    case KEY_HOME:
		page = offset = 0;
		redraw = 1;
		break;

	    case 'H':
		if (HAS_PERM(PERM_SYSOP)) {
		    currutmp->userlevel ^= PERM_SYSOPHIDE;
		    redrawall = redraw = 1;
		}
		break;

	    case 'D':
		if (HAS_PERM(PERM_SYSOP)) {
		    char            buf[100];
		    snprintf(buf, sizeof(buf),
			     SHM->i18nstr[cuser.language][2032], currutmp->userid);
		    if (!getdata(1, 0, buf, currutmp->userid,
				 sizeof(buf), DOECHO))
			strlcpy(currutmp->userid, cuser.userid, sizeof(currutmp->userid));
		    redrawall = redraw = 1;
		}
		break;

	    case 'F':
		if (HAS_PERM(PERM_SYSOP)) {
		    char            buf[100];

		    snprintf(buf, sizeof(buf), SHM->i18nstr[cuser.language][2033], currutmp->from);
		    if (!getdata(1, 0, buf, currutmp->from,
				 sizeof(currutmp->from), DOECHO))
			strncpy(currutmp->from, buf, 23);
		    redrawall = redraw = 1;
		}
		break;

	    case 'C':
#if !HAVE_FREECLOAK
		if (HAS_PERM(PERM_CLOAK))
#endif
		{
		    currutmp->invisible ^= 1;
		    redrawall = redraw = 1;
		}
		break;

	    case ' ':
	    case KEY_PGDN:
	    case Ctrl('F'):{
		    int             newpage;
		    if ((newpage = page + 1) >= pickup_maxpages(pickup_way,
								nfriend))
			newpage = offset = 0;
		    if (newpage != page) {
			page = newpage;
			redraw = 1;
		    } else if (now >= lastupdate + 2)
			redrawall = redraw = 1;
		}
		break;

	    case KEY_UP:
	    case 'k':
		if (--offset == -1) {
		    offset = nPickups - 1;
		    if (--page == -1)
			page = pickup_maxpages(pickup_way, nfriend)
			    - 1;
		    redraw = 1;
		}
		break;

	    case KEY_PGUP:
	    case Ctrl('B'):
	    case 'P':
		if (--page == -1)
		    page = pickup_maxpages(pickup_way, nfriend) - 1;
		offset = 0;
		redraw = 1;
		break;

	    case KEY_END:
	    case '$':
		page = pickup_maxpages(pickup_way, nfriend) - 1;
		offset = -1;
		redraw = 1;
		break;

	    case '/':
		/*
		 * getdata_buf(b_lines-1,0,"請輸入暱稱關鍵字:", keyword,
		 * sizeof(keyword), DOECHO); state = US_PICKUP;
		 */
		break;

	    case 's':
		if (!(cuser.uflag & FRIEND_FLAG)) {
		    int             si;	/* utmpshm->sorted[X][0][si] */
		    int             fi;	/* allpickuplist[fi] */
		    char            swid[IDLEN + 1];
		    move(1, 0);
		    si = generalnamecomplete(msg_uid, swid,
					     sizeof(swid), SHM->UTMPnumber,
					     completeutmp_compar,
					     completeutmp_permission,
					     completeutmp_getname);
		    if (si >= 0) {
			pickup_t        friends[MAX_FRIEND + 1];
			int             nGots, i;
			userinfo_t **utmp =
			    SHM->sorted[SHM->currsorted]
			    [((pickup_way == 0) ? 0 : (pickup_way - 1))];
			
			fi = utmp[si] - &SHM->uinfo[0];

			nGots = pickup_myfriend(friends, &myfriend,
						&friendme, &badfriend);
			for (i = 0; i < nGots; ++i)
			    if (friends[i].uoffset == fi)
				break;

			fi = 0;
			offset = 0;
			if( i != nGots ){
			    page = i / nPickups;
			    for( ; i < nGots && fi < nPickups ; ++i )
				if( isvisible(currutmp, friends[i].ui) )
				    currpickup[fi++] = friends[i];
			    i = 0;
			}
			else{
			    page = (si + nGots) / nPickups;
			    i = si;
			}

			for( ; fi < nPickups && i < SHM->UTMPnumber ; ++i )
			    if( isvisible(currutmp, utmp[i]) ){
				currpickup[fi].ui = utmp[i];
				currpickup[fi++].friend = 0;
			    }
			skippickup = 1;
		    }
		    redrawall = redraw = 1;
		}
		/*
		 * if ((i = search_pickup(num, actor, pklist)) >= 0) num = i;
		 * state = US_ACTION;
		 */
		break;

	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		{		/* Thor: 可以打數字跳到該人 */
		    int             tmp;
		    if ((tmp = search_num(ch, SHM->UTMPnumber)) >= 0) {
			if (tmp / nPickups == page) {
			    /*
			     * in2:目的在目前這一頁, 直接 更新 offset ,
			     * 不用重畫畫面
			     */
			    offset = tmp % nPickups;
			} else {
			    page = tmp / nPickups;
			    offset = tmp % nPickups;
			}
			redrawall = redraw = 1;
		    }
		}
		break;

#ifdef SHOWUID
	    case 'U':
		if (HAS_PERM(PERM_SYSOP)) {
		    show_uid ^= 1;
		    redrawall = redraw = 1;
		}
		break;
#endif
#if defined(SHOWBOARD) && defined(DEBUG)
	    case 'Y':
		if (HAS_PERM(PERM_SYSOP)) {
		    show_board ^= 1;
		    redrawall = redraw = 1;
		}
		break;
#endif
#ifdef  SHOWPID
	    case '#':
		if (HAS_PERM(PERM_SYSOP)) {
		    show_pid ^= 1;
		    redrawall = redraw = 1;
		}
		break;
#endif

	    case 'b':		/* broadcast */
		if (cuser.uflag & FRIEND_FLAG || HAS_PERM(PERM_SYSOP)) {
		    char            genbuf[60];
		    char            ans[4];

		    if (!getdata(0, 0, SHM->i18nstr[cuser.language][2034], genbuf, sizeof(genbuf), DOECHO))
			break;
		    if (getdata(0, 0, SHM->i18nstr[cuser.language][2035],
				ans, sizeof(ans), LCECHO) &&
			*ans == 'n')
			break;
		    if (!(cuser.uflag & FRIEND_FLAG) && HAS_PERM(PERM_SYSOP)) {
			getdata(1, 0, SHM->i18nstr[cuser.language][2036],
				ans, sizeof(ans), LCECHO);
			if( *ans != 'y' && *ans != 'Y' ){
			    vmsg("abort");
			    break;
			}
			for (i = 0; i < SHM->UTMPnumber; ++i) {
			    uentp = SHM->sorted[SHM->currsorted][0][i];
			    if (uentp->pid && kill(uentp->pid, 0) != -1)
				my_write(uentp->pid, genbuf,
					 uentp->userid, 1, NULL);
			    if (i % 100 == 0)
				sleep(1);
			}
		    } else {
			userinfo_t     *uentp;
			int             where, frstate;
			for (i = 0; currutmp->friend_online[i] &&
			     i < MAX_FRIEND; ++i) {
			    where = currutmp->friend_online[i] & 0xFFFFFF;
			    if (0 <= where && where < MAX_ACTIVE &&
				(uentp = &SHM->uinfo[where]) &&
				uentp->pid &&
				isvisible_stat(currutmp, uentp,
					       frstate =
					   currutmp->friend_online[i] >> 24)
				&& kill(uentp->pid, 0) != -1 &&
				uentp->pager != 3 &&
				(uentp->pager != 4 || frstate & HFM) &&
				!(frstate & IRH)) {
				my_write(uentp->pid, genbuf,
					 uentp->userid, 1, NULL);
			    }
			}
		    }
		    redrawall = redraw = 1;
		}
		break;

	    case 'S':		/* 顯示好友描述 */
		show_mode = (show_mode+1) % MAX_SHOW_MODE;
		redrawall = redraw = 1;
		break;

	    case 'u':		/* 線上修改資料 */
		if (HAS_PERM(PERM_ACCOUNTS)) {
		    int             id;
		    userec_t        muser;
		    strlcpy(currauthor, uentp->userid, sizeof(currauthor));
		    stand_title(SHM->i18nstr[cuser.language][2037]);
		    move(1, 0);
		    if ((id = getuser(uentp->userid)) > 0) {
			memcpy(&muser, &xuser, sizeof(muser));
			user_display(&muser, 1);
			uinfo_query(&muser, 1, id);
		    }
		    redrawall = redraw = 1;
		}
		break;

	    case 'i':{
		    char            mindbuf[5];
		    getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][2038],
			    mindbuf, sizeof(mindbuf), DOECHO);
		    if (strcmp(mindbuf, SHM->i18nstr[cuser.language][2039]) == 0)
			vmsg(SHM->i18nstr[cuser.language][2040]);
		    else if (strcmp(mindbuf, SHM->i18nstr[cuser.language][2041]) == 0)
			vmsg(SHM->i18nstr[cuser.language][2042]);
		    else
			memcpy(currutmp->mind, mindbuf, 4);
		}
		redrawall = redraw = 1;
		break;

	    case Ctrl('S'):
		break;

	    case 't':
		if (HAS_PERM(PERM_LOGINOK)) {
		    if (uentp->pid != currpid &&
			    strcmp(uentp->userid, cuser.userid) != 0) {
			move(1, 0);
			clrtobot();
			move(3, 0);
			my_talk(uentp, fri_stat, 0);
			redrawall = redraw = 1;
		    }
		}
		break;
	    case 'K':
		if (HAS_PERM(PERM_ACCOUNTS)) {
		    my_kick(uentp);
		    redrawall = redraw = 1;
		}
		break;
	    case 'w':
		if (call_in(uentp, fri_stat))
		    redrawall = redraw = 1;
		break;
	    case 'a':
		if (HAS_PERM(PERM_LOGINOK) && !(fri_stat & IFH)) {
		    if (getans(SHM->i18nstr[cuser.language][2043]) == 'y') {
			friend_add(uentp->userid, FRIEND_OVERRIDE,uentp->username);
			friend_load(FRIEND_OVERRIDE);
		    }
		    redrawall = redraw = 1;
		}
		break;

	    case 'd':
		if (HAS_PERM(PERM_LOGINOK) && (fri_stat & IFH)) {
		    if (getans(SHM->i18nstr[cuser.language][2044]) == 'y') {
			friend_delete(uentp->userid, FRIEND_OVERRIDE);
			friend_load(FRIEND_OVERRIDE);
		    }
		    redrawall = redraw = 1;
		}
		break;

	    case 'o':
		if (HAS_PERM(PERM_LOGINOK)) {
		    t_override();
		    redrawall = redraw = 1;
		}
		break;

	    case 'f':
		if (HAS_PERM(PERM_LOGINOK)) {
		    cuser.uflag ^= FRIEND_FLAG;
		    redrawall = redraw = 1;
		}
		break;

	    case 'g':
		if (HAS_PERM(PERM_LOGINOK) &&
		    strcmp(uentp->userid, cuser.userid) != 0) {
		    char genbuf[10];
		    move(b_lines - 2, 0);
		    prints(SHM->i18nstr[cuser.language][2045], uentp->userid);
		    if (getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][2046],
				genbuf, 7, LCECHO)) {
			clrtoeol();
			if ((ch = atoi(genbuf)) <= 0 || ch <= give_tax(ch)){
			    redrawall = redraw = 1;
			    break;
			}
			sprintf(genbuf, SHM->i18nstr[cuser.language][2047], uentp->userid, ch);
			if (getans(genbuf) != 'y'){
			    redrawall = redraw = 1;
			    break;
			}
			reload_money();

			if (ch > cuser.money) {
			    outs(SHM->i18nstr[cuser.language][2048]);
			} else {
			    deumoney(uentp->uid, ch - give_tax(ch));
			    log_file(FN_MONEY, 1, SHM->i18nstr[cuser.language][2050],
			    	cuser.userid,uentp->userid, ch,ctime(&currutmp->lastact));
			    mail_redenvelop(cuser.userid, uentp->userid,
					    ch - give_tax(ch), 'Y');
				vmsg(SHM->i18nstr[cuser.language][2049],demoney(-ch));
			}
		    } else {
			clrtoeol();
			vmsg(SHM->i18nstr[cuser.language][2051]);
		    }
		    redrawall = redraw = 1;
		}
		break;

	    case 'm':
		if (HAS_PERM(PERM_BASIC)) {
		    stand_title(SHM->i18nstr[cuser.language][2052]);
		    prints(SHM->i18nstr[cuser.language][2053], uentp->userid);
		    my_send(uentp->userid);
		    setutmpmode(LUSERS);
		    redrawall = redraw = 1;
		}
		break;

	    case 'q':
		strlcpy(currauthor, uentp->userid, sizeof(currauthor));
		my_query(uentp->userid);
		setutmpmode(LUSERS);
		redrawall = redraw = 1;
		break;

	    case 'c':
		if (HAS_PERM(PERM_LOGINOK)) {
		    chicken_query(uentp->userid);
		    redrawall = redraw = 1;
		}
		break;

	    case 'l':
		if (HAS_PERM(PERM_LOGINOK)) {
		    t_display();
		    redrawall = redraw = 1;
		}
		break;

	    case 'h':
		t_showhelp();
		redrawall = redraw = 1;
		break;

	    case 'p':
		if (HAS_PERM(PERM_BASIC)) {
		    t_pager();
		    redrawall = redraw = 1;
		}
		break;

	    case Ctrl('W'):
		if (HAS_PERM(PERM_LOGINOK)) {
		    int             tmp;
		    char           *wm[3] = {SHM->i18nstr[cuser.language][2054], SHM->i18nstr[cuser.language][2055], SHM->i18nstr[cuser.language][2056]};
		    tmp = cuser.uflag2 & WATER_MASK;
		    cuser.uflag2 -= tmp;
		    tmp = (tmp + 1) % 3;
		    cuser.uflag2 |= tmp;
		    vmsg(SHM->i18nstr[cuser.language][2057], wm[tmp]);
		    redrawall = redraw = 1;
		}
		break;

	    case 'r':
		if (HAS_PERM(PERM_LOGINOK)) {
		    m_read();
		    setutmpmode(LUSERS);
		    redrawall = redraw = 1;
		}
		break;

	    case 'N':
		oldgetdata(1, 0, SHM->i18nstr[cuser.language][2058],
			cuser.username, sizeof(cuser.username), DOECHO);
		strcpy(currutmp->username, cuser.username);
		redrawall = redraw = 1;
		break;

	    default:
		if (now >= lastupdate + 2)
		    redraw = 1;
	    }
	}
    }
    free(currpickup);
}

int
t_users(void)
{
    int             destuid0 = currutmp->destuid;
    int             mode0 = currutmp->mode;
    int             stat0 = currstat;

    if( cuser.userid[0] != currutmp->userid[0] ){
	if( HAS_PERM(PERM_SYSOP) )
	    vmsg("warning: currutmp userid is changed");
	else
	    abort_bbs(0);
    }
    setutmpmode(LUSERS);
    userlist();
    currutmp->mode = mode0;
    currutmp->destuid = destuid0;
    currstat = stat0;
    return 0;
}

int
t_pager(void)
{
    currutmp->pager = (currutmp->pager + 1) % 5;
    return 0;
}

int
t_idle(void)
{
    int             destuid0 = currutmp->destuid;
    int             mode0 = currutmp->mode;
    int             stat0 = currstat;
    char            genbuf[20];
    char            passbuf[PASSLEN];

    setutmpmode(IDLE);
    getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][2059], genbuf, 3, DOECHO);
    if (genbuf[0] == 'q' || genbuf[0] == 'Q') {
	currutmp->mode = mode0;
	currstat = stat0;
	return 0;
    } else if (genbuf[0] >= '1' && genbuf[0] <= '6')
	currutmp->destuid = genbuf[0] - '0';
    else
	currutmp->destuid = 0;

    if (currutmp->destuid == 6)
	if (!cuser.userlevel ||
	    !getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][2060],
		     currutmp->chatid, sizeof(currutmp->chatid), DOECHO))
	    currutmp->destuid = 0;
    do {
	move(b_lines - 2, 0);
	clrtoeol();
	prints(SHM->i18nstr[cuser.language][2061], (currutmp->destuid != 6) ?
		 SHM->i18nstr[cuser.language][1890 + currutmp->destuid] : currutmp->chatid);
	refresh();
	getdata(b_lines - 1, 0, MSG_PASSWD, passbuf, sizeof(passbuf), NOECHO);
	passbuf[8] = '\0';
    }
    while (!checkpasswd(cuser.passwd, passbuf) &&
	   strcmp(STR_GUEST, cuser.userid));

    currutmp->mode = mode0;
    currutmp->destuid = destuid0;
    currstat = stat0;

    return 0;
}

int
t_qchicken(void)
{
    char            uident[STRLEN];

    stand_title(SHM->i18nstr[cuser.language][2062]);
    usercomplete(msg_uid, uident);
    if (uident[0])
	chicken_query(uident);
    return 0;
}

int
t_query(void)
{
    char            uident[STRLEN];

    stand_title(SHM->i18nstr[cuser.language][2063]);
    usercomplete(msg_uid, uident);
    if (uident[0])
	my_query(uident);
    return 0;
}

int
t_talk()
{
    char            uident[16];
    int             tuid, unum, ucount;
    userinfo_t     *uentp;
    char            genbuf[4];
    /*
     * if (count_ulist() <= 1){ outs("目前線上只有您一人，快邀請朋友來光臨【"
     * BBSNAME "】吧！"); return XEASY; }
     */
    stand_title(SHM->i18nstr[cuser.language][2064]);
    generalnamecomplete(msg_uid, uident, sizeof(uident),
			SHM->UTMPnumber,
			completeutmp_compar,
			completeutmp_permission,
			completeutmp_getname);
    if (uident[0] == '\0')
	return 0;

    move(3, 0);
    if (!(tuid = searchuser(uident)) || tuid == usernum) {
	outs(err_uid);
	pressanykey();
	return 0;
    }
    /* multi-login check */
    unum = 1;
    while ((ucount = count_logins(tuid, 0)) > 1) {
	outs(SHM->i18nstr[cuser.language][2065]);
	count_logins(tuid, 1);
	getdata(1, 33, SHM->i18nstr[cuser.language][2066], genbuf, 4, DOECHO);
	unum = atoi(genbuf);
	if (unum == 0)
	    return 0;
	move(3, 0);
	clrtobot();
	if (unum > 0 && unum <= ucount)
	    break;
    }

    if ((uentp = (userinfo_t *) search_ulistn(tuid, unum)))
	my_talk(uentp, friend_stat(currutmp, uentp), 0);

    return 0;
}

int
reply_connection_request(userinfo_t *uip)
{
    int		    a;
    char            buf[4], genbuf[200];
    struct hostent *h;
    struct sockaddr_in sin;

    if (uip->mode != PAGE) {
	snprintf(genbuf, sizeof(genbuf),
		 SHM->i18nstr[cuser.language][2067], page_requestor);
	getdata(0, 0, genbuf, buf, sizeof(buf), LCECHO);
	return -1;
    }
    currutmp->msgcount = 0;
    strlcpy(save_page_requestor, page_requestor, sizeof(save_page_requestor));
    memset(page_requestor, 0, sizeof(page_requestor));
    if (!(h = gethostbyname("localhost"))) {
	perror("gethostbyname");
	return -1;
    }
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = h->h_addrtype;
    memcpy(&sin.sin_addr, h->h_addr, h->h_length);
    sin.sin_port = uip->sockaddr;
    a = socket(sin.sin_family, SOCK_STREAM, 0);
    if ((connect(a, (struct sockaddr *) & sin, sizeof(sin)))) {
	perror("connect err");
	return -1;
    }
    return a;
}

/* 有人來串門子了，回應呼叫器 */
void
talkreply(void)
{
    char            buf[4];
    char            genbuf[200];
    int             a, sig = currutmp->sig;

    uip = &SHM->uinfo[currutmp->destuip];
    snprintf(page_requestor, sizeof(page_requestor),
	    "%s (%s)", uip->userid, uip->username);
    currutmp->destuid = uip->uid;
    currstat = REPLY;		/* 避免出現動畫 */

    clear();

    prints("\n\n");
    prints(SHM->i18nstr[cuser.language][2068], sig_des(sig));
    prints(SHM->i18nstr[cuser.language][2069],
	    sig_des(sig), sig_des(sig));
    prints(SHM->i18nstr[cuser.language][2070]);
    prints(SHM->i18nstr[cuser.language][2071]);
    prints(SHM->i18nstr[cuser.language][2072], sig_des(sig), sig_des(sig));

    getuser(uip->userid);
    currutmp->msgs[0].pid = uip->pid;
    strlcpy(currutmp->msgs[0].userid, uip->userid, sizeof(currutmp->msgs[0].userid));
    strlcpy(currutmp->msgs[0].last_call_in, SHM->i18nstr[cuser.language][2073],
	    sizeof(currutmp->msgs[0].last_call_in));
    prints(SHM->i18nstr[cuser.language][2074],
	    uip->from, xuser.numlogins, xuser.numposts);
    showplans(uip->userid);
    show_call_in(0, 0);

    snprintf(genbuf, sizeof(genbuf),
	    SHM->i18nstr[cuser.language][2075],
	    page_requestor, sig_des(sig));
    getdata(0, 0, genbuf, buf, sizeof(buf), LCECHO);
    a = reply_connection_request(uip);

    if (!buf[0] || !strchr("yabcdef12", buf[0]))
	buf[0] = 'n';
    write(a, buf, 1);
    if (buf[0] == 'f' || buf[0] == 'F') {
	if (!getdata(b_lines, 0, SHM->i18nstr[cuser.language][2076], genbuf, 60, DOECHO))
	    strlcpy(genbuf, SHM->i18nstr[cuser.language][2077], sizeof(genbuf));
	write(a, genbuf, 60);
    }

    uip->destuip = currutmp - &SHM->uinfo[0];
    if (buf[0] == 'y')
	switch (sig) {
	case SIG_DARK:
	    main_dark(a, uip);
	    break;
	case SIG_PK:
	    chickenpk(a);
	    break;
	case SIG_GOMO:
	    gomoku(a);
	    break;
	case SIG_CHC:
	    chc(a, CHC_VERSUS);
	    break;
	case SIG_TALK:
	default:
	    do_talk(a);
	}
    else
	close(a);
    clear();
}

/* 網友動態簡表 */
/*
 * not used static int shortulist(userinfo_t * uentp) { static int lineno,
 * fullactive, linecnt; static int moreactive, page, num; char uentry[50];
 * int state;
 * 
 * if (!lineno){ lineno = 3; page = moreactive ? (page + p_lines * 3) : 0;
 * linecnt = num = moreactive = 0; move(1, 70); prints("Page: %d", page /
 * (p_lines) / 3 + 1); move(lineno, 0); }
 * 
 * if (uentp == NULL){ int finaltally;
 * 
 * clrtoeol(); move(++lineno, 0); clrtobot(); finaltally = fullactive; lineno =
 * fullactive = 0; return finaltally; }
 * 
 * if ((!HAS_PERM(PERM_SYSOP) && !HAS_PERM(PERM_SEECLOAK) && uentp->invisible)
 * || ((friend_stat(currutmp, uentp) & HRM) && !HAS_PERM(PERM_SYSOP))){ if
 * (lineno >= b_lines) return 0; if (num++ < page) return 0; memset(uentry, '
 * ', 25); uentry[25] = '\0'; } else{ fullactive++; if (lineno >= b_lines){
 * moreactive = 1; return 0; } if (num++ < page) return 0;
 * 
 * state = (currutmp == uentp) ? 10 :
 * (friend_stat(currutmp,uentp)&ST_FRIEND)>>2;
 * 
 * if (PERM_HIDE(uentp)) state = 9;
 * 
 * sprintf(uentry, "%s%-13s%c%-10s%s ", fcolor[state], uentp->userid,
 * uentp->invisible ? '#' : ' ', modestring(uentp, 1), state ? "\033[0m" :
 * ""); } if (++linecnt < 3){ strcat(uentry, "│"); outs(uentry); } else{
 * outs(uentry); linecnt = 0; clrtoeol(); move(++lineno, 0); } return 0; }
 */
