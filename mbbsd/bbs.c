/* $Id$ */
#include "bbs.h"

static int recommend(int ent, fileheader_t * fhdr, char *direct);

#ifdef ASSESS
static char *badpost_reason[] = {
    SHM->i18nstr[cuser.language][245], SHM->i18nstr[cuser.language][246], SHM->i18nstr[cuser.language][247]
};
#endif

static void
mail_by_link(char *owner, char *title, char *path)
{
    char            genbuf[200];
    fileheader_t    mymail;
  
    sethomepath(genbuf, cuser.userid); 
    stampfile(genbuf, &mymail);
    strlcpy(mymail.owner, owner, sizeof(mymail.owner));
    snprintf(mymail.title, sizeof(mymail.title), title);
    unlink(genbuf);
    Link(path, genbuf);
    sethomedir(genbuf, cuser.userid);

    append_record(genbuf, &mymail, sizeof(mymail));
}


void
anticrosspost()
{
	log_file("etc/illegal_money", 1, SHM->i18nstr[cuser.language][248], cuser.userid, ctime(&now));
    post_violatelaw(cuser.userid, SHM->i18nstr[cuser.language][249], "Cross-post", SHM->i18nstr[cuser.language][250]);
    cuser.userlevel |= PERM_VIOLATELAW;
    cuser.vl_count++;
    mail_by_link(SHM->i18nstr[cuser.language][251], SHM->i18nstr[cuser.language][252],
		 BBSHOME "/etc/crosspost.txt");
    u_exit("Cross Post");
    exit(0);
}

/* Heat CharlieL */
int
save_violatelaw()
{
    char            buf[128], ok[3];

    setutmpmode(VIOLATELAW);
    clear();
    stand_title(SHM->i18nstr[cuser.language][253]);

    if (!(cuser.userlevel & PERM_VIOLATELAW)) {
	mouts(22, 0, SHM->i18nstr[cuser.language][254]);
	pressanykey();
	return 0;
    }
    reload_money();
    if (cuser.money < (int)cuser.vl_count * 1000) {
	snprintf(buf, sizeof(buf), SHM->i18nstr[cuser.language][255],
           (int)cuser.vl_count, (int)cuser.vl_count * 1000, cuser.money);
	mouts(22, 0, buf);
	pressanykey();
	return 0;
    }
    move(5, 0);
    prints(SHM->i18nstr[cuser.language][256]);
    prints(SHM->i18nstr[cuser.language][257]);

    if (!getdata(10, 0, SHM->i18nstr[cuser.language][258], ok, sizeof(ok), LCECHO) ||
	ok[0] == 'n' || ok[0] == 'N') {
	mouts(22, 0, SHM->i18nstr[cuser.language][259]);
	pressanykey();
	return 0;
    }
    snprintf(buf, sizeof(buf), SHM->i18nstr[cuser.language][260],
	     cuser.vl_count, cuser.vl_count * 1000);
    mouts(11, 0, buf);

    if (!getdata(10, 0, SHM->i18nstr[cuser.language][261], ok, sizeof(ok), LCECHO) ||
	ok[0] == 'N' || ok[0] == 'n') {

	mouts(22, 0, SHM->i18nstr[cuser.language][262]);
	pressanykey();
	return 0;
    }
    demoney(-1000 * cuser.vl_count);
    cuser.userlevel &= (~PERM_VIOLATELAW);
    passwd_update(usernum, &cuser);
    return 0;
}

/*
 * void make_blist() { CreateNameList(); apply_boards(g_board_names); }
 */

static time_t   *board_note_time;

void
set_board()
{
    boardheader_t  *bp;

    bp = getbcache(currbid);
    if( !HasPerm(bp) ){
	vmsg("access control violation, exit");
	u_exit("access control violation!");
	exit(-1);
    }
    board_note_time = &bp->bupdate;
    if(bp->BM[0] <= ' ')
	strcpy(currBM, SHM->i18nstr[cuser.language][263]);
    else
	snprintf(currBM, sizeof(currBM), SHM->i18nstr[cuser.language][264], bp->BM);

    /* init basic perm, but post perm is checked on demand */
    currmode = (currmode & (MODE_DIRTY | MODE_GROUPOP)) | MODE_STARTED;
    if (HAS_PERM(PERM_ALLBOARD) || is_BM_cache(currbid)) {
	currmode = currmode | MODE_BOARD | MODE_POST | MODE_POSTCHECKED;
    }
}

/* check post perm on demand, no double checks in current board */
int CheckPostPerm(void)
{
    if (!(currmode & MODE_POSTCHECKED)) {
	currmode |= MODE_POSTCHECKED;
	if (haspostperm(currboard)) {
	    currmode |= MODE_POST;
	    return 1;
	}
	currmode &= ~MODE_POST;
	return 0;
    }
    return (currmode & MODE_POST);
}

static void
readtitle()
{
    boardheader_t  *bp;
    char    *brd_title;

    bp = getbcache(currbid);
    if(bp->bvote != 2 && bp->bvote)
	brd_title = SHM->i18nstr[cuser.language][265];
    else
	brd_title = bp->title + 7;

    showtitle(currBM, brd_title);
    prints(SHM->i18nstr[cuser.language][266], SHM->bcache[currbid - 1].nuser);
}

static void
readdoent(int num, fileheader_t * ent)
{
    int             type;
    char           *mark, *title, color, special = 0, isonline = 0, recom[5];
    userinfo_t     *uentp;
    type = brc_unread(ent->filename, brc_num, brc_list) ? '+' : ' ';

    if ((currmode & MODE_BOARD) && (ent->filemode & FILE_DIGEST))
	type = (type == ' ') ? '*' : '#';
    else if (currmode & MODE_BOARD || HAS_PERM(PERM_LOGINOK)) {
	if (ent->filemode & FILE_BOTTOM)
	    type = '_' ;
	else if (ent->filemode & FILE_MARKED)
	    type = (type == ' ') ? 'm' : 'M';

	else if (TagNum && !Tagger(atoi(ent->filename + 2), 0, TAG_NIN))
	    type = 'D';

	else if (ent->filemode & FILE_SOLVED)
	    type = 's';
    }
    title = subject(mark = ent->title);
    if (ent->filemode & FILE_VOTE)
	color = '2', mark = SHM->i18nstr[cuser.language][267];
    else if (ent->filemode & FILE_BID)
	color = '6', mark = SHM->i18nstr[cuser.language][268];
    else if (title == mark)
	color = '1', mark = SHM->i18nstr[cuser.language][269];
    else
	color = '3', mark = "R:";

    if (title[45])
	strlcpy(title + 42, SHM->i18nstr[cuser.language][270], sizeof(title) - 42);	/* ��h�l�� string �屼 */

    if (!strncmp(title, SHM->i18nstr[cuser.language][271], 6))
	special = 1;
#if 1
    if (!strchr(ent->owner, '.') && !SHM->GV2.e.noonlineuser &&
	(uentp = search_ulist_userid(ent->owner)) && isvisible(currutmp, uentp))
	isonline = 1;
#else
    if (!strchr(ent->owner, '.') && (uid = searchuser(ent->owner)) &&
	!SHM->GV2.e.noonlineuser &&
	(uentp = search_ulist(uid)) && isvisible(currutmp, uentp))
	isonline = 1;
#endif
    if(ent->recommend>99)
	  strcpy(recom,SHM->i18nstr[cuser.language][272]);
    else if(ent->recommend>9)
	  sprintf(recom,"3m%2d",ent->recommend);
    else if(ent->recommend>0)
	  sprintf(recom,"2m%2d",ent->recommend);
    else strcpy(recom,"0m  "); 

    prints(
#ifdef COLORDATE
	   "%6d %c\033[1;3%4.4s\033[%dm%-6s\033[m\033[%dm%-13.12s",
#else
	   "%6d %c\033[1;3%4.4s\033[m%-6s\033[%dm%-13.12s",
#endif
	   num, type, recom,
#ifdef COLORDATE
	   (ent->date[3] + ent->date[4]) % 7 + 31,
#endif
	   ent->date, isonline, ent->owner);
	   
    if (strncmp(currtitle, title, TTLEN))
	prints("\033[m%s \033[1m%.*s\033[m%s\n",
	       mark, special ? 6 : 0, title, special ? title + 6 : title);
    else
	prints("\033[1;3%cm%s %s\033[m\n",
	       color, mark, title);
}

int
whereami(int ent, fileheader_t * fhdr, char *direct)
{
    boardheader_t  *bh, *p[32], *root;
    int             i, j;

    if (!currutmp->brc_id)
	return 0;

    move(1, 0);
    clrtobot();
    bh = getbcache(currutmp->brc_id);
    root = getbcache(1);
    p[0] = bh;
    for (i = 0; i < 31 && p[i]->parent != root && p[i]->parent; i++)
	p[i + 1] = p[i]->parent;
    j = i;
    prints(SHM->i18nstr[cuser.language][273], p[j]->title + 7, p[j]->BM);
    for (j--; j >= 0; j--)
	prints("%*s %-13.13s %-37.37s %.13s\n", (i - j) * 2, "",
	       p[j]->brdname, p[j]->title,
	       p[j]->BM);

    pressanykey();
    return FULLUPDATE;
}

static int
do_select(int ent, fileheader_t * fhdr, char *direct)
{
    char            bname[20];
    char            bpath[60];
    boardheader_t  *bh;
    struct stat     st;
    int             i;

    setutmpmode(SELECT);
    move(0, 0);
    clrtoeol();
    generalnamecomplete(MSG_SELECT_BOARD, bname, sizeof(bname),
			SHM->Bnumber,
			completeboard_compar,
			completeboard_permission,
			completeboard_getname);
    if (bname[0] == '\0' || !(i = getbnum(bname)))
	return FULLUPDATE;
    bh = getbcache(i);
    if (!HasPerm(bh))
	return FULLUPDATE;
    strlcpy(bname, bh->brdname, sizeof(bname));
    brc_update();
    currbid = i;

    setbpath(bpath, bname);
    if ((*bname == '\0') || (stat(bpath, &st) == -1)) {
	move(2, 0);
	clrtoeol();
	outs(err_bid);
	return FULLUPDATE;
    }
    setutmpbid(currbid);

    brc_initial_board(bname);
    set_board();
    setbdir(currdirect, currboard);

    move(1, 0);
    clrtoeol();
    return NEWDIRECT;
}

/* ----------------------------------------------------- */
/* ��} innbbsd ��X�H��B�s�u��H���B�z�{��             */
/* ----------------------------------------------------- */
void
outgo_post(fileheader_t * fh, char *board)
{
    FILE           *foo;

    if ((foo = fopen("innd/out.bntp", "a"))) {
	fprintf(foo, "%s\t%s\t%s\t%s\t%s\n", board,
		fh->filename, cuser.userid, cuser.username, fh->title);
	fclose(foo);
    }
}

