/* $Id$ */
#include "bbs.h"

#define NUM_KINDS   15		/* 有多少種動物 */

static          char *chicken_type[NUM_KINDS];
static          char *chicken_food[NUM_KINDS];
static          const int egg_price[NUM_KINDS] = {
    5, 25, 30, 40,
    80, 50, 15, 35,
    17, 100, 85, 200,
    200, 100, 77};
static          const int food_price[NUM_KINDS] = {
    4, 6, 8, 10,
    12, 12, 5, 6,
    5, 20, 15, 23,
    23, 10, 19};
static          char *attack_type[NUM_KINDS];

static          char *damage_degree[16];

enum {
    OO, FOOD, WEIGHT, CLEAN, RUN, ATTACK, BOOK, HAPPY, SATIS,
    TEMPERAMENT, TIREDSTRONG, SICK, HP_MAX, MM_MAX
};

static int      age;

static          const short time_change[NUM_KINDS][14] =
/* 補品 食物 體重 乾淨 敏捷 攻擊力 知識 快樂 滿意 氣質 疲勞 病氣 滿血 滿法 */
{
    /* 雞 */
    {1, 1, 30, 3, 8, 3, 3, 40, 9, 1, 7, 3, 30, 1},
    /* 美少女 */
    {1, 1, 110, 1, 4, 7, 41, 20, 9, 25, 25, 7, 110, 15},
    /* 勇士 */
    {1, 1, 200, 5, 4, 10, 33, 20, 15, 10, 27, 1, 200, 9},
    /* 蜘蛛 */
    {1, 1, 10, 5, 8, 1, 1, 5, 3, 1, 4, 1, 10, 30},
    /* 恐龍 */
    {1, 1, 1000, 9, 1, 13, 4, 12, 3, 1, 200, 1, 1000, 3},
    /* 老鷹 */
    {1, 1, 90, 7, 10, 7, 4, 12, 3, 30, 20, 5, 90, 20},
    /* 貓 */
    {1, 1, 30, 5, 5, 6, 4, 8, 3, 15, 7, 4, 30, 21},
    /* 蠟筆小新 */
    {1, 1, 100, 9, 7, 7, 20, 50, 10, 8, 24, 4, 100, 9},
    /* 狗 */
    {1, 1, 45, 8, 7, 9, 3, 40, 20, 3, 9, 5, 45, 1},
    /* 惡魔 */
    {1, 1, 45, 10, 11, 11, 5, 21, 11, 1, 9, 5, 45, 25},
    /* 忍者 */
    {1, 1, 45, 2, 12, 10, 25, 1, 1, 10, 9, 5, 45, 26},
    /* 阿扁 */
    {1, 1, 150, 4, 8, 13, 95, 25, 7, 10, 25, 5, 175, 85},
    /* 馬英九 */
    {1, 1, 147, 2, 10, 10, 85, 20, 4, 25, 25, 5, 145, 95},
    /* 就可人 */
    {1, 1, 200, 3, 15, 15, 50, 50, 10, 5, 10, 2, 300, 0},
    /* 羅利 */
    {1, 1, 80, 2, 9, 10, 2, 5, 7, 8, 12, 1, 135, 5},
};

int
reload_chicken()
{

    chicken_t *mychicken = &cuser.mychicken;

    passwd_query(usernum, &xuser);
    memcpy(mychicken, &xuser.mychicken, sizeof(chicken_t));
    if (!mychicken->name[0])
	return 0;
    else
	return 1;
}

#define CHICKENLOG  "etc/chicken"

