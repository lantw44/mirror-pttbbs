/* $Id$ */
#include "bbs.h"
static char            currmaildir[32];
static char     listfile[] = "list.0";
static int      mailkeep = 0, mailsum = 0;
static int      mailsumlimit = 0, mailmaxkeep = 0;

int
setforward()
{
    char            buf[80], ip[50] = "", yn[4];
    FILE           *fp;

    sethomepath(buf, cuser.userid);
    strcat(buf, "/.forward");
    if ((fp = fopen(buf, "r"))) {
	fscanf(fp, "%s", ip); // XXX check buffer size
	fclose(fp);
    }
    getdata_buf(b_lines - 1, 0, SHM->i18nstr[cuser.language][1202],
		ip, sizeof(ip), DOECHO);
    if (ip[0] && ip[0] != ' ') {
	getdata(b_lines, 0, SHM->i18nstr[cuser.language][1203], yn, sizeof(yn),
		LCECHO);
	if (yn[0] != 'n' && (fp = fopen(buf, "w"))) {
	    fprintf(fp, "%s", ip);
	    fclose(fp);
	    vmsg(SHM->i18nstr[cuser.language][1204]);
	    return 0;
	}
    }
    unlink(buf);
    vmsg(SHM->i18nstr[cuser.language][1205]);
    return 0;
}

int
built_mail_index()
{
    char            genbuf[128];

    move(b_lines - 4, 0);
    outs(SHM->i18nstr[cuser.language][1206]);
    getdata(b_lines - 1, 0,
	    SHM->i18nstr[cuser.language][1207], genbuf, 3,
	    LCECHO);
    if (genbuf[0] != 'y')
	return 0;

    snprintf(genbuf, sizeof(genbuf),
	     BBSHOME "/bin/buildir " BBSHOME "/home/%c/%s",
	     cuser.userid[0], cuser.userid);
    move(22, 0);
    prints(SHM->i18nstr[cuser.language][1208]);
    pressanykey();
    system(genbuf);
    return 0;
}

int
mailalert(char *userid)
{
    userinfo_t     *uentp = NULL;
    int             n, tuid, i;

    if ((tuid = searchuser(userid)) == 0)
	return -1;

    n = count_logins(tuid, 0);
    for (i = 1; i <= n; i++)
	if ((uentp = (userinfo_t *) search_ulistn(tuid, i)))
	    uentp->mailalert = 1;
    return 0;
}

int
mail_muser(userec_t muser, char *title, char *filename)
{
    return mail_id(muser.userid, title, filename, cuser.userid);
}

/* Heat: 用id來寄信,內容則link準備好的檔案 */
int
mail_id(char *id, char *title, char *filename, char *owner)
{
    fileheader_t    mhdr;
    char            genbuf[128];
    sethomepath(genbuf, id);
    if (stampfile(genbuf, &mhdr))
	return 0;
    strlcpy(mhdr.owner, owner, sizeof(mhdr.owner));
    strncpy(mhdr.title, title, TTLEN);
    mhdr.filemode = 0;
    Link(filename, genbuf);
    sethomedir(genbuf, id);
    append_record_forward(genbuf, &mhdr, sizeof(mhdr));
    mailalert(id);
    return 0;
}

int
invalidaddr(char *addr)
{
    if (*addr == '\0')
	return 1;		/* blank */
    while (*addr) {
	if (not_alnum(*addr) && !strchr("[].@-_", *addr))
	    return 1;
	addr++;
    }
    return 0;
}

int
m_internet()
{
    char            receiver[60];

    getdata(20, 0, SHM->i18nstr[cuser.language][1209], receiver, sizeof(receiver), DOECHO);
    if (strchr(receiver, '@') && !invalidaddr(receiver) &&
	getdata(21, 0, SHM->i18nstr[cuser.language][1210], save_title, STRLEN, DOECHO))
	do_send(receiver, save_title);
    else {
	move(22, 0);
	outs(SHM->i18nstr[cuser.language][1211]);
	pressanykey();
    }
    return 0;
}

void
m_init()
{
    sethomedir(currmaildir, cuser.userid);
}

int
chkmailbox()
{
    if (!HAVE_PERM(PERM_SYSOP) && !HAVE_PERM(PERM_MAILLIMIT)) {
	int             max_keepmail = MAX_KEEPMAIL;
	if (HAS_PERM(PERM_SYSSUBOP) || HAS_PERM(PERM_SMG) ||
	    HAS_PERM(PERM_PRG) || HAS_PERM(PERM_ACTION) || HAS_PERM(PERM_PAINT)) {
	    mailsumlimit = 700;
	    max_keepmail = 500;
	} else if (HAS_PERM(PERM_BM)) {
	    mailsumlimit = 500;
	    max_keepmail = 300;
	} else if (HAS_PERM(PERM_LOGINOK))
	    mailsumlimit = 200;
	else
	    mailsumlimit = 50;
	mailsumlimit += cuser.exmailbox * 10;
	mailmaxkeep = max_keepmail + cuser.exmailbox;
	m_init();
	if ((mailkeep = get_num_records(currmaildir, sizeof(fileheader_t))) >
		mailmaxkeep ||
		(mailsum = get_sum_records(currmaildir, sizeof(fileheader_t))) >
			mailsumlimit) {
	    bell();
	    bell();
	    vmsg(SHM->i18nstr[cuser.language][1212], mailkeep, mailmaxkeep);
	    return mailkeep;
	}
    }
    return 0;
}

static void
do_hold_mail(char *fpath, char *receiver, char *holder)
{
    char            buf[80], title[128];

    fileheader_t    mymail;

    sethomepath(buf, holder);
    stampfile(buf, &mymail);

    mymail.filemode = FILE_READ ;
    strlcpy(mymail.owner, SHM->i18nstr[cuser.language][1214], sizeof(mymail.owner));
    if (receiver) {
	snprintf(title, sizeof(title), "(%s) %s", receiver, save_title);
	strncpy(mymail.title, title, TTLEN);
    } else
	strlcpy(mymail.title, save_title, sizeof(mymail.title));

    sethomedir(title, holder);

    unlink(buf);
    Link(fpath, buf);
    append_record_forward(title, &mymail, sizeof(mymail));
}

void
hold_mail(char *fpath, char *receiver)
{
    char            buf[4];

    getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][1215],
	    buf, sizeof(buf), LCECHO);

    if (buf[0] == 'y')
	do_hold_mail(fpath, receiver, cuser.userid);
}

