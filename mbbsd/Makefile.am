########################################################################
# PTT BBS Automake
# Create: piaip, Sat Sep 10 10:36:56 CST 2005
########################################################################
# $Id$

bin_PROGRAMS=mbbsd

mbbsd_SOURCES= \
	       ../include/var.h \
	       admin.c announce.c args.c assess.c bbs.c board.c brc.c cache.c \
	       cal.c calendar.c card.c chat.c chc.c chc_tab.c chess.c chicken.c \
	       convert.c crypt.c dark.c edit.c fav.c file.c friend.c gamble.c \
	       go.c gomo.c guess.c indict.c io.c kaede.c lovepaper.c mail.c \
	       mbbsd.c menu.c more.c name.c osdep.c othello.c passwd.c pmore.c \
	       read.c record.c register.c screen.c stuff.c syspost.c talk.c \
	       term.c topsong.c user.c var.c vice.c vote.c voteboard.c xyz.c

../include/var.h:	var.c
	perl ../util/parsevar.pl < var.c > ../include/var.h

## reduce .bss align overhead
#.if !defined(DEBUG)
#LDFLAGS+=-Wl,--sort-common
#.endif

#$(PROG): $(OBJS)
#	sh ../util/newvers.sh
#	$(DIETCC) $(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS) $(EXT_LIBS) vers.c

#test: $(PROG)
#	killall -9 testmbbsd || true
#	cp mbbsd testmbbsd
#	./testmbbsd 9000
#	rm -f testmbbsd
#
#install: $(PROG)
#	install -d $(BBSHOME)/bin/
#	install -c -m 755 $(PROG) $(BBSHOME)/bin/
#	mv -f $(BBSHOME)/bin/mbbsd $(BBSHOME)/bin/mbbsd.`date '+%m%d%H%M'`
#	ln -sv $(BBSHOME)/bin/mbbsd.`date '+%m%d%H%M'` $(BBSHOME)/bin/mbbsd