static int
new_chicken()
{
    chicken_t *mychicken = &cuser.mychicken;
    char            buf[150];
    int             price;

    clear();
    move(2, 0);
    outs(SHM->i18nstr[cuser.language][867]);
    getdata_str(7, 0, SHM->i18nstr[cuser.language][868], buf, 3, LCECHO, "0");

    buf[0] -= 'a';
    if (buf[0] < 0 || buf[0] > NUM_KINDS - 1)
	return 0;

    mychicken->type = buf[0];

    reload_money();
    price = egg_price[(int)mychicken->type];
    if (cuser.money < price) {
	prints(SHM->i18nstr[cuser.language][869], price);
	refresh();
	return 0;
    }
    vice(price, SHM->i18nstr[cuser.language][870]);
    while (strlen(mychicken->name) < 3)
	getdata(8, 0, SHM->i18nstr[cuser.language][871], mychicken->name,
		sizeof(mychicken->name), DOECHO);

    snprintf(buf, sizeof(buf),
	     SHM->i18nstr[cuser.language][872], cuser.userid,
	     mychicken->name, chicken_type[(int)mychicken->type], ctime(&now));
    log_file(CHICKENLOG, buf, 1);
    mychicken->lastvisit = mychicken->birthday = mychicken->cbirth = now;
    mychicken->food = 0;
    mychicken->weight = time_change[(int)mychicken->type][WEIGHT] / 3;
    mychicken->clean = 0;
    mychicken->run = time_change[(int)mychicken->type][RUN];
    mychicken->attack = time_change[(int)mychicken->type][ATTACK];
    mychicken->book = time_change[(int)mychicken->type][BOOK];
    mychicken->happy = time_change[(int)mychicken->type][HAPPY];
    mychicken->satis = time_change[(int)mychicken->type][SATIS];
    mychicken->temperament = time_change[(int)mychicken->type][TEMPERAMENT];
    mychicken->tiredstrong = 0;
    mychicken->sick = 0;
    mychicken->hp = time_change[(int)mychicken->type][WEIGHT];
    mychicken->hp_max = time_change[(int)mychicken->type][WEIGHT];
    mychicken->mm = 0;
    mychicken->mm_max = 0;
    return 1;
}

int
show_file(char *filename, int y, int lines, int mode)
{
    FILE           *fp;
    char            buf[256];

    if (y >= 0)
	move(y, 0);
    clrtoline(lines + y);
    if ((fp = fopen(filename, "r"))) {
	while (fgets(buf, sizeof(buf), fp) && lines--)
	    outs(Ptt_prints(buf, mode));
	fclose(fp);
    } else
	return 0;
    return 1;
}

static void
show_chicken_stat(chicken_t * thechicken)
{
    struct tm      *ptime;

    ptime = localtime(&thechicken->birthday);
    prints(SHM->i18nstr[cuser.language][873],
	   thechicken->name, chicken_type[(int)thechicken->type],
	   15 - strlen(thechicken->name), "",
	   ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday,
	   SHM->i18nstr[cuser.language][age > 16 ? 789 : age + 789],
	   age, thechicken->hp, thechicken->hp_max,
	   thechicken->mm, thechicken->mm_max,
	   thechicken->attack, thechicken->run, thechicken->book,
	   thechicken->happy, thechicken->satis, thechicken->tiredstrong,
	   thechicken->temperament,
	   ((float)(thechicken->hp_max + (thechicken->weight / 50))) / 100,
	   thechicken->sick, thechicken->clean, thechicken->food,
	   thechicken->oo, thechicken->medicine);
}

#define CHICKEN_PIC "etc/chickens"

void
show_chicken_data(chicken_t * thechicken, chicken_t * pkchicken)
{
    char            buf[1024];
    age = ((now - thechicken->cbirth) / (60 * 60 * 24));
    if (age < 0) {
	thechicken->birthday = thechicken->cbirth = now - 10 * (60 * 60 * 24);
	age = 10;
    }
    /* Ptt:debug */
    thechicken->type %= NUM_KINDS;
    clear();
    showtitle(pkchicken ? SHM->i18nstr[cuser.language][874] : SHM->i18nstr[cuser.language][875], BBSName);
    move(1, 0);

    show_chicken_stat(thechicken);

    snprintf(buf, sizeof(buf), CHICKEN_PIC "/%c%d", thechicken->type + 'a',
	     age > 16 ? 16 : age);
    show_file(buf, 5, 14, NO_RELOAD);

    move(18, 0);

    if (thechicken->sick)
	outs(SHM->i18nstr[cuser.language][876]);
    if (thechicken->sick > thechicken->hp / 5)
	outs(SHM->i18nstr[cuser.language][877]);

    if (thechicken->clean > 150)
	outs(SHM->i18nstr[cuser.language][878]);
    else if (thechicken->clean > 80)
	outs(SHM->i18nstr[cuser.language][879]);
    else if (thechicken->clean < 20)
	outs(SHM->i18nstr[cuser.language][880]);

    if (thechicken->weight > thechicken->hp_max * 4)
	outs(SHM->i18nstr[cuser.language][881]);
    else if (thechicken->weight > thechicken->hp_max * 3)
	outs(SHM->i18nstr[cuser.language][882]);
    else if (thechicken->weight < (thechicken->hp_max / 4))
	outs(SHM->i18nstr[cuser.language][883]);
    else if (thechicken->weight < (thechicken->hp_max / 2))
	outs(SHM->i18nstr[cuser.language][884]);

    if (thechicken->tiredstrong > thechicken->hp * 1.7)
	outs(SHM->i18nstr[cuser.language][885]);
    else if (thechicken->tiredstrong > thechicken->hp)
	outs(SHM->i18nstr[cuser.language][886]);
    else if (thechicken->tiredstrong < thechicken->hp / 4)
	outs(SHM->i18nstr[cuser.language][887]);

    if (thechicken->hp < thechicken->hp_max / 4)
	outs(SHM->i18nstr[cuser.language][888]);
    if (thechicken->happy > 500)
	outs(SHM->i18nstr[cuser.language][889]);
    else if (thechicken->happy < 100)
	outs(SHM->i18nstr[cuser.language][890]);
    if (thechicken->satis > 500)
	outs(SHM->i18nstr[cuser.language][891]);
    else if (thechicken->satis < 50)
	outs(SHM->i18nstr[cuser.language][892]);

    if (pkchicken) {
	outs("\n");
	show_chicken_stat(pkchicken);
	outs(SHM->i18nstr[cuser.language][893]);
    }
}

