# $Id$
.include "../pttbbs.mk"

all:	cacheserver utmpserver

cacheserver:	cacheserver.c
	${CC} ${CFLAGS} ${LDFLAGS} -o cacheserver cacheserver.c avltree.c

utmpserver:	utmpserver.c
	${CC} ${CFLAGS} ${LDFLAGS} -o utmpserver utmpserver.c

clean:
	rm -f cacheserver utmpserver *~

test:	utmpserver
	./utmpserver
