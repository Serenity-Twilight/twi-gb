#include <stddef.h>
#include <stdio.h>
#define GB_LOG_MAX_LEVEL LVL_TRC
#include "gb/log.h"
#include "gb/pak.h"
#include "gb/pak/header.h"
#include "gb/pak/typedef.h"

int main() {
	struct gb_pak* pak = gb_pak_create("tetris.gb");
	if (pak != NULL)
		fputs("Success\n", stdout);
	else {
		fputs("Failure\n", stdout);
		return 1;
	}

	char dumpbuf[8192];
	pakhdr_dump(dumpbuf, sizeof(dumpbuf), pak->rom, NULL);
	LOGI("Pak header info:\n%s", dumpbuf);
	gb_pak_delete(pak);
	return 0;
} // end main()

