# $Id$
.include "../pttbbs.mk"

all:	cacheserver

cacheserver:	cacheserver.c
	${CC} ${CFLAGS} ${LDFLAGS} -o cacheserver cacheserver.c avltree.c

test:	cacheserver
	./cacheserver -p /home/backup/ptt/PASSWDS.NEW
