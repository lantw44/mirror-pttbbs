/* $Id$ */
#include "bbs.h"
#include <err.h>

int tobind(int);
int toread(int fd, void *buf, int len);

struct {
    char    userid[IDLEN + 1];
    short   nFriends, nRejects;
    int     friend[MAX_FRIEND];
    int     reject[MAX_REJECT];
} utmp[MAX_ACTIVE];

int main(int argc, char **argv)
{
    struct  sockaddr_in     clientaddr;
    int     ch, port = 5120, sfd, cfd, len, index, i;
    //char    buf[2048];

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
	if( (cfd = accept(sfd, (struct sockaddr *)&clientaddr, &len)) < 0 ){
	    if( errno != EINTR )
		sleep(1);
	    continue;
	}
	toread(cfd, &index, sizeof(index));
	if( index == -1 ){
	    for( i = 0 ; i < MAX_ACTIVE ; ++i )
		if( toread(cfd, utmp[i].userid, sizeof(utmp[i].userid)) &&
		    toread(cfd, utmp[i].friend, sizeof(utmp[i].friend)) &&
		    toread(cfd, utmp[i].reject, sizeof(utmp[i].reject)) )
		    ;
		else
		    for( ; i < MAX_ACTIVE ; ++i )
			utmp[i].userid[0] = 0;
	    close(cfd);
	    continue;
	}
    }
    return 0;
}

/* utils */
int tobind(int port)
{
    int     sockfd, val;
    struct  sockaddr_in     servaddr;

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	err(1, NULL);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
	       (char *)&val, sizeof(val));
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    if( bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 )
	err(1, NULL);
    if( listen(sockfd, 5) < 0 )
	err(1, NULL);

    return sockfd;
}

int toread(int fd, void *buf, int len)
{
    int     l;
    for( l = 0 ; l < len ; l += read(fd, buf + l, len - l) )
	;
    return len;
}
