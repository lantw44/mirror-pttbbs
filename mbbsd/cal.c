/* $Id$ */
#include "bbs.h"

/* 防堵 Multi play */
static int
count_multiplay(int unmode)
{
    register int    i, j;
    register userinfo_t *uentp;

    for (i = j = 0; i < USHM_SIZE; i++) { // XXX linear search
	uentp = &(SHM->uinfo[i]);
	if (uentp->uid == usernum)
	    if (uentp->lockmode == unmode)
		j++;
    }
    return j;
}

int
lockutmpmode(int unmode, int state)
{
    int             errorno = 0;

    if (currutmp->lockmode)
	errorno = 1;
    else if (count_multiplay(unmode))
	errorno = 2;

    if (errorno && !(state == LOCK_THIS && errorno == LOCK_MULTI)) {
	clear();
	move(10, 20);
	if (errorno == 1)
	    prints(gettext[532],
		   gettext[ModeTypeTable[currutmp->lockmode]],
		   gettext[ModeTypeTable[unmode]]);
	else
	    prints(gettext[533],
		   gettext[ModeTypeTable[unmode]]);
	pressanykey();
	return errorno;
    }
    setutmpmode(unmode);
    currutmp->lockmode = unmode;
    return 0;
}

int
unlockutmpmode()
{
    currutmp->lockmode = 0;
    return 0;
}

/* 使用錢的函數 */
#define VICE_NEW   "vice.new"

/* Heat:發票 */
int
vice(int money, char *item)
{
   char            buf[128];
   unsigned int viceserial = (currutmp->lastact % 1000000) * 100 + rand() % 100;

    demoney(-money);
    setuserfile(buf, VICE_NEW);
    log_file(buf, LOG_CREAT | LOG_VF, "%8.8d\n", viceserial);
    snprintf(buf, sizeof(buf),
	     gettext[534], item, money, viceserial);
    mail_id(cuser.userid, buf, "etc/vice.txt", gettext[535]);
    return 0;
}

#define lockreturn(unmode, state) if(lockutmpmode(unmode, state)) return
#define lockreturn0(unmode, state) if(lockutmpmode(unmode, state)) return 0
#define lockbreak(unmode, state) if(lockutmpmode(unmode, state)) break
#define SONGBOOK  "etc/SONGBOOK"
#define OSONGPATH "etc/SONGO"