static void
cancelpost(fileheader_t * fh, int by_BM, char *newpath)
{
    FILE           *fin, *fout;
    char           *ptr, *brd;
    fileheader_t    postfile;
    char            genbuf[200];
    char            nick[STRLEN], fn1[STRLEN];


    if(!fh->filename[0]) return;
    setbfile(fn1, currboard, fh->filename);
    if ((fin = fopen(fn1, "r"))) {
	brd = by_BM ? "deleted" : "junk";

	setbpath(newpath, brd);
	stampfile(newpath, &postfile);
	memcpy(postfile.owner, fh->owner, IDLEN + TTLEN + 10);

	nick[0] = '\0';
	while (fgets(genbuf, sizeof(genbuf), fin)) {
	    if (!strncmp(genbuf, str_author1, LEN_AUTHOR1) ||
		!strncmp(genbuf, str_author2, LEN_AUTHOR2)) {
		if ((ptr = strrchr(genbuf, ')')))
		    *ptr = '\0';
		if ((ptr = (char *)strchr(genbuf, '(')))
		    strlcpy(nick, ptr + 1, sizeof(nick));
		break;
	    }
	}

	if ((fout = fopen("innd/cancel.bntp", "a"))) {
	    fprintf(fout, "%s\t%s\t%s\t%s\t%s\n", currboard, fh->filename,
		    cuser.userid, nick, fh->title);
	    fclose(fout);
	}
	fclose(fin);
	Rename(fn1, newpath);
	setbdir(genbuf, brd);
	setbtotal(getbnum(brd));
	append_record(genbuf, &postfile, sizeof(postfile));
    }
}

/* ----------------------------------------------------- */
/* �o��B�^���B�s��B����峹                            */
/* ----------------------------------------------------- */
void
do_reply_title(int row, char *title)
{
    char            genbuf[200];
    char            genbuf2[4];

    if (strncasecmp(title, str_reply, 4))
	snprintf(save_title, sizeof(save_title), "Re: %s", title);
    else
	strlcpy(save_title, title, sizeof(save_title));
    save_title[TTLEN - 1] = '\0';
    snprintf(genbuf, sizeof(genbuf), SHM->i18nstr[cuser.language][274], save_title);
    getdata(row, 0, genbuf, genbuf2, 4, LCECHO);
    if (genbuf2[0] == 'n' || genbuf2[0] == 'N')
	getdata(++row, 0, SHM->i18nstr[cuser.language][275], save_title, TTLEN, DOECHO);
}

static void
do_unanonymous_post(char *fpath)
{
    fileheader_t    mhdr;
    char            title[128];
    char            genbuf[200];

    setbpath(genbuf, "UnAnonymous");
    if (dashd(genbuf)) {
	stampfile(genbuf, &mhdr);
	unlink(genbuf);
	Link(fpath, genbuf);
	strlcpy(mhdr.owner, cuser.userid, sizeof(mhdr.owner));
	strlcpy(mhdr.title, save_title, sizeof(mhdr.title));
	mhdr.filemode = 0;
	setbdir(title, "UnAnonymous");
	append_record(title, &mhdr, sizeof(mhdr));
    }
}

#ifdef NO_WATER_POST
#ifndef DEBUG
static time_t   last_post_time = 0;
#endif
static time_t   water_counts = 0;
#endif

void 
do_crosspost(char *brd, fileheader_t *postfile, const char *fpath)
{
    char            genbuf[200];
    int             len = 42-strlen(currboard);
    fileheader_t    fh;
    if(!strncasecmp(postfile->title, str_reply, 3))
        len=len+4;
    setbpath(genbuf, brd);
    stampfile(genbuf, &fh);
    strcpy(fh.owner, postfile->owner);
    strcpy(fh.date, postfile->date);
    sprintf(fh.title,SHM->i18nstr[cuser.language][276],  len, len, postfile->title, currboard);
    unlink(genbuf);
    Link((char *)fpath, genbuf);
    postfile->filemode = FILE_LOCAL;
    setbdir(genbuf, brd);
    if (append_record(genbuf, &fh, sizeof(fileheader_t)) != -1) {
	setbtotal(getbnum(brd));
    }
}
static void 
setupbidinfo(bid_t *bidinfo)
{
        char buf[256];
        bidinfo->enddate = gettime(20, now+86400,SHM->i18nstr[cuser.language][277]);
        do
         getdata_str(21,0,SHM->i18nstr[cuser.language][278],buf, 8, LCECHO, "1");
        while((bidinfo->high=atoi(buf))<=0);
        do
           getdata_str(21,20, SHM->i18nstr[cuser.language][279],buf, 5, LCECHO, "1");
        while((bidinfo->increment=atoi(buf))<=0);
        getdata(21,44, SHM->i18nstr[cuser.language][280],buf, 10, LCECHO);
        bidinfo->buyitnow=atoi(buf);
	
        getdata_str(22,0,
          SHM->i18nstr[cuser.language][281],
          buf, 3, LCECHO,"1");
          bidinfo->payby=(buf[0]-'1');
        if(bidinfo->payby<0 ||bidinfo->payby>3)bidinfo->payby=0;
        getdata_str(23,0, SHM->i18nstr[cuser.language][282], buf, 6, LCECHO, "0"); 
        bidinfo->shipping = atoi(buf);
        if(bidinfo->shipping<0)  bidinfo->shipping=0;
}
static void
print_bidinfo(FILE *io, bid_t bidinfo)
{
    char *payby[4]={ SHM->i18nstr[cuser.language][283],SHM->i18nstr[cuser.language][284],SHM->i18nstr[cuser.language][285],SHM->i18nstr[cuser.language][286]};
    if(io)
    {
     if(!bidinfo.userid[0])
      fprintf(io,SHM->i18nstr[cuser.language][287],bidinfo.high);
     else 
      fprintf(io, SHM->i18nstr[cuser.language][288],bidinfo.high, bidinfo.userid);
     fprintf(io, SHM->i18nstr[cuser.language][289],payby[bidinfo.payby%4],Cdate(& bidinfo.enddate));
     if(bidinfo.buyitnow)
      fprintf(io, SHM->i18nstr[cuser.language][290],bidinfo.buyitnow);
     if(bidinfo.shipping)
      fprintf(io, SHM->i18nstr[cuser.language][291], bidinfo.shipping);
     fprintf(io, "\n");
    }
    else
    {
     if(!bidinfo.userid[0])
      prints(SHM->i18nstr[cuser.language][292],bidinfo.high);
     else 
      prints(SHM->i18nstr[cuser.language][293],bidinfo.high, bidinfo.userid);
     prints(SHM->i18nstr[cuser.language][294],payby[bidinfo.payby%4],Cdate(& bidinfo.enddate));
     if(bidinfo.buyitnow)
      prints(SHM->i18nstr[cuser.language][295],bidinfo.buyitnow);
     if(bidinfo.shipping)
      prints(SHM->i18nstr[cuser.language][296], bidinfo.shipping);
     prints("\n");
    }

}
static int
do_general(int isbid)
{
    bid_t bidinfo;
    fileheader_t    postfile;
    char            fpath[80], buf[80];
    int             aborted, defanony, ifuseanony, i;
    char            genbuf[200], *owner, ctype[8][5];
    boardheader_t  *bp;
    int             islocal, posttype=-1;

    ifuseanony = 0;
    bp = getbcache(currbid);

    clear();
    if (!CheckPostPerm()
#ifdef FOREIGN_REG
	    // ���O�~�y�ϥΪ̦b PttForeign �O
	    && !((cuser.uflag2 & FOREIGN) && strcmp(bp->brdname, "PttForeign") == 0)
#endif
	) {
	vmsg(SHM->i18nstr[cuser.language][305]);
	return READ_REDRAW;
    }
#ifdef NO_WATER_POST
#ifndef DEBUG /* why we need this in DEBUG mode? */
    /* �T�������̦h�o���g�峹 */
    if (currutmp->lastact - last_post_time < 60 * 3) {
	if (water_counts >= 5) {
	    vmsg(SHM->i18nstr[cuser.language][306]);
	    return READ_REDRAW;
	}
    } else {
	last_post_time = currutmp->lastact;
	water_counts = 0;
    }
#endif
#endif

    if(!isbid)
       setbfile(genbuf, currboard, FN_POST_NOTE);
    else
       setbfile(genbuf, currboard, FN_POST_BID);

    if (more(genbuf, NA) == -1)
      {
       if(!isbid)
   	   more("etc/" FN_POST_NOTE, NA);
       else
   	   more("etc/" FN_POST_BID, NA);
      }
    move(19, 0);
    prints(SHM->i18nstr[cuser.language][307],
           isbid?SHM->i18nstr[cuser.language][308]:SHM->i18nstr[cuser.language][309],
	  currboard, bp->title + 7);

    if(isbid)
       {
	memset(&bidinfo,0,sizeof(bidinfo)); 
        setupbidinfo(&bidinfo);
        postfile.money=bidinfo.high;
        move(20,0);
        clrtobot();
       }
    if (quote_file[0])
	do_reply_title(20, currtitle);
    else {
	if(!isbid)
        {
         move(21,0);
         prints(SHM->i18nstr[cuser.language][310]);
         for(i=0; i<8 ; i++)
         	strlcpy(ctype[i], SHM->i18nstr[cuser.language][297 + i], 4);
         for(i=0; i<8 && bp->posttype[i*4]; i++)
            strncpy(ctype[i],bp->posttype+4*i,4);
         if(i==0) i=8;
         for(aborted=0; aborted<i; aborted++)
            prints("%d.%4.4s ", aborted+1, ctype[aborted]);
         sprintf(buf,SHM->i18nstr[cuser.language][311],i);
         getdata(21, 6+7*i, buf, save_title, 3, LCECHO); 
	 posttype = save_title[0] - '1';
	 if (posttype >= 0 && posttype < i)
	    snprintf(save_title, sizeof(save_title),
		     "[%s] ", ctype[posttype]);
	 else
           {
	    save_title[0] = '\0';
            posttype=-1;
           }
	}
	getdata_buf(22, 0, SHM->i18nstr[cuser.language][312], save_title, TTLEN, DOECHO);
	strip_ansi(save_title, save_title, STRIP_ALL);
    }
    if (save_title[0] == '\0')
	return FULLUPDATE;

    curredit &= ~EDIT_MAIL;
    curredit &= ~EDIT_ITEM;
    setutmpmode(POSTING);
    /* ����� Internet �v���̡A�u��b�����o��峹 */
    if (HAS_PERM(PERM_INTERNET))
	local_article = 0;
    else
	local_article = 1;

    /* build filename */
    setbpath(fpath, currboard);
    stampfile(fpath, &postfile);
    if(isbid)
      {
       aborted = (int)fopen(fpath, "w");
       if(aborted)
         {
          print_bidinfo((FILE*)aborted, bidinfo);
          fclose((FILE*)aborted);
         }
      }
    else if(posttype!=-1 && ((1<<posttype) & bp->posttype_f))
     {
          setbnfile(genbuf, bp->brdname, "postsample", posttype);
          Copy(genbuf, fpath);
     }
    
    aborted = vedit(fpath, YEA, &islocal);
    if (aborted == -1) {
	unlink(fpath);
	pressanykey();
	return FULLUPDATE;
    }
    water_counts++;		/* po���\ */

    /* set owner to Anonymous , for Anonymous board */

#ifdef HAVE_ANONYMOUS
    /* Ptt and Jaky */
    defanony = currbrdattr & BRD_DEFAULTANONYMOUS;
    if ((currbrdattr & BRD_ANONYMOUS) &&
	((strcmp(real_name, "r") && defanony) || (real_name[0] && !defanony))
	) {
	strcat(real_name, ".");
	owner = real_name;
	ifuseanony = 1;
    } else
	owner = cuser.userid;
#else
    owner = cuser.userid;
#endif
    /* �� */
    aborted = (aborted > MAX_POST_MONEY * 2) ? MAX_POST_MONEY : aborted / 2;
    if(ifuseanony)
       {
        postfile.filemode |= FILE_ANONYMOUS;
        postfile.money = currutmp->uid;
       }
    else if(!isbid)
       postfile.money = aborted;
    
    strlcpy(postfile.owner, owner, sizeof(postfile.owner));
    strlcpy(postfile.title, save_title, sizeof(postfile.title));
    if (islocal)		/* local save */
	postfile.filemode |= FILE_LOCAL;

    setbdir(buf, currboard);
    if(isbid)
       {
	 sprintf(genbuf, "%s.bid", fpath);
         append_record(genbuf,(void*) &bidinfo, sizeof(bidinfo));
         postfile.filemode |= FILE_BID ;
       }
    if (append_record(buf, &postfile, sizeof(postfile)) != -1) {
	setbtotal(currbid);

	if (currmode & MODE_SELECT)
	    append_record(currdirect, &postfile, sizeof(postfile));
	if (!islocal && !(bp->brdattr & BRD_NOTRAN))
	    outgo_post(&postfile, currboard);
	brc_addlist(postfile.filename);

	if (!(currbrdattr & BRD_HIDE) &&
	    (!bp->level || (currbrdattr & BRD_POSTMASK))) {
            do_crosspost(ALLPOST, &postfile, fpath);
	}
	outs(SHM->i18nstr[cuser.language][313]);

#ifdef MAX_POST_MONEY
	aborted = (aborted > MAX_POST_MONEY) ? MAX_POST_MONEY : aborted;
#endif
	if (strcmp(currboard, "Test") && !ifuseanony) {
	    prints(SHM->i18nstr[cuser.language][314],++cuser.numposts);
            if(postfile.filemode&FILE_BID)
                prints(SHM->i18nstr[cuser.language][315]);
            else if(currbrdattr&BRD_BAD)
                prints(SHM->i18nstr[cuser.language][316]);
            else
              {
                prints(SHM->i18nstr[cuser.language][317],aborted);
                demoney(aborted);    
              }
	} else
	    outs(SHM->i18nstr[cuser.language][318]);

	/* �^�����@�̫H�c */

	if (curredit & EDIT_BOTH) {
	    char           *str, *msg = SHM->i18nstr[cuser.language][319];

	    if ((str = strchr(quote_user, '.'))) {
		if (
#ifndef USE_BSMTP
		    bbs_sendmail(fpath, save_title, str + 1)
#else
		    bsmtp(fpath, save_title, str + 1, 0)
#endif
		    < 0)
		    msg = SHM->i18nstr[cuser.language][320];
	    } else {
		sethomepath(genbuf, quote_user);
		stampfile(genbuf, &postfile);
		unlink(genbuf);
		Link(fpath, genbuf);

		strlcpy(postfile.owner, cuser.userid, sizeof(postfile.owner));
		strlcpy(postfile.title, save_title, sizeof(postfile.title));
		sethomedir(genbuf, quote_user);
		if (append_record(genbuf, &postfile, sizeof(postfile)) == -1)
		    msg = err_uid;
	    }
	    outs(msg);
	    curredit ^= EDIT_BOTH;
	}
	if (currbrdattr & BRD_ANONYMOUS)
	    do_unanonymous_post(fpath);
    }
    pressanykey();
    return FULLUPDATE;
}

