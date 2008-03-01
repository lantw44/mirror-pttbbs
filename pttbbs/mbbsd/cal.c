/* $Id$ */
#include "bbs.h"

/* ���� Multi play */
static int
is_playing(int unmode)
{
    register int    i;
    register userinfo_t *uentp;
    unsigned int p = StringHash(cuser.userid) % USHM_SIZE;

    for (i = 0; i < USHM_SIZE; i++, p++) { // XXX linear search
	if (p == USHM_SIZE)
	    p = 0;
	uentp = &(SHM->uinfo[p]);
	if (uentp->uid == usernum)
	    if (uentp->lockmode == unmode)
		return 1;
    }
    return 0;
}

int
lockutmpmode(int unmode, int state)
{
    int             errorno = 0;

    if (currutmp->lockmode)
	errorno = LOCK_THIS;
    else if (state == LOCK_MULTI && is_playing(unmode))
	errorno = LOCK_MULTI;

    if (errorno) {
	clear();
	move(10, 20);
	if (errorno == LOCK_THIS)
	    prints("�Х����} %s �~��A %s ",
		   ModeTypeTable[currutmp->lockmode],
		   ModeTypeTable[unmode]);
	else
	    prints("��p! �z�w����L�u�ۦP��ID���b%s",
		   ModeTypeTable[unmode]);
	pressanykey();
	return errorno;
    }
    setutmpmode(unmode);
    currutmp->lockmode = unmode;
    return 0;
}

int
unlockutmpmode(void)
{
    currutmp->lockmode = 0;
    return 0;
}

/* �ϥο������ */
#define VICE_NEW   "vice.new"

/* Heat:�o�� */
int
vice(int money, const char *item)
{
    char            buf[128];
    unsigned int viceserial = (currutmp->lastact % 10000) * 10000 + 
	random() % 10000;

    // new logic: do not send useless vice tickets
    demoney(-money);
    if (money < VICE_MIN)
	return 0;

    setuserfile(buf, VICE_NEW);
    log_filef(buf, LOG_CREAT, "%8.8d\n", viceserial);
    snprintf(buf, sizeof(buf),
	    "%s ��F$%d �s��[%08d]", item, money, viceserial);
    mail_id(cuser.userid, buf, "etc/vice.txt", BBSMNAME "�g�ٳ�");
    return 0;
}

#define lockreturn(unmode, state)  if(lockutmpmode(unmode, state)) return
#define lockreturn0(unmode, state) if(lockutmpmode(unmode, state)) return 0
#define lockbreak(unmode, state)   if(lockutmpmode(unmode, state)) break
#define SONGBOOK  "etc/SONGBOOK"
#define OSONGPATH "etc/SONGO"

