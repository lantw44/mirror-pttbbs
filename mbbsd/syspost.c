/* $Id$ */
#include "bbs.h"

int
post_msg(char *bname, char *title, char *msg, char *author)
{
    FILE           *fp;
    int             bid;
    fileheader_t    fhdr;
    char            genbuf[256];

    /* 在 bname 板發表新文章 */
    snprintf(genbuf, sizeof(genbuf), "boards/%c/%s", bname[0], bname);
    stampfile(genbuf, &fhdr);
    fp = fopen(genbuf, "w");

    if (!fp)
	return -1;

    fprintf(fp, gettext[2551], author, bname, title);
    fprintf(fp, gettext[2552], ctime(&now));

    /* 文章的內容 */
    fprintf(fp, "%s", msg);
    fclose(fp);

    /* 將檔案加入列表 */
    strlcpy(fhdr.title, title, sizeof(fhdr.title));
    strlcpy(fhdr.owner, author, sizeof(fhdr.owner));
    setbdir(genbuf, bname);
    if (append_record(genbuf, &fhdr, sizeof(fhdr)) != -1)
	if ((bid = getbnum(bname)) > 0)
	    setbtotal(bid);
    return 0;
}

int
post_file(char *bname, char *title, char *filename, char *author)
{
    int             size = dashs(filename);
    char           *msg;
    FILE           *fp;

    if (size <= 0)
	return -1;
    if (!(fp = fopen(filename, "r")))
	return -1;
    msg = (char *)malloc(size + 1);
    size = fread(msg, 1, size, fp);
    msg[size] = 0;
    size = post_msg(bname, title, msg, author);
    fclose(fp);
    free(msg);
    return size;
}

void
post_change_perm(int oldperm, int newperm, char *sysopid, char *userid)
{
    FILE           *fp;
    fileheader_t    fhdr;
    char            genbuf[200], reason[30];
    int             i, flag = 0;

    strlcpy(genbuf, "boards/S/Security", sizeof(genbuf));
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
	return;

    fprintf(fp, gettext[2553], ctime(&now));
    for (i = 5; i < NUMPERMS; i++) {
	if (((oldperm >> i) & 1) != ((newperm >> i) & 1)) {
	    fprintf(fp, gettext[2554],
		    sysopid,
	       (((oldperm >> i) & 1) ? gettext[2555] : gettext[2556]),
		    userid, gettext[str_permid[i]]);
	    flag++;
	}
    }

    if (flag) {
	clrtobot();
	clear();
	while (!getdata_str(5, 0, gettext[2557],
			    reason, sizeof(reason), DOECHO, gettext[2558]));
	fprintf(fp, gettext[2559],
		cuser.userid, reason);
	fclose(fp);

	snprintf(fhdr.title, sizeof(fhdr.title),
		 gettext[2560],
		 cuser.userid, userid);
	strlcpy(fhdr.owner, gettext[2561], sizeof(fhdr.owner));
	append_record("boards/S/Security/.DIR", &fhdr, sizeof(fhdr));
    } else
	fclose(fp);
}

void
post_violatelaw(char *crime, char *police, char *reason, char *result)
{
    char            genbuf[200];
    fileheader_t    fhdr;
    FILE           *fp;
/*
    strlcpy(genbuf, "boards/S/Security", sizeof(genbuf));
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
	return;
    fprintf(fp, "作者: [Ptt法院] 看板: Security\n"
	    "標題: [報告] %-20s 違法判決報告\n"
	    "時間: %s\n"
	    "\033[1;32m%s\033[m判決：\n     \033[1;32m%s\033[m"
	    "因\033[1;35m%s\033[m行為，\n違反本站站規，處以\033[1;35m%s\033[m，特此公告",
	    crime, ctime(&now), police, crime, reason, result);
    fclose(fp);
    snprintf(fhdr.title, sizeof(fhdr.title),
	     "[報告] %-20s 違法判決報告", crime);
    strlcpy(fhdr.owner, "[Ptt法院]", sizeof(fhdr.owner));
    append_record("boards/S/Security/.DIR", &fhdr, sizeof(fhdr));

*/
    strlcpy(genbuf, "boards/V/ViolateLaw", sizeof(genbuf));
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
	return;
    fprintf(fp, gettext[2562],
	    crime, ctime(&now), police, crime, reason, result);
    fclose(fp);
    snprintf(fhdr.title, sizeof(fhdr.title),
	     gettext[2563], crime);
    strlcpy(fhdr.owner, gettext[2564], sizeof(fhdr.owner));

    append_record("boards/V/ViolateLaw/.DIR", &fhdr, sizeof(fhdr));

}

void
post_newboard(char *bgroup, char *bname, char *bms)
{
    char            genbuf[256], title[128];
    snprintf(title, sizeof(title), gettext[2565], bname);
    snprintf(genbuf, sizeof(genbuf),
	     gettext[2566],
	     cuser.userid, bname, bgroup, bms);
    post_msg("Record", title, genbuf, gettext[2567]);
}

void
give_money_post(char *userid, int money)
{
    FILE           *fp;
    fileheader_t    fhdr;
    time_t          now = time(0);
    char            genbuf[200];

    strlcpy(genbuf, "boards/S/Security", sizeof(genbuf));
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
	return;
    fprintf(fp, gettext[2568], cuser.userid, ctime(&now));
    clrtobot();
    clear();
    fprintf(fp, gettext[2569],
	    cuser.userid, userid, money);

    fclose(fp);
    snprintf(fhdr.title, sizeof(fhdr.title), gettext[2570],
	     cuser.userid);
    strlcpy(fhdr.owner, gettext[2571], sizeof(fhdr.owner));
    append_record("boards/S/Security/.DIR", &fhdr, sizeof(fhdr));
}
