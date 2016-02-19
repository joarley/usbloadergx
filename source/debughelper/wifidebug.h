#ifndef SOURCE_WIFI_DEBUG_H_
#define SOURCE_WIFI_DEBUG_H_

#ifdef __cplusplus
extern "C"
{
#endif

void printf_wifidebug(const char *format, ...);
bool init_wifidebug(bool waitgdb);
void redirect_output_wifidebug();

#ifdef __cplusplus
}
#endif

#endif /* SOURCE_WIFI_DEBUG_H_ */
