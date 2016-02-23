#include <ogc\usb.h>
#include <ogc\usbstorage.h>
#include "usb_new.h"
#include <string.h>
#include "../debughelper/debughelper.h"
#include <stdio.h>

static bool devices_initialized[MAX_USB_STORAGE_DEVICES];
static usb_device_entry devices[MAX_USB_STORAGE_DEVICES];
static usbstorage_handle handles[MAX_USB_STORAGE_DEVICES];
static u32 hdd_sector_size[MAX_USB_STORAGE_DEVICES];
static u8 num_devices = 0;
static bool started = false;


bool usbstorage_init() {
	printf("Iniciando usb storage");
	int i = 0;

	if(USB_Initialize() != USB_OK){
		printf("Erro ao iniciar usb");
		return false;
	}
	printf("usb iniciado");

	if(USBStorage_Initialize() != USB_OK){
		printf("Erro ao iniciar usb storage");
		return false;
	}
	printf("usb storage iniciado");

	if(USB_GetDeviceList(devices, MAX_USB_STORAGE_DEVICES, 0x08, &num_devices) != USB_OK){
		printf("Erro ao buscar lista de devices, %d devices encontrados", num_devices);
		return false;
	}
	printf("%d devices encontrados", num_devices);

	memset(handles, 0, sizeof(usbstorage_handle) * MAX_USB_STORAGE_DEVICES);
	memset(devices_initialized, 0, sizeof(bool) * MAX_USB_STORAGE_DEVICES);
	for(; i < MAX_USB_STORAGE_DEVICES;i++)
		hdd_sector_size[i] = 0;

	printf("USB storage iniciado");
	started = true;
	return true;
}

void usbstorage_deinit() {
	printf("Iniciando usb deinit");
	int i = 0;
	for(; i < MAX_USB_STORAGE_DEVICES;i++){
		printf("porta %d iniciado = %d", i, devices_initialized[i]);
		if(devices_initialized[i]){
			usbstorage_shutdown(i);
		}
		hdd_sector_size[i] = 0;
	}

	memset(devices, 0, sizeof(usb_device_entry) * MAX_USB_STORAGE_DEVICES);
	memset(handles, 0, sizeof(usbstorage_handle) * MAX_USB_STORAGE_DEVICES);
	memset(devices_initialized, 0, sizeof(bool) * MAX_USB_STORAGE_DEVICES);
	started = false;
	num_devices = 0;

	USB_Deinitialize();
}

u8 usbstorage_get_num_devices() {
	return num_devices;
}

u32 usbstorage_get_sector_size(int port){
	if(!devices_initialized[port])
		return 0;
	return hdd_sector_size[port];
}

int usbstorage_startup(int port) {
	debughelper_printf("iniciando startup port %d", port);
	if(!started){
		if(!usbstorage_init())
			return -1;
		if(num_devices <= port)
			return -2;
	}

	if(devices_initialized[port])
		return 1;

	s32 ret = USBStorage_Open(&handles[port], devices[port].device_id, devices[port].vid, devices[port].pid);
	if(ret != USB_OK)
	{
		debughelper_printf("error ao abrir porta %d", ret);
		return -3;
	}

	u32 sector_num = 0;
	ret = USBStorage_ReadCapacity(&handles[port], 0, &hdd_sector_size[port], &sector_num);

	if(ret < 0){
		debughelper_printf("error ao verificar tamanho %d", ret);
	}

	devices_initialized[port] = true;
	return 1;
}

u32 usbstorage_get_capacity(int port) {
	debughelper_printf("iniciando get_capacity %d", port);
	if(!devices_initialized[port])
		return 0;

	u32 sector_num = 0;
	s32 ret = USBStorage_ReadCapacity(&handles[port], 0, &hdd_sector_size[port], &sector_num);
	debughelper_printf(
			"USBStorage_ReadCapacity retornou %d e sector size é %d, sector num é %d",
			ret, hdd_sector_size[port], sector_num);
	if(ret != USB_OK)
		return 0;

	return hdd_sector_size[port] * sector_num;
}

bool usbstorage_is_inserted(int port) {
	debughelper_printf("iniciando is inserted %d", port);
	if(!devices_initialized[port])
		return false;

	return usbstorage_get_capacity(port) > 0;
}