int
do_post()
{
    boardheader_t  *bp;
    bp = getbcache(currbid);
    if (bp->brdattr & BRD_VOTEBOARD)
	return do_voteboard(0);
    else if (!(bp->brdattr & BRD_GROUPBOARD))
	return do_general(0);
    return 0;
}

int
do_post_vote()
{
    return do_voteboard(1);
}

int
do_post_openbid()
{
    boardheader_t  *bp;
    bp = getbcache(currbid);
    if (!(bp->brdattr & BRD_VOTEBOARD))
	return do_general(1);
    return 0;
}

static void
do_generalboardreply(fileheader_t * fhdr)
{
    char            genbuf[3];
    getdata(b_lines - 1, 0,
	    SHM->i18nstr[cuser.language][321],
	    genbuf, sizeof(genbuf), LCECHO);
    switch (genbuf[0]) {
    case 'm':
	mail_reply(0, fhdr, 0);
    case 'q':
	break;

    case 'b':
	curredit = EDIT_BOTH;
    default:
	strlcpy(currtitle, fhdr->title, sizeof(currtitle));
	strlcpy(quote_user, fhdr->owner, sizeof(quote_user));
	do_post();
    }
    *quote_file = 0;
}

int
invalid_brdname(char *brd)
{
    register char   ch;

    ch = *brd++;
    if (!isalpha(ch))
	return 1;
    while ((ch = *brd++)) {
	if (not_alnum(ch) && ch != '_' && ch != '-' && ch != '.')
	    return 1;
    }
    return 0;
}

static int
b_call_in(int ent, fileheader_t * fhdr, char *direct)
{
    userinfo_t     *u = search_ulist(searchuser(fhdr->owner));
    if (u) {
	int             fri_stat;
	fri_stat = friend_stat(currutmp, u);
	if (isvisible_stat(currutmp, u, fri_stat) && call_in(u, fri_stat))
	    return FULLUPDATE;
    }
    return DONOTHING;
}


static int
b_posttype(int ent, fileheader_t * fhdr, char *direct)
{
   boardheader_t  *bp;
   int i, aborted;
   char filepath[256], genbuf[60], title[5], posttype_f, posttype[33]="";

   if(!(currmode & MODE_BOARD)) return DONOTHING;
   
   bp = getbcache(currbid);

   move(2,0);
   clrtobot();
   posttype_f =  bp->posttype_f;
   for(i=0; i<8; i++)
     {
       move(2,0);
       outs(SHM->i18nstr[cuser.language][322]);
       strncpy(genbuf, bp->posttype+i*4, 4);
       genbuf[4]=0;
       sprintf(title,"%d.",i+1);
       if(!getdata_buf(2,11, title, genbuf, 5, DOECHO)) break;
       sprintf(posttype+i*4,"%-4.4s", genbuf); 
       if( posttype_f & (1<<i) )
          {
            if(getdata(2, 20, SHM->i18nstr[cuser.language][323], genbuf, 3, LCECHO) &&
                genbuf[0]=='n')
                {
                 posttype_f &= ~(1<<i);
                 continue;
                }
          }
       else if (!getdata(2, 20, SHM->i18nstr[cuser.language][324], genbuf, 3, LCECHO) ||
              genbuf[0]!='y') continue;

       setbnfile(filepath, bp->brdname, "postsample", i);
       aborted = vedit(filepath, NA, NULL);
       if (aborted == -1) {
           clear();
           posttype_f &= ~(1<<i);
           continue;
        } 
       posttype_f |= (1<<i);
     } 
   bp->posttype_f = posttype_f; 
   strncpy(bp->posttype, posttype, 32); /* �o�����ӭn��race condition */

   substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
   return FULLUPDATE;
}

static void
do_reply(fileheader_t * fhdr)
{
    boardheader_t  *bp;
    bp = getbcache(currbid);
    if (bp->brdattr & BRD_VOTEBOARD || (fhdr->filemode & FILE_VOTE))
	do_voteboardreply(fhdr);
    else
	do_generalboardreply(fhdr);
}

static int
reply_post(int ent, fileheader_t * fhdr, char *direct)
{
    if (!CheckPostPerm())
	return DONOTHING;

    setdirpath(quote_file, direct, fhdr->filename);
    do_reply(fhdr);
    *quote_file = 0;
    return FULLUPDATE;
}

static int
edit_post(int ent, fileheader_t * fhdr, char *direct)
{
    char            fpath[80];
    char            genbuf[200];
    fileheader_t    postfile;
    boardheader_t  *bp = getbcache(currbid);

    if (fhdr->filemode&FILE_BOTTOM && strcmp(bp->brdname, "Security") == 0)
	return DONOTHING;

    if (!HAS_PERM(PERM_SYSOP) && ((bp->brdattr & BRD_VOTEBOARD) || fhdr->filemode & FILE_VOTE))
	return DONOTHING;

    if( !HAS_PERM(PERM_SYSOP) &&
	(!CheckPostPerm() || strcmp(fhdr->owner, cuser.userid) != 0) )
	return DONOTHING;

    if( currmode & MODE_SELECT )
	return DONOTHING;

#ifdef SAFE_ARTICLE_DELETE
    if( fhdr->filename[0] == '.' )
	return DONOTHING;
#endif

    setutmpmode(REEDIT);
    setbpath(fpath, currboard);
    stampfile(fpath, &postfile);
    setdirpath(genbuf, direct, fhdr->filename);
    local_article = fhdr->filemode & FILE_LOCAL;
    Copy(genbuf, fpath);
    strlcpy(save_title, fhdr->title, sizeof(save_title));

	if (vedit(fpath, 0, NULL) != -1) {
		Rename(fpath, genbuf);
            
		if (!(currbrdattr & BRD_HIDE) && (!bp->level || (currbrdattr & BRD_POSTMASK)))
		    do_crosspost(ALLPOST, fhdr, fpath);
    }
    return FULLUPDATE;
}

