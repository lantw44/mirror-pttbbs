/* 
 * �Y�z�ϥΪ��O Current Ptt�A�h���������󪺭ץ��C
 * �Y�z�ϥΪ��O��L������ bbs�A�бN Makefile ���� -DPTTBBS ����������A
 * �æۦ��U���� #elseif ����[�J�A���{���X�C
 */

#define RATE 0.95

int bank_moneyof(int uid){
#ifdef PTTBBS
    printf("[server] : uid = %d\n", uid);
    return moneyof(uid);
#else
    //
#endif
}

int bank_searchuser(char *userid){

#ifdef PTTBBS
    return searchuser(userid);
#else
    //
#endif
}

int bank_deumoney(char *user, int money){

#ifdef PTTBBS

    money *= RATE;
    printf("give user: %s money: %d add:%d\n", user, moneyof(bank_searchuser(user)), money);
    deumoney(bank_searchuser(user), money);
#else
    //
#endif
}
