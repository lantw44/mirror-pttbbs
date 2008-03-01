/* $Id$ */
#include "bbs.h"

/* �i�����y�Ŷ� */
int
m_loginmsg(void)
{
  char msg[100];
  move(21,0);
  clrtobot();
  if(SHM->loginmsg.pid && SHM->loginmsg.pid != currutmp->pid)
    {
      outs("�ثe�w�g���H�U�� �i�����y�]�w�Х���զn�A�]�w..");
      getmessage(SHM->loginmsg);
    }
  getdata(22, 0, 
     "�i�����y:��������,���z�Z�ϥΪ̬���,�]�w�������۰ʨ���,�T�w�n�]?(y/N)",
          msg, 3, LCECHO);

  if(msg[0]=='y' &&

     getdata_str(23, 0, "�]�w�i�����y:", msg, 56, DOECHO, SHM->loginmsg.last_call_in))
    {
          SHM->loginmsg.pid=currutmp->pid; /*�������h �N����race condition */
          strcpy(SHM->loginmsg.last_call_in, msg);
          strcpy(SHM->loginmsg.userid, cuser.userid);
    }
  return 0;
}

/* �ϥΪ̺޲z */
int
m_user(void)
{
    userec_t        xuser;
    int             id;
    char            genbuf[200];

    stand_title("�ϥΪ̳]�w");
    usercomplete(msg_uid, genbuf);
    if (*genbuf) {
	move(2, 0);
	if ((id = getuser(genbuf, &xuser))) {
	    user_display(&xuser, 1);
	    if( HasUserPerm(PERM_ACCOUNTS) )
		uinfo_query(&xuser, 1, id);
	    else
		pressanykey();
	} else {
	    outs(err_uid);
	    clrtoeol();
	    pressanykey();
	}
    }
    return 0;
}

static int retrieve_backup(userec_t *user)
{
    int     uid;
    char    src[PATHLEN], dst[PATHLEN];
    char    ans;

    if ((uid = searchuser(user->userid, user->userid))) {
	userec_t orig;
	passwd_query(uid, &orig);
	strlcpy(user->passwd, orig.passwd, sizeof(orig.passwd));
	setumoney(uid, user->money);
	passwd_update(uid, user);
	return 0;
    }

    ans = getans("�ثe�� PASSWD �ɨS���� ID�A�s�W�ܡH[y/N]");

    if (ans != 'y') {
	vmsg("�ثe�� PASSWDS �ɨS���� ID�A�Х��s�W���b��");
	return -1;
    }

    if (setupnewuser((const userec_t *)user) >= 0) {
	sethomepath(dst, user->userid);
	if (!dashd(dst)) {
	    snprintf(src, sizeof(src), "tmp/%s", user->userid);
	    if (!dashd(src) || !Rename(src, dst))
		mkuserdir(user->userid);
	}
	return 0;
    }
    return -1;
}

static int
search_key_user(const char *passwdfile, int mode)
{
    userec_t        user;
    int             ch;
    int             unum = 0;
    FILE            *fp1 = fopen(passwdfile, "r");
    char            friendfile[128]="", key[22], *keymatch;
    int		    keytype = 0;
    char	    isCurrentPwd = 0;

    isCurrentPwd = (strcmp(passwdfile, FN_PASSWD) == 0) ? 1 : 0;
    assert(fp1);
    clear();
    if (!mode)
    {
	getdata(0, 0, "�п�Jid :", key, sizeof(key), DOECHO);
    } else {
	// improved search
	getdata(0, 0, "�j�M�������?"
		"([0]���� 1.ID 2.�m�W 3.�ʺ� 4.�a�} 5.email 6.IP 7.�{��/�q��) ",
		key, 2, DOECHO);
	if (isascii(key[0]) && isdigit(key[0]))
	    keytype = key[0] - '0';
	if (keytype < 0 || keytype > 7)
	    keytype = 0;
	getdata(0, 0, "�п�J����r: ", key, sizeof(key), DOECHO);
    }

    if(!key[0]) {
	fclose(fp1);
	return 0;
    }
    while ((fread(&user, sizeof(user), 1, fp1)) > 0 && unum++ < MAX_USERS) {

	// skip empty records
	if (!user.userid[0])
	    continue;

	if (!(unum & 0xFF)) {
	    move(1, 0);
	    prints("�� [%d] �����\n", unum);
	    refresh();
	}

        keymatch = NULL;

	if (!mode)
	{
	    // only verify id
	    if (!strcasecmp(user.userid, key))
		keymatch = user.userid; 
	} else {
	    // search by keytype
	    if ((!keytype || keytype == 1) &&
		strcasestr(user.userid, key))
		keymatch = user.userid;
	    else if ((!keytype || keytype == 2) &&
		strcasestr(user.realname, key))
		keymatch = user.realname;
	    else if ((!keytype || keytype == 3) &&
		strcasestr(user.nickname, key))
		keymatch = user.nickname;
	    else if ((!keytype || keytype == 4) &&
		strcasestr(user.address, key))
		keymatch = user.address;
	    else if ((!keytype || keytype == 5) &&
		strcasestr(user.email, key))
		keymatch = user.email;
	    else if ((!keytype || keytype == 6) &&
		strcasestr(user.lasthost, key))
		keymatch = user.lasthost;
	    else if ((!keytype || keytype == 7) &&
		strcasestr(user.justify, key))
		keymatch = user.justify;
	    else if ((!keytype) &&
		strcasestr(user.mychicken.name, key))
		keymatch = user.mychicken.name; 
	}

        if(keymatch) {
	    move(1, 0);
	    prints("�� [%d] �����\n", unum);
	    refresh();

	    user_display(&user, 1);
	    // user_display does not have linefeed in tail.

	    if (isCurrentPwd && HasUserPerm(PERM_ACCOUNTS))
		uinfo_query(&user, 1, unum);
	    else
		outs("\n");

	    outs(ANSI_COLOR(44) "               �ť���" \
		 ANSI_COLOR(37) ":�j�M�U�@��          " \
		 ANSI_COLOR(33)" Q" ANSI_COLOR(37)": ���}");
	    outs(mode ? 
                 "      A: add to namelist " ANSI_RESET " " :
		 "      S: ���γƥ����    " ANSI_RESET " ");
	    while (1) {
		while ((ch = igetch()) == 0);
                if (ch == 'a' || ch=='A' )
                  {
                   if(!friendfile[0])
                    {
                     friend_special();
                     setfriendfile(friendfile, FRIEND_SPECIAL);
                    }
                   friend_add(user.userid, FRIEND_SPECIAL, keymatch);
                   break;
                  }
		if (ch == ' ')
		    break;
		if (ch == 'q' || ch == 'Q') {
		    fclose(fp1);
		    return 0;
		}
		if (ch == 's' && !mode) {
		    if (retrieve_backup(&user) >= 0) {
			fclose(fp1);
			return 0;
		    }
		}
	    }
	}
    }

    fclose(fp1);
    return 0;
}

/* �H���N key �M��ϥΪ� */
int
search_user_bypwd(void)
{
    search_key_user(FN_PASSWD, 1);
    return 0;
}

/* �M��ƥ����ϥΪ̸�� */
int
search_user_bybakpwd(void)
{
    char           *choice[] = {
	"PASSWDS.NEW1", "PASSWDS.NEW2", "PASSWDS.NEW3",
	"PASSWDS.NEW4", "PASSWDS.NEW5", "PASSWDS.NEW6",
	"PASSWDS.BAK"
    };
    int             ch;

    clear();
    move(1, 1);
    outs("�п�J�A�n�ΨӴM��ƥ����ɮ� �Ϋ� 'q' ���}\n");
    outs(" [" ANSI_COLOR(1;31) "1" ANSI_RESET "]�@�ѫe,"
	 " [" ANSI_COLOR(1;31) "2" ANSI_RESET "]��ѫe," 
	 " [" ANSI_COLOR(1;31) "3" ANSI_RESET "]�T�ѫe\n");
    outs(" [" ANSI_COLOR(1;31) "4" ANSI_RESET "]�|�ѫe,"
	 " [" ANSI_COLOR(1;31) "5" ANSI_RESET "]���ѫe,"
	 " [" ANSI_COLOR(1;31) "6" ANSI_RESET "]���ѫe\n");
    outs(" [7]�ƥ���\n");
    do {
	move(5, 1);
	outs("��� => ");
	ch = igetch();
	if (ch == 'q' || ch == 'Q')
	    return 0;
    } while (ch < '1' || ch > '7');
    ch -= '1';
    if( access(choice[ch], R_OK) != 0 )
	vmsg("�ɮפ��s�b");
    else
	search_key_user(choice[ch], 0);
    return 0;
}

static void
bperm_msg(const boardheader_t * board)
{
    prints("\n�]�w [%s] �ݪO��(%s)�v���G", board->brdname,
	   board->brdattr & BRD_POSTMASK ? "�o��" : "�\\Ū");
}

unsigned int
setperms(unsigned int pbits, const char * const pstring[])
{
    register int    i;

    move(4, 0);
    for (i = 0; i < NUMPERMS / 2; i++) {
	prints("%c. %-20s %-15s %c. %-20s %s\n",
	       'A' + i, pstring[i],
	       ((pbits >> i) & 1 ? "��" : "��"),
	       i < 10 ? 'Q' + i : '0' + i - 10,
	       pstring[i + 16],
	       ((pbits >> (i + 16)) & 1 ? "��" : "��"));
    }
    clrtobot();
    while (
       (i = getkey("�Ы� [A-5] �����]�w�A�� [Return] �����G"))!='\r')
    {
	if (isdigit(i))
	    i = i - '0' + 26;
	else if (isalpha(i))
	    i = tolower(i) - 'a';
	else {
	    bell();
	    continue;
	}

	pbits ^= (1 << i);
	move(i % 16 + 4, i <= 15 ? 24 : 64);
	outs((pbits >> i) & 1 ? "��" : "��");
    }
    return pbits;
}

