/* $Id$ */
#include "bbs.h"

#define VOTEBOARD "NewBoard"

void
do_voteboardreply(fileheader_t * fhdr)
{
    char            genbuf[256];
    char            reason[36]="";
    char            fpath[80];
    char            oldfpath[80];
    char            opnion[10];
    char           *ptr;
    FILE           *fo,*fi;
    fileheader_t    votefile;
    int             yes=0, no=0, len;
    int             fd;
    time_t          endtime=0;


    clear();
    if (!CheckPostPerm()) {
	move(5, 10);
	prints(gettext[2457]);
	pressanykey();
	return;
    }
    setbpath(fpath, currboard);
    stampfile(fpath, &votefile);

    setbpath(oldfpath, currboard);

    strcat(oldfpath, "/");
    strcat(oldfpath, fhdr->filename);

    fi = fopen(oldfpath, "r");
    assert(fi);

    while (fgets(genbuf, sizeof(genbuf), fi)) {

        if (yes>=0)
           {
            if (!strncmp(genbuf, "----------",10))
               {yes=-1; continue;}
            else 
                yes++;
           }
        if (yes>3) prints(genbuf);

	if (!strncmp(genbuf, gettext[2458], 12)) {
	    ptr = strchr(genbuf, '(');
	    assert(ptr);
	    sscanf(ptr + 1, "%ld", &endtime);
	    if (endtime < now) {
		prints(gettext[2459]);
		pressanykey();
		fclose(fi);
		return;
	    }
	}
        if(yes>=0) continue; 

        strtok(genbuf+4," \n");
	if (!strncmp(genbuf + 4, cuser.userid, IDLEN)) {
	    move(5, 10);
	    prints(gettext[2460]);
	    getdata(17, 0, gettext[2461], opnion, 3, LCECHO);
	    if (opnion[0] != 'y') {
		fclose(fi);
		return;
	    }
	    strlcpy(reason, genbuf + 19, 34);
            break;
	}
    }
    fclose(fi);
    do {
	if (!getdata(19, 0, gettext[2462], opnion, 3, LCECHO)) {
	    return;
	}
    } while (opnion[0] != 'y' && opnion[0] != 'n');
    sprintf(genbuf, gettext[2463],
	    opnion[0] == 'y' ? gettext[2464] : gettext[2465]);
    if (!getdata_buf(20, 0, genbuf, reason, 35, DOECHO)) {
	return;
    }
    if ((fd = open(oldfpath, O_RDONLY)) == -1)
	return;
    if(flock(fd, LOCK_EX)==-1 )
       {close(fd); return;}
    if(!(fi = fopen(oldfpath, "r")))
       {flock(fd, LOCK_UN); close(fd); return;}
     
    if(!(fo = fopen(fpath, "w")))
       {
        flock(fd, LOCK_UN);
        close(fd);
        fclose(fi);
	return;
       }

    while (fgets(genbuf, sizeof(genbuf), fi)) {
        if (!strncmp("----------", genbuf, 10))
	    break;
	fprintf(fo, "%s", genbuf);
    }
    if (!endtime) {
	now += 14 * 24 * 60 * 60;
	fprintf(fo, gettext[2466], now, ctime(&now));
	now -= 14 * 24 * 60 * 60;
    }
    fprintf(fo, "%s", genbuf);
    len = strlen(cuser.userid); 
    for(yes=0; fgets(genbuf, sizeof(genbuf), fi);) {
	if (!strncmp("----------", genbuf, 10))
	    break;
	if (strlen(genbuf)<30 || (genbuf[4+len]==' ' && !strncmp(genbuf + 4, cuser.userid, len)))
            continue;
	fprintf(fo, "%3d.%s", ++yes, genbuf + 4);
      }
    if (opnion[0] == 'y')
	fprintf(fo, gettext[2467], ++yes, cuser.userid, reason, cuser.lasthost);
    fprintf(fo, "%s", genbuf);

    for(no=0; fgets(genbuf, sizeof(genbuf), fi);) {
	if (!strncmp("----------", genbuf, 10))
	    break;
	if (strlen(genbuf)<30 || (genbuf[4+len]==' ' && !strncmp(genbuf + 4, cuser.userid, len)))
            continue;
	fprintf(fo, "%3d.%s", ++no, genbuf + 4);
    }
    if (opnion[0] == 'n')
	fprintf(fo, gettext[2468], ++no, cuser.userid, reason, cuser.lasthost);
    fprintf(fo, gettext[2469]);
    fprintf(fo, gettext[2470], yes, no);
    fprintf(fo, "%s"BBSNAME"("MYHOSTNAME"%s", gettext[2471],
                gettext[2472]);

    flock(fd, LOCK_UN);
    close(fd);
    fclose(fi);
    fclose(fo);
    unlink(oldfpath);
    rename(fpath, oldfpath);
}

