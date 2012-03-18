#!/usr/bin/env python
# -*- coding: Big5 -*-

import collections
import struct

IDLEN = 12
IPV4LEN = 15
PASSLEN = 14
REGLEN = 38

PASSWD_VERSION = 4194

#define PASSWD_VERSION	4194

"""
typedef struct userec_t {
    uint32_t	version;	/* version number of this sturcture, we
    				 * use revision number of project to denote.*/

    char	userid[IDLEN+1];/* �ϥΪ�ID */
    char	realname[20];	/* �u��m�W */
    char	nickname[24];	/* �ʺ� */
    char	passwd[PASSLEN];/* �K�X */

    char	pad_1;

    uint32_t    uflag;		/* �ߺD1 , see uflags.h */
    uint32_t    deprecated_uflag2;		/* deprecated: �ߺD2 , see uflags.h */
    uint32_t    userlevel;	/* �v�� */
    uint32_t    numlogindays;	/* �W�u��� (�C��̦h+1���n�J����) */
    uint32_t    numposts;	/* �峹�g�� */
    time4_t	firstlogin;	/* ���U�ɶ� */
    time4_t	lastlogin;	/* �̪�W���ɶ�(�]�t����) */
    char	lasthost[IPV4LEN+1];/* �W���W���ӷ� */
    int32_t     money;		/* Ptt�� */

    char	unused_1[4];

    char	email[50];	/* Email */
    char	address[50];	/* ��} */
    char	justify[REGLEN + 1];/* �f�ָ�� */
    uint8_t	month;		/* �ͤ� �� */
    uint8_t	day;		/* �ͤ� �� */
    uint8_t	year;		/* �ͤ� �~ */
    uint8_t     nonuse_sex;     /* �ʧO (�w����), ���M�ŹL */

    uint8_t	pager_ui_type;	/* �I�s���ɭ����O (was: WATER_*) */
    uint8_t	pager;		/* �I�s�����A */
    uint8_t	invisible;	/* ���Ϊ��A */

    char	unused_3[2];

    uint32_t    exmailbox;	/* �ʶR�H�c�� TODO short �N���F */

    // r3968 ���X sizeof(chicken_t)=128 bytes
    char	chkpad0[4];

    char	career[40];	/* �Ǿ�¾�~ */
    char	phone[20];	/* �q�� */

    uint32_t	old_numlogins;	/* �ഫ�e�� numlogins, �ƥ��˵��� */
    char	chkpad1[48];
    time4_t	lastseen;	/* �̪�W���ɶ�(�������p) */
    time4_t	chkpad2[2];	/* in case ���H�ѤF�� time4_t �զn... */
    // �H�W���� sizeof(chicken_t) �P���j�p
    
    time4_t	lastsong;	/* �W���I�q�ɶ� */
    uint32_t	loginview;	/* �i���e�� */

    uint8_t	unused_4;	// was: channel
    uint8_t	pad_2;

    uint16_t	vl_count;	/* �H�k�O�� ViolateLaw counter */
    uint16_t	five_win;	/* ���l�Ѿ��Z �� */
    uint16_t	five_lose;	/* ���l�Ѿ��Z �� */
    uint16_t	five_tie;	/* ���l�Ѿ��Z �M */
    uint16_t	chc_win;	/* �H�Ѿ��Z �� */
    uint16_t	chc_lose;	/* �H�Ѿ��Z �� */
    uint16_t	chc_tie;	/* �H�Ѿ��Z �M */
    int32_t     mobile;		/* ������X */
    char	mind[4];	/* �߱� XXX not a null-terminate string */
    uint16_t	go_win;		/* ��Ѿ��Z �� */
    uint16_t	go_lose;	/* ��Ѿ��Z �� */
    uint16_t	go_tie;		/* ��Ѿ��Z �M */

    char	unused_5[5];	/* �q�e�� ident �����Ҧr���A�ϥΫe�Х��M0 */

    uint8_t	signature;	/* �D��ñ�W�� */
    uint8_t	goodpost;	/* �������n�峹�� */
    uint8_t	badpost;	/* �������a�峹�� */
    uint8_t	unused_6;	/* �q�e���v�Цn��(goodsale), �ϥΫe�Х��M0 */
    uint8_t	unused_7;	/* �q�e���v���a��(badsale),  �ϥΫe�Х��M0 */
    char	myangel[IDLEN+1];/* �ڪ��p�Ѩ� */

    char	pad_3;

    uint16_t	chess_elo_rating;/* �H�ѵ��Ť� */
    uint32_t    withme;		/* �ڷQ��H�U�ѡA���.... */
    time4_t	timeremovebadpost;/* �W���R���H��ɶ� */
    time4_t	timeviolatelaw; /* �Q�}�@��ɶ� */

    char	pad_tail[28];
} PACKSTRUCT userec_t;
"""

BTLEN = 48