#ifdef CHESSCOUNTRY
static void
AddingChessCountryFiles(const char* apath)
{
    char filename[PATHLEN];
    char symbolicname[PATHLEN];
    char adir[PATHLEN];
    FILE* fp;
    fileheader_t fh;

    setadir(adir, apath);

    /* creating chess country regalia */
    snprintf(filename, sizeof(filename), "%s/chess_ensign", apath);
    close(open(filename, O_CREAT | O_WRONLY, 0644));

    strlcpy(symbolicname, apath, sizeof(symbolicname));
    stampfile(symbolicname, &fh);
    symlink("chess_ensign", symbolicname);

    strcpy(fh.title, "�� �Ѱ���� (����R���A�t�λݭn)");
    strcpy(fh.owner, str_sysop);
    append_record(adir, &fh, sizeof(fileheader_t));

    /* creating member list */
    snprintf(filename, sizeof(filename), "%s/chess_list", apath);
    if (!dashf(filename)) {
	fp = fopen(filename, "w");
	assert(fp);
	fputs("�Ѱ��W\n"
		"�b��            ����    �[�J���        ���ũγQ�֫R��\n"
		"�w�w�w�w�w�w    �w�w�w  �w�w�w�w�w      �w�w�w�w�w�w�w\n",
		fp);
	fclose(fp);
    }

    strlcpy(symbolicname, apath, sizeof(symbolicname));
    stampfile(symbolicname, &fh);
    symlink("chess_list", symbolicname);

    strcpy(fh.title, "�� �Ѱꦨ���� (����R���A�t�λݭn)");
    strcpy(fh.owner, str_sysop);
    append_record(adir, &fh, sizeof(fileheader_t));

    /* creating profession photos' dir */
    snprintf(filename, sizeof(filename), "%s/chess_photo", apath);
    mkdir(filename, 0755);

    strlcpy(symbolicname, apath, sizeof(symbolicname));
    stampfile(symbolicname, &fh);
    symlink("chess_photo", symbolicname);

    strcpy(fh.title, "�� �Ѱ�Ӥ��� (����R���A�t�λݭn)");
    strcpy(fh.owner, str_sysop);
    append_record(adir, &fh, sizeof(fileheader_t));
}
#endif /* defined(CHESSCOUNTRY) */

/* �۰ʳ]�ߺ�ذ� */
void
setup_man(const boardheader_t * board, const boardheader_t * oldboard)
{
    char            genbuf[200];

    setapath(genbuf, board->brdname);
    mkdir(genbuf, 0755);

#ifdef CHESSCOUNTRY
    if (oldboard == NULL || oldboard->chesscountry != board->chesscountry)
	if (board->chesscountry != CHESSCODE_NONE)
	    AddingChessCountryFiles(genbuf);
	// else // doesn't remove files..
#endif
}

void delete_symbolic_link(boardheader_t *bh, int bid)
{
    assert(0<=bid-1 && bid-1<MAX_BOARD);
    memset(bh, 0, sizeof(boardheader_t));
    substitute_record(fn_board, bh, sizeof(boardheader_t), bid);
    reset_board(bid);
    sort_bcache(); 
    log_usies("DelLink", bh->brdname);
}

int dir_cmp(const void *a, const void *b)
{
  return (atoi( &((fileheader_t *)a)->filename[2] ) -
          atoi( &((fileheader_t *)b)->filename[2] ));
}

void merge_dir(const char *dir1, const char *dir2, int isoutter)
{
     int i, pn, sn;
     fileheader_t *fh;
     char *p1, *p2, bakdir[128], file1[128], file2[128];
     strcpy(file1,dir1);
     strcpy(file2,dir2);
     if((p1=strrchr(file1,'/')))
	 p1 ++;
     else
	 p1 = file1;
     if((p2=strrchr(file2,'/')))
	 p2 ++;
     else
	 p2 = file2;

     pn=get_num_records(dir1, sizeof(fileheader_t));
     sn=get_num_records(dir2, sizeof(fileheader_t));
     if(!sn) return;
     fh= (fileheader_t *)malloc( (pn+sn)*sizeof(fileheader_t));
     get_records(dir1, fh, sizeof(fileheader_t), 1, pn);
     get_records(dir2, fh+pn, sizeof(fileheader_t), 1, sn);
     if(isoutter)
         {
             for(i=0; i<sn; i++)
               if(fh[pn+i].owner[0])
                   strcat(fh[pn+i].owner, "."); 
         }
     qsort(fh, pn+sn, sizeof(fileheader_t), dir_cmp);
     sprintf(bakdir,"%s.bak", dir1);
     Rename(dir1, bakdir);
     for(i=1; i<=pn+sn; i++ )
        {
         if(!fh[i-1].filename[0]) continue;
         if(i == pn+sn ||  strcmp(fh[i-1].filename, fh[i].filename))
	 {
                fh[i-1].recommend =0;
		fh[i-1].filemode |= 1;
                append_record(dir1, &fh[i-1], sizeof(fileheader_t));
		strcpy(p1, fh[i-1].filename);
                if(!dashf(file1))
		      {
			  strcpy(p2, fh[i-1].filename);
			  Copy(file2, file1);
		      } 
	 }
         else
                fh[i].filemode |= fh[i-1].filemode;
        }
     
     free(fh);
}

