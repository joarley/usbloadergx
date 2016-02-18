#ifndef SOURCE_DEBUGHELPER_DEBUGHELPER_H_
#define SOURCE_DEBUGHELPER_DEBUGHELPER_H_

#include <gccore.h>

#ifdef DEBUG_GECKO
#include "gecko.h"
#endif

#ifdef DEBUG_WIFI
#include "wifidebug.h"
#endif

#ifndef DEBUG_WAIT_GDB
#define DEBUG_WAIT_GDB false
#endif

#define ASCII(CHAR) (((CHAR) < 0x20)?('.'): (CHAR > 0x7E) ? ('.'): (CHAR))

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(DEBUG_GECKO) || defined(DEBUG_WIFI)
bool __debughelper_init(bool waitgdb);
void __debughelper_hexdump(void *d, int len);
#else
	bool __return_true();
	#define debughelper_init() __return_true()
	#define debughelper_printf(...) do{}while(false)
	#define debughelper_hexdump(...)  do{}while(false)
	#define debughelper_redirect_output()  do{}while(false)
#endif

#ifdef DEBUG_GECKO
	#define debughelper_printf(...) gprintf_gecko(__VA_ARGS__)
	#define debughelper_init() __debughelper_init(false)
	#define debughelper_hexdump(...) __debughelper_hexdump(...)
	#define debughelper_redirect_output() redirect_output_gecko()
#elif DEBUG_WIFI
	#define debughelper_printf(...) gprintf_wifidebug(__VA_ARGS__)
	#define debughelper_init() __debughelper_init(DEBUG_WAIT_GDB)
	#define debughelper_hexdump(...) __debughelper_hexdump(...)
	#define debughelper_redirect_output() redirect_output_wifidebug()
#endif

#ifdef __cplusplus
}
#endif

#endif /* SOURCE_DEBUGHELPER_DEBUGHELPER_H_ */