int
do_send(char *userid, char *title)
{
    fileheader_t    mhdr;
    char            fpath[STRLEN];
    char            receiver[IDLEN + 1];
    char            genbuf[200];
    int             internet_mail, i;

    if (strchr(userid, '@'))
	internet_mail = 1;
    else {
	internet_mail = 0;
	if (!getuser(userid))
	    return -1;
	if (!(xuser.userlevel & PERM_READMAIL))
	    return -3;

	if (!title)
	    getdata(2, 0, SHM->i18nstr[cuser.language][1216], save_title, STRLEN - 20, DOECHO);
	curredit |= EDIT_MAIL;
	curredit &= ~EDIT_ITEM;
    }

    setutmpmode(SMAIL);

    fpath[0] = '\0';

    if (internet_mail) {
	int             res, ch;

	if (vedit(fpath, NA, NULL) == -1) {
	    unlink(fpath);
	    clear();
	    return -2;
	}
	clear();
	prints(SHM->i18nstr[cuser.language][1217],
	       userid, title);
	ch = igetch();
	switch (ch) {
	case 'N':
	case 'n':
	    outs(SHM->i18nstr[cuser.language][1218]);
	    res = -2;
	    break;
	default:
	    outs(SHM->i18nstr[cuser.language][1219]);
	    res =
#ifndef USE_BSMTP
		bbs_sendmail(fpath, title, userid);
#else
		bsmtp(fpath, title, userid, 0);
#endif
	    hold_mail(fpath, userid);
	}
	unlink(fpath);
	return res;
    } else {
	strlcpy(receiver, userid, sizeof(receiver));
	sethomepath(genbuf, userid);
	stampfile(genbuf, &mhdr);
	strlcpy(mhdr.owner, cuser.userid, sizeof(mhdr.owner));
	strncpy(mhdr.title, save_title, TTLEN);
	if (vedit(genbuf, YEA, NULL) == -1) {
	    unlink(genbuf);
	    clear();
	    return -2;
	}
	clear();
	sethomefile(fpath, userid, FN_OVERRIDES);
	i = belong(fpath, cuser.userid);
	sethomefile(fpath, userid, FN_REJECT);

	if (i || !belong(fpath, cuser.userid)) {/* Ptt: 用belong有點討厭 */
	    sethomedir(fpath, userid);
	    if (append_record_forward(fpath, &mhdr, sizeof(mhdr)) == -1)
		return -1;
	    mailalert(userid);
	}
	hold_mail(genbuf, userid);
	return 0;
    }
}

void
my_send(char *uident)
{
    switch (do_send(uident, NULL)) {
	case -1:
	outs(err_uid);
	break;
    case -2:
	outs(msg_cancel);
	break;
    case -3:
	prints(SHM->i18nstr[cuser.language][1220], uident);
	break;
    }
    pressanykey();
}

int
m_send()
{
    char            uident[40];

    stand_title(SHM->i18nstr[cuser.language][1221]);
    usercomplete(msg_uid, uident);
    showplans(uident);
    if (uident[0])
	my_send(uident);
    return 0;
}

/* 群組寄信、回信 : multi_send, multi_reply */
static void
multi_list(int *reciper)
{
    char            uid[16];
    char            genbuf[200];

    while (1) {
	stand_title(SHM->i18nstr[cuser.language][1222]);
	ShowNameList(3, 0, SHM->i18nstr[cuser.language][1201]);
	move(1, 0);
	prints(SHM->i18nstr[cuser.language][1223]);
	getdata(2, 0,
	       SHM->i18nstr[cuser.language][1224],
		genbuf, 4, LCECHO);
	switch (genbuf[0]) {
	case 'a':
	    while (1) {
		move(1, 0);
		usercomplete(SHM->i18nstr[cuser.language][1225], uid);
		if (uid[0] == '\0')
		    break;

		move(2, 0);
		clrtoeol();

		if (!searchuser(uid))
		    outs(err_uid);
		else if (!InNameList(uid)) {
		    AddNameList(uid);
		    (*reciper)++;
		}
		ShowNameList(3, 0, SHM->i18nstr[cuser.language][1201]);
	    }
	    break;
	case 'd':
	    while (*reciper) {
		move(1, 0);
		namecomplete(SHM->i18nstr[cuser.language][1226], uid);
		if (uid[0] == '\0')
		    break;
		if (RemoveNameList(uid))
		    (*reciper)--;
		ShowNameList(3, 0, SHM->i18nstr[cuser.language][1201]);
	    }
	    break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	    listfile[5] = genbuf[0];
	    genbuf[0] = '1';
	case 'i':
	    setuserfile(genbuf, genbuf[0] == '1' ? listfile : fn_overrides);
	    ToggleNameList(reciper, genbuf, SHM->i18nstr[cuser.language][1201]);
	    break;
	case 'o':
	    setuserfile(genbuf, "alohaed");
	    ToggleNameList(reciper, genbuf, SHM->i18nstr[cuser.language][1201]);
	    break;
	case 'n':
	    setuserfile(genbuf, "postlist");
	    ToggleNameList(reciper, genbuf, SHM->i18nstr[cuser.language][1201]);
	    break;
	case 'q':
	    *reciper = 0;
	    return;
	default:
	    return;
	}
    }
}