static void
ch_eat()
{
    chicken_t *mychicken = &cuser.mychicken;
    if (mychicken->food) {
	mychicken->weight += time_change[(int)mychicken->type][WEIGHT] +
	mychicken->hp_max / 5;
	mychicken->tiredstrong +=
	    time_change[(int)mychicken->type][TIREDSTRONG] / 2;
	mychicken->hp_max++;
	mychicken->happy += 5;
	mychicken->satis += 7;
	mychicken->food--;
	move(10, 10);

	show_file(CHICKEN_PIC "/eat", 5, 14, NO_RELOAD);
	pressanykey();
    }
}

static void
ch_clean()
{
    chicken_t *mychicken = &cuser.mychicken;
    mychicken->clean = 0;
    mychicken->tiredstrong +=
	time_change[(int)mychicken->type][TIREDSTRONG] / 3;
    show_file(CHICKEN_PIC "/clean", 5, 14, NO_RELOAD);
    pressanykey();
}

static void
ch_guess()
{
    char           *guess[3] = {SHM->i18nstr[cuser.language][894], SHM->i18nstr[cuser.language][895], SHM->i18nstr[cuser.language][896]}, me, ch, win;

    chicken_t *mychicken = &cuser.mychicken;
    mychicken->happy += time_change[(int)mychicken->type][HAPPY] * 1.5;
    mychicken->satis += time_change[(int)mychicken->type][SATIS];
    mychicken->tiredstrong += time_change[(int)mychicken->type][TIREDSTRONG];
    mychicken->attack += time_change[(int)mychicken->type][ATTACK] / 4;
    move(20, 0);
    clrtobot();
    outs(SHM->i18nstr[cuser.language][897]);
    me = igetch();
    me -= '1';
    if (me > 2 || me < 0)
	me = 0;
    win = (int)(3.0 * rand() / (RAND_MAX + 1.0)) - 1;
    ch = (me + win + 3) % 3;
    prints("%s:%s !      %s:%s !.....%s",
	   cuser.userid, guess[(int)me], mychicken->name, guess[(int)ch],
	   win == 0 ? SHM->i18nstr[cuser.language][898] : win < 0 ? SHM->i18nstr[cuser.language][899] : SHM->i18nstr[cuser.language][900]);
    pressanykey();
}

static void
ch_book()
{
    chicken_t *mychicken = &cuser.mychicken;
    mychicken->book += time_change[(int)mychicken->type][BOOK];
    mychicken->tiredstrong += time_change[(int)mychicken->type][TIREDSTRONG];
    show_file(CHICKEN_PIC "/read", 5, 14, NO_RELOAD);
    pressanykey();
}

static void
ch_kiss()
{
    chicken_t *mychicken = &cuser.mychicken;
    mychicken->happy += time_change[(int)mychicken->type][HAPPY];
    mychicken->satis += time_change[(int)mychicken->type][SATIS];
    mychicken->tiredstrong +=
	time_change[(int)mychicken->type][TIREDSTRONG] / 2;
    show_file(CHICKEN_PIC "/kiss", 5, 14, NO_RELOAD);
    pressanykey();
}

