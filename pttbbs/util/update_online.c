#define _UTIL_C_
#include "bbs.h"

/* Update user online info. */

static int verbose = 0;

void fastcheck()
{
    int i, total = SHM->UTMPnumber - 1;
    int sorted[USHM_SIZE], last_uid = -1;
    userec_t urec;
    userinfo_t u;

    assert(sizeof(sorted) == sizeof(**SHM->sorted));
    memcpy(sorted, SHM->sorted[SHM->currsorted][7],
           sizeof(sorted));
    for (i = 0; i < total; i++) {
        if (sorted[i] < 0 || sorted[i] >= USHM_SIZE)
            continue;
        memcpy(&u, SHM->uinfo + sorted[i], sizeof(u));
        if (u.mode == DEBUGSLEEPING ||
            !u.userid[0] ||
            (u.userlevel & PERM_VIOLATELAW) ||
            !(u.userlevel & PERM_LOGINOK) ||
            u.pid <= 0 ||
            kill(u.pid, 0) != 0)
            continue;
        if (last_uid == u.uid)
            continue;

        // Found new online user.
        passwd_query(u.uid, &urec);
        if (strcmp(urec.userid, u.userid) != 0) {
            if (verbose)
                fprintf(stderr, "warning: UTMP (%s) does not match PW(%s).\n",
                        u.userid, urec.userid);
            continue;
        }
        last_uid = u.uid;
        if (verbose)
            fprintf(stderr, "checking: %s (%s)\n", urec.userid, Cdatelite(&urec.lastlogin));

        /* user still online, let's mock it. */
        if (now < urec.lastlogin + DAY_SECONDS)
            continue;

        if (verbose)
            fprintf(stderr, "update: %s (%s, %d) ->", urec.userid,
                    Cdatelite(&urec.lastlogin), urec.numlogindays);
        urec.lastlogin = now;
        urec.numlogindays++;
        if (verbose)
            fprintf(stderr, "(%s, %d).\n", Cdatelite(&urec.lastlogin), urec.numlogindays);
        passwd_update(last_uid, &urec);
    }
}

int main(int argc GCC_UNUSED, char **argv GCC_UNUSED)
{
    const char *prog = argv[0];
    while (argc > 1) {
        if (strcmp(argv[1], "-v") == 0) {
            verbose++;
            argc--, argv++;
        } else {
            fprintf(stderr, "usage: %s [-v]\n", prog);
            return -1;
        }
    }
    now = time(NULL);
    chdir(BBSHOME);

    attach_SHM();
    fastcheck();
    return 0;
}
