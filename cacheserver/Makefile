# $Id$
.include "../pttbbs.mk"

AUTO_PROGRAMS=	utmpserver
PROGRAMS=	${AUTO_PROGRAMS}	utmpsync

all:	${PROGRAMS}

#cacheserver:	cacheserver.c
#	${CC} ${CFLAGS} ${LDFLAGS} -o cacheserver cacheserver.c avltree.c

utmpsync: utmpsync.c
	${CC} ${CFLAGS} ${LDFLAGS} -DPTTBBS_UTIL -o utmpsync utmpsync.c \
	../util/util_*.o

.for fn in ${AUTO_PROGRAMS}
${fn}: ${fn}.c
	${CC} ${CFLAGS} ${LDFLAGS} -o ${fn} ${fn}.c
.endfor

clean:
	rm -f *~ ${PROGRAMS}

test:	utmpserver
	./utmpserver