static void
ch_hit()
{
    chicken_t *mychicken = &cuser.mychicken;
    mychicken->attack += time_change[(int)mychicken->type][ATTACK];
    mychicken->run += time_change[(int)mychicken->type][RUN];
    mychicken->mm_max += time_change[(int)mychicken->type][MM_MAX] / 15;
    mychicken->weight -= mychicken->hp_max / 15;
    mychicken->hp -= (int)((float)time_change[(int)mychicken->type][HP_MAX] *
			   rand() / (RAND_MAX + 1.0)) / 2 + 1;

    if (mychicken->book > 2)
	mychicken->book -= 2;
    if (mychicken->happy > 2)
	mychicken->happy -= 2;
    if (mychicken->satis > 2)
	mychicken->satis -= 2;
    mychicken->tiredstrong += time_change[(int)mychicken->type][TIREDSTRONG];
    show_file(CHICKEN_PIC "/hit", 5, 14, NO_RELOAD);
    pressanykey();
}

void
ch_buyitem(int money, char *picture, int *item, int haveticket)
{
    int             num = 0;
    char            buf[5];

    getdata_str(b_lines - 1, 0, SHM->i18nstr[cuser.language][901],
		buf, sizeof(buf), DOECHO, "1");
    num = atoi(buf);
    if (num < 1)
	return;
    reload_money();
    if (cuser.money > money * num) {
	*item += num;
	if( haveticket )
	    vice(money * num, SHM->i18nstr[cuser.language][902]);
	else
	    demoney(-money * num);
	show_file(picture, 5, 14, NO_RELOAD);
    } else {
	move(b_lines - 1, 0);
	clrtoeol();
	outs(SHM->i18nstr[cuser.language][903]);
    }
    pressanykey();
}

static void
ch_eatoo()
{
    chicken_t *mychicken = &cuser.mychicken;
    if (mychicken->oo > 0) {
	mychicken->oo--;
	mychicken->tiredstrong = 0;
	if (mychicken->happy > 5)
	    mychicken->happy -= 5;
	show_file(CHICKEN_PIC "/oo", 5, 14, NO_RELOAD);
	pressanykey();
    }
}

static void
ch_eatmedicine()
{
    chicken_t *mychicken = &cuser.mychicken;
    if (mychicken->medicine > 0) {
	mychicken->medicine--;
	mychicken->sick = 0;
	if (mychicken->hp_max > 10)
	    mychicken->hp_max -= 3;
	mychicken->hp = mychicken->hp_max;
	if (mychicken->happy > 10)
	    mychicken->happy -= 10;
	show_file(CHICKEN_PIC "/medicine", 5, 14, NO_RELOAD);
	pressanykey();
    }
}

static void
ch_kill()
{
    chicken_t *mychicken = &cuser.mychicken;
    char            buf[150], ans[4];

    snprintf(buf, sizeof(buf), SHM->i18nstr[cuser.language][904],
	    chicken_type[(int)mychicken->type]);
    getdata_str(23, 0, buf, ans, sizeof(ans), DOECHO, "N");
    if (ans[0] == 'y') {

	vice(100, SHM->i18nstr[cuser.language][905]);
	more(CHICKEN_PIC "/deadth", YEA);
	snprintf(buf, sizeof(buf),
		 SHM->i18nstr[cuser.language][906], cuser.userid, mychicken->name,
		 chicken_type[(int)mychicken->type], ctime(&now));
	log_file(CHICKENLOG, buf, 1);
	mychicken->name[0] = 0;
    }
}

static int
ch_sell()
{
    chicken_t *mychicken = &cuser.mychicken;
    /*
     * int money = (mychicken->weight -
     * time_change[(int)mychicken->type][WEIGHT])
     * (food_price[(int)mychicken->type])/4 + ( + ((mychicken->clean /
     * time_change[(int)mychicken->type][CLEAN]) + (mychicken->run /
     * time_change[(int)mychicken->type][RUN]) + (mychicken->attack /
     * time_change[(int)mychicken->type][ATTACK]) + (mychicken->book /
     * time_change[(int)mychicken->type][BOOK]) + (mychicken->happy /
     * time_change[(int)mychicken->type][HAPPY]) + (mychicken->satis /
     * time_change[(int)mychicken->type][SATIS]) + (mychicken->temperament /
     * time_change[(int)mychicken->type][TEMPERAMENT]) -
     * (mychicken->tiredstrong /
     * time_change[(int)mychicken->type][TIREDSTRONG]) - (mychicken->sick /
     * time_change[(int)mychicken->type][SICK]) + (mychicken->hp /
     * time_change[(int)mychicken->type][HP_MAX]) + (mychicken->mm /
     * time_change[(int)mychicken->type][MM_MAX]) + 7 - abs(age - 7)) * 3 ;
     */
    int             money = (age * food_price[(int)mychicken->type] * 3
			     + (mychicken->hp_max * 10 + mychicken->weight) /
			time_change[(int)mychicken->type][HP_MAX]) * 3 / 2 -
    mychicken->sick;
    char            buf[150], ans[4];

    if (money < 0)
	money = 0;
    else if (money > MAX_CHICKEN_MONEY)
	money = MAX_CHICKEN_MONEY;
    //防止怪雞
    if (mychicken->type == 1 || mychicken->type == 7) {
	outs(SHM->i18nstr[cuser.language][907]);
	pressanykey();
	return 0;
    }
    if (age < 5) {
	outs(SHM->i18nstr[cuser.language][908]);
	pressanykey();
	return 0;
    }
    if (age > 30) {
	outs(SHM->i18nstr[cuser.language][909]);
	pressanykey();
	return 0;
    }
    snprintf(buf, sizeof(buf), SHM->i18nstr[cuser.language][910], age,
	     chicken_type[(int)mychicken->type], money);
    getdata_str(23, 0, buf, ans, sizeof(ans), DOECHO, "N");
    if (ans[0] == 'y') {
	snprintf(buf, sizeof(buf), SHM->i18nstr[cuser.language][911],
		cuser.userid, mychicken->name,
		chicken_type[(int)mychicken->type], money, ctime(&now));
	log_file(CHICKENLOG, buf, 1);
	mychicken->lastvisit = mychicken->name[0] = 0;
	passwd_update(usernum, &cuser);
	more(CHICKEN_PIC "/sell", YEA);
	demoney(money);
	return 1;
    }
    return 0;
}

