#include "bbs.h"

int main(){
    int i;
    int orig_fd, new_fd;
    userec_t u;
    int count = 0;

    orig_fd = open(BBSHOME "/.PASSWDS", O_RDONLY);
    if( orig_fd < 0 ){
	perror("opening " BBSHOME "/.PASSWDS for reading");
	return 1;
    }
    printf("Reading from " BBSHOME "/.PASSWDS\n");

    new_fd = open(BBSHOME "/PASSWDS.NEW", O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if( new_fd < 0 ){
	perror("opening " BBSHOME "/PASSWDS.NEW for writing");
	return 1;
    }
    printf("Writing to " BBSHOME "/PASSWDS.NEW\n");

    while(read(orig_fd, &u, sizeof(userec_t)) == sizeof(userec_t)){
	u.uflag2 &= 0x03ff; // clear 0x400, 0x800, and 0x3000
	if( u.userlevel & OLD_PERM_NOOUTMAIL )
	    u.uflag2 |= REJ_OUTTAMAIL;
	u.userlevel &= ~PERM_ANGEL;
	bzero(u.myangel, IDLEN + 1);
	write(new_fd, &u, sizeof(userec_t));
	++count;
    }

    close(orig_fd);
    close(new_fd);
    printf("Done, totally %d accounts transfered\n", count);
    return 0;
}
