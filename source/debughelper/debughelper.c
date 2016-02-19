#include "debughelper.h"

#if defined(DEBUG_GECKO) || defined(DEBUG_WIFI)

bool __debughelper_init(bool waitgdb){
#if DEBUG_GECKO
	return init_gecko();
#else
	return init_wifidebug(waitgdb);
#endif
}

void __debughelper_hexdump(void *d, int len)
{
	u8 *data;
	int i, off;
	data = (u8*) d;

	debughelper_printf("\n       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F  0123456789ABCDEF");
	debughelper_printf("\n====  ===============================================  ================\n");

	for (off = 0; off < len; off += 16)
	{
		debughelper_printf("%04x  ", off);
		for (i = 0; i < 16; i++)
			if ((i + off) >= len)
				debughelper_printf("   ");
			else debughelper_printf("%02x ", data[off + i]);

		debughelper_printf(" ");
		for (i = 0; i < 16; i++)
			if ((i + off) >= len)
				debughelper_printf(" ");
			else debughelper_printf("%c", ASCII(data[off + i]));
		debughelper_printf("\n");
	}
}
#else
bool __return_true()
{
	return true;
}
#endif