static void
geting_old(int *hp, int *weight, int diff, int age)
{
    float           ex = 0.9;

    if (age > 70)
	ex = 0.1;
    else if (age > 30)
	ex = 0.5;
    else if (age > 20)
	ex = 0.7;

    diff /= 60 * 6;
    while (diff--) {
	*hp *= ex;
	*weight *= ex;
    }
}

/* 依時間變動的資料 */
void
time_diff(chicken_t * thechicken)
{
    int             diff;
    int             theage = ((now - thechicken->cbirth) / (60 * 60 * 24));

    thechicken->type %= NUM_KINDS;
    diff = (now - thechicken->lastvisit) / 60;

    if ((diff) < 1)
	return;

    if (theage > 13)		/* 老死 */
	geting_old(&thechicken->hp_max, &thechicken->weight, diff, age);

    thechicken->lastvisit = now;
    thechicken->weight -= thechicken->hp_max * diff / 540;	/* 體重 */
    if (thechicken->weight < 1) {
	thechicken->sick -= thechicken->weight / 10;	/* 餓得病氣上升 */
	thechicken->weight = 1;
    }
    /* 清潔度 */
    thechicken->clean += diff * time_change[(int)thechicken->type][CLEAN] / 30;

    /* 快樂度 */
    thechicken->happy -= diff / 60;
    if (thechicken->happy < 0)
	thechicken->happy = 0;
    thechicken->attack -=
	time_change[(int)thechicken->type][ATTACK] * diff / (60 * 32);
    if (thechicken->attack < 0)
	thechicken->attack = 0;
    /* 攻擊力 */
    thechicken->run -= time_change[(int)thechicken->type][RUN] * diff / (60 * 32);
    /* 敏捷 */
    if (thechicken->run < 0)
	thechicken->run = 0;
    thechicken->book -= time_change[(int)thechicken->type][BOOK] * diff / (60 * 32);
    /* 知識 */
    if (thechicken->book < 0)
	thechicken->book = 0;
    /* 氣質 */
    thechicken->temperament++;

    thechicken->satis -= diff / 60 / 3 * time_change[(int)thechicken->type][SATIS];
    /* 滿意度 */
    if (thechicken->satis < 0)
	thechicken->satis = 0;

    /* 髒病的 */
    if (thechicken->clean > 1000)
	thechicken->sick += (thechicken->clean - 400) / 10;

    if (thechicken->weight > 1)
	thechicken->sick -= diff / 60;
    /* 病氣恢護 */
    if (thechicken->sick < 0)
	thechicken->sick = 0;
    thechicken->tiredstrong -= diff *
	time_change[(int)thechicken->type][TIREDSTRONG] / 4;
    /* 疲勞 */
    if (thechicken->tiredstrong < 0)
	thechicken->tiredstrong = 0;
    /* hp_max */
    if (thechicken->hp >= thechicken->hp_max / 2)
	thechicken->hp_max +=
	    time_change[(int)thechicken->type][HP_MAX] * diff / (60 * 12);
    /* hp恢護 */
    if (!thechicken->sick)
	thechicken->hp +=
	    time_change[(int)thechicken->type][HP_MAX] * diff / (60 * 6);
    if (thechicken->hp > thechicken->hp_max)
	thechicken->hp = thechicken->hp_max;
    /* mm_max */
    if (thechicken->mm >= thechicken->mm_max / 2)
	thechicken->mm_max +=
	    time_change[(int)thechicken->type][MM_MAX] * diff / (60 * 8);
    /* mm恢護 */
    if (!thechicken->sick)
	thechicken->mm += diff;
    if (thechicken->mm > thechicken->mm_max)
	thechicken->mm = thechicken->mm_max;
}

