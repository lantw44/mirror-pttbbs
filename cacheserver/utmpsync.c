/* $Id$ */
#include "bbs.h"
#include <err.h>

extern SHM_t *SHM;
int towrite(int fd, void *buf, int len);
int toconnect(char *host, int port);

int main(int argc, char **argv)
{
    int     sfd, index, i;
    attach_SHM();
    if( (sfd = toconnect(OUTTACACHEHOST, OUTTACACHEPORT)) < 0 )
	return 1;

    index = -1;
    towrite(sfd, &index, sizeof(index));
    for( i = 0 ; i < MAX_ACTIVE ; ++i )
	if( !towrite(sfd, &SHM->uinfo[i].uid, sizeof(SHM->uinfo[i].uid)) ||
	    !towrite(sfd, SHM->uinfo[i].friend, sizeof(SHM->uinfo[i].friend))||
	    !towrite(sfd, SHM->uinfo[i].reject, sizeof(SHM->uinfo[i].reject))){
	    fprintf(stderr, "sync error %d\n", i);
	}
    return 0;
}

/* utils */
int towrite(int fd, void *buf, int len)
{
    int     l;
    for( l = 0 ; len > 0 ; )
	if( (l = write(fd, buf, len)) < 0 )
	    return -1;
	else{
	    buf += l;
	    len -= l;
	}
    return len;
}

int toconnect(char *host, int port)
{
    int    sock;
    struct sockaddr_in serv_name;
    if( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
        perror("socket");
        return -1;
    }

    serv_name.sin_family = AF_INET;
    serv_name.sin_addr.s_addr = inet_addr(host);
    serv_name.sin_port = htons(port);
    if( connect(sock, (struct sockaddr*)&serv_name, sizeof(serv_name)) < 0 ){
        close(sock);
        return -1;
    }
    return sock;
}
