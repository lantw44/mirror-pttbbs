/* $Id: merge.c 2060 2004-06-11 17:18:06Z Ptt $ */
#define _XOPEN_SOURCE
#define _ISOC99_SOURCE
/* this is a interface provided when we merge BBS */ 
#include "bbs.h"
#include "fpg.h"

int
m_fpg()
{
    char genbuf[256], buf[1024], userid[25], passbuf[24];
    int count=0, i;
    FILE *fp;
    ACCT man;
    time_t d;

    clear();
    move(1,0);

    outs(
	   "���\\��O�M�����U���`���ϥΪ�,\n"
    "��ۤv���ܬ���`Ptt�ϥΪ�,����骺�ϥΪ̨ɦ������w��������.\n"
    "������[Enter]���}.\n\n"
    "�S�O�m�{: \n"
    " ���F�b���w��,�z�u���s��T���K�X���~�����|,�Фp�߿�J.\n"
    " �s��T�����~�z���ܨ��\\��N�|�Q�}�@��ê����q������.\n"
    " �Ф��n�b�ܨ��L�{�������`�_�u, ��N�_�u�ܥb�~�H�������t�d��.");

   if(search_ulistn(usernum,2)) 
        {vmsg("�Ф��nmulti-login�ɨϥ�, �H�K�ܨ�����"); return 0;}
   do
   {
    if(!getdata(8,0, "�Фp�߿�J�p����ID [�^��j�p�g�n�������T]:", userid, 20,
	       DOECHO)) return 0;
    if(bad_user_id(userid)) continue;
    sprintf(genbuf, "/home/bbs/fpg/home/%c/%s.ACT",userid[0], userid);
    if(!(fp=fopen(genbuf, "r"))) 
	{
	  vmsg("�d�L���H�Τw�g�פJ�L..�Ъ`�N�j�p�g ");
          continue;
	}
    count = fread(&man, sizeof(man), 1, fp);
    fclose(fp);
   }while(!count);
   count = 0;
   do{
    getdata(9,0, "�Фp�߿�J�z�b�p�����K�X:", passbuf, sizeof(passbuf), 
		  NOECHO);
    if(++count>=3)
    {
          cuser.userlevel |= PERM_VIOLATELAW;
          cuser.vl_count++;
	  passwd_update(usernum, &cuser);
          post_violatelaw(cuser.userid, "[PTTĵ��]", "���դp���b�����~�T��",
		          "�H�k�[��");
          mail_violatelaw(cuser.userid, "[PTTĵ��]", "���դp���b�����~�T��",
		          "�H�k�[��");

          return 0;
    }
   } while(!checkpasswd(man.passwd, passbuf));
   if(!dashf(genbuf))  // avoid multi-login
     {
       vmsg("�d�L���H�Τw�g�פJ�L..�Ъ`�N�j�p�g ");
       return 0;
     }
   sprintf(buf,"%s.done",genbuf);
   rename(genbuf,buf);
   move(10,0);
   reload_money(); 
   
   prints("�z�������� %d ���⦨ Ptt ���� %d (�ײv 155:1), �פJ��@�� %d\n", 
	    man.money, man.money/155, cuser.money + man.money/155);
   demoney(man.money/155);

   cuser.exmailbox +=  man.mailk;
   prints("�z�����H�ۦ� %d , �פJ��@�� %d\n", 
	    man.mailk, cuser.exmailbox );

   if(cuser.firstlogin>man.firstlogin) d = man.firstlogin;
   else  d = cuser.firstlogin;
   prints("�����U��� %s �P���b�� %s �� �N�� %s",
	    Cdate(&man.firstlogin), Cdate(&cuser.firstlogin),
            Cdate(&d) );
   cuser.firstlogin = d;

   if(cuser.numlogins < man.numlogins) i = man.numlogins;
   else i = cuser.numlogins;

   prints("���i������ %d �P���b�� %d �� �N�� %d", man.numlogins,
	   cuser.numlogins, i);
   cuser.numlogins = i;

   if(cuser.numposts < man.numposts ) i = man.numposts;
   else i = cuser.numposts;
   prints("���峹���� %d �P���b�� %d �� �N�� %d", man.numposts,cuser.numposts,
	   i); 
   cuser.numposts = i;
   while(search_ulistn(usernum,2)) 
        {vmsg("�бN���ФW����L�u����! �A�~��");}
   passwd_update(usernum, &cuser);
   sethomeman(genbuf, cuser.userid);
   mkdir(genbuf, 0600);
   sprintf(buf, "cd home/bbs/fpg/tmp; tar zxvf ../home/%c/%s.tgz"
	   "cd home/bbs/home/%c; mv %s ../../../..;"
	   "cd ../../../../%s; ",
	   userid[0], userid, userid[0], userid, userid);
   i = 0;
   if (getans("�O�_�פJ�ӤH�H�c�H�ΫH�c��ذ�? (Y/n)")!='n')
   {
       sprintf(genbuf,
	   "mv M.* /home/bbs/home/%c/%s;" 
	   "mv man/* /home/bbs/home/%c/%s/man;", cuser.userid[0], cuser.userid, 
	      cuser.userid[0], cuser.userid);
       strcat(buf,genbuf);
       i++;
   }
   if (getans("�O�_�פJ�n�ͦW��? (�|�л\\�{���]�w, ID�i��O���P�H)? (y/N)")=='y')
   {
       sprintf(genbuf,"mv overrides /home/bbs/home/%c/%s; ",
	         cuser.userid[0], cuser.userid);
       strcat(buf, genbuf);
       i++;
   }
   if(i) system(buf);
   vmsg("���߱z�����b���ܨ�..");
   return 0;
}
