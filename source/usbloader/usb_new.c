#include <ogc\usb.h>
#include <ogc\usbstorage.h>
#include "usb_new.h"
#include <string.h>
#include "../debughelper/debughelper.h"
#include <stdio.h>
#include "../memory/mem2.h"

#define DEVICE_TYPE_WII_UMS (('W'<<24)|('U'<<16)|('M'<<8)|'S')


#ifdef USE_OGC_USB
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

bool usbstorage_startup(int port) {
	debughelper_printf("iniciando startup port %d", port);
	if(!started){
		if(!usbstorage_init())
			return false;
		if(num_devices <= port)
			return false;
	}

	if(devices_initialized[port])
		return true;

	s32 ret = USBStorage_Open(&handles[port], devices[port].device_id, devices[port].vid, devices[port].pid);
	if(ret != USB_OK)
	{
		debughelper_printf("error ao abrir porta %d", ret);
		return false;
	}

	u32 sector_num = 0;
	ret = USBStorage_ReadCapacity(&handles[port], 0, &hdd_sector_size[port], &sector_num);

	if(ret < 0){
		debughelper_printf("error ao verificar tamanho %d", ret);
	}

	devices_initialized[port] = true;
	return true;
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

#else

/* IOCTL commands */
#define UMS_BASE			(('U'<<24)|('M'<<16)|('S'<<8))
#define USB_IOCTL_UMS_INIT			  (UMS_BASE+0x1)
#define USB_IOCTL_UMS_GET_CAPACITY	  (UMS_BASE+0x2)
#define USB_IOCTL_UMS_READ_SECTORS	  (UMS_BASE+0x3)
#define USB_IOCTL_UMS_WRITE_SECTORS	 (UMS_BASE+0x4)
#define USB_IOCTL_UMS_READ_STRESS	   (UMS_BASE+0x5)
#define USB_IOCTL_UMS_SET_VERBOSE	   (UMS_BASE+0x6)
#define USB_IOCTL_UMS_UMOUNT			(UMS_BASE+0x10)
#define USB_IOCTL_UMS_WATCHDOG		  (UMS_BASE+0x80)

#define USB_IOCTL_UMS_TESTMODE		  (UMS_BASE+0x81)
#define USB_IOCTL_SET_PORT				(UMS_BASE+0x83)

#define WBFS_BASE (('W'<<24)|('F'<<16)|('S'<<8))
#define USB_IOCTL_WBFS_OPEN_DISC			(WBFS_BASE+0x1)
#define USB_IOCTL_WBFS_READ_DISC			(WBFS_BASE+0x2)
#define USB_IOCTL_WBFS_SET_DEVICE		   (WBFS_BASE+0x50)
#define USB_IOCTL_WBFS_SET_FRAGLIST		 (WBFS_BASE+0x51)

#define MAX_SECTOR_SIZE		 4096
#define MAX_BUFFER_SECTORS	  128
#define UMS_HEAPSIZE			2*1024

static char fs[] ATTRIBUTE_ALIGN(32) = "/dev/usb2";
static char fs2[] ATTRIBUTE_ALIGN(32) = "/dev/usb123";
static char fs3[] ATTRIBUTE_ALIGN(32) = "/dev/usb/ehc";

static s32 hid = -1;
static s32 fd = -1;
static bool started = false;
static u8 num_devices = 0;
static u32 hdd_sector_size[MAX_USB_STORAGE_DEVICES];
static bool devices_initialized[MAX_USB_STORAGE_DEVICES];
static u32 current_port = 500;
static u8 * mem2_ptr = NULL;

static bool usbstorage_setport(u32 port)
{
	if(port == current_port)
		return true;

	//! Port = 2 is handle in the loader, no need to handle it in cIOS
	if(port > MAX_USB_STORAGE_DEVICES)
		return false;

	if (fd >= 0) {
		s32 ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_SET_PORT, "i:", port);
		if(ret >= 0)
		{
			current_port = port;
			return true;
		}
	}
	return false;
}

bool usbstorage_init(){
	int i;

	if(started)
		return true;

	if (hid < 0)
	{
		hid = iosCreateHeap(UMS_HEAPSIZE);
		if (hid < 0) return IPC_ENOMEM;
	}

	/* Open USB device */
	if (fd < 0)
		fd = IOS_Open(fs, 0);
	if (fd < 0)
		fd = IOS_Open(fs2, 0);
	if (fd < 0)
		fd = IOS_Open(fs3, 0);
	if (fd < 0)
		return false;

	num_devices = 0;
	for(i = 0; i < MAX_USB_STORAGE_DEVICES; i++){
		if(usbstorage_setport(i)){
			u32 sector_size = 0;
			s32 ret = 0;

			ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_INIT, ":");
			if(ret < 0)
				continue;
			ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_GET_CAPACITY, ":i", &sector_size);
			if(ret < 0)
				continue;

			hdd_sector_size[i] = sector_size;
			num_devices++;
		}
	}

	memset(devices_initialized, 0, sizeof(bool) * MAX_USB_STORAGE_DEVICES);
	return true;
}