static void
check_sick()
{
    chicken_t *mychicken = &cuser.mychicken;
    /* 髒病的 */
    if (mychicken->tiredstrong > mychicken->hp * 0.3 && mychicken->clean > 150)
	mychicken->sick += (mychicken->clean - 150) / 10;
    /* 累病的 */
    if (mychicken->tiredstrong > mychicken->hp * 1.3)
	mychicken->sick += time_change[(int)mychicken->type][SICK];
    /* 病氣太重還做事減hp */
    if (mychicken->sick > mychicken->hp / 5) {
	mychicken->hp -= (mychicken->sick - mychicken->hp / 5) / 4;
	if (mychicken->hp < 0)
	    mychicken->hp = 0;
    }
}

static int
deadtype(chicken_t * thechicken)
{
    chicken_t *mychicken = &cuser.mychicken;
    int             i;
    char            buf[150];

    if (thechicken->hp <= 0)	/* hp用盡 */
	i = 1;
    else if (thechicken->tiredstrong > thechicken->hp * 3)	/* 操勞過度 */
	i = 2;
    else if (thechicken->weight > thechicken->hp_max * 5)	/* 肥胖過度 */
	i = 3;
    else if (thechicken->weight == 1 &&
	     thechicken->sick > thechicken->hp_max / 4)
	i = 4;			/* 餓死了 */
    else if (thechicken->satis <= 0)	/* 很不滿意 */
	i = 5;
    else
	return 0;

    if (thechicken == mychicken) {
	snprintf(buf, sizeof(buf),
		 SHM->i18nstr[cuser.language][912],
		 cuser.userid, thechicken->name,
		 chicken_type[(int)thechicken->type],
		 ctime(&now));
	log_file(CHICKENLOG, buf, 1);
	mychicken->name[0] = 0;
	passwd_update(usernum, &cuser);
    }
    return i;
}

int
showdeadth(int type)
{
    switch (type) {
	case 1:
	more(CHICKEN_PIC "/nohp", YEA);
	break;
    case 2:
	more(CHICKEN_PIC "/tootired", YEA);
	break;
    case 3:
	more(CHICKEN_PIC "/toofat", YEA);
	break;
    case 4:
	more(CHICKEN_PIC "/nofood", YEA);
	break;
    case 5:
	more(CHICKEN_PIC "/nosatis", YEA);
	break;
    default:
	return 0;
    }
    more(CHICKEN_PIC "/deadth", YEA);
    return type;
}

int
isdeadth(chicken_t * thechicken)
{
    int             i;

    if (!(i = deadtype(thechicken)))
	return 0;
    return showdeadth(i);
}

static void
ch_changename()
{
    chicken_t *mychicken = &cuser.mychicken;
    char            buf[150], newname[20] = "";

    getdata_str(b_lines - 1, 0, SHM->i18nstr[cuser.language][913], newname, 18, DOECHO,
		mychicken->name);

    if (strlen(newname) >= 3 && strcmp(newname, mychicken->name)) {
	snprintf(buf, sizeof(buf),
		 SHM->i18nstr[cuser.language][914],
		 cuser.userid, mychicken->name,
		 chicken_type[(int)mychicken->type],
		 newname, ctime(&now));
	strlcpy(mychicken->name, newname, sizeof(mychicken->name));
	log_file(CHICKENLOG, buf, 1);
    }
}

