#include "bbs.h"

#ifdef I18N
int main() {
	attach_SHM();
	load_i18nstring();
	return 0;
}
#endif
