/* $Id$ */
#include "bbs.h"

#ifndef _BBS_UTIL_C_
#define MAX_ITEM	8	//最大 賭項(item) 個數
#define MAX_ITEM_LEN	30	//最大 每一賭項名字長度
#define MAX_SUBJECT_LEN 650	//8*81 = 648 最大 主題長度

static int      currbid;

static int
load_ticket_record(char *direct, int ticket[])
{
    char            buf[256];
    int             i, total = 0;
    FILE           *fp;
    snprintf(buf, sizeof(buf), "%s/" FN_TICKET_RECORD, direct);
    if (!(fp = fopen(buf, "r")))
	return 0;
    for (i = 0; i < MAX_ITEM && fscanf(fp, "%9d ", &ticket[i])==1; i++)
	total = total + ticket[i];
    fclose(fp);
    return total;
}

static int
show_ticket_data(char betname[MAX_ITEM][MAX_ITEM_LEN],char *direct, int *price, boardheader_t * bh)
{
    int             i, count, total = 0, end = 0, ticket[MAX_ITEM] = {0, 0, 0, 0, 0, 0, 0, 0};
    FILE           *fp;
    char            genbuf[256], t[25];

    clear();
    if (bh) {
	snprintf(genbuf, sizeof(genbuf), gettext[1061], bh->brdname);
	if (bh->endgamble && now < bh->endgamble &&
	    bh->endgamble - now < 3600) {
	    snprintf(t, sizeof(t),
		     gettext[1062], (int)(bh->endgamble - now));
	    showtitle(genbuf, t);
	} else
	    showtitle(genbuf, BBSNAME);
    } else
	showtitle(gettext[1063], BBSNAME);
    move(2, 0);
    snprintf(genbuf, sizeof(genbuf), "%s/" FN_TICKET_ITEMS, direct);
    if (!(fp = fopen(genbuf, "r"))) {
	prints(gettext[1064]);
	snprintf(genbuf, sizeof(genbuf), "%s/" FN_TICKET_OUTCOME, direct);
	more(genbuf, NA);
	return 0;
    }
    fgets(genbuf, MAX_ITEM_LEN, fp);
    *price = atoi(genbuf);
    for (count = 0; fgets(betname[count], MAX_ITEM_LEN, fp) && count < MAX_ITEM; count++)
	strtok(betname[count], "\r\n");
    fclose(fp);

    prints(gettext[1065], *price,
	   bh ? gettext[1066] :
	        gettext[1067],
	   bh ? gettext[1068] : "",
	   bh ? gettext[1069] : gettext[1070]);


    snprintf(genbuf, sizeof(genbuf), "%s/" FN_TICKET, direct);
    if (!dashf(genbuf)) {
	snprintf(genbuf, sizeof(genbuf), "%s/" FN_TICKET_END, direct);
	end = 1;
    }
    show_file(genbuf, 8, -1, NO_RELOAD);
    move(15, 0);
    prints(gettext[1071]);

    total = load_ticket_record(direct, ticket);

    prints("\033[33m");
    for (i = 0; i < count; i++) {
	prints("%d.%-8s: %-7d", i + 1, betname[i], ticket[i]);
	if (i == 3)
	    prints("\n");
    }
    prints(gettext[1072], total * (*price));
    if (end) {
	prints(gettext[1073]);
	return -count;
    }
    return count;
}

static void
append_ticket_record(char *direct, int ch, int n, int count)
{
    FILE           *fp;
    int             ticket[8] = {0, 0, 0, 0, 0, 0, 0, 0}, i;
    char            genbuf[256];
    snprintf(genbuf, sizeof(genbuf), "%s/" FN_TICKET_USER, direct);

    if ((fp = fopen(genbuf, "a"))) {
	fprintf(fp, "%s %d %d\n", cuser.userid, ch, n);
	fclose(fp);
    }
    load_ticket_record(direct, ticket);
    ticket[ch] += n;
    snprintf(genbuf, sizeof(genbuf), "%s/" FN_TICKET_RECORD, direct);
    if ((fp = fopen(genbuf, "w"))) {
	for (i = 0; i < count; i++)
	    fprintf(fp, "%d ", ticket[i]);
	fclose(fp);
    }
}

