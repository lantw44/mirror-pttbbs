/* $Id$ */
#define INCLUDE_VAR_H
#include "bbs.h"

char           *str_permid[] = {
    "���v�O",			/* PERM_BASIC */
    "�i�J��ѫ�",		/* PERM_CHAT */
    "��H���",			/* PERM_PAGE */
    "�o��峹",			/* PERM_POST */
    "���U�{�ǻ{��",		/* PERM_LOGINOK */
    "�H��L�W��",		/* PERM_MAILLIMIT */
    "�����N",			/* PERM_CLOAK */
    "�ݨ��Ԫ�",			/* PERM_SEECLOAK */
    "�ä[�O�d�b��",		/* PERM_XEMPT */
    "���������N",		/* PERM_DENYPOST */
    "�O�D",			/* PERM_BM */
    "�b���`��",			/* PERM_ACCOUNTS */
    "��ѫ��`��",		/* PERM_CHATCLOAK */
    "�ݪO�`��",			/* PERM_BOARD */
    "����",			/* PERM_SYSOP */
    "BBSADM",			/* PERM_POSTMARK */
    "���C�J�Ʀ�]",		/* PERM_NOTOP */
    "�H�k�q�r��",		/* PERM_VIOLATELAW */
    "���������~���H",		/* PERM_ */
    "�S�Q��",			/* PERM_ */
    "��ı����",			/* PERM_VIEWSYSOP */
    "�[��ϥΪ̦���",		/* PERM_LOGUSER */
    "��ذ��`��z�v",		/* PERM_Announce */
    "������",			/* PERM_RELATION */
    "�S�Ȳ�",			/* PERM_SMG */
    "�{����",			/* PERM_PRG */
    "���ʲ�",			/* PERM_ACTION */
    "���u��",			/* PERM_PAINT */
    "�ߪk��",			/* PERM_LAW */
    "�p�ժ�",			/* PERM_SYSSUBOP */
    "�@�ťD��",			/* PERM_LSYSOP */
    "�ޢ���"			/* PERM_PTT */
};

char           *str_permboard[] = {
    "���i Zap",			/* BRD_NOZAP */
    "���C�J�έp",		/* BRD_NOCOUNT */
    "����H",			/* BRD_NOTRAN */
    "�s�ժO",			/* BRD_GROUPBOARD */
    "���êO",			/* BRD_HIDE */
    "����(���ݳ]�w)",		/* BRD_POSTMASK */
    "�ΦW�O",			/* BRD_ANONYMOUS */
    "�w�]�ΦW�O",		/* BRD_DEFAULTANONYMOUS */
    "�H�k��i���ݪO",		/* BRD_BAD */
    "�s�p�M�άݪO",		/* BRD_VOTEBOARD */
    "�wĵ�i�n�o��",		/* BRD_WARNEL */
    "�����ݪO�s��",		/* BRD_TOP */
    "���i����",                 /* BRD_NORECOMMEND */
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
};

int             usernum;
int             currmode = 0;
int             curredit = 0;
int             showansi = 1;
int             paste_level;
int             currbid;
char            quote_file[80] = "\0";
char            quote_user[80] = "\0";
char            paste_title[STRLEN];
char            paste_path[256];
char            currtitle[TTLEN + 1] = "\0";
char            vetitle[TTLEN + 1] = "\0";
char            currowner[IDLEN + 2] = "\0";
char            currauthor[IDLEN + 2] = "\0";
char            currfile[FNLEN];/* current file name @ bbs.c mail.c */
char            currboard[IDLEN + 2];
char            currBM[IDLEN * 3 + 10];
char            reset_color[4] = "\033[m";
char            margs[64] = "\0";	/* main argv list */
pid_t           currpid;	/* current process ID */
time_t          login_start_time;
time_t          start_time;
time_t          paste_time;
userec_t        cuser;		/* current user structure */
userec_t        xuser;		/* lookup user structure */
crosspost_t     postrecord;	/* anti cross post */
unsigned int    currbrdattr;
unsigned int    currstat;
unsigned char   currfmode;	/* current file mode */