#define UPDATE_USEREC   (currmode |= MODE_DIRTY)

static int
cross_post(int ent, fileheader_t * fhdr, char *direct)
{
    char            xboard[20], fname[80], xfpath[80], xtitle[80], inputbuf[10];
    fileheader_t    xfile;
    FILE           *xptr;
    int             author = 0;
    char            genbuf[200];
    char            genbuf2[4];
    boardheader_t  *bp;
    if (!CheckPostPerm()) {
	move(5, 10);
	outs(SHM->i18nstr[cuser.language][325]);
	pressanykey();
	return FULLUPDATE;
    }
    move(2, 0);
    clrtoeol();
    move(1, 0);
    bp = getbcache(currbid);
    if (bp && (bp->brdattr & BRD_VOTEBOARD) )
	return FULLUPDATE;
    generalnamecomplete(SHM->i18nstr[cuser.language][326], xboard, sizeof(xboard),
			SHM->Bnumber,
			completeboard_compar,
			completeboard_permission,
			completeboard_getname);
    if (*xboard == '\0' || !haspostperm(xboard))
	return FULLUPDATE;

    if ((ent = str_checksum(fhdr->title)) != 0 &&
	ent == postrecord.checksum[0]) {
	/* �ˬd cross post ���� */
	if (postrecord.times++ > MAX_CROSSNUM)
	    anticrosspost();
    } else {
	postrecord.times = 0;
	postrecord.checksum[0] = ent;
    }

    ent = 1;
    if (HAS_PERM(PERM_SYSOP) || !strcmp(fhdr->owner, cuser.userid)) {
	getdata(2, 0, SHM->i18nstr[cuser.language][327],
		genbuf, 3, DOECHO);
	if (genbuf[0] != '2') {
	    ent = 0;
	    getdata(2, 0, SHM->i18nstr[cuser.language][328], inputbuf, 3, DOECHO);
	    if (inputbuf[0] != 'n' && inputbuf[0] != 'N')
		author = 1;
	}
    }
    if (ent)
	snprintf(xtitle, sizeof(xtitle), SHM->i18nstr[cuser.language][329], fhdr->title);
    else
	strlcpy(xtitle, fhdr->title, sizeof(xtitle));

    snprintf(genbuf, sizeof(genbuf), SHM->i18nstr[cuser.language][330], xtitle);
    getdata(2, 0, genbuf, genbuf2, 4, LCECHO);
    if (genbuf2[0] == 'n' || genbuf2[0] == 'N') {
	if (getdata_str(2, 0, SHM->i18nstr[cuser.language][331], genbuf, TTLEN, DOECHO, xtitle))
	    strlcpy(xtitle, genbuf, sizeof(xtitle));
    }
    getdata(2, 0, SHM->i18nstr[cuser.language][332], genbuf, 3, LCECHO);
    if (genbuf[0] == 'l' || genbuf[0] == 's') {
	int             currmode0 = currmode;
	char           *save_currboard;

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
	setbfile(fname, currboard, fhdr->filename);
	xptr = fopen(xfpath, "w");

	strlcpy(save_title, xfile.title, sizeof(save_title));
	save_currboard = currboard;
	currboard = xboard;
	write_header(xptr);
	currboard = save_currboard;

	fprintf(xptr, SHM->i18nstr[cuser.language][333], currboard);

	b_suckinfile(xptr, fname);
	addsignature(xptr, 0);
	fclose(xptr);
	/*
	 * Cross fs�����D } else { unlink(xfpath); link(fname, xfpath); }
	 */
	setbdir(fname, xboard);
	append_record(fname, &xfile, sizeof(xfile));
	bp = getbcache(getbnum(xboard));
	if (!xfile.filemode && !(bp->brdattr & BRD_NOTRAN))
	    outgo_post(&xfile, xboard);
	setbtotal(getbnum(xboard));
	cuser.numposts++;
	UPDATE_USEREC;
	outs(SHM->i18nstr[cuser.language][334]);
	pressanykey();
	currmode = currmode0;
    }
    return FULLUPDATE;
}
static int
read_post(int ent, fileheader_t * fhdr, char *direct)
{
    char            genbuf[200];
    int             more_result;

    if (fhdr->owner[0] == '-')
	return READ_SKIP;

    setdirpath(genbuf, direct, fhdr->filename);

    if ((more_result = more(genbuf, YEA)) == -1)
        return READ_SKIP;

    brc_addlist(fhdr->filename);
    strncpy(currtitle, subject(fhdr->title), TTLEN);

	if (more_result) {
		if(more_result == 999) {
			if (CheckPostPerm()) {
				strlcpy(quote_file, genbuf, sizeof(quote_file));
				do_reply(fhdr);
				*quote_file = 0;
			}
			return FULLUPDATE;
		}
		if(more_result == 998) {
			recommend(ent, fhdr, direct);
			return FULLUPDATE;
		}
		else 
			return more_result;
	}
	return FULLUPDATE;                                                                                                                                                                        
}

/* ----------------------------------------------------- */
/* �Ķ���ذ�                                            */
/* ----------------------------------------------------- */
static int
b_man()
{
    char            buf[64];

    setapath(buf, currboard);
    if ((currmode & MODE_BOARD) || HAS_PERM(PERM_SYSOP)) {
	char            genbuf[128];
	int             fd;
	snprintf(genbuf, sizeof(genbuf), "%s/.rebuild", buf);
	if ((fd = open(genbuf, O_CREAT, 0640)) > 0)
	    close(fd);
    }
    return a_menu(currboard, buf, HAS_PERM(PERM_ALLBOARD) ? 2 :
		  (currmode & MODE_BOARD ? 1 : 0));
}

#ifndef NO_GAMBLE
static int
stop_gamble()
{
    boardheader_t  *bp = getbcache(currbid);
    char            fn_ticket[128], fn_ticket_end[128];
    if (!bp->endgamble || bp->endgamble > now)
	return 0;

    setbfile(fn_ticket, currboard, FN_TICKET);
    setbfile(fn_ticket_end, currboard, FN_TICKET_END);

    rename(fn_ticket, fn_ticket_end);
    if (bp->endgamble) {
	bp->endgamble = 0;
	substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    }
    return 1;
}
static int
join_gamble(int ent, fileheader_t * fhdr, char *direct)
{
    if (!HAS_PERM(PERM_LOGINOK))
	return DONOTHING;
    if (stop_gamble()) {
	vmsg(SHM->i18nstr[cuser.language][336]);
	return DONOTHING;
    }
    ticket(currbid);
    return FULLUPDATE;
}
static int
hold_gamble(int ent, fileheader_t * fhdr, char *direct)
{
    char            fn_ticket[128], fn_ticket_end[128], genbuf[128], msg[256] = "",
                    yn[10] = "";
    boardheader_t  *bp = getbcache(currbid);
    int             i;
    FILE           *fp = NULL;

    if (!(currmode & MODE_BOARD))
	return 0;
    setbfile(fn_ticket, currboard, FN_TICKET);
    setbfile(fn_ticket_end, currboard, FN_TICKET_END);
    setbfile(genbuf, currboard, FN_TICKET_LOCK);
    if (dashf(fn_ticket)) {
	getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][337], yn, 3, LCECHO);
	if (yn[0] != 'y')
	    return FULLUPDATE;
	rename(fn_ticket, fn_ticket_end);
	if (bp->endgamble) {
	    bp->endgamble = 0;
	    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);

	}
	return FULLUPDATE;
    }
    if (dashf(fn_ticket_end)) {
	getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][338], yn, 3, LCECHO);
	if (yn[0] != 'y')
	    return FULLUPDATE;
	openticket(currbid);
	return FULLUPDATE;
    } else if (dashf(genbuf)) {
	vmsg(SHM->i18nstr[cuser.language][339]);
	return FULLUPDATE;
    }
    getdata(b_lines - 2, 0, SHM->i18nstr[cuser.language][340], yn, 3, LCECHO);
    if (yn[0] != 'y')
	return FULLUPDATE;
    getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][341],
	    msg, 20, DOECHO);
    if (msg[0] == 0 ||
	vedit(fn_ticket_end, NA, NULL) < 0)
	return FULLUPDATE;

    clear();
    showtitle(SHM->i18nstr[cuser.language][342], BBSNAME);
    setbfile(genbuf, currboard, FN_TICKET_ITEMS);

    //sprintf(genbuf, "%s/" FN_TICKET_ITEMS, direct);

    if (!(fp = fopen(genbuf, "w")))
	return FULLUPDATE;
    do {
	getdata(2, 0, SHM->i18nstr[cuser.language][343], yn, 6, LCECHO);
	i = atoi(yn);
    } while (i < 10 || i > 10000);
    fprintf(fp, "%d\n", i);
    if (!getdata(3, 0, SHM->i18nstr[cuser.language][344], yn, 3, LCECHO) || yn[0] != 'n') {
	bp->endgamble = gettime(4, now, SHM->i18nstr[cuser.language][345]);
	substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    }
    move(6, 0);
    snprintf(genbuf, sizeof(genbuf),
	     SHM->i18nstr[cuser.language][346],
	     currboard,
	     i, i < 100 ? SHM->i18nstr[cuser.language][347] : i < 500 ? SHM->i18nstr[cuser.language][348] :
	     i < 1000 ? SHM->i18nstr[cuser.language][349] : i < 5000 ? SHM->i18nstr[cuser.language][350] : SHM->i18nstr[cuser.language][351],
	     bp->endgamble ? SHM->i18nstr[cuser.language][352] : "",
	     bp->endgamble ? Cdate(&bp->endgamble) : ""
	     );
    strcat(msg, genbuf);
    prints(SHM->i18nstr[cuser.language][353]);
    for (i = 0; i < 8; i++) {
	snprintf(yn, sizeof(yn), " %d)", i + 1);
	getdata(7 + i, 0, yn, genbuf, 9, DOECHO);
	if (!genbuf[0] && i > 1)
	    break;
	fprintf(fp, "%s\n", genbuf);
    }
    fclose(fp);

    setbfile(genbuf, currboard, FN_TICKET_RECORD);
    unlink(genbuf); // Ptt: �����Q�Τ��Pid�P���|����
    setbfile(genbuf, currboard, FN_TICKET_USER);
    unlink(genbuf); // Ptt: �����Q�Τ��Pid�P���|����

    move(8 + i, 0);
    prints(SHM->i18nstr[cuser.language][354]);
    snprintf(genbuf, sizeof(genbuf), SHM->i18nstr[cuser.language][355], currboard);
    post_msg(currboard, genbuf, msg, cuser.userid);
    post_msg("Record", genbuf + 7, msg, SHM->i18nstr[cuser.language][356]);
    /* Tim ����CS, �H�K���b����user���Ƥw�g�g�i�� */
    rename(fn_ticket_end, fn_ticket);
    /* �]�w���~���ɦW��L�� */

    return FULLUPDATE;
}
#endif

