/* $Id$ */
#include "bbs.h"

static int
card_remain(int cards[])
{
    int             i, temp = 0;

    for (i = 0; i < 52; i++)
	temp += cards[i];
    if (temp == 52)
	return 1;
    return 0;
}

/* 0 Spare ,  1 heart , ...3 dimon */
static int
card_flower(int card)
{
    return (card / 13);
}

/* 1...13 */
static int
card_number(int card)
{
    return (card % 13 + 1);
}

static int
card_select(int *now)
{
    char           *cc[2] = {"\033[44m            \033[m",
    I18N[580]};

    while (1) {
	move(20, 0);
	clrtoeol();
	prints("%s%s%s%s%s", (*now == 0) ? cc[1] : cc[0],
	       (*now == 1) ? cc[1] : cc[0],
	       (*now == 2) ? cc[1] : cc[0],
	       (*now == 3) ? cc[1] : cc[0],
	       (*now == 4) ? cc[1] : cc[0]);
	switch (igetch()) {
	case 'Q':
	case 'q':
	    return 0;
	case '+':
	case ',':
	    return 1;
	case '\r':
	    return -1;
	case KEY_LEFT:
	    *now = (*now + 4) % 5;
	    break;
	case KEY_RIGHT:
	    *now = (*now + 1) % 5;
	    break;
	case '1':
	    *now = 0;
	    break;
	case '2':
	    *now = 1;
	    break;
	case '3':
	    *now = 2;
	    break;
	case '4':
	    *now = 3;
	    break;
	case '5':
	    *now = 4;
	    break;
	}
    }
}

static void
card_display(int cline, int number, int flower, int show)
{
    int             color = 31;
    char           *cn[13] = {I18N[581], I18N[582], I18N[583], I18N[584], I18N[585], I18N[586],
    I18N[587], I18N[588], I18N[589], "10", I18N[590], I18N[591], I18N[592]};
    if (flower == 0 || flower == 3)
	color = 36;
    if ((show < 0) && (cline > 1 && cline < 8))
	prints(I18N[593]);
    else
	switch (cline) {
	case 1:
	    prints(I18N[594]);
	    break;
	case 2:
	    prints(I18N[595], color, cn[number - 1]);
	    break;
	case 3:
	    if (flower == 1)
		prints(I18N[596], color);
	    else
		prints(I18N[597], color);
	    break;
	case 4:
	    if (flower == 1)
		prints(I18N[598], color);
	    else if (flower == 3)
		prints(I18N[599], color);
	    else
		prints(I18N[600], color);
	    break;
	case 5:
	    if (flower == 0)
		prints(I18N[601], color);
	    else if (flower == 3)
		prints(I18N[602], color);
	    else
		prints(I18N[603], color);
	    break;
	case 6:
	    if (flower == 0)
		prints(I18N[604], color);
	    else if (flower == 3)
		prints(I18N[605], color);
	    else
		prints(I18N[606], color);
	    break;
	case 7:
	    prints(I18N[607], color, cn[number - 1]);
	    break;
	case 8:
	    prints(I18N[608]);
	    break;
	}
}

static void
card_show(int cpu[], int c[], int me[], int m[])
{
    int             i, j;

    for (j = 0; j < 8; j++) {
	move(2 + j, 0);
	clrtoeol();
	for (i = 0; i < 5 && cpu[i] >= 0; i++)
	    card_display(j + 1, card_number(cpu[i]),
			 card_flower(cpu[i]), c[i]);
    }

    for (j = 0; j < 8; j++) {
	move(11 + j, 0);
	clrtoeol();
	for (i = 0; i < 5 && me[i] >= 0; i++)
	    card_display(j + 1, card_number(me[i]), card_flower(me[i]), m[i]);
    }
}
static void
card_new(int cards[])
{
    memset(cards, 0, sizeof(int) * 52);
}

static int
card_give(int cards[])
{
    int             i, error;
    for (error = 0, i = rand() % 52; cards[i] == 1 && error < 52; error++, i = rand() % 52);
    if (error == 52)
	card_new(cards);
    /* Ptt: �o�䦳 dead lock �����D */
    cards[i] = 1;
    return i;
}

static void
card_start(char name[])
{
    clear();
    stand_title(name);
    move(1, 0);
    prints(I18N[609]);
    move(10, 0);
    prints(I18N[610]);
    move(19, 0);
    prints(I18N[611]);
}

