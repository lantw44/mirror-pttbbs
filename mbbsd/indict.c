/* $Id$ */
#include "bbs.h"

#define REFER "etc/dicts"

static void
addword(char *database,char word[])
{
    char            buf[150], a[3];
    FILE           *fp = fopen(database, "r+");

    if (fp == NULL) {
	vmsg("database error");
	return;
    }
    fgets(buf, 130, fp);
    fseek(fp, 0, 2);
    if (HAVE_PERM(PERM_LOGINOK)) {
	clear();
	move(4, 0);
	outs(SHM->i18nstr[cuser.language][1179]);
	prints(SHM->i18nstr[cuser.language][1180], buf);
	outs(SHM->i18nstr[cuser.language][1181]);
	getdata(10, 0, ":", buf, 65, DOECHO);
	if (buf[0]) {
	    getdata(13, 0, SHM->i18nstr[cuser.language][1182], a, sizeof(a), LCECHO);
	    if (a[0] != 'n')
		fprintf(fp, "%-65s[%s]\n", buf, cuser.userid);
	}
    }
    fclose(fp);
    clear();
}

static int
choose_dict(char *dict,int dictlen,char *database,int databaselen)
{
#define MAX_DICT 10
    int             n,c;
    FILE           *fp;
    char            buf[MAX_DICT][21], data[MAX_DICT][21], cho[10];

    move(12, 0);
    clrtobot();
    outs(SHM->i18nstr[cuser.language][1183]);

    if ((fp = fopen(REFER, "r"))) {
	for(n=0; n<MAX_DICT && fscanf(fp,"%s %s",buf[n],data[n])==2; n++) { // XXX check buffer size
	    prints(SHM->i18nstr[cuser.language][1184], n + 1, buf[n]);
	}
	fclose(fp);

	getdata(22, 14, SHM->i18nstr[cuser.language][1185], cho, 3, LCECHO);
	c=atoi(cho);

	if (c >= 1 && c <= n) {
	    strlcpy(dict, buf[c-1], dictlen);
	    strlcpy(database, data[c-1], databaselen);
	    return 1;
	} else
	    return 0;
    }
    return 0;
}

int
use_dict(char *dict,char *database)
{
    FILE           *fp;
    char            lang[150], word[80] = "";
    char            j, f, buf[120], sys[100];
    int             i = 0;

	strlcpy(sys, SHM->i18nstr[cuser.language][1186], sizeof(sys));
    setutmpmode(DICT);
    if (!HAS_PERM(PERM_SYSOP))
	sys[0] = 0;

    clear();

    snprintf(buf, sizeof(buf),
	     SHM->i18nstr[cuser.language][1187], dict);
    strlcpy(&buf[100], "\033[m\n", sizeof(buf) - 100);
    for (;;) {
	move(0, 0);
	prints(SHM->i18nstr[cuser.language][1188], dict);
	prints(SHM->i18nstr[cuser.language][1189], sys);
	getdata(2, 0, ":", word, 18, DOECHO);
	outs(SHM->i18nstr[cuser.language][1190]);
	str_lower(word, word);
	if (word[0] == 0)
	    return 0;
	clear();
	move(4, 0);
	outs(buf);
	if (strlen(word) == 1) {
	    if (word[0] == 'a') {
		clear();
		move(4, 0);
		outs(buf);
		addword(database,word);
		continue;
	    } else if (word[0] == 't')
		word[0] = 0;
	    else if (word[0] == 'h') {
		more("etc/dict.hlp", YEA);
		clear();
		continue;
	    } else if (word[0] == 'e' && HAS_PERM(PERM_SYSOP)) {
		vedit(database, NA, NULL);
		clear();
		continue;
	    } else {
		outs(SHM->i18nstr[cuser.language][1191]);
		continue;
	    }
	}
	i = 0;
	if ((fp = fopen(database, "r"))) {
	    while (fgets(lang, sizeof(lang), fp) != NULL) {
		if (lang[65] == '[') {
		    lang[65] = 0;
		    f = 1;
		} else
		    f = 0;
		if (strstr_lower(lang, word)) {
		    if (f == 1)
			lang[65] = '[';
		    outs(lang);
		    i++;
		    if (!((i + 1) % 17)) {
			move(23, 0);
			outs(SHM->i18nstr[cuser.language][1192]);
			j = igetch();
			if (j == 'q')
			    break;
			else {
			    clear();
			    move(4, 0);
			    outs(buf);
			}
		    }
		}
	    }
	    fclose(fp);
	}
	if (i == 0) {
	    getdata(5, 0, SHM->i18nstr[cuser.language][1193], lang, 3, LCECHO);
	    if (lang[0] == 'y') {
		clear();
		move(4, 0);
		outs(buf);
		addword(database,word);
	    }
	}
    }
}

int
x_dict()
{
    char dict[41], database[41];
    if (choose_dict(dict,sizeof(dict),database,sizeof(database)))
	use_dict(dict,database);
    return 0;
}