/* global string variables */
/* filename */

char           *fn_passwd = FN_PASSWD;
char           *fn_board = FN_BOARD;
char           *fn_note_ans = FN_NOTE_ANS;
char           *fn_register = "register.new";
char           *fn_plans = "plans";
char           *fn_writelog = "writelog";
char           *fn_talklog = "talklog";
char           *fn_overrides = FN_OVERRIDES;
char           *fn_reject = FN_REJECT;
char           *fn_canvote = FN_CANVOTE;
char           *fn_notes = "notes";
char           *fn_water = FN_WATER;
char           *fn_visable = FN_VISABLE;
char           *fn_mandex = "/.Names";
char           *fn_proverb = "proverb";

/* are descript in userec.loginview */

char           *loginview_file[NUMVIEWFILE][2] = {
    {FN_NOTE_ANS, "�Ĳ��W���y���O"},
    {FN_TOPSONG, "�I�q�Ʀ�]"},
    {"etc/topusr", "�Q�j�Ʀ�]"},
    {"etc/topusr100", "�ʤj�Ʀ�]"},
    {"etc/birth.today", "����جP"},
    {"etc/weather.tmp", "�Ѯ�ֳ�"},
    {"etc/stock.tmp", "�ѥ��ֳ�"},
    {"etc/day", "����Q�j���D"},
    {"etc/week", "�@�g���Q�j���D"},
    {"etc/today", "���ѤW���H��"},
    {"etc/yesterday", "�Q��W���H��"},
    {"etc/history", "���v�W������"},
    {"etc/topboardman", "��ذϱƦ�]"},
    {"etc/topboard.tmp", "�ݪO�H��Ʀ�]"}
};

/* message */
char           *msg_seperator = MSG_SEPERATOR;
char           *msg_mailer = MSG_MAILER;
char           *msg_shortulist = MSG_SHORTULIST;

char           *msg_cancel = MSG_CANCEL;
char           *msg_usr_left = MSG_USR_LEFT;
char           *msg_nobody = MSG_NOBODY;

char           *msg_sure_ny = MSG_SURE_NY;
char           *msg_sure_yn = MSG_SURE_YN;

char           *msg_bid = MSG_BID;
char           *msg_uid = MSG_UID;

char           *msg_del_ok = MSG_DEL_OK;
char           *msg_del_ny = MSG_DEL_NY;

char           *msg_fwd_ok = MSG_FWD_OK;
char           *msg_fwd_err1 = MSG_FWD_ERR1;
char           *msg_fwd_err2 = MSG_FWD_ERR2;

char           *err_board_update = ERR_BOARD_UPDATE;
char           *err_bid = ERR_BID;
char           *err_uid = ERR_UID;
char           *err_filename = ERR_FILENAME;

char           *str_mail_address = "." BBSUSER "@" MYHOSTNAME;
char           *str_new = "new";
char           *str_reply = "Re: ";
char           *str_space = " \t\n\r";
char           *str_sysop = "SYSOP";
char           *str_author1 = STR_AUTHOR1;
char           *str_author2 = STR_AUTHOR2;
char           *str_post1 = STR_POST1;
char           *str_post2 = STR_POST2;
char           *BBSName = BBSNAME;

/* #define MAX_MODES 78 */
/* MAX_MODES is defined in common.h */

