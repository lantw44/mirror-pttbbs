#include "bbs.h"

#define MAX_LANGFILE_SIZE (512 * 1024)
#define MAX_STRING_LEN 4096
#define MAX_BUFFER_SIZE (MAX_LANG * MAX_STRING * 20)

#ifdef I18N
void load_language(char *filename, char *lang_str[MAX_STRING], int *_offset) {
	int i, num_string, string_index, filesize;
	int inside_string, digit;
	char *buf, *start, temp[256];
	FILE *f;	
	char *p_str_head = SHM->i18nstrbody;
	int offset = *_offset;
	sprintf (temp, "~bbs/etc/%s", filename);
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
						//printf ("%d \"%s\"\n", string_index, start);
						//printf("%d\n", strlen(lang_str[string_index]));
						//fflush(0);
					}
					string_index = 0;
					offset++;
					continue;
				}
				if (inside_string == TRUE) {
					if (offset < MAX_BUFFER_SIZE) {
						p_str_head[offset] = buf[i];
						offset++;
					}
				}
			}	
			for (i = 0; i < MAX_STRING; i++)
				if (lang_str[i] == 0)
					lang_str[i] = p_str_head;
			free(buf);
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

char *lang_file[] = {LANG_FILE};
void load_i18nstring() {
	int i, offset;
	*SHM->i18nstrbody = 0;
	offset = 1;
	for (i = 0; i < MAX_LANG; i++)
		load_language(lang_file[i], SHM->i18nstr[i], &offset);
	//printf ("%d\n", offset);
}
#endif
