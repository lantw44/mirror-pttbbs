/* $Id$ */
#include "bbs.h"
#define BRC_STRLEN 15		/* Length of board name */
#define BRC_MAXSIZE     24576   /* Effective size of brc rc file */
#define BRC_ITEMSIZE    (BRC_STRLEN + 1 + BRC_MAXNUM * sizeof( int ))
    /* Maximum size of each record */
#define BRC_MAXNUM      80      /* Upper bound of brc_num, size of brc_list  */

/* brc rc file form:
 * board_name     15 bytes
 * brc_num         1 byte, binary integer
 * brc_list       brc_num * sizeof(int) bytes, brc_num binary integer(s) */

static char    *
brc_getrecord(char *ptr, char *name, int *pnum, int *list)
{
    int             num;
    char           *tmp;

    strncpy(name, ptr, BRC_STRLEN); /* board_name */
    ptr += BRC_STRLEN;
    num = (*ptr++) & 0xff;          /* brc_num */
    tmp = ptr + num * sizeof(int);  /* end of this record */
    if (num > BRC_MAXNUM) /* FIXME if this happens... may crash next time. */
	num = BRC_MAXNUM;
    *pnum = num;
    memcpy(list, ptr, num * sizeof(int)); /* brc_list */
    return tmp;
}

static time_t   brc_expire_time;
 /* Will be set to the time one year before login. All the files created
  * before then will be recognized read. */

static char    *
brc_putrecord(char *ptr, const char *name, int num, const int *list)
{
    if (num > 0 && list[0] > brc_expire_time) {
	if (num > BRC_MAXNUM)
	    num = BRC_MAXNUM;

	while (num > 1 && list[num - 1] < brc_expire_time)
	    num--; /* not to write the times before brc_expire_time */

	strncpy(ptr, name, BRC_STRLEN);  /* write in board_name */
	ptr += BRC_STRLEN;
	*ptr++ = num;                    /* write in brc_num */
	memcpy(ptr, list, num * sizeof(int)); /* write in brc_list */
	ptr += num * sizeof(int);
    }
    return ptr;
}

static int      brc_changed = 0;
/* The below two will be filled by read_brc_buf() and brc_update() */
static char     brc_buf[BRC_MAXSIZE];
static int      brc_size;
//static char     brc_name[BRC_STRLEN]; /* board name of the brc data */
static char * const fn_boardrc = ".boardrc";
/* unused variable
char *brc_buf_addr=brc_buf;
*/

void
brc_update()
{
    if (brc_changed && cuser.userlevel) {
	char            dirfile[STRLEN], *ptr;
	char            tmp_buf[BRC_MAXSIZE - BRC_ITEMSIZE], *tmp;
	char            tmp_name[BRC_STRLEN];
	int             tmp_list[BRC_MAXNUM], tmp_num;
	int             fd, tmp_size;

	ptr = brc_buf;
	if (brc_num > 0)
	    ptr = brc_putrecord(ptr, currboard, brc_num, brc_list);

	setuserfile(dirfile, fn_boardrc);
	if ((fd = open(dirfile, O_RDONLY)) != -1) {
	    tmp_size = read(fd, tmp_buf, sizeof(tmp_buf));
	    close(fd);
	} else {
	    tmp_size = 0;
	}

	tmp = tmp_buf;
	while (tmp < &tmp_buf[tmp_size] && (*tmp >= ' ' && *tmp <= 'z')) {
	    /* for each available records */
	    tmp = brc_getrecord(tmp, tmp_name, &tmp_num, tmp_list);
	    if (strncmp(tmp_name, currboard, BRC_STRLEN))
		/* not overwrite the currend record */
		ptr = brc_putrecord(ptr, tmp_name, tmp_num, tmp_list);
	}
	brc_size = (int)(ptr - brc_buf);

	if ((fd = open(dirfile, O_WRONLY | O_CREAT, 0644)) != -1) {
	    ftruncate(fd, 0);
	    write(fd, brc_buf, brc_size);
	    close(fd);
	}
	brc_changed = 0;
    }
}

static void
read_brc_buf()
{
    char            dirfile[STRLEN];
    int             fd;

    if (brc_buf[0] == '\0') {
	setuserfile(dirfile, fn_boardrc);
	if ((fd = open(dirfile, O_RDONLY)) != -1) {
	    brc_size = read(fd, brc_buf, sizeof(brc_buf));
	    close(fd);
	} else {
	    brc_size = 0;
	}
    }
}

int
brc_initial(const char *boardname)
{
    char           *ptr;
    char            tmp_name[BRC_STRLEN];
    if (strcmp(currboard, boardname) == 0) {
	return brc_num;
    }
    brc_update(); /* write back first */
    strlcpy(currboard, boardname, sizeof(currboard));
    currbid = getbnum(currboard);
    currbrdattr = bcache[currbid - 1].brdattr;
    read_brc_buf();

    ptr = brc_buf;
    while (ptr < &brc_buf[brc_size] && (*ptr >= ' ' && *ptr <= 'z')) {
	/* for each available records */
	ptr = brc_getrecord(ptr, tmp_name, &brc_num, brc_list);
	if (strncmp(tmp_name, currboard, BRC_STRLEN) == 0)
	    return brc_num;
    }
    strncpy(tmp_name, boardname, BRC_STRLEN);
    brc_num = brc_list[0] = 1;
    /* We don't have to set brc_changed to 0 here, since brc_update() already
     * did that. */
    return 0;
}

void
brc_addlist(const char *fname)
{
    int             ftime, n, i;

    if (!cuser.userlevel)
	return;

    ftime = atoi(&fname[2]);
    if (ftime <= brc_expire_time /* too old, don't do any thing  */
	 /* || fname[0] != 'M' || fname[1] != '.' */ ) {
	return;
    }
    if (brc_num <= 0) { /* uninitialized */
	brc_list[0] = ftime;
	brc_num = 1;
	brc_changed = 1;
	return;
    }
    if ((brc_num == 1) && (ftime < brc_list[0])) /* most when after 'v' */
	return;
    for (n = 0; n < brc_num; n++) { /* using linear search */
	if (ftime == brc_list[n]) {
	    return;
	} else if (ftime > brc_list[n]) {
	    if (brc_num < BRC_MAXNUM)
		brc_num++;
	    /* insert ftime in to brc_list */
	    for (i = brc_num - 1; --i >= n; brc_list[i + 1] = brc_list[i]);
	    brc_list[n] = ftime;
	    brc_changed = 1;
	    return;
	}
    }
    /* (by scw) These lines are no used. Since if it reachs here, this file
     * is already been labeled read.
    if (brc_num < BRC_MAXNUM) {
	brc_list[brc_num++] = ftime;
	brc_changed = 1;
    }
    */
}