static int
select_menu()
{
    chicken_t *mychicken = &cuser.mychicken;
    char            ch;

    reload_money();
    move(19, 0);
    prints(SHM->i18nstr[cuser.language][915],
	   cuser.money,
    /*
     * chicken_food[(int)mychicken->type],
     * chicken_type[(int)mychicken->type],
     * chicken_type[(int)mychicken->type],
     */
	   chicken_food[(int)mychicken->type],
	   food_price[(int)mychicken->type]);
    do {
	switch (ch = igetch()) {
	case '1':
	    ch_clean();
	    check_sick();
	    break;
	case '2':
	    ch_eat();
	    check_sick();
	    break;
	case '3':
	    ch_guess();
	    check_sick();
	    break;
	case '4':
	    ch_book();
	    check_sick();
	    break;
	case '5':
	    ch_kiss();
	    break;
	case '6':
	    ch_hit();
	    check_sick();
	    break;
	case '7':
	    ch_buyitem(food_price[(int)mychicken->type], CHICKEN_PIC "/food",
		       &mychicken->food, 1);
	    break;
	case '8':
	    ch_eatoo();
	    break;
	case '9':
	    ch_eatmedicine();
	    break;
	case 'O':
	case 'o':
	    ch_buyitem(100, CHICKEN_PIC "/buyoo", &mychicken->oo, 1);
	    break;
	case 'M':
	case 'm':
	    ch_buyitem(10, CHICKEN_PIC "/buymedicine", &mychicken->medicine, 1);
	    break;
	case 'N':
	case 'n':
	    ch_changename();
	    break;
	case 'K':
	case 'k':
	    ch_kill();
	    return 0;
	case 'S':
	case 's':
	    if (!ch_sell())
		break;
	case 'Q':
	case 'q':
	    return 0;
	}
    } while (ch < ' ' || ch > 'z');
    return 1;
}

static int
recover_chicken(chicken_t * thechicken)
{
    char            buf[200];
    int             price = egg_price[(int)thechicken->type], money = price + (rand() % price);

    if (now - thechicken->lastvisit > (60 * 60 * 24 * 7))
	return 0;
    outmsg(SHM->i18nstr[cuser.language][916]);
    bell();
    igetch();
    outmsg(SHM->i18nstr[cuser.language][917]);
    bell();
    igetch();
    snprintf(buf, sizeof(buf), SHM->i18nstr[cuser.language][918],
	     chicken_type[(int)thechicken->type], price * 2);
    outmsg(buf);
    bell();
    getdata_str(21, 0, SHM->i18nstr[cuser.language][919], buf, 3, LCECHO, "N");
    if (buf[0] == 'y' || buf[0] == 'Y') {
	reload_money();
	if (cuser.money < price * 2) {
	    outmsg(SHM->i18nstr[cuser.language][920]);
	    bell();
	    igetch();
	    return 0;
	}
	strlcpy(thechicken->name, SHM->i18nstr[cuser.language][921], sizeof(thechicken->name));
	thechicken->hp = thechicken->hp_max;
	thechicken->sick = 0;
	thechicken->satis = 2;
	vice(money, SHM->i18nstr[cuser.language][922]);
	snprintf(buf, sizeof(buf),
		 SHM->i18nstr[cuser.language][923], money);
	outmsg(buf);
	bell();
	igetch();
	return 1;
    }
    outmsg(SHM->i18nstr[cuser.language][924]);
    bell();
    igetch();
    thechicken->lastvisit = 0;
    passwd_update(usernum, &cuser);
    return 0;
}

#define lockreturn0(unmode, state) if(lockutmpmode(unmode, state)) return 0

void copy_i18nstring() {
	int i;
	for (i = 0; i < NUM_KINDS; i++) {
		chicken_type[i] = SHM->i18nstr[cuser.language][806 + i];
		chicken_food[i] = SHM->i18nstr[cuser.language][821 + i];
		attack_type[i] = SHM->i18nstr[cuser.language][836 + i];
	}
	for (i = 0; i < 16; i++)
		damage_degree[i] = SHM->i18nstr[cuser.language][851 + i];
}
int
chicken_main()
{
    chicken_t *mychicken = &cuser.mychicken;
    copy_i18nstring();
    lockreturn0(CHICKEN, LOCK_MULTI);
    reload_chicken();
    age = ((now - mychicken->cbirth) / (60 * 60 * 24));
    if (!mychicken->name[0] && !recover_chicken(mychicken) && !new_chicken()) {
	unlockutmpmode();
	return 0;
    }
    do {
	time_diff(mychicken);
	if (isdeadth(mychicken))
	    break;
	show_chicken_data(mychicken, NULL);
    } while (select_menu());
    reload_money();
    passwd_update(usernum, &cuser);
    unlockutmpmode();
    return 0;
}

