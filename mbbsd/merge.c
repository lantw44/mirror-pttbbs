/* $Id: merge.c 2060 2004-06-11 17:18:06Z Ptt $ */
#define _XOPEN_SOURCE
#define _ISOC99_SOURCE
/* this is a interface provided when we merge BBS */ 
#include "bbs.h"
#include "fpg.h"

int
m_fpg()
{
    char genbuf[256], buf[256], userid[25], passbuf[24];
    int count=0, i;
    FILE *fp;
    ACCT man;
    time_t d;

    clear();
    move(1,0);

    outs(
 "    小魚的紫色花園,\n"
 "      讓花園的使用者轉移個人資產以及重要信用資料, 享有平等安全的環境.\n"
 "      如果您不需要, 請直接按[Enter]離開.\n"
 "    -----------------------------------------------------------------\n"
 "    特別叮嚀:\n"
 "      為了帳號安全,您只有連續三次密碼錯誤的機會,請小心輸入.\n"
 "      連續三次錯誤您的變身功\能就會被開罰單並直接通知站長.\n"
 "      請不要在變身過程中不正常斷線, 刻意斷線變半獸人站長不救唷.\n"
	);


   if(search_ulistn(usernum,2)) 
        {vmsg("請登出其他視窗, 以免變身失敗"); return 0;}
   do
   {
    if(!getdata(10,0, "      小魚的ID [英文大小寫要完全正確]:", userid, 20,
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
    getdata(11,0, "      小魚的密碼:", passbuf, sizeof(passbuf), 
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
   move(12,0);
   clrtobot();
   reload_money(); 
   
   prints("您的花園幣有 %d 換算成 Ptt 幣為 %d (匯率 155:1), 匯入後共有 %d\n", 
	    man.money, man.money/155, cuser.money + man.money/155);
   demoney(man.money/155);

   cuser.exmailbox +=  (man.mailk + man.keepmail);
   if (cuser.exmailbox > 1000) cuser.exmailbox = 1000;
   prints("您的花園信箱有 %d : %d, 匯入後共有 %d\n", 
	    man.mailk, man.keepmail, cuser.exmailbox );

   if(cuser.firstlogin > man.firstlogin) d = man.firstlogin;
   else  d = cuser.firstlogin;
   prints("花園註冊日期 %s ", Cdatedate(&(man.firstlogin)));
   prints("此帳號註冊日期 %s 將取 ",Cdatedate(&(cuser.firstlogin)));
   prints("%s", Cdatedate(&d) );
   cuser.firstlogin = d;

   if(cuser.numlogins < man.numlogins) i = man.numlogins;
   else i = cuser.numlogins;

   prints("花園進站次數 %d 此帳號 %d 將取 %d \n", man.numlogins,
	   cuser.numlogins, i);
   cuser.numlogins = i;

   if(cuser.numposts < man.numposts ) i = man.numposts;
   else i = cuser.numposts;
   prints("花園文章次數 %d 此帳號 %d 將取 %d\n", man.numposts,cuser.numposts,
	   i); 
   cuser.numposts = i;
   while(search_ulistn(usernum,2)) 
        {vmsg("請將重覆上站其他線關閉! 再繼續");}
   passwd_update(usernum, &cuser);
   sethomeman(genbuf, cuser.userid);
   mkdir(genbuf, 0600);
   sprintf(buf, "tar zxvf home/%c/%s.tgz>/dev/null",
	   userid[0], userid);
   chdir("fpg");
   system(buf);
   chdir(BBSHOME);

   if (getans("是否匯入個人信箱? (Y/n)")!='n')
    {
	sethomedir(buf, cuser.userid);
	sprintf(genbuf, "fpg/home/bbs/home/%c/%s/.DIR",
		userid[0], userid);
	merge_dir(buf, genbuf);
    }
   if(getans("是否匯入個人信箱精華區? (Y/n)")!='n')
   {
        sprintf(buf,
	   "mv fpg/home/bbs/home/%c/%s/man home/%c/%s/man", 
	      userid[0], userid,
	      cuser.userid[0], cuser.userid);
        system(buf);
   }
   if(getans("是否匯入好友名單? (會覆蓋\現有設定, ID可能是不同人)? (y/N)")=='y')
   {
       sethomefile(genbuf, cuser.userid, "overrides");
       sprintf(buf, "fpg/home/bbs/home/%c/%s/overrides",userid[0],userid);
       Copy(buf, genbuf);
       strcat(buf, genbuf);
       friend_load(FRIEND_OVERRIDE);
   }
   vmsg("恭喜您完成帳號變身..");
   return 0;
}

void
m_fpg_brd(char *bname, char *fromdir)
{
  char fbname[25], buf[256];
  fileheader_t fh;

  fromdir[0]=0;
  do{

     if(!getdata(20,0, "小魚的板名 [英文大小寫要完全正確]:", fbname, 20,
	        DOECHO)) return;
  }
  while(invalid_brdname(fbname));

  sprintf(buf, "fpg/boards/%s.inf", fbname);
  if(!dashf(buf))
  {
       vmsg("無此看板");
       return;
  }
  chdir("fpg");
  sprintf(buf, "tar zxf boards/%s.tgz >/dev/null",fbname);
  system(buf);
  sprintf(buf, "tar zxf boards/%s.man.tgz >/dev/null", fbname);
  system(buf);
  chdir(BBSHOME);
  sprintf(buf, "mv fpg/home/bbs/man/boards/%s man/boards/%c/%s", fbname,
	    bname[0], bname);
  system(buf);
  sprintf(fh.title, "%s 精華區", fbname);
  sprintf(fh.filename, fbname);
  sprintf(fh.owner, cuser.userid);
  sprintf(buf, "man/boards/%c/%s/.DIR", bname[0], bname);
  append_record(buf, &fh, sizeof(fh));
  vmsg("匯入成功\ 精華區請link %s",fbname);
}