static int
brc_unread_time(time_t ftime, int bnum, const int *blist)
{
    int             n;

    if (ftime <= brc_expire_time) /* too old */
	return 0;

    if (brc_num <= 0)
	return 1;
    for (n = 0; n < bnum; n++) { /* using linear search */
	if (ftime > blist[n])
	    return 1;
	else if (ftime == blist[n])
	    return 0;
    }
    return 0;
}

int
brc_unread(const char *fname, int bnum, const int *blist)
{
    int             ftime, n;

    ftime = atoi(&fname[2]); /* this will get the time of the file created */

    if (ftime <= brc_expire_time) /* too old */
	return 0;

    if (brc_num <= 0)
	return 1;
    for (n = 0; n < bnum; n++) { /* using linear search */
	if (ftime > blist[n])
	    return 1;
	else if (ftime == blist[n])
	    return 0;
    }
    return 0;
}

#define BRD_NULL	0
#define BRD_FAV    	1
#define BRD_BOARD	2
#define BRD_LINE   	4
#define BRD_FOLDER	8
#define BRD_TAG        16
#define BRD_UNREAD     32

#define FAVNB      ".favnb"
//#define FAV3       ".fav3"

#define B_TOTAL(bptr)        (SHM->total[(bptr)->bid - 1])
#define B_LASTPOSTTIME(bptr) (SHM->lastposttime[(bptr)->bid - 1])
#define B_BH(bptr)           (&bcache[(bptr)->bid - 1])
typedef struct {
    int             bid;
    unsigned char   myattr;
} __attribute__ ((packed)) boardstat_t;

static boardstat_t *nbrd = NULL;
static char	choose_board_depth = 0;
static short    brdnum;
static char     yank_flag = 1;

#define BRD_OLD 0
#define BRD_NEW 1
#define BRD_END 2

void updatenewfav(int mode)
{
    /* mode: 0: don't write to fav  1: write to fav */
    int i, fd;
    char fname[80], *brd;

    if(!(cuser.uflag2 & FAVNEW_FLAG))
	return;

    setuserfile(fname, FAVNB);

    if( (fd = open(fname, O_RDWR, 0600)) != -1 ){

	brd = (char *)malloc((numboards + 1) * sizeof(char));
	read(fd, brd, (numboards + 1) * sizeof(char));
	
	for(i = 0; i < numboards + 1 && brd[i] != BRD_END; i++){
	    if(brd[i] == BRD_NEW){
		if(bcache[i].brdname[0] && Ben_Perm(&bcache[i])){ // check the permission if the board exsits
		    if(mode)
			fav_add_board(i + 1);
		    brd[i] = BRD_OLD;
		}
	    }
	    else{
		if(!bcache[i].brdname[0])
		    brd[i] = BRD_NEW;
	    }
	}
	if( i < numboards) // the board number may change
	    for(i-- ; i < numboards; i++){
		if(bcache[i].brdname[0] && Ben_Perm(&bcache[i])){
		    if(mode)
			fav_add_board(i + 1);
		    brd[i] = BRD_OLD;
		}
		else
		    brd[i] = BRD_NEW;
	    }

	brd[i] = BRD_END;
	
	lseek(fd, 0, SEEK_SET);
	write(fd, brd, (numboards + 1 ) * sizeof(char));
	free(brd);
	close(fd);
    }
}

void imovefav(int old)
{
    char buf[5];
    int new;
    
    getdata(b_lines - 1, 0, "請輸入新次序:", buf, sizeof(buf), DOECHO);
    new = atoi(buf) - 1;
    if (new < 0 || brdnum <= new){
	vmsg("輸入範圍有誤!");
	return;
    }
    move_in_current_folder(old, new);
}

inline int validboard(int bid){
    return bcache[bid].brdname[0];
}

void load_brdbuf(void)
{
    static  char    firsttime = 1;
    int     fd;
    char    fname[80];

    setuserfile(fname, FAV4);
    if( (fd = open(fname, O_RDONLY)) == -1 ){
	fav_v3_to_v4();
    }
    fav_load();
    updatenewfav(1);
    firsttime = 0;
}

void
init_brdbuf()
{
    brc_expire_time = login_start_time - 365 * 86400;
}

void
save_brdbuf(void)
{
    fav_save();
    fav_free();
}

int
Ben_Perm(boardheader_t * bptr)
{
    register int    level, brdattr;

    level = bptr->level;
    brdattr = bptr->brdattr;

    if (HAS_PERM(PERM_SYSOP))
	return 1;

    if( is_BM_cache(bptr - bcache + 1) ) /* XXXbid */
	return 1;

    /* 祕密看板：核對首席板主的好友名單 */

    if (brdattr & BRD_HIDE) {	/* 隱藏 */
	if (hbflcheck((int)(bptr - bcache) + 1, currutmp->uid)) {
	    if (brdattr & BRD_POSTMASK)
		return 0;
	    else
		return 2;
	} else
	    return 1;
    }
    /* 限制閱讀權限 */
    if (level && !(brdattr & BRD_POSTMASK) && !HAS_PERM(level))
	return 0;

    return 1;
}

#if 0
static int
have_author(char *brdname)
{
    char            dirname[100];

    snprintf(dirname, sizeof(dirname),
	     "正在搜尋作者[33m%s[m 看板:[1;33m%s[0m.....",
	     currauthor, brdname);
    move(b_lines, 0);
    clrtoeol();
    outs(dirname);
    refresh();

    setbdir(dirname, brdname);
    str_lower(currowner, currauthor);

    return search_rec(dirname, cmpfowner);
}
#endif