static void
multi_send(char *title)
{
    FILE           *fp;
    struct word_t  *p = NULL;
    fileheader_t    mymail;
    char            fpath[TTLEN], *ptr;
    int             reciper, listing;
    char            genbuf[256];

    CreateNameList();
    listing = reciper = 0;
    if (*quote_file) {
	AddNameList(quote_user);
	reciper = 1;
	fp = fopen(quote_file, "r");
	assert(fp);
	while (fgets(genbuf, 256, fp)) {
	    if (strncmp(genbuf, SHM->i18nstr[cuser.language][1227], 3)) {
		if (listing)
		    break;
	    } else {
		if (listing) {
		    strtok(ptr = genbuf + 3, " \n\r");
		    do {
			if (searchuser(ptr) && !InNameList(ptr) &&
			    strcmp(cuser.userid, ptr)) {
			    AddNameList(ptr);
			    reciper++;
			}
		    } while ((ptr = (char *)strtok(NULL, " \n\r")));
		} else if (!strncmp(genbuf + 3, SHM->i18nstr[cuser.language][1228], 6))
		    listing = 1;
	    }
	}
	fclose(fp);
	ShowNameList(3, 0, SHM->i18nstr[cuser.language][1201]);
    }
    multi_list(&reciper);
    move(1, 0);
    clrtobot();

    if (reciper) {
	setutmpmode(SMAIL);
	if (title)
	    do_reply_title(2, title);
	else {
	    getdata(2, 0, SHM->i18nstr[cuser.language][1229], fpath, sizeof(fpath), DOECHO);
	    snprintf(save_title, sizeof(save_title), SHM->i18nstr[cuser.language][1230], fpath);
	}

	setuserfile(fpath, fn_notes);

	if ((fp = fopen(fpath, "w"))) {
	    fprintf(fp, SHM->i18nstr[cuser.language][1231], reciper);
	    listing = 80;

	    for (p = toplev; p; p = p->next) {
		reciper = strlen(p->word) + 1;
		if (listing + reciper > 75) {
		    listing = reciper;
		    fprintf(fp, SHM->i18nstr[cuser.language][1232]);
		} else
		    listing += reciper;

		fprintf(fp, " %s", p->word);
	    }
	    memset(genbuf, '-', 75);
	    genbuf[75] = '\0';
	    fprintf(fp, "\n%s\n\n", genbuf);
	    fclose(fp);
	}
	curredit |= EDIT_LIST;

	if (vedit(fpath, YEA, NULL) == -1) {
	    unlink(fpath);
	    curredit = 0;
	    vmsg(msg_cancel);
	    return;
	}
	listing = 80;

	for (p = toplev; p; p = p->next) {
	    reciper = strlen(p->word) + 1;
	    if (listing + reciper > 75) {
		listing = reciper;
		outc('\n');
	    } else {
		listing += reciper;
		outc(' ');
	    }
	    outs(p->word);
	    if (searchuser(p->word) && strcmp(STR_GUEST, p->word))
		sethomepath(genbuf, p->word);
	    else
		continue;
	    stampfile(genbuf, &mymail);
	    unlink(genbuf);
	    Link(fpath, genbuf);

	    strlcpy(mymail.owner, cuser.userid, sizeof(mymail.owner));
	    strlcpy(mymail.title, save_title, sizeof(mymail.title));
	    mymail.filemode |= FILE_MULTI;	/* multi-send flag */
	    sethomedir(genbuf, p->word);
	    if (append_record_forward(genbuf, &mymail, sizeof(mymail)) == -1)
		vmsg(err_uid);
	    mailalert(p->word);
	}
	hold_mail(fpath, NULL);
	unlink(fpath);
	curredit = 0;
    } else
	vmsg(msg_cancel);
}

static int
multi_reply(int ent, fileheader_t * fhdr, char *direct)
{
    if (!(fhdr->filemode & FILE_MULTI))
	return mail_reply(ent, fhdr, direct);

    stand_title(SHM->i18nstr[cuser.language][1234]);
    strlcpy(quote_user, fhdr->owner, sizeof(quote_user));
    setuserfile(quote_file, fhdr->filename);
    multi_send(fhdr->title);
    return 0;
}

int
mail_list()
{
    stand_title(SHM->i18nstr[cuser.language][1235]);
    multi_send(NULL);
    return 0;
}

int
mail_all()
{
    FILE           *fp;
    fileheader_t    mymail;
    char            fpath[TTLEN];
    char            genbuf[200];
    int             i, unum;
    char           *userid;

    stand_title(SHM->i18nstr[cuser.language][1236]);
    setutmpmode(SMAIL);
    getdata(2, 0, SHM->i18nstr[cuser.language][1237], fpath, sizeof(fpath), DOECHO);
    snprintf(save_title, sizeof(save_title),
	     SHM->i18nstr[cuser.language][1238], fpath);

    setuserfile(fpath, fn_notes);

    if ((fp = fopen(fpath, "w"))) {
	fprintf(fp, SHM->i18nstr[cuser.language][1239]);
	fprintf(fp, "---------------------------------------------------------------------------\n");
	fclose(fp);
    }
    *quote_file = 0;

    curredit |= EDIT_MAIL;
    curredit &= ~EDIT_ITEM;
    if (vedit(fpath, YEA, NULL) == -1) {
	curredit = 0;
	unlink(fpath);
	outs(msg_cancel);
	pressanykey();
	return 0;
    }
    curredit = 0;

    setutmpmode(MAILALL);
    stand_title(SHM->i18nstr[cuser.language][1240]);

    sethomepath(genbuf, cuser.userid);
    stampfile(genbuf, &mymail);
    unlink(genbuf);
    Link(fpath, genbuf);
    unlink(fpath);
    strcpy(fpath, genbuf);

    strlcpy(mymail.owner, cuser.userid, sizeof(mymail.owner));	/* 站長 ID */
    strlcpy(mymail.title, save_title, sizeof(mymail.title));

    sethomedir(genbuf, cuser.userid);
    if (append_record_forward(genbuf, &mymail, sizeof(mymail)) == -1)
	outs(err_uid);

    for (unum = SHM->number, i = 0; i < unum; i++) {
	if (bad_user_id(SHM->userid[i]))
	    continue;		/* Ptt */

	userid = SHM->userid[i];
	if (strcmp(userid, STR_GUEST) && strcmp(userid, "new") &&
	    strcmp(userid, cuser.userid)) {
	    sethomepath(genbuf, userid);
	    stampfile(genbuf, &mymail);
	    unlink(genbuf);
	    Link(fpath, genbuf);

	    strlcpy(mymail.owner, cuser.userid, sizeof(mymail.owner));
	    strlcpy(mymail.title, save_title, sizeof(mymail.title));
	    /* mymail.filemode |= FILE_MARKED; Ptt 公告改成不會mark */
	    sethomedir(genbuf, userid);
	    if (append_record_forward(genbuf, &mymail, sizeof(mymail)) == -1)
		outs(err_uid);
	    snprintf(genbuf, sizeof(genbuf),
		     "%*s %5d / %5d", IDLEN + 1, userid, i + 1, unum);
	    vmsg(genbuf);
	}
    }
    return 0;
}

int
mail_mbox()
{
    char            cmd[100];
    fileheader_t    fhdr;

    snprintf(cmd, sizeof(cmd), "/tmp/%s.uu", cuser.userid);
    snprintf(fhdr.title, sizeof(fhdr.title), SHM->i18nstr[cuser.language][1241], cuser.userid);
    doforward(cmd, &fhdr, 'Z');
    return 0;
}

static int
m_forward(int ent, fileheader_t * fhdr, char *direct)
{
    char            uid[STRLEN];

    stand_title(SHM->i18nstr[cuser.language][1242]);
    usercomplete(msg_uid, uid);
    if (uid[0] == '\0')
	return FULLUPDATE;

    strlcpy(quote_user, fhdr->owner, sizeof(quote_user));
    setuserfile(quote_file, fhdr->filename);
    snprintf(save_title, sizeof(save_title), "%.64s (fwd)", fhdr->title);
    move(1, 0);
    clrtobot();
    prints(SHM->i18nstr[cuser.language][1243], uid, save_title);

    switch (do_send(uid, save_title)) {
    case -1:
	outs(err_uid);
	break;
    case -2:
	outs(msg_cancel);
	break;
    case -3:
	prints(SHM->i18nstr[cuser.language][1244], uid);
	break;
    }
    pressanykey();
    return FULLUPDATE;
}

