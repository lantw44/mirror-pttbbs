/* $Id$ */
#include "bbs.h"

/* �i�����y�Ŷ� */
int
m_loginmsg()
{
  char msg[100];
  move(21,0);
  clrtobot();
  if(SHM->loginmsg.pid && SHM->loginmsg.pid != currutmp->pid)
    {
      prints(I18N[0]);
      getmessage(SHM->loginmsg);
    }
  getdata(22, 0, 
     I18N[1],
          msg, 3, LCECHO);

  if(msg[0]=='y' &&

     getdata_str(23, 0, I18N[2], msg, 56, DOECHO, SHM->loginmsg.last_call_in))
    {
          SHM->loginmsg.pid=currutmp->pid; /*�������h �N����race condition */
          strcpy(SHM->loginmsg.last_call_in, msg);
          strcpy(SHM->loginmsg.userid, cuser.userid);
    }
  return 0;
}

/* �ϥΪ̺޲z */
int
m_user()
{
    userec_t        muser;
    int             id;
    char            genbuf[200];

    stand_title(I18N[3]);
    usercomplete(msg_uid, genbuf);
    if (*genbuf) {
	move(2, 0);
	if ((id = getuser(genbuf))) {
	    memcpy(&muser, &xuser, sizeof(muser));
	    user_display(&muser, 1);
	    uinfo_query(&muser, 1, id);
	} else {
	    outs(err_uid);
	    clrtoeol();
	    pressanykey();
	}
    }
    return 0;
}

