/* $Id$ */
#include "bbs.h"
#include <avltree.h>
#include <err.h>

int tobind(int);
AVL_IX_DESC        avl;

typedef struct {
    int             uid;
    unsigned int    count;
    char            key[IDLEN + 1];
} rectype_t;

void usage(void)
{
    fprintf(stderr, "usage: cacheserver -f PASSWD -p port\n");
}

int loadpasswd(char *pfn)
{
    int     fd, i, nUsers = 0;
    userec_t   u;
    rectype_t  rec;

    if( pfn == NULL ){
	fprintf(stderr, "no password file!\n");
	return -1;
    }
    if( (fd = open(pfn, O_RDONLY)) < 0 ){
	perror("open password");
	return -1;
    }

    avl_create_index(&avl, AVL_NO_DUP_KEYS, IDLEN + 1);

    for( i = 0 ; read(fd, &u, sizeof(u)) == sizeof(u) ; ++i )
	if( u.userid[0] ){
	    ++nUsers;
	    rec.uid = i;
	    rec.count = 0;
	    strlcpy(rec.key, u.userid, sizeof(rec.key));
	    avl_add_key((AVL_IX_REC*)&rec, &avl);
	}
    fprintf(stderr, "%s users added\n", nUsers);

#if 0
    rec.uid = 0;
    rec.count = 0;
    strcpy(rec.key, "KTDOG");
    avl_find_key((AVL_IX_REC*)&rec, &avl);
    printf("%d\n", rec.uid);
#endif
    return 0;
}

int service(int sfd)
{
    int     cfd, len, size;
    struct  sockaddr_in     clientaddr;

    char    buf[10240];
    while( 1 ){
	if( (cfd = accept(sfd, (struct sockaddr *)&clientaddr, &len)) < 0 ){
	    if( errno != EINTR )
		sleep(1);
	    continue;
	}

	read(cfd, &size, sizeof(size));
	for( len = 0 ; len < size ;
	     len += read(cfd, &buf[len], sizeof(buf) - len) )
	    ;
	printf("total %d bytes\n", len);
	close(cfd);
    }
}

int main(int argc, char **argv)
{
    int     ch, port = 0, sfd;
    char    *pfn = NULL;
    while( (ch = getopt(argc, argv, "f:p:h")) != -1 )
	switch( ch ){
	case 'f':
	    pfn = strdup(optarg);
	    break;

	case 'p':
	    port = atoi(optarg);
	    break;

	case 'h':
	default:
	    usage();
	    return 1;
	}

    if( (sfd = tobind(port)) < 0 )
	return 1;

    if( loadpasswd(pfn) < 0 )
	return 1;

    service(sfd);
    return 0;
}

/* utils */
int tobind(int port)
{
    int    i, sockfd, val;
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