int
chickenpk(int fd)
{
    chicken_t *mychicken = &cuser.mychicken;
    char            mateid[IDLEN + 1], data[200], buf[200];
    int             ch = 0;

    userinfo_t     *uin = &SHM->uinfo[currutmp->destuip];
    userec_t        ouser;
    chicken_t      *ochicken = &ouser.mychicken;
    int             r, attmax, i, datac, duid = currutmp->destuid, catched = 0,
                    count = 0;

    lockreturn0(CHICKEN, LOCK_MULTI);

    strlcpy(mateid, currutmp->mateid, sizeof(mateid));
    /* 把對手的id用local buffer記住 */

    getuser(mateid);
    memcpy(&ouser, &xuser, sizeof(userec_t));
    reload_chicken();
    if (!ochicken->name[0] || !mychicken->name[0]) {
	outmsg(SHM->i18nstr[cuser.language][925]);	/* Ptt:妨止page時把寵物賣掉 */
	bell();
	refresh();
	add_io(0, 0);
	close(fd);
	unlockutmpmode();
	sleep(1);
	return 0;
    }
    show_chicken_data(ochicken, mychicken);
    add_io(fd, 3);		/* 把fd加到igetch監視 */
    while (1) {
	r = rand();
	ch = igetkey();
	getuser(mateid);
	memcpy(&ouser, &xuser, sizeof(userec_t));
	reload_chicken();
	show_chicken_data(ochicken, mychicken);
	time_diff(mychicken);

	i = mychicken->attack * mychicken->hp / mychicken->hp_max;
	for (attmax = 2; (i = i * 9 / 10); attmax++);

	if (ch == I_OTHERDATA) {
	    count = 0;
	    datac = recv(fd, data, sizeof(data), 0);
	    if (datac <= 1)
		break;
	    move(17, 0);
	    outs(data + 1);
	    switch (data[0]) {
	    case 'c':
		catched = 1;
		move(16, 0);
		outs(SHM->i18nstr[cuser.language][926]);
		break;
	    case 'd':
		move(16, 0);
		outs(SHM->i18nstr[cuser.language][927]);
		break;
	    }
	    if (data[0] == 'd' || data[0] == 'q' || data[0] == 'l')
		break;
	    continue;
	} else if (currutmp->turn) {
	    count = 0;
	    currutmp->turn = 0;
	    uin->turn = 1;
	    mychicken->tiredstrong++;
	    switch (ch) {
	    case 'y':
		if (catched == 1) {
		    snprintf(data, sizeof(data),
			     SHM->i18nstr[cuser.language][928], ochicken->name);
		}
		break;
	    case 'n':
		catched = 0;
	    default:
	    case 'k':
		r = r % (attmax + 2);
		if (r) {
		    snprintf(data, sizeof(data),
			     SHM->i18nstr[cuser.language][929], mychicken->name,
			     damage_degree[r / 3 > 15 ? 15 : r / 3],
			     attack_type[(int)mychicken->type],
			     ochicken->name, r);
		    ochicken->hp -= r;
		} else
		    snprintf(data, sizeof(data),
			     SHM->i18nstr[cuser.language][930], mychicken->name);
		break;
	    case 'o':
		if (mychicken->oo > 0) {
		    mychicken->oo--;
		    mychicken->hp += 300;
		    if (mychicken->hp > mychicken->hp_max)
			mychicken->hp = mychicken->hp_max;
		    mychicken->tiredstrong = 0;
		    snprintf(data, sizeof(data), SHM->i18nstr[cuser.language][931],
			     mychicken->name);
		} else
		    snprintf(data, sizeof(data),
			    SHM->i18nstr[cuser.language][932],
			    mychicken->name);
		break;
	    case 'q':
		if (r % (mychicken->run + 1) > r % (ochicken->run + 1))
		    snprintf(data, sizeof(data), SHM->i18nstr[cuser.language][933],
			     mychicken->name);
		else
		    snprintf(data, sizeof(data),
			     SHM->i18nstr[cuser.language][934],
			     mychicken->name, ochicken->name);
		break;
	    }
	    if (deadtype(ochicken)) {
		strtok(data, "\n");
		strlcpy(buf, data, sizeof(buf));
		snprintf(data, sizeof(data), SHM->i18nstr[cuser.language][935],
			 buf + 1, ochicken->name, mychicken->name);
	    }
	    move(17, 0);
	    outs(data + 1);
	    i = strlen(data) + 1;
	    passwd_update(duid, &ouser);
	    passwd_update(usernum, &cuser);
	    send(fd, data, i, 0);
	    if (data[0] == 'q' || data[0] == 'd')
		break;
	} else {
	    move(17, 0);
	    if (count++ > 30)
		break;
	}
    }
    add_io(0, 0);		/* 把igetch恢復回 */
    pressanykey();
    close(fd);
    showdeadth(deadtype(mychicken));
    unlockutmpmode();
    return 0;
}
