#include "bbs.h"

/* Created by Chia-Kuang Yu, Apr 26th, 2004 */

#define MAX_LANGFILE_SIZE (512 * 1024)
#define MAX_STRING_LEN 4096
#define MAX_BUFFER_SIZE (MAX_LANG * MAX_STRING * 20)

#ifdef I18N
int parse_hex(unsigned char *buf, unsigned char *ret) {
	int len = 0;
	unsigned char table[] = "0123456789ABCDEF";
	unsigned char digit[2];
	unsigned char *ptr;
	digit[0] = toupper(*buf);
	digit[1] = toupper(*(buf + 1));
	if ((ptr = strchr(table, digit[0]))) {
		len++;
		*ret = (unsigned char)(ptr - table);
		if ((ptr = strchr(table, digit[1]))) {
			len++;
			*ret = *ret * 16 + (unsigned char)(ptr - table);
		}
	}
	return len;
}

int parse_oct(unsigned char *buf, unsigned char *ret) {
	int i = 0, length = 0;
	int ret_val = 0;
	while(buf[i] >= '0' && buf[i] <= '7' && i < 3) {
		ret_val = ret_val * 8 + buf[i] - '0';
		length++;
		i++;
	}
	if (ret_val > 255)
		length = 0;
	else
		*ret = (unsigned char)ret_val;
	return length;
}
unsigned char parse_escape(char *buf, int *length) {
	unsigned char input[] = "abfnrtv\\?'\"";
	unsigned char output[] = "\a\b\f\n\r\t\v\\\?\'\"";
	unsigned char ret = *buf, *ptr;
	int len = 1;
	*length = 1;
	for (ptr = input; *ptr != 0; ptr++)
		if (*buf == *ptr)
			return output[ptr - input];
	
	/* hexidecimal number*/
	if (*buf == 'x') {
		len = parse_hex(buf + 1, &ret);
		*length = len;	
				
	/* octal number */
	}
	else if(isdigit(*buf)) {
		len = parse_oct(buf, &ret);
		*length = len;
	}
	return ret;
}

void load_language(char *filename, char *lang_str[MAX_STRING], int *_offset) {
	int i, num_string, string_index, filesize;
	int inside_string, digit, length;
	char *buf = 0, *start = 0, temp[256];
	char escape_sequence;
	FILE *f;	
	char *p_str_head = SHM->i18nstrbody;
	int offset = *_offset;
	sprintf (temp, BBSHOME"/etc/%s", filename);
	f = fopen(temp, "r");
	if (f) {
		if ((buf = (char *)(malloc(sizeof(char) * MAX_LANGFILE_SIZE)))) {
			num_string = string_index = 0;
			memset(lang_str, 0, sizeof(char *) * MAX_STRING);
			filesize = fread(buf, 1, MAX_LANGFILE_SIZE, f);
			inside_string = string_index = 0;
			for (i = 0; i < filesize; i++) {
				if (isdigit(buf[i]) && inside_string == FALSE) {
					string_index = string_index * 10 + buf[i] - '0';
					digit++;	
				}
				else if (buf[i] == '"' && inside_string == FALSE) {
					inside_string = TRUE;
					start = p_str_head + offset;
					continue;
				}
				else if (inside_string == TRUE
							&& buf[i] == '"' && buf[i -1] == '\\' && buf[i - 2] != '\\') {
				}
				else if (i < filesize - 1 && buf[i] == '\\' && buf[i + 1] == '\n') {
					i++;
					continue;
				}
				else if (buf[i] == '"' && inside_string == TRUE) {
					inside_string = FALSE;
					p_str_head[offset] = 0;
					if (string_index >= 0 && string_index < MAX_STRING) {
						lang_str[string_index] = start;
						printf("%d \"%s\"\n", string_index, start);
					}
					string_index = 0;
					offset++;
					continue;
				}
				else if (buf[i] == '\\' && inside_string == TRUE) {
					escape_sequence = parse_escape(buf + i + 1, &length);
					if (length) {
						i += length;
						buf[i] = escape_sequence;
					}
					else
						i++;
				}
				if (inside_string == TRUE) {
					if (offset < MAX_BUFFER_SIZE) {
						p_str_head[offset] = buf[i];
						offset++;
					}
				}
			}	
			for (i = 0; i < MAX_STRING; i++)
				if (lang_str[i] == 0) {
					printf ("DEBUG: %d\n", i);
					lang_str[i] = p_str_head;
				}
			free(buf);
			printf("%s loaded..\n", temp);
			fflush(0);
		}
		else
			printf("Warning: out of memory!!\n");
		fclose(f);		
	}
	else {
		printf ("Warning: %s doesn't exist!!\n", buf);
	}
	*_offset = offset;
}

char *lang_file[] = LANG_FILE;
void load_i18nstring() {
	int i, offset;
	*SHM->i18nstrbody = 0;
	offset = 1;
	for (i = 0; i < MAX_LANG; i++) {
		load_language(lang_file[i], SHM->i18nstr[i], &offset);
	}
}
#endif
