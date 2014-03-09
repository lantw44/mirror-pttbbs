/* $Id$ */
#include "bbs.h"

#ifdef ASSESS

/* do (*num) + n, n is integer. */
inline static void inc(unsigned char *num, int n)
{
    if (n >= 0 && UCHAR_MAX - *num <= (unsigned)n)
	(*num) = UCHAR_MAX;
    else if (n < 0 && *num < -n)
	(*num) = 0;
    else
	(*num) += n;
}

static time4_t
adjust_badpost_time(time4_t timeremovebadpost) {
    time4_t min_timer;

    // reset timer to at least some days to prevent user directly
    // canceling his badpost.
    syncnow();
    min_timer = (now - (BADPOST_CLEAR_DURATION - BADPOST_MIN_CLEAR_DURATION) *
                 DAY_SECONDS);
    if (timeremovebadpost < min_timer)
        timeremovebadpost = min_timer;

    return timeremovebadpost;
}

int
inc_badpost(const char *userid, int num) {
    userec_t xuser;
    int uid = getuser(userid, &xuser);

    if (uid <= 0)
        return 0;

    xuser.timeremovebadpost = adjust_badpost_time(xuser.timeremovebadpost);
    xuser.badpost += num;
    passwd_sync_update(uid, &xuser);
    return xuser.badpost;
}

static char * const badpost_reason[] = {
    "�s�i", "�������", "�H������"
};

#define DIM(x)	(sizeof(x)/sizeof(x[0]))

int assign_badpost(const char *userid, fileheader_t *fhdr,
	const char *newpath, const char *comment)
{
    char genbuf[STRLEN];
    char reason[STRLEN];
    char rptpath[PATHLEN];
    int i, tusernum = searchuser(userid, NULL);

    strcpy(rptpath, newpath);
    assert(tusernum > 0 && tusernum < MAX_USERS);
    move(b_lines - 2, 0);
    clrtobot();
    for (i = 0; i < (int)DIM(badpost_reason); i++)
	prints("%d.%s ", i + 1, badpost_reason[i]);
    prints("%d.%s ", i + 1, "��L");
    prints("0.�����H�� ");
    do {
        getdata(b_lines - 1, 0, "�п��: ", genbuf, 2, NUMECHO);
        i = genbuf[0] - '1';
        if (i == -1) {
            vmsg("�����]�w�H��C");
            return -1;
        }
        if (i < 0 || i > (int)DIM(badpost_reason))
            bell();
        else
            break;
    } while (1);

    if (i < (int)DIM(badpost_reason)) {
        snprintf(reason, sizeof(reason), "%s", badpost_reason[i]);
    } else if (i == DIM(badpost_reason)) {
        while (!getdata(b_lines, 0, "�п�J��]", reason, 50, DOECHO)) {
            // ��� comment �ثe�i�H���ӡA���Dcomment �媽���R���ҥH�S�k cancel
            if (comment) {
                vmsg("�����]�w�H��C");
                return -1;
            }
            bell();
            continue;
        }
    }
    assert(i >= 0 && i <= (int)DIM(badpost_reason));

    sprintf(genbuf,"�H%s��h�^(%s)", comment ? "��" : "", reason);

    if (fhdr) strncat(genbuf, fhdr->title, 64-strlen(genbuf));

#ifdef USE_COOLDOWN
    add_cooldowntime(tusernum, 60);
    add_posttimes(tusernum, 15); //Ptt: �ᵲ post for 1 hour
#endif

    if (!(inc_badpost(userid, 1) % 5)){
	userec_t xuser;
	post_violatelaw(userid, BBSMNAME "�t��ĵ��",
		"�H��֭p 5 �g", "�@��@�i");
	mail_violatelaw(userid, BBSMNAME "�t��ĵ��",
		"�H��֭p 5 �g", "�@��@�i");
	kick_all(userid);
	passwd_sync_query(tusernum, &xuser);
	xuser.money = moneyof(tusernum);
	xuser.vl_count++;
	xuser.userlevel |= PERM_VIOLATELAW;
	xuser.timeviolatelaw = now;
	passwd_sync_update(tusernum, &xuser);
    }

    if (!comment) {
        log_filef(newpath, LOG_CREAT, "�� BadPost Reason: %s\n", reason);
    }

#ifdef BAD_POST_RECORD
    // we also change rptpath because such record contains more information
    {
	int rpt_bid = getbnum(BAD_POST_RECORD);
	if (rpt_bid > 0) {
	    fileheader_t report_fh;
	    char rptdir[PATHLEN];
	    FILE *fp;

	    setbpath(rptpath, BAD_POST_RECORD);
	    stampfile(rptpath, &report_fh);

	    strcpy(report_fh.owner, "[" BBSMNAME "ĵ�]");
	    snprintf(report_fh.title, sizeof(report_fh.title),
		    "%s �O %s �O�D���� %s �@�g�H%s��",
		    currboard, cuser.userid, userid, comment ? "��" : "");
	    Copy(newpath, rptpath);
	    fp = fopen(rptpath, "at");

	    if (fp)
	    {
		fprintf(fp, "\n�H���]: %s\n", genbuf);
		if (comment)
		    fprintf(fp, "\n�Q�H���嶵��:\n%s", comment);
		fprintf(fp, "\n");
		fclose(fp);
	    }

	    setbdir(rptdir, BAD_POST_RECORD);
	    append_record(rptdir, &report_fh, sizeof(report_fh));

	    touchbtotal(rpt_bid);
	}
    }
#endif /* defined(BAD_POST_RECORD) */

    sendalert(userid,  ALERT_PWD_PERM);
    mail_id(userid, genbuf, rptpath, cuser.userid);

    return 0;
}