void usbstorage_deinit(){
	if (fd >= 0)
		IOS_Close(fd);  // not sure to close the fd is needed

	fd = -1;
	num_devices = 0;
	started = false;
	current_port = -1;
	int i = 0;

	for(i = 0; i < MAX_USB_STORAGE_DEVICES;i++)
		hdd_sector_size[i] = 0;
	memset(devices_initialized, 0, sizeof(bool) * MAX_USB_STORAGE_DEVICES);
}

u8 usbstorage_get_num_devices(){
	return num_devices;
}

u32 usbstorage_get_sector_size(int port){
	if(!devices_initialized[port])
		return 0;
	return hdd_sector_size[port];
}

bool usbstorage_startup(int port){
	if(!started) {
		if(!usbstorage_init())
			return false;
		if(num_devices <= port)
			return false;
	}

	if(devices_initialized[port])
		return true;

	if(!usbstorage_setport(port))
		return false;

	s32 ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_INIT, ":");
	if(ret < 0)
		return false;
	devices_initialized[port] = true;
	return true;
}

u32 usbstorage_get_capacity(int port){
	if(!devices_initialized[port])
		return 0;
	if(!usbstorage_setport(port))
		return 0;

	u32 sector_size = 0;
	s32 ret = 0;

	ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_GET_CAPACITY, ":i", &sector_size);
	if(ret <= 0)
		return 0;

	return ret * sector_size;
}

bool usbstorage_is_inserted(int port){
	return usbstorage_get_capacity(port) > 0;
}

bool usbstorage_read_sectors(int port, u32 sector, u32 numSectors, void *buffer){
	if(!devices_initialized[port])
		return false;
	if (!mem2_ptr)
		mem2_ptr = (u8 *) MEM2_alloc(MAX_SECTOR_SIZE * MAX_BUFFER_SECTORS);
	if(!usbstorage_setport(port))
		return false;

	s32 read_secs;
	s32 read_size;
	s32 ret =  0;
	u8 *buf = (u8 *) buffer;

	while(numSectors > 0)
	{
		read_secs = numSectors > MAX_BUFFER_SECTORS ? MAX_BUFFER_SECTORS : numSectors;
		read_size = read_secs*hdd_sector_size[port];

		// Do not read more than MAX_BUFFER_SECTORS sectors at once and create a mem overflow!
		if (!isMEM2Buffer(buffer))
		{
			ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_READ_SECTORS, "ii:d", sector, read_secs, mem2_ptr, read_size);
			if(ret < 0)
				return false;

			memcpy(buf, mem2_ptr, read_size);
		}
		else
		{
			/* Read data */
			ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_READ_SECTORS, "ii:d", sector, read_secs, buf, read_size);
			if(ret < 0)
				return false;
		}

		sector += read_secs;
		numSectors -= read_secs;
		buf += read_size;
	}

	return true;
}

bool usbstorage_write_sectors(int port, u32 sector, u32 numSectors, const void *buffer){
	if(!devices_initialized[port])
		return false;

	if(!devices_initialized[port])
		return false;
	if (!mem2_ptr)
		mem2_ptr = (u8 *) MEM2_alloc(MAX_SECTOR_SIZE * MAX_BUFFER_SECTORS);
	if(!usbstorage_setport(port))
		return false;

	s32 write_size;
	s32 write_secs;
	s32 ret = 0;
	u8 *buf = (u8 *) buffer;

	while(numSectors > 0)
	{
		write_secs = numSectors > MAX_BUFFER_SECTORS ? MAX_BUFFER_SECTORS : numSectors;
		write_size = write_secs*hdd_sector_size[port];

		/* MEM1 buffer */
		if (!isMEM2Buffer(buffer))
		{
			// Do not read more than MAX_BUFFER_SECTORS sectors at once and create a mem overflow!
			memcpy(mem2_ptr, buf, write_size);

			ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_WRITE_SECTORS, "ii:d", sector, write_secs, mem2_ptr, write_size);
			if(ret < 0)
				return ret;
		}
		else
		{
			/* Write data */
			ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_WRITE_SECTORS, "ii:d", sector, write_secs, buf, write_size);
			if(ret < 0)
				return ret;
		}

		sector += write_secs;
		numSectors -= write_secs;
		buf += write_size;
	}

	return true;
}

bool usbstorage_shutdown(int port){
	if(devices_initialized[port]){
		devices_initialized[port] = false;
	}

	return true;
}

bool usbstorage_clear_status(int port){
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

#endif