static int
cite_post(int ent, fileheader_t * fhdr, char *direct)
{
    char            fpath[256];
    char            title[TTLEN + 1];

    setbfile(fpath, currboard, fhdr->filename);
    strlcpy(title, SHM->i18nstr[cuser.language][357], sizeof(title));
    strlcpy(title + 3, fhdr->title, TTLEN - 3);
    title[TTLEN] = '\0';
    a_copyitem(fpath, title, 0, 1);
    b_man();
    return FULLUPDATE;
}

int
edit_title(int ent, fileheader_t * fhdr, char *direct)
{
    char            genbuf[200];
    fileheader_t    tmpfhdr = *fhdr;
    int             dirty = 0;

    if (currmode & MODE_BOARD || !strcmp(cuser.userid, fhdr->owner)) {
	if (getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][358], genbuf, TTLEN, DOECHO)) {
	    strlcpy(tmpfhdr.title, genbuf, sizeof(tmpfhdr.title));
	    dirty++;
	}
    }
    if (HAS_PERM(PERM_SYSOP)) {
	if (getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][359], genbuf, IDLEN + 2, DOECHO)) {
	    strlcpy(tmpfhdr.owner, genbuf, sizeof(tmpfhdr.owner));
	    dirty++;
	}
	if (getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][360], genbuf, 6, DOECHO)) {
	    snprintf(tmpfhdr.date, sizeof(tmpfhdr.date), "%.5s", genbuf);
	    dirty++;
	}
    }
    if (currmode & MODE_BOARD || !strcmp(cuser.userid, fhdr->owner)) {
	getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][361], genbuf, 3, DOECHO);
	if ((genbuf[0] == 'y' || genbuf[0] == 'Y') && dirty) {
	    *fhdr = tmpfhdr;
	     substitute_ref_record(direct, fhdr, ent);
	}
	return FULLUPDATE;
    }
    return DONOTHING;
}

static int
solve_post(int ent, fileheader_t * fhdr, char *direct)
{
    if (HAS_PERM(PERM_SYSOP)) {
		fhdr->filemode ^= FILE_SOLVED;
		substitute_ref_record(direct, fhdr, ent);
		return PART_REDRAW;
    }
    return DONOTHING;
}

static int
recommend_cancel(int ent, fileheader_t * fhdr, char *direct)
{
    char            yn[5];
    if (!(currmode & MODE_BOARD))
	return DONOTHING;
    getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][362], yn, 5, LCECHO);
    if (yn[0] != 'y')
	return FULLUPDATE;
#ifdef ASSESS
    // to save resource
    if (fhdr->recommend > 9)
	inc_goodpost(searchuser(fhdr->owner), -1 * (fhdr->recommend / 10));
#endif
    fhdr->recommend = 0;

	substitute_ref_record(direct, fhdr, ent);
    return FULLUPDATE;
}
static int
do_add_recommend(char *direct, fileheader_t *fhdr, int ent, char *buf)
{
    char    path[256];
    int     fd;
    /*
      race here:
      ���F��� system calls , �{�b�����η�e������� +1 �g�J .DIR ��.
      �y��
      1.�Y�Ӥ��ɦW�Q��������, ����N�g�����ɦW�� (�y�����F��)
      2.�S�����sŪ�@��, �ҥH����ƥi��Q�ֺ�
      3.�Y�����ɭԫe��Q�R, �N�[���媺�����
     */
    setdirpath(path, direct, fhdr->filename);
    if( log_file(path, 0, buf) == -1 ){ // �� CREATE
	vmsg(SHM->i18nstr[cuser.language][363]);
	return -1;
    }

    /* get_record(direct, fhdr, sizeof(fhdr), ent);
     * This is a solution to avoid most racing (still some), but cost four
     * system calls.                                                        */

    if( fhdr->recommend < 100 ){
	fileheader_t t;
	if( (fd = open(direct, O_WRONLY)) < 0 )
	    return -1;

	++(fhdr->recommend);
	if( lseek(fd, (off_t)(sizeof(*fhdr) * (ent - 1) +
			(int)&t.recommend - (int)&t), SEEK_SET) >= 0)
	    // �p�G lseek ���ѴN���| write
	    write(fd, &fhdr->recommend, sizeof(char));

	close(fd);
    }
    return 0;
}
static int
do_bid(int ent, fileheader_t * fhdr, boardheader_t  *bp, char *direct,  struct tm      *ptime)
{
    char            genbuf[200], fpath[256],say[30],*money;
    bid_t           bidinfo;
    int mymax, next;

    setdirpath(fpath, direct, fhdr->filename);
    strcat(fpath,".bid");
    get_record(fpath, &bidinfo, sizeof(bidinfo), 1);

    move(18,0); clrtobot();
    prints(SHM->i18nstr[cuser.language][364], fhdr->title);
    print_bidinfo(0, bidinfo);
    if(!bidinfo.payby) money="Ptt$ "; else money=" NT$ ";
    if(now>bidinfo.enddate || bidinfo.high==bidinfo.buyitnow)
    {
	prints(SHM->i18nstr[cuser.language][365]);
	if( bidinfo.userid[0]) {
	    /*if(!payby && bidinfo.usermax!=-1)
	      {�HPtt���۰ʦ���
	      }*/
	    prints(SHM->i18nstr[cuser.language][366], bidinfo.userid, 
		    bidinfo.high);
#ifdef ASSESS
	    if (!(bidinfo.flag & SALE_COMMENTED) && strcmp(bidinfo.userid, currutmp->userid) == 0){
		char tmp = getans(SHM->i18nstr[cuser.language][367]);
		if ('1' <= tmp && tmp <= '3'){
		    switch(tmp){
			case 1:
			    inc_goodsale(currutmp->uid, 1);
			    break;
			case 2:
			    inc_badpost(currutmp->uid, 1);
			    break;
		    }
		    bidinfo.flag |= SALE_COMMENTED;
		    substitute_record(fpath, &bidinfo, sizeof(bidinfo), 1);
		}
	    }
#endif
	}
	else prints(SHM->i18nstr[cuser.language][368]);
	pressanykey();
	return FULLUPDATE;
    }
    if(bidinfo.userid[0])
    {
        prints(SHM->i18nstr[cuser.language][369], money,bidinfo.high + bidinfo.increment);
	if(bidinfo.buyitnow)
	     prints(SHM->i18nstr[cuser.language][370],bidinfo.buyitnow);
	next=bidinfo.high + bidinfo.increment;
    }
    else
    {
        prints(SHM->i18nstr[cuser.language][371], bidinfo.high);
	next=bidinfo.high;
    }
    if(!strcmp(cuser.userid,bidinfo.userid))
    {
	prints(SHM->i18nstr[cuser.language][372]);
        pressanykey();
	return FULLUPDATE;
    }
    if (strcmp(cuser.userid, fhdr->owner) == 0){
	vmsg(SHM->i18nstr[cuser.language][373]);
	return FULLUPDATE;
    }
    getdata_str(23,0,SHM->i18nstr[cuser.language][374], genbuf, 3, LCECHO,"n");
    if(genbuf[0]!='y') return FULLUPDATE;

    getdata(23, 0, SHM->i18nstr[cuser.language][375], genbuf, 10, LCECHO);

    mymax=atoi(genbuf);

    getdata(23,0,SHM->i18nstr[cuser.language][376],say,12,DOECHO);

    get_record(fpath, &bidinfo, sizeof(bidinfo), 1);

    if(bidinfo.buyitnow && mymax>bidinfo.buyitnow)
        mymax=bidinfo.buyitnow;
    else if(!bidinfo.userid[0])
	next=bidinfo.high;
    else
	next=bidinfo.high + bidinfo.increment;
    

    if(mymax< next || (bidinfo.payby==0 && cuser.money<mymax ))
    {
	vmsg(SHM->i18nstr[cuser.language][377]);
        return FULLUPDATE;
    }
    
    snprintf(genbuf, sizeof(genbuf),
 SHM->i18nstr[cuser.language][378],
	     cuser.userid,say,
	     31 - strlen(cuser.userid) - strlen(say), " ", 
             money,
	     next, fromhost,
	     ptime->tm_mon + 1, ptime->tm_mday);
    do_add_recommend(direct, fhdr,  ent, genbuf);
    if(next > bidinfo.usermax)
    {
       bidinfo.usermax=mymax;
       bidinfo.high=next;
       strcpy(bidinfo.userid,cuser.userid);
    }
    else if(mymax>bidinfo.usermax)
    {
	bidinfo.high=bidinfo.usermax+bidinfo.increment;
        if(bidinfo.high>mymax) bidinfo.high=mymax; 
	bidinfo.usermax=mymax;
        strcpy(bidinfo.userid,cuser.userid);
	
        snprintf(genbuf, sizeof(genbuf),
SHM->i18nstr[cuser.language][379],
	     cuser.userid, 
	     20 - strlen(cuser.userid) , " ",money, 
	     bidinfo.high, 
	     ptime->tm_mon + 1, ptime->tm_mday);
        do_add_recommend(direct, fhdr,  ent, genbuf);
    }
    else
    {
	 if(mymax+bidinfo.increment<bidinfo.usermax)
	  bidinfo.high=mymax+bidinfo.increment;
	 else
	  bidinfo.high=bidinfo.usermax; /*�o��ǩǪ�*/ 
        snprintf(genbuf, sizeof(genbuf),
SHM->i18nstr[cuser.language][380],
	     bidinfo.userid, 
	     20 - strlen(bidinfo.userid) , " ", money, 
	     bidinfo.high,
	     ptime->tm_mon + 1, ptime->tm_mday);
        do_add_recommend(direct, fhdr,  ent, genbuf);
    }
    substitute_record(fpath, &bidinfo, sizeof(bidinfo), 1);
    vmsg(SHM->i18nstr[cuser.language][381]);
    return FULLUPDATE;
}

