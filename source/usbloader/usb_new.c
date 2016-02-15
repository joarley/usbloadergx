#include <ogc\usb.h>
#include <ogc\usbstorage.h>
#include "usb_new.h"
#include <string.h>

bool devices_initialized[30];
usb_device_entry devices[30];
usbstorage_handle handles[30];
u32 hdd_sector_size[30];
u8 num_devices = 0;
bool started = false;


bool usbstorage_init() {
  int i = 0;

  if(USB_Initialize() != USB_OK)
    return false;

  if(USBStorage_Initialize() != USB_OK)
    return false;

  if(USB_GetDeviceList(devices,30, 0x08, &num_devices) != USB_OK)
    return false;

  memset(handles, 0, sizeof(usb_device_entry) * 30);
  memset(devices_initialized, 0, sizeof(bool) * 30);
  for(; i < 30;i++)
    hdd_sector_size[i] = 0;

  return true;
}

void usbstorage_deinit() {
  int i = 0;
  USB_Deinitialize();

  memset(devices, 0, sizeof(usb_device_entry) * 30);
  memset(handles, 0, sizeof(usbstorage_handle) * 30);
  memset(devices_initialized, 0, sizeof(bool) * 30);
  for(; i < 30;i++)
    hdd_sector_size[i] = 0;
  started = false;
  num_devices = 0;
}

int usbstorage_get_num_devices() {
  return num_devices;
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

u32 usbstorage_getCapacity(int port) {
  if(!devices_initialized[port])
    return 0;

  u32 sector_num = 0;
  if(USBStorage_ReadCapacity(&handles[port], 0, &hdd_sector_size[port], &sector_num) != USB_OK)
    return 0;

  return hdd_sector_size[port] * sector_num;
}

bool usbstorage_isInserted(int port) {
	if(!devices_initialized[port])
    return false;

  return usbstorage_getCapacity(port) > 0;
}

bool usbstorage_readsectors(int port, u32 sector, u32 numSectors, void *buffer) {
	if(!devices_initialized[port])
    return false;

  if(USBStorage_Read(&handles[port], 0, sector, numSectors, buffer) != USB_OK)
    return false;
  return true;
}

bool usbstorage_writesectors(int port, u32 sector, u32 numSectors, const void *buffer) {
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
bool __usbstorage_IsInserted##PORT(){return usbstorage_isInserted(PORT);}\
bool __usbstorage_ReadSectors##PORT(u32 sector, u32 numSectors, void *buffer){\
  return usbstorage_readsectors(PORT, sector, numSectors, buffer);}\
bool __usbstorage_WriteSectors##PORT(u32 sector, u32 numSectors, const void *buffer){\
  return usbstorage_writesectors(PORT, sector, numSectors, buffer);}\
bool __usbstorage_ClearStatus##PORT(){return usbstorage_clear_status(PORT);}\
bool __usbstorage_Shutdown##PORT(){return usbstorage_shutdown(PORT);}\
const DISC_INTERFACE __io_usbstorage2_port##PORT = {\
	DEVICE_TYPE_WII_UMS, FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE | FEATURE_WII_USB,\
	(FN_MEDIUM_STARTUP) &__usbstorage_Startup##PORT,\
	(FN_MEDIUM_ISINSERTED) &__usbstorage_IsInserted##PORT,\
	(FN_MEDIUM_READSECTORS) &__usbstorage_ReadSectors##PORT,\
	(FN_MEDIUM_WRITESECTORS) &__usbstorage_WriteSectors##PORT,\
	(FN_MEDIUM_CLEARSTATUS) &__usbstorage_ClearStatus##PORT,\
	(FN_MEDIUM_SHUTDOWN) &__usbstorage_Shutdown##PORT\
};

DECLARE_PORT(0);
DECLARE_PORT(1);
DECLARE_PORT(2);
DECLARE_PORT(3);
DECLARE_PORT(4);
DECLARE_PORT(5);
DECLARE_PORT(6);
DECLARE_PORT(7);
