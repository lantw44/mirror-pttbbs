#include "bbs.h"

#ifndef BRC_MAXNUM
#define BRC_MAXSIZE     24576   /* Effective size of brc rc file, 8192 * 3 */
#define BRC_ITEMSIZE    ((BRC_MAXNUM + 2) * sizeof( int ))
    /* Maximum size of each record */
#define BRC_MAXNUM      80      /* Upper bound of brc_num, size of brc_list  */
#endif

/* brc rc file form:
 * board_name     15 bytes
 * brc_num         1 byte, binary integer
 * brc_list       brc_num * sizeof(int) bytes, brc_num binary integer(s) */

static time_t   brc_expire_time;
 /* Will be set to the time one year before login. All the files created
  * before then will be recognized read. */

static int      brc_changed = 0;
/* The below two will be filled by read_brc_buf() and brc_update() */
static int     *brc_buf = NULL;
static int      brc_size;

static char * const fn_oldboardrc = ".boardrc";
static char * const fn_brc = ".brc2";

static int    *
brc_getrecord(int *ptr, int *endp, int *bid, int *pnum, int *list)
{
    int  num;
    int *tmp;

    if (ptr + 2 > endp)
	return endp + 1; /* dangling, ignoring it */
    *bid = *ptr++;       /* bid */
    num = *ptr++;          /* brc_num */
    tmp = ptr + num;     /* end of this record */
    if (tmp <= endp){
	memcpy(list, ptr, num * sizeof(int)); /* brc_list */
	if (num > BRC_MAXNUM)
	    num = BRC_MAXNUM;
	*pnum = num;
    }
    return tmp;
}

static int *
brc_putrecord(int *ptr, int *endp, int bid, int num, const int *list)
{
    if (num > 0 && list[0] > brc_expire_time &&
	    ptr + 2 <= endp) {
	if (num > BRC_MAXNUM)
	    num = BRC_MAXNUM;

	while (num > 1 && list[num - 1] < brc_expire_time)
	    num--; /* don't write the times before brc_expire_time */

	if( num == 0 ) return ptr;

	*ptr++ = bid;         /* write in bid */
	*ptr++ = num;         /* write in brc_num */
	if (ptr + num <= endp)
	    memcpy(ptr, list, num * sizeof(int)); /* write in brc_list */
	ptr += num;
    }
    return ptr;
}

#error "Below not changed yet"
static inline void
brc_insert_record(const char* board, int num, int* list)
{
    int            *ptr, *endp, *tmpp = 0;
    char            tmp_buf[BRC_ITEMSIZE];
    char            tmp_name[BRC_STRLEN];
    int             tmp_list[BRC_MAXNUM];
    int             new_size, end_size, tmp_num;
    int             found = 0;

    ptr = brc_buf;
    endp = &brc_buf[brc_size];
    while (ptr < endp && (*ptr >= ' ' && *ptr <= 'z')) {
	/* for each available records */
	tmpp = brc_getrecord(ptr, endp, tmp_name, &tmp_num, tmp_list);

	if ( tmpp > endp ){
	    /* dangling, ignore the trailing data */
	    brc_size = (int)(ptr - brc_buf);
	    break;
	}
	if ( strncmp(tmp_name, board, BRC_STRLEN) == 0 ){
	    found = 1;
	    break;
	}
	ptr = tmpp;
    }

    if( ! found ){
	/* put on the beginning */
	ptr = brc_putrecord(tmp_buf, tmp_buf + BRC_MAXSIZE,
		board, num, list);
	new_size = (int)(ptr - tmp_buf);
	if( new_size ){
	    brc_size += new_size;
	    if ( brc_size > BRC_MAXSIZE )
		brc_size = BRC_MAXSIZE;
	    memmove(brc_buf + new_size, brc_buf, brc_size);
	    memmove(brc_buf, tmp_buf, new_size);
	}
    }else{
	/* ptr points to the old current brc list.
	 * tmpp is the end of it (exclusive).       */
	end_size = endp - tmpp;
	new_size = (int)(brc_putrecord(tmp_buf, tmp_buf + BRC_ITEMSIZE,
		    board, num, list) - tmp_buf);
	if( new_size ){
	    brc_size += new_size - (tmpp - ptr);
	    if ( brc_size > BRC_MAXSIZE ){
		end_size -= brc_size - BRC_MAXSIZE;
		brc_size = BRC_MAXSIZE;
	    }
	    if ( end_size > 0 && ptr + new_size != tmpp )
		memmove(ptr + new_size, tmpp, end_size);
	    memmove(ptr, tmp_buf, new_size);
	}else
	    memmove(ptr, tmpp, brc_size - (tmpp - brc_buf));
    }

    brc_changed = 0;
}

void
brc_update(){
    if (brc_changed && cuser.userlevel && brc_num > 0)
	brc_insert_record(currboard, brc_num, brc_list);
}

inline static void
read_old_brc()
{
    /* XXX: read from old brc file */
}

inline static void
read_brc_buf()
{
    if( brc_buf == NULL ){
	char            dirfile[STRLEN];
	int             fd;

	brc_buf = malloc(BRC_MAXSIZE);
	setuserfile(dirfile, fn_brc);
	if ((fd = open(dirfile, O_RDONLY)) != -1) {
	    brc_size = read(fd, brc_buf, BRC_MAXSIZE);
	    close(fd);
	} else {
	    read_old_brc();
	}
    }
}

void
brc_finalize(){
    char brcfile[STRLEN];
    int fd;
    brc_update();
    setuserfile(brcfile, fn_boardrc);
    if (brc_buf != NULL &&
	(fd = open(brcfile, O_WRONLY | O_CREAT | O_TRUNC, 0644)) != -1) {
	write(fd, brc_buf, brc_size);
	close(fd);
    }
}

void
brc_initialize(){
    static char done = 0;
    if( done )
	return;
    done = 1;
    brc_expire_time = login_start_time - 365 * 86400;
    read_brc_buf();
}

int
brc_read_record(const char* bname, int* num, int* list){
    char            *ptr;
    char            *endp;
    char            tmp_name[BRC_STRLEN];
    ptr = brc_buf;
    endp = &brc_buf[brc_size];
    while (ptr < endp && (*ptr >= ' ' && *ptr <= 'z')) {
	/* for each available records */
	ptr = brc_getrecord(ptr, endp, tmp_name, num, list);
	if (strncmp(tmp_name, bname, BRC_STRLEN) == 0)
	    return *num;
    }
    *num = list[0] = 1;
    return 0;
}

int
brc_initial_board(const char *boardname)
{
    brc_initialize();

    if (strcmp(currboard, boardname) == 0) {
	return brc_num;
    }

    brc_update(); /* write back first */
    currbid = getbnum(boardname);
    currboard = bcache[currbid - 1].brdname;
    currbrdattr = bcache[currbid - 1].brdattr;

    return brc_read_record(boardname, &brc_num, brc_list);
}

void
brc_trunc(const char* brdname, int ftime){
    brc_insert_record(brdname, 1, &ftime);
    if (strncmp(brdname, currboard, BRC_STRLEN) == 0){
	brc_num = 1;
	brc_list[0] = ftime;
	brc_changed = 0;
    }
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
}

int
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
