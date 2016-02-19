
#ifndef _GECKO_H_
#define _GECKO_H_

#ifdef __cplusplus
extern "C"
{
#endif

void printf_gecko(const char *format, ...);
bool init_gecko();
void redirect_output_gecko();

#ifdef __cplusplus
}
#endif

#endif