static int
card_99_add(int i, int aom, int count)
{
    if (i == 4 || i == 5 || i == 11)
	return count;
    else if (i == 12)
	return count + 20 * aom;
    else if (i == 10)
	return count + 10 * aom;
    else if (i == 13)
	return 99;
    else
	return count + i;
}

static int
card_99_cpu(int cpu[], int *count)
{
    int             stop = -1;
    int             twenty = -1;
    int             ten = -1;
    int             kill = -1;
    int             temp, num[10];
    int             other = -1;
    int             think = 99 - (*count);
    int             i, j;

    for (i = 0; i < 10; i++)
	num[i] = -1;
    for (i = 0; i < 5; i++) {
	temp = card_number(cpu[i]);
	if (temp == 4 || temp == 5 || temp == 11)
	    stop = i;
	else if (temp == 12)
	    twenty = i;
	else if (temp == 10)
	    ten = i;
	else if (temp == 13)
	    kill = i;
	else {
	    other = i;
	    num[temp] = i;
	}
    }
    for (j = 9; j > 0; j--)
	if (num[j] >= 0 && j != 4 && j != 5 && think >= j) {
	    (*count) += j;
	    return num[j];
	}
    if ((think >= 20) && (twenty >= 0)) {
	(*count) += 20;
	return twenty;
    } else if ((think >= 10) && (ten >= 0)) {
	(*count) += 10;
	return ten;
    } else if (stop >= 0)
	return stop;
    else if (kill >= 0) {
	(*count) = 99;
	return kill;
    } else if (ten >= 0) {
	(*count) -= 10;
	return ten;
    } else if (twenty >= 0) {
	(*count) -= 20;
	return twenty;
    } else {
	(*count) += card_number(cpu[0]);
	return 0;
    }
}

int
card_99()
{
    int             i, j, turn;
    int             cpu[5], c[5], me[5], m[5];
    int             cards[52];
    int             count = 0;
    char           *ff[4] = {I18N[612], I18N[613],
    I18N[614], I18N[615]};
    char           *cn[13] = {I18N[616], I18N[617], I18N[618], I18N[619], I18N[620], I18N[621],
    I18N[622], I18N[623], I18N[624], "10", I18N[625], I18N[626], I18N[627]};
    for (i = 0; i < 5; i++)
	cpu[i] = c[i] = me[i] = m[i] = -1;
    setutmpmode(CARD_99);
    card_start(I18N[628]);
    card_new(cards);
    for (i = 0; i < 5; i++) {
	cpu[i] = card_give(cards);
	me[i] = card_give(cards);
	m[i] = 1;
    }
    card_show(cpu, c, me, m);
    j = 0;
    turn = 1;
    move(21, 0);
    clrtoeol();
    prints(I18N[629], count, 99 - count);
    prints(I18N[630]);
    while (1) {
	i = card_select(&j);
	if (i == 0)		/* ���C�� */
	    return 0;
	count = card_99_add(card_number(me[j]), i, count);
	move(21 + (turn / 2) % 2, 0);
	clrtoeol();
	prints(I18N[631],
	       turn, ff[card_flower(me[j])],
	       cn[card_number(me[j]) - 1], count, 99 - count);
	me[j] = card_give(cards);
	turn++;
	if (count < 0)
	    count = 0;
	card_show(cpu, c, me, m);
	pressanykey();
	if (count > 99) {
	    move(22, 0);
	    clrtoeol();
	    prints(I18N[632],
		   turn, count, 99 - count);
	    pressanykey();
	    return 0;
	}
	i = card_99_cpu(cpu, &count);
	move(21 + (turn / 2 + 1) % 2, 40);
	prints(I18N[633],
	       turn, ff[card_flower(cpu[i])],
	       cn[card_number(cpu[i]) - 1], count, 99 - count);
	cpu[i] = card_give(cards);
	turn++;
	if (count < 0)
	    count = 0;
	if (count > 99) {
	    move(22, 0);
	    clrtoeol();
	    prints(I18N[634],
		   turn, count, 99 - count);
	    pressanykey();
	    return 0;
	}
	if (!card_remain(cards)) {
	    card_new(cards);
	    for (i = 0; i < 5; i++) {
		cards[me[i]] = 1;
		cards[cpu[i]] = 1;
	    }
	}
    }
}