BOARDHEADER_SIZE = 256
BOARDHEADER_FMT = (
    ("brdname": "%ds" % (IDLEN + 1)),
    ("title": "%ds" % (BTLEN + 1)),
    ("BM": "%ds" % (IDLEN * 3 + 3)),
    ("pad1": "3s"),
    ("brdattr": "I"),  # uint32_t
    ("chesscountry": "B"),
    ("vote_limit_posts": "B"),
    ("vote_limit_logins": "B"),
    ("pad2_1": "1B"),
    ("bupdate": "I"),  # time4_t
    ("post_limit_posts": "B"),
    ("post_limit_logins": "B"),
    ("pad2_2": "1B"),
    ("bvote": "1B"),
    ("vtime": "I"),  # time4_t
    ("level": "I"),  # uint32_t
    ("perm_reload": "I"),  # time4_t
    ("gid": "I"),  # uint32_t
    ("next": "2I"),  # uint32_t
    ("firstchild": "2I"),  # uint32_t
    ("parent": "I"),  # uint32_t
    ("childcount": "I"),  # uint32_t
    ("nuser": "I"),  # uint32_t
    ("postexpire": "I"),  # uint32_t
    ("endgamble": "I"),  # time4_t
    ("posttype", "33s"),
    ("posttype_f", "B"),
    ("fastrecommend_pause", "B"),
    ("vote_limit_badpost": "B"),
    ("post_limit_badpost": "B"),
    ("pad3", "3s"),
    ("SRexpire", "I"),  # time4_t
    ("pad4", "40s"),
    )

BRD_NOCOUNT            = 0x00000002      # ���C�J�έp 
BRD_NOTRAN             = 0x00000004      # ����H 
BRD_GROUPBOARD         = 0x00000008      # �s�ժO 
BRD_HIDE               = 0x00000010      # ���êO (�ݪO�n�ͤ~�i��) 
BRD_POSTMASK           = 0x00000020      # ����o��ξ\Ū 
BRD_ANONYMOUS          = 0x00000040      # �ΦW�O 
BRD_DEFAULTANONYMOUS   = 0x00000080      # �w�]�ΦW�O 
BRD_NOCREDIT           = 0x00000100      # �o��L���y�ݪO 
BRD_VOTEBOARD          = 0x00000200      # �s�p���ݪO 
BRD_WARNEL             = 0x00000400      # �s�p���ݪO 
BRD_TOP                = 0x00000800      # �����ݪO�s�� 
BRD_NORECOMMEND        = 0x00001000      # ���i���� 
BRD_BLOG               = 0x00002000      # (�w����) ������ 
BRD_BMCOUNT            = 0x00004000      # �O�D�]�w�C�J�O�� 
BRD_SYMBOLIC           = 0x00008000      # symbolic link to board 
BRD_NOBOO              = 0x00010000      # ���i�N 
BRD_LOCALSAVE          = 0x00020000      # �w�] Local Save 
BRD_RESTRICTEDPOST     = 0x00040000      # �O�ͤ~��o�� 
BRD_GUESTPOST          = 0x00080000      # guest�� post 
BRD_COOLDOWN           = 0x00100000      # �N�R 
BRD_CPLOG              = 0x00200000      # �۰ʯd����O�� 
BRD_NOFASTRECMD        = 0x00400000      # �T��ֳt���� 
BRD_IPLOGRECMD         = 0x00800000      # ����O�� IP 
BRD_OVER18             = 0x01000000      # �Q�K�T 
BRD_NOREPLY            = 0x02000000      # ���i�^�� 
BRD_ALIGNEDCMT         = 0x04000000      # ����������� 
BRD_NOSELFDELPOST      = 0x08000000      # ���i�ۧR 


FNLEN = 28  # Length of filename
TTLEN = 64  # Length of title
DISP_TTLEN = 46

FILEHEADER_FMT = (
        ("filename", "%ds" % FNLEN),
        ("modified", "I"),
        ("pad", "B"),
        ("recommend", "b"),
        ("owner", "%ds" % (IDLEN + 2)),
        ("date", "6s"),
        ("title", "%ds" % (TTLEN + 1)),
        ("pad2", "B"),
        ("multi", "i"),
        ("filemode", "B"),
        ("pad3", "3s"))

FILEHEADER_SIZE = 128

def get_fileheader_format():
    return '<' + ''.join(value for _, value in FILEHEADER_FMT)

def unpack_fileheader(blob):
    fmt = get_fileheader_format()
    assert struct.calcsize(fmt) == FILEHEADER_SIZE
    return dict(zip((name for name, _ in FILEHEADER_FMT),
        struct.unpack_from(fmt, blob)))

FILE_LOCAL     = 0x01    # local saved,  non-mail
FILE_READ      = 0x01    # already read, mail only
FILE_MARKED    = 0x02    # non-mail + mail
FILE_DIGEST    = 0x04    # digest,       non-mail
FILE_REPLIED   = 0x04    # replied,      mail only
FILE_BOTTOM    = 0x08    # push_bottom,  non-mail
FILE_MULTI     = 0x08    # multi send,   mail only
FILE_SOLVED    = 0x10    # problem solved, sysop/BM non-mail only
FILE_HIDE      = 0x20    # hide,        in announce
FILE_BID       = 0x20    # bid,         in non-announce
FILE_BM        = 0x40    # BM only,     in announce
FILE_VOTE      = 0x40    # for vote,    in non-announce
FILE_ANONYMOUS = 0x80    # anonymous file

STRLEN = 80 # Length of most string data


if __name__ == '__main__':
    with open('/home/bbs/boards/A/ALLPOST/.DIR', 'rb') as f:
	entry = f.read(FILEHEADER_SIZE)
    header = unpack_fileheader(entry)
    print header
    print header['title'].decode('big5')