#define lockreturn0(unmode, state) if(lockutmpmode(unmode, state)) return 0
int
ticket(int bid)
{
    int             ch, n, price, count, end = 0;
    char            path[128], fn_ticket[128];
    char            betname[MAX_ITEM][MAX_ITEM_LEN];
    boardheader_t  *bh = NULL;

    if (bid) {
	bh = getbcache(bid);
	setbpath(path, bh->brdname);
	setbfile(fn_ticket, bh->brdname, FN_TICKET);
	currbid = bid;
    } else
	strcpy(path, "etc/");

    lockreturn0(TICKET, LOCK_MULTI);
    while (1) {
	count = show_ticket_data(betname, path, &price, bh);
	if (count <= 0) {
	    pressanykey();
	    break;
	}
	move(20, 0);
	reload_money();
	prints(gettext[1074], cuser.money, count);
	ch = igetch();
	/*--
	  Tim011127
	  為了控制CS問題 但是這邊還不能完全解決這問題,
	  若user通過檢查下去, 剛好板主開獎, 還是會造成user的這次紀錄
	  很有可能跑到下次賭盤的紀錄去, 也很有可能被板主新開賭盤時洗掉
	  不過這邊至少可以做到的是, 頂多只會有一筆資料是錯的
	--*/
	if (ch == 'q' || ch == 'Q')
	    break;
	ch -= '1';
	if (end || ch >= count || ch < 0)
	    continue;
	n = 0;
	ch_buyitem(price, "etc/buyticket", &n, 0);

	if (bid && !dashf(fn_ticket)) {
	    vmsg(gettext[1075]);
	    break;
	}

	if (n > 0)
	    append_ticket_record(path, ch, n, count);
    }
    unlockutmpmode();
    return 0;
}