int
m_mod_board(char *bname)
{
    boardheader_t   bh, newbh;
    int             bid;
    char            genbuf[256], ans[4];

    bid = getbnum(bname);
    if (!bid || !bname[0] || get_record(fn_board, &bh, sizeof(bh), bid) == -1) {
	vmsg(err_bid);
	return -1;
    }
    assert(0<=bid-1 && bid-1<MAX_BOARD);
    prints("�ݪO�W�١G%s\n�ݪO�����G%s\n�ݪObid�G%d\n�ݪOGID�G%d\n"
	   "�O�D�W��G%s", bh.brdname, bh.title, bid, bh.gid, bh.BM);
    bperm_msg(&bh);

    /* Ptt �o���_��|�ɨ�U�� */
    move(9, 0);
    snprintf(genbuf, sizeof(genbuf), "(E)�]�w (V)�H�k/�Ѱ�%s%s [Q]�����H",
	    HasUserPerm(PERM_SYSOP |
		     PERM_BOARD) ? " (B)Vote (S)�Ϧ^ (C)�X�� (G)��L�ѥd" : "",
	    HasUserPerm(PERM_SYSSUBOP | PERM_SYSSUPERSUBOP | PERM_BOARD) ? " (D)�R��" : "");
    getdata(10, 0, genbuf, ans, 3, LCECHO);

    switch (*ans) {
    case 'g':
	if (HasUserPerm(PERM_SYSOP | PERM_BOARD)) {
	    char            path[PATHLEN];
	    setbfile(genbuf, bname, FN_TICKET_LOCK);
	    setbfile(path, bname, FN_TICKET_END);
	    rename(genbuf, path);
	}
	break;
    case 's':
	if (HasUserPerm(PERM_SYSOP | PERM_BOARD)) {
	  snprintf(genbuf, sizeof(genbuf),
		   BBSHOME "/bin/buildir boards/%c/%s &",
		   bh.brdname[0], bh.brdname);
	    system(genbuf);
	}
	break;
    case 'c':
	if (HasUserPerm(PERM_SYSOP)) {
	   char frombname[20], fromdir[256];
#ifdef MERGEBBS
	   if(getans("�O�_�פJSOB�ݪO? (y/N)")=='y')
	   { 
                 setbdir(genbuf, bname);
	         m_sob_brd(bname, fromdir);
		 if(!fromdir[0]) break;
                 merge_dir(genbuf, fromdir, 1);
           }
	   else{
#endif
	    CompleteBoard(MSG_SELECT_BOARD, frombname);
            if (frombname[0] == '\0' || !getbnum(frombname) ||
		!strcmp(frombname,bname))
	                     break;
            setbdir(genbuf, bname);
            setbdir(fromdir, frombname);
            merge_dir(genbuf, fromdir, 0);
#ifdef MERGEBBS
	   }
#endif
	    touchbtotal(bid);
	}
	break;
    case 'b':
	if (HasUserPerm(PERM_SYSOP | PERM_BOARD)) {
	    char            bvotebuf[10];

	    memcpy(&newbh, &bh, sizeof(bh));
	    snprintf(bvotebuf, sizeof(bvotebuf), "%d", newbh.bvote);
	    move(20, 0);
	    prints("�ݪO %s ��Ӫ� BVote�G%d", bh.brdname, bh.bvote);
	    getdata_str(21, 0, "�s�� Bvote�G", genbuf, 5, LCECHO, bvotebuf);
	    newbh.bvote = atoi(genbuf);
	    assert(0<=bid-1 && bid-1<MAX_BOARD);
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
	    log_usies("SetBoardBvote", newbh.brdname);
	    break;
	} else
	    break;
    case 'v':
	memcpy(&newbh, &bh, sizeof(bh));
	outs("�ݪO�ثe��");
	outs((bh.brdattr & BRD_BAD) ? "�H�k" : "���`");
	getdata(21, 0, "�T�w���H", genbuf, 5, LCECHO);
	if (genbuf[0] == 'y') {
	    if (newbh.brdattr & BRD_BAD)
		newbh.brdattr = newbh.brdattr & (!BRD_BAD);
	    else
		newbh.brdattr = newbh.brdattr | BRD_BAD;
	    assert(0<=bid-1 && bid-1<MAX_BOARD);
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
	    log_usies("ViolateLawSet", newbh.brdname);
	}
	break;
    case 'd':
	if (!(HasUserPerm(PERM_SYSOP | PERM_BOARD) ||
		    (HasUserPerm(PERM_SYSSUPERSUBOP) && GROUPOP())))
	    break;
	getdata_str(9, 0, msg_sure_ny, genbuf, 3, LCECHO, "N");
	if (genbuf[0] != 'y' || !bname[0])
	    outs(MSG_DEL_CANCEL);
	else if (bh.brdattr & BRD_SYMBOLIC) {
	    delete_symbolic_link(&bh, bid);
	}
	else {
	    strlcpy(bname, bh.brdname, sizeof(bh.brdname));
	    snprintf(genbuf, sizeof(genbuf),
		    "/bin/tar zcvf tmp/board_%s.tgz boards/%c/%s man/boards/%c/%s >/dev/null 2>&1;"
		    "/bin/rm -fr boards/%c/%s man/boards/%c/%s",
		    bname, bname[0], bname, bname[0],
		    bname, bname[0], bname, bname[0], bname);
	    system(genbuf);
	    memset(&bh, 0, sizeof(bh));
	    snprintf(bh.title, sizeof(bh.title),
		     "     %s �ݪO %s �R��", bname, cuser.userid);
	    post_msg(GLOBAL_SECURITY, bh.title, "�Ъ`�N�R�����X�k��", "[�t�Φw����]");
	    assert(0<=bid-1 && bid-1<MAX_BOARD);
	    substitute_record(fn_board, &bh, sizeof(bh), bid);
	    reset_board(bid);
            sort_bcache(); 
	    log_usies("DelBoard", bh.title);
	    outs("�R�O����");
	}
	break;
    case 'e':
	if( bh.brdattr & BRD_SYMBOLIC ){
	    vmsg("�T���ʳs���ݪO�A�Ъ����ץ���ݪO");
	    break;
	}
	move(8, 0);
	outs("������ [Return] ���ק�Ӷ��]�w");
	memcpy(&newbh, &bh, sizeof(bh));

	while (getdata(9, 0, "�s�ݪO�W�١G", genbuf, IDLEN + 1, DOECHO)) {
	    if (getbnum(genbuf)) {
		move(3, 0);
		outs("���~! �O�W�p�P");
	    } else if ( !invalid_brdname(genbuf) ){
		strlcpy(newbh.brdname, genbuf, sizeof(newbh.brdname));
		break;
	    }
	}

	do {
	    getdata_str(12, 0, "�ݪO���O�G", genbuf, 5, DOECHO, bh.title);
	    if (strlen(genbuf) == 4)
		break;
	} while (1);

	strcpy(newbh.title, genbuf);

	newbh.title[4] = ' ';

	getdata_str(14, 0, "�ݪO�D�D�G", genbuf, BTLEN + 1, DOECHO,
		    bh.title + 7);
	if (genbuf[0])
	    strlcpy(newbh.title + 7, genbuf, sizeof(newbh.title) - 7);
	if (getdata_str(15, 0, "�s�O�D�W��G", genbuf, IDLEN * 3 + 3, DOECHO,
			bh.BM)) {
	    trim(genbuf);
	    strlcpy(newbh.BM, genbuf, sizeof(newbh.BM));
	}
#ifdef CHESSCOUNTRY
	if (HasUserPerm(PERM_SYSOP)) {
	    snprintf(genbuf, sizeof(genbuf), "%d", bh.chesscountry);
	    if (getdata_str(16, 0, "�]�w�Ѱ� (0)�L (1)���l�� (2)�H�� (3)���", ans,
			sizeof(ans), LCECHO, genbuf)){
		newbh.chesscountry = atoi(ans);
		if (newbh.chesscountry > CHESSCODE_MAX ||
			newbh.chesscountry < CHESSCODE_NONE)
		    newbh.chesscountry = bh.chesscountry;
	    }
	}
#endif /* defined(CHESSCOUNTRY) */
	if (HasUserPerm(PERM_SYSOP|PERM_BOARD)) {
	    move(1, 0);
	    clrtobot();
	    newbh.brdattr = setperms(newbh.brdattr, str_permboard);
	    move(1, 0);
	    clrtobot();
	}
	{
	    const char* brd_symbol;
	    if (newbh.brdattr & BRD_GROUPBOARD)
        	brd_symbol = "�U";
	    else if (newbh.brdattr & BRD_NOTRAN)
		brd_symbol = "��";
	    else
		brd_symbol = "��";

	    newbh.title[5] = brd_symbol[0];
	    newbh.title[6] = brd_symbol[1];
	}

	if (HasUserPerm(PERM_SYSOP|PERM_BOARD) && !(newbh.brdattr & BRD_HIDE)) {
	    getdata_str(14, 0, "�]�wŪ�g�v��(Y/N)�H", ans, sizeof(ans), LCECHO, "N");
	    if (*ans == 'y') {
		getdata_str(15, 0, "���� [R]�\\Ū (P)�o���H", ans, sizeof(ans), LCECHO,
			    "R");
		if (*ans == 'p')
		    newbh.brdattr |= BRD_POSTMASK;
		else
		    newbh.brdattr &= ~BRD_POSTMASK;

		move(1, 0);
		clrtobot();
		bperm_msg(&newbh);
		newbh.level = setperms(newbh.level, str_permid);
		clear();
	    }
	}

	getdata(b_lines - 1, 0, "�бz�T�w(Y/N)�H[Y]", genbuf, 4, LCECHO);

	if ((*genbuf != 'n') && memcmp(&newbh, &bh, sizeof(bh))) {
	    char buf[64];

	    if (strcmp(bh.brdname, newbh.brdname)) {
		char            src[60], tar[60];

		setbpath(src, bh.brdname);
		setbpath(tar, newbh.brdname);
		Rename(src, tar);

		setapath(src, bh.brdname);
		setapath(tar, newbh.brdname);
		Rename(src, tar);
	    }
	    setup_man(&newbh, &bh);
	    assert(0<=bid-1 && bid-1<MAX_BOARD);
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
            sort_bcache(); 
	    log_usies("SetBoard", newbh.brdname);

	    snprintf(buf, sizeof(buf), "[�ݪO�ܧ�] %s (by %s)", bh.brdname, cuser.userid);
	    snprintf(genbuf, sizeof(genbuf),
		    "�O�W: %s => %s\n"
		    "�O�D: %s => %s\n",
		    bh.brdname, newbh.brdname, bh.BM, newbh.BM);
	    post_msg(GLOBAL_SECURITY, buf, genbuf, "[�t�Φw����]");
	}
    }
    return 0;
}

/* �]�w�ݪO */
int
m_board(void)
{
    char            bname[32];

    stand_title("�ݪO�]�w");
    CompleteBoardAndGroup(msg_bid, bname);
    if (!*bname)
	return 0;
    m_mod_board(bname);
    return 0;
}

/* �]�w�t���ɮ� */
int
x_file(void)
{
    int             aborted, num;
    char            ans[4], *fpath, buf[256];

    move(b_lines - 7, 0);
    /* Ptt */
    outs("�]�w (1)�����T�{�H (4)post�`�N�ƶ� (5)���~�n�J�T�� (6)���U�d�� (7)�q�L�T�{�q��\n");
    outs("     (8)email post�q�� (9)�t�Υ\\����F (A)���� (B)�����W�� (C)email�q�L�T�{\n");
    outs("     (D)�s�ϥΪ̻ݪ� (E)�����T�{��k (F)�w��e�� (G)�i���e�� "
#ifdef MULTI_WELCOME_LOGIN
	 "(X)�R���i���e��"
#endif
	 "\n");
    outs("     (H)�ݪO���� (I)�G�m (J)�X���e�� (K)�ͤ�d (L)�`�� (M)�~�y�ϥΪ̻{�ҳq��\n");
    outs("     (N)�~�y�ϥΪ̹L��ĵ�i�q�� (O)�ݪO�C�� help (P)�峹�C�� help\n");
#ifdef PLAY_ANGEL
    outs("     (R)�p�Ѩϻ{�ҳq�� (S)�p�Ѩϥ\\�໡��\n");
#endif
    getdata(b_lines - 1, 0, "[Q]����[1-9 A-P]�H", ans, sizeof(ans), LCECHO);

    switch (ans[0]) {
    case '1':
	fpath = "etc/confirm";
	break;
    case '4':
	fpath = "etc/post.note";
	break;
    case '5':
	fpath = "etc/goodbye";
	break;
    case '6':
	fpath = "etc/register";
	break;
    case '7':
	fpath = "etc/registered";
	break;
    case '8':
	fpath = "etc/emailpost";
	break;
    case '9':
	fpath = "etc/hint";
	break;
    case 'b':
	fpath = "etc/sysop";
	break;
    case 'c':
	fpath = "etc/bademail";
	break;
    case 'd':
	fpath = "etc/newuser";
	break;
    case 'e':
	fpath = "etc/justify";
	break;
    case 'f':
	fpath = "etc/Welcome";
	break;
    case 'g':
#ifdef MULTI_WELCOME_LOGIN
	getdata(b_lines - 1, 0, "�ĴX�Ӷi���e��[0-4]", ans, sizeof(ans), LCECHO);
	if (ans[0] == '1') {
	    fpath = "etc/Welcome_login.1";
	} else if (ans[0] == '2') {
	    fpath = "etc/Welcome_login.2";
	} else if (ans[0] == '3') {
	    fpath = "etc/Welcome_login.3";
	} else if (ans[0] == '4') {
	    fpath = "etc/Welcome_login.4";
	} else {
	    fpath = "etc/Welcome_login.0";
	}
#else
	fpath = "etc/Welcome_login";
#endif
	break;

#ifdef MULTI_WELCOME_LOGIN
    case 'x':
	getdata(b_lines - 1, 0, "�ĴX�Ӷi���e��[1-4]", ans, sizeof(ans), LCECHO);
	if (ans[0] == '1') {
	    unlink("etc/Welcome_login.1");
	    vmsg("ok");
	} else if (ans[0] == '2') {
	    unlink("etc/Welcome_login.2");
	    vmsg("ok");
	} else if (ans[0] == '3') {
	    unlink("etc/Welcome_login.3");
	    vmsg("ok");
	} else if (ans[0] == '4') {
	    unlink("etc/Welcome_login.4");
	    vmsg("ok");
	} else {
	    vmsg("�ҫ��w���i���e���L�k�R��");
	}
	return FULLUPDATE;

#endif

    case 'h':
	fpath = "etc/expire.conf";
	break;
    case 'i':
	fpath = "etc/domain_name_query.cidr";
	break;
    case 'j':
	fpath = "etc/Logout";
	break;
    case 'k':
	mouts(b_lines - 3, 0, "1.���~  2.���~  3.����  4.�d��  5.����  6.���l");
	mouts(b_lines - 2, 0, "7.����  8.��l  9.�B�k 10.�ѯ� 11.���� 12.�g��");
	getdata(b_lines - 1, 0, "�п�� [1-12]", ans, sizeof(ans), LCECHO);
	num = atoi(ans);
	if (num <= 0 || num > 12)
	    return FULLUPDATE;
	snprintf(buf, sizeof(buf), "etc/Welcome_birth.%d", num);
	fpath = buf;
	break;
    case 'l':
	fpath = "etc/feast";
	break;
    case 'm':
	fpath = "etc/foreign_welcome";
	break;
    case 'n':
	fpath = "etc/foreign_expired_warn";
	break;
    case 'o':
	fpath = "etc/boardlist.help";
	break;
    case 'p':
	fpath = "etc/board.help";
	break;

#ifdef PLAY_ANGEL
    case 'r':
	fpath = "etc/angel_notify";
	break;

    case 's':
	fpath = "etc/angel_usage";
	break;
#endif

    default:
	return FULLUPDATE;
    }
    aborted = vedit(fpath, NA, NULL);
    vmsgf("\n\n�t���ɮ�[%s]: %s", fpath,
	 (aborted == -1) ? "������" : "��s����");
    return FULLUPDATE;
}

