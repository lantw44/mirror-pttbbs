/* $Id$ */
#define _XOPEN_SOURCE
#define _ISOC99_SOURCE
 
#include "bbs.h"

static char    *sex[8] = {
    MSG_BIG_BOY, MSG_BIG_GIRL, MSG_LITTLE_BOY, MSG_LITTLE_GIRL,
    MSG_MAN, MSG_WOMAN, MSG_PLANT, MSG_MIME
};
int
kill_user(int num)
{
  userec_t u;
  memset(&u, 0, sizeof(userec_t));
  log_usies("KILL", getuserid(num));
  setuserid(num, "");
  passwd_update(num, &u);
  return 0;
}
int
u_loginview()
{
    int             i;
    unsigned int    pbits = cuser.loginview;

    clear();
    move(4, 0);
    for (i = 0; i < NUMVIEWFILE; i++)
	prints("    %c. %-20s %-15s \n", 'A' + i,
	       SHM->i18nstr[cuser.language][loginview_file[i].string_index]
	       , ((pbits >> i) & 1 ? SHM->i18nstr[cuser.language][2079] : 
	       SHM->i18nstr[cuser.language][2080]));

    clrtobot();
    while ((i = getkey(SHM->i18nstr[cuser.language][2081]))!='\r') {
	i = i - 'a';
               	
	if (i >= NUMVIEWFILE || i < 0)
	    bell();
	else {
	    pbits ^= (1 << i);
	    move(i + 4, 28);
	    prints((pbits >> i) & 1 ? SHM->i18nstr[cuser.language][2082] : SHM->i18nstr[cuser.language][2083]);
	}
    }

    if (pbits != cuser.loginview) {
	cuser.loginview = pbits;
	passwd_update(usernum, &cuser);
    }
    return 0;
}

void
user_display(userec_t * u, int real)
{
    int             diff = 0;
    char            genbuf[200];

    clrtobot();
    prints(
	   SHM->i18nstr[cuser.language][2084]);
    prints(SHM->i18nstr[cuser.language][2085],
	   u->userid, u->username, u->realname,
#ifdef FOREIGN_REG
	   u->uflag2 & FOREIGN ? SHM->i18nstr[cuser.language][2087] : "",
	   u->uflag2 & FOREIGN ?
		(u->uflag2 & LIVERIGHT) ? SHM->i18nstr[cuser.language][2088] : SHM->i18nstr[cuser.language][2089]
		: "",
#else
		"","",
#endif
	   u->address, u->email,
	   sex[u->sex % 8], u->money);

    sethomedir(genbuf, u->userid);
    prints(SHM->i18nstr[cuser.language][2090],
	   get_num_records(genbuf, sizeof(fileheader_t)),
	   u->exmailbox, u->mobile,
	   u->month, u->day, u->year % 100, u->mychicken.name);
    prints(SHM->i18nstr[cuser.language][2091], ctime(&u->firstlogin));
    prints(SHM->i18nstr[cuser.language][2092], ctime(&u->lastlogin));
    prints(SHM->i18nstr[cuser.language][2093], ctime(&u->lastsong));
    prints(SHM->i18nstr[cuser.language][2094],
	   u->numlogins, u->numposts);

    if (real) {
	strcpy(genbuf, "bTCPRp#@XWBA#VSM0123456789ABCDEF");
	for (diff = 0; diff < 32; diff++)
	    if (!(u->userlevel & (1 << diff)))
		genbuf[diff] = '-';
	prints(SHM->i18nstr[cuser.language][2095],
	       u->justify, genbuf);
    } else {
	diff = (now - login_start_time) / 60;
	prints(SHM->i18nstr[cuser.language][2096],
	       diff / 60, diff % 60);
    }

    /* Thor: 想看看這個 user 是那些板的板主 */
    if (u->userlevel >= PERM_BM) {
	int             i;
	boardheader_t  *bhdr;

	outs(SHM->i18nstr[cuser.language][2097]);

	for (i = 0, bhdr = bcache; i < numboards; i++, bhdr++) {
	    if (is_uBM(bhdr->BM, u->userid)) {
		outs(bhdr->brdname);
		outc(' ');
	    }
	}
	outc('\n');
    }
    outs(SHM->i18nstr[cuser.language][2098]);

    outs((u->userlevel & PERM_LOGINOK) ?
	 SHM->i18nstr[cuser.language][2099] :
	 SHM->i18nstr[cuser.language][2100]);

#ifdef NEWUSER_LIMIT
    if ((u->lastlogin - u->firstlogin < 3 * 86400) && !HAS_PERM(PERM_POST))
	outs(SHM->i18nstr[cuser.language][2101]);
#endif
}

void
mail_violatelaw(char *crime, char *police, char *reason, char *result)
{
    char            genbuf[200];
    fileheader_t    fhdr;
    FILE           *fp;
    snprintf(genbuf, 200, "home/%c/%s", crime[0], crime);
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
	return;
    fprintf(fp, SHM->i18nstr[cuser.language][2102],
	    ctime(&now), police, crime, reason, result);
    fclose(fp);
    strcpy(fhdr.title, SHM->i18nstr[cuser.language][2103]);
    strcpy(fhdr.owner, SHM->i18nstr[cuser.language][2104]);
    snprintf(genbuf, 200, "home/%c/%s/.DIR", crime[0], crime);
    append_record(genbuf, &fhdr, sizeof(fhdr));
}

static void
violate_law(userec_t * u, int unum)
{
    char            ans[4], ans2[4];
    char            reason[128];
    move(1, 0);
    clrtobot();
    move(2, 0);
    prints(SHM->i18nstr[cuser.language][2105]);
    prints(SHM->i18nstr[cuser.language][2106]);
    getdata(5, 0, SHM->i18nstr[cuser.language][2107], ans, 3, DOECHO);
    switch (ans[0]) {
    case '1':
	strcpy(reason, "Cross-post");
	break;
    case '2':
	strcpy(reason, SHM->i18nstr[cuser.language][2108]);
	break;
    case '3':
	strcpy(reason, SHM->i18nstr[cuser.language][2109]);
	break;
    case '4':
	while (!getdata(7, 0, SHM->i18nstr[cuser.language][2110], reason, 50, DOECHO));
	strcat(reason, SHM->i18nstr[cuser.language][2111]);
	break;
    case '8':
    case '9':
	while (!getdata(6, 0, SHM->i18nstr[cuser.language][2112], reason, 50, DOECHO));
	break;
    default:
	return;
    }
    getdata(7, 0, msg_sure_ny, ans2, 3, LCECHO);
    if (*ans2 != 'y')
	return;
    if (ans[0] == '9') {
	char            src[STRLEN], dst[STRLEN];
	snprintf(src, sizeof(src), "home/%c/%s", u->userid[0], u->userid);
	snprintf(dst, sizeof(dst), "tmp/%s", u->userid);
	Rename(src, dst);
	post_violatelaw(u->userid, cuser.userid, reason, SHM->i18nstr[cuser.language][2113]);
        kill_user(unum);

    } else {
	u->userlevel |= PERM_VIOLATELAW;
	u->vl_count++;
	passwd_update(unum, u);
	post_violatelaw(u->userid, cuser.userid, reason, SHM->i18nstr[cuser.language][2114]);
	mail_violatelaw(u->userid, cuser.userid, reason, SHM->i18nstr[cuser.language][2115]);
    }
    pressanykey();
}

