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
	   "此功\能是專門給各位資深花園使用者,\n"
    "把自己的變為資深Ptt使用者,讓花園的使用者享有平等安全的環境.\n"
    "直接按[Enter]離開.\n\n"
    "特別叮嚀: \n"
    " 為了帳號安全,您只有連續三次密碼錯誤的機會,請小心輸入.\n"
    " 連續三次錯誤您的變身功\能就會被開罰單並直接通知站長.\n"
    " 請不要在變身過程中不正常斷線, 刻意斷線變半獸人站長不負責唷.");

   if(search_ulistn(usernum,2)) 
        {vmsg("請不要multi-login時使用, 以免變身失敗"); return 0;}
   do
   {
    if(!getdata(8,0, "請小心輸入小魚的ID [英文大小寫要完全正確]:", userid, 20,
	       DOECHO)) return 0;
    if(bad_user_id(userid)) continue;
    sprintf(genbuf, "/home/bbs/fpg/home/%c/%s.ACT",userid[0], userid);
    if(!(fp=fopen(genbuf, "r"))) 
	{
	  vmsg("查無此人或已經匯入過..請注意大小寫 ");
          continue;
	}
    count = fread(&man, sizeof(man), 1, fp);
    fclose(fp);
   }while(!count);
   count = 0;
   do{
    getdata(9,0, "請小心輸入您在小魚的密碼:", passbuf, sizeof(passbuf), 
		  NOECHO);
    if(++count>=3)
    {
          cuser.userlevel |= PERM_VIOLATELAW;
          cuser.vl_count++;
	  passwd_update(usernum, &cuser);
          post_violatelaw(cuser.userid, "[PTT警察]", "測試小魚帳號錯誤三次",
		          "違法觀察");
          mail_violatelaw(cuser.userid, "[PTT警察]", "測試小魚帳號錯誤三次",
		          "違法觀察");

          return 0;
    }
   } while(!checkpasswd(man.passwd, passbuf));
   if(!dashf(genbuf))  // avoid multi-login
     {
       vmsg("查無此人或已經匯入過..請注意大小寫 ");
       return 0;
     }
   sprintf(buf,"%s.done",genbuf);
   rename(genbuf,buf);
   move(10,0);
   reload_money(); 
   
   prints("您的花園幣有 %d 換算成 Ptt 幣為 %d (匯率 155:1), 匯入後共有 %d\n", 
	    man.money, man.money/155, cuser.money + man.money/155);
   demoney(man.money/155);

   cuser.exmailbox +=  man.mailk;
   prints("您的花園信相有 %d , 匯入後共有 %d\n", 
	    man.mailk, cuser.exmailbox );

   if(cuser.firstlogin>man.firstlogin) d = man.firstlogin;
   else  d = cuser.firstlogin;
   prints("花園註冊日期 %s 與此帳號 %s 比 將取 %s",
	    Cdate(&man.firstlogin), Cdate(&cuser.firstlogin),
            Cdate(&d) );
   cuser.firstlogin = d;

   if(cuser.numlogins < man.numlogins) i = man.numlogins;
   else i = cuser.numlogins;

   prints("花園進站次數 %d 與此帳號 %d 比 將取 %d", man.numlogins,
	   cuser.numlogins, i);
   cuser.numlogins = i;

   if(cuser.numposts < man.numposts ) i = man.numposts;
   else i = cuser.numposts;
   prints("花園文章次數 %d 與此帳號 %d 比 將取 %d", man.numposts,cuser.numposts,
	   i); 
   cuser.numposts = i;
   while(search_ulistn(usernum,2)) 
        {vmsg("請將重覆上站其他線關閉! 再繼續");}
   passwd_update(usernum, &cuser);
   sethomeman(genbuf, cuser.userid);
   mkdir(genbuf, 0600);
   sprintf(buf, "cd home/bbs/fpg/tmp; tar zxvf ../home/%c/%s.tgz"
	   "cd home/bbs/home/%c; mv %s ../../../..;"
	   "cd ../../../../%s; ",
	   userid[0], userid, userid[0], userid, userid);
   i = 0;
   if (getans("是否匯入個人信箱以及信箱精華區? (Y/n)")!='n')
   {
       sprintf(genbuf,
	   "mv M.* /home/bbs/home/%c/%s;" 
	   "mv man/* /home/bbs/home/%c/%s/man;", cuser.userid[0], cuser.userid, 
	      cuser.userid[0], cuser.userid);
       strcat(buf,genbuf);
       i++;
   }
   if (getans("是否匯入好友名單? (會覆蓋\現有設定, ID可能是不同人)? (y/N)")=='y')
   {
       sprintf(genbuf,"mv overrides /home/bbs/home/%c/%s; ",
	         cuser.userid[0], cuser.userid);
       strcat(buf, genbuf);
       i++;
   }
   if(i) system(buf);
   vmsg("恭喜您完成帳號變身..");
   return 0;
}