static int
check_newpost(boardstat_t * ptr)
{				/* Ptt 改 */
    int             tbrc_list[BRC_MAXNUM], tbrc_num;
    char            bname[BRC_STRLEN];
    char           *po;
    time_t          ftime;

    ptr->myattr &= ~BRD_UNREAD;
    if (B_BH(ptr)->brdattr & BRD_GROUPBOARD)
	return 0;

    if (B_TOTAL(ptr) == 0)
	setbtotal(ptr->bid);
    if (B_TOTAL(ptr) == 0)
	return 0;
    ftime = B_LASTPOSTTIME(ptr);
    if( ftime > now )
	ftime = B_LASTPOSTTIME(ptr) = now - 1;
    read_brc_buf();
    po = brc_buf;
    while (po < &brc_buf[brc_size] && (*po >= ' ' && *po <= 'z')) {
	po = brc_getrecord(po, bname, &tbrc_num, tbrc_list);
	if (strncmp(bname, B_BH(ptr)->brdname, BRC_STRLEN) == 0) {
	    if (brc_unread_time(ftime, tbrc_num, tbrc_list)) {
		ptr->myattr |= BRD_UNREAD;
	    }
	    return 1;
	}
    }

    ptr->myattr |= BRD_UNREAD;
    return 1;
}

static void
load_uidofgid(const int gid, const int type)
{
    boardheader_t  *bptr, *currbptr;
    int             n, childcount = 0;
    currbptr = &bcache[gid - 1];
    for (n = 0; n < numboards; ++n) {
	bptr = SHM->bsorted[type][n];
	if (bptr->brdname[0] == '\0')
	    continue;
	if (bptr->gid == gid) {
	    if (currbptr == &bcache[gid - 1])
		currbptr->firstchild[type] = bptr;
	    else {
		currbptr->next[type] = bptr;
		currbptr->parent = &bcache[gid - 1];
	    }
	    childcount++;
	    currbptr = bptr;
	}
    }
    bcache[gid - 1].childcount = childcount;
    if (currbptr == &bcache[gid - 1])
	currbptr->firstchild[type] = NULL;
    else
	currbptr->next[type] = NULL;
}

static boardstat_t *
addnewbrdstat(int n, int state)
{
    boardstat_t    *ptr;

    ptr = &nbrd[brdnum++];
    //boardheader_t  *bptr = &bcache[n];
    //ptr->total = &(SHM->total[n]);
    //ptr->lastposttime = &(SHM->lastposttime[n]);
    
    ptr->bid = n + 1;
    ptr->myattr = state;
    if ((B_BH(ptr)->brdattr & BRD_HIDE) && state == BRD_BOARD)
	B_BH(ptr)->brdattr |= BRD_POSTMASK;
    if (yank_flag != 0)
	ptr->myattr &= ~BRD_FAV;
    check_newpost(ptr);
    return ptr;
}

static int
cmpboardfriends(const void *brd, const void *tmp)
{
    return ((B_BH((boardstat_t*)tmp)->nuser) -
	    (B_BH((boardstat_t*)brd)->nuser));
}

static void
load_boards(char *key)
{
    boardheader_t  *bptr = NULL;
    int             type = cuser.uflag & BRDSORT_FLAG ? 1 : 0;
    int             i, n;
    int             state;
    char            byMALLOC = 0, needREALLOC = 0;

    if (class_bid > 0) {
	bptr = &bcache[class_bid - 1];
	if (bptr->firstchild[type] == NULL || bptr->childcount <= 0)
	    load_uidofgid(class_bid, type);
    }
    brdnum = 0;
    if (nbrd) {
        free(nbrd);
	nbrd = NULL;
    }
    if (class_bid <= 0) {
	if( yank_flag == 0 ){ // fav mode
	    fav_t *fav = get_current_fav();

	    nbrd = (boardstat_t *)malloc(sizeof(boardstat_t) * get_data_number(fav));
            for( i = 0 ; i < fav->DataTail; ++i ){
		int state;
		if (!is_visible_item(&fav->favh[i]))
		    continue;

		if (get_item_type(&fav->favh[i]) == FAVT_LINE && !key[0])
		    state = BRD_LINE;
		else if (get_item_type(&fav->favh[i]) == FAVT_FOLDER && !key[0])
		    state = BRD_FOLDER;
		else{
		    bptr = &bcache[ fav_getid(&fav->favh[i]) - 1];
		    if( (state = Ben_Perm(bptr)) && (!key[0] || strcasestr(bptr->title, key)))
			state = BRD_BOARD;
		    else
			continue;
		    if (is_set_attr(&fav->favh[i], FAVH_UNREAD))
			state |= BRD_UNREAD;
		}

		if (is_set_attr(&fav->favh[i], FAVH_TAG))
		    state |= BRD_TAG;
		addnewbrdstat(fav_getid(&fav->favh[i]) - 1, BRD_FAV | state);
	    }
	    if (brdnum == 0)
		addnewbrdstat(0, 0);
	    byMALLOC = 0;
	    needREALLOC = (get_data_number(fav) != brdnum);
	}
	else{ // general case
	    nbrd = (boardstat_t *) MALLOC(sizeof(boardstat_t) * numboards);
	    for (i = 0; i < numboards; i++) {
		if ((bptr = SHM->bsorted[type][i]) == NULL)
		    continue;
		n = (int)(bptr - bcache);
		if (!bptr->brdname[0] ||
			(bptr->brdattr & BRD_GROUPBOARD) ||
			!((state = Ben_Perm(bptr)) || (currmode & MODE_MENU)) ||
			(key[0] && !strcasestr(bptr->title, key)) ||
			(class_bid == -1 && bptr->nuser < 5))
		    continue;
		addnewbrdstat(n, state);
	    }
#ifdef CRITICAL_MEMORY
	    byMALLOC = 1;
#else
	    byMALLOC = 0;
#endif
	    needREALLOC = 1;
	}
	if (class_bid == -1)
	    qsort(nbrd, brdnum, sizeof(boardstat_t), cmpboardfriends);

    } else {
	int     childcount = bptr->childcount;
	nbrd = (boardstat_t *) malloc(childcount * sizeof(boardstat_t));
	for (bptr = bptr->firstchild[type]; bptr != NULL;
	     bptr = bptr->next[type]) {
	    n = (int)(bptr - bcache);
	    if (!((state = Ben_Perm(bptr)) || (currmode & MODE_MENU))
		|| (yank_flag == 0 && !(getbrdattr(n) & BRD_FAV)) ||
		(key[0] && !strcasestr(bptr->title, key)))
		continue;
	    addnewbrdstat(n, state);
	}
	byMALLOC = 0;
	needREALLOC = (childcount != brdnum);
    }

    if( needREALLOC && brdnum > 0){
	if( byMALLOC ){
	    boardstat_t *newnbrd;
	    newnbrd = (boardstat_t *)malloc(sizeof(boardstat_t) * brdnum);
	    memcpy(newnbrd, nbrd, sizeof(boardstat_t) * brdnum);
	    FREE(nbrd);
	    nbrd = newnbrd;
	}
	else {
	    nbrd = (boardstat_t *)realloc(nbrd, sizeof(boardstat_t) * brdnum);
	}
    }
}