static void Customize(void)
{
    char    done = 0, mindbuf[5];

    showtitle(SHM->i18nstr[cuser.language][2119], SHM->i18nstr[cuser.language][2120]);
    memcpy(mindbuf, &currutmp->mind, 4);
    mindbuf[4] = 0;
    while( !done ){
	move(2, 0);
	prints(SHM->i18nstr[cuser.language][2121]);
	move(4, 0);
	prints("%-30s%10s\n", SHM->i18nstr[cuser.language][2122],
	       SHM->i18nstr[cuser.language][2116 + (cuser.uflag2 & WATER_MASK)]);
	prints("%-30s%10s\n", SHM->i18nstr[cuser.language][2123],
	       ((cuser.userlevel & PERM_NOOUTMAIL) ? SHM->i18nstr[cuser.language][2124] : SHM->i18nstr[cuser.language][2125]));
	prints("%-30s%10s\n", SHM->i18nstr[cuser.language][2126],
	       ((cuser.uflag2 & FAVNEW_FLAG) ? SHM->i18nstr[cuser.language][2127] : SHM->i18nstr[cuser.language][2128]));
	prints("%-30s%10s\n", SHM->i18nstr[cuser.language][2129], mindbuf);
	prints("%-30s%10s\n", SHM->i18nstr[cuser.language][2130], 
	       ((cuser.uflag2 & FAVNOHILIGHT) ? SHM->i18nstr[cuser.language][2131] : SHM->i18nstr[cuser.language][2132]));
	switch(getkey(SHM->i18nstr[cuser.language][2133])){
	case 'a':{
	    int     currentset = cuser.uflag2 & WATER_MASK;
	    currentset = (currentset + 1) % 3;
	    cuser.uflag2 &= ~WATER_MASK;
	    cuser.uflag2 |= currentset;
	    vmsg(SHM->i18nstr[cuser.language][2134]);
	}
	    break;
	case 'b':
	    cuser.userlevel ^= PERM_NOOUTMAIL;
	    break;
	case 'c':
	    cuser.uflag2 ^= FAVNEW_FLAG;
	    if (cuser.uflag2 & FAVNEW_FLAG)
		subscribe_newfav();
	    break;
	case 'd':{
	    getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][2135],
		    mindbuf, sizeof(mindbuf), DOECHO);
	    if (strcmp(mindbuf, SHM->i18nstr[cuser.language][2136]) == 0)
		vmsg(SHM->i18nstr[cuser.language][2137]);
	    else if (strcmp(mindbuf, SHM->i18nstr[cuser.language][2138]) == 0)
		vmsg(SHM->i18nstr[cuser.language][2139]);
	    else
		memcpy(currutmp->mind, mindbuf, 4);
	}
	    break;
	case 'e':
	    cuser.uflag2 ^= FAVNOHILIGHT;
	    break;
	default:
	    done = 1;
	}
	passwd_update(usernum, &cuser);
    }
    pressanykey();
}

