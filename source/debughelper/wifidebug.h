#ifndef SOURCE_WIFI_DEBUG_H_
#define SOURCE_WIFI_DEBUG_H_

#include <gccore.h>

#ifdef __cplusplus
extern "C"
{
#endif

bool init_wifidebug(bool waitgdb);
void debughelper_deinit_wifidebug();
void printf_wifidebug(const char *format, ...);
void redirect_output_wifidebug();

#ifdef __cplusplus
}
#endif

#endif /* SOURCE_WIFI_DEBUG_H_ */