static int add_board_record(const boardheader_t *board)
{
    int bid;
    if ((bid = getbnum("")) > 0) {
	assert(0<=bid-1 && bid-1<MAX_BOARD);
	substitute_record(fn_board, board, sizeof(boardheader_t), bid);
	reset_board(bid);
        sort_bcache(); 
    } else if (append_record(fn_board, (fileheader_t *)board, sizeof(boardheader_t)) == -1) {
	return -1;
    } else {
	addbrd_touchcache();
    }
    return 0;
}

/**
 * open a new board
 * @param whatclass In which sub class
 * @param recover   Forcely open a new board, often used for recovery.
 * @return -1 if failed
 */
int
m_newbrd(int whatclass, int recover)
{
    boardheader_t   newboard;
    char            ans[4];
    char            genbuf[200];

    stand_title("�إ߷s�O");
    memset(&newboard, 0, sizeof(newboard));

    newboard.gid = whatclass;
    if (newboard.gid == 0) {
	vmsg("�Х���ܤ@�����O�A�}�O!");
	return -1;
    }
    do {
	if (!getdata(3, 0, msg_bid, newboard.brdname,
		     sizeof(newboard.brdname), DOECHO))
	    return -1;
    } while (invalid_brdname(newboard.brdname));

    do {
	getdata(6, 0, "�ݪO���O�G", genbuf, 5, DOECHO);
	if (strlen(genbuf) == 4)
	    break;
    } while (1);

    strcpy(newboard.title, genbuf);

    newboard.title[4] = ' ';

    getdata(8, 0, "�ݪO�D�D�G", genbuf, BTLEN + 1, DOECHO);
    if (genbuf[0])
	strlcpy(newboard.title + 7, genbuf, sizeof(newboard.title) - 7);
    setbpath(genbuf, newboard.brdname);

    if (!recover && 
        (getbnum(newboard.brdname) > 0 || mkdir(genbuf, 0755) == -1)) {
	vmsg("���ݪO�w�g�s�b! �Ш����P�^��O�W");
	return -1;
    }
    newboard.brdattr = BRD_NOTRAN;
#ifdef DEFAULT_AUTOCPLOG
    newboard.brdattr |= BRD_CPLOG;
#endif

    if (HasUserPerm(PERM_SYSOP)) {
	move(1, 0);
	clrtobot();
	newboard.brdattr = setperms(newboard.brdattr, str_permboard);
	move(1, 0);
	clrtobot();
    }
    getdata(9, 0, "�O�ݪO? (N:�ؿ�) (Y/n)�G", genbuf, 3, LCECHO);
    if (genbuf[0] == 'n')
    {
	newboard.brdattr |= BRD_GROUPBOARD;
	newboard.brdattr &= ~BRD_CPLOG;
    }

	{
	    const char* brd_symbol;
	    if (newboard.brdattr & BRD_GROUPBOARD)
        	brd_symbol = "�U";
	    else if (newboard.brdattr & BRD_NOTRAN)
		brd_symbol = "��";
	    else
		brd_symbol = "��";

	    newboard.title[5] = brd_symbol[0];
	    newboard.title[6] = brd_symbol[1];
	}

    newboard.level = 0;
    getdata(11, 0, "�O�D�W��G", newboard.BM, sizeof(newboard.BM), DOECHO);
#ifdef CHESSCOUNTRY
    if (getdata_str(12, 0, "�]�w�Ѱ� (0)�L (1)���l�� (2)�H�� (3)���", ans,
		sizeof(ans), LCECHO, "0")){
	newboard.chesscountry = atoi(ans);
	if (newboard.chesscountry > CHESSCODE_MAX ||
		newboard.chesscountry < CHESSCODE_NONE)
	    newboard.chesscountry = CHESSCODE_NONE;
    }
#endif /* defined(CHESSCOUNTRY) */

    if (HasUserPerm(PERM_SYSOP) && !(newboard.brdattr & BRD_HIDE)) {
	getdata_str(14, 0, "�]�wŪ�g�v��(Y/N)�H", ans, sizeof(ans), LCECHO, "N");
	if (*ans == 'y') {
	    getdata_str(15, 0, "���� [R]�\\Ū (P)�o���H", ans, sizeof(ans), LCECHO, "R");
	    if (*ans == 'p')
		newboard.brdattr |= BRD_POSTMASK;
	    else
		newboard.brdattr &= (~BRD_POSTMASK);

	    move(1, 0);
	    clrtobot();
	    bperm_msg(&newboard);
	    newboard.level = setperms(newboard.level, str_permid);
	    clear();
	}
    }

    add_board_record(&newboard);
    getbcache(whatclass)->childcount = 0;
    pressanykey();
    setup_man(&newboard, NULL);
    outs("\n�s�O����");
    post_newboard(newboard.title, newboard.brdname, newboard.BM);
    log_usies("NewBoard", newboard.title);
    pressanykey();
    return 0;
}

int make_symbolic_link(const char *bname, int gid)
{
    boardheader_t   newboard;
    int bid;
    
    bid = getbnum(bname);
    if(bid==0) return -1;
    assert(0<=bid-1 && bid-1<MAX_BOARD);
    memset(&newboard, 0, sizeof(newboard));

    /*
     * known issue:
     *   These two stuff will be used for sorting.  But duplicated brdnames
     *   may cause wrong binary-search result.  So I replace the last 
     *   letters of brdname to '~'(ascii code 126) in order to correct the
     *   resuilt, thought I think it's a dirty solution.
     *
     *   Duplicate entry with same brdname may cause wrong result, if
     *   searching by key brdname.  But people don't need to know a board
     *   is symbolic, so just let SYSOP know it. You may want to read
     *   board.c:load_boards().
     */

    strlcpy(newboard.brdname, bname, sizeof(newboard.brdname));
    newboard.brdname[strlen(bname) - 1] = '~';
    strlcpy(newboard.title, bcache[bid - 1].title, sizeof(newboard.title));
    strcpy(newboard.title + 5, "�I�ݪO�s��");

    newboard.gid = gid;
    BRD_LINK_TARGET(&newboard) = bid;
    newboard.brdattr = BRD_NOTRAN | BRD_SYMBOLIC;

    if (add_board_record(&newboard) < 0)
	return -1;
    return bid;
}

int make_symbolic_link_interactively(int gid)
{
    char buf[32];

    CompleteBoard(msg_bid, buf);
    if (!buf[0])
	return -1;

    stand_title("�إ߬ݪO�s��");

    if (make_symbolic_link(buf, gid) < 0) {
	vmsg("�ݪO�s���إߥ���");
	return -1;
    }
    log_usies("NewSymbolic", buf);
    return 0;
}

/* FIXME �u�O�@�ΩU��
 *
 * fdata �ΤF�Ӧh magic number
 * return value ���ӬO�� reason (return index + 1)
 * ans[0] �����O�b�޿�ܪ��u���~�����v (Register ���̬ݨ쪺����)
 */
