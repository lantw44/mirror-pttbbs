/* $Id$ */
#include "bbs.h"
#include <err.h>

struct {
    int     uid;
    short   nFriends, nRejects;
    int     friend[MAX_FRIEND];
    int     reject[MAX_REJECT];
} utmp[MAX_ACTIVE];

inline void countarray(int *s, int max)
{
    int     i;
    for( i = 0 ; i < max && s[i] ; ++i )
	;
    return i;
}

int main(int argc, char **argv)
{
    struct  sockaddr_in     clientaddr;
    int     ch, port = 5120, sfd, cfd, len, index, i, uid;

    while( (ch = getopt(argc, argv, "p:h")) != -1 )
	switch( ch ){
	case 'p':
	    port = atoi(optarg);
	    break;

	case 'h':
	default:
	    fprintf(stderr, "usage: utmpserver [-p port]\n");
	    return 1;
	}

    if( (sfd = tobind(port)) < 0 )
	return 1;

    while( 1 ){
	len = sizeof(clientaddr);
	if( (cfd = accept(sfd, (struct sockaddr *)&clientaddr, &len)) < 0 ){
	    if( errno != EINTR )
		sleep(1);
	    continue;
	}
	toread(cfd, &index, sizeof(index));
	if( index == -1 ){
	    int     nSynced = 0;
	    for( i = 0 ; i < MAX_ACTIVE ; ++i, ++nSynced )
		if( toread(cfd, &utmp[i].uid, sizeof(utmp[i].uid)) > 0      &&
		    toread(cfd, utmp[i].friend, sizeof(utmp[i].friend)) > 0 &&
		    toread(cfd, utmp[i].reject, sizeof(utmp[i].reject)) > 0 )
		    ;
		else
		    for( ; i < MAX_ACTIVE ; ++i )
			utmp[i].uid = 0;
	    close(cfd);
	    fprintf(stderr, "%d users synced\n", nSynced);
	    continue;
	}

	if( toread(cfd, &uid, sizeof(uid)) > 0                              &&
	    toread(cfd, utmp[index].friend, sizeof(utmp[index].friend)) > 0 &&
	    toread(cfd, utmp[index].reject, sizeof(utmp[index].reject)) > 0 ){
	    int     nFriends = 0, frarray[MAX_FRIEND];
	    utmp[index].uid = uid;
	    utmp[index].nFriends = countarray(utmp[index].friend, MAX_FRIEND);
	    utmp[index].nRejects = countarray(utmp[index].reject, MAX_REJECT);

	}
	close(cfd);
    }
    return 0;
}