#define PMONEY     (10)
#define TEN_HALF   (5)		/* �Q�I�b��Ticket */
#define JACK      (10)		/* �³ǧJ��Ticket */
#define NINE99    (99)		/* 99    ��Ticket */

static int
game_log(int type, int money)
{
    FILE           *fp;

    if (money > 0)
	demoney(money);

    switch (type) {
    case JACK:
	fp = fopen(BBSHOME "/etc/card/jack.log", "a");
	if (!fp)
	    return 0;
	fprintf(fp, "%s win:%d\n", cuser.userid, money);
	fclose(fp);
	break;
    case TEN_HALF:
	fp = fopen(BBSHOME "/etc/card/tenhalf.log", "a");
	if (!fp)
	    return 0;
	fprintf(fp, "%s win:%d\n", cuser.userid, money);
	fclose(fp);
	break;
    }
    return 0;
}

static int
card_double_ask()
{
    char            buf[100], buf2[3];

    snprintf(buf, sizeof(buf),
	     I18N[635],
	     cuser.userid, cuser.money, JACK);
    reload_money();
    if (cuser.money < JACK)
	return 0;
    getdata(20, 0, buf, buf2, sizeof(buf2), LCECHO);
    if (buf2[0] == 'y' || buf2[0] == 'Y')
	return 1;
    return 0;
}

static int
card_ask()
{
    char            buf[100], buf2[3];

    snprintf(buf, sizeof(buf), I18N[636],
	    cuser.userid, cuser.money);
    getdata(20, 0, buf, buf2, sizeof(buf2), LCECHO);
    if (buf2[0] == 'y' || buf2[0] == 'Y')
	return 1;
    return 0;
}

static int
card_alls_lower(int all[])
{
    int             i, count = 0;
    for (i = 0; i < 5 && all[i] >= 0; i++)
	if (card_number(all[i]) <= 10)
	    count += card_number(all[i]);
	else
	    count += 10;
    return count;
}

static int
card_alls_upper(int all[])
{
    int             i, count;

    count = card_alls_lower(all);
    for (i = 0; i < 5 && all[i] >= 0 && count <= 11; i++)
	if (card_number(all[i]) == 1)
	    count += 10;
    return count;
}

static int
card_jack(int *db)
{
    int             i, j;
    int             cpu[5], c[5], me[5], m[5];
    int             cards[52];

    for (i = 0; i < 5; i++)
	cpu[i] = c[i] = me[i] = m[i] = -1;

    if ((*db) < 0) {
	card_new(cards);
	card_start(I18N[637]);
	for (i = 0; i < 2; i++) {
	    cpu[i] = card_give(cards);
	    me[i] = card_give(cards);
	}
    } else {
	card_start(I18N[638]);
	cpu[0] = card_give(cards);
	cpu[1] = card_give(cards);
	me[0] = *db;
	me[1] = card_give(cards);
    }
    c[1] = m[0] = m[1] = 1;
    card_show(cpu, c, me, m);
    if ((card_number(me[0]) == 0 && card_number(me[1]) == 12) ||
	(card_number(me[1]) == 0 && card_number(me[0]) == 12)) {
	if (card_flower(me[0]) == 0 && card_flower(me[1]) == 0) {
	    game_log(JACK, JACK * 10);
	    vmsg(I18N[639], JACK * 10);
	    return 0;
	} else {
	    game_log(JACK, JACK * 5);
	    vmsg(I18N[640], JACK * 5);
	    return 0;
	}
    }
    if ((card_number(cpu[0]) == 0 && card_number(cpu[1]) == 12) ||
	(card_number(cpu[1]) == 0 && card_number(cpu[0]) == 12)) {
	c[0] = 1;
	card_show(cpu, c, me, m);
	game_log(JACK, 0);
	vmsg(I18N[641]);
	return 0;
    }
    if ((*db < 0) && (card_number(me[0]) == card_number(me[1])) &&
	(card_double_ask())) {
	*db = me[1];
	me[1] = card_give(cards);
	card_show(cpu, c, me, m);
    }
    i = 2;
    while (i < 5 && card_ask()) {
	me[i] = card_give(cards);
	m[i] = 1;
	card_show(cpu, c, me, m);
	if (card_alls_lower(me) > 21) {
	    game_log(JACK, 0);
	    vmsg(I18N[642]);
	    return 0;
	}
	i++;
	if ((i == 3) && (card_number(me[0]) == 7) &&
	    (card_number(me[1]) == 7) && (card_number(me[2]) == 7)) {
	    game_log(JACK, JACK * 7);
	    vmsg(I18N[643], JACK * 7);
	    return 0;
	}
    }
    if (i == 5) {		/* �L���� */
	game_log(JACK, JACK * 5);
	vmsg(I18N[644], 5 * JACK);
	return 0;
    }
    j = 2;
    c[0] = 1;
    while ((card_alls_upper(cpu) < card_alls_upper(me)) ||
	((card_alls_upper(cpu) == card_alls_upper(me) && j < i) && j < 5)) {
	cpu[j] = card_give(cards);
	c[j] = 1;
	if (card_alls_lower(cpu) > 21) {
	    card_show(cpu, c, me, m);
	    game_log(JACK, JACK * 2);
	    vmsg(I18N[645], JACK * 2);
	    return 0;
	}
	j++;
    }
    card_show(cpu, c, me, m);
    game_log(JACK, 0);
    vmsg(I18N[646]);
    return 0;
}