static int      delmsgs[128];
static int      delcnt;
static int      mrd;

static int
read_new_mail(fileheader_t * fptr)
{
    static int      idc;
    char            done = NA, delete_it;
    char            fname[256];
    char            genbuf[4];

    if (fptr == NULL) {
	delcnt = 0;
	idc = 0;
	return 0;
    }
    idc++;
    if (fptr->filemode)
	return 0;
    clear();
    move(10, 0);
    prints(SHM->i18nstr[cuser.language][1245], fptr->owner, fptr->title);
    getdata(11, 0, SHM->i18nstr[cuser.language][1246], genbuf, 3, DOECHO);
    if (genbuf[0] == 'q')
	return QUIT;
    if (genbuf[0] == 'n')
	return 0;

    setuserfile(fname, fptr->filename);
    fptr->filemode |= FILE_READ;
    if (substitute_record(currmaildir, fptr, sizeof(*fptr), idc))
	return -1;

    mrd = 1;
    delete_it = NA;
    while (!done) {
	int             more_result = more(fname, YEA);

	switch (more_result) {
		case 999:
			mail_reply(idc, fptr, currmaildir);
			return FULLUPDATE;
		case -1:
			return READ_SKIP;
		case 0:
			break;
		default:
			return more_result;
	}
	outmsg(msg_mailer);

	switch (igetch()) {
	case 'r':
	case 'R':
	    mail_reply(idc, fptr, currmaildir);
	    break;
	case 'x':
	    m_forward(idc, fptr, currmaildir);
	    break;
	case 'y':
	    multi_reply(idc, fptr, currmaildir);
	    break;
	case 'd':
	case 'D':
	    delete_it = YEA;
	default:
	    done = YEA;
	}
    }
    if (delete_it) {
	clear();
	prints(SHM->i18nstr[cuser.language][1247], fptr->title);
	getdata(1, 0, msg_sure_ny, genbuf, 2, LCECHO);
	if (genbuf[0] == 'y') {
	    unlink(fname);
	    delmsgs[delcnt++] = idc; // FIXME 一次刪太多信 out of array boundary
	}
    }
    clear();
    return 0;
}

int
m_new()
{
    clear();
    mrd = 0;
    setutmpmode(RMAIL);
    read_new_mail(NULL);
    clear();
    curredit |= EDIT_MAIL;
    curredit &= ~EDIT_ITEM;
    if (apply_record(currmaildir, read_new_mail, sizeof(fileheader_t)) == -1) {
	outs(SHM->i18nstr[cuser.language][1248]);
	pressanykey();
	return -1;
    }
    curredit = 0;
    if (delcnt) {
	while (delcnt--)
	    delete_record(currmaildir, sizeof(fileheader_t), delmsgs[delcnt]);
    }
    outs(mrd ? SHM->i18nstr[cuser.language][1249] : SHM->i18nstr[cuser.language][1250]);
    pressanykey();
    return -1;
}

static void
mailtitle()
{
    char            buf[256];

    showtitle(SHM->i18nstr[cuser.language][1251], BBSName);
    prints(SHM->i18nstr[cuser.language][1252],
	     HAS_PERM(PERM_NOOUTMAIL) ? SHM->i18nstr[cuser.language][1253] : SHM->i18nstr[cuser.language][1254]);
    buf[0] = 0;
    if (mailsumlimit) {
	snprintf(buf, sizeof(buf),
		 SHM->i18nstr[cuser.language][1255], mailsum, mailsumlimit,
		 mailkeep, mailmaxkeep);
    }
    prints("%-29s\033[m", buf);
}

static void
maildoent(int num, fileheader_t * ent)
{
    char           *title, *mark, color, type = "+ Mm"[(ent->filemode & 3)];

    if (TagNum && !Tagger(atoi(ent->filename + 2), 0, TAG_NIN))
	type = 'D';

    title = subject(mark = ent->title);
    if (title == mark) {
	color = '1';
	mark = SHM->i18nstr[cuser.language][1256];
    } else {
	color = '3';
	mark = "R:";
    }

    if (strncmp(currtitle, title, TTLEN))
	prints("%5d %c %-7s%-15.14s%s %.46s\n", num, type,
	       ent->date, ent->owner, mark, title);
    else
	prints("%5d %c %-7s%-15.14s\033[1;3%cm%s %.46s\033[0m\n", num, type,
	       ent->date, ent->owner, color, mark, title);
}


static int
mail_del(int ent, fileheader_t * fhdr, char *direct)
{
    char            genbuf[200];

    if (fhdr->filemode & FILE_MARKED)
	return DONOTHING;

    if (currmode & MODE_SELECT) {
		vmsg("請先回到正常模式後再進行刪除...");
		return READ_REDRAW;
	}
                        
	if (getans(msg_del_ny) == 'y') {                        
	if (!delete_record(direct, sizeof(*fhdr), ent)) {
	    setdirpath(genbuf, direct, fhdr->filename);
	    unlink(genbuf);
	    return DIRCHANGED;
	}
    }
    return READ_REDRAW;
}

static int
mail_read(int ent, fileheader_t * fhdr, char *direct)
{
    char            buf[64];
    char            done, delete_it, replied;

    clear();
    setdirpath(buf, direct, fhdr->filename);
    strncpy(currtitle, subject(fhdr->title), TTLEN);
    done = delete_it = replied = NA;
    while (!done) {
	int             more_result = more(buf, YEA);

	if (more_result != -1) {
	    fhdr->filemode |= FILE_READ;
			substitute_ref_record(direct, fhdr, ent);
	}
	switch (more_result) {
	case 999:
		mail_reply(ent, fhdr, direct);
		return FULLUPDATE;
	case -1:
		return READ_SKIP;
        case 0:
		break;
	default:
		return more_result;                                                                            	
	}
	outmsg(msg_mailer);

	switch (igetch()) {
	case 'r':
	case 'R':
	    replied = YEA;
	    mail_reply(ent, fhdr, direct);
	    break;
	case 'x':
	    m_forward(ent, fhdr, direct);
	    break;
	case 'y':
	    multi_reply(ent, fhdr, direct);
	    break;
	case 'd':
	    delete_it = YEA;
	default:
	    done = YEA;
	}
    }
    if (delete_it)
	mail_del(ent, fhdr, direct);
    else {
		fhdr->filemode |= FILE_READ;
		substitute_ref_record(direct, fhdr, ent);
    }
    return FULLUPDATE;
}