static int
recommend(int ent, fileheader_t * fhdr, char *direct)
{
    struct tm      *ptime = localtime(&now);
    char            buf[200], path[200], yn[5];
    boardheader_t  *bp;
    static time_t   lastrecommend = 0;

    bp = getbcache(currbid);
    if( bp->brdattr & BRD_NORECOMMEND ){
	vmsg(SHM->i18nstr[cuser.language][382]);
	return FULLUPDATE;
    }
    if (!CheckPostPerm() || bp->brdattr & BRD_VOTEBOARD || fhdr->filemode & FILE_VOTE) {
	vmsg(SHM->i18nstr[cuser.language][383]);
	return FULLUPDATE;
    }
#ifdef SAFE_ARTICLE_DELETE
    if( fhdr->filename[0] == '.' ){
	vmsg(SHM->i18nstr[cuser.language][384]);
	return FULLUPDATE;
    }
#endif

    if( fhdr->filemode & FILE_BID){
	return do_bid(ent, fhdr, bp, direct, ptime);
    }
    setdirpath(path, direct, fhdr->filename);

    if (fhdr->recommend == 0 && strcmp(cuser.userid, fhdr->owner) == 0){
	vmsg(SHM->i18nstr[cuser.language][385]);
	return FULLUPDATE;
    }
#ifndef DEBUG
    if (!(currmode & MODE_BOARD) && getuser(cuser.userid) &&
	now - lastrecommend < 40) {
	vmsg(SHM->i18nstr[cuser.language][386]);
	return FULLUPDATE;
    }
#endif


    if (!getdata(b_lines - 2, 0, SHM->i18nstr[cuser.language][387], path, 40, DOECHO) ||
	    path == NULL ||
	!getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][388],
		 yn, 5, LCECHO)
	|| yn[0] != 'y')
	return FULLUPDATE;

    snprintf(buf, sizeof(buf),
	     SHM->i18nstr[cuser.language][389],
	     cuser.userid, path,
	     51 - strlen(cuser.userid) - strlen(path), " ", fromhost,
	     ptime->tm_mon + 1, ptime->tm_mday);
    do_add_recommend(direct, fhdr,  ent, buf);
#ifdef ASSESS
    /* �C 10 ������ �[�@�� goodpost */
    if ((fhdr->filemode & FILE_MARKED) && fhdr->recommend % 10 == 0) {
	int uid = searchuser(fhdr->owner);
	if (uid > 0)
	    inc_goodpost(uid, 1);
    }
#endif
    lastrecommend = now;
    return FULLUPDATE;
}

static int
mark_post(int ent, fileheader_t * fhdr, char *direct)
{
    char buf[STRLEN], fpath[STRLEN];

    if (!(currmode & MODE_BOARD))
	return DONOTHING;

    setbpath(fpath, currboard);
    sprintf(buf, "%s/%s", fpath, fhdr->filename);

    if( !(fhdr->filemode & FILE_MARKED) && /* �Y�ثe�٨S�� mark �~�n check */
	access(buf, F_OK) < 0 )
	return DONOTHING;

    fhdr->filemode ^= FILE_MARKED;

#ifdef ASSESS
    if (!(fhdr->filemode & FILE_BID)){
	if (fhdr->filemode & FILE_MARKED) {
	    if (!(currbrdattr & BRD_BAD) && fhdr->recommend >= 10)
		inc_goodpost(searchuser(fhdr->owner), fhdr->recommend / 10);
	}
	else if (fhdr->recommend > 9)
    	    inc_goodpost(searchuser(fhdr->owner), -1 * (fhdr->recommend / 10));
    }
#endif
	substitute_ref_record(direct, fhdr, ent);
    return PART_REDRAW;
}

int
del_range(int ent, fileheader_t *fhdr, char *direct)
{
    char            num1[8], num2[8];
    int             inum1, inum2;
    boardheader_t  *bp = NULL;

    /* ���T�ر��p�|�i�o��, �H��, �ݪO, ��ذ� */
    if( !(direct[0] == 'h') ){ /* �H�󤣥� check */
        bp = getbcache(currbid);
	if (strcmp(bp->brdname, "Security") == 0)
	    return DONOTHING;
    }

    /* rocker.011018: �걵�Ҧ��U�٬O�����\�R������n */
    if (currmode & MODE_SELECT) {
	vmsg(SHM->i18nstr[cuser.language][390]);
	return FULLUPDATE;
    }

    if ((currstat != READING) || (currmode & MODE_BOARD)) {
	getdata(1, 0, SHM->i18nstr[cuser.language][391], num1, 6, DOECHO);
	inum1 = atoi(num1);
	if (inum1 <= 0) {
	    vmsg(SHM->i18nstr[cuser.language][392]);
	    return FULLUPDATE;
	}
	getdata(1, 28, SHM->i18nstr[cuser.language][393], num2, 6, DOECHO);
	inum2 = atoi(num2);
	if (inum2 < inum1) {
	    vmsg(SHM->i18nstr[cuser.language][394]);
	    return FULLUPDATE;
	}
	getdata(1, 48, msg_sure_ny, num1, 3, LCECHO);
	if (*num1 == 'y') {
	    outmsg(SHM->i18nstr[cuser.language][395]);
	    refresh();
#ifdef SAFE_ARTICLE_DELETE
		if(bp && !(currmode & MODE_DIGEST) && bp->nuser > 20 )
			safe_article_delete_range(direct, inum1, inum2);
		else
			delete_range(direct, inum1, inum2);
#else
		delete_range(direct, inum1, inum2);
#endif
		fixkeep(direct, inum1);
		
		if (currmode & MODE_BOARD)
			setbtotal(currbid);
		return DIRCHANGED;
	}
	return FULLUPDATE;
	}
    return DONOTHING;
}
                                                                                        
static int
del_post(int ent, fileheader_t * fhdr, char *direct)
{
    char            genbuf[100], newpath[256];
    int             num, not_owned;
    boardheader_t  *bp;

    bp = getbcache(currbid);
    if (strcmp(bp->brdname, "Security") == 0)
	return DONOTHING;
    if ((fhdr->filemode & FILE_BOTTOM) || 
       (fhdr->filemode & FILE_MARKED) || (fhdr->filemode & FILE_DIGEST) ||
	(fhdr->owner[0] == '-'))
	return DONOTHING;

    not_owned = strcmp(fhdr->owner, cuser.userid);
    if ((!(currmode & MODE_BOARD) && not_owned) ||
	((bp->brdattr & BRD_VOTEBOARD) && !HAS_PERM(PERM_SYSOP)) ||
	!strcmp(cuser.userid, STR_GUEST))
	return DONOTHING;

    if (currmode & MODE_SELECT) {
		vmsg("�Ц^��@��Ҧ��A�R���峹");
		return DONOTHING;
	}
                            

    getdata(1, 0, msg_del_ny, genbuf, 3, LCECHO);
    if (genbuf[0] == 'y') {
	if(
#ifdef SAFE_ARTICLE_DELETE
		(bp->nuser>20 && !(currmode & MODE_DIGEST) &&
			!safe_article_delete(ent, fhdr, direct)) ||                   
#endif
	   !delete_record(direct, sizeof(fileheader_t), ent)
	   ) {
		cancelpost(fhdr, not_owned, newpath);
			if(fhdr->filemode & FILE_ANONYMOUS)
		/* When the file is anonymous posted, fhdr->money is author.
		 * see do_general() */
				num = fhdr->money;
			else
            num = searchuser(fhdr->owner);
#ifdef ASSESS
#define SIZE	sizeof(badpost_reason) / sizeof(char *)

	    if (not_owned && num > 0 && !(currmode & MODE_DIGEST)) {
                getdata(1, 40, SHM->i18nstr[cuser.language][396], genbuf, 3, LCECHO);
		if(genbuf[0]=='y') {
		    int i;
		    char reason[64];
		    move(b_lines - 2, 0);
		    for (i = 0; i < SIZE; i++)
			prints("%d.%s ", i + 1, badpost_reason[i]);
		    prints("%d.%s", i + 1, SHM->i18nstr[cuser.language][397]);
		    getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][398], reason, sizeof(reason), LCECHO);
		    i = reason[0] - '0';
		    if (i <= 0 || i > SIZE)
			getdata(b_lines, 0, SHM->i18nstr[cuser.language][399], reason, sizeof(reason), DOECHO);
		    else
			strcpy(reason, badpost_reason[i - 1]);
		    if (!(inc_badpost(num, 1) % 10)){
			post_violatelaw(xuser.userid, SHM->i18nstr[cuser.language][400], SHM->i18nstr[cuser.language][401], SHM->i18nstr[cuser.language][402]);
			mail_violatelaw(xuser.userid, SHM->i18nstr[cuser.language][403], SHM->i18nstr[cuser.language][404], SHM->i18nstr[cuser.language][405]);
			xuser.userlevel |= PERM_VIOLATELAW;
		    }
		    sprintf(genbuf,SHM->i18nstr[cuser.language][406], reason, fhdr->title);
		    mail_id(xuser.userid, genbuf, newpath, cuser.userid);
		}
	    }
#undef SIZE
#endif

	    setbtotal(currbid);
	    if (fhdr->money < 0 || fhdr->filemode & FILE_ANONYMOUS)
		fhdr->money = 0;
	    if (not_owned && strcmp(currboard, "Test")) {
		deumoney(num, -fhdr->money);
	    }
	    if (!not_owned && strcmp(currboard, "Test")) {
		if (cuser.numposts)
		    cuser.numposts--;
		if (!(currmode & MODE_DIGEST && currmode & MODE_BOARD)){
		    demoney(-fhdr->money);
		    vmsg(SHM->i18nstr[cuser.language][407], msg_del_ok,
			    cuser.numposts, fhdr->money);
		}
	    }
	    return DIRCHANGED;
	}
    }
    return FULLUPDATE;
}

static int  // Ptt: �ץ��Y��   
show_filename(int ent, fileheader_t * fhdr, char *direct)
{
    if(!HAS_PERM(PERM_SYSOP)) return DONOTHING;

    vmsg(SHM->i18nstr[cuser.language][408], fhdr->filename);
    return PART_REDRAW;
}

static int
view_postmoney(int ent, fileheader_t * fhdr, char *direct)
{
    move(b_lines - 1, 0);
    if(currmode & MODE_SELECT){
	vmsg(SHM->i18nstr[cuser.language][409]);
	return FULLUPDATE;
    }
    if(fhdr->filemode & FILE_ANONYMOUS)
	/* When the file is anonymous posted, fhdr->money is author.
	 * see do_general() */
	prints(SHM->i18nstr[cuser.language][410],
		fhdr->money + currutmp->pid);
    else
	vmsg(SHM->i18nstr[cuser.language][411], fhdr->money);
    return FULLUPDATE;
}