static int
auto_scan(char fdata[][STRLEN], char ans[])
{
    int             good = 0;
    int             count = 0;
    int             i;
    char            temp[10];

    if (!strncmp(fdata[1], "�p", 2) || strstr(fdata[1], "�X")
	|| strstr(fdata[1], "��") || strstr(fdata[1], "��")) {
	ans[0] = '0';
	return 1;
    }
    strlcpy(temp, fdata[1], 3);

    /* �|�r */
    if (!strncmp(temp, &(fdata[1][2]), 2)) {
	ans[0] = '0';
	return 1;
    }
    if (strlen(fdata[1]) >= 6) {
	if (strstr(fdata[1], "������")) {
	    ans[0] = '0';
	    return 1;
	}
	if (strstr("�����]���P�d�G��", temp))
	    good++;
	else if (strstr("���C���L���x�E���B", temp))
	    good++;
	else if (strstr("Ĭ��d�f����i����Ĭ", temp))
	    good++;
	else if (strstr("�}�¥ۿc�I���έ�", temp))
	    good++;
    }
    if (!good)
	return 0;

    if (!strcmp(fdata[2], fdata[3]) ||
	!strcmp(fdata[2], fdata[4]) ||
	!strcmp(fdata[3], fdata[4])) {
	ans[0] = '4';
	return 5;
    }
    if (strstr(fdata[2], "�j")) {
	if (strstr(fdata[2], "�x") || strstr(fdata[2], "�H") ||
	    strstr(fdata[2], "��") || strstr(fdata[2], "�F") ||
	    strstr(fdata[2], "�M") || strstr(fdata[2], "ĵ") ||
	    strstr(fdata[2], "�v") || strstr(fdata[2], "�ʶ�") ||
	    strstr(fdata[2], "����") || strstr(fdata[2], "��") ||
	    strstr(fdata[2], "��") || strstr(fdata[2], "�F�d"))
	    good++;
    } else if (strstr(fdata[2], "�k��"))
	good++;

    if (strstr(fdata[3], "�a�y") || strstr(fdata[3], "�t�z") ||
	strstr(fdata[3], "�H�c")) {
	ans[0] = '2';
	return 3;
    }
    if (strstr(fdata[3], "��") || strstr(fdata[3], "��")) {
	if (strstr(fdata[3], "��") || strstr(fdata[3], "��")) {
	    if (strstr(fdata[3], "��"))
		good++;
	}
    }
    for (i = 0; fdata[4][i]; i++) {
	if (isdigit((int)fdata[4][i]))
	    count++;
    }

    if (count <= 4) {
	ans[0] = '3';
	return 4;
    } else if (count >= 7)
	good++;

    if (good >= 3) {
	ans[0] = 'y';
	return -1;
    } else
	return 0;
}

#define REJECT_REASONS (6)
#define FN_REGISTER_LOG "register.log"

// read count entries from regsrc to a temp buffer
FILE *
pull_regform(const char *regfile, char *workfn, int count)
{
    FILE *fp = NULL;

    snprintf(workfn, PATHLEN, "%s.tmp", regfile);
    if (dashf(workfn)) {
	vmsg("��L SYSOP �]�b�f�ֵ��U�ӽг�");
	return NULL;
    }

    // count < 0 means unlimited pulling
    Rename(regfile, workfn);
    if ((fp = fopen(workfn, "r")) == NULL) {
	vmsgf("�t�ο��~�A�L�kŪ�����U�����: %s", workfn);
	return NULL;
    }
    return fp;
}

// write all left in "remains" to regfn.
void
pump_regform(const char *regfn, FILE *remains)
{
    // restore trailing tickets
    char buf[PATHLEN];
    FILE *fout = fopen(regfn, "at");
    if (!fout)
	return;

    while (fgets(buf, sizeof(buf), remains))
	fputs(buf, fout);
    fclose(fout);
}

/* �B�z Register Form */
// TODO XXX process someone directly, according to target_uid.
int
scan_register_form(const char *regfile, int automode, const char *target_uid)
{
    char            genbuf[200];
    char    *logfile = FN_REGISTER_LOG;
    char    *field[] = {
	"uid", "name", "career", "addr", "phone", "email", NULL
    };
    char    *finfo[] = {
	"�b��", "�u��m�W", "�A�ȳ��", "�ثe���}",
	"�s���q��", "�q�l�l��H�c", NULL
    };
    char    *reason[REJECT_REASONS+1] = {
	"��J�u��m�W",
	"�Զ�u(���~)�ǮդΡy�t�z�y�šz�v�Ρu�A�ȳ��(�t���ݿ�����¾��)�v",
	"��g���㪺���}��� (�t�����W��, �x�_���Чt��F�ϰ�^",
	"�Զ�s���q�� (�t�ϰ�X, �������Υ[ '-', '(', ')'���Ÿ�",
	"��T�ç����g���U�ӽЪ�",
	"�Τ����g�ӽг�",
	NULL
    };
    char    *autoid = "AutoScan";
    userec_t        muser;
    FILE           *fn, *fout, *freg;
    char            fdata[6][STRLEN];
    char            fname[STRLEN] = "", buf[STRLEN];
    char            ans[4], *ptr, *uid;
    int             n = 0, unum = 0, tid = 0;
    int             nSelf = 0, nAuto = 0;

    uid = cuser.userid;
    move(2, 0);

    fn = pull_regform(regfile, fname, -1);
    if (!fn)
	return -1;

    while( fgets(genbuf, STRLEN, fn) ){
	memset(fdata, 0, sizeof(fdata));
	do {
	    if( genbuf[0] == '-' )
		break;
	    if ((ptr = (char *)strstr(genbuf, ": "))) {
		*ptr = '\0';
		for (n = 0; field[n]; n++) {
		    if (strcmp(genbuf, field[n]) == 0) {
			strlcpy(fdata[n], ptr + 2, sizeof(fdata[n]));
			if ((ptr = (char *)strchr(fdata[n], '\n')))
			    *ptr = '\0';
		    }
		}
	    }
	} while( fgets(genbuf, STRLEN, fn) );
	tid ++;

	if ((unum = getuser(fdata[0], &muser)) == 0) {
	    move(2, 0);
	    clrtobot();
	    outs("�t�ο��~�A�d�L���H\n\n");
	    for (n = 0; field[n]; n++)
		prints("%s     : %s\n", finfo[n], fdata[n]);
	    pressanykey();
	} else {
	    if (automode)
		uid = autoid;

	    if ((!automode || !auto_scan(fdata, ans))) {
		uid = cuser.userid;

		move(1, 0);
		clrtobot();
		prints("�b����m    : %d\n", unum);
		user_display(&muser, 1);
		move(14, 0);
		prints(ANSI_COLOR(1;32) "------------- "
			"�Я����Y��f�֨ϥΪ̸�ơA�o�O�� %d ��"
			"------------" ANSI_RESET "\n", tid);
	    	prints("  %-12s: %s\n", finfo[0], fdata[0]);
#ifdef FOREIGN_REG
		prints("0.%-12s: %s%s\n", finfo[1], fdata[1],
		       muser.uflag2 & FOREIGN ? " (�~�y)" : "");
#else
		prints("0.%-12s: %s\n", finfo[1], fdata[1]);
#endif
		for (n = 2; field[n]; n++) {
		    prints("%d.%-12s: %s\n", n - 1, finfo[n], fdata[n]);
		}
		if (muser.userlevel & PERM_LOGINOK) {
		    ans[0] = getkey("���b���w�g�������U, "
				    "��s(Y/N/Skip)�H[N] ");
		    if (ans[0] != 'y' && ans[0] != 's')
			ans[0] = 'd';
		} else {
		    if (search_ulist(unum) == NULL)
		    {
			move(b_lines, 0); clrtoeol();
			outs("�O�_���������(Y/N/Q/Del/Skip)�H[S] ");
			// FIXME if the user got online here
		        ans[0] = igetch();
		    }
		    else
			ans[0] = 's';
		    ans[0] = tolower(ans[0]);
		    if (ans[0] != 'y' && ans[0] != 'n' && 
			ans[0] != 'q' && ans[0] != 'd' && 
			!('0' <= ans[0] && ans[0] < ('0' + REJECT_REASONS)))
			ans[0] = 's';
		    ans[1] = 0;
		}
		nSelf++;
	    } else
		nAuto++;

	    switch (ans[0]) {
	    case 'q':
		if ((freg = fopen(regfile, "a"))) {
		    for (n = 0; field[n]; n++)
			fprintf(freg, "%s: %s\n", field[n], fdata[n]);
		    fprintf(freg, "----\n");
		    while (fgets(genbuf, STRLEN, fn))
			fputs(genbuf, freg);
		    fclose(freg);
		}
	    case 'd':
		break;

	    case '0': case '1': case '2':
	    case '3': case '4': case '5':
		/* please confirm match REJECT_REASONS here */
	    case 'n':
		if (ans[0] == 'n') {
		    int nf = 0;
		    move(8, 0);
		    clrtobot();
		    outs("�д��X�h�^�ӽЪ���]�A�� <enter> ����\n");
		    for (n = 0; n < REJECT_REASONS; n++)
			prints("%d) ��%s\n", n, reason[n]);
		    outs("\n"); // preserved for prompt
		    for (nf = 0; field[nf]; nf++)
			prints("%s: %s\n", finfo[nf], fdata[nf]);
		} else
		    buf[0] = ans[0];

		if (ans[0] != 'n' ||
		    getdata(9 + n, 0, "�h�^��]: ", buf, 60, DOECHO))
		    if ((buf[0] - '0') >= 0 && (buf[0] - '0') < n) {
			int             i;
			fileheader_t    mhdr;
			char            title[128], buf1[80];
			FILE           *fp;

			sethomepath(buf1, muser.userid);
			stampfile(buf1, &mhdr);
			strlcpy(mhdr.owner, cuser.userid, sizeof(mhdr.owner));
			strlcpy(mhdr.title, "[���U����]", TTLEN);
			mhdr.filemode = 0;
			sethomedir(title, muser.userid);
			if (append_record(title, &mhdr, sizeof(mhdr)) != -1) {
			    char rejfn[PATHLEN];
			    fp = fopen(buf1, "w");
			    
			    for(i = 0; buf[i] && i < sizeof(buf); i++){
				if (buf[i] >= '0' && buf[i] < '0'+n)
				{
				    fprintf(fp, "[�h�^��]] ��%s\n",
					    reason[buf[i] - '0']);
				}
			    }

			    fclose(fp);

			    // build reject file
			    setuserfile(rejfn, "justify.reject");
			    Copy(buf1, rejfn);
			}
			if ((fout = fopen(logfile, "a"))) {
			    for (n = 0; field[n]; n++)
				fprintf(fout, "%s: %s\n", field[n], fdata[n]);
			    fprintf(fout, "Date: %s\n", Cdate(&now));
			    fprintf(fout, "Rejected: %s [%s]\n----\n",
				    uid, buf);
			    fclose(fout);
			}
			break;
		    }
		move(10, 0);
		clrtobot();
		outs("�����h�^�����U�ӽЪ�");
		/* no break? */

	    case 's':
		if ((freg = fopen(regfile, "a"))) {
		    for (n = 0; field[n]; n++)
			fprintf(freg, "%s: %s\n", field[n], fdata[n]);
		    fprintf(freg, "----\n");
		    fclose(freg);
		}
		break;

	    default:
		outs("�H�U�ϥΪ̸�Ƥw�g��s:\n");
		mail_muser(muser, "[���U���\\�o]", "etc/registered");

#if FOREIGN_REG_DAY > 0
		if(muser.uflag2 & FOREIGN)
		    mail_muser(muser, "[�X�J�Һ޲z��]", "etc/foreign_welcome");
#endif

		muser.userlevel |= (PERM_LOGINOK | PERM_POST);
		strlcpy(muser.realname, fdata[1], sizeof(muser.realname));
		strlcpy(muser.address, fdata[3], sizeof(muser.address));
		strlcpy(muser.email, fdata[5], sizeof(muser.email));
		snprintf(genbuf, sizeof(genbuf), "%s:%s:%s",
			 fdata[4], fdata[2], uid);
		strlcpy(muser.justify, genbuf, sizeof(muser.justify));

		passwd_update(unum, &muser);
		// XXX TODO notify users?
		sendalert(muser.userid,  ALERT_PWD_PERM); // force to reload perm

		sethomefile(buf, muser.userid, "justify");
		log_file(buf, LOG_CREAT, genbuf);

		if ((fout = fopen(logfile, "a"))) {
		    for (n = 0; field[n]; n++)
			fprintf(fout, "%s: %s\n", field[n], fdata[n]);
		    fprintf(fout, "Date: %s\n", Cdate(&now));
		    fprintf(fout, "Approved: %s\n", uid);
		    fprintf(fout, "----\n");
		    fclose(fout);
		}
		sethomefile(genbuf, muser.userid, "justify.wait");
		unlink(genbuf);
		break;
	    }
	}
    }

    fclose(fn);
    unlink(fname);

    move(0, 0);
    clrtobot();

    move(5, 0);
    prints("�z�f�F %d �����U��AAutoScan �f�F %d ��", nSelf, nAuto);

    pressanykey();
    return (0);
}