int
reassign_badpost(const char *userid) {
    // �H�h�_�@�͡A�ɦH���h�C�ɦH�L�q���A�Τ�d�����C
    // �Τ�Y�Q�ɦH�֡A�F�o���|�t�ΡC
    //
    // �ҥH�A�p�G�u���n�ɦH�A�@�w�n�i���ϥΪ̡C
    userec_t u;
    char title[TTLEN+1];
    char msg[512];
    char buf[10];
    char reason[50];
    int orig_badpost = 0;
    int uid;

    vs_hdr2(" �H��ץ� ", userid);
    if ((uid = getuser(userid, &u)) == 0) {
        vmsgf("�䤣��ϥΪ� %s�C", userid);
        return -1;
    }
    orig_badpost = u.badpost;
    prints("\n�ϥΪ� %s ���H��ƥثe��: %d\n", userid, u.badpost);
    snprintf(buf, sizeof(buf), "%d", u.badpost);
    if (!getdata_str(5, 0, "�վ�H��ƥج�: ", buf, sizeof(buf), DOECHO, buf) ||
        atoi(buf) == u.badpost) {
        vmsg("�H��ƥؤ��ܡA���ܰʡC");
        return 0;
    }

    // something changed.
    u.badpost = atoi(buf);
    if (u.badpost > orig_badpost) {
        u.timeremovebadpost = adjust_badpost_time(u.timeremovebadpost);
    }
    prints("\n�ϥΪ� %s ���H��Y�N�� %d �אּ %d�C�п�J�z��(�|�H���ϥΪ�)\n",
           userid, orig_badpost, u.badpost);
    if (!getdata(7, 0, "�z��: ", reason, sizeof(reason), DOECHO)) {
        vmsg("���~: ����L�z�ѡC");
        return -1;
    }

    move(6, 0); clrtobot();
    prints("�ϥΪ� %s ���H��� %d �אּ %d�C\n�z��: %s\n",
           userid, orig_badpost, u.badpost, reason);
    if (!getdata(9, 0, "�T�w�H [y/N]", buf, 3, LCECHO) || buf[0] != 'y') {
        vmsg("���~: ���T�w�A���C");
        return -1;
    }

    // GOGOGO
    snprintf(msg, sizeof(msg),
             "   ����" ANSI_COLOR(1;32) "%s" ANSI_RESET "��" ANSI_COLOR(1;32)
             "%s" ANSI_RESET "���H��q" ANSI_COLOR(1;35) "%d" ANSI_RESET
             "�令" ANSI_COLOR(1;35) "%d" ANSI_RESET "\n"
             "   " ANSI_COLOR(1;37) "�ק�z�ѬO�G%s" ANSI_RESET,
             cuser.userid, u.userid, orig_badpost, u.badpost, reason);
    snprintf(title, sizeof(title),
             "[�w�����i] ����%s�ק�%s�H����i", cuser.userid, u.userid);
    post_msg(BN_SECURITY, title, msg, "[�t�Φw����]");
    mail_log2id_text(u.userid, "[�t�γq��] �H���ܧ�", msg,
                     "[�t�Φw����]", NA);
    passwd_sync_update(uid, &u);
    kick_all(u.userid);

    return 0;
}

// XXX ����ثe���]�p�@��ӴN�O�L�k�޲z�A
// ���L�b���g��M�u��^���t�Ϋe�]�O�S��k���ơA
// ���g��������
// XXX �O�D�y���έ��ƦH�h�����D�ثe��@�]���H�ѨM
// �i���@ "�h�s�i����r" �������b
// �`���o�q code �N����ۧa orz
int
bad_comment(const char *fn)
{
    FILE *fp = NULL;
    char buf[ANSILINELEN];
    char uid[IDLEN+1];
    int done = 0;
    int i = 0, c;

    vs_hdr("�H����");
    usercomplete("�п�J�n�H���媺 ID: ", uid);
    if (!*uid)
	return -1;

    fp = fopen(fn, "rt");
    if (!fp)
	return -1;

    vs_hdr2(" �H���� ", uid);
    // search file for it
    while (fgets(buf, sizeof(buf), fp) && *buf)
    {
	if (strstr(buf, uid) == NULL)
	    continue;
	if (strstr(buf, ANSI_COLOR(33)) == NULL)
	    continue;

	// found something, let's show it
	move(2, 0); clrtobot();
	prints("�� %d ������:\n", ++i);
	outs(buf);

	move (5, 0);
	outs("�аݬO�n�H�o�ӱ���ܡH(Y:�T�w,N:��U��,Q:���}) [y/N/q]: ");
	c = vkey();
	if (isascii(c)) c = tolower(c);
	if (c == 'q')
	    break;
	if (c != 'y')
	    continue;

	if (assign_badpost(uid, NULL, fn, buf) != 0)
	    continue;

	done = 1;
	vmsg("�w�H����C");
	break;
    }
    fclose(fp);

    if (!done)
    {
	vmsgf("�䤣��䥦�u%s�v������F", uid);
	return -1;
    }

    return 0;
}

#endif // ASSESS