static int
osong(char *defaultid)
{
    char            destid[IDLEN + 1], buf[200], genbuf[200], filename[256],
                    say[51];
    char            receiver[45], ano[2];
    FILE           *fp, *fp1;
    //*fp2;
    fileheader_t    mail;
    int             nsongs;

    strlcpy(buf, Cdatedate(&now), sizeof(buf));

    lockreturn0(OSONG, LOCK_MULTI);

    /* Jaky 一人一天點一首 */
    if (!strcmp(buf, Cdatedate(&cuser.lastsong)) && !HAS_PERM(PERM_SYSOP)) {
	move(22, 0);
	vmsg(gettext[536]);
	unlockutmpmode();
	return 0;
    }
    if (cuser.money < 200) {
	move(22, 0);
	vmsg(gettext[537]);
	unlockutmpmode();
	return 0;
    }
    move(12, 0);
    clrtobot();
    prints(gettext[538], cuser.userid);
    trans_buffer[0] = 0;
    if (!defaultid) {
	getdata(13, 0, gettext[539],
		destid, sizeof(destid), DOECHO);
	while (!destid[0]) {
	    a_menu(gettext[540], SONGBOOK, 0);
	    clear();
	    getdata(13, 0, gettext[541],
		    destid, sizeof(destid), DOECHO);
	}
    } else
	strlcpy(destid, defaultid, sizeof(destid));

    /* Heat:點歌者匿名功能 */
    getdata(14, 0, gettext[542], ano, sizeof(ano), DOECHO);

    if (!destid[0]) {
	unlockutmpmode();
	return 0;
    }
    getdata_str(14, 0, gettext[543], say,
		sizeof(say), DOECHO, gettext[544]);
    snprintf(save_title, sizeof(save_title),
	     "%s:%s", (ano[0] == 'y') ? gettext[545] : cuser.userid, say);
    getdata_str(16, 0, gettext[546],
		receiver, sizeof(receiver), LCECHO, destid);

    if (!trans_buffer[0]) {
	outs(gettext[547]);
	pressanykey();
	a_menu(gettext[548], SONGBOOK, 0);
    }
    if (!trans_buffer[0] || strstr(trans_buffer, "home") ||
	strstr(trans_buffer, "boards") || !(fp = fopen(trans_buffer, "r"))) {
	unlockutmpmode();
	return 0;
    }
    strlcpy(filename, OSONGPATH, sizeof(filename));

    stampfile(filename, &mail);

    unlink(filename);

    if (!(fp1 = fopen(filename, "w"))) {
	fclose(fp);
	unlockutmpmode();
	return 0;
    }
    strlcpy(mail.owner, gettext[549], sizeof(mail.owner));
    snprintf(mail.title, sizeof(mail.title),
	     gettext[550],
	     (ano[0] == 'y') ? gettext[551] : cuser.userid, destid);

    while (fgets(buf, sizeof(buf), fp)) {
	char           *po;
	if (!strncmp(buf, gettext[552], 6)) {
	    clear();
	    move(10, 10);
	    outs(buf);
	    pressanykey();
	    fclose(fp);
	    fclose(fp1);
	    unlockutmpmode();
	    return 0;
	}
	while ((po = strstr(buf, "<~Src~>"))) {
	    po[0] = 0;
	    snprintf(genbuf, sizeof(genbuf),
		     "%s%s%s", buf,
		     (ano[0] == 'y') ? gettext[553] : cuser.userid, po + 7);
	    strlcpy(buf, genbuf, sizeof(buf));
	}
	while ((po = strstr(buf, "<~Des~>"))) {
	    po[0] = 0;
	    snprintf(genbuf, sizeof(genbuf), "%s%s%s", buf, destid, po + 7);
	    strlcpy(buf, genbuf, sizeof(buf));
	}
	while ((po = strstr(buf, "<~Say~>"))) {
	    po[0] = 0;
	    snprintf(genbuf, sizeof(genbuf), "%s%s%s", buf, say, po + 7);
	    strlcpy(buf, genbuf, sizeof(buf));
	}
	fputs(buf, fp1);
    }
    fclose(fp1);
    fclose(fp);


    if (append_record(OSONGPATH "/.DIR", &mail, sizeof(mail)) != -1) {
	cuser.lastsong = now;
	/* Jaky 超過 500 首歌就開始砍 */
	nsongs = get_num_records(OSONGPATH "/.DIR", sizeof(mail));
	if (nsongs > 500) {
	    delete_range(OSONGPATH "/.DIR", 1, nsongs - 500);
	}
	/* 把第一首拿掉 */
	vice(200, gettext[554]);
    }
    snprintf(save_title, sizeof(save_title),
	     "%s:%s", (ano[0] == 'y') ? gettext[555] : cuser.userid, say);
    hold_mail(filename, destid);

    if (receiver[0]) {
#ifndef USE_BSMTP
	bbs_sendmail(filename, save_title, receiver);
#else
	bsmtp(filename, save_title, receiver, 0);
#endif
    }
    clear();
    outs(
	 gettext[556]);
    pressanykey();
    sortsong();
    topsong();

    unlockutmpmode();
    return 1;
}

int
ordersong()
{
    osong(NULL);
    return 0;
}

static int
inmailbox(int m)
{
    passwd_query(usernum, &xuser);
    cuser.exmailbox = xuser.exmailbox + m;
    passwd_update(usernum, &cuser);
    return cuser.exmailbox;
}


#if !HAVE_FREECLOAK
/* 花錢選單 */
int
p_cloak()
{
    if (getans(currutmp->invisible ? gettext[557] : gettext[558]) != 'y')
	return 0;
    if (cuser.money >= 19) {
	vice(19, gettext[559]);
	currutmp->invisible %= 2;
	vmsg((currutmp->invisible ^= 1) ? MSG_CLOAKED : MSG_UNCLOAK);
    }
    return 0;
}
#endif

int
p_from()
{
    if (getans(gettext[560]) != 'y')
	return 0;
    reload_money();
    if (cuser.money < 49)
	return 0;
    if (getdata_buf(b_lines - 1, 0, gettext[561],
		    currutmp->from, sizeof(currutmp->from), DOECHO)) {
	vice(49, gettext[562]);
	currutmp->from_alias = 0;
    }
    return 0;
}

