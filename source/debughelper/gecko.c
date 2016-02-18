#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <sys/iosupport.h>

// #define DEBUG_TO_FILE

/* init-globals */
static bool geckoinit = false;

void printf_gecko(const char *format, ...)
{
	#ifndef DEBUG_TO_FILE
		#ifndef WIFI_GECKO
		if (!geckoinit)
			return;
		#endif
	#endif

	static char stringBuf[4096];
	int len;
	va_list va;
	va_start(va, format);
	if((len = vsnprintf(stringBuf, sizeof(stringBuf), format, va)) > 0)
	{
		#ifdef DEBUG_TO_FILE
		FILE *debugF = fopen("sd:/debug.txt", "a");
		if(!debugF)
			debugF = fopen("sd:/debug.txt", "w");
		if(debugF)
		{
			fwrite(stringBuf, 1, strlen(stringBuf), debugF);
			fclose(debugF);
		}
		#else
		usb_sendbuffer(1, stringBuf, len);
		#endif
	}
	va_end(va);
}

bool init_gecko()
{
	u32 geckoattached = usb_isgeckoalive(EXI_CHANNEL_1);
	if (geckoattached)
	{
		usb_flush(EXI_CHANNEL_1);
		geckoinit = true;
		return true;
	}

	return false;
}

static ssize_t __out_write(struct _reent *r, int fd, const char *ptr, size_t len)
{
	if(len > 0)
		usb_sendbuffer(1, ptr, len);

	return len;
}

static const devoptab_t gecko_out = {
	"stdout",	// device name
	0,			// size of file structure
	NULL,		// device open
	NULL,		// device close
	__out_write,// device write
	NULL,		// device read
	NULL,		// device seek
	NULL,		// device fstat
	NULL,		// device stat
	NULL,		// device link
	NULL,		// device unlink
	NULL,		// device chdir
	NULL,		// device rename
	NULL,		// device mkdir
	0,			// dirStateSize
	NULL,		// device diropen_r
	NULL,		// device dirreset_r
	NULL,		// device dirnext_r
	NULL,		// device dirclose_r
	NULL,		// device statvfs_r
	NULL,		// device ftruncate_r
	NULL,		// device fsync_r
	NULL,		// device deviceData
	NULL,		// device chmod_r
	NULL,		// device fchmod_r
};

void redirect_output_gecko()
{
	devoptab_list[STD_OUT] = &gecko_out;
	devoptab_list[STD_ERR] = &gecko_out;
}