static int
search_board()
{
    int             num;
    char            genbuf[IDLEN + 2];
    move(0, 0);
    clrtoeol();
    CreateNameList();
    for (num = 0; num < brdnum; num++)
	if (nbrd[num].myattr & BRD_BOARD)
	    AddNameList(B_BH(&nbrd[num])->brdname);
    namecomplete(MSG_SELECT_BOARD, genbuf);
    FreeNameList();
    toplev = NULL;

    for (num = 0; num < brdnum; num++)
	if (!strcasecmp(B_BH(&nbrd[num])->brdname, genbuf))
	    return num;
    return -1;
}

static int
unread_position(char *dirfile, boardstat_t * ptr)
{
    fileheader_t    fh;
    char            fname[FNLEN];
    register int    num, fd, step, total;

    total = B_TOTAL(ptr);
    num = total + 1;
    if ((ptr->myattr & BRD_UNREAD) && (fd = open(dirfile, O_RDWR)) > 0) {
	if (!brc_initial(B_BH(ptr)->brdname)) {
	    num = 1;
	} else {
	    num = total - 1;
	    step = 4;
	    while (num > 0) {
		lseek(fd, (off_t) (num * sizeof(fh)), SEEK_SET);
		if (read(fd, fname, FNLEN) <= 0 ||
		    !brc_unread(fname, brc_num, brc_list))
		    break;
		num -= step;
		if (step < 32)
		    step += step >> 1;
	    }
	    if (num < 0)
		num = 0;
	    while (num < total) {
		lseek(fd, (off_t) (num * sizeof(fh)), SEEK_SET);
		if (read(fd, fname, FNLEN) <= 0 ||
		    brc_unread(fname, brc_num, brc_list))
		    break;
		num++;
	    }
	}
	close(fd);
    }
    if (num < 0)
	num = 0;
    return num;
}

static void
brdlist_foot()
{
    prints("\033[34;46m  選擇看板  \033[31;47m  (c)\033[30m新文章模式  "
	   "\033[31m(v/V)\033[30m標記已讀/未讀  \033[31m(y)\033[30m篩選%s"
	   "  \033[31m(m)\033[30m切換最愛  \033[m",
	   yank_flag == 0 ? "最愛" : yank_flag == 1 ? "部份" : "全部");
}

