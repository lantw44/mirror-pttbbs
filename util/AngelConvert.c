#include "bbs.h"

int main(){
    int i;
    int orig_fd, new_fd;
    userec_t u;
    int count = 0;

    orig_fd = open(BBSHOME "/.PASSWD", O_RDONLY);
    if( orig_fd < 0 ){
	perror("opening " BBSHOME "/.PASSWD for reading");
	return 1;
    }
    printf("Reading from " BBSHOME "/.PASSWD\n");

    new_fd = open(BBSHOME "/PASSWD.NEW", O_WRONLY);
    if( new_fd < 0 ){
	perror("opening " BBSHOME "/PASSWD.NEW for writing");
	return 1;
    }
    printf("Writing to " BBSHOME "/PASSWD.NEW\n");

    while(read(orig_fd, &u, sizeof(userec_t)) == sizeof(userec_t)){
	u.uflag2 &= 0x03ff; // clear 0x400 and 0x1000
	if( u.userlevel & OLD_PERM_NOOUTMAIL )
	    u.uflag2 |= REJ_OUTTAMAIL;
	u.userlevel &= ~PERM_ANGEL;
	write(new_fd, &u, sizeof(userec_t));
	++count;
    }

    close(orig_fd);
    close(new_fd);
    printf("Done, totally %d accounts transfered\n", count);
    return 0;
}
