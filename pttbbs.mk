# $Id$
# 定義基本初值
BBSHOME?=	$(HOME)
BBSHOME?=	/home/bbs
OS!=		uname
OSTYPE?=	$(OS)
CC?=		gcc
CCACHE!=	which ccache|sed -e 's/^.*\///'
PTT_CFLAGS=	-Wall -pipe -DBBSHOME='"$(BBSHOME)"' -I../include
PTT_LDFLAGS=	-pipe -Wall -L/usr/local/lib
PTT_LIBS=	-lcrypt

# enable assert()
#PTT_CFLAGS+=	-DNDEBUG 

# FreeBSD特有的環境
CFLAGS_FreeBSD=	-DHAVE_SETPROCTITLE -DFreeBSD -I/usr/local/include
LDFLAGS_FreeBSD=
LIBS_FreeBSD=	-lkvm -liconv

# Linux特有的環境
CFLAGS_Linux=	-DHAVE_DES_CRYPT
LDFLAGS_Linux=	-pipe -Wall 
LIBS_Linux=	

# SunOS特有的環境
CFLAGS_Solaris= -DSolaris -I/usr/local/include 
LDFLAGS_Solaris= -L/usr/local/lib -L/usr/lib/
LIBS_Solaris= -lnsl -lsocket -liconv


# CFLAGS, LDFLAGS, LIBS 加入 OS 相關參數
PTT_CFLAGS+=	$(CFLAGS_$(OSTYPE))
PTT_LDFLAGS+=	$(LDFLAGS_$(OSTYPE))
PTT_LIBS+=	$(LIBS_$(OSTYPE))

# 若有定義 PROFILING
.if defined(PROFILING)
PTT_CFLAGS+=	-pg
PTT_LDFLAGS+=	-pg
NO_OMITFP=	yes
NO_FORK=	yes
.endif

.if defined(USE_ICC)
CC=		icc
CFLAGS=		$(PTT_CFLAGS) -O1 -tpp6 -mcpu=pentiumpro -march=pentiumiii \
		-ip -ipo
LDFLAGS+=	-O1 -tpp6 -mcpu=pentiumpro -march=pentiumiii -ip -ipo \
		$(PTT_LDFLAGS) $(PTT_LIBS)
.else
# 若有定義 GDB或 DEBUG, 則加入 -g , 否則用 -O
.if defined(GDB) || defined(DEBUG)
CFLAGS=		-g $(PTT_CFLAGS)
LDFLAGS=	-g $(PTT_LDFLAGS) $(PTT_LIBS)
.else
CFLAGS+=	-Os -fstrength-reduce \
		-fthread-jumps -fexpensive-optimizations \
		$(PTT_CFLAGS) $(EXT_CFLAGS)
LDFLAGS+=	-Os $(PTT_LDFLAGS) $(PTT_LIBS)

.if !defined(NO_OMITFP)
CFLAGS+=	-fomit-frame-pointer
.endif
.endif
.endif

# 若有定義 DEBUG, 則在 CFLAGS內定義 DEBUG
.if defined(DEBUG)
CFLAGS+=	-DDEBUG
.endif

# 若有定義 NO_FORK, 則在 CFLAGS內定義 NO_FORK
.if defined(NO_FORK)
CFLAGS+=	-DNO_FORK
.endif