int
g_card_jack()
{
    int             db;
    char            buf[3];

    setutmpmode(JACK_CARD);
    while (1) {
	reload_money();
	if (cuser.money < JACK) {
	    outs(I18N[647]);
	    return 0;
	}
	getdata(b_lines - 1, 0, I18N[648],
		buf, 3, LCECHO);
	if ((*buf != 'y') && (*buf != 'Y'))
	    break;
	else {
	    db = -1;
	    vice(PMONEY, I18N[649]);
	    card_jack(&db);
	    if (db >= 0)
		card_jack(&db);
	}
    }
    return 0;
}

static int
card_all(int all[])
{
    int             i, count = 0;

    for (i = 0; i < 5 && all[i] >= 0; i++)
	if (card_number(all[i]) <= 10)
	    count += 2 * card_number(all[i]);
	else
	    count += 1;
    return count;
}

static int
ten_helf()
{
    int             i, j;
    int             cpu[5], c[5], me[5], m[5];
    int             cards[52];

    card_start(I18N[650]);
    card_new(cards);
    for (i = 0; i < 5; i++)
	cpu[i] = c[i] = me[i] = m[i] = -1;

    cpu[0] = card_give(cards);
    me[0] = card_give(cards);
    m[0] = 1;
    card_show(cpu, c, me, m);
    i = 1;
    while (i < 5 && card_ask()) {
	me[i] = card_give(cards);
	m[i] = 1;
	card_show(cpu, c, me, m);
	if (card_all(me) > 21) {
	    game_log(TEN_HALF, 0);
	    vmsg(I18N[651]);
	    return 0;
	}
	i++;
    }
    if (i == 5) {		/* �L���� */
	game_log(TEN_HALF, PMONEY * 5);
	vmsg(I18N[652], 5 * PMONEY);
	return 0;
    }
    j = 1;
    c[0] = 1;
    while (j < 5 && ((card_all(cpu) < card_all(me)) ||
		     (card_all(cpu) == card_all(me) && j < i))) {
	cpu[j] = card_give(cards);
	c[j] = 1;
	if (card_all(cpu) > 21) {
	    card_show(cpu, c, me, m);
	    game_log(TEN_HALF, PMONEY * 2);
	    vmsg(I18N[653], PMONEY * 2);
	    return 0;
	}
	j++;
    }
    card_show(cpu, c, me, m);
    game_log(TEN_HALF, 0);
    vmsg(I18N[654]);
    return 0;
}

int
g_ten_helf()
{
    char            buf[3];

    setutmpmode(TENHALF);
    while (1) {
	reload_money();
	if (cuser.money < TEN_HALF) {
	    outs(I18N[655]);
	    return 0;
	}
	getdata(b_lines - 1, 0,
		I18N[656],
		buf, 3, LCECHO);
	if (buf[0] != 'y' && buf[0] != 'Y')
	    return 0;
	else {
	    vice(PMONEY, I18N[657]);
	    ten_helf();
	}
    }
    return 0;
}
