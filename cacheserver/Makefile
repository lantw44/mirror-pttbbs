# $Id$
.include "../pttbbs.mk"

all:	cacheserver

cacheserver:	cacheserver.c
	${CC} ${CFLAGS} ${LDFLAGS} -o cacheserver cacheserver.c avltree.c

clean:
	rm -f cacheserver *~

test:	cacheserver
	./cacheserver -f /home/backup/ptt2/PASSWDS.NEW -p 5120
