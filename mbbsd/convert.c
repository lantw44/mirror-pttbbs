/* $Id: convert.c 1374 2003-11-27 14:11:40Z victor $ */
#include "bbs.h"

/*
 * 只有當 CONVERT 這個 option 開起來時才會用到這些東西。
 * 
 * Main idea
 * =========
 *   為了達到跟原本 read/write 的 transparency，這邊分別對每種 encoding 定義
 * 了他們的 converting function。外界使用時透過 read_wrapper 跟 write_wrapper
 * (function pointer)，所以只須把 wrapper 指到 convert.c 中定義的 converting
 * function 即可達到目的。
 * 
 *   目前在轉換 encoding 方面，使用了 libhz 這個 library。
 * 
 * Subroutines
 * ===========
 * 
 * void set_converting_type(int which);
 *   事實上，這邊不希望外界直接 override wrapper 的值。使用這個界面可以設定
 *   轉換的 encoding。
 * 
 *   which 可以是:
 *  	CONV_NORMAL	不作轉換
 * 	CONV_GB		Big5 <-> GB
 * 	CONV_UTF8	Big5 <-> UTF-8
 */

#ifdef CONVERT

extern read_write_type write_type;
extern read_write_type read_type;

unsigned char *gb2big(unsigned char *, int* , int);
unsigned char *big2gb(unsigned char *, int* , int);
unsigned char *utf8_uni(unsigned char *, int *, int);
unsigned char *uni_utf8(unsigned char *, int *, int);
unsigned char *uni2big(unsigned char *, int* , int);
unsigned char *big2uni(unsigned char *, int* , int);

static int gb_read(int fd, void *buf, size_t count)
{
    count = read(fd, buf, count);
    if (count > 0)
	gb2big((char *)buf, &count, 0);
    return count;
}

static int gb_write(int fd, void *buf, size_t count)
{
    big2gb((char *)buf, &count, 0);
    return write(fd, buf, count);
}

static int utf8_read(int fd, void *buf, size_t count)
{
    count = read(fd, buf, count);
    if (count > 0) {
	utf8_uni(buf, &count, 0);
	uni2big(buf, &count, 0);
	((char *)buf)[count] = 0;
    }
    return count;
}

static int utf8_write(int fd, void *buf, size_t count)
{
    big2uni(buf, &count, 0);
    uni_utf8(buf, &count, 0);
    ((char *)buf)[count] = 0;
    return write(fd, buf, count);
}

void set_converting_type(int which)
{
    if (which == CONV_NORMAL) {
	read_type = read;
	write_type = (read_write_type)write;
    }
    else if (which == CONV_GB) {
	read_type = gb_read;
	write_type = gb_write;
    }
    else if (which == CONV_UTF8) {
	read_type = utf8_read;
	write_type = utf8_write;
    }
}

#endif
