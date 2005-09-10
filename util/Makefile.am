########################################################################
# PTT BBS Automake
# Create: piaip, Sat Sep 10 10:36:56 CST 2005
########################################################################
# $Id$

CFLAGS=-DPTTBBS_UTIL
BBSBASE=../include/var.h

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
	chesscountry	tunepasswd	buildir		xchatd

# Compile directly.
CPROG_WITHOUT_UTIL= \
	countalldice	bbsrf		bbsmail		initbbs		\
	gamble_fix	merge_board	merge_passwd	showboard	\
	userlist	uhash_loader

# No need to be compiled.
SCRIPT_PROG = \
	BM_money.sh	backpasswd.sh	mailog.sh	opendice.sh	\
	openticket.sh	stock.sh	topsong.sh	weather.sh	\
	stock.perl	weather.perl	toplazyBM.sh	toplazyBBM.sh	\
	dailybackup.pl	tarqueue.pl	waterball.pl	filtermail.pl	\
	getbackup.pl	udnnews.pl	rebuildaloha.pl

bin_PRORAMS=$(SCRIPT_PROG) $(CPROG_WITHOUT_UTIL) $(CPROG_WITH_UTIL) 	\
	    shmctl

UTIL_LIB=	\
	util_cache.c	util_record.c	util_passwd.c	util_var.c	\
	util_stuff.c	util_osdep.c	util_args.c	util_file.c	\
	util_crypt.c

MBBSD_OBJS= \
	cache		record		passwd		var		\
	stuff		osdep		args		file		\
	crypt

../include/var.h: ../mbbsd/var.c
	cd ../mbbsd; $(MAKE) ../include/var.h

.for fn in ${CPROG_WITH_UTIL}
${fn}: ${BBSBASE} ${fn}.c ${UTIL_OBJS}
	$(CCACHE) ${CC} ${CFLAGS} ${LDFLAGS} -o ${fn} ${UTIL_OBJS} ${fn}.c
.endfor

.for fn in ${MBBSD_OBJS}
util_${fn}.o: ${BBSBASE} ../mbbsd/${fn}.c
	$(CCACHE) ${CC} ${CFLAGS} -D_BBS_UTIL_C_ -c -o $@ ../mbbsd/${fn}.c
.endfor

shmctl: ${BBSBASE} shmctl.c ${UTIL_OBJS}
	$(CCACHE) ${CC} ${CFLAGS} ${LDFLAGS} -o shmctl ${UTIL_OBJS} shmctl.c

bbsmail: ${BBSBASE} bbsmail.c ../innbbsd/str_decode.c $(UTIL_OBJS)
	$(CCACHE) $(CC) $(CFLAGS) $(LDFLAGS) -o bbsmail -DUSE_ICONV \
	bbsmail.c ../innbbsd/str_decode.c $(UTIL_OBJS)

install: $(PROGS)
	install -d $(BBSHOME)/bin/
	install -c -m 755 $(PROGS) $(BBSHOME)/bin/
	chmod 4755 $(BBSHOME)/bin/post
.if defined(WITHFILTERMAIL)
	$(MAKE) installfiltermail
.endif

clean:
	rm -f *.o $(CPROGS) $(CPROG_WITH_UTIL) $(CPROG_WITHOUT_UTIL)


installfiltermail:
	mv $(BBSHOME)/bin/bbsmail $(BBSHOME)/bin/realbbsmail
	ln -s $(BBSHOME)/bin/filtermail.pl $(BBSHOME)/bin/bbsmail

# for diskstat(FreeBSD 4.x only) .
# diskstat should be compiled with bbs and installed with root
diskstat: diskstat.c
	$(CCACHE) $(CC) $(CFLAGS) -o diskstat diskstat.c -ldevstat -lkvm

installdiskstat: diskstat
	cp -f diskstat /usr/local/bin/
	chgrp kmem /usr/local/bin/diskstat
	chmod 2755 /usr/local/bin/diskstat

# for bbsctl. bbsctl should be compiled with bbs and installed with root
bbsctl:	bbsctl.c
	$(CCACHE) $(CC) $(CFLAGS) -o $@ $@.c

installbbsctl: bbsctl
	rm -f /home/bbs/bin/bbsctl
	cp /home/bbs/pttbbs/util/bbsctl /home/bbs/bin/bbsctl
	chown root /home/bbs/bin/bbsctl
	chmod 4755 /home/bbs/bin/bbsctl

cleanpasswd: cleanpasswd.c ${UTIL_OBJS}
	$(CCACHE) ${CC} ${CFLAGS} ${LDFLAGS} -o cleanpasswd ${UTIL_OBJS} cleanpasswd.c

r2014transfer: r2014convert
	$(CCACHE) ${CC} ${CFLAGS} ${LDFLAGS} -o r2014convert r2014convert.c
	./r2014convert
	rm r2014convert

passwdconverter: passwdconverter.c
	$(CCACHE) $(CC) $(CFLAGS) $(LDFLAGS) $(UTIL_OBJS) -o passwdconverter passwdconverter.c