#ifdef OUTJOBSPOOL
/* �ݪO�ƥ� */
static int
tar_addqueue(int ent, fileheader_t * fhdr, char *direct)
{
    char            email[60], qfn[80], ans[2];
    FILE           *fp;
    char            bakboard, bakman;
    clear();
    showtitle(SHM->i18nstr[cuser.language][412], BBSNAME);
    move(2, 0);
    if (!((currmode & MODE_BOARD) || HAS_PERM(PERM_SYSOP))) {
	move(5, 10);
	outs(SHM->i18nstr[cuser.language][413]);
	pressanykey();
	return FULLUPDATE;
    }
    snprintf(qfn, sizeof(qfn), BBSHOME "/jobspool/tarqueue.%s", currboard);
    if (access(qfn, 0) == 0) {
	outs(SHM->i18nstr[cuser.language][414]);
	pressanykey();
	return FULLUPDATE;
    }
    if (!getdata(4, 0, SHM->i18nstr[cuser.language][415], email, sizeof(email), DOECHO))
	return FULLUPDATE;

    /* check email -.-"" */
    if (strstr(email, "@") == NULL || strstr(email, ".bbs@") != NULL) {
	move(6, 0);
	outs(SHM->i18nstr[cuser.language][416]);
	pressanykey();
	return FULLUPDATE;
    }
    getdata(6, 0, SHM->i18nstr[cuser.language][417], ans, sizeof(ans), LCECHO);
    bakboard = (ans[0] == 'n' || ans[0] == 'N') ? 0 : 1;
    getdata(7, 0, SHM->i18nstr[cuser.language][418], ans, sizeof(ans), LCECHO);
    bakman = (ans[0] == 'y' || ans[0] == 'Y') ? 1 : 0;
    if (!bakboard && !bakman) {
	move(8, 0);
	outs(SHM->i18nstr[cuser.language][419]);
	pressanykey();
	return FULLUPDATE;
    }
    fp = fopen(qfn, "w");
    fprintf(fp, "%s\n", cuser.userid);
    fprintf(fp, "%s\n", email);
    fprintf(fp, "%d,%d\n", bakboard, bakman);
    fclose(fp);

    move(10, 0);
    outs(SHM->i18nstr[cuser.language][420]);
    outs(SHM->i18nstr[cuser.language][421]);
    pressanykey();
    return FULLUPDATE;
}
#endif

static int      sequent_ent;
static int      continue_flag;

/* ----------------------------------------------------- */
/* �̧�Ū�s�峹                                          */
/* ----------------------------------------------------- */
static int
sequent_messages(fileheader_t * fptr)
{
    static int      idc;
    char            genbuf[200];

    if (fptr == NULL)
	return (idc = 0);

    if (++idc < sequent_ent)
	return 0;

    if (!brc_unread(fptr->filename, brc_num, brc_list))
	return 0;

    if (continue_flag)
	genbuf[0] = 'y';
    else {
	prints(SHM->i18nstr[cuser.language][422],
	       currboard, fptr->owner, fptr->title);
	getdata(3, 0, "(Y/N/Quit) [Y]: ", genbuf, 3, LCECHO);
    }

    if (genbuf[0] != 'y' && genbuf[0]) {
	clear();
	return (genbuf[0] == 'q' ? QUIT : 0);
    }
    setbfile(genbuf, currboard, fptr->filename);
    brc_addlist(fptr->filename);

    if (more(genbuf, YEA) == 0)
	outmsg(SHM->i18nstr[cuser.language][423]);
    continue_flag = 0;

    switch (igetch()) {
    case KEY_LEFT:
    case 'e':
    case 'q':
    case 'Q':
	break;

    case 'y':
    case 'r':
    case 'Y':
    case 'R':
	if (CheckPostPerm()) {
	    strlcpy(quote_file, genbuf, sizeof(quote_file));
	    do_reply(fptr);
	    *quote_file = 0;
	}
	break;

    case ' ':
    case KEY_DOWN:
    case '\n':
    case 'n':
	continue_flag = 1;
    }

    clear();
    return 0;
}

static int
sequential_read(int ent, fileheader_t * fhdr, char *direct)
{
    char            buf[40];

    clear();
    sequent_messages((fileheader_t *) NULL);
    sequent_ent = ent;
    continue_flag = 0;
    setbdir(buf, currboard);
    apply_record(buf, sequent_messages, sizeof(fileheader_t));
    return FULLUPDATE;
}

/* ----------------------------------------------------- */
/* �ݪO�Ƨѿ��B��K�B��ذ�                              */
/* ----------------------------------------------------- */
int
b_note_edit_bname(int bid)
{
    char            buf[64];
    int             aborted;
    boardheader_t  *fh = getbcache(bid);
    setbfile(buf, fh->brdname, fn_notes);
    aborted = vedit(buf, NA, NULL);
    if (aborted == -1) {
	clear();
	outs(msg_cancel);
	pressanykey();
    } else {
	if (!getdata(2, 0, SHM->i18nstr[cuser.language][424], buf, 3, LCECHO)
	    || buf[0] != 'n')
	    fh->bupdate = gettime(3, fh->bupdate ? fh->bupdate : now, 
		      SHM->i18nstr[cuser.language][425]);
	else
	    fh->bupdate = 0;
	substitute_record(fn_board, fh, sizeof(boardheader_t), bid);
    }
    return 0;
}

static int
b_notes_edit()
{
    if (currmode & MODE_BOARD) {
	b_note_edit_bname(currbid);
	return FULLUPDATE;
    }
    return 0;
}

static int
b_water_edit()
{
    if (currmode & MODE_BOARD) {
	friend_edit(BOARD_WATER);
	return FULLUPDATE;
    }
    return 0;
}

static int
visable_list_edit()
{
    if (currmode & MODE_BOARD) {
	friend_edit(BOARD_VISABLE);
	hbflreload(currbid);
	return FULLUPDATE;
    }
    return 0;
}

static int
b_post_note()
{
    char            buf[200], yn[3];
    if (currmode & MODE_BOARD) {
	setbfile(buf, currboard, FN_POST_NOTE);
	if (more(buf, NA) == -1)
	    more("etc/" FN_POST_NOTE, NA);
	getdata(b_lines - 2, 0, SHM->i18nstr[cuser.language][426], yn, sizeof(yn), LCECHO);
	if (yn[0] == 'y')
	    vedit(buf, NA, NULL);
	else
	    unlink(buf);


	setbfile(buf, currboard, FN_POST_BID);
	if (more(buf, NA) == -1)
	    more("etc/" FN_POST_BID, NA);
	getdata(b_lines - 2, 0, SHM->i18nstr[cuser.language][427], yn, sizeof(yn), LCECHO);
	if (yn[0] == 'y')
	    vedit(buf, NA, NULL);
	else
	    unlink(buf);

	return FULLUPDATE;
    }
    return 0;
}


static int
can_vote_edit()
{
    if (currmode & MODE_BOARD) {
	friend_edit(FRIEND_CANVOTE);
	return FULLUPDATE;
    }
    return 0;
}

static int
bh_title_edit()
{
    boardheader_t  *bp;

    if (currmode & MODE_BOARD) {
	char            genbuf[BTLEN];

	bp = getbcache(currbid);
	move(1, 0);
	clrtoeol();
	getdata_str(1, 0, SHM->i18nstr[cuser.language][428], genbuf,
		    BTLEN - 16, DOECHO, bp->title + 7);

	if (!genbuf[0])
	    return 0;
	strip_ansi(genbuf, genbuf, STRIP_ALL);
	strlcpy(bp->title + 7, genbuf, sizeof(bp->title) - 7);
	substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
	log_usies("SetBoard", currboard);
	return FULLUPDATE;
    }
    return 0;
}

static int
b_notes()
{
    char            buf[64];

    setbfile(buf, currboard, fn_notes);
    if (more(buf, NA) == -1) {
	clear();
	move(4, 20);
	outs(SHM->i18nstr[cuser.language][429]);
    }
    pressanykey();
    return FULLUPDATE;
}

int
board_select()
{
    char            fpath[80];
    char            genbuf[100];

    currmode &= ~MODE_SELECT;
    snprintf(fpath, sizeof(fpath), "SR.%s", cuser.userid);
    setbfile(genbuf, currboard, fpath);
    unlink(genbuf);
    if (currstat == RMAIL)
	sethomedir(currdirect, cuser.userid);
    else
	setbdir(currdirect, currboard);
    return NEWDIRECT;
}

int
board_digest()
{
    if (currmode & MODE_SELECT)
	board_select();
    currmode ^= MODE_DIGEST;
    if (currmode & MODE_DIGEST)
	currmode &= ~MODE_POST;
    else if (haspostperm(currboard))
	currmode |= MODE_POST;

    setbdir(currdirect, currboard);
    return NEWDIRECT;
}

int
board_etc()
{
    if (!HAS_PERM(PERM_SYSOP))
	return DONOTHING;
    currmode ^= MODE_ETC;
    if (currmode & MODE_ETC)
	currmode &= ~MODE_POST;
    else if (haspostperm(currboard))
	currmode |= MODE_POST;

    setbdir(currdirect, currboard);
    return NEWDIRECT;
}

static int
push_bottom(int ent, fileheader_t * fhdr, char *direct)
{
    int num;
    char buf[256];
    if ((currmode & MODE_DIGEST) || !(currmode & MODE_BOARD))
        return DONOTHING;
    setbottomtotal(currbid);  // Ptt : will be remove when stable
    num = getbottomtotal(currbid);
    if(getans(fhdr->filemode & FILE_BOTTOM ?
       SHM->i18nstr[cuser.language][430]:
       SHM->i18nstr[cuser.language][431])!='y') return READ_REDRAW;
    fhdr->filemode ^= FILE_BOTTOM;
    if(fhdr->filemode & FILE_BOTTOM)
       {
          sprintf(buf, "%s.bottom", direct);
          if(num >= 5)
            {
              vmsg(SHM->i18nstr[cuser.language][432]);
              return READ_REDRAW;
            }
          fhdr->money = ent | FHR_REFERENCE;
          append_record(buf, fhdr, sizeof(fileheader_t)); 
       }
    else
       {
          num = delete_record(direct, sizeof(fileheader_t), ent);
       }
    setbottomtotal(currbid);
    return DIRCHANGED;
}