int
p_exmail()
{
    char            ans[4], buf[200];
    int             n;

    if (cuser.exmailbox >= MAX_EXKEEPMAIL) {
	vmsg(gettext[563], MAX_EXKEEPMAIL);
	return 0;
    }
    snprintf(buf, sizeof(buf),
	     gettext[564], cuser.exmailbox);

    getdata_str(b_lines - 2, 0, buf, ans, sizeof(ans), LCECHO, "10");

    n = atoi(ans);
    if (!ans[0] || !n)
	return 0;
    if (n < 0)
	n = 100;
    if (n + cuser.exmailbox > MAX_EXKEEPMAIL)
	n = MAX_EXKEEPMAIL - cuser.exmailbox;
    reload_money();
    if (cuser.money < n * 1000)
	return 0;
    vice(n * 1000, gettext[565]);
    inmailbox(n);
    return 0;
}

void
mail_redenvelop(char *from, char *to, int money, char mode)
{
    char            genbuf[200];
    fileheader_t    fhdr;
    FILE           *fp;
    snprintf(genbuf, sizeof(genbuf), "home/%c/%s", to[0], to);
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
	return;
    fprintf(fp, gettext[566]
	    ,from, ctime(&now), to, money);
    fclose(fp);
    snprintf(fhdr.title, sizeof(fhdr.title), gettext[567]);
    strlcpy(fhdr.owner, from, sizeof(fhdr.owner));

    if (mode == 'y')
	vedit(genbuf, NA, NULL);
    snprintf(genbuf, sizeof(genbuf), "home/%c/%s/.DIR", to[0], to);
    append_record(genbuf, &fhdr, sizeof(fhdr));
}

/* 計算贈與稅 */
int
give_tax(int money)
{
    int             i, tax = 0;
    int      tax_bound[] = {1000000, 100000, 10000, 1000, 0};
    double   tax_rate[] = {0.4, 0.3, 0.2, 0.1, 0.08};
    for (i = 0; i <= 4; i++)
	if (money > tax_bound[i]) {
	    tax += (money - tax_bound[i]) * tax_rate[i];
	    money -= (money - tax_bound[i]);
	}
    return (tax <= 0) ? 1 : tax;
}

int
p_give()
{
    int             money, tax;
    char            id[IDLEN + 1], money_buf[20];

    move(1, 0);
    usercomplete(gettext[568], id);
    if (!id[0] || !strcmp(cuser.userid, id) ||
	!getdata(2, 0, gettext[569], money_buf, 7, LCECHO))
	return 0;
    money = atoi(money_buf);
    reload_money();
    if (money > 0 && cuser.money >= money) {
	tax = give_tax(money);
	if (money - tax <= 0)
	    return 0;		/* 繳完稅就沒錢給了 */
	deumoney(searchuser(id), money - tax);
	demoney(-money);
	log_file(FN_MONEY,LOG_CREAT | LOG_VF, gettext[570],
                 cuser.userid, id, money - tax, ctime(&now));
	mail_redenvelop(cuser.userid, id, money - tax, getans(gettext[571]));
    }
    return 0;
}

int
p_sysinfo(void)
{
    char            *cpuloadstr;
    int             load;
    extern char    *compile_time;

    load = cpuload(NULL);
    cpuloadstr = (load < 5 ? gettext[572] : (load < 20 ? gettext[573] : gettext[574]));

    clear();
    showtitle(gettext[575], BBSNAME);
    move(2, 0);
    prints(gettext[576]);
    prints(TITLE_COLOR BBSNAME);
    prints("\033[m (" MYIP);
    prints(gettext[577],
	   cpuloadstr, SHM->UTMPnumber,
#ifdef DYMAX_ACTIVE
	   GLOBALVAR[9] > 1000 ? GLOBALVAR[9] : MAX_ACTIVE,
#else
	   MAX_ACTIVE,
#endif
	   compile_time, ctime(&start_time));
    if (HAS_PERM(PERM_SYSOP)) {
	struct rusage ru;
	getrusage(RUSAGE_SELF, &ru);
	prints(gettext[578],
	       ((int)sbrk(0) - 0x8048000) / 1024,
	       (int)ru.ru_idrss, (int)ru.ru_isrss);
	prints(gettext[579]
#ifdef CRITICAL_MEMORY
		" CRITICAL_MEMORY"
#endif
#ifdef OUTTACACHE
		" OUTTACACHE"
#endif
		);
    }
    pressanykey();
    return 0;
}