#ifdef EXP_ADMIN_REGFORM

#define FORMS_IN_PAGE (10)
#define REASON_LEN (60)
static const char *reasonstr[REJECT_REASONS] = {
    "��J�u��m�W",
    "�Զ�(���~)�Ǯաy�t�z�y�šz�ΪA�ȳ��(�t���ݿ�����¾��)",
    "��g���㪺���}��� (�t�����W��, �x�_���Чt��F�ϰ�)",
    "�Զ�s���q�� (�t�ϽX, �������[ '-', '(', ')' ���Ÿ�)",
    "��T�ç����g���U�ӽЪ�",
    "�Τ����g�ӽг�",
};

#define REASON_FIRSTABBREV '0'
#define REASON_IN_ABBREV(x) \
    ((x) >= REASON_FIRSTABBREV && (x) - REASON_FIRSTABBREV < REJECT_REASONS)
#define REASON_EXPANDABBREV(x)	 reasonstr[(x) - REASON_FIRSTABBREV]

static void
prompt_regform_ui()
{
    move(b_lines, 0);
    outs(ANSI_COLOR(30;47)  "  "
	    ANSI_COLOR(31) "y" ANSI_COLOR(30) "���� "
	    ANSI_COLOR(31) "n" ANSI_COLOR(30) "�ڵ� "
	    ANSI_COLOR(31) "d" ANSI_COLOR(30) "�R�� "
	    ANSI_COLOR(31) "s" ANSI_COLOR(30) "���L "
	    ANSI_COLOR(31) "u" ANSI_COLOR(30) "�_�� "
	    " "
	    ANSI_COLOR(31) "0-9jk����" ANSI_COLOR(30) "���� "
	    ANSI_COLOR(31) "�ť�/PgDn" ANSI_COLOR(30) "�x�s+�U�� "
	    " "
	    ANSI_COLOR(31) "q/END" ANSI_COLOR(30) "����  "
	    ANSI_RESET);
}

void
resolve_reason(char *s, int y)
{
    // should start with REASON_FIRSTABBREV
    const char *reason_prompt = 
	" (0)�u��m�W (1)�Զ�t�� (2)������}"
	" (3)�Զ�q�� (4)�T���g (5)�����g";

    s[0] = 0;
    move(y, 0);
    outs(reason_prompt); outs("\n");

    do {
	getdata(y+1, 0, 
		"�h�^��]: ", s, REASON_LEN, DOECHO);

	// convert abbrev reasons (format: single digit, or multiple digites)
	if (REASON_IN_ABBREV(s[0]))
	{
	    if (s[1] == 0) // simple replace ment
	    {
		strlcpy(s+2, REASON_EXPANDABBREV(s[0]),
			REASON_LEN-2);
		s[0] = 0xbd; // '��'[0];
		s[1] = 0xd0; // '��'[1];
	    } else {
		// strip until all digites
		char *p = s;
		while (*p)
		{
		    if (!REASON_IN_ABBREV(*p))
			*p = ' ';
		    p++;
		}
		strip_blank(s, s);
		strlcat(s, " [�h����]]", REASON_LEN);
	    }
	} 

	if (strlen(s) < 4)
	{
	    if (vmsg("��]�ӵu�C �n�����h�^�ܡH (y/N): ") == 'y')
	    {
		*s = 0;
		return;
	    }
	}
    } while (strlen(s) < 4);
}

void 
regform_accept(const char *userid, const char *justify)
{
    char buf[PATHLEN];
    int unum = 0;
    userec_t muser;

    unum = getuser(userid, &muser);
    if (unum == 0)
	return; // invalid user

    muser.userlevel |= (PERM_LOGINOK | PERM_POST);
    strlcpy(muser.justify, justify, sizeof(muser.justify));
    // manual accept sets email to 'x'
    strlcpy(muser.email, "x", sizeof(muser.email)); 

    // handle files
    sethomefile(buf, muser.userid, "justify.wait");
    unlink(buf);
    sethomefile(buf, muser.userid, "justify.reject");
    unlink(buf);
    sethomefile(buf, muser.userid, "justify");
    log_filef(buf, LOG_CREAT, "%s\n", muser.justify);

    // update password file
    passwd_update(unum, &muser);

    // alert online users?
    sendalert(muser.userid,  ALERT_PWD_PERM|ALERT_PWD_JUSTIFY); // force to reload perm

#if FOREIGN_REG_DAY > 0
    if(muser.uflag2 & FOREIGN)
	mail_muser(muser, "[�X�J�Һ޲z��]", "etc/foreign_welcome");
    else
#endif
    // last: send notification mail
    mail_muser(muser, "[���U���\\�o]", "etc/registered");
}

void 
regform_reject(const char *userid, char *reason)
{
    char buf[PATHLEN];
    FILE *fp = NULL;
    int unum = 0;
    userec_t muser;

    unum = getuser(userid, &muser);
    if (unum == 0)
	return; // invalid user

    muser.userlevel &= ~(PERM_LOGINOK | PERM_POST);

    // handle files
    sethomefile(buf, muser.userid, "justify.wait");
    unlink(buf);

    // update password file
    passwd_update(unum, &muser);

    // alert notify users?
    sendalert(muser.userid,  ALERT_PWD_PERM); // force to reload perm

    // last: send notification
    mkuserdir(muser.userid);
    sethomefile(buf, muser.userid, "justify.reject");
    fp = fopen(buf, "wt");
    assert(fp);
    syncnow();
    fprintf(fp, "%s ���U���ѡC\n", Cdate(&now));

    // multiple abbrev loop
    if (REASON_IN_ABBREV(reason[0]))
    {
	int i = 0;
	for (i = 0; i < REASON_LEN && REASON_IN_ABBREV(reason[i]); i++)
	    fprintf(fp, "[�h�^��]] ��%s\n", REASON_EXPANDABBREV(reason[i]));
    } else {
	fprintf(fp, "[�h�^��]] %s\n", reason);
    }
    fclose(fp);
    mail_muser(muser, "[���U����]", buf);
}

// TODO define and use structure instead, even in reg request file.
//
typedef struct {
    // current format:
    // (optional) num: unum, date
    // [0] uid: xxxxx	(IDLEN=12)
    // [1] name: RRRRRR (20)
    // [2] career: YYYYYYYYYYYYYYYYYYYYYYYYYY (40)
    // [3] addr: TTTTTTTTT (50)
    // [4] phone: 02DDDDDDDD (20)
    // [5] email: x (50) (deprecated)
    // [6] mobile: (deprecated)
    // [7] ----
    //     lasthost: 16
    char userid[IDLEN+1];

    char exist;
    char online;
    char pad   [ 5];     // IDLEN(12)+1+1+1+5=20

    char name  [20];
    char career[40];
    char addr  [50];
    char phone [20];
} RegformEntry;