static void
show_brdlist(int head, int clsflag, int newflag)
{
    int             myrow = 2;
    if (class_bid == 1) {
	currstat = CLASS;
	myrow = 6;
	showtitle("分類看板", BBSName);
	movie(0);
	move(1, 0);
	outs(
	    "                                                              "
	    "◣  ╭—\033[33m●\n"
	    "                                                    �寣X  \033[m "
	    "◢█\033[47m☉\033[40m██◣�蔌n"
	    "  \033[44m   ︿︿︿︿︿︿︿︿                               "
	    "\033[33m�鱋033[m\033[44m ◣◢███▼▼▼�� \033[m\n"
	    "  \033[44m                                                  "
	    "\033[33m  \033[m\033[44m ◤◥███▲▲▲ �鱋033[m\n"
	    "                                  ︿︿︿︿︿︿︿︿    \033[33m"
	    "│\033[m   ◥████◤ �鱋n"
	    "                                                      \033[33m��"
	    "——\033[m  ◤      —＋\033[m");
    } else if (clsflag) {
	showtitle("看板列表", BBSName);
	prints("[←]主選單 [→]閱\讀 [↑↓]選擇 [y]載入 [S]排序 [/]搜尋 "
	       "[TAB]文摘•看板 [h]求助\n"
	       "\033[7m%-20s 類別 轉信%-31s人氣 板    主     \033[m",
	       newflag ? "總數 未讀 看  板" : "  編號  看  板",
	       "  中   文   敘   述");
	move(b_lines, 0);
	brdlist_foot();
    }
    if (brdnum > 0) {
	boardstat_t    *ptr;
	char    *color[8] = {"", "\033[32m",
	    "\033[33m", "\033[36m", "\033[34m", "\033[1m",
	"\033[1;32m", "\033[1;33m"};
	char    *unread[2] = {"\33[37m  \033[m", "\033[1;31mˇ\033[m"};

	if (yank_flag == 0 && nbrd[0].myattr == 0){
	    move(3, 0);
	    prints("        --- 空目錄 ---");
	    return;
	}

	while (++myrow < b_lines) {
	    move(myrow, 0);
	    clrtoeol();
	    if (head < brdnum) {
		ptr = &nbrd[head++];
		if (ptr->myattr & BRD_LINE){
		    prints("%5d %c %s------------      ------------------------------------------\033[m", head, ptr->myattr & BRD_TAG ? 'D' : ' ', ptr->myattr & BRD_FAV ? "" : "\033[1;30m");
		    continue;
		}
		else if (ptr->myattr & BRD_FOLDER){
		    char *title = get_folder_title(ptr->bid);
		    prints("%5d %c %sMyFavFolder\033[m  目錄 □%-34s\033[m",
			    head,
			    ptr->myattr & BRD_TAG ? 'D' : ' ',
			    !(cuser.uflag2 & FAVNOHILIGHT) ? "\033[1;36m" : "",
			    title);
		    continue;
		}

		if (class_bid == 1)
		    prints("          ");
		if (!newflag) {
		    prints("%5d%c%s", head,
			   !(B_BH(ptr)->brdattr & BRD_HIDE) ? ' ' :
			   (B_BH(ptr)->brdattr & BRD_POSTMASK) ? ')' : '-',
			   (ptr->myattr & BRD_TAG) ? "D " :
			   (B_BH(ptr)->brdattr & BRD_GROUPBOARD) ? "  " :
			   unread[ptr->myattr & BRD_UNREAD ? 1 : 0]);
		} else {
		    if (newflag) {
			if ((B_BH(ptr)->brdattr & BRD_GROUPBOARD) || ptr->myattr & BRD_FOLDER)
			    prints("        ");
			else
			    prints("%6d%s", (int)(B_TOTAL(ptr)),
				    unread[ptr->myattr & BRD_UNREAD]);
		    }
		}
		if (class_bid != 1) {
		    prints("%s%-13s\033[m%s%5.5s\033[0;37m%2.2s\033[m"
			   "%-34.34s",
			   ((!(cuser.uflag2 & FAVNOHILIGHT) &&
			     getboard(ptr->bid) != NULL))? "\033[1;36m" : "",
			    B_BH(ptr)->brdname,
			    color[(unsigned int)
			    (B_BH(ptr)->title[1] + B_BH(ptr)->title[2] +
			     B_BH(ptr)->title[3] + B_BH(ptr)->title[0]) & 07],
			    B_BH(ptr)->title, B_BH(ptr)->title + 5, B_BH(ptr)->title + 7);

		    if (B_BH(ptr)->brdattr & BRD_BAD)
			prints(" X ");
		    else if (B_BH(ptr)->nuser >= 100)
			prints("\033[1mHOT\033[m");
		    else if (B_BH(ptr)->nuser > 50)
			prints("\033[1;31m%2d\033[m ", B_BH(ptr)->nuser);
		    else if (B_BH(ptr)->nuser > 10)
			prints("\033[1;33m%2d\033[m ", B_BH(ptr)->nuser);
		    else if (B_BH(ptr)->nuser > 0)
			prints("%2d ", B_BH(ptr)->nuser);
		    else
			prints(" %c ", B_BH(ptr)->bvote ? 'V' : ' ');
		    prints("%.*s\033[K", t_columns - 67, B_BH(ptr)->BM);
		} else {
		    prints("%-40.40s %.*s", B_BH(ptr)->title + 7,
			   t_columns - 67, B_BH(ptr)->BM);
		}
	    }
	    clrtoeol();
	}
    }
}

static char    *choosebrdhelp[] = {
    "\0看板選單輔助說明",
    "\01基本指令",
    "(p)(↑)/(n)(↓)上一個看板 / 下一個看板",
    "(P)(^B)(PgUp)  上一頁看板",
    "(N)(^F)(PgDn)  下一頁看板",
    "($)/(s)/(/)    最後一個看板 / 搜尋看板 / 以中文搜尋看板關鍵字",
    "(數字)         跳至該項目",
    "\01進階指令",
    "(^W)           迷路了 我在哪裡",
    "(r)(→)/(q)(←)進入多功\能閱\讀選單 / 回到主選單",
    "(y/Z)          我的最愛,訂閱\看板,所有看板 / 訂閱\新開看板",
    "(g/L)          加入分隔線 / 目錄至我的最愛",
    "(K/T)          備份,清理我的最愛 / 修改我的最愛目錄名稱",
    "(v/V)          通通看完/全部未讀",
    "(S)            按照字母/分類排序",
    "(t/^T/^A/^D)   標記看板/取消所有標記/ 將已標記者加入/移出我的最愛",
    "(m/M)          把看板加入或移出我的最愛,移除分隔線/搬移我的最愛",
    "\01小組長指令",
    "(E/W/B)        設定看板/設定小組備忘/開新看板",
    "(^P)           移動已標記看板到此分類",
    NULL
};


static void
set_menu_BM(char *BM)
{
    if (HAS_PERM(PERM_ALLBOARD) || is_BM(BM)) {
	currmode |= MODE_MENU;
	cuser.userlevel |= PERM_SYSSUBOP;
    }
}

static char    *privateboard =
"\n\n\n\n         對不起 此板目前只准看板好友進入  請先向板主申請入境許\可";

inline static void cursor_set(short int *num, char yank_flag, short int step)
{
    (*num) = step;
    if (yank_flag == 0 && brdnum > 0)
	fav_cursor_set(step);
}

inline static void cursor_down(short int *num, char yank_flag)
{
    (*num)++;
    if (yank_flag == 0 && brdnum > 0)
	fav_cursor_down();
}

inline static void cursor_up(short int *num, char yank_flag)
{
    (*num)--;
    if (yank_flag == 0 && brdnum > 0)
	fav_cursor_up();
}

inline static void cursor_down_step(short int *num, char yank_flag, short int step)
{
    (*num) += step;
    if (yank_flag == 0 && brdnum > 0)
	fav_cursor_down_step(step);
}

inline static void cursor_up_step(short int *num, char yank_flag, short int step)
{
    (*num) -= step;
    if (yank_flag == 0 && brdnum > 0)
	fav_cursor_up_step(step);
}