/* in boards/mail 回信給原作者，轉信站亦可 */
int
mail_reply(int ent, fileheader_t * fhdr, char *direct)
{
    char            uid[STRLEN];
    char           *t;
    FILE           *fp;
    char            genbuf[512];

    stand_title(SHM->i18nstr[cuser.language][1257]);

    /* 判斷是 boards 或 mail */
    if (curredit & EDIT_MAIL)
	setuserfile(quote_file, fhdr->filename);
    else
	setbfile(quote_file, currboard, fhdr->filename);

    /* find the author */
    strlcpy(quote_user, fhdr->owner, sizeof(quote_user));
    if (strchr(quote_user, '.')) {
	genbuf[0] = '\0';
	if ((fp = fopen(quote_file, "r"))) {
	    fgets(genbuf, sizeof(genbuf), fp);
	    fclose(fp);
	}
	t = strtok(genbuf, str_space);
	if (!strcmp(t, str_author1) || !strcmp(t, str_author2))
	    strlcpy(uid, strtok(NULL, str_space), sizeof(uid));
	else {
	    outs(SHM->i18nstr[cuser.language][1258]);
	    pressanykey();
	    return FULLUPDATE;
	}
    } else
	strlcpy(uid, quote_user, sizeof(uid));

    /* make the title */
    do_reply_title(3, fhdr->title);
    prints(SHM->i18nstr[cuser.language][1259], uid, save_title);

    /* edit, then send the mail */
    ent = curredit;
    switch (do_send(uid, save_title)) {
    case -1:
	outs(err_uid);
	break;
    case -2:
	outs(msg_cancel);
	break;
    case -3:
	prints(SHM->i18nstr[cuser.language][1260], uid);
	break;
    }
    curredit = ent;
    pressanykey();
    return FULLUPDATE;
}

static int
mail_edit(int ent, fileheader_t * fhdr, char *direct)
{
    char            genbuf[200];

    if (!HAS_PERM(PERM_SYSOP) &&
	strcmp(cuser.userid, fhdr->owner) &&
	strcmp(SHM->i18nstr[cuser.language][1261], fhdr->owner))
	return DONOTHING;

    setdirpath(genbuf, direct, fhdr->filename);
    vedit(genbuf, NA, NULL);
    return FULLUPDATE;
}

static int
mail_nooutmail(int ent, fileheader_t * fhdr, char *direct)
{
    cuser.userlevel ^= PERM_NOOUTMAIL;
    passwd_update(usernum, &cuser);
    return TITLE_REDRAW;

}

static int
mail_mark(int ent, fileheader_t * fhdr, char *direct)
{
    fhdr->filemode ^= FILE_MARKED;
    
	substitute_ref_record(direct, fhdr, ent);
    return PART_REDRAW;
}

/* help for mail reading */
static int    mail_help[] = {1262, 1263, 1264, 1265, 1266, 1267, 1268,
1269, 1270, 1271, 1272, 1273, 1274, 1275, 1276, 1277, 1278, 1279,
1280, 1281, 1282, -1};

static int
m_help()
{
    show_help(mail_help);
    return FULLUPDATE;
}

static int
mail_cross_post(int ent, fileheader_t * fhdr, char *direct)
{
    char            xboard[20], fname[80], xfpath[80], xtitle[80], inputbuf[10];
    fileheader_t    xfile;
    FILE           *xptr;
    int             author = 0;
    char            genbuf[200];
    char            genbuf2[4];

    move(2, 0);
    clrtoeol();
    move(1, 0);
    generalnamecomplete(SHM->i18nstr[cuser.language][1283], xboard, sizeof(xboard),
			SHM->Bnumber,
			completeboard_compar,
			completeboard_permission,
			completeboard_getname);
    if (*xboard == '\0' || !haspostperm(xboard))
	return TITLE_REDRAW;

    ent = 1;
    if (HAS_PERM(PERM_SYSOP) || !strcmp(fhdr->owner, cuser.userid)) {
	getdata(2, 0, SHM->i18nstr[cuser.language][1284],
		genbuf, 3, DOECHO);
	if (genbuf[0] != '2') {
	    ent = 0;
	    getdata(2, 0, SHM->i18nstr[cuser.language][1285], inputbuf, 3, DOECHO);
	    if (inputbuf[0] != 'n' && inputbuf[0] != 'N')
		author = 1;
	}
    }
    if (ent)
	snprintf(xtitle, sizeof(xtitle), SHM->i18nstr[cuser.language][1286], fhdr->title);
    else
	strlcpy(xtitle, fhdr->title, sizeof(xtitle));

    snprintf(genbuf, sizeof(genbuf), SHM->i18nstr[cuser.language][1287], xtitle);
    getdata(2, 0, genbuf, genbuf2, sizeof(genbuf2), LCECHO);
    if (*genbuf2 == 'n')
	if (getdata(2, 0, SHM->i18nstr[cuser.language][1288], genbuf, TTLEN, DOECHO))
	    strlcpy(xtitle, genbuf, sizeof(xtitle));

    getdata(2, 0, SHM->i18nstr[cuser.language][1289], genbuf, 3, LCECHO);
    if (genbuf[0] == 'l' || genbuf[0] == 's') {
	int             currmode0 = currmode;

	currmode = 0;
	setbpath(xfpath, xboard);
	stampfile(xfpath, &xfile);
	if (author)
	    strlcpy(xfile.owner, fhdr->owner, sizeof(xfile.owner));
	else
	    strlcpy(xfile.owner, cuser.userid, sizeof(xfile.owner));
	strlcpy(xfile.title, xtitle, sizeof(xfile.title));
	if (genbuf[0] == 'l') {
	    xfile.filemode = FILE_LOCAL;
	}
	setuserfile(fname, fhdr->filename);
	if (ent) {
	    char *save_currboard;
	    xptr = fopen(xfpath, "w");
	    assert(xptr);

	    strlcpy(save_title, xfile.title, sizeof(save_title));
	    save_currboard = currboard;
	    currboard = xboard;
	    write_header(xptr);
	    currboard = save_currboard;

	    fprintf(xptr, SHM->i18nstr[cuser.language][1290], cuser.userid);

	    b_suckinfile(xptr, fname);
	    addsignature(xptr, 0);
	    fclose(xptr);
	} else {
	    unlink(xfpath);
	    Link(fname, xfpath);
	}

	setbdir(fname, xboard);
	append_record(fname, &xfile, sizeof(xfile));
	setbtotal(getbnum(xboard));
	if (!xfile.filemode)
	    outgo_post(&xfile, xboard);
	cuser.numposts++;
	outmsg(SHM->i18nstr[cuser.language][1291]);
	pressanykey();
	currmode = currmode0;
    }
    return FULLUPDATE;
}