static int
good_post(int ent, fileheader_t * fhdr, char *direct)
{
    char            genbuf[200];
    char            genbuf2[200];
    int             delta = 0;

    if ((currmode & MODE_DIGEST) || !(currmode & MODE_BOARD))
	return DONOTHING;

    if(getans(fhdr->filemode & FILE_DIGEST ? 
              SHM->i18nstr[cuser.language][433] : SHM->i18nstr[cuser.language][434]) == 'n')
	return READ_REDRAW;

    if (fhdr->filemode & FILE_DIGEST) {
	fhdr->filemode = (fhdr->filemode & ~FILE_DIGEST);
	if (!strcmp(currboard, "Note") || !strcmp(currboard, "PttBug") ||
	    !strcmp(currboard, "Artdsn") || !strcmp(currboard, "PttLaw")) {
	    deumoney(searchuser(fhdr->owner), -1000);
	    if (!(currmode & MODE_SELECT))
		fhdr->money -= 1000;
	    else
		delta = -1000;
	}
    } else {
	fileheader_t    digest;
	char           *ptr, buf[64];

	memcpy(&digest, fhdr, sizeof(digest));
	digest.filename[0] = 'G';
	strlcpy(buf, direct, sizeof(buf));
	ptr = strrchr(buf, '/');
	assert(ptr);
	ptr++;
	ptr[0] = '\0';
	snprintf(genbuf, sizeof(genbuf), "%s%s", buf, digest.filename);

	if (dashf(genbuf))
	    unlink(genbuf);

	digest.filemode = 0;
	snprintf(genbuf2, sizeof(genbuf2), "%s%s", buf, fhdr->filename);
	Link(genbuf2, genbuf);
	strcpy(ptr, fn_mandex);
	append_record(buf, &digest, sizeof(digest));

#ifdef GLOBAL_DIGEST
	if(!(getbcache(currbid)->brdattr & BRD_HIDE)) { 
          getdata(1, 0, SHM->i18nstr[cuser.language][435], genbuf2, 3, LCECHO);
          if(genbuf2[0] == 'y')
	      do_crosspost(GLOBAL_DIGEST, &digest, genbuf);
        }
#endif

	fhdr->filemode = (fhdr->filemode & ~FILE_MARKED) | FILE_DIGEST;
	if (!strcmp(currboard, "Note") || !strcmp(currboard, "PttBug") ||
	    !strcmp(currboard, "Artdsn") || !strcmp(currboard, "PttLaw")) {
	    deumoney(searchuser(fhdr->owner), 1000);
	    if (!(currmode & MODE_SELECT))
		fhdr->money += 1000;
	    else
		delta = 1000;
	}
    }
	substitute_ref_record(direct, fhdr, ent);
    return FULLUPDATE;
}

/* help for board reading */
static int board_help[] = {
	436, 437, 438, 439, 440, 441, 442, 443, 444, 445, 446, 447,
#ifdef INTERNET_EMAIL
    448,
#endif
    449, 450, 451, 452, 453, 454,
#ifdef NO_GAMBLE
	455,
#else
    456,
#endif
    -1
};

static int
b_help()
{
    show_help(board_help);
    return FULLUPDATE;
}

static int
b_changerecommend(int ent, fileheader_t * fhdr, char *direct)
{
    boardheader_t   *bp=NULL;
    if (!((currmode & MODE_BOARD) || HAS_PERM(PERM_SYSOP)))
	return DONOTHING;
    bp = getbcache(currbid); 
    bp->brdattr ^= BRD_NORECOMMEND; 
    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    vmsg(SHM->i18nstr[cuser.language][457],
         (bp->brdattr & BRD_NORECOMMEND) ? SHM->i18nstr[cuser.language][458] : SHM->i18nstr[cuser.language][459]);
    return FULLUPDATE;
}

/* ----------------------------------------------------- */
/* �O�D�]�w����/ ������                                  */
/* ----------------------------------------------------- */
char            board_hidden_status;
#ifdef  BMCHS
static int
change_hidden(int ent, fileheader_t * fhdr, char *direct)
{
    boardheader_t   *bp;
    if (!((currmode & MODE_BOARD) || HAS_PERM(PERM_SYSOP)))
	return DONOTHING;

    bp = getbcache(currbid);
    if (((bp->brdattr & BRD_HIDE) && (bp->brdattr & BRD_POSTMASK))) {
	if (getans(SHM->i18nstr[cuser.language][460]) != 'y')
	    return FULLUPDATE;
	bp->brdattr &= ~BRD_HIDE;
	bp->brdattr &= ~BRD_POSTMASK;
	outs(SHM->i18nstr[cuser.language][461]);
	board_hidden_status = 0;
	hbflreload(currbid);
    } else {
	if (getans(SHM->i18nstr[cuser.language][462]) != 'y')
	    return FULLUPDATE;
	bp->brdattr |= BRD_HIDE;
	bp->brdattr |= BRD_POSTMASK;
	outs(SHM->i18nstr[cuser.language][463]);
	board_hidden_status = 1;
    }
    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    log_usies("SetBoard", bp->brdname);
    pressanykey();
    return FULLUPDATE;
}

static int
change_counting(int ent, fileheader_t * fhdr, char *direct)
{   

    boardheader_t   *bp;
    if (!((currmode & MODE_BOARD) || HAS_PERM(PERM_SYSOP)))
	return DONOTHING;
    bp = getbcache(currbid);
    if (!(bp->brdattr & BRD_HIDE && bp->brdattr & BRD_POSTMASK))
	return FULLUPDATE;

    if (bp->brdattr & BRD_BMCOUNT) {
	if (getans(SHM->i18nstr[cuser.language][464]) != 'y')
	    return FULLUPDATE;
	bp->brdattr &= ~BRD_BMCOUNT;
	outs(SHM->i18nstr[cuser.language][465]);
    } else {
	if (getans(SHM->i18nstr[cuser.language][466]) != 'y')
	    return FULLUPDATE;
	bp->brdattr |= BRD_BMCOUNT;
	outs(SHM->i18nstr[cuser.language][467]);
    }
    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    pressanykey();
    return FULLUPDATE;
}

#endif

/* ----------------------------------------------------- */
/* �ݪO�\���                                            */
/* ----------------------------------------------------- */
/* onekey_size was defined in ../include/pttstruct.h, as ((int)'z') */
onekey_t read_comms[] = {
    show_filename, // Ctrl('A') 
    NULL, // Ctrl('B')
    NULL, // Ctrl('C')
    NULL, // Ctrl('D')
    NULL, // Ctrl('E')
    NULL, // Ctrl('F')
#ifdef NO_GAMBLE
    NULL, // Ctrl('G')
#else
    hold_gamble, // Ctrl('G')
#endif
    NULL, // Ctrl('H')
    board_digest, // Ctrl('I') KEY_TAB 9
    NULL, // Ctrl('J')
    NULL, // Ctrl('K')
    NULL, // Ctrl('L')
    NULL, // Ctrl('M')
#ifdef BMCHS
    change_counting, // Ctrl('N')
#else
    NULL, // Ctrl('N')
#endif
    do_post_openbid, // Ctrl('O')
    do_post, // Ctrl('P')
    NULL, // Ctrl('Q')
    NULL, // Ctrl('R')
    NULL, // Ctrl('S')
    NULL, // Ctrl('T')
    NULL, // Ctrl('U')
    do_post_vote, // Ctrl('V')
    whereami, // Ctrl('W')
    NULL, // Ctrl('X')
    NULL, // Ctrl('Y')
    push_bottom, // Ctrl('Z') 26
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, // 'A' 65
    bh_title_edit, // 'B'
    board_etc, // 'C'
    del_range, // 'D'
    edit_post, // 'E'
    NULL, // 'F'
    NULL, // 'G'
#ifdef BMCHS
    change_hidden, // 'H'
#else
    NULL, // 'H'
#endif
    b_changerecommend, // 'I'
    NULL, // 'J'
    b_water_edit, // 'K'
    solve_post, // 'L'
    b_vote_maintain, // 'M'
    NULL, // 'N'
    b_post_note, // 'O'
    NULL, // 'P'
    view_postmoney, // 'Q'
    b_results, // 'R'
    sequential_read, // 'S'
    edit_title, // 'T'
    NULL, // 'U'
    b_vote, // 'V'
    b_notes_edit, // 'W'
    recommend, // 'X'
    recommend_cancel, // 'Y'
    NULL, // 'Z' 90
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, // 'a' 97
    b_notes, // 'b'
    cite_post, // 'c'
    del_post, // 'd'
    NULL, // 'e'
#ifdef NO_GAMBLE
    NULL, // 'f'
#else
    join_gamble, // 'f'
#endif
    good_post, // 'g'
    b_help, // 'h'
    b_posttype, // 'i'
    NULL, // 'j'
    NULL, // 'k'
    NULL, // 'l'
    mark_post, // 'm'
    NULL, // 'n'
    can_vote_edit, // 'o'
    NULL, // 'p'
    NULL, // 'q'
    read_post, // 'r'
    do_select, // 's'
    NULL, // 't'
#ifdef OUTJOBSPOOL
    tar_addqueue, // 'u'
#else
    NULL, // 'u'
#endif
    visable_list_edit, // 'v'
    b_call_in, // 'w'
    cross_post, // 'x'
    reply_post, // 'y'
    b_man, // 'z' 122
};

int
Read()
{
    int             mode0 = currutmp->mode;
    int             stat0 = currstat, tmpbid = currutmp->brc_id;
    char            buf[40];
#ifdef LOG_BOARD
    time_t          usetime = now;
#endif

    if ( ! currboard[0] )
	brc_initial_board(DEFAULT_BOARD);

    setutmpmode(READING);
    set_board();

    if (board_visit_time < *board_note_time) {
	setbfile(buf, currboard, fn_notes);
	if(more(buf, NA)!=-1)
   	    pressanykey();
        else
            *board_note_time=0;
    }
    setutmpbid(currbid);
    setbdir(buf, currboard);
    curredit &= ~EDIT_MAIL;
    i_read(READING, buf, readtitle, readdoent, read_comms,
	   currbid);
    currmode &= ~MODE_POSTCHECKED;
#ifdef LOG_BOARD
    log_board(currboard, now - usetime);
#endif
    brc_update();
    setutmpbid(tmpbid);
    currutmp->mode = mode0;
    currstat = stat0;
    return 0;
}

void
ReadSelect()
{
    int             mode0 = currutmp->mode;
    int             stat0 = currstat;
    char            genbuf[200];

    currstat = SELECT;
    if (do_select(0, 0, genbuf) == NEWDIRECT)
	Read();
    setutmpbid(0);
    currutmp->mode = mode0;
    currstat = stat0;
}

#ifdef LOG_BOARD
static void
log_board(char *mode, time_t usetime)
{
    if (usetime > 30) {
		log_file(FN_USEBOARD, 1, "USE %-20.20s Stay: %5ld (%s) %s\n", 
			mode, usetime, cuser.userid, ctime(&now));                     
    }
}
#endif

int
Select()
{
    char            genbuf[200];

    do_select(0, NULL, genbuf);
    return 0;
}
#ifdef HAVEMOBILE
void
mobile_message(char *mobile, char *message)
{
    bsmtp(char *fpath, char *title, char *rcpt, int method);
}

#endif
