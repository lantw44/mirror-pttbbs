/* $Id$ */
#include "bbs.h"
#ifdef PLAY_ANGEL

// PTT-BBS Angel System

#include "daemons.h"
#define FN_ANGELMSG "angelmsg"

/////////////////////////////////////////////////////////////////////////////
// Angel Beats! Client

// this works for simple requests: (return 1/angel_uid for success, 0 for fail
static int 
angel_beats_do_request(int op, int master_uid, int angel_uid) {
    int fd;
    int ret = 1;
    angel_beats_data req = {0};

    req.cb = sizeof(req);
    req.operation = op;
    req.master_uid = master_uid;
    req.angel_uid = angel_uid;
    assert(op != ANGELBEATS_REQ_INVALID);

    if ((fd = toconnect(ANGELBEATS_ADDR)) < 0)
        return -1;

    assert(req.operation != ANGELBEATS_REQ_INVALID);

    if (towrite(fd, &req, sizeof(req)) < 0 ||
        toread (fd, &req, sizeof(req)) < 0 ||
        req.cb != sizeof(req)) {
        ret = -2;
    } else {
        ret = (req.angel_uid > 0) ? req.angel_uid : 1;
    }

    close(fd);
    return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Local Angel Service

void 
angel_notify_activity() {
    static time4_t t = 0;
    time4_t tick = now;

    // tick: every 1 minutes.
    tick -= tick % (1 * 60);

    // ping daemon only in different ticks.
    if (tick == t)
        return;
    t = tick;
    angel_beats_do_request(ANGELBEATS_REQ_HEARTBEAT, 0, usernum);
}

void 
angel_toggle_pause()
{
    // TODO record angels that don't do their job
    if (!HasUserPerm(PERM_ANGEL) || !currutmp)
	return;
    currutmp->angelpause ++;
    currutmp->angelpause %= ANGELPAUSE_MODES;
}

void
angel_parse_nick_fp(FILE *fp, char *nick, int sznick)
{
    char buf[PATHLEN];
    // should be in first line
    rewind(fp);
    *buf = 0;
    if (fgets(buf, sizeof(buf), fp))
    {
	// verify first line
	if (buf[0] == '%' && buf[1] == '%' && buf[2] == '[')
	{
	    chomp(buf+3);
	    strlcpy(nick, buf+3, sznick);
	}
    }
}

void
angel_load_my_fullnick(char *buf, int szbuf)
{
    char fn[PATHLEN];
    FILE *fp = NULL;

    *buf = 0;
    setuserfile(fn, FN_ANGELMSG);
    if ((fp = fopen(fn, "rt")))
    {
	angel_parse_nick_fp(fp, buf, szbuf);
	fclose(fp);
    }
    strlcat(buf, "�p�Ѩ�", szbuf);
}

// cache my angel's nickname
static char _myangel[IDLEN+1] = "",
	    _myangel_nick[IDLEN+1] = "";
static time4_t _myangel_touched = 0;
static char _valid_angelmsg = 0;

void 
angel_reload_nick()
{
    char reload = 0;
    char fn[PATHLEN];
    time4_t ts = 0;
    FILE *fp = NULL;

    fn[0] = 0;
    // see if we have angel id change (reload whole)
    if (strcmp(_myangel, cuser.myangel) != 0)
    {
	strlcpy(_myangel, cuser.myangel, sizeof(_myangel));
	reload = 1;
    }
    // see if we need to check file touch date
    if (!reload && _myangel[0] && _myangel[0] != '-')
    {
	sethomefile(fn, _myangel, FN_ANGELMSG);
	ts = dasht(fn);
	if (ts != -1 && ts > _myangel_touched)
	    reload = 1;
    }
    // if no need to reload, reuse current data.
    if (!reload)
    {
	// vmsg("angel_data: no need to reload.");
	return;
    }

    // reset cache
    _myangel_touched = ts;
    _myangel_nick[0] = 0;
    _valid_angelmsg = 0;

    // quick check
    if (_myangel[0] == '-' || !_myangel[0])
	return;

    // do reload data.
    if (!fn[0])
    {
	sethomefile(fn, _myangel, FN_ANGELMSG);
	ts = dasht(fn);
	_myangel_touched = ts;
    }

    assert(*fn);
    // complex load
    fp = fopen(fn, "rt");
    if (fp)
    {
	_valid_angelmsg = 1;
	angel_parse_nick_fp(fp, _myangel_nick, sizeof(_myangel_nick));
	fclose(fp);
    }
}

const char * 
angel_get_nick()
{
    angel_reload_nick();
    return _myangel_nick;
}

static int
do_changeangel(int force) {
    char buf[4];
    static time_t last_time = 0;
    const char *prompt = "�n�O�����A�U���I�s�ɷ|�q�W�u���ѨϤ���X�s���p�Ѩ�";

    /* cuser.myangel == "-" means banned for calling angel */
    if (cuser.myangel[0] == '-')
        return 0;

    if (!cuser.myangel[0]) {
        vmsg(prompt);
        return 0;
    }

    // TODO Allow changing only if user really tried to contact angel.
#ifdef ANGEL_CHANGE_TIMELIMIT_MINS
    if (force)
        last_time = 0;

    if (last_time &&
        (now - last_time < ANGEL_CHANGE_TIMELIMIT_MINS * 60)) {
        vmsgf("�C���󴫤p�Ѩϳֶ̤��j %d �����C",
              ANGEL_CHANGE_TIMELIMIT_MINS);
        return 0;
    }

#endif

    mvouts(b_lines - 3, 0, "\n"
           "�Ъ`�N�Y���I�s�L�ثe���p�ѨϴN�ӽЧ󴫡A�ܥi��|�A����P�ӤѨ�\n");
    getdata(b_lines - 1, 0,
	    "�󴫤p�Ѩϫ�N�L�k���^�F��I �O�_�n�󴫤p�ѨϡH [y/N]",
	    buf, 3, LCECHO);
    if (buf[0] == 'y') {
	log_filef(BBSHOME "/log/changeangel.log",LOG_CREAT,
                  "%s �p�D�H %s ���� %s �p�Ѩ�\n",
                  Cdatelite(&now), cuser.userid, cuser.myangel);
        if (cuser.myangel[0])
            angel_beats_do_request(ANGELBEATS_REQ_REMOVE_LINK,
                    usernum, searchuser(cuser.myangel, NULL));
	pwcuSetMyAngel("");
        last_time = now;
        vmsg(prompt);
    }
    return 0;
}

int a_changeangel(void) {
    return do_changeangel(0);
}

int 
a_angelmsg(){
    char msg[3][75] = { "", "", "" };
    char nick[10] = "";
    char buf[512];
    int i;
    FILE* fp;

    move(1, 0); clrtobot();
    setuserfile(buf, "angelmsg");
    fp = fopen(buf, "r");
    if (fp) {
	i = 0;
	if (fgets(msg[0], sizeof(msg[0]), fp)) {
	    chomp(msg[0]);
	    if (strncmp(msg[0], "%%[", 3) == 0) {
		strlcpy(nick, msg[0] + 3, 7);
		move(4, 0);
		prints("�즳�ʺ١G%s�p�Ѩ�", nick);
		msg[0][0] = 0;
	    } else
		i = 1;
	} else
	    msg[0][0] = 0;

	move(5, 0);
	outs("�즳�d���G\n");
	if(msg[0][0])
	    outs(msg[0]);
	for (; i < 3; ++i) {
	    if(fgets(msg[i], sizeof(msg[0]), fp)) {
		outs(msg[i]);
		chomp(msg[i]);
	    } else
		break;
	}
	fclose(fp);
    }

    getdata_buf(11, 0, "�p�Ѩϼʺ١G", nick, 7, 1);
    do {
	move(12, 0);
	clrtobot();
	outs("���b���ɭԭn��p�D�H������O�H"
	     "�̦h�T��A��[Enter]����");
        // the -1 is for newline and we use to fgets() to read later
	for (i = 0; i < 3 &&
		getdata_buf(14 + i, 0, "�G", msg[i], sizeof(msg[i])-1, DOECHO);
		++i);
	getdata(b_lines - 2, 0, "(S)�x�s (E)���s�ӹL (Q)�����H[S]",
		buf, 4, LCECHO);
    } while (buf[0] == 'e');
    if (buf[0] == 'q')
	return 0;
    setuserfile(buf, "angelmsg");
    if (msg[0][0] == 0)
	unlink(buf);
    else {
	FILE* fp = fopen(buf, "w");
	if (!fp)
	    return 0;
	if(nick[0])
	    fprintf(fp, "%%%%[%s\n", nick);
	for (i = 0; i < 3 && msg[i][0]; ++i) {
	    fputs(msg[i], fp);
	    fputc('\n', fp);
	}
	fclose(fp);
    }
    return 0;
}

int a_angelreport() {
    angel_beats_report rpt = {0};
    angel_beats_data   req = {0};
    int fd;

    vs_hdr2(" Angel Beats! �ѨϤ��| ", " �ѨϪ��A���i ");
    outs("\n");

    if ((fd = toconnect(ANGELBEATS_ADDR)) < 0) {
        outs("��p�A�L�k�s�u�ܤѨϤ��|�C\n"
             "�Ц� " BN_BUGREPORT " �ݪO�q������޲z�H���C\n");
        pressanykey();
        return 0;
    }

    req.cb = sizeof(req);
    req.operation = ANGELBEATS_REQ_REPORT;
    req.master_uid = usernum;

    if (HasUserPerm(PERM_ANGEL))
        req.angel_uid = usernum;

    if (towrite(fd, &req, sizeof(req)) < 0 ||
        toread(fd, &rpt, sizeof(rpt)) < 0 ||
        rpt.cb != sizeof(rpt)) {
        outs("��p�A�ѨϤ��|�s�u���`�C\n"
                "�Ц� " BN_BUGREPORT " �ݪO�q������޲z�H���C\n");
    } else {
        prints(
            "\t �{�b�ɶ�: %s\n\n"
            "\n\t �t�Τ��w�n�O���ѨϬ� %d ��C\n"
            "\n\t �ثe�� %d ��ѨϦb�u�W�A�䤤 %d �쯫�٩I�s�����]�w�}��F\n",
            Cdatelite(&now),
            rpt.total_angels,
            rpt.total_online_angels,
            rpt.total_active_angels);

        if (HasUserPerm(PERM_SYSOP)) {
            prints(
                "\n\t �W�u�ѨϤ��A�֦��p�D�H�ƥس̤֬� %d ��A�̦h�� %d ��\n"
                "\n\t �W�u�B�}�񦬥D�H���ѨϤ��A�D�H�̤� %d ��A�̦h %d ��\n",
                rpt.min_masters_of_online_angels,
                rpt.max_masters_of_online_angels,
                rpt.min_masters_of_active_angels,
                rpt.max_masters_of_active_angels);
        } else {
            // some people with known min/max signature may leak their own 
            // identify and then complain about privacy. well, I believe this
            // is their own fault but anyway let's make them happy
            // TODO avg+std is better?
            double base1 = rpt.min_masters_of_online_angels,
                   base2 = rpt.min_masters_of_active_angels;
            if (!base1) base1 = 1;
            if (!base2) base2 = 1;
            prints(
                    "\n\t �W�u�ѨϤ��A�֦��̦h�p�D�H�ƥجO�̤֪� %.1f ���F\n"
                    "\n\t �W�u�}�񦬥D�H���ѨϤ��A�D�H�ƥخt���� %.1f ��\n",
                    rpt.max_masters_of_online_angels/base1,
                    rpt.max_masters_of_active_angels/base2);
        }

        if (HasUserPerm(PERM_ANGEL)) {
            if (currutmp->angelpause != ANGELPAUSE_NONE)
                prints("\n\t �ѩ�z�ثe�ڦ��p�D�H�ҥH�L�����T\n");
            else if (rpt.my_active_index == 0)
                prints("\n\t �z���G���䥦�n�J�����D�H�A�ҥH�ثe�L�p�Ѩ϶���C\n");
            else
                prints("\n\t �z���u�W�p�Ѩ϶��쬰 %d�C"
                       "\n\t ������i��|�]�䥦�p�ѨϤW�u�Χ��ܩI�s�����ܤj\n",
                       rpt.my_active_index);
            prints("\n\t �z�ثe�j���� %d ��p�D�H�C\n", rpt.my_active_masters);
        }
    }
    close(fd);
    pressanykey();
    return 0;
}

int a_angelreload() {
    vs_hdr2(" Angel Beats! �ѨϤ��| ", " ����Ѩϸ�T ");
    outs("\n"
         "\t �ѩ󭫷s�έp�ѨϤp�D�H�ƥثD�`��ɶ��A�ҥH�ثe�t��\n"
         "\t �@��u�|��s�w�����ѨϡC  �Y���観�s�W�ΧR���ѨϮ�\n"
         "\t �Цb�粒�Ҧ��Ѩ��v����Q�Φ��\\��ӭ���Ѩϸ�T�C\n"
         "\n"
         "\t �t�~�ѩ󭫾�ɩҦ���ѨϦ������\\�ೣ�|�Ȱ� 30 �� ~ �@������A\n"
         "\t �Ф��n�S�ƴN����A�ӬO�u�����վ�ѨϦW���~�ϥΡC\n");
    if (vans("�аݽT�w�n����Ѩϸ�T�F��? [y/N]: ") != 'y') {
        vmsg("���C");
        return 0;
    }

    move(1, 0); clrtobot();
    outs("\n�s�u��...\n"); refresh();

    if (angel_beats_do_request(ANGELBEATS_REQ_RELOAD, usernum, 0) < 0) {
        outs("��p�A�L�k�s�u�ܤѨϤ��|�C\n"
             "�Ц� " BN_BUGREPORT " �ݪO�q������޲z�H���C\n");
    } else {
        outs("\n����!\n");
    }

    pressanykey();
    return 0;
}

inline int
angel_reject_me(userinfo_t * uin){
    int* iter = uin->reject;
    int unum;
    while ((unum = *iter++)) {
	if (unum == currutmp->uid) {
            // �W�Ŧn��?
            if (intbsearch(unum, uin->myfriend, uin->nFriends))
                return 0;
	    return 1;
	}
    }
    return 0;
}

static int
FindAngel(void){

    int angel_uid = 0;
    userinfo_t *angel = NULL;
    int retries = 2;
   
    do {
        angel_uid = angel_beats_do_request(ANGELBEATS_REQ_SUGGEST_AND_LINK,
                                           usernum, 0);
        if (angel_uid < 0)  // connection failure
            return -1;
        if (angel_uid > 0)
            angel = search_ulist(angel_uid);
    } while ((retries-- < 1) && !(angel && (angel->userlevel & PERM_ANGEL)));

    if (angel) {
        // got new angel
        pwcuSetMyAngel(angel->userid);
        return 1;
    }

    return 0;
}

#ifdef BN_NEWBIE
static inline void
GotoNewHand(){
    char old_board[IDLEN + 1] = "";
    int canRead = 1;

    if (currutmp && currutmp->mode == EDITING)
	return;

    // usually crashed as 'assert(currbid == brc_currbid)'
    if (currboard[0]) {
	strlcpy(old_board, currboard, IDLEN + 1);
	currboard = ""; // force enter_board
    }

    if (enter_board(BN_NEWBIE) == 0)
	canRead = 1;

    if (canRead)
	Read();

    if (canRead && old_board[0])
	enter_board(old_board);
}
#endif


static inline void
NoAngelFound(const char* msg){
    // don't worry about the screen - 
    // it should have been backuped before entering here.
    
    grayout(0, b_lines-3, GRAYOUT_DARK);
    move(b_lines-4, 0); clrtobot();
    outs(msg_separator);
    move(b_lines-2, 0);
    if (!msg)
	msg = "�A���p�Ѩϲ{�b���b�u�W";
    outs(msg);
#ifdef BN_NEWBIE
    if (currutmp == NULL || currutmp->mode != EDITING)
	outs("�A�Х��b�s��O�W�M�䵪�שΫ� Ctrl-P �o��");
    if (vmsg("�Ы����N���~��A�Y�Q�����i�J�s��O�o��Ы� p") == 'p')
	GotoNewHand();
#else
    pressanykey();
#endif
}

static inline void
AngelNotOnline(){
    char buf[PATHLEN];
    FILE *fp;

    // use cached angel data (assume already called before.)
    // angel_reload_nick();
    if (!_valid_angelmsg)
    {
	NoAngelFound(NULL);
	return;
    }

    // valid angelmsg is ready for being loaded.
    sethomefile(buf, cuser.myangel, FN_ANGELMSG);
    fp = fopen(buf, "rt");
    if (!fp)
    {
	// safer
	NoAngelFound(NULL);
	return;
    }
    clear();
    showtitle("�p�Ѩϯd��", BBSNAME);
    move(4, 0);
    buf[0] = 0;
    prints("�z��%s�p�Ѩϲ{�b���b�u�W", _myangel_nick);

    outs("\n͢�d�����A�G\n");
    outs(ANSI_COLOR(1;31;44) "��s�w�w�w�w�w�w�w�w�w�w�w�w�w�w�t" ANSI_COLOR(37) ""
	    "�p�Ѩϯd��" ANSI_COLOR(31) "�u�w�w�w�w�w�w�w�w�w�w�w�w�w�w�s��" ANSI_RESET "\n");
    outs(ANSI_COLOR(1;31) "�~�t" ANSI_COLOR(32) " �p�Ѩ�                          "
	    "                                     " ANSI_COLOR(31) "�u��" ANSI_RESET "\n");
    fgets(buf, sizeof(buf), fp); // skip first line: entry for nick
    while (fgets(buf, sizeof(buf), fp))
    {
	chomp(buf);
	prints(ANSI_COLOR(1;31) "�x" ANSI_RESET "%-74.74s" ANSI_COLOR(1;31) "�x" ANSI_RESET "\n", buf);
    }
    fclose(fp);
    outs(ANSI_COLOR(1;31) "���s�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"
	    "�w�w�w�w�w�w�w�w�w�w�w�w�w�s��" ANSI_RESET "\n");
    outs(ANSI_COLOR(1;31;44) "��r�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"
	    "�w�w�w�w�w�w�w�w�w�w�w�w�w�w�r��" ANSI_RESET "\n");
    prints("%55s%s", "�d�����: ", Cdatelite(&_myangel_touched));


    move(b_lines - 4, 0);
#ifdef BN_NEWBIE
    outs("�{�ɧ䤣��p�Ѩϥi��s�⪩(" BN_NEWBIE ")\n"
#endif
#ifdef BN_ANGELPRAY
	 "�Q�d�����p�ѨϽШ�\\�@��(" BN_ANGELPRAY ")\n"
#endif
#ifdef BN_ASKBOARD
	 "�Q��ݪO�b�����ܥi��(" BN_ASKBOARD ")\n"
#endif
	 "�Х��b�U�O�W�M�䵪�שΫ� Ctrl-P �o��");

    // Query if user wants to go to newbie board
    switch(tolower(vmsg("�Q�����ثe�b�u�W���p�ѨϽЫ� h, "
#ifdef BN_NEWBIE
                    "�i�s��O�Ы� p, "
#endif
                    "�䥦���N�����}"))) {
        case 'h':
            move(b_lines - 4, 0); clrtobot();
            do_changeangel(1);
            break;
#ifdef BN_NEWBIE
        case 'p':
            GotoNewHand();
            break;
#endif
    }
}

static void
TalkToAngel(){
    static char AngelPermChecked = 0;
    static userinfo_t* lastuent = NULL;
    userinfo_t *uent;

    if (strcmp(cuser.myangel, "-") == 0){
	NoAngelFound("�A�S���p�Ѩ�");
	return;
    }

    if (cuser.myangel[0] && !AngelPermChecked) {
	userec_t xuser;
	memset(&xuser, 0, sizeof(xuser));
	getuser(cuser.myangel, &xuser); // XXX if user doesn't exist
	if (!(xuser.userlevel & PERM_ANGEL))
	    pwcuSetMyAngel("");
    }
    AngelPermChecked = 1;

    if (cuser.myangel[0] == 0) {
        int ret = FindAngel();
        if (ret <= 0) {
            lastuent = NULL;
            NoAngelFound(
                (ret < 0) ? "��p�A�ѨϨt�άG�١A�Ц�" BN_BUGREPORT "�^���C" :
                            "�{�b�S���p�ѨϦb�u�W");
            return;
        }
    }

    // now try to load angel data.
    // This MUST be done before calling AngelNotOnline,
    // because it relies on this data.
    angel_reload_nick();

    uent = search_ulist_userid(cuser.myangel);
    if (uent == NULL || angel_reject_me(uent) || uent->mode == DEBUGSLEEPING){
	lastuent = NULL;
	AngelNotOnline();
	return;
    }

    // check angelpause: if talked then should accept.
    if (uent == lastuent) {
	// we've talked to angel.
	// XXX what if uentp reused by other? chance very, very low... 
	if (uent->angelpause >= ANGELPAUSE_REJALL)
	{
	    AngelNotOnline();
	    return;
	}
    } else {
	if (uent->angelpause) {
	    // lastuent = NULL;
	    AngelNotOnline();
	    return;
	}
    }

    more("etc/angel_usage", NA);

    /* �o�q�ܩγ\�i�H�b�p�ѨϦ^�����D�� show �X��
    move(b_lines - 1, 0);
    outs("�{�b�A��id����O�K�A�^���A���D���p�ѨϨä����D�A�O��       \n"
         "�A�i�H��ܤ��V���z�S�ۤv�����ӫO�@�ۤv                   ");
	 */

    {
	char xnick[IDLEN+1], prompt[IDLEN*2];
	snprintf(xnick, sizeof(xnick), "%s�p�Ѩ�", _myangel_nick);
	snprintf(prompt, sizeof(prompt), "��%s�p�Ѩ�: ", _myangel_nick);
	// if success, record uent.
	if (my_write(uent->pid, prompt, xnick, WATERBALL_ANGEL, uent)) {
	    lastuent = uent;
        }
    }
    return;
}

void
CallAngel(){
    static int      entered = 0;
    screen_backup_t old_screen;

    if (entered)
	return;
    entered = 1;
    scr_dump(&old_screen);
    TalkToAngel();
    scr_restore(&old_screen);
    entered = 0;
}

void
pressanykey_or_callangel(){
    int w = t_columns - 2; // see vtuikit.c, SAFE_MAX_COL

    if (!HasBasicUserPerm(PERM_LOGINOK) ||
        strcmp(cuser.myangel, "-") == 0) {
	pressanykey();
	return;
    }

    move(b_lines, 0); clrtoeol();

    // message string length = 38
    outs(VCLR_PAUSE_PAD " ");
    w -= 1 + 38;
    vpad(w / 2, VMSG_PAUSE_PAD);
    outs(VCLR_PAUSE     " �Ы� " ANSI_COLOR(36) "�ť���" 
           VCLR_PAUSE     " �~��A�� " ANSI_COLOR(36) "H"
           VCLR_PAUSE     " �I�s�p�ѨϨ�U " VCLR_PAUSE_PAD);
    vpad(w - w / 2, VMSG_PAUSE_PAD);
    outs(" " ANSI_RESET);

    if (tolower(vkey()) == 'h')
        CallAngel();

    move(b_lines, 0);
    clrtoeol();
}

#endif // PLAY_ANGEL