int
mail_man()
{
    char            buf[64], buf1[64];
    if (HAS_PERM(PERM_MAILLIMIT)) {
	int             mode0 = currutmp->mode;
	int             stat0 = currstat;

	sethomeman(buf, cuser.userid);
	snprintf(buf1, sizeof(buf1), SHM->i18nstr[cuser.language][1292], cuser.userid);
	a_menu(buf1, buf, 1);
	currutmp->mode = mode0;
	currstat = stat0;
	return FULLUPDATE;
    }
    return DONOTHING;
}

static int
mail_cite(int ent, fileheader_t * fhdr, char *direct)
{
    char            fpath[256];
    char            title[TTLEN + 1];
    static char     xboard[20];
    char            buf[20];
    int             bid;

    setuserfile(fpath, fhdr->filename);
    strlcpy(title, SHM->i18nstr[cuser.language][1293], sizeof(title));
    strncpy(title + 3, fhdr->title, TTLEN - 3);
    title[TTLEN] = '\0';
    a_copyitem(fpath, title, 0, 1);

    if (cuser.userlevel >= PERM_BM) {
	move(2, 0);
	clrtoeol();
	move(3, 0);
	clrtoeol();
	move(1, 0);

	generalnamecomplete(SHM->i18nstr[cuser.language][1294],
			    buf, sizeof(buf),
			    SHM->Bnumber,
			    completeboard_compar,
			    completeboard_permission,
			    completeboard_getname);
	if (*buf)
	    strlcpy(xboard, buf, sizeof(xboard));
	if (*xboard && ((bid = getbnum(xboard)) >= 0)){ /* XXXbid */
	    setapath(fpath, xboard);
	    setutmpmode(ANNOUNCE);
	    a_menu(xboard, fpath, HAS_PERM(PERM_ALLBOARD) ? 2 :
		   is_BM_cache(bid) ? 1 : 0);
	} else {
	    mail_man();
	}
	return FULLUPDATE;
    } else {
	mail_man();
	return FULLUPDATE;
    }
}

static int
mail_save(int ent, fileheader_t * fhdr, char *direct)
{
    char            fpath[256];
    char            title[TTLEN + 1];

    if (HAS_PERM(PERM_MAILLIMIT)) {
	setuserfile(fpath, fhdr->filename);
	strlcpy(title, SHM->i18nstr[cuser.language][1295], sizeof(title));
	strncpy(title + 3, fhdr->title, TTLEN - 3);
	title[TTLEN] = '\0';
	a_copyitem(fpath, title, fhdr->owner, 1);
	sethomeman(fpath, cuser.userid);
	a_menu(cuser.userid, fpath, 1);
	return FULLUPDATE;
    }
    return DONOTHING;
}

#ifdef OUTJOBSPOOL
static int
mail_waterball(int ent, fileheader_t * fhdr, char *direct)
{
    static char     address[60], cmode = 1;
    char            fname[500], genbuf[200];
    FILE           *fp;

    if (!(strstr(fhdr->title, SHM->i18nstr[cuser.language][1296]) && strstr(fhdr->title, SHM->i18nstr[cuser.language][1297]))) {
	vmsg(SHM->i18nstr[cuser.language][1298]);
	return 1;
    }
    if (!address[0])
	strlcpy(address, cuser.email, sizeof(address));
    move(b_lines - 8, 0);
    outs(SHM->i18nstr[cuser.language][1299]);
    if (address[0]) {
	snprintf(genbuf, sizeof(genbuf), SHM->i18nstr[cuser.language][1300], address);
	getdata(b_lines - 5, 0, genbuf, fname, 3, LCECHO);
	if (fname[0] == 'q') {
	    outmsg(SHM->i18nstr[cuser.language][1301]);
	    return 1;
	}
	if (fname[0] == 'n')
	    address[0] = '\0';
    }
    if (!address[0]) {
	getdata(b_lines - 5, 0, SHM->i18nstr[cuser.language][1302], fname, 60, DOECHO);
	if (fname[0] && strchr(fname, '.')) {
	    strlcpy(address, fname, sizeof(address));
	} else {
	    vmsg(SHM->i18nstr[cuser.language][1303]);
	    return 1;
	}
    }
    if (invalidaddr(address))
	return -2;
    if( strstr(address, ".bbs") && HAS_PERM(PERM_NOOUTMAIL) ){
	move(b_lines - 4, 0);
	outs(SHM->i18nstr[cuser.language][1304]);
	vmsg(SHM->i18nstr[cuser.language][1305]);
	return FULLUPDATE;
    }

    //snprintf(fname, sizeof(fname), "%d\n", cmode);
    move(b_lines - 4, 0);
    outs(SHM->i18nstr[cuser.language][1306]);
    getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][1307], fname, 3, LCECHO);
    if (fname[0] == 'Q' || fname[0] == 'q') {
	outmsg(SHM->i18nstr[cuser.language][1308]);
	return 1;
    }
    cmode = (fname[0] != '0' && fname[0] != '1') ? 1 : fname[0] - '0';

    snprintf(fname, sizeof(fname), BBSHOME "/jobspool/water.src.%s-%d",
	     cuser.userid, (int)now);
    snprintf(genbuf, sizeof(genbuf), "cp " BBSHOME "/home/%c/%s/%s %s",
	     cuser.userid[0], cuser.userid, fhdr->filename, fname);
    system(genbuf);
    /* dirty code ;x */
    snprintf(fname, sizeof(fname), BBSHOME "/jobspool/water.des.%s-%d",
	     cuser.userid, (int)now);
    fp = fopen(fname, "wt");
    assert(fp);
    fprintf(fp, "%s\n%s\n%d\n", cuser.userid, address, cmode);
    fclose(fp);
    vmsg(SHM->i18nstr[cuser.language][1309]);
    return FULLUPDATE;
}
#endif
static onekey_t mail_comms[] = {
    NULL, // Ctrl('A') 1
    NULL, // Ctrl('B')
    NULL, // Ctrl('C')
    NULL, // Ctrl('D')
    NULL, // Ctrl('E')
    NULL, // Ctrl('F')
    built_mail_index, // Ctrl('G')
    NULL, // Ctrl('H')
    NULL, // Ctrl('I') 
    NULL, // Ctrl('J')
    NULL, // Ctrl('K')
    NULL, // Ctrl('L')
    NULL, // Ctrl('M')
    NULL, // Ctrl('N')
    NULL, // Ctrl('O')
    NULL, // Ctrl('P')
    NULL, // Ctrl('Q')
    NULL, // Ctrl('R')
    NULL, // Ctrl('S')
    NULL, // Ctrl('T')
    NULL, // Ctrl('U')
    NULL, // Ctrl('V')
    NULL, // Ctrl('W')
    NULL, // Ctrl('X')
    NULL, // Ctrl('Y')
    NULL, // Ctrl('Z') 26
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, // 'A' 65
    NULL, // 'B'
    NULL, // 'C'
    del_range, // 'D'
    mail_edit, // 'E'
    NULL, // 'F'
    NULL, // 'G'
    NULL, // 'H'
    NULL, // 'I'
    NULL, // 'J'
    NULL, // 'K'
    NULL, // 'L'
    NULL, // 'M'
    NULL, // 'N'
    mail_nooutmail, // 'O'
    NULL, // 'P'
    NULL, // 'Q'
    mail_reply, // 'R'
    NULL, // 'S'
    edit_title, // 'T'
    NULL, // 'U'
    NULL, // 'V'
    NULL, // 'W'
    mail_cross_post, // 'X'
    NULL, // 'Y'
    NULL, // 'Z' 90
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, // 'a' 97
    NULL, // 'b'
    mail_cite, // 'c'
    mail_del, // 'd'
    NULL, // 'e'
    NULL, // 'f'
    NULL, // 'g'
    m_help, // 'h'
    NULL, // 'i'
    NULL, // 'j'
    NULL, // 'k'
    NULL, // 'l'
    mail_mark, // 'm'
    NULL, // 'n'
    NULL, // 'o'
    NULL, // 'p'
    NULL, // 'q'
    mail_read, // 'r'
    mail_save, // 's'
    NULL, // 't'
#ifdef OUTJOBSPOOL
    mail_waterball, // 'u'
#else
    NULL, // 'u'
#endif
    NULL, // 'v'
    NULL, // 'w'
    m_forward, // 'x'
    multi_reply, // 'y'
    mail_man, // 'z' 122
};

