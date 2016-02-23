#include "wifidebug.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>

#include <network.h>
#include <ogc/lwp.h>
#include <ogc/consol.h>
#include <debug.h>
#include <sys/iosupport.h>

#define MAX_CLIENTS 10
#define SERVER_MESSAGE_SEPARATOR "%%&_"

static bool initialized = false;
static bool serverInitialized = false;
static s32 clientSockets[MAX_CLIENTS];
static lwp_t serverThreadHandler = 0;
static s32 listemSock = 0;
static u64 lastMessageId = -1;

static bool __sendto_socket(s32 sock, const char* ptr, size_t len){
	char message[5000];
	snprintf(message, sizeof(message),
		"INIT"SERVER_MESSAGE_SEPARATOR
		"%llu"SERVER_MESSAGE_SEPARATOR
		"%.*s"SERVER_MESSAGE_SEPARATOR
		"END"SERVER_MESSAGE_SEPARATOR, ++lastMessageId, len, ptr);
	size_t messageLen = strlen(message);
	s32 sended = 0;

	while((u32)sended < messageLen) {
		s32 ret = net_send(sock, message + sended, messageLen - sended, 0);
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

static ssize_t __write_to_all_clients(struct _reent *r, int fd, const char *ptr, size_t len)
{
	int i;
	if(serverInitialized && len > 0)
		for(i =0; i < MAX_CLIENTS;i++)
			if(clientSockets[i] > 0)
				if(!__sendto_socket(clientSockets[i], ptr, len)){
					net_close(clientSockets[i]);
					clientSockets[i] = 0;
					break;
				}
	return len;
}

static void * __remote_print_server(void* args){
	listemSock = net_socket (AF_INET, SOCK_STREAM, IPPROTO_IP);
	struct sockaddr_in clientAddr;
	struct sockaddr_in serverAddr;

	memset(clientSockets, 0, MAX_CLIENTS);

	if (listemSock != INVALID_SOCKET) {
		memset (&serverAddr, 0, sizeof (serverAddr));
		memset (&clientAddr, 0, sizeof (clientAddr));

		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons (1808);
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		s32 ret = net_bind(listemSock, (struct sockaddr *) &serverAddr, sizeof (serverAddr));

		if(ret == 0){
			ret = net_listen(listemSock, 5);
			if(ret == 0){
				serverInitialized = true;
				while(serverInitialized){
					socklen_t addrClientlen = sizeof(clientAddr);;
					s32 clientSocket = net_accept(listemSock, (struct sockaddr *) &clientAddr, &addrClientlen);
					if(clientSocket <= 0)
						continue;
					if(__sendto_socket(clientSocket, "Connected", strlen("Connected")))
					{
						int i;
						for(i = 0;i < MAX_CLIENTS;i++){
							if(clientSockets[i] == 0){
								clientSockets[i] = clientSocket;
								break;
							}
						}
						if(i == MAX_CLIENTS)
							net_close(clientSocket);
					} else
						net_close(clientSocket);
				}
			}
		}
	}
	return NULL;
}

void printf_wifidebug(const char *format, ...) {
	if(!serverInitialized)
		return;

	static char stringBuf[4096];
	int len;
	va_list va;
	va_start(va, format);

	if((len = vsnprintf(stringBuf, sizeof(stringBuf), format, va)) > 0)
		__write_to_all_clients(NULL, 0, stringBuf, len);
}

bool init_wifidebug(bool waitgdb) {
	if(initialized)
		return true;
	char ip[50];
	initialized = if_config(ip, NULL, NULL, true) >= 0;
	if(initialized) {
		DEBUG_Init(100, 5656);
		LWP_CreateThread(&serverThreadHandler,
				__remote_print_server, NULL, NULL, 16*1024, 50);
		if(waitgdb)
			_break();
	}
	return initialized;
}

void debughelper_deinit_wifidebug(){
	printf_wifidebug("Stopping debug over wifi");
	initialized = false;
	serverInitialized = false;
	net_close(listemSock);
}

int __console_write_(struct _reent *r,int fd,const char *ptr,size_t len){
	return __write_to_all_clients(r, fd, ptr, len);
}

static const devoptab_t wifi_out = {
		"stdout",	// device name
		0,			// size of file structure
		NULL,		// device open
		NULL,		// device close
		__console_write_,//__write_to_all_clients,// device write
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

	setvbuf(stdout, (char*)NULL, _IONBF, 0);
	setvbuf(stderr, (char*)NULL, _IONBF, 0);
}
