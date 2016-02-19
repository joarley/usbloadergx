#include <network.h>
#include <ogc/lwp.h>
#include "wifidebug.h"
#include <debug.h>

#define MAX_CLIENTS_WIFI 10

static bool initialized = false;
static bool server_initialized = false;
static s32 clients[MAX_CLIENTS_WIFI] = {0};
static lwp_t init_server_handler = 0;

static ssize_t __out_write(struct _reent *r, int fd, const char *ptr, size_t len)
{
	if(server_initialized && len > 0)
		for(int i =0; i < MAX_CLIENTS_WIFI;i++)
			if(clients[i] > 0) {
				if(net_send(clients[i], ptr, len, 0) < 0){
					net_close(clients[i]);
					clients[i] = 0;
				}
			}
	return len;
}

static void __init_server(void*){
	s32 sock = net_socket (AF_INET, SOCK_STREAM, IPPROTO_IP);
	struct sockaddr_in client;
	struct sockaddr_in server;

	if (sock != INVALID_SOCKET) {
		memset (&server, 0, sizeof (server));
		memset (&client, 0, sizeof (client));

		server.sin_family = AF_INET;
		server.sin_port = htons (1808);
		server.sin_addr.s_addr = INADDR_ANY;
		s32 ret = net_bind(sock, (struct sockaddr *) &server, sizeof (server));

		if(ret == 0){
			ret = net_listen( sock, 5);
			if(ret == 0){
				server_initialized = true;
				while(true){
					s32 clientlen;
					s32 client_sock = net_accept (sock, (struct sockaddr *) &client, &clientlen);
					for(int i = 0;i < MAX_CLIENTS_WIFI;i++){
						if(clients[i] == 0)
							clients[i] = client_sock;
					}
				}
			}
		}
	}
}

void printf_wifidebug(const char *format, ...) {
	if(!server_initialized)
		return;

	static char stringBuf[4096];
	int len;
	va_list va;
	va_start(va, format);

	if((len = vsnprintf(stringBuf, sizeof(stringBuf), format, va)) > 0)
		__out_write(NULL, NULL, stringBuf, len);
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