int
load_regform_entry(RegformEntry *pre, FILE *fp)
{
    char buf[STRLEN];
    char *v;

    memset(pre, 0, sizeof(RegformEntry));
    while (fgets(buf, sizeof(buf), fp))
    {
	if (buf[0] == '-')
	    break;
	buf[sizeof(buf)-1] = 0;
	v = strchr(buf, ':');
	if (v == NULL)
	    continue;
	*v++ = 0;
	if (*v == ' ') v++;
	chomp(v);

	if (strcmp(buf, "uid") == 0)
	    strlcpy(pre->userid, v, sizeof(pre->userid));
	else if (strcmp(buf, "name") == 0)
	    strlcpy(pre->name, v, sizeof(pre->name));
	else if (strcmp(buf, "career") == 0)
	    strlcpy(pre->career, v, sizeof(pre->career));
	else if (strcmp(buf, "addr") == 0)
	    strlcpy(pre->addr, v, sizeof(pre->addr));
	else if (strcmp(buf, "phone") == 0)
	    strlcpy(pre->phone, v, sizeof(pre->phone));
    }
    return pre->userid[0] ? 1 : 0;
}

int
print_regform_entry(const RegformEntry *pre, FILE *fp, int close)
{
    fprintf(fp, "uid: %s\n",	pre->userid);
    fprintf(fp, "name: %s\n",	pre->name);
    fprintf(fp, "career: %s\n", pre->career);
    fprintf(fp, "addr: %s\n",	pre->addr);
    fprintf(fp, "phone: %s\n",	pre->phone);
    fprintf(fp, "email: %s\n",	"x");
    if (close)
	fprintf(fp, "----\n");
    return 1;
}

int
append_regform(const RegformEntry *pre, const char *logfn, 
	const char *varname, const char *varval1, const char *varval2)
{
    FILE *fout = fopen(logfn, "at");
    if (!fout)
	return 0;

    print_regform_entry(pre, fout, 0);
    if (varname && *varname)
    {
	syncnow();
	fprintf(fout, "Date: %s\n", Cdate(&now));
	if (!varval1) varval1 = "";
	fprintf(fout, "%s: %s", varname, varval1);
	if (varval2) fprintf(fout, " %s", varval2);
	fprintf(fout, "\n");
    }
    // close it
    fprintf(fout, "----\n");
    fclose(fout);
    return 1;
}

// #define REGFORM_DISABLE_ONLINE_USER

int
handle_register_form(const char *regfile, int dryrun)
{
    int unum = 0;
    int yMsg = FORMS_IN_PAGE*2+1;
    FILE *fp = NULL;
    userec_t muser;
    RegformEntry forms [FORMS_IN_PAGE];
    char ans	[FORMS_IN_PAGE];
    char rejects[FORMS_IN_PAGE][REASON_LEN];	// reject reason length
    char fname  [PATHLEN] = "";
    char justify[REGLEN+1];
    char rsn	[REASON_LEN];
    int cforms = 0,	// current loaded forms
	parsed = 0,	// total parsed forms
	ci = 0, // cursor index
	ch = 0,	// input key
	i, blanks;
    long fsz = 0, fpos = 0;

    // prepare reg tickets
    if (dryrun)
    {
	// directly open regfile to try
	fp = fopen(regfile, "rt");
    } else {
	fp = pull_regform(regfile, fname, -1);
    }

    if (!fp)
	return 0;

    // retreieve file info
    fpos = ftell(fp);
    fseek(fp, 0, SEEK_END);
    fsz = ftell(fp);
    fseek(fp, fpos, SEEK_SET);
    if (!fsz) fsz = 1;

    while (ch != 'q')
    {
	// initialize and prepare
	memset(ans, 0, sizeof(ans));
	memset(rejects, 0, sizeof(rejects));
	memset(forms, 0, sizeof(forms));
	cforms = 0;

	// load forms
	while (cforms < FORMS_IN_PAGE && load_regform_entry(&forms[cforms], fp))
	    cforms++, parsed ++;

	// if no more forms then leave.
	// TODO what if regform error?
	if (cforms < 1)
	    break;

	// adjust cursor if required
	if (ci >= cforms)
	    ci = cforms-1;

	// display them all.
	clear();
	for (i = 0; i < cforms; i++)
	{
	    // fetch user information
	    memset(&muser, 0, sizeof(muser));
	    unum = getuser(forms[i].userid, &muser);
	    forms[i].exist = unum ? 1 : 0;
	    if (unum) forms[i].online = search_ulist(unum) ? 1 : 0;

	    // if already got login level, delete by default.
	    if (!unum)
		ans[i] = 'd';
	    else {
		if (muser.userlevel & PERM_LOGINOK)
		    ans[i] = 'd';
#ifdef REGFORM_DISABLE_ONLINE_USER
		else if (forms[i].online)
		    ans[i] = 's';
#endif // REGFORM_DISABLE_ONLINE_USER
	    }

	    // print
	    move(i*2, 0);
	    prints("  %2d%s %s%-12s " ANSI_RESET, 
		    i+1, 
		    (unum == 0) ? ANSI_COLOR(1;31) "D" :
		    ( (muser.userlevel & PERM_LOGINOK) ? 
		      ANSI_COLOR(1;33) "Y" : 
#ifdef REGFORM_DISABLE_ONLINE_USER
			  forms[i].online ? "s" : 
#endif
			  "."),
		    forms[i].online ?  ANSI_COLOR(1;35) : ANSI_COLOR(1),
		    forms[i].userid);

	    prints( ANSI_COLOR(1;31) "%19s " 
		    ANSI_COLOR(1;32) "%-40s" ANSI_RESET"\n", 
		    forms[i].name, forms[i].career);

	    move(i*2+1, 0); 
	    prints("    %s %-50s%20s\n", 
		    (muser.userlevel & PERM_NOREGCODE) ? 
		    ANSI_COLOR(1;31) "T" ANSI_RESET : " ",
		    forms[i].addr, forms[i].phone);
	}

	// display page info
	{
	    char msg[STRLEN];
	    fpos = ftell(fp);
	    if (fpos > fsz) fsz = fpos*10;
	    snprintf(msg, sizeof(msg),
		    " �w��� %d �����U�� (%2d%%)  ",
		    parsed, (int)(fpos*100/fsz));
	    prints(ANSI_COLOR(7) "\n%78s" ANSI_RESET "\n", msg);
	}

	// handle user input
	prompt_regform_ui();
	ch = 0;
	while (ch != 'q' && ch != ' ') {
	    ch = cursor_key(ci*2, 0);
	    switch (ch)
	    {
		// nav keys
		case KEY_UP:
		case 'k':
		    if (ci > 0) ci--;
		    break;

		case KEY_DOWN:
		case 'j':
		    ch = 'j'; // go next
		    break;

		    // quick nav (assuming to FORMS_IN_PAGE=10)
		case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9':
		    ci = ch - '1';
		    if (ci >= cforms) ci = cforms-1;
		    break;
		case '0':
		    ci = 10-1;
		    if (ci >= cforms) ci = cforms-1;
		    break;

		    /*
		case KEY_HOME: ci = 0; break;
		case KEY_END:  ci = cforms-1; break;
		    */

		    // abort
		case KEY_END:
		case 'q':
		    ch = 'q';
		    if (getans("�T�w�n���}�F�ܡH (�����ܧ�N���|�x�s) [y/N]: ") != 'y')
		    {
			prompt_regform_ui();
			ch = 0;
			continue;
		    }
		    break;

		    // prepare to go next page
		case KEY_PGDN:
		case ' ':
		    ch = ' ';

		    // solving blank (undecided entries)
		    for (i = 0, blanks = 0; i < cforms; i++)
			if (ans[i] == 0) blanks ++;

		    if (!blanks)
			break;

		    // have more blanks
		    ch = getans("�|�����w�� %d �Ӷ��حn: (S���L/y�q�L/n�ڵ�/e�~��s��): ", 
			    blanks);

		    if (ch == 'e')
		    {
			prompt_regform_ui();
			ch = 0;
			continue;
		    }
		    if (ch == 'y') {
			// do nothing.
		    } else if (ch == 'n') {
			// query reject reason
			resolve_reason(rsn, yMsg);
			if (*rsn == 0)
			    ch = 's';
		    } else ch = 's';

		    // filling answers
		    for (i = 0; i < cforms; i++)
		    {
			if (ans[i] != 0)
			    continue;
			ans[i] = ch;
			if (ch != 'n')
			    continue;
			strlcpy(rejects[i], rsn, REASON_LEN);
		    }

		    ch = ' '; // go to page mode!
		    break;

		    // function keys
		case 'y':	// accept
#ifdef REGFORM_DISABLE_ONLINE_USER
		    if (forms[ci].online)
		    {
			vmsg("�Ȥ��}��f�֦b�u�W�ϥΪ̡C");
			break;
		    }
#endif
		case 's':	// skip
		case 'd':	// delete
		case KEY_DEL:	//delete
		    if (ch == KEY_DEL) ch = 'd';

		    grayout(ci*2, ci*2+1, GRAYOUT_DARK);
		    move_ansi(ci*2, 4); outc(ch);
		    ans[ci] = ch;
		    ch = 'j'; // go next
		    break;

		case 'u':	// undo
#ifdef REGFORM_DISABLE_ONLINE_USER
		    if (forms[ci].online)
		    {
			vmsg("�Ȥ��}��f�֦b�u�W�ϥΪ̡C");
			break;
		    }
#endif
		    grayout(ci*2, ci*2+1, GRAYOUT_NORM);
		    move_ansi(ci*2, 4); outc('.');
		    ans[ci] = 0;
		    ch = 'j'; // go next
		    break;

		case 'n':	// reject
#ifdef REGFORM_DISABLE_ONLINE_USER
		    if (forms[ci].online)
		    {
			vmsg("�Ȥ��}��f�֦b�u�W�ϥΪ̡C");
			break;
		    }
#endif
		    // query for reason
		    resolve_reason(rejects[ci], yMsg);
		    prompt_regform_ui();

		    if (!rejects[ci][0])
			break;

		    move(yMsg, 0);
		    prints("�h�^ %s ���U���]:\n %s\n", forms[ci].userid, rejects[ci]);

		    // do reject
		    grayout(ci*2, ci*2+1, GRAYOUT_DARK);
		    move_ansi(ci*2, 4); outc(ch);
		    ans[ci] = ch;
		    ch = 'j'; // go next

		    break;
	    } // switch(ch)

	    // change cursor
	    if (ch == 'j' && ++ci >= cforms)
		ci = cforms -1;
	} // while(ch != QUIT/SAVE)

	// if exit, we still need to skip all read forms
	if (ch == 'q')
	{
	    for (i = 0; i < cforms; i++)
		ans[i] = 's';
	}

	// page complete (save).
	assert(ch == ' ' || ch == 'q');

	// save/commit if required.
	if (dryrun) 
	{
	    // prmopt for debug
	    clear();
	    stand_title("���ռҦ�");
	    outs("�z���b������ռҦ��A�ҥH��f�����U��ä��|�ͮġC\n"
		    "�U���C�X���O��~�z�f�������G:\n\n");

	    for (i = 0; i < cforms; i++)
	    {
		if (ans[i] == 'y')
		    snprintf(justify, sizeof(justify), // build justify string
			    "%s:%s:%s", forms[i].phone, forms[i].career, cuser.userid);

		prints("%2d. %-12s - %c %s\n", i+1, forms[i].userid, ans[i],
			ans[i] == 'n' ? rejects[i] : 
			ans[i] == 'y' ? justify : "");
	    }
	    if (ch != 'q')
		pressanykey();
	} 
	else 
	{
	    // real functionality
	    for (i = 0; i < cforms; i++)
	    {
		if (ans[i] == 'y')
		{
		    // build justify string
		    snprintf(justify, sizeof(justify), 
			    "%s:%s:%s", forms[i].phone, forms[i].career, cuser.userid);

		    regform_accept(forms[i].userid, justify);
		    // log form to FN_REGISTER_LOG
		    append_regform(&forms[i], FN_REGISTER_LOG,
			    "Approved", cuser.userid, NULL);
		}
		else if (ans[i] == 'n')
		{
		    regform_reject(forms[i].userid, rejects[i]);
		    // log form to FN_REGISTER_LOG
		    append_regform(&forms[i], FN_REGISTER_LOG,
			    "Rejected", cuser.userid, rejects[i]);
		}
		else if (ans[i] == 's')
		{
		    // append form back to fn_register
		    append_regform(&forms[i], fn_register,
			    NULL, NULL, NULL);
		}
	    }
	} // !dryrun

    } // while (ch != 'q')

    // cleaning left regforms
    if (!dryrun)
    {
	pump_regform(regfile, fp);
	fclose(fp);
        unlink(fname);
    } else {
	// directly close file should be OK.
	fclose(fp);
    }

    return 0;
}