int
openticket(int bid)
{
    char            path[128], buf[256], outcome[128];
    int             i, money = 0, count, bet, price, total = 0, ticket[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    boardheader_t  *bh = getbcache(bid);
    FILE           *fp, *fp1;
    char            betname[MAX_ITEM][MAX_ITEM_LEN];

    setbpath(path, bh->brdname);
    count = -show_ticket_data(betname, path, &price, bh);
    if (count == 0) {
	setbfile(buf, bh->brdname, FN_TICKET_END);
	unlink(buf);
//Ptt:	有bug
	    return 0;
    }
    lockreturn0(TICKET, LOCK_MULTI);
    do {
	do {
	    getdata(20, 0,
		    gettext[1076], buf, 3, LCECHO);
	    bet = atoi(buf);
	    move(0, 0);
	    clrtoeol();
	} while ((bet < 0 || bet > count) && bet != 99);
	if (bet == 0) {
	    unlockutmpmode();
	    return 0;
	}
	getdata(21, 0, gettext[1077], buf, 3, LCECHO);
    } while (bet != atoi(buf));

    if (fork()) {
	/* Ptt: 用 fork() 防止不正常斷線洗錢 */
	move(22, 0);
	prints(gettext[1078]);
	pressanykey();
	unlockutmpmode();
	return 0;
    }
    close(0);
    close(1);
    setproctitle("open ticket");
#ifdef CPULIMIT
    {
	struct rlimit   rml;
	rml.rlim_cur = RLIM_INFINITY;
	rml.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CPU, &rml);
    }
#endif


    bet--;			/* 轉成矩陣的index */

    total = load_ticket_record(path, ticket);
    setbfile(buf, bh->brdname, FN_TICKET_END);
    if (!(fp1 = fopen(buf, "r")))
	exit(1);

    /* 還沒開完獎不能賭博 只要mv一項就好 */
    if (bet != 98) {
	money = total * price;
	demoney(money * 0.02);
	mail_redenvelop(gettext[1079], cuser.userid, money * 0.02, 'n');
	money = ticket[bet] ? money * 0.95 / ticket[bet] : 9999999;
    } else {
	vice(price * 10, gettext[1080]);
	money = price;
    }
    setbfile(outcome, bh->brdname, FN_TICKET_OUTCOME);
    if ((fp = fopen(outcome, "w"))) {
	fprintf(fp, gettext[1081]);
	while (fgets(buf, sizeof(buf), fp1)) {
	    buf[sizeof(buf)-1] = 0;
	    fprintf(fp, "%s", buf);
	}
	fprintf(fp, gettext[1082]);

	fprintf(fp, "\033[33m");
	for (i = 0; i < count; i++) {
	    fprintf(fp, "%d.%-8s: %-7d", i + 1, betname[i], ticket[i]);
	    if (i == 3)
		fprintf(fp, "\n");
	}
	fprintf(fp, "\033[m\n");

	if (bet != 98) {
	    fprintf(fp, gettext[1083],
	    Cdatelite(&now), betname[bet], total * price, ticket[bet], total,
		    (float)ticket[bet] / total, money);

	    fprintf(fp, gettext[1084],
		    Cdatelite(&now), betname[bet], total * price, money,
		    total ? (float)ticket[bet] / total : 0);
	} else
	    fprintf(fp, gettext[1085], Cdatelite(&now));

    } // XXX somebody may use fp even fp==NULL
    fclose(fp1);

    setbfile(buf, bh->brdname, FN_TICKET_END);
    setbfile(path, bh->brdname, FN_TICKET_LOCK);
    rename(buf, path);
    /*
     * 以下是給錢動作
     */
    setbfile(buf, bh->brdname, FN_TICKET_USER);
    if ((bet == 98 || ticket[bet]) && (fp1 = fopen(buf, "r"))) {
	int             mybet, uid;
	char            userid[IDLEN + 1];

	while (fscanf(fp1, "%s %d %d\n", userid, &mybet, &i) != EOF) {
	    if (bet == 98 && mybet >= 0 && mybet < count) {
		if (fp)
		    fprintf(fp, gettext[1086]
			    ,userid, i, betname[mybet], money * i);
		snprintf(buf, sizeof(buf),
			 gettext[1087], bh->brdname, money * i);
	    } else if (mybet == bet) {
		if (fp)
		    fprintf(fp, gettext[1088]
			    ,userid, i, betname[mybet], money * i);
		snprintf(buf, sizeof(buf), gettext[1089], bh->brdname, money * i);
	    } else
		continue;
	    if ((uid = searchuser(userid)) == 0)
		continue;
	    deumoney(uid, money * i);
	    mail_id(userid, buf, "etc/ticket.win", gettext[1090]);
	}
	fclose(fp1);
    }
    if (fp)
      {
        fprintf(fp, "%s"BBSNAME"("MYHOSTNAME"%s%s\n", gettext[1091],
                gettext[1092], fromhost);
	fclose(fp);
      }

    if (bet != 98)
	snprintf(buf, sizeof(buf), gettext[1093], bh->brdname);
    else
	snprintf(buf, sizeof(buf), gettext[1094], bh->brdname);
    post_file(bh->brdname, buf, outcome, gettext[1095]);
    post_file("Record", buf + 7, outcome, gettext[1096]);

    setbfile(buf, bh->brdname, FN_TICKET_RECORD);
    unlink(buf);
    setbfile(buf, bh->brdname, FN_TICKET_USER);
    unlink(buf);
    setbfile(buf, bh->brdname, FN_TICKET_LOCK);
    unlink(buf);
    exit(1);
    return 0;
}

int
ticket_main()
{
    ticket(0);
    return 0;
}
#endif
