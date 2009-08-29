/* $Id$ */
#define PWCU_IMPL
#include "bbs.h"

#ifdef _BBS_UTIL_C_
#error sorry, mbbsd/passwd.c does not support utility mode anymore. please use libcmbbs instead.
#endif

#ifdef CONST_CUSER
 #undef  cuser
 #define cuser pwcuser
#endif

void
passwd_force_update(int flag)
{
    if(!currutmp || (currutmp->alerts & ALERT_PWD) == 0)
	return;
    currutmp->alerts &= ~flag;
}

int 
initcuser(const char *userid)
{
    usernum = passwd_load_user(userid, &cuser);
    return usernum;
}

// XXX I don't like the stupid synchronization here,
// but simply following previous work here...
int
passwd_sync_update(int num, userec_t * buf)
{
    int alerts;

    if (num < 1 || num > MAX_USERS)
	return -1;

    // money update should be done before everything.
    buf->money = moneyof(num);

    if(usernum == num && currutmp && ((alerts = currutmp->alerts)  & ALERT_PWD))
    {
	userec_t u;
	if (passwd_sync_query(num, &u) != 0)
	    return -1;

	if(alerts & ALERT_PWD_BADPOST)
	   cuser.badpost = buf->badpost = u.badpost;
        if(alerts & ALERT_PWD_PERM)	
	   cuser.userlevel = buf->userlevel = u.userlevel;
        if(alerts & ALERT_PWD_JUSTIFY)	
	{
	    memcpy(buf->justify,  u.justify, sizeof(u.justify));
	    memcpy(cuser.justify, u.justify, sizeof(u.justify));
	    memcpy(buf->email,  u.email, sizeof(u.email));
	    memcpy(cuser.email, u.email, sizeof(u.email));
	}
	currutmp->alerts &= ~ALERT_PWD;

	// ALERT_PWD_RELOAD: reload all! No need to write.
	if (alerts & ALERT_PWD_RELOAD)
	{
	    memcpy(&cuser, &u, sizeof(u));
	    return 0;
	}
    }

    if (passwd_update(num, buf) != 0)
	return -1;

    return 0;
}

// XXX I don't like the stupid synchronization here,
// but simply following previous work here...
int
passwd_sync_query(int num, userec_t * buf)
{
    if (passwd_query(num, buf) < 0)
	return -1;

    return 0;
}

// pwcu*: current user password helpers

static int
pwcuInitCUser(userec_t *u)
{
    assert(usernum > 0 && usernum <= MAX_USERS);
    if (passwd_query(usernum, u) != 0)
	return -1;
    assert(strncmp(u->userid, cuser.userid, IDLEN) == 0);
    if    (strncmp(u->userid, cuser.userid, IDLEN) != 0)
	return -1;
    return 0;
}

static int
pwcuFinalCUser(userec_t *u)
{
    assert(usernum > 0 && usernum <= MAX_USERS);
    assert(strcmp(u->userid, cuser.userid) == 0);
    if (passwd_update(usernum, u) != 0)
	return -1;
    return 0;
}

#define PWCU_START()  userec_t u; if(pwcuInitCUser (&u) != 0) return -1
#define PWCU_END()    if (pwcuFinalCUser(&u) != 0) return -1; return 0

#define _ENABLE_BIT( var,mask) var |=  (mask)
#define _DISABLE_BIT(var,mask)	var &= ~(mask)

int pwcuBitEnableLevel	(unsigned int mask)
{
    PWCU_START();
    _ENABLE_BIT(    u.userlevel, mask);
    _ENABLE_BIT(cuser.userlevel, mask);
    PWCU_END();
}

int pwcuBitDisableLevel	(unsigned int mask)
{
    PWCU_START();
    _DISABLE_BIT(    u.userlevel, mask);
    _DISABLE_BIT(cuser.userlevel, mask);
    PWCU_END();
}

int 
pwcuIncNumPost()
{
    PWCU_START();
    cuser.numposts =  ++u.numposts;
    PWCU_END();
}

int 
pwcuDecNumPost()
{
    PWCU_START();
    if (u.numposts > 0)
	u.numposts--;
    cuser.numposts = u.numposts;
    PWCU_END();
}

int 
pwcuViolateLaw	()
{
    PWCU_START();
    _ENABLE_BIT(    u.userlevel, PERM_VIOLATELAW);
    _ENABLE_BIT(cuser.userlevel, PERM_VIOLATELAW);
    u.timeviolatelaw     = now;
    cuser.timeviolatelaw = u.timeviolatelaw;
    u.vl_count++;
    cuser.vl_count = u.vl_count;
    PWCU_END();
}

int
pwcuSaveViolateLaw()
{
    PWCU_START();
    _DISABLE_BIT(    u.userlevel, PERM_VIOLATELAW);
    _DISABLE_BIT(cuser.userlevel, PERM_VIOLATELAW);
    PWCU_END();
}

int 
pwcuAddExMailBox(int m)
{
    PWCU_START();
    u.exmailbox    += m;
    cuser.exmailbox = u.exmailbox;
    PWCU_END();
}

int pwcuSetLastSongTime (time4_t clk)
{
    PWCU_START();
    u.lastsong     = clk;
    cuser.lastsong = clk;
    PWCU_END();
}