static void
choose_board(int newflag)
{
    static short    num = 0;
    boardstat_t    *ptr;
    int             head = -1, ch = 0, currmodetmp, tmp, tmp1, bidtmp;
    char            keyword[13] = "";

    setutmpmode(newflag ? READNEW : READBRD);
    if( get_current_fav() == NULL )
	load_brdbuf();
    ++choose_board_depth;
    brdnum = 0;
    if (!cuser.userlevel)	/* guest yank all boards */
	yank_flag = 2;

    do {
	if (brdnum <= 0) {
	    if (yank_flag == 0 && brdnum > 0)
		fav_cursor_set(num);
	    load_boards(keyword);
	    if (brdnum <= 0 && yank_flag > 0) {
		if (keyword[0] != 0) {
		    mprints(b_lines - 1, 0, "沒有任何看板標題有此關鍵字 "
			    "(板主應注意看板標題命名)");
		    pressanykey();
		    keyword[0] = 0;
		    brdnum = -1;
		    continue;
		}
		if (yank_flag < 2) {
		    brdnum = -1;
		    yank_flag++;
		    continue;
		}
		if (HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU)) {
		    if (m_newbrd(0) == -1)
			break;
		    brdnum = -1;
		    continue;
		} else
		    break;
	    }
	    head = -1;
	}

	/* reset the cursor when out of range */
	if (num < 0)
	    cursor_set(&num, yank_flag, 0);
	else if (num >= brdnum){
	    cursor_set(&num, yank_flag, brdnum - 1);
	}

	if (head < 0) {
	    if (newflag) {
		tmp = num;
		while (num < brdnum) {
		    ptr = &nbrd[num];
		    if (ptr->myattr & BRD_UNREAD)
			break;
		    cursor_down(&num, yank_flag);
		}
		if (num >= brdnum){
		    cursor_set(&num, yank_flag, tmp);
		}
	    }
	    head = (num / p_lines) * p_lines;
	    show_brdlist(head, 1, newflag);
	} else if (num < head || num >= head + p_lines) {
	    head = (num / p_lines) * p_lines;
	    show_brdlist(head, 0, newflag);
	}
	if (class_bid == 1)
	    ch = cursor_key(7 + num - head, 10);
	else
	    ch = cursor_key(3 + num - head, 0);

	switch (ch) {
	case Ctrl('W'):
	    whereami(0, NULL, NULL);
	    head = -1;
	    break;
	case 'e':
	case KEY_LEFT:
	case EOF:
	    ch = 'q';
	case 'q':
	    if (keyword[0]) {
		keyword[0] = 0;
		brdnum = -1;
		ch = ' ';
	    }
	    break;
	case 'c':
	    show_brdlist(head, 1, newflag ^= 1);
	    break;
	case KEY_PGUP:
	case 'P':
	case 'b':
	case Ctrl('B'):
	    if (num) {
		cursor_up_step(&num, yank_flag, p_lines);
		break;
	    }
	case KEY_END:
	case '$':
	    cursor_set(&num, yank_flag, brdnum - 1);
	    break;
	case ' ':
	case KEY_PGDN:
	case 'N':
	case Ctrl('F'):
	    if (num == brdnum - 1){
		cursor_set(&num, yank_flag, 0);
	    }
	    else{
		cursor_down_step(&num, yank_flag, p_lines);
	    }
	    break;
	case Ctrl('C'):
	    cal();
	    show_brdlist(head, 1, newflag);
	    break;
	case Ctrl('I'):
	    t_idle();
	    show_brdlist(head, 1, newflag);
	    break;
	case KEY_UP:
	case 'p':
	case 'k':
	    cursor_up(&num, yank_flag);
	    if (num < 0)
		cursor_set(&num, yank_flag, brdnum - 1);
	    break;
	case 't':
	    ptr = &nbrd[num];
	    if (yank_flag == 0 && get_data_number(get_current_fav()) > 0)
		fav_tag_current(2);
	    ptr->myattr ^= BRD_TAG;
	    head = 9999;
	case KEY_DOWN:
	case 'n':
	case 'j':
	    cursor_down(&num, yank_flag);
	    if (num < brdnum)
		break;
	case '0':
	case KEY_HOME:
	    cursor_set(&num, yank_flag, 0);
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
	    if ((tmp = search_num(ch, brdnum)) >= 0){
		cursor_set(&num, yank_flag, tmp);
	    }
	    brdlist_foot();
	    break;
	case 'F':
	case 'f':
	    if (class_bid>0 && HAS_PERM(PERM_SYSOP)) {
		bcache[class_bid - 1].firstchild[cuser.uflag & BRDSORT_FLAG ? 1 : 0]
		    = NULL;
		brdnum = -1;
	    }
	    break;
	case 'h':
	    show_help(choosebrdhelp);
	    show_brdlist(head, 1, newflag);
	    break;
	case '/':
	    getdata_buf(b_lines - 1, 0, "請輸入看板中文關鍵字:",
			keyword, sizeof(keyword), DOECHO);
	    brdnum = -1;
	    break;
	case 'S':
	    if(yank_flag == 0){
		char    input[4];
		move(b_lines - 2, 0);
		prints("重新排序看板 "
			"\033[1;33m(注意, 這個動作會覆寫原來設定)\033[m \n");
		getdata(b_lines - 1, 0,
			"排序方式 (1)按照板名排序 (2)按照類別排序 ==> [0]取消 ",
			input, sizeof(input), DOECHO);
		if( input[0] == '1' )
		    fav_sort_by_name();
		else if( input[0] == '2' )
		    fav_sort_by_class();
	    }
	    else
		cuser.uflag ^= BRDSORT_FLAG;
	    brdnum = -1;
	    break;
	case 'y':
	    yank_flag = (yank_flag + 1) % 2;
	    brdnum = -1;
	    break;
	case Ctrl('D'):
	    fav_remove_all_tagged_item();
	    brdnum = -1;
	    break;
	case Ctrl('A'):
	    fav_add_all_tagged_item();
	    brdnum = -1;
	    break;
	case Ctrl('T'):
	    fav_remove_all_tag();
	    brdnum = -1;
	    break;
	case Ctrl('P'):
	    if (class_bid != 0 &&
		(HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU))) {
		for (tmp = 0; tmp < brdnum; tmp++) {
		    short   bid = nbrd[tmp].bid;
		    boardheader_t  *bh = &bcache[ bid - 1 ];
		    /*
		    if (!(fav->b[tmp].attr & BRD_TAG) || bh->gid == class_bid)
			continue;
		    */
		    if( !(nbrd[tmp].myattr & BRD_TAG) )
			continue;
		    nbrd[tmp].myattr &= ~BRD_TAG;
		    if (bh->gid != class_bid) {
			bh->gid = class_bid;
			substitute_record(FN_BOARD, bh,
					  sizeof(boardheader_t), bid);
			reset_board(bid);
			log_usies("SetBoardGID", bh->brdname);
		    }
		}
		brdnum = -1;
	    }
	    break;
	case 'L':
	    if (HAS_PERM(PERM_BASIC)) {
		if (fav_add_line() == NULL) {
		    vmsg("新增失敗，分隔線/總最愛 數量達最大值。");
		    break;
		}
		/* done move if it's the first item. */
		if (get_data_number(get_current_fav()) > 1)
		    move_in_current_folder(brdnum, num);
		brdnum = -1;
		head = 9999;
	    }
	    break;
	case 'm':
	    if (HAS_PERM(PERM_BASIC)) {
		if (yank_flag == 0) {
		    if (num > brdnum)
			break;
		    if (nbrd[num].myattr & BRD_FAV && getans("你確定刪除嗎? [N/y]") == 'y'){
			fav_remove_current();

			/* nessesery ? */
			if (get_current_fav_level() == 1 &&
			    get_data_number(get_current_fav()) == 0)
			    ch = 'q';

			nbrd[num].myattr &= ~BRD_FAV;
		    }
		}
		else{
		    if (nbrd[num].myattr & BRD_FAV){
			fav_remove_current();
			nbrd[num].myattr &= ~BRD_FAV;
		    }
		    else{
			if (fav_add_board(nbrd[num].bid) == NULL)
			    vmsg("你的最愛太多了啦 真花心");
			else
			    nbrd[num].myattr |= BRD_FAV;
		    }
		}
		brdnum = -1;
		head = 9999;
	    }
	    break;
	case 'M':
	    if (HAS_PERM(PERM_BASIC)){
		if (class_bid == 0 && yank_flag == 0){
		    imovefav(num);
		    brdnum = -1;
		    head = 9999;
		}
	    }
	    break;
	case 'g':
	    if (HAS_PERM(PERM_BASIC)) {
		fav_type_t  *ft;
		if (fav_max_folder_level()){
		    vmsg("目錄已達最大層數!!");
		    break;
		}
		if ((ft = fav_add_folder()) == NULL) {
		    vmsg("新增失敗，目錄/總最愛 數量達最大值。");
		    break;
		}
		fav_set_folder_title(ft, "新的目錄");
		/* done move if it's the first item. */
		if (get_data_number(get_current_fav()) > 1)
		    move_in_current_folder(brdnum, num);
		brdnum = -1;
    		head = 9999;
	    }
	    break;
	case 'T':
	    if (HAS_PERM(PERM_BASIC)) {
		char title[64];
		fav_type_t *ft = get_current_entry();
		if (get_item_type(ft) != FAVT_FOLDER)
		    break;
		strlcpy(title, get_item_title(ft), sizeof(title));
		getdata_buf(b_lines - 1, 0, "請輸入檔名:", title, sizeof(title), DOECHO);
		fav_set_folder_title(ft, title);
		brdnum = -1;
	    }
	    break;
	case 'K':
	    if (HAS_PERM(PERM_BASIC)) {
		char c, fname[80], genbuf[256];
		int fd;
		c = getans("請選擇 1)清除不可見看板 2)備份我的最愛 3)取回最愛備份 [Q]");
		if(!c)
		    break;
		if(getans("確定嗎 [y/N] ") != 'y')
		    break;
		switch(c){
		    case '1':
			cleanup();
			break;
		    case '2':
			fav_save();
			setuserfile(fname, FAV4);
			sprintf(genbuf, "cp -f %s %s.bak", fname, fname);
			system(genbuf);
			break;
		    case '3':
			setuserfile(fname, FAV4);
			sprintf(genbuf, "%s.bak", fname);
			if((fd = open(genbuf, O_RDONLY)) < 0){
			    vmsg("你沒有備份你的最愛喔");
			    break;
			}
			close(fd);
			sprintf(genbuf, "cp -f %s.bak %s", fname, fname);
			system(genbuf);
			fav_free();
			load_brdbuf();
			break;
		}
		brdnum = -1;
	    }
	    break;
	case 'z':
	    //vmsg("嘿嘿 這個功\能已經被我的最愛取代掉了喔!");
	    break;
	case 'Z':
	    if (!HAS_PERM(PERM_BASIC))
		break;
	    cuser.uflag2 ^= FAVNEW_FLAG;
	    if(cuser.uflag2 & FAVNEW_FLAG){
		char fname[80];

		setuserfile(fname, FAVNB);

		if( (tmp = open(fname, O_RDONLY, 0600)) != -1 ){
		    close(tmp);
		    updatenewfav(0);
		}
		else{
		    char stat;
		    if( (tmp = open(fname, O_WRONLY | O_CREAT, 0600)) != -1 ){
			for(tmp1 = 0; tmp1 < numboards; tmp1++){
			    if(bcache[tmp1].brdname[0] && Ben_Perm(&bcache[tmp1]))
				stat = BRD_OLD;
			    else
				stat = BRD_NEW;
			    write(tmp, &stat, sizeof(char));
			}
			stat = BRD_END;
			write(tmp, &stat, sizeof(char));
			close(tmp);
		    }
		}
	    }
	    vmsg((cuser.uflag2 & FAVNEW_FLAG) ? "切換為訂閱\新看板模式" : "切換為正常模式");
	    break;
	case 'v':
	case 'V':
	    if(nbrd[num].bid < 0)
		break;
	    ptr = &nbrd[num];
	    brc_initial(B_BH(ptr)->brdname);
	    if (ch == 'v') {
		ptr->myattr &= ~BRD_UNREAD;
		brc_list[0] = now;
		setbrdtime(ptr->bid, now);
	    } else {
		brc_list[0] = 1;
		setbrdtime(ptr->bid, 1);
		ptr->myattr |= BRD_UNREAD;
	    }
	    brc_num = brc_changed = 1;
	    brc_update();
	    show_brdlist(head, 0, newflag);
	    break;
	case 's':
	    if ((tmp = search_board()) == -1) {
		show_brdlist(head, 1, newflag);
		break;
	    }
	    cursor_set(&num, yank_flag, tmp);
	    break;
	case 'E':
	    if (HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU)) {
		ptr = &nbrd[num];
		move(1, 1);
		clrtobot();
		m_mod_board(B_BH(ptr)->brdname);
		brdnum = -1;
	    }
	    break;
	case 'R':
	    if (HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU)) {
		m_newbrd(1);
		brdnum = -1;
	    }
	    break;
	case 'B':
	    if (HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU)) {
		m_newbrd(0);
		brdnum = -1;
	    }
	    break;
	case 'W':
	    if (class_bid > 0 &&
		(HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU))) {
		char            buf[128];
		setbpath(buf, bcache[class_bid - 1].brdname);
		mkdir(buf, 0755);	/* Ptt:開群組目錄 */
		b_note_edit_bname(class_bid);
		brdnum = -1;
	    }
	    break;
