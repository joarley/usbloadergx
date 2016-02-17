#include <network.h>
#include "wifi_debug.h"
#include <debug.h>

static bool initialized = false;

bool wifi_debug_init() {
	char ip[50];
	initialized = if_config(ip, NULL, NULL, true) >= 0;
	if(initialized)
		DEBUG_Init(100, 5656);
	return initialized;
}

void wifi_debug_redirect_output() {

}