int pwcuSetMyAngel	(const char *angel_uid)
{
    PWCU_START();
    strlcpy(    u.myangel, angel_uid, sizeof(    u.myangel));
    strlcpy(cuser.myangel, angel_uid, sizeof(cuser.myangel));
    PWCU_END();
}

int pwcuSetNickname	(const char *nickname)
{
    PWCU_START();
    strlcpy(    u.nickname, nickname, sizeof(    u.nickname));
    strlcpy(cuser.nickname, nickname, sizeof(cuser.nickname));
    PWCU_END();
}

int 
pwcuToggleOutMail()
{
    PWCU_START();
    u.uflag2     ^=  REJ_OUTTAMAIL;
    cuser.uflag2 &= ~REJ_OUTTAMAIL;
    cuser.uflag2 |= (REJ_OUTTAMAIL & u.uflag2);
    PWCU_END();
}

int 
pwcuSetLoginView(unsigned int bits)
{
    PWCU_START();
    u.loginview     = bits;
    cuser.loginview = u.loginview;
    PWCU_END();
}


// non-important variables (only save on exit)

int
pwcuSetSignature(unsigned char newsig)
{
    cuser.signature = newsig;
    return 0;
}

int 
pwcuSetWaterballMode(unsigned int bm)
{
    bm		 &=  WATER_MASK;
    cuser.uflag2 &= ~WATER_MASK;  
    cuser.uflag2 |= bm;  
    return 0;
}

int pwcuToggleSortBoard ()
{
    cuser.uflag ^= BRDSORT_FLAG;
    return 0;
}

int pwcuToggleFriendList()
{
    cuser.uflag ^= FRIEND_FLAG;
    return 0;
}

// session save

// XXX this is a little different - only invoked at login,
// which we should update/calculate every variables to log.
int pwcuLoginSave	()
{
    PWCU_START();   // XXX no need to reload for speed up?

    // new host from 'fromhost'
    strlcpy(cuser.lasthost, fromhost, sizeof(cuser.lasthost));

    // calculate numlogins (only increase one per each key)
    if (((login_start_time - cuser.firstlogin) % 86400) != 
	((cuser.lastlogin  - cuser.firstlogin) % 86400) )
	cuser.numlogins++;

    // update last login time
    cuser.lastlogin = login_start_time;

    if (!PERM_HIDE(currutmp))
	cuser.lastseen = login_start_time;

    PWCU_END();
}

// XXX this is a little different - only invoked at exist,
// so no need to sync back to cuser.
int pwcuExitSave	()
{
    PWCU_START();

    _DISABLE_BIT(u.uflag, (PAGER_FLAG | CLOAK_FLAG));
    if (currutmp->pager != PAGER_ON)
	_ENABLE_BIT(u.uflag, PAGER_FLAG);
    if (currutmp->invisible)
	_ENABLE_BIT(u.uflag, CLOAK_FLAG);

    u.invisible = currutmp->invisible;
    u.withme    = currutmp->withme;
    u.pager     = currutmp->pager;

    // XXX 當初設計的人把 mind 設計成非 NULL terminated 的...
    // assert(sizeof(u.mind) == sizeof(currutmp->mind));
    memcpy(u.mind,currutmp->mind, sizeof(u.mind));

    reload_money();

    PWCU_END();
}

// Initialization

void pwcuInitZero	()
{
    bzero(&cuser, sizeof(cuser));
}

int pwcuInitAdminPerm	()
{
    PWCU_START();
    cuser.userlevel = PERM_BASIC | PERM_CHAT | PERM_PAGE |
	PERM_POST | PERM_LOGINOK | PERM_MAILLIMIT |
	PERM_CLOAK | PERM_SEECLOAK | PERM_XEMPT |
	PERM_SYSOPHIDE | PERM_BM | PERM_ACCOUNTS |
	PERM_CHATROOM | PERM_BOARD | PERM_SYSOP | PERM_BBSADM;
    PWCU_END();
}

void pwcuInitGuestPerm	()
{
    cuser.userlevel = 0;
    cuser.uflag = PAGER_FLAG | BRDSORT_FLAG | MOVIE_FLAG;
    cuser.uflag2= 0; // we don't need FAVNEW_FLAG or anything else.
# ifdef GUEST_DEFAULT_DBCS_NOINTRESC
    _ENABLE_BIT(cuser.uflag, DBCS_NOINTRESC);
# endif
}

#undef  DIM
#define DIM(x) (sizeof(x)/sizeof(x[0]))

void pwcuInitGuestInfo	()
{
    int i;
    char *nick[] = {
	"椰子", "貝殼", "內衣", "寶特瓶", "翻車魚",
	"樹葉", "浮萍", "鞋子", "潛水艇", "魔王",
	"鐵罐", "考卷", "大美女"
    };

    i = random() % DIM(nick);
    snprintf(cuser.nickname, sizeof(cuser.nickname),
	    "海邊漂來的%s", nick[i]);
    strlcpy(currutmp->nickname, cuser.nickname,
	    sizeof(currutmp->nickname));
    strlcpy(cuser.realname, "guest", sizeof(cuser.realname));
    memset (cuser.mind, 0, sizeof(cuser.mind));
    cuser.sex = i % 8;
}