#ifdef DEBUG
	case 'A': {
	    char buf[128];
	    fav_type_t *ft = get_current_entry();
	    fav_t *fp = get_current_fav();
	    sprintf(buf, "d: %d b: %d f: %d bn: %d num: %d t: %d, id: %d", fp->DataTail, fp->nBoards, fp->nFolders, brdnum, num, ft->type, fav_getid(ft));
	    vmsg(buf);
		  }
	    break;
#endif
	case KEY_RIGHT:
	case '\n':
	case '\r':
	case 'r':
	    {
		char            buf[STRLEN];

		ptr = &nbrd[num];
		if (ptr->myattr & BRD_LINE)
		    break;
		else if (ptr->myattr & BRD_FOLDER){
		    int t = num;
		    fav_folder_in();
		    choose_board(0);
		    fav_folder_out();
		    cursor_set(&num, yank_flag, t);
		    brdnum = -1;
		    head = 9999;
		    break;
		}

		if (!(B_BH(ptr)->brdattr & BRD_GROUPBOARD)) {	/* 非sub class */
		    if (!(B_BH(ptr)->brdattr & BRD_HIDE) ||
			(B_BH(ptr)->brdattr & BRD_POSTMASK)) {
			brc_initial(B_BH(ptr)->brdname);

			if (newflag) {
			    setbdir(buf, currboard);
			    tmp = unread_position(buf, ptr);
			    head = tmp - t_lines / 2;
			    getkeep(buf, head > 1 ? head : 1, tmp + 1);
			}
			board_visit_time = getbrdtime(ptr->bid);
			Read();
			check_newpost(ptr);
			head = -1;
			setutmpmode(newflag ? READNEW : READBRD);
		    } else {
			setbfile(buf, B_BH(ptr)->brdname, FN_APPLICATION);
			if (more(buf, YEA) == -1) {
			    move(1, 0);
			    clrtobot();
			    outs(privateboard);
			    pressanykey();
			}
			head = -1;
		    }
		} else {	/* sub class */
		    move(12, 1);
		    bidtmp = class_bid;
		    currmodetmp = currmode;
		    tmp1 = num;
		    cursor_set(&num, yank_flag, 0);
		    if (!(B_BH(ptr)->brdattr & BRD_TOP))
			class_bid = ptr->bid;
		    else
			class_bid = -1;	/* 熱門群組用 */

		    if (!(currmode & MODE_MENU))	/* 如果還沒有小組長權限 */
			set_menu_BM(B_BH(ptr)->BM);

		    if (now < B_BH(ptr)->bupdate) {
			setbfile(buf, B_BH(ptr)->brdname, fn_notes);
			if (more(buf, NA) != -1)
			    pressanykey();
		    }
		    tmp = currutmp->brc_id;
		    setutmpbid(ptr->bid);
		    free(nbrd);
		    nbrd = NULL;
	    	    if (yank_flag == 0 ) {
    			yank_flag = 1;
			choose_board(0);
			yank_flag = 0;
    		    }
		    else
			choose_board(0);
		    currmode = currmodetmp;	/* 離開板板後就把權限拿掉喔 */
		    cursor_set(&num, yank_flag, tmp1);
		    class_bid = bidtmp;
		    setutmpbid(tmp);
		    brdnum = -1;
		}
	    }
	}
    } while (ch != 'q');
    free(nbrd);
    nbrd = NULL;
    --choose_board_depth;
}

