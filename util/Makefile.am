########################################################################
# PTT BBS Automake
# Create: piaip, Sat Sep 10 10:36:56 CST 2005
########################################################################
# $Id$

AUTOMAKE_OPTIONS=foreign

CFLAGS=-DPTTBBS_UTIL
BBSBASE=../include/var.h
CFLAGS+=-Wall -pipe -DBBSHOME='"/home/bbs"' -I ../include

# Compile and build with BBS utility library.
CPROG_WITH_UTIL= \
	boardlist	BM_money	post		poststat	\
	jungo		account		birth		deluserfile	\
	expire		mandex		rmuid		horoscope	\
	openvice	parse_news	openticket	topusr		\
	yearsold	toplazyBM	toplazyBBM	writemoney	\
	reaper		buildAnnounce	inndBM		mailangel	\
	outmail		chkhbf		checkmoney      merge_dir       \
	transman	angel		gamblegive	checkdir	\
	chesscountry	tunepasswd	buildir		xchatd		\
	cleanpasswd	\
	shmctl

# Compile directly.
CPROG_WITHOUT_UTIL= \
	countalldice	bbsrf		bbsmail		initbbs		\
	gamble_fix	merge_board	merge_passwd	showboard	\
	userlist	uhash_loader	

# migration, not used in general case.
CPROG_NO_BUILD=r2014convert	passwdconverter


# No need to be compiled.
SCRIPT_PROG = \
	BM_money.sh	backpasswd.sh	mailog.sh	opendice.sh	\
	openticket.sh	stock.sh	topsong.sh	weather.sh	\
	stock.perl	weather.perl	toplazyBM.sh	toplazyBBM.sh	\
	dailybackup.pl	tarqueue.pl	waterball.pl	filtermail.pl	\
	getbackup.pl	udnnews.pl	rebuildaloha.pl

MBBSD_PATH=../mbbsd

MBBSD_LIB= \
	$(MBBSD_PATH)/cache.c	$(MBBSD_PATH)/record.c	\
	$(MBBSD_PATH)/passwd.c	$(MBBSD_PATH)/var.c	\
	$(MBBSD_PATH)/stuff.c	$(MBBSD_PATH)/osdep.c	\
	$(MBBSD_PATH)/args.c	$(MBBSD_PATH)/file.c	\
	$(MBBSD_PATH)/crypt.c

bin_PROGRAMS=$(CPROG_WITH_UTIL) $(CPROG_WITHOUT_UTIL) 

noinst_LIBRARIES=libmbbsd.a
libmbbsd_a_SOURCES=$(MBBSD_LIB) $(BBSBASE)
libmbbsd_a_CFLAGS=-D_BBS_UTIL_C_

LDADD=libmbbsd.a

$(BBSBASE): ../mbbsd/var.c
	cd ../mbbsd; $(MAKE) ../include/var.h

#bbsmail: ${BBSBASE} bbsmail.c ../innbbsd/str_decode.c $(UTIL_OBJS)
#	$(CCACHE) $(CC) $(CFLAGS) $(LDFLAGS) -o bbsmail -DUSE_ICONV \
#	bbsmail.c ../innbbsd/str_decode.c $(UTIL_OBJS)

#install: $(PROGS)
#	install -d $(BBSHOME)/bin/
#	install -c -m 755 $(PROGS) $(BBSHOME)/bin/
#	chmod 4755 $(BBSHOME)/bin/post
#.if defined(WITHFILTERMAIL)
#	$(MAKE) installfiltermail
#.endif

#installfiltermail:
#	mv $(BBSHOME)/bin/bbsmail $(BBSHOME)/bin/realbbsmail
#	ln -s $(BBSHOME)/bin/filtermail.pl $(BBSHOME)/bin/bbsmail

## for diskstat(FreeBSD 4.x only) .
## diskstat should be compiled with bbs and installed with root
#diskstat: diskstat.c
#	$(CCACHE) $(CC) $(CFLAGS) -o diskstat diskstat.c -ldevstat -lkvm

#installdiskstat: diskstat
#	cp -f diskstat /usr/local/bin/
#	chgrp kmem /usr/local/bin/diskstat
#	chmod 2755 /usr/local/bin/diskstat

# for bbsctl. bbsctl should be compiled with bbs and installed with root
#bbsctl:	bbsctl.c
#	$(CCACHE) $(CC) $(CFLAGS) -o $@ $@.c

#installbbsctl: bbsctl
#	rm -f /home/bbs/bin/bbsctl
#	cp /home/bbs/pttbbs/util/bbsctl /home/bbs/bin/bbsctl
#	chown root /home/bbs/bin/bbsctl
#	chmod 4755 /home/bbs/bin/bbsctl
