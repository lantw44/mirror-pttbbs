/* $Id$ */
/* �}���� utility */
#define _UTIL_C_
#include "bbs.h"

#define MAX_ITEM	8	//�̤j �䶵(item) �Ӽ�
#define MAX_ITEM_LEN	30	//�̤j �C�@�䶵�W�r����
#define MAX_DISP_LEN    8       //�C����̤ܳj����
#define PRICE           100     //Default price
#define MAX_DES 7		/* �̤j�O�d���� */
#define VALID_UNUM(u) ((u) > 0 && (u) < MAX_USERS)

#if (MAX_ITEM_LEN < IDLEN)
# error MAX_ITEM_LEN must be greater than IDLEN.
#endif

const char unique_betnames[MAX_ITEM][MAX_ITEM_LEN] = {
    "Ptt", "Jaky",  "Action",  "Heat",
    "DUNK", "Jungo", "waiting", "wofe",
};

int
sendalert_uid(int uid, int alert){
    userinfo_t     *uentp = NULL;

    if ((uentp = (userinfo_t *) search_ulistn(uid, 1)))
        uentp->alerts |= alert;
    return 0;
}

int load_ticket_items(const char *filename, int n_items,
                      int *pPrice, char items[MAX_ITEM][MAX_ITEM_LEN]) {
    FILE *fp = fopen(filename, "rt");
    char genbuf[MAX_ITEM_LEN] = "";
    int i;

    if (!fp)
        return 0;

    if (fgets(genbuf, sizeof(genbuf), fp) && *genbuf) {
        if (pPrice)
            *pPrice = atoi(genbuf);
    }

    for (i = 0; fgets(items[i], MAX_ITEM_LEN, fp) && i < n_items; i++) {
        chomp(items[i]);
    }
    fclose(fp);
    return 1;
}

int save_ticket_items(const char *filename, int n_items,
                      int price, char items[MAX_ITEM][MAX_ITEM_LEN]) {
    FILE *fp = fopen(filename, "wt");
    int i;
    if (!fp)
        return 0;
    fprintf(fp, "%d\n", price);
    for (i = 0; i < n_items; i++) {
        fprintf(fp, "%s\n", items[i]);
    }
    fclose(fp);
    return 1;
}

void create_new_items(char items[MAX_ITEM][MAX_ITEM_LEN],
                      char ref_items[MAX_ITEM][MAX_ITEM_LEN],
                      int n_items) {
    int i;
    int unum = 0;
    time4_t now = (time4_t)time(NULL);

    // initialize names by duplicating
    for (i = 0; i < n_items; i++) {
        strlcpy(items[i], ref_items[i], MAX_ITEM_LEN);
    }

    // find last user index 
    for (i = n_items - 1; i >= 0; i--) {
        unum = searchuser(items[i], NULL);
        if (VALID_UNUM(unum)) {
            unum++;
            break;
        }
    }
    log_filef("log/openticket_items.log", LOG_CREAT,
             "%s last user index: %d (%s)\n",
             Cdatelite(&now), unum,
             VALID_UNUM(unum) ? SHM->userid[unum - 2] : "-unkonwn-");

    // pickup new names
    for (i = 0; i < n_items; i++) {
        // find next valid unum
        for (; VALID_UNUM(unum) && !*SHM->userid[unum - 1]; unum++) {
            // find next one
        }
        if (!VALID_UNUM(unum)) {
            strlcpy(items[i], unique_betnames[i], MAX_ITEM_LEN);
            // rewind
            unum = 1;
            log_filef("log/openticket_items.log", LOG_CREAT,
                      "%s rewind user index.\n",
                      Cdatelite(&now));
            continue;
        }

        strlcpy(items[i], SHM->userid[unum - 1], MAX_ITEM_LEN);
        unum ++;
    }
}
    