int
root_board()
{
    class_bid = 1;
    yank_flag = 1;
    choose_board(0);
    return 0;
}

int
Boards()
{
    class_bid = 0;
    yank_flag = 0;
    choose_board(0);
    return 0;
}


int
New()
{
    int             mode0 = currutmp->mode;
    int             stat0 = currstat;

    class_bid = 0;
    choose_board(1);
    currutmp->mode = mode0;
    currstat = stat0;
    return 0;
}

/*
 * int v_favorite(){ char fname[256]; char inbuf[2048]; FILE* fp; int nGroup;
 * char* strtmp;
 * 
 * setuserfile(fname,str_favorite);
 * 
 * if (!(fp=fopen(fname,"r"))) return -1; move(0,0); clrtobot();
 * fgets(inbuf,sizeof(inbuf),fp); nGroup=atoi(inbuf);
 * 
 * currutmp->nGroup=0; currutmp->ninRoot=0;
 * 
 * while(nGroup!=currutmp->nGroup+1){ fgets(inbuf,sizeof(inbuf),fp);
 * prints("%s\n",strtmp=strtok(inbuf," \n"));
 * strcpy(currutmp->gfavorite[currutmp->nGroup++],strtmp);
 * while((strtmp=strtok(NULL, " \n"))){ prints("     %s
 * %d\n",strtmp,getbnum(strtmp)); } currutmp->nGroup++; }
 * prints("+++%d+++\n",currutmp->nGroup);
 * 
 * fgets(inbuf,sizeof(inbuf),fp);
 * 
 * for(strtmp=strtok(inbuf, " \n");strtmp;strtmp=strtok(NULL, " \n")){ if
 * (strtmp[0]!='#') prints("*** %s %d\n",strtmp, getbnum(strtmp)); else
 * prints("*** %s %d\n",strtmp+1, -1); currutmp->ninRoot++; }
 * 
 * fclose(fp); pressanykey(); return 0; }
 */
