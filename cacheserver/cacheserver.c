/* $Id$ */
#include "bbs.h"
#include <avltree.h>

void usage(void)
{
    fprintf(stderr, "usage: cacheserver -p PASSWD\n");
}

AVL_IX_DESC        avl;

typedef struct {
    int             uid;
    unsigned int    count;
    char            key[IDLEN + 1];
} rectype_t;

int loadpasswd(char *pfn)
{
    int     fd, i;
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

    for( i = 0 ; read(fd, &u, sizeof(u)) == sizeof(u) && i < 1024 ; ++i ){
	rec.uid = i;
	rec.count = 0;
	strlcpy(rec.key, u.userid, sizeof(rec.key));
	printf("add %s -> %d\n", rec.key, rec.uid);
	avl_add_key((AVL_IX_REC*)&rec, &avl);
    }

#if 0
    rec.uid = 0;
    rec.count = 0;
    strcpy(rec.key, "KTDOG");
    avl_find_key((AVL_IX_REC*)&rec, &avl);
    printf("%d\n", rec.uid);
#endif
    return 0;
}

int main(int argc, char **argv)
{
    int     ch;
    char    *pfn = NULL;
    while( (ch = getopt(argc, argv, "p:h")) != -1 )
	switch( ch ){
	case 'p':
	    pfn = strdup(optarg);
	    break;

	case 'h':
	default:
	    usage();
	    return 1;
	}

    if( loadpasswd(pfn) < 0 )
	return 1;

    return 0;
}
