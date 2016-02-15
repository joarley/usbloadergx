#ifndef _USB_NEW_H_
#define _USB_NEW_H_

#include "ogc/disc_io.h"
#include <fat.h>
#include <ogc/usb.h>

#ifdef __cplusplus
extern "C"
{
#endif


extern const DISC_INTERFACE __io_usbstorage2_port0;
extern const DISC_INTERFACE __io_usbstorage2_port1;
extern const DISC_INTERFACE __io_usbstorage2_port2;
extern const DISC_INTERFACE __io_usbstorage2_port3;
extern const DISC_INTERFACE __io_usbstorage2_port4;
extern const DISC_INTERFACE __io_usbstorage2_port5;
extern const DISC_INTERFACE __io_usbstorage2_port6;
extern const DISC_INTERFACE __io_usbstorage2_port7;

bool usbstorage_init();
void usbstorage_deinit();
int usbstorage_get_num_devices();
bool usbstorage_startup(int port);
u32 usbstorage_getCapacity(int port);
bool usbstorage_isInserted(int port);
bool usbstorage_readsectors(int port, u32 sector, u32 numSectors, void *buffer);
bool usbstorage_writesectors(int port, u32 sector, u32 numSectors, const void *buffer);
bool usbstorage_shutdown(int port);
bool usbstorage_clear_status(int port);

#define DEVICE_TYPE_WII_UMS (('W'<<24)|('U'<<16)|('M'<<8)|'S')


#ifdef __cplusplus
}
#endif


#endif