char           *ModeTypeTable[MAX_MODES] = {
    "�o�b",			/* IDLE */
    "�D���",			/* MMENU */
    "�t�κ��@",			/* ADMIN */
    "�l����",			/* MAIL */
    "��Ϳ��",			/* TMENU */
    "�ϥΪ̿��",		/* UMENU */
    "XYZ ���",			/* XMENU */
    "�����ݪO",			/* CLASS */
    "Play���",			/* PMENU */
    "�s�S�O�W��",		/* NMENU */
    "��tt�q�c��",		/* PSALE */
    "�o��峹",			/* POSTING */
    "�ݪO�C��",			/* READBRD */
    "�\\Ū�峹",		/* READING */
    "�s�峹�C��",		/* READNEW */
    "��ܬݪO",			/* SELECT */
    "Ū�H",			/* RMAIL */
    "�g�H",			/* SMAIL */
    "��ѫ�",			/* CHATING */
    "��L",			/* XMODE */
    "�M��n��",			/* FRIEND */
    "�W�u�ϥΪ�",		/* LAUSERS */
    "�ϥΪ̦W��",		/* LUSERS */
    "�l�ܯ���",			/* MONITOR */
    "�I�s",			/* PAGE */
    "�d��",			/* TQUERY */
    "���",			/* TALK  */
    "�s�W����",			/* EDITPLAN */
    "�sñ�W��",			/* EDITSIG */
    "�벼��",			/* VOTING */
    "�]�w���",			/* XINFO */
    "�H������",			/* MSYSOP */
    "�L�L�L",			/* WWW */
    "���j�ѤG",			/* BIG2 */
    "�^��",			/* REPLY */
    "�Q���y����",		/* HIT */
    "���y�ǳƤ�",		/* DBACK */
    "���O��",			/* NOTE */
    "�s��峹",			/* EDITING */
    "�o�t�γq�i",		/* MAILALL */
    "�N���",			/* MJ */
    "�q���ܤ�",			/* P_FRIEND */
    "�W���~��",			/* LOGIN */
    "�d�r��",			/* DICT */
    "�����P",			/* BRIDGE */
    "���ɮ�",			/* ARCHIE */
    "���a��",			/* GOPHER */
    "��News",			/* NEWS */
    "���Ѳ��;�",		/* LOVE */
    "�s�軲�U��",		/* EDITEXP */
    "�ӽ�IP��}",		/* IPREG */
    "���޿줽��",		/* NetAdm */
    "������~�{",		/* DRINK */
    "�p���",			/* CAL */
    "�s��y�k��",		/* PROVERB */
    "���G��",			/* ANNOUNCE */
    "��y���O",			/* EDNOTE */
    "�^�~½Ķ��",		/* CDICT */
    "�˵��ۤv���~",		/* LOBJ */
    "�I�q",			/* OSONG */
    "���b���p��",		/* CHICKEN */
    "���m��",			/* TICKET */
    "�q�Ʀr",			/* GUESSNUM */
    "�C�ֳ�",			/* AMUSE */
    "�¥մ�",			/* OTHELLO */
    "����l",			/* DICE */
    "�o�����",			/* VICE */
    "�G�G��ing",		/* BBCALL */
    "ú�@��",			/* CROSSPOST */
    "���l��",			/* M_FIVE */
    "21�Iing",			/* JACK_CARD */
    "10�I�bing",		/* TENHALF */
    "�W�ŤE�Q�E",		/* CARD_99 */
    "�����d��",			/* RAIL_WAY */
    "�j�M���",			/* SREG */
    "�U�H��",			/* CHC */
    "�U�t�X",			/* DARK */
    "NBA�j�q��"			/* TMPJACK */
    "��tt�d�]�t��",		/* JCEE */
    "���s�峹"			/* REEDIT */
    "������",                   /* BLOGGING */
    "", /* for future usage */
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    ""
};

/* indict.c */
char            dict[41], database[41];

/* term.c */
int             b_lines = 23;
int             t_lines = 24;
int             p_lines = 20;
int             t_columns = 80;
char           *strtstandout = "\33[7m";
int             strtstandoutlen = 4;
char           *endstandout = "\33[m";
int             endstandoutlen = 3;
char           *clearbuf = "\33[H\33[J";
int             clearbuflen = 6;
char           *cleolbuf = "\33[K";
int             cleolbuflen = 3;
char           *scrollrev = "\33M";
int             scrollrevlen = 2;
int             automargins = 1;