bool usbstorage_read_sectors(int port, u32 sector, u32 numSectors, void *buffer) {
	debughelper_printf("Iniciando leitura porta(%d) %d sector %d numSectos %d",
		devices_initialized[port], port, sector, numSectors);
	if(!devices_initialized[port])
		return false;

	s32 readResult = USBStorage_MountLUN(&handles[port], 0);
	debughelper_printf("resultado mountlun %d", readResult);
	readResult = USBStorage_Read(&handles[port], 0, sector, numSectors, buffer);
	debughelper_printf("resultado leitura %d", readResult);

	if(readResult != USB_OK)
		return false;
	return true;
}

bool usbstorage_write_sectors(int port, u32 sector, u32 numSectors, const void *buffer) {
	debughelper_printf("iniciando write sectors porta %d", port);
	if(!devices_initialized[port])
		return false;

	if(USBStorage_Write(&handles[port], 0, sector, numSectors, buffer) != USB_OK)
		return false;
	return true;
}

bool usbstorage_shutdown(int port) {
	debughelper_printf("iniciando shutdown porta %d", port);
	if(devices_initialized[port])
		USBStorage_Close(&handles[port]);

	devices_initialized[port] = false;
	return true;
}

bool usbstorage_clear_status(int port)
{
	return true;
}

#define DECLARE_PORT(PORT) \
	bool __usbstorage_Startup##PORT(){return usbstorage_startup(PORT) == 1;}\
	bool __usbstorage_IsInserted##PORT(){return usbstorage_is_inserted(PORT);}\
	bool __usbstorage_ReadSectors##PORT(u32 sector, u32 numSectors, void *buffer){\
		return usbstorage_read_sectors(PORT, sector, numSectors, buffer);}\
	bool __usbstorage_WriteSectors##PORT(u32 sector, u32 numSectors, const void *buffer){\
		return usbstorage_write_sectors(PORT, sector, numSectors, buffer);}\
	bool __usbstorage_ClearStatus##PORT(){return usbstorage_clear_status(PORT);}\
		bool __usbstorage_Shutdown##PORT(){return usbstorage_shutdown(PORT);}\
	const DISC_INTERFACE __io_usbstorage2_port##PORT = {\
		DEVICE_TYPE_WII_UMS, FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE | FEATURE_WII_USB,\
		(FN_MEDIUM_STARTUP) &__usbstorage_Startup##PORT,\
		(FN_MEDIUM_ISINSERTED) &__usbstorage_IsInserted##PORT,\
		(FN_MEDIUM_READSECTORS) &__usbstorage_ReadSectors##PORT,\
		(FN_MEDIUM_WRITESECTORS) &__usbstorage_WriteSectors##PORT,\
		(FN_MEDIUM_CLEARSTATUS) &__usbstorage_ClearStatus##PORT,\
		(FN_MEDIUM_SHUTDOWN) &__usbstorage_Shutdown##PORT}

DECLARE_PORT(0);
DECLARE_PORT(1);
DECLARE_PORT(2);
DECLARE_PORT(3);
DECLARE_PORT(4);
DECLARE_PORT(5);
DECLARE_PORT(6);
DECLARE_PORT(7);
DECLARE_PORT(8);
DECLARE_PORT(9);
DECLARE_PORT(10);
DECLARE_PORT(11);
DECLARE_PORT(12);
DECLARE_PORT(13);
DECLARE_PORT(14);
DECLARE_PORT(15);

static const DISC_INTERFACE* interfaces[MAX_USB_STORAGE_DEVICES] = {&__io_usbstorage2_port0, &__io_usbstorage2_port1,
	&__io_usbstorage2_port2, &__io_usbstorage2_port3, &__io_usbstorage2_port4, &__io_usbstorage2_port5,
	&__io_usbstorage2_port6, &__io_usbstorage2_port7, &__io_usbstorage2_port8, &__io_usbstorage2_port9,
	&__io_usbstorage2_port10, &__io_usbstorage2_port11, &__io_usbstorage2_port12, &__io_usbstorage2_port13,
	&__io_usbstorage2_port14, &__io_usbstorage2_port15};

const DISC_INTERFACE* usbstorage_get_disc_interface(int port) {
	return interfaces[port];
}