static int
search_key_user(char *passwdfile, int mode)
{
    userec_t        user;
    int             ch;
    int             coun = 0;
    FILE            *fp1 = fopen(passwdfile, "r");
    char            friendfile[128]="", key[22], genbuf[8],
                    *keymatch;


    assert(fp1);
    clear();
    getdata(0, 0, mode ? I18N[4] : I18N[5], key, sizeof(key), DOECHO);
    if(!key[0])
	return 0;
    while ((fread(&user, sizeof(user), 1, fp1)) > 0 && coun < MAX_USERS) {
	if (!(++coun & 15)) {
	    move(1, 0);
	    prints(I18N[6], coun);
	    refresh();
	}
        keymatch = NULL;
	if (!strcasecmp(user.userid, key))
             keymatch = user.userid; 
        else if(mode) {
             if(strstr(user.realname, key))
                 keymatch = user.realname; 
             else if(strstr(user.username, key))
                 keymatch = user.username; 
             else if(strstr(user.lasthost, key))
                 keymatch = user.lasthost; 
             else if(strstr(user.email, key))
                 keymatch = user.email; 
             else if(strstr(user.address, key))
                 keymatch = user.address; 
             else if(strstr(user.justify, key))
                 keymatch = user.justify; 
             else if(strstr(user.mychicken.name, key))
                 keymatch = user.mychicken.name; 
	     else if(strstr(user.ident, key))
		 keymatch = user.ident;
	}
        if(keymatch) {
	    move(1, 0);
	    prints(I18N[7], coun);
	    refresh();

	    user_display(&user, 1);
	    uinfo_query(&user, 1, coun);
	    outs(I18N[8]);
	    outs(mode ? 
                 "      A: add to namelist \033[m " :
		 I18N[9]);
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
		    if ((ch = searchuser(user.userid))) {
			setumoney(ch, user.money);
			passwd_update(ch, &user);
			fclose(fp1);
			return 0;
		    } else {
			move(b_lines - 1, 0);
			getdata(0, 0,
				I18N[10],
				genbuf, 3, LCECHO);
			if (genbuf[0] != 'y') {
			    outs(I18N[11]);
			} else {
			    int             allocid = getnewuserid();

			    if (allocid > MAX_USERS || allocid <= 0) {
				fprintf(stderr, I18N[12]);
				exit(1);
			    }
			    if (passwd_update(allocid, &user) == -1) {
				fprintf(stderr, I18N[13]);
				exit(1);
			    }
			    setuserid(allocid, user.userid);
			    if (!searchuser(user.userid)) {
				fprintf(stderr, I18N[14]);
				exit(1);
			    }
			    fclose(fp1);
			    return 0;
			}
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
search_user_bypwd()
{
    search_key_user(FN_PASSWD, 1);
    return 0;
}

/* �M��ƥ����ϥΪ̸�� */
int
search_user_bybakpwd()
{
    char           *choice[] = {
	"PASSWDS.NEW1", "PASSWDS.NEW2", "PASSWDS.NEW3",
	"PASSWDS.NEW4", "PASSWDS.NEW5", "PASSWDS.NEW6",
	"PASSWDS.BAK"
    };
    int             ch;

    clear();
    move(1, 1);
    outs(I18N[15]);
    outs(I18N[16]);
    outs(I18N[17]);
    outs(I18N[18]);
    do {
	move(5, 1);
	outs(I18N[19]);
	ch = igetch();
	if (ch == 'q' || ch == 'Q')
	    return 0;
    } while (ch < '1' || ch > '8');
    ch -= '1';
    search_key_user(choice[ch], 0);
    return 0;
}

static void
bperm_msg(boardheader_t * board)
{
    prints(I18N[20], board->brdname,
	   board->brdattr & BRD_POSTMASK ? I18N[21] : I18N[22]);
}

unsigned int
setperms(unsigned int pbits, int *pstring)
{
    register int    i;
    char            choice[4];

    move(4, 0);
    for (i = 0; i < NUMPERMS / 2; i++) {
	prints("%c. %-20s %-15s %c. %-20s %s\n",
	       'A' + i, I18N[pstring[i]],
	       ((pbits >> i) & 1 ? I18N[23] : I18N[24]),
	       i < 10 ? 'Q' + i : '0' + i - 10,
	       I18N[pstring[i + 16]],
	       ((pbits >> (i + 16)) & 1 ? I18N[25] : I18N[26]));
    }
    clrtobot();
    while(
    	(i = getkey(I18N[27]))!='\r')
    	{
    i = i - 'a';
	if (i < 0)
	    i = choice[0] - '0' + 26;
	if (i >= NUMPERMS)
	    bell();
	else {
	    pbits ^= (1 << i);
	    move(i % 16 + 4, i <= 15 ? 24 : 64);
	    prints((pbits >> i) & 1 ? I18N[28] : I18N[29]);
	}
    }
    return pbits;
}

/* �۰ʳ]�ߺ�ذ� */
void
setup_man(boardheader_t * board)
{
    char            genbuf[200];

    setapath(genbuf, board->brdname);
    mkdir(genbuf, 0755);
}

void delete_symbolic_link(boardheader_t *bh, int bid)
{
    memset(bh, 0, sizeof(boardheader_t));
    substitute_record(fn_board, bh, sizeof(boardheader_t), bid);
    reset_board(bid);
    sort_bcache();
    log_usies("DelLink", bh->brdname);
}

int
m_mod_board(char *bname)
{
    boardheader_t   bh, newbh;
    int             bid;
    char            genbuf[256], ans[4];

    bid = getbnum(bname);
    if (!bid || !bname[0] || get_record(fn_board, &bh, sizeof(bh), bid) == -1) {
	outs(err_bid);
	pressanykey();
	return -1;
    }
    prints(I18N[30], bh.brdname, bh.title, bid, bh.gid, bh.BM);
    bperm_msg(&bh);

    /* Ptt �o���_��|�ɨ�U�� */
    move(9, 0);
    snprintf(genbuf, sizeof(genbuf), I18N[31],
	    HAS_PERM(PERM_SYSOP) ? I18N[32] : "",
	    HAS_PERM(PERM_SYSSUBOP) ? I18N[33] : "");
    getdata(10, 0, genbuf, ans, sizeof(ans), LCECHO);

    switch (*ans) {
    case 'g':
	if (HAS_PERM(PERM_SYSOP)) {
	    char            path[256];
	    setbfile(genbuf, bname, FN_TICKET_LOCK);
	    setbfile(path, bname, FN_TICKET_END);
	    rename(genbuf, path);
	}
	break;
    case 's':
	if (HAS_PERM(PERM_SYSOP)) {
	  snprintf(genbuf, sizeof(genbuf),
		   BBSHOME "/bin/buildir boards/%c/%s &",
		   bh.brdname[0], bh.brdname);
	    system(genbuf);
	}
	break;
    case 'b':
	if (HAS_PERM(PERM_SYSOP)) {
	    char            bvotebuf[10];

	    memcpy(&newbh, &bh, sizeof(bh));
	    snprintf(bvotebuf, sizeof(bvotebuf), "%d", newbh.bvote);
	    move(20, 0);
	    prints(I18N[34], bh.brdname, bh.bvote);
	    getdata_str(21, 0, I18N[35], genbuf, 5, LCECHO, bvotebuf);
	    newbh.bvote = atoi(genbuf);
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
	    log_usies("SetBoardBvote", newbh.brdname);
	    break;
	} else
	    break;
    case 'v':
	memcpy(&newbh, &bh, sizeof(bh));
	outs(I18N[36]);
	outs((bh.brdattr & BRD_BAD) ? I18N[37] : I18N[38]);
	getdata(21, 0, I18N[39], genbuf, 5, LCECHO);
	if (genbuf[0] == 'y') {
	    if (newbh.brdattr & BRD_BAD)
		newbh.brdattr = newbh.brdattr & (!BRD_BAD);
	    else
		newbh.brdattr = newbh.brdattr | BRD_BAD;
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
	    log_usies("ViolateLawSet", newbh.brdname);
	}
	break;
    case 'd':
	if (HAS_PERM(PERM_SYSSUBOP) && !HAS_PERM(PERM_SYSOP))
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
		    "/bin/tar zcvf tmp/board_%s.tgz boards/%c/%s man/boards/%c/%s >/dev/null 2>&1;/bin/rm -fr boards/%c/%s man/boards/%c/%s",
		    bname, bname[0], bname, bname[0],
		    bname, bname[0], bname, bname[0], bname);
	    system(genbuf);
	    memset(&bh, 0, sizeof(bh));
	    snprintf(bh.title, sizeof(bh.title),
		     I18N[40], bname, cuser.userid);
	    post_msg("Security", bh.title, I18N[41], I18N[42]);
	    substitute_record(fn_board, &bh, sizeof(bh), bid);
	    reset_board(bid);
	    sort_bcache();
	    log_usies("DelBoard", bh.title);
	    outs(I18N[43]);
	}
	break;
    case 'e':
	move(8, 0);
	outs(I18N[44]);
	memcpy(&newbh, &bh, sizeof(bh));

	while (getdata(9, 0, I18N[45], genbuf, IDLEN + 1, DOECHO)) {
	    if (getbnum(genbuf)) {
		move(3, 0);
		outs(I18N[46]);
	    } else if ( !invalid_brdname(genbuf) ){
		strlcpy(newbh.brdname, genbuf, sizeof(newbh.brdname));
		break;
	    }
	}

	do {
	    getdata_str(12, 0, I18N[47], genbuf, 5, DOECHO, bh.title);
	    if (strlen(genbuf) == 4)
		break;
	} while (1);

	if (strlen(genbuf) >= 4)
	    strncpy(newbh.title, genbuf, 4);

	newbh.title[4] = ' ';

	getdata_str(14, 0, I18N[48], genbuf, BTLEN + 1, DOECHO,
		    bh.title + 7);
	if (genbuf[0])
	    strlcpy(newbh.title + 7, genbuf, sizeof(newbh.title) - 7);
	if (getdata_str(15, 0, I18N[49], genbuf, IDLEN * 3 + 3, DOECHO,
			bh.BM)) {
	    trim(genbuf);
	    strlcpy(newbh.BM, genbuf, sizeof(newbh.BM));
	}
	if (HAS_PERM(PERM_SYSOP)) {
	    move(1, 0);
	    clrtobot();
	    newbh.brdattr = setperms(newbh.brdattr, str_permboard);
	    move(1, 0);
	    clrtobot();
	}
	if (newbh.brdattr & BRD_GROUPBOARD)
	    strncpy(newbh.title + 5, I18N[50], 2);
	else if (newbh.brdattr & BRD_NOTRAN)
	    strncpy(newbh.title + 5, I18N[51], 2);
	else
	    strncpy(newbh.title + 5, I18N[52], 2);

	if (HAS_PERM(PERM_SYSOP) && !(newbh.brdattr & BRD_HIDE)) {
	    getdata_str(14, 0, I18N[53], ans, sizeof(ans), LCECHO, "N");
	    if (*ans == 'y') {
		getdata_str(15, 0, I18N[54], ans, sizeof(ans), LCECHO,
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
	getdata(b_lines - 1, 0, I18N[55], genbuf, 4, LCECHO);

	if ((*genbuf != 'n') && memcmp(&newbh, &bh, sizeof(bh))) {
	    if (strcmp(bh.brdname, newbh.brdname)) {
		char            src[60], tar[60];

		setbpath(src, bh.brdname);
		setbpath(tar, newbh.brdname);
		Rename(src, tar);

		setapath(src, bh.brdname);
		setapath(tar, newbh.brdname);
		Rename(src, tar);
	    }
	    setup_man(&newbh);
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
	    sort_bcache();
	    log_usies("SetBoard", newbh.brdname);
	}
    }
    return 0;
}

/* �]�w�ݪO */
int
m_board()
{
    char            bname[32];

    stand_title(I18N[56]);
    generalnamecomplete(msg_bid, bname, sizeof(bname), SHM->Bnumber,
			completeboard_compar,
			completeboard_permission,
			completeboard_getname);
    if (!*bname)
	return 0;
    m_mod_board(bname);
    return 0;
}

/* �]�w�t���ɮ� */
int
x_file()
{
    int             aborted;
    char            ans[4], *fpath;

    move(b_lines - 6, 0);
    /* Ptt */
    outs(I18N[57]);
    outs(I18N[58]);
    outs(I18N[59]);
#ifdef MULTI_WELCOME_LOGIN
	 outs(I18N[60]);
#endif
	outs("\n");
    outs(I18N[61]);
    outs(I18N[62]);
    getdata(b_lines - 1, 0, I18N[63], ans, sizeof(ans), LCECHO);

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
	getdata(b_lines - 1, 0, I18N[64], ans, sizeof(ans), LCECHO);
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
	getdata(b_lines - 1, 0, I18N[65], ans, sizeof(ans), LCECHO);
	if (ans[0] == '1') {
	    unlink("etc/Welcome_login.1");
	    outs("ok");
	} else if (ans[0] == '2') {
	    unlink("etc/Welcome_login.2");
	    outs("ok");
	} else if (ans[0] == '3') {
	    unlink("etc/Welcome_login.3");
	    outs("ok");
	} else if (ans[0] == '4') {
	    unlink("etc/Welcome_login.4");
	    outs("ok");
	} else {
	    outs(I18N[66]);
	}
	pressanykey();
	return FULLUPDATE;

#endif

    case 'h':
	fpath = "etc/expire.conf";
	break;
    case 'i':
	fpath = "etc/domain_name_query";
	break;
    case 'j':
	fpath = "etc/Logout";
	break;
    case 'k':
	fpath = "etc/Welcome_birth";
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
    default:
	return FULLUPDATE;
    }
    aborted = vedit(fpath, NA, NULL);
    prints(I18N[67], fpath,
	   (aborted == -1) ? I18N[68] : I18N[69]);
    pressanykey();
    return FULLUPDATE;
}

static int add_board_record(boardheader_t *board)
{
    int bid;
    if ((bid = getbnum("")) > 0) {
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

int
m_newbrd(int recover)
{
    boardheader_t   newboard;
    char            ans[4];
    char            genbuf[200];

    stand_title(I18N[70]);
    memset(&newboard, 0, sizeof(newboard));

    newboard.gid = class_bid;
    if (newboard.gid == 0) {
	move(6, 0);
	outs(I18N[71]);
	pressanykey();
	return -1;
    }
    do {
	if (!getdata(3, 0, msg_bid, newboard.brdname,
		     sizeof(newboard.brdname), DOECHO))
	    return -1;
    } while (invalid_brdname(newboard.brdname));

    do {
	getdata(6, 0, I18N[72], genbuf, 5, DOECHO);
	if (strlen(genbuf) == 4)
	    break;
    } while (1);

    strncpy(newboard.title, genbuf, 4);

    newboard.title[4] = ' ';

    getdata(8, 0, I18N[73], genbuf, BTLEN + 1, DOECHO);
    if (genbuf[0])
	strlcpy(newboard.title + 7, genbuf, sizeof(newboard.title) - 7);
    setbpath(genbuf, newboard.brdname);

    if (recover) {
	if (dashd(genbuf)) {
	    outs(I18N[74]);
	    pressanykey();
	    return -1;
	}
    } else if (getbnum(newboard.brdname) > 0 || mkdir(genbuf, 0755) == -1) {
	outs(I18N[75]);
	pressanykey();
	return -1;
    }
    newboard.brdattr = BRD_NOTRAN;

    if (HAS_PERM(PERM_SYSOP)) {
	move(1, 0);
	clrtobot();
	newboard.brdattr = setperms(newboard.brdattr, str_permboard);
	move(1, 0);
	clrtobot();
    }
    getdata(9, 0, I18N[76], genbuf, 3, LCECHO);
    if (genbuf[0] == 'n')
	newboard.brdattr |= BRD_GROUPBOARD;

    if (newboard.brdattr & BRD_GROUPBOARD)
	strncpy(newboard.title + 5, I18N[77], 2);
    else if (newboard.brdattr & BRD_NOTRAN)
	strncpy(newboard.title + 5, I18N[78], 2);
    else
	strncpy(newboard.title + 5, I18N[79], 2);

    newboard.level = 0;
    getdata(11, 0, I18N[80], newboard.BM, sizeof(newboard.BM), DOECHO);

    if (HAS_PERM(PERM_SYSOP) && !(newboard.brdattr & BRD_HIDE)) {
	getdata_str(14, 0, I18N[81], ans, sizeof(ans), LCECHO, "N");
	if (*ans == 'y') {
	    getdata_str(15, 0, I18N[82], ans, sizeof(ans), LCECHO, "R");
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
    getbcache(class_bid)->childcount = 0;
    pressanykey();
    setup_man(&newboard);

    outs(I18N[83]);
    post_newboard(newboard.title, newboard.brdname, newboard.BM);
    log_usies("NewBoard", newboard.title);
    pressanykey();
    return 0;
}

int make_symbolic_link(char *bname, int gid)
{
    boardheader_t   newboard;
    int bid;
    
    bid = getbnum(bname);
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
    strcpy(newboard.title + 5, I18N[84]);

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

    generalnamecomplete(msg_bid, buf, sizeof(buf), SHM->Bnumber,
			completeboard_compar,
			completeboard_permission,
			completeboard_getname);
    if (!buf[0])
	return -1;

    stand_title(I18N[85]);

    if (make_symbolic_link(buf, gid) < 0) {
	vmsg(I18N[86]);
	return -1;
    }
    log_usies("NewSymbolic", buf);
    return 0;
}

static int
auto_scan(char fdata[][STRLEN], char ans[])
{
    int             good = 0;
    int             count = 0;
    int             i;
    char            temp[10];

    if (!strncmp(fdata[2], I18N[87], 2) || strstr(fdata[2], I18N[88])
	|| strstr(fdata[2], I18N[89]) || strstr(fdata[2], I18N[90])) {
	ans[0] = '0';
	return 1;
    }
    strncpy(temp, fdata[2], 2);
    temp[2] = '\0';

    /* �|�r */
    if (!strncmp(temp, &(fdata[2][2]), 2)) {
	ans[0] = '0';
	return 1;
    }
    if (strlen(fdata[2]) >= 6) {
	if (strstr(fdata[2], I18N[91])) {
	    ans[0] = '0';
	    return 1;
	}
	if (strstr(I18N[92], temp))
	    good++;
	else if (strstr(I18N[93], temp))
	    good++;
	else if (strstr(I18N[94], temp))
	    good++;
	else if (strstr(I18N[95], temp))
	    good++;
    }
    if (!good)
	return 0;

    if (!strcmp(fdata[3], fdata[4]) ||
	!strcmp(fdata[3], fdata[5]) ||
	!strcmp(fdata[4], fdata[5])) {
	ans[0] = '4';
	return 5;
    }
    if (strstr(fdata[3], I18N[96])) {
	if (strstr(fdata[3], I18N[97]) || strstr(fdata[3], I18N[98]) ||
	    strstr(fdata[3], I18N[99]) || strstr(fdata[3], I18N[100]) ||
	    strstr(fdata[3], I18N[101]) || strstr(fdata[3], I18N[102]) ||
	    strstr(fdata[3], I18N[103]) || strstr(fdata[3], I18N[104]) ||
	    strstr(fdata[3], I18N[105]) || strstr(fdata[3], I18N[106]) ||
	    strstr(fdata[3], I18N[107]) || strstr(fdata[3], I18N[108]))
	    good++;
    } else if (strstr(fdata[3], I18N[109]))
	good++;

    if (strstr(fdata[4], I18N[110]) || strstr(fdata[4], I18N[111]) ||
	strstr(fdata[4], I18N[112])) {
	ans[0] = '2';
	return 3;
    }
    if (strstr(fdata[4], I18N[113]) || strstr(fdata[4], I18N[114])) {
	if (strstr(fdata[4], I18N[115]) || strstr(fdata[4], I18N[116])) {
	    if (strstr(fdata[4], I18N[117]))
		good++;
	}
    }
    for (i = 0; fdata[5][i]; i++) {
	if (isdigit(fdata[5][i]))
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

/* �B�z Register Form */
int
scan_register_form(char *regfile, int automode, int neednum)
{
    char            genbuf[200];
    char    *logfile = "register.log";
    char    *field[] = {
	"uid", "ident", "name", "career", "addr", "phone", "email", NULL
    };
    char    *finfo[] = {
	I18N[118], I18N[119], I18N[120], I18N[121], I18N[122],
	I18N[123], I18N[124], NULL
    };
    char    *reason[] = {
	I18N[125],
	I18N[126],
	I18N[127],
	I18N[128],
	I18N[129],
	I18N[130],
	I18N[131],
	NULL
    };
    char    *autoid = "AutoScan";
    userec_t        muser;
    FILE           *fn, *fout, *freg;
    char            fdata[7][STRLEN];
    char            fname[STRLEN], buf[STRLEN];
    char            ans[4], *ptr, *uid;
    int             n = 0, unum = 0;
    int             nSelf = 0, nAuto = 0;

    uid = cuser.userid;
    snprintf(fname, sizeof(fname), "%s.tmp", regfile);
    move(2, 0);
    if (dashf(fname)) {
	if (neednum == 0) {	/* �ۤv�i Admin �Ӽf�� */
	    outs(I18N[132]);
	    pressanykey();
	}
	return -1;
    }
    Rename(regfile, fname);
    if ((fn = fopen(fname, "r")) == NULL) {
	prints(I18N[133], fname);
	pressanykey();
	return -1;
    }
    if (neednum) {		/* �Q�j���f�� */
	move(1, 0);
	clrtobot();
	prints(I18N[134], neednum);
	prints(I18N[135]);
	pressanykey();
    }
    memset(fdata, 0, sizeof(fdata));
    while (fgets(genbuf, STRLEN, fn)) {
	if ((ptr = (char *)strstr(genbuf, ": "))) {
	    *ptr = '\0';
	    for (n = 0; field[n]; n++) {
		if (strcmp(genbuf, field[n]) == 0) {
		    strlcpy(fdata[n], ptr + 2, sizeof(fdata[n]));
		    if ((ptr = (char *)strchr(fdata[n], '\n')))
			*ptr = '\0';
		}
	    }
	} else if ((unum = getuser(fdata[0])) == 0) {
	    move(2, 0);
	    clrtobot();
	    outs(I18N[136]);
	    for (n = 0; field[n]; n++)
		prints("%s     : %s\n", finfo[n], fdata[n]);
	    pressanykey();
	    neednum--;
	} else {
	    neednum--;
	    memcpy(&muser, &xuser, sizeof(muser));
	    if (automode)
		uid = autoid;

	    if ((!automode || !auto_scan(fdata, ans))) {
		uid = cuser.userid;

		move(1, 0);
		prints(I18N[137], unum);
		user_display(&muser, 1);
		move(14, 0);
		prints(I18N[138], neednum);
	    	prints(I18N[139], finfo[0], fdata[0]);
		prints(I18N[140], finfo[1], fdata[1]);
#ifdef FOREIGN_REG
		prints(I18N[141], finfo[2], fdata[2], muser.uflag2 & FOREIGN ? I18N[142] : "");
#else
		prints(I18N[143], finfo[2], fdata[2]);
#endif
		for (n = 3; field[n]; n++) {
		    prints(I18N[144], n - 2, finfo[n], fdata[n]);
		}
		if (muser.userlevel & PERM_LOGINOK) {
		    ans[0] = getkey(I18N[145]);
		    if (ans[0] != 'y' && ans[0] != 's')
			ans[0] = 'd';
		} else {
			if (search_ulist(unum) == NULL)
				ans[0] = vmsg_lines(22, I18N[146]);
			else
			ans[0] = 's';
		    if ('A' <= ans[0] && ans[0] <= 'Z')
			ans[0] += 32;
		    if (ans[0] != 'y' && ans[0] != 'n' && ans[0] != 'q' &&
			ans[0] != 'd' && !('0' <= ans[0] && ans[0] <= '4'))
			ans[0] = 's';
		    ans[1] = 0;
		}
		nSelf++;
	    } else
		nAuto++;
	    if (neednum > 0 && ans[0] == 'q') {
		move(2, 0);
		clrtobot();
		prints(I18N[147]);
		pressanykey();
		ans[0] = 's';
	    }
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
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case 'n':
		if (ans[0] == 'n') {
		    for (n = 0; field[n]; n++)
			prints("%s: %s\n", finfo[n], fdata[n]);
		    move(9, 0);
		    prints(I18N[148]);
		    for (n = 0; reason[n]; n++)
			prints(I18N[149], n, reason[n]);
		} else
		    buf[0] = ans[0];
		if (ans[0] != 'n' ||
		    getdata(10 + n, 0, I18N[150], buf, 60, DOECHO))
		    if ((buf[0] - '0') >= 0 && (buf[0] - '0') < n) {
			int             i;
			fileheader_t    mhdr;
			char            title[128], buf1[80];
			FILE           *fp;

			sethomepath(buf1, muser.userid);
			stampfile(buf1, &mhdr);
			strlcpy(mhdr.owner, cuser.userid, sizeof(mhdr.owner));
			strlcpy(mhdr.title, I18N[151], TTLEN);
			mhdr.filemode = 0;
			sethomedir(title, muser.userid);
			if (append_record(title, &mhdr, sizeof(mhdr)) != -1) {
			    fp = fopen(buf1, "w");
			    
			    for(i = 0; buf[i] && i < sizeof(buf); i++){
				if (!isdigit(buf[i]))
				    continue;
				snprintf(genbuf, sizeof(genbuf),
				    I18N[152], reason[buf[i] - '0']);
				fprintf(fp, "%s\n", genbuf);
			    }

			    fclose(fp);
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
		prints(I18N[153]);
	    case 's':
		if ((freg = fopen(regfile, "a"))) {
		    for (n = 0; field[n]; n++)
			fprintf(freg, "%s: %s\n", field[n], fdata[n]);
		    fprintf(freg, "----\n");
		    fclose(freg);
		}
		break;
	    default:
		prints(I18N[154]);
		mail_muser(muser, I18N[155], "etc/registered");
		if(muser.uflag2 & FOREIGN)
		    mail_muser(muser, I18N[156], "etc/foreign_welcome");
		muser.userlevel |= (PERM_LOGINOK | PERM_POST);
		strlcpy(muser.realname, fdata[2], sizeof(muser.realname));
		strlcpy(muser.address, fdata[4], sizeof(muser.address));
		strlcpy(muser.email, fdata[6], sizeof(muser.email));
		strncpy(muser.justify, genbuf, REGLEN);
		sethomefile(buf, muser.userid, "justify");
		log_file(buf, 1, "%s:%s:%s\n", fdata[5], fdata[3], uid);
		passwd_update(unum, &muser);

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
    prints(I18N[157], nSelf, nAuto);

    /**	DickG: �N�f�F�X����������� post �� Security �O�W	***********/
    /*
     * DickG: �]���s�������W���ݼf�֤�סA�O�G�S�����n�d�U record ime(buf,
     * 200, "%Y/%m/%d/%H:%M", pt);
     * 
     * strcpy(xboard, "Security"); setbpath(xfpath, xboard); stampfile(xfpath,
     * &xfile); strcpy(xfile.owner, "�t��"); strcpy(xfile.title, "[���i]
     * �f�ְO��"); xptr = fopen(xfpath, "w"); fprintf(xptr, "\n�ɶ��G%s %s
     * �f�F %d �����U��\n AutoScan �f�F %d �����U��\n �@�p %d ���C", buf,
     * cuser.userid, nSelf, nAuto, nSelf+nAuto); fclose(xptr); setbdir(fname,
     * xboard); append_record(fname, &xfile, sizeof(xfile));
     * outgo_post(&xfile, xboard); touchbtotal(getbnum(xboard));
     * cuser.numposts++;
     */
    /*********************************************/
    pressanykey();
    return (0);
}

int
m_register()
{
    FILE           *fn;
    int             x, y, wid, len;
    char            ans[4];
    char            genbuf[200];

    if ((fn = fopen(fn_register, "r")) == NULL) {
	outs(I18N[158]);
	return XEASY;
    }
    stand_title(I18N[159]);
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
    getdata(b_lines - 1, 0, I18N[160], ans, sizeof(ans), LCECHO);
    if (ans[0] == 'a')
	scan_register_form(fn_register, 1, 0);
    else if (ans[0] == 'y')
	scan_register_form(fn_register, 0, 0);

    return 0;
}

int
cat_register()
{
    if (system("cat register.new.tmp >> register.new") == 0 &&
	system("rm -f register.new.tmp") == 0)
	vmsg(I18N[161]);
    else
	vmsg(I18N[162]);
    return 0;
}

static void
give_id_money(char *user_id, int money, FILE * log_fp, char *mail_title, time_t t)
{
    char            tt[TTLEN + 1] = {0};

    if (deumoney(searchuser(user_id), money) < 0) {
	move(12, 0);
	clrtoeol();
	prints(I18N[163], user_id, money);
	pressanykey();
    } else {
	fprintf(log_fp, "%d %s %d", (int)t, user_id, money);
	snprintf(tt, sizeof(tt), I18N[164], mail_title, money);
	mail_id(user_id, tt, "etc/givemoney.why", I18N[165]);
    }
}

int
give_money()
{
    FILE           *fp, *fp2;
    char           *ptr, *id, *mn;
    char            buf[200] = "", tt[TTLEN + 1] = "";
    struct tm      *pt = localtime(&now);
    int             to_all = 0, money = 0;

    getdata(0, 0, I18N[166], buf, sizeof(buf), LCECHO);
    if (buf[0] == 'q')
	return 1;
    else if (buf[0] == 'a') {
	to_all = 1;
	getdata(1, 0, I18N[167], buf, 20, DOECHO);
	money = atoi(buf);
	if (money <= 0) {
	    move(2, 0);
	    prints(I18N[168]);
	    pressanykey();
	    return 1;
	}
    } else {
	if (vedit("etc/givemoney.txt", NA, NULL) < 0)
	    return 1;
    }

    clear();
    getdata(0, 0, I18N[169], buf, 3, LCECHO);
    if (buf[0] != 'y')
	return 1;

    if (!(fp2 = fopen("etc/givemoney.log", "a")))
	return 1;
    strftime(buf, sizeof(buf), "%Y/%m/%d/%H:%M", pt);
    fprintf(fp2, "%s\n", buf);


    getdata(1, 0, I18N[170], tt, TTLEN, DOECHO);
    move(2, 0);

    prints(I18N[171]);
    pressanykey();
    if (vedit("etc/givemoney.why", NA, NULL) < 0) {
        fclose(fp2);
	return 1;
    }

    stand_title(I18N[172]);
    if (to_all) {
	int             i, unum;
	for (unum = SHM->number, i = 0; i < unum; i++) {
	    if (bad_user_id(SHM->userid[i]))
		continue;
	    id = SHM->userid[i];
	    give_id_money(id, money, fp2, tt, now);
	}
	//something wrong @ _ @
	    //give_money_post(I18N[173], atoi(money));
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
	    give_id_money(id, atoi(mn), fp2, tt, now);
	    give_money_post(id, atoi(mn));
	}
	fclose(fp);
    }

    fclose(fp2);
    pressanykey();
    return FULLUPDATE;
}
