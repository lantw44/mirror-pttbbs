#include "bbs.h"

int main() {
#ifdef I18N
	attach_SHM();
	load_i18nstring();
#endif
	return 0;
}