#endif // EXP_ADMIN_REGFORM

int
m_register(void)
{
    FILE           *fn;
    int             x, y, wid, len;
    char            ans[4];
    char            genbuf[200];

    if ((fn = fopen(fn_register, "r")) == NULL) {
	outs("�ثe�õL�s���U���");
	return XEASY;
    }
    stand_title("�f�֨ϥΪ̵��U���");
    y = 2;
    x = wid = 0;

    while (fgets(genbuf, STRLEN, fn) && x < 65) {
	if (strncmp(genbuf, "uid: ", 5) == 0) {
	    move(y++, x);
	    outs(genbuf + 5);
	    len = strlen(genbuf + 5);
	    if (len > wid)
		wid = len;
	    if (y >= t_lines - 3) {
		y = 2;
		x += wid + 2;
	    }
	}
    }
    fclose(fn);
    getdata(b_lines - 1, 0, 
#ifdef EXP_ADMIN_REGFORM
	    "�}�l�f�ֶ�(Auto�۰�/Yes���/No���f/Exp�s�ɭ�)�H[N] ", 
#else
	    "�}�l�f�ֶ�(Auto�۰�/Yes���/No���f)�H[N] ", 
#endif
	    ans, sizeof(ans), LCECHO);
    if (ans[0] == 'a')
	scan_register_form(fn_register, 1, NULL);
    else if (ans[0] == 'y')
	scan_register_form(fn_register, 0, NULL);

#ifdef EXP_ADMIN_REGFORM
    else if (ans[0] == 'e')
    {
#ifdef EXP_ADMIN_REGFORM_DRYRUN
	int dryrun = 0;
	if (getans("�A�n�i��´���(T)�٬O�u������f��(y)�H") == 'y')
	{
	    vmsg("�i�J��ڰ���Ҧ��A�Ҧ��f�ְʧ@���O�u���C");
	    dryrun = 0;
	} else {
	    vmsg("���ռҦ��C");
	    dryrun = 1;
	}
	handle_register_form(fn_register, dryrun);
#else
	// run directly.
	handle_register_form(fn_register, 0);
#endif
    }
#endif

    return 0;
}

int
cat_register(void)
{
    if (system("cat register.new.tmp >> register.new") == 0 &&
	unlink("register.new.tmp") == 0)
	vmsg("OK �P~~ �~��h�İ��a!!");
    else
	vmsg("�S��kCAT�L�h�O �h�ˬd�@�U�t�Χa!!");
    return 0;
}

static void
give_id_money(const char *user_id, int money, const char *mail_title)
{
    char            tt[TTLEN + 1] = {0};

    if (deumoney(searchuser(user_id, NULL), money) < 0) { // TODO if searchuser() return 0
	move(12, 0);
	clrtoeol();
	prints("id:%s money:%d ����a!!", user_id, money);
	pressanykey();
    } else {
	snprintf(tt, sizeof(tt), "%s : %d " MONEYNAME " ��", mail_title, money);
	mail_id(user_id, tt, "etc/givemoney.why", "[PTT �Ȧ�]");
    }
}

int
give_money(void)
{
    FILE           *fp, *fp2;
    char           *ptr, *id, *mn;
    char            buf[200] = "", reason[100], tt[TTLEN + 1] = "";
    int             to_all = 0, money = 0;
    int             total_money=0, count=0;

    getdata(0, 0, "���w�ϥΪ�(S) �����ϥΪ�(A) ����(Q)�H[S]", buf, 3, LCECHO);
    if (buf[0] == 'q')
	return 1;
    else if (buf[0] == 'a') {
	to_all = 1;
	getdata(1, 0, "�o�h�ֿ��O?", buf, 20, DOECHO);
	money = atoi(buf);
	if (money <= 0) {
	    move(2, 0);
	    vmsg("��J���~!!");
	    return 1;
	}
    } else {
	if (vedit("etc/givemoney.txt", NA, NULL) < 0)
	    return 1;
    }

    clear();

    unlink("etc/givemoney.log");
    if (!(fp2 = fopen("etc/givemoney.log", "w")))
	return 1;

    getdata(0, 0, "�ʥΰ�w!�п�J�����z��(�p���ʦW��):", reason, 40, LCECHO);
    fprintf(fp2,"\n�ϥβz��: %s\n", reason);

    getdata(1, 0, "�n�o���F��(Y/N)[N]", buf, 3, LCECHO);
    if (buf[0] != 'y')
       {
        fclose(fp2);
	return 1;
       }

    getdata(1, 0, "���]�U���D �G", tt, TTLEN, DOECHO);
    fprintf(fp2,"\n���]�U���D: %s\n", tt);
    move(2, 0);

    vmsg("�s���]�U���e");
    if (vedit("etc/givemoney.why", NA, NULL) < 0) {
        fclose(fp2);
	return 1;
    }

    stand_title("�o����...");
    if (to_all) {
	int             i, unum;
	for (unum = SHM->number, i = 0; i < unum; i++) {
	    if (bad_user_id(SHM->userid[i]))
		continue;
	    id = SHM->userid[i];
	    give_id_money(id, money, tt);
            fprintf(fp2,"�� %s : %d\n", id, money);
            count++;
	}
        sprintf(buf, "(%d�H:%d"MONEYNAME"��)", count, count*money);
        strcat(reason, buf);
    } else {
	if (!(fp = fopen("etc/givemoney.txt", "r+"))) {
	    fclose(fp2);
	    return 1;
	}
	while (fgets(buf, sizeof(buf), fp)) {
	    clear();
	    if (!(ptr = strchr(buf, ':')))
		continue;
	    *ptr = '\0';
	    id = buf;
	    mn = ptr + 1;
            money = atoi(mn);
	    give_id_money(id, money, tt);
            fprintf(fp2,"�� %s : %d\n", id, money);
            total_money += money;
            count++;
	}
	fclose(fp);
        sprintf(buf, "(%d�H:%d"MONEYNAME"��)", count, total_money);
        strcat(reason, buf);
    
    }

    fclose(fp2);

    sprintf(buf, "%s ���]��: %s", cuser.userid, reason);
    post_file(GLOBAL_SECURITY, buf, "etc/givemoney.log", "[���]�����i]");
    pressanykey();
    return FULLUPDATE;
}