int
m_read()
{
    int back_bid;
    if (get_num_records(currmaildir, sizeof(fileheader_t))) {
	curredit = EDIT_MAIL;
	curredit &= ~EDIT_ITEM;
	back_bid = currbid;
	currbid = 0;
	i_read(RMAIL, currmaildir, mailtitle, maildoent, mail_comms, -1);
	currbid = back_bid;
	curredit = 0;
	currutmp->mailalert = load_mailalert(cuser.userid);
	return 0;
    } else {
	outs(SHM->i18nstr[cuser.language][1310]);
	return XEASY;
    }
}

/* 寄站內信 */
static int
send_inner_mail(char *fpath, char *title, char *receiver)
{
    char            genbuf[256];
    fileheader_t    mymail;

    if (!searchuser(receiver))
	return -2;
    sethomepath(genbuf, receiver);
    stampfile(genbuf, &mymail);
    if (!strcmp(receiver, cuser.userid)) {
	strlcpy(mymail.owner, "[" BBSNAME "]", sizeof(mymail.owner));
	mymail.filemode = FILE_READ;
    } else
	strlcpy(mymail.owner, cuser.userid, sizeof(mymail.owner));
    strncpy(mymail.title, title, TTLEN);
    unlink(genbuf);
    Link(fpath, genbuf);
    sethomedir(genbuf, receiver);
    return append_record_forward(genbuf, &mymail, sizeof(mymail));
}

#include <netdb.h>
#include <pwd.h>
#include <time.h>

#ifndef USE_BSMTP
static int
bbs_sendmail(char *fpath, char *title, char *receiver)
{
    char           *ptr;
    char            genbuf[256];
    FILE           *fin, *fout;

    /* 中途攔截 */
    if ((ptr = strchr(receiver, ';'))) {
	*ptr = '\0';
    }
    if ((ptr = strstr(receiver, str_mail_address)) || !strchr(receiver, '@')) {
	char            hacker[20];
	int             len;

	if (strchr(receiver, '@')) {
	    len = ptr - receiver;
	    memcpy(hacker, receiver, len);
	    hacker[len] = '\0';
	} else
	    strlcpy(hacker, receiver, sizeof(hacker));
	return send_inner_mail(fpath, title, hacker);
    }
    /* Running the sendmail */
    if (fpath == NULL) {
	snprintf(genbuf, sizeof(genbuf),
		 "/usr/sbin/sendmail %s > /dev/null", receiver);
	fin = fopen("etc/confirm", "r");
    } else {
	snprintf(genbuf, sizeof(genbuf),
		 "/usr/sbin/sendmail -f %s%s %s > /dev/null",
		 cuser.userid, str_mail_address, receiver);
	fin = fopen(fpath, "r");
    }
    fout = popen(genbuf, "w");
    if (fin == NULL || fout == NULL) // XXX no fclose() if only one fopen succeed
	return -1;

    if (fpath)
	fprintf(fout, "Reply-To: %s%s\nFrom: %s <%s%s>\n",
		cuser.userid, str_mail_address,
		cuser.username,
		cuser.userid, str_mail_address);
    fprintf(fout,"To: %s\nSubject: %s\nMime-Version: 1.0\r\nContent-Type: text/plain; charset=\"big5\"\r\nContent-Transfer-Encoding: 8bit\r\nX-Disclaimer: " BBSNAME SHM->i18nstr[cuser.language][1311],
		receiver, title);

    while (fgets(genbuf, sizeof(genbuf), fin)) {
	if (genbuf[0] == '.' && genbuf[1] == '\n')
	    fputs(". \n", fout);
	else
	    fputs(genbuf, fout);
    }
    fclose(fin);
    fprintf(fout, ".\n");
    pclose(fout);
    return 0;
}
#else				/* USE_BSMTP */

int
bsmtp(char *fpath, char *title, char *rcpt, int method)
{
    char            buf[80], *ptr;
    time_t          chrono;
    MailQueue       mqueue;

    /* check if the mail is a inner mail */
    if ((ptr = strstr(rcpt, str_mail_address)) || !strchr(rcpt, '@')) {
	char            hacker[20];
	int             len;

	if (strchr(rcpt, '@')) {
	    len = ptr - rcpt;
	    memcpy(hacker, rcpt, len);
	    hacker[len] = '\0';
	} else
	    strlcpy(hacker, rcpt, sizeof(hacker));
	return send_inner_mail(fpath, title, hacker);
    }
    chrono = now;
    if (method != MQ_JUSTIFY) {	/* 認證信 */
	/* stamp the queue file */
	strlcpy(buf, "out/", sizeof(buf));
	for (;;) {
	    snprintf(buf + 4, sizeof(buf) - 4, "M.%d.A", (int)++chrono);
	    if (!dashf(buf)) {
		Link(fpath, buf);
		break;
	    }
	}

	fpath = buf;

	strlcpy(mqueue.filepath, fpath, sizeof(mqueue.filepath));
	strlcpy(mqueue.subject, title, sizeof(mqueue.subject));
    }
    /* setup mail queue */
    mqueue.mailtime = chrono;
    mqueue.method = method;
    strlcpy(mqueue.sender, cuser.userid, sizeof(mqueue.sender));
    strlcpy(mqueue.username, cuser.username, sizeof(mqueue.username));
    strlcpy(mqueue.rcpt, rcpt, sizeof(mqueue.rcpt));
    if (append_record("out/.DIR", (fileheader_t *) & mqueue, sizeof(mqueue)) < 0)
	return 0;
    return chrono;
}
#endif				/* USE_BSMTP */