int main()
{
    int  money, bet, n, total = 0;
    int  ticket[MAX_ITEM] = {0};
    FILE *fp;
    time4_t now = (time4_t)time(NULL);
    char des[MAX_DES][200] = {"", "", "", ""};

    char newbetname[MAX_ITEM][MAX_ITEM_LEN] = {0};
    char betname[MAX_ITEM][MAX_ITEM_LEN] = {
        "Ptt", "Jaky",  "Action",  "Heat",
        "DUNK", "Jungo", "waiting", "wofe",
    };

    nice(10);

    chdir(BBSHOME);  // for sethomedir
    attach_SHM();
    srand(now);

    load_ticket_items("etc/" FN_TICKET_ITEMS, MAX_ITEM, NULL, betname);
    create_new_items(newbetname, betname, MAX_ITEM);

    rename("etc/" FN_TICKET_RECORD, "etc/" FN_TICKET_RECORD ".tmp");
    rename("etc/" FN_TICKET_USER,   "etc/" FN_TICKET_USER   ".tmp");

    save_ticket_items("etc/" FN_TICKET_ITEMS, MAX_ITEM, PRICE, newbetname);

    if (!(fp = fopen("etc/"FN_TICKET_RECORD ".tmp", "r")))
	return 0;

    fscanf(fp, "%9d %9d %9d %9d %9d %9d %9d %9d\n",
	   &ticket[0], &ticket[1], &ticket[2], &ticket[3],
	   &ticket[4], &ticket[5], &ticket[6], &ticket[7]);
    for (n = 0; n < 8; n++)
	total += ticket[n];
    fclose(fp);

    if (!total)
	return 0;

    if((fp = fopen("etc/" FN_TICKET , "r")))
    {
	for (n = 0; n < MAX_DES && fgets(des[n], 200, fp); n++) ;
	fclose(fp);
    }

    if ((fp = fopen("/dev/random", "rb")) != NULL) {
        bet = fgetc(fp) % MAX_ITEM;
        fclose(fp);
    } else {
        /* �{�b�}�����X�èS�Ψ� random function.
         * �p���� UTMPnumber �i�����w��, �B UTMPnumber �w�]�@��~��s�@��
         * �}���@�q�ɶ����}�� pid ���ӵL�k�w��.
         * �Y�O�p�����}���e�}��, �h���Q�q�����i�� */
        bet = (SHM->UTMPnumber+getpid()) % MAX_ITEM;
    }

    money = ticket[bet] ? total * 95 / ticket[bet] : 9999999;
    if((fp = fopen("etc/" FN_TICKET, "w")))
    {
	if (des[MAX_DES - 1][0])
	    n = 1;
	else
	    n = 0;

	for (; n < MAX_DES && des[n][0] != 0; n++)
	{
	    fputs(des[n], fp);
	}

	printf("\n\n�}���ɶ��G %s\n\n"
	       "�}�����G�G %s\n\n"
	       "�U�`�`���B�G %d00 " MONEYNAME "\n"
	       "������ҡG %d�i/%d�i  (%f)\n"
	       "�C�i�����m���i�o %d �T" MONEYNAME "\n\n",
	       Cdatelite(&now), betname[bet], total, ticket[bet], total,
	       (float) ticket[bet] / total, money);

	fprintf(fp, "%s �}�X:%s �`���B:%d00 ����/�i:%d ���v:%1.2f\n",
		Cdatelite(&now), betname[bet], total, money,
		(float) ticket[bet] / total);
	fclose(fp);

    }

    if (ticket[bet] && (fp = fopen("etc/" FN_TICKET_USER ".tmp", "r")))
    {
	int mybet, num, uid;
	char userid[20], genbuf[200];
	fileheader_t mymail;

	while (fscanf(fp, "%s %d %d\n", userid, &mybet, &num) != EOF)
	{
	    if (mybet == bet)
	    {
                int oldm, newm;
		printf("���� %-*s �R�F%9d �i %s, ��o %d �T" MONEYNAME "\n",
                       IDLEN, userid, num, betname[mybet], money * num);
                if((uid=searchuser(userid, userid))==0 ||
                    !is_validuserid(userid)) 
                    continue;

                oldm = moneyof(uid);
		deumoney(uid, money * num);
                newm = moneyof(uid);
                {
                    char reason[256];
                    sprintf(reason, "��L���� (%s x %d)", betname[mybet], num);
                    sethomefile(genbuf, userid, FN_RECENTPAY);
                    log_payment(genbuf, -money * num, oldm, newm, reason, now);
                }
                sethomepath(genbuf, userid);
		stampfile(genbuf, &mymail);
		strcpy(mymail.owner, BBSNAME);
		sprintf(mymail.title, "[%s] �����o! $ %d", Cdatelite(&now), money * num);
		unlink(genbuf);
		Link("etc/ticket", genbuf);
		sprintf(genbuf, "home/%c/%s/.DIR", userid[0], userid);
		append_record(genbuf, &mymail, sizeof(mymail));
                sendalert_uid(uid, ALERT_NEW_MAIL);
	    } else {
		printf("     %-15s�R�F%9d �i %s\n" ,userid, num, betname[mybet]);
            }
	}
    }
    unlink("etc/" FN_TICKET_RECORD ".tmp");
    unlink("etc/" FN_TICKET_USER ".tmp");
    return 0;
}