void
uinfo_query(userec_t * u, int real, int unum)
{
    userec_t        x;
    register int    i = 0, fail, mail_changed;
    int             uid, ans;
    char            buf[STRLEN], *p;
    char            genbuf[200], reason[50];
    int money = 0;
    fileheader_t    fhdr;
    int             flag = 0, temp = 0, money_change = 0;

    FILE           *fp;

    fail = mail_changed = 0;

    memcpy(&x, u, sizeof(userec_t));
	ans = getans(real ?
		SHM->i18nstr[cuser.language][2140] :
		SHM->i18nstr[cuser.language][2141]);
    if (ans > '2' && ans != 'C' && ans != 'c' && !real)
	ans = '0';

    if (ans == '1' || ans == '3') {
	clear();
	i = 1;
	move(i++, 0);
	outs(msg_uid);
	outs(x.userid);
    }
    switch (ans) {
    case 'C':
    case 'c':
	Customize();
	return;
    case '7':
	violate_law(&x, unum);
	return;
    case '1':
	move(0, 0);
	outs(SHM->i18nstr[cuser.language][2142]);

	getdata_buf(i++, 0, SHM->i18nstr[cuser.language][2143], x.username,
		    sizeof(x.username), DOECHO);
	if (real) {
	    getdata_buf(i++, 0, SHM->i18nstr[cuser.language][2144],
			x.realname, sizeof(x.realname), DOECHO);
#ifdef FOREIGN_REG
	    getdata_buf(i++, 0, cuser.uflag2 & FOREIGN ? SHM->i18nstr[cuser.language][2145] : SHM->i18nstr[cuser.language][2146], x.ident, sizeof(x.ident), DOECHO);
#else
	    getdata_buf(i++, 0, SHM->i18nstr[cuser.language][2147], x.ident, sizeof(x.ident), DOECHO);
#endif
	    getdata_buf(i++, 0, SHM->i18nstr[cuser.language][2148],
			x.address, sizeof(x.address), DOECHO);
	}
	snprintf(buf, sizeof(buf), "%010d", x.mobile);
	getdata_buf(i++, 0, SHM->i18nstr[cuser.language][2149], buf, 11, LCECHO);
	x.mobile = atoi(buf);
	getdata_str(i++, 0, SHM->i18nstr[cuser.language][2150], buf, 50, DOECHO,
		    x.email);
	if (strcmp(buf, x.email) && strchr(buf, '@')) {
	    strlcpy(x.email, buf, sizeof(x.email));
	    mail_changed = 1 - real;
	}
	snprintf(genbuf, sizeof(genbuf), "%i", (u->sex + 1) % 8);
	getdata_str(i++, 0, SHM->i18nstr[cuser.language][2151],
		    buf, 3, DOECHO, genbuf);
	if (buf[0] >= '1' && buf[0] <= '8')
	    x.sex = (buf[0] - '1') % 8;
	else
	    x.sex = u->sex % 8;

	while (1) {
	    int             len;

	    snprintf(genbuf, sizeof(genbuf), "%02i/%02i/%02i",
		     u->month, u->day, u->year % 100);
	    len = getdata_str(i, 0, SHM->i18nstr[cuser.language][2152], buf, 9,
			      DOECHO, genbuf);
	    if (len && len != 8)
		continue;
	    if (!len) {
		x.month = u->month;
		x.day = u->day;
		x.year = u->year;
	    } else if (len == 8) {
		x.month = (buf[0] - '0') * 10 + (buf[1] - '0');
		x.day = (buf[3] - '0') * 10 + (buf[4] - '0');
		x.year = (buf[6] - '0') * 10 + (buf[7] - '0');
	    } else
		continue;
	    if (!real && (x.month > 12 || x.month < 1 || x.day > 31 ||
			  x.day < 1 || x.year > 90 || x.year < 40))
		continue;
	    i++;
	    break;
	}
	if (real) {
	    int l;
	    if (HAS_PERM(PERM_BBSADM)) {
		snprintf(genbuf, sizeof(genbuf), "%d", x.money);
		if (getdata_str(i++, 0, SHM->i18nstr[cuser.language][2153], buf, 10, DOECHO, genbuf))
		    if ((l = atol(buf)) != 0) {
			if (l != x.money) {
			    money_change = 1;
			    money = x.money;
			    x.money = l;
			}
		    }
	    }
	    snprintf(genbuf, sizeof(genbuf), "%d", x.exmailbox);
	    if (getdata_str(i++, 0, SHM->i18nstr[cuser.language][2154], buf, 6,
			    DOECHO, genbuf))
		if ((l = atol(buf)) != 0)
		    x.exmailbox = (int)l;

	    getdata_buf(i++, 0, SHM->i18nstr[cuser.language][2155], x.justify,
			sizeof(x.justify), DOECHO);
	    getdata_buf(i++, 0, SHM->i18nstr[cuser.language][2156],
			x.lasthost, sizeof(x.lasthost), DOECHO);

	    snprintf(genbuf, sizeof(genbuf), "%d", x.numlogins);
	    if (getdata_str(i++, 0, SHM->i18nstr[cuser.language][2157], buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.numlogins = fail;
	    snprintf(genbuf, sizeof(genbuf), "%d", u->numposts);
	    if (getdata_str(i++, 0, SHM->i18nstr[cuser.language][2158], buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.numposts = fail;
	    snprintf(genbuf, sizeof(genbuf), "%d", u->goodpost);
	    if (getdata_str(i++, 0, SHM->i18nstr[cuser.language][2159], buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.goodpost = fail;
	    snprintf(genbuf, sizeof(genbuf), "%d", u->badpost);
	    if (getdata_str(i++, 0, SHM->i18nstr[cuser.language][2160], buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.badpost = fail;
	    snprintf(genbuf, sizeof(genbuf), "%d", u->vl_count);
	    if (getdata_str(i++, 0, SHM->i18nstr[cuser.language][2161], buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.vl_count = fail;

	    snprintf(genbuf, sizeof(genbuf),
		     "%d/%d/%d", u->five_win, u->five_lose, u->five_tie);
	    if (getdata_str(i++, 0, SHM->i18nstr[cuser.language][2162], buf, 16, DOECHO,
			    genbuf))
		while (1) {
		    p = strtok(buf, "/\r\n");
		    if (!p)
			break;
		    x.five_win = atoi(p);
		    p = strtok(NULL, "/\r\n");
		    if (!p)
			break;
		    x.five_lose = atoi(p);
		    p = strtok(NULL, "/\r\n");
		    if (!p)
			break;
		    x.five_tie = atoi(p);
		    break;
		}
	    snprintf(genbuf, sizeof(genbuf),
		     "%d/%d/%d", u->chc_win, u->chc_lose, u->chc_tie);
	    if (getdata_str(i++, 0, SHM->i18nstr[cuser.language][2163], buf, 16, DOECHO,
			    genbuf))
		while (1) {
		    p = strtok(buf, "/\r\n");
		    if (!p)
			break;
		    x.chc_win = atoi(p);
		    p = strtok(NULL, "/\r\n");
		    if (!p)
			break;
		    x.chc_lose = atoi(p);
		    p = strtok(NULL, "/\r\n");
		    if (!p)
			break;
		    x.chc_tie = atoi(p);
		    break;
		}
#ifdef FOREIGN_REG
	    if (getdata_str(i++, 0, SHM->i18nstr[cuser.language][2164], buf, 2, DOECHO, x.uflag2 & FOREIGN ? "2" : "1"))
		if ((fail = atoi(buf)) > 0){
		    if (fail == 2){
			x.uflag2 |= FOREIGN;
		    }
		    else
			x.uflag2 &= ~FOREIGN;
		}
	    if (x.uflag2 & FOREIGN)
		if (getdata_str(i++, 0, SHM->i18nstr[cuser.language][2165], buf, 2, DOECHO, x.uflag2 & LIVERIGHT ? "1" : "2")){
		    if ((fail = atoi(buf)) > 0){
			if (fail == 1){
			    x.uflag2 |= LIVERIGHT;
			    x.userlevel |= (PERM_LOGINOK | PERM_POST);
			}
			else{
			    x.uflag2 &= ~LIVERIGHT;
			    x.userlevel &= ~(PERM_LOGINOK | PERM_POST);
			}
		    }
		}
#endif
	    fail = 0;
	}
	break;

    case '2':
	i = 19;
	if (!real) {
	    if (!getdata(i++, 0, SHM->i18nstr[cuser.language][2166], buf, PASSLEN, NOECHO) ||
		!checkpasswd(u->passwd, buf)) {
		outs(SHM->i18nstr[cuser.language][2167]);
		fail++;
		break;
	    }
	} else {
	    char            witness[3][32];
	    for (i = 0; i < 3; i++) {
		if (!getdata(19 + i, 0, SHM->i18nstr[cuser.language][2168],
			     witness[i], sizeof(witness[i]), DOECHO)) {
		    outs(SHM->i18nstr[cuser.language][2169]);
		    fail++;
		    break;
		} else if (!(uid = getuser(witness[i]))) {
		    outs(SHM->i18nstr[cuser.language][2170]);
		    fail++;
		    break;
		} else {
		    userec_t        atuser;
		    passwd_query(uid, &atuser);
		    if (now - atuser.firstlogin < 6 * 30 * 24 * 60 * 60) {
			outs(SHM->i18nstr[cuser.language][2171]);
			i--;
		    }
		}
	    }
	    if (i < 3)
		break;
	    else
		i = 20;
	}

	if (!getdata(i++, 0, SHM->i18nstr[cuser.language][2172], buf, PASSLEN, NOECHO)) {
	    outs(SHM->i18nstr[cuser.language][2173]);
	    fail++;
	    break;
	}
	strncpy(genbuf, buf, PASSLEN);

	getdata(i++, 0, SHM->i18nstr[cuser.language][2174], buf, PASSLEN, NOECHO);
	if (strncmp(buf, genbuf, PASSLEN)) {
	    outs(SHM->i18nstr[cuser.language][2175]);
	    fail++;
	    break;
	}
	buf[8] = '\0';
	strncpy(x.passwd, genpasswd(buf), PASSLEN);
	if (real)
	    x.userlevel &= (!PERM_LOGINOK);
	break;

    case '3':
	i = setperms(x.userlevel, str_permid);
	if (i == x.userlevel)
	    fail++;
	else {
	    flag = 1;
	    temp = x.userlevel;
	    x.userlevel = i;
	}
	break;

    case '4':
	i = QUIT;
	break;

    case '5':
	if (getdata_str(b_lines - 3, 0, SHM->i18nstr[cuser.language][2176], genbuf, IDLEN + 1,
			DOECHO, x.userid)) {
	    if (searchuser(genbuf)) {
		outs(SHM->i18nstr[cuser.language][2177]);
		fail++;
	    } else
		strlcpy(x.userid, genbuf, sizeof(x.userid));
	}
	break;
    case '6':
	if (x.mychicken.name[0])
	    x.mychicken.name[0] = 0;
	else
	    strlcpy(x.mychicken.name, SHM->i18nstr[cuser.language][2178], sizeof(x.mychicken.name));
	break;
    default:
	return;
    }

    if (fail) {
	pressanykey();
	return;
    }
	if (getans(msg_sure_ny) == 'y') {
	if (flag)
	    post_change_perm(temp, i, cuser.userid, x.userid);
	if (strcmp(u->userid, x.userid)) {
	    char            src[STRLEN], dst[STRLEN];

	    sethomepath(src, u->userid);
	    sethomepath(dst, x.userid);
	    Rename(src, dst);
	    setuserid(unum, x.userid);
	}
	memcpy(u, &x, sizeof(x));
	if (mail_changed) {
#ifdef EMAIL_JUSTIFY
	    x.userlevel &= ~PERM_LOGINOK;
	    mail_justify();
#endif
	}
	if (i == QUIT) {
	    char            src[STRLEN], dst[STRLEN];

	    snprintf(src, sizeof(src), "home/%c/%s", x.userid[0], x.userid);
	    snprintf(dst, sizeof(dst), "tmp/%s", x.userid);
	    Rename(src, dst);	/* do not remove user home */
            kill_user(unum);
	    return;
	} else
	    log_usies("SetUser", x.userid);
	if (money_change)
	    setumoney(unum, x.money);
	passwd_update(unum, &x);
	if (money_change) {
	    strlcpy(genbuf, "boards/S/Security", sizeof(genbuf));
	    stampfile(genbuf, &fhdr);
	    if (!(fp = fopen(genbuf, "w")))
		return;

	    fprintf(fp, SHM->i18nstr[cuser.language][2179],
		    ctime(&now), cuser.userid, x.userid, money, x.money);

	    clrtobot();
	    clear();
	    while (!getdata(5, 0, SHM->i18nstr[cuser.language][2180],
			    reason, sizeof(reason), DOECHO));

	    fprintf(fp, SHM->i18nstr[cuser.language][2181],
		    cuser.userid, reason);
	    fclose(fp);
	    snprintf(fhdr.title, sizeof(fhdr.title),
		     SHM->i18nstr[cuser.language][2182], cuser.userid,
		     x.userid);
	    strlcpy(fhdr.owner, SHM->i18nstr[cuser.language][2183], sizeof(fhdr.owner));
	    append_record("boards/S/Security/.DIR", &fhdr, sizeof(fhdr));
	}
    }
}

int
u_info()
{
    move(2, 0);
    user_display(&cuser, 0);
    uinfo_query(&cuser, 0, usernum);
    strlcpy(currutmp->username, cuser.username, sizeof(currutmp->username));
    return 0;
}

int
u_ansi()
{
    showansi ^= 1;
    cuser.uflag ^= COLOR_FLAG;
    outs(reset_color);
    return 0;
}

int
u_cloak()
{
    outs((currutmp->invisible ^= 1) ? MSG_CLOAKED : MSG_UNCLOAK);
    return XEASY;
}

int
u_switchproverb()
{
    /* char *state[4]={"用功\型","安逸型","自定型","SHUTUP"}; */
    char            buf[100];

    cuser.proverb = (cuser.proverb + 1) % 4;
    setuserfile(buf, fn_proverb);
    if (cuser.proverb == 2 && dashd(buf)) {
	FILE           *fp = fopen(buf, "a");
	assert(fp);

	fprintf(fp, SHM->i18nstr[cuser.language][2184]);
	fclose(fp);
    }
    passwd_update(usernum, &cuser);
    return 0;
}

int
u_editproverb()
{
    char            buf[100];

    setutmpmode(PROVERB);
    setuserfile(buf, fn_proverb);
    move(1, 0);
    clrtobot();
    outs(SHM->i18nstr[cuser.language][2185]);
    pressanykey();
    vedit(buf, NA, NULL);
    return 0;
}

void
showplans(char *uid)
{
    char            genbuf[200];

    sethomefile(genbuf, uid, fn_plans);
    if (!show_file(genbuf, 7, MAX_QUERYLINES, ONLY_COLOR))
	prints(SHM->i18nstr[cuser.language][2186], uid);
}

int
showsignature(char *fname, int *j)
{
    FILE           *fp;
    char            buf[256];
    int             i, num = 0;
    char            ch;

    clear();
    move(2, 0);
    setuserfile(fname, "sig.0");
    *j = strlen(fname) - 1;

    for (ch = '1'; ch <= '9'; ch++) {
	fname[*j] = ch;
	if ((fp = fopen(fname, "r"))) {
	    prints(SHM->i18nstr[cuser.language][2187], ch);
	    for (i = 0; i < MAX_SIGLINES && fgets(buf, sizeof(buf), fp); i++)
		outs(buf);
	    num++;
	    fclose(fp);
	}
    }
    return num;
}

int
u_editsig()
{
    int             aborted;
    char            ans[4];
    int             j;
    char            genbuf[200];

    showsignature(genbuf, &j);

    getdata(0, 0, SHM->i18nstr[cuser.language][2188],
	    ans, sizeof(ans), LCECHO);

    aborted = 0;
    if (ans[0] == 'd')
	aborted = 1;
    if (ans[0] == 'e')
	aborted = 2;

    if (aborted) {
	if (!getdata(1, 0, SHM->i18nstr[cuser.language][2189], ans, sizeof(ans), DOECHO))
	    ans[0] = '1';
	if (ans[0] >= '1' && ans[0] <= '9') {
	    genbuf[j] = ans[0];
	    if (aborted == 1) {
		unlink(genbuf);
		outs(msg_del_ok);
	    } else {
		setutmpmode(EDITSIG);
		aborted = vedit(genbuf, NA, NULL);
		if (aborted != -1)
		    outs(SHM->i18nstr[cuser.language][2190]);
	    }
	}
	pressanykey();
    }
    return 0;
}

int
u_editplan()
{
    char            genbuf[200];

    getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][2191],
	    genbuf, 3, LCECHO);

    if (genbuf[0] == 'e') {
	int             aborted;

	setutmpmode(EDITPLAN);
	setuserfile(genbuf, fn_plans);
	aborted = vedit(genbuf, NA, NULL);
	if (aborted != -1)
	    outs(SHM->i18nstr[cuser.language][2192]);
	pressanykey();
	return 0;
    } else if (genbuf[0] == 'd') {
	setuserfile(genbuf, fn_plans);
	unlink(genbuf);
	outmsg(SHM->i18nstr[cuser.language][2193]);
    }
    return 0;
}

int
u_editcalendar()
{
    char            genbuf[200];

    getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][2194],
	    genbuf, 3, LCECHO);

    sethomefile(genbuf, cuser.userid, "calendar");
    if (genbuf[0] == 'e') {
	int             aborted;

	setutmpmode(EDITPLAN);
	sethomefile(genbuf, cuser.userid, "calendar");
	aborted = vedit(genbuf, NA, NULL);
	if (aborted != -1)
	    vmsg(SHM->i18nstr[cuser.language][2195]);
	return 0;
    } else if (genbuf[0] == 'd') {
	unlink(genbuf);
	vmsg(SHM->i18nstr[cuser.language][2196]);
    }
    return 0;
}

/* 使用者填寫註冊表格 */
static void
getfield(int line, char *info, char *desc, char *buf, int len)
{
    char            prompt[STRLEN];
    char            genbuf[200];

    move(line, 2);
    prints(SHM->i18nstr[cuser.language][2197], buf, info);
    snprintf(prompt, sizeof(prompt), SHM->i18nstr[cuser.language][2198], desc);
    if (getdata_str(line + 1, 2, prompt, genbuf, len, DOECHO, buf))
	strcpy(buf, genbuf);
    move(line, 2);
    prints(SHM->i18nstr[cuser.language][2199], desc, buf);
    clrtoeol();
}

static int
removespace(char *s)
{
    int             i, index;

    for (i = 0, index = 0; s[i]; i++) {
	if (s[i] != ' ')
	    s[index++] = s[i];
    }
    s[index] = '\0';
    return index;
}

static int
ispersonalid(char *inid)
{
    char           *lst = "ABCDEFGHJKLMNPQRSTUVXYWZIO", id[20];
    int             i, j, cksum;

    strlcpy(id, inid, sizeof(id));
    i = cksum = 0;
    if (!isalpha(id[0]) && (strlen(id) != 10))
	return 0;
    if (!(id[1] == '1' || id[1] == '2'))
	return 0;
    id[0] = toupper(id[0]);

    if( strcmp(id, "A100000001") == 0 ||
	strcmp(id, "A200000003") == 0 ||
	strcmp(id, "A123456789") == 0    )
	return 0;
    /* A->10, B->11, ..H->17,I->34, J->18... */
    while (lst[i] != id[0])
	i++;
    i += 10;
    id[0] = i % 10 + '0';
    if (!isdigit(id[9]))
	return 0;
    cksum += (id[9] - '0') + (i / 10);

    for (j = 0; j < 9; ++j) {
	if (!isdigit(id[j]))
	    return 0;
	cksum += (id[j] - '0') * (9 - j);
    }
    return (cksum % 10) == 0;
}

static char    *
getregcode(char *buf)
{
    sprintf(buf, "%s", crypt(cuser.userid, "02"));
    return buf;
}

static int
isvalidemail(char *email)
{
    FILE           *fp;
    char            buf[128], *c;
    if (!strstr(email, "@"))
	return 0;
    for (c = strstr(email, "@"); *c != 0; ++c)
	if ('A' <= *c && *c <= 'Z')
	    *c += 32;

    if ((fp = fopen("etc/banemail", "r"))) {
	while (fgets(buf, sizeof(buf), fp)) {
	    if (buf[0] == '#')
		continue;
	    buf[strlen(buf) - 1] = 0;
	    if (buf[0] == 'A' && strcasecmp(&buf[1], email) == 0)
		return 0;
	    if (buf[0] == 'P' && strcasestr(email, &buf[1]))
		return 0;
	    if (buf[0] == 'S' && strcasecmp(strstr(email, "@") + 1, &buf[1]) == 0)
		return 0;
	}
	fclose(fp);
    }
    return 1;
}

static void
toregister(char *email, char *genbuf, char *phone, char *career,
	   char *ident, char *rname, char *addr, char *mobile)
{
    FILE           *fn;
    char            buf[128];

    sethomefile(buf, cuser.userid, "justify.wait");
    if (phone[0] != 0) {
	fn = fopen(buf, "w");
	assert(fn);
	fprintf(fn, "%s\n%s\n%s\n%s\n%s\n%s\n",
		phone, career, ident, rname, addr, mobile);
	fclose(fn);
    }
    clear();
    stand_title(SHM->i18nstr[cuser.language][2200]);
    if (cuser.userlevel & PERM_NOREGCODE){
	strcpy(email, "x");
	goto REGFORM2;
    }
    move(2, 0);
    outs(SHM->i18nstr[cuser.language][2201]);

#ifdef HAVEMOBILE
    outs(SHM->i18nstr[cuser.language][2202]);
#endif

    while (1) {
	email[0] = 0;
	getfield(15, SHM->i18nstr[cuser.language][2203], "E-Mail Address", email, 50);
	if (strcmp(email, "x") == 0 || strcmp(email, "X") == 0)
	    break;
#ifdef HAVEMOBILE
	else if (strcmp(email, "m") == 0 || strcmp(email, "M") == 0) {
	    if (isvalidmobile(mobile)) {
		char            yn[3];
		getdata(16, 0, SHM->i18nstr[cuser.language][2204],
			yn, sizeof(yn), LCECHO);
		if (yn[0] == 'Y' || yn[0] == 'y')
		    break;
	    } else {
		move(17, 0);
		prints(SHM->i18nstr[cuser.language][2205]);
	    }

	}
#endif
	else if (isvalidemail(email)) {
	    char            yn[3];
	    getdata(16, 0, SHM->i18nstr[cuser.language][2206],
		    yn, sizeof(yn), LCECHO);
	    if (yn[0] == 'Y' || yn[0] == 'y')
		break;
	} else {
	    move(17, 0);
	    prints(SHM->i18nstr[cuser.language][2207]);
	}
    }
    strncpy(cuser.email, email, sizeof(cuser.email));
 REGFORM2:
    if (strcasecmp(email, "x") == 0) {	/* 手動認證 */
	if ((fn = fopen(fn_register, "a"))) {
	    fprintf(fn, "num: %d, %s", usernum, ctime(&now));
	    fprintf(fn, "uid: %s\n", cuser.userid);
	    fprintf(fn, "ident: %s\n", ident);
	    fprintf(fn, "name: %s\n", rname);
	    fprintf(fn, "career: %s\n", career);
	    fprintf(fn, "addr: %s\n", addr);
	    fprintf(fn, "phone: %s\n", phone);
	    fprintf(fn, "mobile: %s\n", mobile);
	    fprintf(fn, "email: %s\n", email);
	    fprintf(fn, "----\n");
	    fclose(fn);
	}
    } else {
	char            tmp[IDLEN + 1];
	if (phone != NULL) {
#ifdef HAVEMOBILE
	    if (strcmp(email, "m") == 0 || strcmp(email, "M") == 0)
		sprintf(genbuf, sizeof(genbuf),
			"%s:%s:<Mobile>", phone, career);
	    else
#endif
		snprintf(genbuf, sizeof(genbuf),
			 "%s:%s:<Email>", phone, career);
	    strncpy(cuser.justify, genbuf, REGLEN);
	    sethomefile(buf, cuser.userid, "justify");
	}
	snprintf(buf, sizeof(buf), "%s%s%s%s",
		 SHM->i18nstr[cuser.language][2208], BBSNAME, SHM->i18nstr[cuser.language][2209], getregcode(genbuf));
	strlcpy(tmp, cuser.userid, sizeof(tmp));
	strlcpy(cuser.userid, "SYSOP", sizeof(cuser.userid));
#ifdef HAVEMOBILE
	if (strcmp(email, "m") == 0 || strcmp(email, "M") == 0)
	    mobile_message(mobile, buf);
	else
#endif
	    bsmtp("etc/registermail", buf, email, 0);
	strlcpy(cuser.userid, tmp, sizeof(cuser.userid));
	outs(SHM->i18nstr[cuser.language][2210]);
	pressanykey();
	return;
    }
}

#ifndef FOREIGN_REG
static int HaveRejectStr(char *s, char **rej)
{
    int     i;
    char    *ptr, *rejectstr[] =
	{SHM->i18nstr[cuser.language][2211], SHM->i18nstr[cuser.language][2212], SHM->i18nstr[cuser.language][2213], SHM->i18nstr[cuser.language][2214], SHM->i18nstr[cuser.language][2215], SHM->i18nstr[cuser.language][2216], SHM->i18nstr[cuser.language][2217], "..", "xx",
	 SHM->i18nstr[cuser.language][2218], SHM->i18nstr[cuser.language][2219], SHM->i18nstr[cuser.language][2220], SHM->i18nstr[cuser.language][2221], SHM->i18nstr[cuser.language][2222], 
	 SHM->i18nstr[cuser.language][2223], SHM->i18nstr[cuser.language][2224], SHM->i18nstr[cuser.language][2225], SHM->i18nstr[cuser.language][2226], SHM->i18nstr[cuser.language][2227], SHM->i18nstr[cuser.language][2228], SHM->i18nstr[cuser.language][2229], SHM->i18nstr[cuser.language][2230], SHM->i18nstr[cuser.language][2231], SHM->i18nstr[cuser.language][2232], SHM->i18nstr[cuser.language][2233],
	 SHM->i18nstr[cuser.language][2234], SHM->i18nstr[cuser.language][2235], SHM->i18nstr[cuser.language][2236], SHM->i18nstr[cuser.language][2237],/*"ㄔ",*/    SHM->i18nstr[cuser.language][2238], SHM->i18nstr[cuser.language][2239], SHM->i18nstr[cuser.language][2240], SHM->i18nstr[cuser.language][2241], SHM->i18nstr[cuser.language][2242],
	 SHM->i18nstr[cuser.language][2243], SHM->i18nstr[cuser.language][2244], SHM->i18nstr[cuser.language][2245], SHM->i18nstr[cuser.language][2246], SHM->i18nstr[cuser.language][2247], SHM->i18nstr[cuser.language][2248], SHM->i18nstr[cuser.language][2249], SHM->i18nstr[cuser.language][2250], SHM->i18nstr[cuser.language][2251], SHM->i18nstr[cuser.language][2252], SHM->i18nstr[cuser.language][2253],
	 SHM->i18nstr[cuser.language][2254], SHM->i18nstr[cuser.language][2255], SHM->i18nstr[cuser.language][2256], SHM->i18nstr[cuser.language][2257], SHM->i18nstr[cuser.language][2258], NULL};

    if( rej != NULL )
	for( i = 0 ; rej[i] != NULL ; ++i )
	    if( strstr(s, rej[i]) )
		return 1;

    for( i = 0 ; rejectstr[i] != NULL ; ++i )
	if( strstr(s, rejectstr[i]) )
	    return 1;

    if( (ptr = strstr(s, SHM->i18nstr[cuser.language][2259])) != NULL ){
	if( ptr != s && strncmp(ptr - 1, SHM->i18nstr[cuser.language][2260], 4) == 0 )
	    return 0;
	return 1;
    }
    return 0;
}
#endif

static char *isvalidname(char *rname)
{
#ifdef FOREIGN_REG
    return NULL;
#else
    char    *rejectstr[] =
	{SHM->i18nstr[cuser.language][2261], SHM->i18nstr[cuser.language][2262], SHM->i18nstr[cuser.language][2263], SHM->i18nstr[cuser.language][2264], SHM->i18nstr[cuser.language][2265], SHM->i18nstr[cuser.language][2266], SHM->i18nstr[cuser.language][2267], SHM->i18nstr[cuser.language][2268], SHM->i18nstr[cuser.language][2269],
	 SHM->i18nstr[cuser.language][2270], SHM->i18nstr[cuser.language][2271], SHM->i18nstr[cuser.language][2272], SHM->i18nstr[cuser.language][2273], SHM->i18nstr[cuser.language][2274], SHM->i18nstr[cuser.language][2275], SHM->i18nstr[cuser.language][2276], SHM->i18nstr[cuser.language][2277], 
	 SHM->i18nstr[cuser.language][2278], SHM->i18nstr[cuser.language][2279], SHM->i18nstr[cuser.language][2280], SHM->i18nstr[cuser.language][2281], SHM->i18nstr[cuser.language][2282], SHM->i18nstr[cuser.language][2283], SHM->i18nstr[cuser.language][2284], SHM->i18nstr[cuser.language][2285],
	 SHM->i18nstr[cuser.language][2286], SHM->i18nstr[cuser.language][2287], SHM->i18nstr[cuser.language][2288], SHM->i18nstr[cuser.language][2289], SHM->i18nstr[cuser.language][2290], SHM->i18nstr[cuser.language][2291],
	 NULL};
    if( removespace(rname) && rname[0] < 0 &&
	strlen(rname) >= 4 &&
	!HaveRejectStr(rname, rejectstr) &&
	strncmp(rname, SHM->i18nstr[cuser.language][2292], 2) != 0   && //起頭是「小」
	strncmp(rname, SHM->i18nstr[cuser.language][2293], 4) != 0 && //起頭是「我是」
	!(strlen(rname) == 4 && strncmp(&rname[2], SHM->i18nstr[cuser.language][2294], 2) == 0) &&
	!(strlen(rname) >= 4 && strncmp(&rname[0], &rname[2], 2) == 0))
	return NULL;
    return SHM->i18nstr[cuser.language][2295];
#endif

}

static char *isvalidcareer(char *career)
{
#ifndef FOREIGN_REG
    char    *rejectstr[] = {NULL};
    if (!(removespace(career) && career[0] < 0 && strlen(career) >= 6) ||
	strcmp(career, SHM->i18nstr[cuser.language][2296]) == 0 || HaveRejectStr(career, rejectstr) )
	return SHM->i18nstr[cuser.language][2297];
    if (strcmp(&career[strlen(career) - 2], SHM->i18nstr[cuser.language][2298]) == 0 ||
	strcmp(&career[strlen(career) - 4], SHM->i18nstr[cuser.language][2299]) == 0 ||
	strcmp(career, SHM->i18nstr[cuser.language][2300]) == 0)
	return SHM->i18nstr[cuser.language][2301];
    if (strcmp(career, SHM->i18nstr[cuser.language][2302]) == 0)
	return SHM->i18nstr[cuser.language][2303];
#endif
    return NULL;
}

static char *isvalidaddr(char *addr)
{
#ifndef FOREIGN_REG
    char    *rejectstr[] =
	{SHM->i18nstr[cuser.language][2304], SHM->i18nstr[cuser.language][2305], SHM->i18nstr[cuser.language][2306], NULL};

    if (!removespace(addr) || addr[0] > 0 || strlen(addr) < 15) 
	return SHM->i18nstr[cuser.language][2307];
    if (strstr(addr, SHM->i18nstr[cuser.language][2308]) != NULL || strstr(addr, SHM->i18nstr[cuser.language][2309]) != NULL) 
	return SHM->i18nstr[cuser.language][2310];
    if ((strstr(addr, SHM->i18nstr[cuser.language][2311]) == NULL && strstr(addr, SHM->i18nstr[cuser.language][2312]) == NULL &&
	 strstr(addr, SHM->i18nstr[cuser.language][2313]) == NULL && strstr(addr, SHM->i18nstr[cuser.language][2314]) == NULL) ||
	HaveRejectStr(addr, rejectstr)             ||
	strcmp(&addr[strlen(addr) - 2], SHM->i18nstr[cuser.language][2315]) == 0 ||
	strcmp(&addr[strlen(addr) - 2], SHM->i18nstr[cuser.language][2316]) == 0 ||
	strcmp(&addr[strlen(addr) - 2], SHM->i18nstr[cuser.language][2317]) == 0 ||
	strcmp(&addr[strlen(addr) - 2], SHM->i18nstr[cuser.language][2318]) == 0 ||
	strcmp(&addr[strlen(addr) - 2], SHM->i18nstr[cuser.language][2319]) == 0 ||
	strcmp(&addr[strlen(addr) - 2], SHM->i18nstr[cuser.language][2320]) == 0 ||
	strcmp(&addr[strlen(addr) - 2], SHM->i18nstr[cuser.language][2321]) == 0    )
	return SHM->i18nstr[cuser.language][2322];
#endif
    return NULL;
}

static char *isvalidphone(char *phone)
{
    int     i;
    for( i = 0 ; phone[i] != 0 ; ++i )
	if( !isdigit(phone[i]) )
	    return SHM->i18nstr[cuser.language][2323];
    if (!removespace(phone) || 
	strlen(phone) < 9 || 
	strstr(phone, "00000000") != NULL ||
	strstr(phone, "22222222") != NULL    ) {
	return SHM->i18nstr[cuser.language][2324] ;
    }
    return NULL;
}

int
u_register(void)
{
    char            rname[21], addr[51], ident[12], mobile[21];
#ifdef FOREIGN_REG
    char            fore[2];
#endif
    char            phone[21], career[41], email[51], birthday[9], sex_is[2],
                    year, mon, day;
    char            inregcode[14], regcode[50];
    char            ans[3], *ptr, *errcode;
    char            genbuf[200];
    FILE           *fn;

    if (cuser.userlevel & PERM_LOGINOK) {
	outs(SHM->i18nstr[cuser.language][2325]);
	return XEASY;
    }
    if ((fn = fopen(fn_register, "r"))) {
	while (fgets(genbuf, STRLEN, fn)) {
	    if ((ptr = strchr(genbuf, '\n')))
		*ptr = '\0';
	    if (strncmp(genbuf, "uid: ", 5) == 0 &&
		strcmp(genbuf + 5, cuser.userid) == 0) {
		fclose(fn);
		outs(SHM->i18nstr[cuser.language][2326]);
		return XEASY;
	    }
	}
	fclose(fn);
    }
    strlcpy(ident, cuser.ident, sizeof(ident));
    strlcpy(rname, cuser.realname, sizeof(rname));
    strlcpy(addr, cuser.address, sizeof(addr));
    strlcpy(email, cuser.email, sizeof(email));
    snprintf(mobile, sizeof(mobile), "0%09d", cuser.mobile);
    if (cuser.month == 0 && cuser.day && cuser.year == 0)
	birthday[0] = 0;
    else
	snprintf(birthday, sizeof(birthday), "%02i/%02i/%02i",
		 cuser.month, cuser.day, cuser.year % 100);
    sex_is[0] = (cuser.sex % 8) + '1';
    sex_is[1] = 0;
    career[0] = phone[0] = '\0';
    sethomefile(genbuf, cuser.userid, "justify.wait");
    if ((fn = fopen(genbuf, "r"))) {
	fgets(phone, 21, fn);
	phone[strlen(phone) - 1] = 0;
	fgets(career, 41, fn);
	career[strlen(career) - 1] = 0;
	fgets(ident, 12, fn);
	ident[strlen(ident) - 1] = 0;
	fgets(rname, 21, fn);
	rname[strlen(rname) - 1] = 0;
	fgets(addr, 51, fn);
	addr[strlen(addr) - 1] = 0;
	fgets(mobile, 21, fn);
	mobile[strlen(mobile) - 1] = 0;
	fclose(fn);
    }

    if (cuser.userlevel & PERM_NOREGCODE) {
	vmsg(SHM->i18nstr[cuser.language][2327]);
	goto REGFORM;
    }

    if (cuser.year != 0 &&	/* 已經第一次填過了~ ^^" */
	strcmp(cuser.email, "x") != 0 &&	/* 上次手動認證失敗 */
	strcmp(cuser.email, "X") != 0) {
	clear();
	stand_title(SHM->i18nstr[cuser.language][2328]);
	move(2, 0);
	prints(SHM->i18nstr[cuser.language][2329],
	       cuser.userid, cuser.username);
	inregcode[0] = 0;
	do{
	    getdata(10, 0, SHM->i18nstr[cuser.language][2330], inregcode, sizeof(inregcode), DOECHO);
	    if( strcmp(inregcode, "x") == 0 ||
		strcmp(inregcode, "X") == 0 ||
		strlen(inregcode) == 13 )
		break;
	    if( strlen(inregcode) != 13 )
		vmsg(SHM->i18nstr[cuser.language][2331]);
	} while( 1 );

	if (strcmp(inregcode, getregcode(regcode)) == 0) {
	    int             unum;
	    if ((unum = getuser(cuser.userid)) == 0) {
		vmsg(SHM->i18nstr[cuser.language][2332]);
		u_exit("getuser error");
		exit(0);
	    }
	    mail_muser(cuser, SHM->i18nstr[cuser.language][2333], "etc/registeredmail");
	    if(cuser.uflag2 & FOREIGN)
		mail_muser(cuser, SHM->i18nstr[cuser.language][2334], "etc/foreign_welcome");
	    cuser.userlevel |= (PERM_LOGINOK | PERM_POST);
	    prints(SHM->i18nstr[cuser.language][2335]);
	    sethomefile(genbuf, cuser.userid, "justify.wait");
	    unlink(genbuf);
	    pressanykey();
	    u_exit("registed");
	    exit(0);
	    return QUIT;
	} else if (strcmp(inregcode, "x") != 0 &&
		   strcmp(inregcode, "X") != 0) {
	    vmsg(SHM->i18nstr[cuser.language][2336]);
	} else {
	    toregister(email, genbuf, phone, career,
		       ident, rname, addr, mobile);
	    return FULLUPDATE;
	}
    }

    REGFORM:
    getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][2337],
	    ans, 3, LCECHO);
    if (ans[0] != 'y')
	return FULLUPDATE;

    move(2, 0);
    clrtobot();
    while (1) {
	clear();
	move(1, 0);
	prints(SHM->i18nstr[cuser.language][2338],
	       cuser.userid, cuser.username);
#ifdef FOREIGN_REG
	fore[0] = 'y';
	fore[1] = 0;
	getfield(2, "Y/n", SHM->i18nstr[cuser.language][2339], fore, 2);
    	if (fore[0] == 'n')
	    fore[0] |= FOREIGN;
	else
	    fore[0] = 0;
	if (!fore[0]){
#endif
	    while( 1 ){
		getfield(3, "D123456789", SHM->i18nstr[cuser.language][2340], ident, 11);
		if ('a' <= ident[0] && ident[0] <= 'z')
		    ident[0] -= 32;
		if( ispersonalid(ident) )
		    break;
		vmsg(SHM->i18nstr[cuser.language][2341]);
	    }
#ifdef FOREIGN_REG
	}
	else{
	    int i;
	    while( 1 ){
		getfield(4, "0123456789",SHM->i18nstr[cuser.language][2342], ident, 11);
		move(6, 2);
		prints(SHM->i18nstr[cuser.language][2343]);
		getdata(7, 2, SHM->i18nstr[cuser.language][2344], ans, 3, LCECHO);
		if (ans[0] == 'y' || ans[0] == 'Y')
		    break;
		vmsg(SHM->i18nstr[cuser.language][2345]);
	    }
	    for(i = 0; ans[i] != 0; i++)
		if ('a' <= ident[0] && ident[0] <= 'z')
		    ident[0] -= 32;
	    if( ispersonalid(ident) ){
		fore[0] = 0;
		vmsg(SHM->i18nstr[cuser.language][2346]);
	    }
	}
#endif
	while (1) {
	    getfield(8, 
#ifdef FOREIGN_REG
                     SHM->i18nstr[cuser.language][2347],
#else
                     SHM->i18nstr[cuser.language][2348],
#endif
                     SHM->i18nstr[cuser.language][2349], rname, 20);
	    if( (errcode = isvalidname(rname)) == NULL )
		break;
	    else
		vmsg(errcode);
	}

	move(11, 0);
	prints(SHM->i18nstr[cuser.language][2350]
	       );
	while (1) {
	    getfield(9, SHM->i18nstr[cuser.language][2351],
		     SHM->i18nstr[cuser.language][2352], career, 40);
	    if( (errcode = isvalidcareer(career)) == NULL )
		break;
	    else
		vmsg(errcode);
	}
	while (1) {
	    getfield(11, SHM->i18nstr[cuser.language][2353],
		     SHM->i18nstr[cuser.language][2354], addr, 50);
	    if( (errcode = isvalidaddr(addr)) == NULL
#ifdef FOREIGN_REG
                && fore[0] == 0 
#endif
		)
		break;
	    else
		vmsg(errcode);
	}
	while (1) {
	    getfield(13, SHM->i18nstr[cuser.language][2355], SHM->i18nstr[cuser.language][2356], phone, 11);
	    if( (errcode = isvalidphone(phone)) == NULL )
		break;
	    else
		vmsg(errcode);
	}
	getfield(15, SHM->i18nstr[cuser.language][2357],
		 SHM->i18nstr[cuser.language][2358], mobile, 20);
	while (1) {
	    int             len;

	    getfield(17, SHM->i18nstr[cuser.language][2359], SHM->i18nstr[cuser.language][2360], birthday, 9);
	    len = strlen(birthday);
	    if (!len) {
		snprintf(birthday, 9, "%02i/%02i/%02i",
			 cuser.month, cuser.day, cuser.year % 100);
		mon = cuser.month;
		day = cuser.day;
		year = cuser.year;
	    } else if (len == 8) {
		mon = (birthday[0] - '0') * 10 + (birthday[1] - '0');
		day = (birthday[3] - '0') * 10 + (birthday[4] - '0');
		year = (birthday[6] - '0') * 10 + (birthday[7] - '0');
	    } else{
		vmsg(SHM->i18nstr[cuser.language][2361]);
		continue;
	    }
	    if (mon > 12 || mon < 1 || day > 31 || day < 1 || year > 90 ||
		year < 40){
		vmsg(SHM->i18nstr[cuser.language][2362]);
		continue;
	    }
	    break;
	}
	getfield(19, SHM->i18nstr[cuser.language][2363], SHM->i18nstr[cuser.language][2364], sex_is, 2);
	getdata(20, 0, SHM->i18nstr[cuser.language][2365],
		ans, 3, LCECHO);
	if (ans[0] == 'q')
	    return 0;
	if (ans[0] == 'y')
	    break;
    }
    strlcpy(cuser.ident, ident,11);
    strlcpy(cuser.realname, rname, 20);
    strlcpy(cuser.address, addr, 50);
    strlcpy(cuser.email, email, 50);
    cuser.mobile = atoi(mobile);
    cuser.sex = (sex_is[0] - '1') % 8;
    cuser.month = mon;
    cuser.day = day;
    cuser.year = year;
#ifdef FOREIGN_REG
    if (fore[0])
	cuser.uflag2 |= FOREIGN;
    else
	cuser.uflag2 &= ~FOREIGN;
#endif
    trim(career);
    trim(addr);
    trim(phone);

    toregister(email, genbuf, phone, career, ident, rname, addr, mobile);

    clear();
    move(9, 3);
    prints(SHM->i18nstr[cuser.language][2366]);
    pressanykey();
    cuser.userlevel |= PERM_POST;
    brc_initial_board("WhoAmI");
    set_board();
    do_post();
    cuser.userlevel &= ~PERM_POST;
    return 0;
}

/* 列出所有註冊使用者 */
static int      usercounter, totalusers;
static unsigned short u_list_special;

static int
u_list_CB(int num, userec_t * uentp)
{
    static int      i;
    char            permstr[8], *ptr;
    register int    level;

    if (uentp == NULL) {
	move(2, 0);
	clrtoeol();
	prints(SHM->i18nstr[cuser.language][2367],
	       SHM->i18nstr[cuser.language][2368],
	       HAS_PERM(PERM_SEEULEVELS) ? SHM->i18nstr[cuser.language][2369] : "");
	i = 3;
	return 0;
    }
    if (bad_user_id(uentp->userid))
	return 0;

    if ((uentp->userlevel & ~(u_list_special)) == 0)
	return 0;

    if (i == b_lines) {
	prints(SHM->i18nstr[cuser.language][2370],
	       usercounter, totalusers, usercounter * 100 / totalusers);
	i = igetch();
	if (i == 'q' || i == 'Q')
	    return QUIT;
	i = 3;
    }
    if (i == 3) {
	move(3, 0);
	clrtobot();
    }
    level = uentp->userlevel;
    strlcpy(permstr, "----", 8);
    if (level & PERM_SYSOP)
	permstr[0] = 'S';
    else if (level & PERM_ACCOUNTS)
	permstr[0] = 'A';
    else if (level & PERM_SYSOPHIDE)
	permstr[0] = 'p';

    if (level & (PERM_BOARD))
	permstr[1] = 'B';
    else if (level & (PERM_BM))
	permstr[1] = 'b';

    if (level & (PERM_XEMPT))
	permstr[2] = 'X';
    else if (level & (PERM_LOGINOK))
	permstr[2] = 'R';

    if (level & (PERM_CLOAK | PERM_SEECLOAK))
	permstr[3] = 'C';

    ptr = (char *)Cdate(&uentp->lastlogin);
    ptr[18] = '\0';
    prints("%-14s %-27.27s%5d %5d  %s  %s\n",
	   uentp->userid,
	   uentp->username,
	   uentp->numlogins, uentp->numposts,
	   HAS_PERM(PERM_SEEULEVELS) ? permstr : "", ptr);
    usercounter++;
    i++;
    return 0;
}

int
u_list()
{
    char            genbuf[3];

    setutmpmode(LAUSERS);
    u_list_special = usercounter = 0;
    totalusers = SHM->number;
    if (HAS_PERM(PERM_SEEULEVELS)) {
	getdata(b_lines - 1, 0, SHM->i18nstr[cuser.language][2371],
		genbuf, 3, DOECHO);
	if (genbuf[0] != '2')
	    u_list_special = PERM_BASIC | PERM_CHAT | PERM_PAGE | PERM_POST | PERM_LOGINOK | PERM_BM;
    }
    u_list_CB(0, NULL);
    if (passwd_apply(u_list_CB) == -1) {
	outs(msg_nobody);
	return XEASY;
    }
    move(b_lines, 0);
    clrtoeol();
    prints(SHM->i18nstr[cuser.language][2372], usercounter, totalusers);
    igetch();
    return 0;
}