static int
osong(void)
{
    char            sender[IDLEN + 1], receiver[IDLEN + 1], buf[200],
		    genbuf[200], filename[256], say[51];
    char            trans_buffer[PATHLEN];
    char            address[45];
    FILE           *fp, *fp1;
    //*fp2;
    fileheader_t    mail;
    int             nsongs;

    strlcpy(buf, Cdatedate(&now), sizeof(buf));

    lockreturn0(OSONG, LOCK_MULTI);

    /* Jaky �@�H�@���I�@�� */
    if (!strcmp(buf, Cdatedate(&cuser.lastsong)) && !HasUserPerm(PERM_SYSOP)) {
	move(22, 0);
	vmsg("�A���Ѥw�g�I�L�o�A���ѦA�I�a....");
	unlockutmpmode();
	return 0;
    }

    while (1) {
	char ans[4];
	move(12, 0);
	clrtobot();
	prints("�˷R�� %s �w��Ө�ڮ�۰��I�q�t��\n\n", cuser.userid);
	outs(ANSI_COLOR(1) "�`�N�I�q���e�ФůA����| �H������ �T��"
	     "���M�V�d ����\n"
	     "�Y���W�z�H�W���ΡA����N�O�d�M�w�O�_���}�����v�Q\n"
	     "�p���P�N�Ы� (3) ���}�C" ANSI_RESET "\n");
	getdata(18, 0, "�п�� " ANSI_COLOR(1) "1)" ANSI_RESET " �}�l�I�q�B"
		ANSI_COLOR(1) "2)" ANSI_RESET " �ݺq���B"
		"�άO " ANSI_COLOR(1) "3)" ANSI_RESET " ���}: ",
		ans, sizeof(ans), DOECHO);

	if (ans[0] == '1')
	    break;
	else if (ans[0] == '2') {
	    a_menu("�I�q�q��", SONGBOOK, 0, 0, NULL);
	    clear();
	}
	else if (ans[0] == '3') {
	    vmsg("���¥��{ :)");
	    unlockutmpmode();
	    return 0;
	}
    }

    if (cuser.money < 200) {
	move(22, 0);
	vmsg("�I�q�n200�ȭ�!....");
	unlockutmpmode();
	return 0;
    }

    getdata_str(19, 0, "�I�q��(�i�ΦW): ", sender, sizeof(sender), DOECHO, cuser.userid);
    getdata(20, 0, "�I��(�i�ΦW): ", receiver, sizeof(receiver), DOECHO);

    getdata_str(21, 0, "�Q�n�n��L(�o)��..:", say,
		sizeof(say), DOECHO, "�ڷR�p..");
    snprintf(save_title, sizeof(save_title),
	     "%s:%s", sender, say);
    getdata_str(22, 0, "�H��֪��H�c(�u�� ID �� E-mail)?",
		address, sizeof(address), LCECHO, receiver);
    vmsg("���ۭn��q�o..�i�J�q���n�n����@���q�a..^o^");
    a_menu("�I�q�q��", SONGBOOK, 0, 0, trans_buffer);
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
    strlcpy(mail.owner, "�I�q��", sizeof(mail.owner));
    snprintf(mail.title, sizeof(mail.title), "�� %s �I�� %s ", sender, receiver);

    while (fgets(buf, sizeof(buf), fp)) {
	char           *po;
	if (!strncmp(buf, "���D: ", 6)) {
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
	    const char *dot = "";
	    if (is_validuserid(sender) && strcmp(sender, cuser.userid) != 0)
		dot = ".";
	    po[0] = 0;
	    snprintf(genbuf, sizeof(genbuf), "%s%s%s%s", buf, sender, dot, po + 7);
	    strlcpy(buf, genbuf, sizeof(buf));
	}
	while ((po = strstr(buf, "<~Des~>"))) {
	    po[0] = 0;
	    snprintf(genbuf, sizeof(genbuf), "%s%s%s", buf, receiver, po + 7);
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

    log_filef("etc/osong.log",  LOG_CREAT, "id: %-12s �� %s �I�� %s : \"%s\", ��H�� %s\n", cuser.userid, sender, receiver, say, address);

    if (append_record(OSONGPATH "/" FN_DIR, &mail, sizeof(mail)) != -1) {
	cuser.lastsong = now;
	/* Jaky �W�L MAX_MOVIE ���q�N�}�l�� */
	nsongs = get_num_records(OSONGPATH "/" FN_DIR, sizeof(mail));
	if (nsongs > MAX_MOVIE) {
	    // XXX race condition
	    delete_range(OSONGPATH "/" FN_DIR, 1, nsongs - MAX_MOVIE);
	}
	snprintf(genbuf, sizeof(genbuf), "%s says \"%s\" to %s.", sender, say, receiver);
	log_usies("OSONG", genbuf);
	/* ��Ĥ@������ */
	vice(200, "�I�q");
    }
    snprintf(save_title, sizeof(save_title), "%s:%s", sender, say);
    hold_mail(filename, receiver);

    if (address[0]) {
#ifndef USE_BSMTP
	bbs_sendmail(filename, save_title, address);
#else
	bsmtp(filename, save_title, address);
#endif
    }
    clear();
    outs(
	 "\n\n  ���߱z�I�q�����o..\n"
	 "  �@�p�ɤ��ʺA�ݪO�|�۰ʭ��s��s\n"
	 "  �j�a�N�i�H�ݨ�z�I���q�o\n\n"
	 "  �I�q��������D�i�H��Note�O����ذϧ䵪��\n"
	 "  �]�i�bNote�O��ذϬݨ�ۤv���I�q�O��\n"
	 "  ������O�Q���N���]�w���Note�O�d��\n"
	 "  ���ˤ����O�D���z�A��\n");
    pressanykey();
    sortsong();
    topsong();

    unlockutmpmode();
    return 1;
}

int
ordersong(void)
{
    osong();
    return 0;
}

static int
inmailbox(int m)
{
    userec_t xuser;
    passwd_query(usernum, &xuser);
    cuser.exmailbox = xuser.exmailbox + m;
    passwd_update(usernum, &cuser);
    return cuser.exmailbox;
}


#if !HAVE_FREECLOAK
/* ������ */
int
p_cloak(void)
{
    if (getans(currutmp->invisible ? "�T�w�n�{��?[y/N]" : "�T�w�n����?[y/N]") != 'y')
	return 0;
    if (cuser.money >= 19) {
	vice(19, "�I�O����");
	currutmp->invisible %= 2;
	vmsg((currutmp->invisible ^= 1) ? MSG_CLOAKED : MSG_UNCLOAK);
    }
    return 0;
}
#endif

int
p_from(void)
{
    char tmp_from[sizeof(currutmp->from)];
    if (getans("�T�w�n��G�m?[y/N]") != 'y')
	return 0;
    reload_money();
    if (cuser.money < 49)
	return 0;
    if (getdata_buf(b_lines - 1, 0, "�п�J�s�G�m:",
		    tmp_from, sizeof(tmp_from), DOECHO)) {
	vice(49, "���G�m");
	strlcpy(currutmp->from, tmp_from, sizeof(currutmp->from));
	currutmp->from_alias = 0;
    }
    return 0;
}

int
p_exmail(void)
{
    char            ans[4], buf[200];
    int             n;

    if (cuser.exmailbox >= MAX_EXKEEPMAIL) {
	vmsgf("�e�q�̦h�W�[ %d �ʡA����A�R�F�C", MAX_EXKEEPMAIL);
	return 0;
    }
    snprintf(buf, sizeof(buf),
	     "�z���W�� %d �ʮe�q�A�٭n�A�R�h��? ", cuser.exmailbox);

    // no need to create default prompt.
    // and people usually come this this by accident...
    getdata(b_lines - 2, 0, buf, ans, sizeof(ans), LCECHO);

    n = atoi(ans);
    if (!ans[0] || n<=0)
	return 0;

    if (n + cuser.exmailbox > MAX_EXKEEPMAIL)
	n = MAX_EXKEEPMAIL - cuser.exmailbox;
    reload_money();
    if (cuser.money < n * 1000)
    {
	vmsg("�A���������C");
	return 0;
    }

    if (vmsgf("�A�Q�ʶR %d �ʫH�c (�n�� %d ��), �T�w�ܡH[y/N] ", 
		n, n*1000) != 'y')
	return 0;

    vice(n * 1000, "�ʶR�H�c");
    inmailbox(n);
    vmsgf("�w�ʶR�H�c�C�s�e�q�W��: %d", cuser.exmailbox);
    return 0;
}

void
mail_redenvelop(const char *from, const char *to, int money, char mode)
{
    char            genbuf[200];
    fileheader_t    fhdr;
    FILE           *fp;

    sethomepath(genbuf, to);
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
	return;
    fprintf(fp, "�@��: %s\n"
	    "���D: �۰]�i�_\n"
	    "�ɶ�: %s\n"
	    ANSI_COLOR(1;33) "�˷R�� %s �G\n\n" ANSI_RESET
	    ANSI_COLOR(1;31) "    �ڥ]���A�@�� %d �����j���]�� ^_^\n\n"
	    "    §�����N���A�Я���...... ^_^" ANSI_RESET "\n",
	    from, ctime4(&now), to, money);
    fclose(fp);
    snprintf(fhdr.title, sizeof(fhdr.title), "�۰]�i�_");
    strlcpy(fhdr.owner, from, sizeof(fhdr.owner));

    if (mode == 'y')
	vedit(genbuf, NA, NULL);
    sethomedir(genbuf, to);
    append_record(genbuf, &fhdr, sizeof(fhdr));
}


int do_give_money(char *id, int uid, int money)
{
    int tax;
#ifdef PLAY_ANGEL
    userec_t        xuser;
#endif

    reload_money();
    if (money > 0 && cuser.money >= money) {
	tax = give_tax(money);
	if (money - tax <= 0)
	    return -1;		/* ú���|�N�S�����F */
	deumoney(uid, money - tax);
	demoney(-money);
	log_filef(FN_MONEY, LOG_CREAT, "%-12s �� %-12s %d\t(�|�� %d)\t%s",
                 cuser.userid, id, money, money - tax, ctime4(&now));
#ifdef PLAY_ANGEL
	getuser(id, &xuser);
	if (!strcmp(xuser.myangel, cuser.userid)){
	    mail_redenvelop(
		    getkey("�L�O�A���p�D�H�A�O�_�ΦW�H[Y/n]") == 'n' ?
		    cuser.userid : "�p�Ѩ�", id, money - tax,
			getans("�n�ۦ�Ѽg���]�U�ܡH[y/N]"));
	} else
#endif
	mail_redenvelop(cuser.userid, id, money - tax,
		getans("�n�ۦ�Ѽg���]�U�ܡH[y/N]"));
	if (money < 50) {
	    usleep(2000000);
	} else if (money < 200) {
	    usleep(500000);
	} else {
	    usleep(100000);
	}
	return 0;
    }
    return -1;
}

int
p_give(void)
{
    give_money_ui(NULL);
    return -1;
}

int 
give_money_ui(const char *userid)
{
    int             uid;
    char            id[IDLEN + 1], money_buf[20];
    char	    passbuf[PASSLEN];
    int		    m = 0, tries = 3, skipauth = 0;
    static time4_t  lastauth = 0;

    // TODO prevent macros, we should check something here,
    // like user pw/id/...
    clear();
    stand_title("��������");
    if (!userid || !*userid)
	usercomplete("�o�쩯�B�઺id: ", id);
    else {
	strlcpy(id, userid, sizeof(id));
	prints("�o�쩯�B�઺id: %s\n", id);
    }
    move(2, 0); clrtobot();

    if (!id[0] || !strcasecmp(cuser.userid, id))
    {
	vmsg("�������!");
	return -1;
    }
    if (!getdata(2, 0, "�n���L�h�ֿ��O: ", money_buf, 7, LCECHO) ||
	((m = atoi(money_buf)) <= 0))
    {
	vmsg("�������!");
	return -1;
    }
    if ((uid = searchuser(id, id)) == 0) {
	vmsg("�d�L���H!");
	return -1;
    }
    move(4, 0);
    prints("������e: %s �N���� %s : %d �� (�n�A���|�� %d ��)\n", 
	    cuser.userid, id, m, give_tax(m));

    if (now - lastauth >= 15*60) // valid through 15 minutes
    {
	outs(ANSI_COLOR(1;31) "���F�קK�~���άO�c�N�B�F�A"
		"�b��������e�n���s�T�{�z�������C" ANSI_RESET);
    } else {
	outs("�A���{�ҩ|���L���A�i�Ȯɸ��L�K�X�{�ҵ{�ǡC\n");
	// auth is valid.
	if (getans("�T�w�i�����ܡH (y/N): ") == 'y')
	    skipauth = 1;
	else
	    tries = -1;
    }

    while (!skipauth && tries-- > 0)
    {
	getdata(6, 0, MSG_PASSWD,
		passbuf, sizeof(passbuf), NOECHO);
	passbuf[8] = '\0';
	if (checkpasswd(cuser.passwd, passbuf))
	{
	    lastauth = now;
	    break;
	}
	if (tries > 0)
	    vmsgf("�K�X���~�A�٦� %d �����|�C", tries);
    }
    if (tries < 0)
    {
	vmsg("�������!");
	return -1;
    }
    // vmsg("�ǳƥ���C");
    // return -1;
    return do_give_money(id, uid, m);
}

void
resolve_over18(void)
{
    /* get local time */
    struct tm ptime = *localtime4(&now);

    over18 = 0;
    /* check if over18 */
    // �ӹ귳�p��A�S�ͤ骺���@���� 18
    if (cuser.year < 1 || cuser.month < 1)
	over18 = 0;
    else if( (ptime.tm_year - cuser.year) > 18)
	over18 = 1;
    else if (ptime.tm_year - cuser.year < 18)
	over18 = 0;
    else if ((ptime.tm_mon+1) > cuser.month)
	over18 = 1;
    else if ((ptime.tm_mon+1) < cuser.month)
	over18 = 0;
    else if (ptime.tm_mday >= cuser.day )
	over18 = 1;
}

int
p_sysinfo(void)
{
    char            *cpuloadstr;
    int             load;
    extern char    *compile_time;
#ifdef DETECT_CLIENT
    extern Fnv32_t  client_code;
#endif

    load = cpuload(NULL);
    cpuloadstr = (load < 5 ? "�}�n" : (load < 20 ? "�|�i" : "�L��"));

    clear();
    showtitle("�t�θ�T", BBSNAME);
    move(2, 0);
    prints("�z�{�b��� " TITLE_COLOR BBSNAME ANSI_RESET " (" MYIP ")\n"
	   "�t�έt�����p: %s\n"
	   "�u�W�A�ȤH��: %d/%d\n"
#ifdef DETECT_CLIENT
	   "client code:  %8.8X\n"
#endif
	   "�sĶ�ɶ�:     %s\n"
	   "�_�l�ɶ�:     %s\n",
	   cpuloadstr, SHM->UTMPnumber,
#ifdef DYMAX_ACTIVE
	   SHM->GV2.e.dymaxactive > 2000 ? SHM->GV2.e.dymaxactive : MAX_ACTIVE,
#else
	   MAX_ACTIVE,
#endif
#ifdef DETECT_CLIENT
	   client_code,
#endif
	   compile_time, ctime4(&start_time));

#ifdef REPORT_PIAIP_MODULES
    outs("\n" ANSI_COLOR(1;30)
	    "Modules powered by piaip:\n"
	    "\ttelnet protocol, ALOHA fixer, BRC v3\n"
#if defined(USE_PIAIP_MORE) || defined(USE_PMORE)
	    "\tpmore (piaip's more) 2007 w/Movie\n"
#endif
#ifdef HAVE_GRAYOUT
	    "\tGrayout Advanced Control �H�J�H�X�S�Ĩt��\n"
#endif 
#ifdef EDITPOST_SMARTMERGE
	    "\tSmart Merge �פ�۰ʦX��\n"
#endif
#ifdef EXP_EDIT_UPLOAD
	    "\t(EXP) Editor Uploader ����W��\n"
#endif
#if defined(USE_PFTERM)
	    "\t(EXP) pfterm (piaip's flat terminal, Perfect Term)\n"
#endif
#if defined(USE_BBSLUA)
	    "\t(EXP) BBS-Lua\n"
#endif
	    ANSI_RESET
	    );
#endif // REPORT_PIAIP_MODULES

    if (HasUserPerm(PERM_SYSOP)) {
	struct rusage ru;
#ifdef __linux__
	int vmdata=0, vmstk=0;
	FILE * fp;
	char buf[128];
	if ((fp = fopen("/proc/self/status", "r"))) {
	    while (fgets(buf, 128, fp)) {
		sscanf(buf, "VmData: %d", &vmdata);
		sscanf(buf, "VmStk: %d", &vmstk);
	    }
	    fclose(fp);
	}
#endif
	getrusage(RUSAGE_SELF, &ru);
	prints("�O����ζq: "
#ifdef IA32
	       "sbrk: %u KB, "
#endif
#ifdef __linux__
	       "VmData: %d KB, VmStk: %d KB, "
#endif
	       "idrss: %d KB, isrss: %d KB\n",
#ifdef IA32
	       ((unsigned int)sbrk(0) - 0x8048000) / 1024,
#endif
#ifdef __linux__
	       vmdata, vmstk,
#endif
	       (int)ru.ru_idrss, (int)ru.ru_isrss);
	prints("CPU �ζq:   %ld.%06ldu %ld.%06lds",
	       (long int)ru.ru_utime.tv_sec, 
	       (long int)ru.ru_utime.tv_usec,
	       (long int)ru.ru_stime.tv_sec, 
	       (long int)ru.ru_stime.tv_usec);
#ifdef CPULIMIT
	prints(" (limit %d secs)", (int)(CPULIMIT * 60));
#endif
	outs("\n�S�O�Ѽ�:"
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