int
doforward(char *direct, fileheader_t * fh, int mode)
{
    static char     address[60];
    char            fname[500];
    int             return_no;
    char            genbuf[200];

    if (!address[0])
	strlcpy(address, cuser.email, sizeof(address));

    if( mode == 'U' ){
	vmsg(SHM->i18nstr[cuser.language][1312]);
    }

    if (address[0]) {
	snprintf(genbuf, sizeof(genbuf),
		 SHM->i18nstr[cuser.language][1313], address);
	getdata(b_lines, 0, genbuf, fname, 3, LCECHO);

	if (fname[0] == 'q') {
	    outmsg(SHM->i18nstr[cuser.language][1314]);
	    return 1;
	}
	if (fname[0] == 'n')
	    address[0] = '\0';
    }
    if (!address[0]) {
	do {
	    getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][1315], fname, 60, DOECHO);
	    if (fname[0]) {
		if (strchr(fname, '.'))
		    strlcpy(address, fname, sizeof(address));
		else
		    snprintf(address, sizeof(address),
			     "%s.bbs@%s", fname, MYHOSTNAME);
	    } else {
		vmsg(SHM->i18nstr[cuser.language][1316]);
		return 1;
	    }
	} while (mode == 'Z' && strstr(address, MYHOSTNAME));
    }
    if (invalidaddr(address))
	return -2;

    outmsg(fname);
    refresh();

    /* 追蹤使用者 */
    if (HAS_PERM(PERM_LOGUSER)) {
    	log_user("mailforward to %s ",address);
    }
    if (mode == 'Z') {
	snprintf(fname, sizeof(fname),
		 TAR_PATH " cfz /tmp/home.%s.tgz home/%c/%s; "
		 MUTT_PATH " -a /tmp/home.%s.tgz -s 'home.%s.tgz' '%s' </dev/null;"
		 "rm /tmp/home.%s.tgz",
		 cuser.userid, cuser.userid[0], cuser.userid,
		 cuser.userid, cuser.userid, address, cuser.userid);
	system(fname);
	return 0;
	snprintf(fname, sizeof(fname), TAR_PATH " cfz - home/%c/%s | "
		"/usr/bin/uuencode %s.tgz > %s",
		cuser.userid[0], cuser.userid, cuser.userid, direct);
	system(fname);
	strlcpy(fname, direct, sizeof(fname));
    } else if (mode == 'U') {
	char            tmp_buf[128];

	snprintf(fname, sizeof(fname), "/tmp/bbs.uu%05d", currpid);
	snprintf(tmp_buf, sizeof(tmp_buf),
		 "/usr/bin/uuencode %s/%s uu.%05d > %s",
		 direct, fh->filename, currpid, fname);
	system(tmp_buf);
    } else if (mode == 'F') {
	char            tmp_buf[128];

	snprintf(fname, sizeof(fname), "/tmp/bbs.f%05d", currpid);
	snprintf(tmp_buf, sizeof(tmp_buf),
		 "cp %s/%s %s", direct, fh->filename, fname);
	system(tmp_buf);
    } else
	return -1;

    return_no =
#ifndef USE_BSMTP
	bbs_sendmail(fname, fh->title, address);
#else
	bsmtp(fname, fh->title, address, mode);
#endif
    unlink(fname);
    return (return_no);
}

int
load_mailalert(char *userid)
{
    struct stat     st;
    char            maildir[256];
    int             fd;
    register int    numfiles;
    fileheader_t    my_mail;

    sethomedir(maildir, userid);
    if (!HAS_PERM(PERM_BASIC))
	return 0;
    if (stat(maildir, &st) < 0)
	return 0;
    numfiles = st.st_size / sizeof(fileheader_t);
    if (numfiles <= 0)
	return 0;

    /* 看看有沒有信件還沒讀過？從檔尾回頭檢查，效率較高 */
    if ((fd = open(maildir, O_RDONLY)) > 0) {
	lseek(fd, st.st_size - sizeof(fileheader_t), SEEK_SET);
	while (numfiles--) {
	    read(fd, &my_mail, sizeof(fileheader_t));
	    if (!(my_mail.filemode & FILE_READ)) {
		close(fd);
		return 1;
	    }
	    lseek(fd, -(off_t) 2 * sizeof(fileheader_t), SEEK_CUR);
	}
	close(fd);
    }
    return 0;
}

#ifdef  EMAIL_JUSTIFY
static void
mail_justify(userec_t muser)
{
    fileheader_t    mhdr;
    char            title[128], buf1[80];
    FILE           *fp;

    sethomepath(buf1, muser.userid);
    stampfile(buf1, &mhdr);
    unlink(buf1);
    strlcpy(mhdr.owner, cuser.userid, sizeof(mhdr.owner));
    strncpy(mhdr.title, SHM->i18nstr[cuser.language][1318], TTLEN);
    mhdr.filemode = 0;

    if (valid_ident(muser.email) && !invalidaddr(muser.email)) {
	char            title[80], *ptr;
	unsigned short  checksum;	/* 16-bit is enough */
	char            ch;

	checksum = searchuser(muser.userid);
	ptr = muser.email;
	while ((ch = *ptr++)) {
	    if (ch <= ' ')
		break;
	    if (ch >= 'A' && ch <= 'Z')
		ch |= 0x20;
	    checksum = (checksum << 1) ^ ch;
	}

	snprintf(title, sizeof(title), "[PTT BBS]To %s(%d:%d) [User Justify]",
		muser.userid, getuser(muser.userid) + MAGIC_KEY, checksum);
	if (
#ifndef USE_BSMTP
	    bbs_sendmail(NULL, title, muser.email)
#else
	    bsmtp(NULL, title, muser.email, MQ_JUSTIFY);
#endif
	<0)
	    Link("etc/bademail", buf1);
	else
	Link("etc/replyemail", buf1);
    } else
	Link("etc/bademail", buf1);
    sethomedir(title, muser.userid);
    append_record_forward(title, &mhdr, sizeof(mhdr));
}
#endif				/* EMAIL_JUSTIFY */
