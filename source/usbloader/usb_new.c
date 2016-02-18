#include <ogc\usb.h>
#include <ogc\usbstorage.h>
#include "usb_new.h"
#include <string.h>

static bool devices_initialized[MAX_USB_STORAGE_DEVICES];
static usb_device_entry devices[MAX_USB_STORAGE_DEVICES];
static usbstorage_handle handles[MAX_USB_STORAGE_DEVICES];
static u32 hdd_sector_size[MAX_USB_STORAGE_DEVICES];
static u8 num_devices = 0;
static bool started = false;


bool usbstorage_init() {
	int i = 0;

	if(USB_Initialize() != USB_OK)
		return false;

	if(USBStorage_Initialize() != USB_OK)
		return false;

	if(USB_GetDeviceList(devices, MAX_USB_STORAGE_DEVICES, 0x08, &num_devices) != USB_OK)
		return false;

	memset(handles, 0, sizeof(usb_device_entry) * MAX_USB_STORAGE_DEVICES);
	memset(devices_initialized, 0, sizeof(bool) * MAX_USB_STORAGE_DEVICES);
	for(; i < MAX_USB_STORAGE_DEVICES;i++)
		hdd_sector_size[i] = 0;

	return true;
}

void usbstorage_deinit() {
	int i = 0;
	for(; i < MAX_USB_STORAGE_DEVICES;i++){
		if(devices_initialized[i])
			usbstorage_shutdown(i);
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

bool usbstorage_startup(int port) {
	if(!started){
		if(!usbstorage_init())
			return false;
		if(num_devices <= port)
			return false;
	}

	if(USBStorage_Open(&handles[port], devices[port].device_id, devices[port].vid, devices[port].pid) != USB_OK)
		return false;

	u32 sector_num = 0;
	USBStorage_ReadCapacity(&handles[port], 0, &hdd_sector_size[port], &sector_num);

	devices_initialized[port] = true;
	return true;
}

u32 usbstorage_get_capacity(int port) {
	if(!devices_initialized[port])
		return 0;

	u32 sector_num = 0;
	if(USBStorage_ReadCapacity(&handles[port], 0, &hdd_sector_size[port], &sector_num) != USB_OK)
		return 0;

	return hdd_sector_size[port] * sector_num;
}

bool usbstorage_is_inserted(int port) {
	if(!devices_initialized[port])
		return false;

	return usbstorage_get_capacity(port) > 0;
}

bool usbstorage_read_sectors(int port, u32 sector, u32 numSectors, void *buffer) {
	if(!devices_initialized[port])
		return false;

	if(USBStorage_Read(&handles[port], 0, sector, numSectors, buffer) != USB_OK)
		return false;
	return true;
}

bool usbstorage_write_sectors(int port, u32 sector, u32 numSectors, const void *buffer) {
	if(!devices_initialized[port])
		return false;

	if(USBStorage_Write(&handles[port], 0, sector, numSectors, buffer) != USB_OK)
		return false;
	return true;
}

bool usbstorage_shutdown(int port) {
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
	bool __usbstorage_Startup##PORT(){return usbstorage_startup(PORT);}\
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
