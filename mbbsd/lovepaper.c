/* $Id$ */
#include "bbs.h"
#define DATA "etc/lovepaper.dat"

int
x_love()
{
    char            buf1[200], save_title[TTLEN + 1];
    char            receiver[61], path[STRLEN] = "home/";
    int             x, y = 0, tline = 0, poem = 0;
    FILE           *fp, *fpo;
    struct tm      *gtime;
    fileheader_t    mhdr;

    setutmpmode(LOVE);
    gtime = localtime(&now);
    snprintf(buf1, sizeof(buf1), "%c/%s/love%d%d",
	    cuser.userid[0], cuser.userid, gtime->tm_sec, gtime->tm_min);
    strcat(path, buf1);
    move(1, 0);
    clrtobot();

    outs(SHM->i18nstr[cuser.language][1195]);
    outs(SHM->i18nstr[cuser.language][1196]);

    if (!getdata(7, 0, SHM->i18nstr[cuser.language][1197], receiver, sizeof(receiver), DOECHO))
	return 0;
    if (receiver[0] && !(searchuser(receiver) &&
			 getdata(8, 0, SHM->i18nstr[cuser.language][1198], save_title,
				 sizeof(save_title), DOECHO))) {
	move(10, 0);
	outs(SHM->i18nstr[cuser.language][1199]);
	pressanykey();
	return 0;
    }
    fpo = fopen(path, "w");
    assert(fpo);
    fprintf(fpo, "\n");
    if ((fp = fopen(DATA, "r"))) {
	while (fgets(buf1, 100, fp)) {
	    switch (buf1[0]) {
	    case '#':
		break;
	    case '@':
		if (!strncmp(buf1, "@begin", 6) || !strncmp(buf1, "@end", 4))
		    tline = 3;
		else if (!strncmp(buf1, "@poem", 5)) {
		    poem = 1;
		    tline = 1;
		    fprintf(fpo, "\n\n");
		} else
		    tline = 2;
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
		sscanf(buf1, "%d", &x);
		y = (rand() % (x - 1)) * tline;
		break;
	    default:
		if (!poem) {
		    if (y > 0)
			y = y - 1;
		    else {
			if (tline > 0) {
			    fprintf(fpo, "%s", buf1);
			    tline--;
			}
		    }
		} else {
		    if (buf1[0] == '$')
			y--;
		    else if (y == 0)
			fprintf(fpo, "%s", buf1);
		}
	    }

	}

	fclose(fp);
	fclose(fpo);
	if (vedit(path, YEA, NULL) == -1) {
	    unlink(path);
	    clear();
	    outs(SHM->i18nstr[cuser.language][1200]);
	    pressanykey();
	    return -2;
	}
	sethomepath(buf1, receiver);
	stampfile(buf1, &mhdr);
	Rename(path, buf1);
	strncpy(mhdr.title, save_title, TTLEN);
	strlcpy(mhdr.owner, cuser.userid, sizeof(mhdr.owner));
	sethomedir(path, receiver);
	if (append_record(path, &mhdr, sizeof(mhdr)) == -1)
	    return -1;
	hold_mail(buf1, receiver);
	return 1;
    }
    fclose(fpo);
    return 0;
}
