#ifndef _USB_NEW_H_
#define _USB_NEW_H_

#include "ogc/disc_io.h"
#include <fat.h>
#include <ogc/usb.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_USB_STORAGE_DEVICES 16

extern const DISC_INTERFACE __io_usbstorage2_port0;
extern const DISC_INTERFACE __io_usbstorage2_port1;
extern const DISC_INTERFACE __io_usbstorage2_port2;
extern const DISC_INTERFACE __io_usbstorage2_port3;
extern const DISC_INTERFACE __io_usbstorage2_port4;
extern const DISC_INTERFACE __io_usbstorage2_port5;
extern const DISC_INTERFACE __io_usbstorage2_port6;
extern const DISC_INTERFACE __io_usbstorage2_port7;
extern const DISC_INTERFACE __io_usbstorage2_port8;
extern const DISC_INTERFACE __io_usbstorage2_port9;
extern const DISC_INTERFACE __io_usbstorage2_port10;
extern const DISC_INTERFACE __io_usbstorage2_port11;
extern const DISC_INTERFACE __io_usbstorage2_port12;
extern const DISC_INTERFACE __io_usbstorage2_port13;
extern const DISC_INTERFACE __io_usbstorage2_port14;
extern const DISC_INTERFACE __io_usbstorage2_port15;

bool usbstorage_init();
void usbstorage_deinit();
u8 usbstorage_get_num_devices();
u32 usbstorage_get_sector_size(int port);
const DISC_INTERFACE * usbstorage_get_disc_interface(int port);
bool usbstorage_startup(int port);
u32 usbstorage_get_capacity(int port);
bool usbstorage_is_inserted(int port);
bool usbstorage_read_sectors(int port, u32 sector, u32 numSectors, void *buffer);
bool usbstorage_write_sectors(int port, u32 sector, u32 numSectors, const void *buffer);
bool usbstorage_shutdown(int port);
bool usbstorage_clear_status(int port);

#ifdef __cplusplus
}
#endif


#endif