int
do_voteboard(int type)
{
    fileheader_t    votefile;
    char            topic[100];
    char            title[80];
    char            genbuf[1024];
    char            fpath[80];
    FILE           *fp;
    int             temp;

    clear();
    if (!CheckPostPerm()) {
	move(5, 10);
	prints(gettext[2473]);
	pressanykey();
	return FULLUPDATE;
    }
    move(0, 0);
    clrtobot();
    prints(gettext[2474]);
    prints(gettext[2475]);
    prints(gettext[2476]);
    move(4, 0);
    clrtobot();
    prints(gettext[2477]);
    if(type==0)
      prints(gettext[2478]);

    do {
	getdata(6, 0, gettext[2479], topic, 3, DOECHO);
	temp = atoi(topic);
    } while (temp < 0 || temp > 9 || (type && temp>2));
    switch (temp) {
    case 0:
         return FULLUPDATE;
    case 1:
	if (!getdata(7, 0, gettext[2480], topic, 30, DOECHO))
	    return FULLUPDATE;
	snprintf(title, sizeof(title), "%s %s", gettext[2481], topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n", gettext[2482], gettext[2483], topic);
	strcat(genbuf, gettext[2484]);
	break;
    case 2:
	if (!getdata(7, 0, gettext[2485], topic, 30, DOECHO))
	    return FULLUPDATE;
	snprintf(title, sizeof(title), "%s %s", gettext[2486], topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n", gettext[2487], gettext[2488], topic);
	strcat(genbuf, gettext[2489]);
	break;
    case 3:
	do {
	    if (!getdata(7, 0, gettext[2490], topic, IDLEN + 1, DOECHO))
		return FULLUPDATE;
	    else if (invalid_brdname(topic))
		outs(gettext[2491]);
	    else if (getbnum(topic) > 0)
		outs(gettext[2492]);
	    else
		break;
	} while (temp > 0);
	snprintf(title, sizeof(title), gettext[2493], topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n%s", gettext[2494], gettext[2495], topic, gettext[2496]);

	if (!getdata(8, 0, gettext[2497], topic, 20, DOECHO))
	    return FULLUPDATE;
	strcat(genbuf, topic);
	strcat(genbuf, gettext[2498]);
	if (!getdata(9, 0, gettext[2499], topic, 20, DOECHO))
	    return FULLUPDATE;
	strcat(genbuf, topic);
	strcat(genbuf, gettext[2500]);
	getdata(10, 0, gettext[2501], topic, IDLEN * 3 + 3, DOECHO);
	strcat(genbuf, topic);
	strcat(genbuf, gettext[2502]);
	break;
    case 4:
        move(1,0); clrtobot();
        generalnamecomplete(gettext[2503],
                            topic, IDLEN+1,
                            SHM->Bnumber,
                            completeboard_compar,
                            completeboard_permission,
                            completeboard_getname);
	snprintf(title, sizeof(title), gettext[2504], topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n", gettext[2505], gettext[2506], topic);
	strcat(genbuf, gettext[2507]);
	break;
    case 5:
        move(1,0); clrtobot();
        generalnamecomplete(gettext[2508],
                            topic, IDLEN+1,
                            SHM->Bnumber,
                            completeboard_compar,
                            completeboard_permission,
                            completeboard_getname);
	snprintf(title, sizeof(title), gettext[2509], topic);
	snprintf(genbuf, sizeof(genbuf), "%s\n\n%s%s\n%s%s", gettext[2510], gettext[2511], topic, gettext[2512], cuser.userid);
	strcat(genbuf, gettext[2513]);
	break;
    case 6:
        move(1,0); clrtobot();
        generalnamecomplete(gettext[2514],
                            topic, IDLEN+1,
                            SHM->Bnumber,
                            completeboard_compar,
                            completeboard_permission,
                            completeboard_getname);
	snprintf(title, sizeof(title), gettext[2515], topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n%s", gettext[2516], gettext[2517],
		 topic, gettext[2518]);
        temp=getbnum(topic);
	do {
	    if (!getdata(7, 0, gettext[2519], topic, IDLEN + 1, DOECHO))
		return FULLUPDATE;
        }while (!userid_is_BM(topic, bcache[temp - 1].BM));
	strcat(genbuf, topic);
	strcat(genbuf, gettext[2520]);
	break;
    case 7:
	if (!getdata(7, 0, gettext[2521], topic, 30, DOECHO))
	    return FULLUPDATE;
	snprintf(title, sizeof(title), gettext[2522], topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n%s%s", gettext[2523], gettext[2524],
		 topic, gettext[2525], cuser.userid);
	strcat(genbuf, gettext[2526]);
	break;
    case 8:
	if (!getdata(7, 0, gettext[2527], topic, 30, DOECHO))
	    return FULLUPDATE;
	snprintf(title, sizeof(title), gettext[2528], topic);
	snprintf(genbuf, sizeof(genbuf), "%s\n\n%s%s\n%s",
		 gettext[2529], gettext[2530], topic, gettext[2531]);
	if (!getdata(8, 0, gettext[2532], topic, IDLEN + 1, DOECHO))
	    return FULLUPDATE;
	strcat(genbuf, topic);
	strcat(genbuf, gettext[2533]);
	break;
    case 9:
	if (!getdata(7, 0, gettext[2534], topic, 30, DOECHO))
	    return FULLUPDATE;
	snprintf(title, sizeof(title), gettext[2535], topic);
	snprintf(genbuf, sizeof(genbuf), "%s\n\n%s%s\n%s%s",
		 gettext[2536], gettext[2537], topic, gettext[2538], cuser.userid);
	strcat(genbuf, gettext[2539]);
	break;
    default:
	return FULLUPDATE;
    }
    outs(gettext[2540]);
    for (temp = 12; temp < 17; temp++) {
	    if (!getdata(temp, 0, gettext[2541], topic, 60, DOECHO))
		break;
	    strcat(genbuf, topic);
	    strcat(genbuf, "\n");
	}
    if (temp == 11)
	    return FULLUPDATE;
    strcat(genbuf, gettext[2542]);
    now += 14 * 24 * 60 * 60;
    snprintf(topic, sizeof(topic), "(%ld)", now);
    strcat(genbuf, topic);
    strcat(genbuf, ctime(&now));
    now -= 14 * 24 * 60 * 60;
    strcat(genbuf, gettext[2543]);
    strcat(genbuf, gettext[2544]);
    outs(gettext[2545]);
    setbpath(fpath, currboard);
    stampfile(fpath, &votefile);

    if (!(fp = fopen(fpath, "w"))) {
	outs(gettext[2546]);
	return FULLUPDATE;
    }
    fprintf(fp, "%s%s %s%s\n%s%s\n%s%s", gettext[2547], cuser.userid,
	    gettext[2548], currboard,
	    gettext[2549], title,
	    gettext[2550], ctime(&now));
    fprintf(fp, "%s\n", genbuf);
    fclose(fp);
    strlcpy(votefile.owner, cuser.userid, sizeof(votefile.owner));
    strlcpy(votefile.title, title, sizeof(votefile.title));
    votefile.filemode |= FILE_VOTE;
    setbdir(genbuf, currboard);
    if (append_record(genbuf, &votefile, sizeof(votefile)) != -1)
	setbtotal(currbid);
    do_voteboardreply(&votefile);
    return FULLUPDATE;
}
