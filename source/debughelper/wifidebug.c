#include <network.h>
#include <ogc/lwp.h>
#include <debug.h>
#include <sys/iosupport.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "wifidebug.h"

#define MAX_CLIENTS_WIFI 10

static bool initialized = false;
static bool server_initialized = false;
static s32 clients[MAX_CLIENTS_WIFI];
static lwp_t init_server_handler = 0;
s32 server_sock = 0;
static u64 lastMessageId = -1;

static bool __sendto(s32 sock, const char* ptr, len){
	char message[5000];
	snprintf(message, sizeof(message), "INIT%%&_%d%%&_%.*s%%&_END", ++lastMessageId, len, ptr);
	size_t messageLen = strlen(message);
	s32 sended = 0;

	while(sended < len) {
		s32 ret = net_send(sock, message, len, 0);
		if(ret < 0){
			return false;
		} else if(ret == 0){
			return true;
		} else {
			sended += ret;
		}
	}

	return true;
}

static ssize_t __out_write(struct _reent *r, int fd, const char *ptr, size_t len)
{
	int i;
	if(server_initialized && len > 0)
		for(i =0; i < MAX_CLIENTS_WIFI;i++)
			if(clients[i] > 0)
				if(!__sendto(clients[i], ptr, len)){
					net_close(clients[i]);
					clients[i] = 0;
					break;
				}
	return len;
}

static void * __init_server(void* args){
	server_sock = net_socket (AF_INET, SOCK_STREAM, IPPROTO_IP);
	struct sockaddr_in client;
	struct sockaddr_in server;

	memset(clients, 0, MAX_CLIENTS_WIFI);

	if (server_sock != INVALID_SOCKET) {
		memset (&server, 0, sizeof (server));
		memset (&client, 0, sizeof (client));

		server.sin_family = AF_INET;
		server.sin_port = htons (1808);
		server.sin_addr.s_addr = INADDR_ANY;
		s32 ret = net_bind(server_sock, (struct sockaddr *) &server, sizeof (server));

		if(ret == 0){
			ret = net_listen(server_sock, 5);
			if(ret == 0){
				server_initialized = true;
				while(server_initialized){
					socklen_t clientlen = sizeof(client);;
					s32 client_sock = net_accept (server_sock, (struct sockaddr *) &client, &clientlen);
					if(client_sock < 0)
						continue;
					if(__sendto(client_sock, "Connected", strlen("Connected")))
					{
						int i;
						for(i = 0;i < MAX_CLIENTS_WIFI;i++){
							if(clients[i] == 0){
								clients[i] = client_sock;
								continue;
							}
						}
					}

					net_close(client_sock);
				}
			}
		}
	}
	return NULL;
}

void printf_wifidebug(const char *format, ...) {
	if(!server_initialized)
		return;

	static char stringBuf[4096];
	int len;
	va_list va;
	va_start(va, format);

	if((len = vsnprintf(stringBuf, sizeof(stringBuf), format, va)) > 0)
		__out_write(NULL, 0, stringBuf, len);
}

bool init_wifidebug(bool waitgdb) {
	if(initialized)
		return true;
	char ip[50];
	initialized = if_config(ip, NULL, NULL, true) >= 0;
	if(initialized) {
		DEBUG_Init(100, 5656);
		LWP_CreateThread(&init_server_handler,
				__init_server, NULL, NULL, 16*1024, 50);
		if(waitgdb)
			_break();
	}
	return initialized;
}

void debughelper_deinit_wifidebug(){
	initialized = false;
	server_initialized = false;
	net_close(server_sock);
}

static const devoptab_t wifi_out = {
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

void redirect_output_wifidebug() {
	devoptab_list[STD_OUT] = &wifi_out;
	devoptab_list[STD_ERR] = &wifi_out;
}