/* io.c */
time_t          now;
int             KEY_ESC_arg;
int             watermode = -1;
int             wmofo = -1;
/*
 * WATERMODE(WATER_ORIG) | WATERMODE(WATER_NEW): Ptt ���y�^�U e = -1
 * �S�b�^���y = 0   �b�^�W�@�����y  (Ctrl-R) > 0   �b�^�e n �����y (Ctrl-R
 * Ctrl-R)
 * 
 * WATERMODE(WATER_OFO)  by in2 wmofo     = -1  �S�b�^���y = 0   ���b�^���y = 1
 * �^���y���S������y wmofo     >=0 �ɦ�����y�N�u���, ���|��water[]��,
 * �ݦ^�����y���ɭԤ@���g�J.
 */


/* cache.c */
int             numboards = -1;
int            *GLOBALVAR;
SHM_t          *SHM;
boardheader_t  *bcache;
userinfo_t     *currutmp;

/* board.c */
int             class_bid = 0;
int             brc_num;
int             brc_list[BRC_MAXNUM];

/* read.c */
int             TagNum;			/* tag's number */
TagItem         TagList[MAXTAGS];	/* ascending list */
int		TagBoard = -1;		/* TagBoard = 0 : user's mailbox
					   TagBoard > 0 : bid where last taged*/
char            currdirect[64];

/* edit.c */
char            save_title[STRLEN];

/* bbs.c */
time_t          board_visit_time;
char            real_name[IDLEN + 1];
int             local_article;

/* mbbsd.c */
int             talkrequest = NA;
char            fromhost[STRLEN] = "\0";
char            water_usies = 0;
FILE           *fp_writelog = NULL;
water_t         water[6], *swater[6], *water_which = &water[0];

/* announce.c */
char            trans_buffer[256];

/* screen.c */
screenline_t   *big_picture = NULL;
unsigned char   scr_lns;
unsigned short  scr_cols;
char            roll;

/* name.c */
word_t         *toplev;

#ifndef _BBS_UTIL_C_
/* menu.c */
commands_t      cmdlist[] = {
    {admin, PERM_SYSOP | PERM_VIEWSYSOP, "00Admin       �i �t�κ��@�� �j"},
    {Announce, 0, "AAnnounce     �i ��ؤ��G�� �j"},
    {Boards, 0, "FFavorite     �i �� �� �̷R �j"},
    {root_board, 0, "CClass        �i ���հQ�װ� �j"},
    {Mail, PERM_BASIC, "MMail         �i �p�H�H��� �j"},
    {Talk, 0, "TTalk         �i �𶢲�Ѱ� �j"},
    {User, 0, "UUser         �i �ӤH�]�w�� �j"},
    {Xyz, 0, "XXyz          �i �t�Τu��� �j"},
    {Play_Play, PERM_BASIC, "PPlay         �i �T��/�𶢥ͬ��j"},
    {Name_Menu, PERM_LOGINOK, "NNamelist     �i �s�S�O�W�� �j"},
    {Goodbye, 0, "GGoodbye       ���}�A�A���K�K"},
    {NULL, 0, NULL}
};
#endif

/* friend.c */
/* Ptt �U�دS�O�W�檺�ɦW */
char           *friend_file[8] = {
    FN_OVERRIDES,
    FN_REJECT,
    "alohaed",
    "postlist",
    "",
    FN_CANVOTE,
    FN_WATER,
    FN_VISABLE
};

#ifdef PTTBBS_UTIL
    #ifdef OUTTA_TIMER
	#define COMMON_TIME (SHM->GV2.e.now)
    #else
	#define COMMON_TIME (time(0))
    #endif
#else
    #define COMMON_TIME (now)
#endif
