/****************************************************************************
 * Copyright (C) 2010
 * by Dimok
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you
 * must not claim that you wrote the original software. If you use
 * this software in a product, an acknowledgment in the product
 * documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and
 * must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 *
 * for WiiXplorer 2010
 ***************************************************************************/
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "usbloader/usb_new.h"
#include "sdcard/wiisd_io.h"
#include "DeviceHandler.hpp"
#include "../usbloader/wbfs.h"
#include <cstdlib>
#include <string.h>

DeviceHandler * DeviceHandler::instance = NULL;

DeviceHandler* DeviceHandler::Instance() {
	if(instance == NULL)
		instance = new DeviceHandler;
	return instance;
}

void DeviceHandler::DestroyInstance() {
	if(instance != NULL)
	{
		delete instance;
		instance = NULL;
	}
}

DeviceHandler::DeviceHandler() {
	usbstorage_init();
	sdHandle = NULL;
	memset(usbHandles, 0, sizeof(PartitionHandle *) * MAX_USB_STORAGE_DEVICES);
}

DeviceHandler::~DeviceHandler() {
	UnMountAll();
	usbstorage_deinit();
}

bool DeviceHandler::MountAll() {
	MountSD();
	MountAllUSB();
	return true;
}

void DeviceHandler::UnMountAll() {
	UnmountSD();
	UnmountAllUSB();
}

bool DeviceHandler::MountSD() {
	printf("Iniciando montagem sd");

	if(sdHandle != NULL){
		printf("sd já montado");
		return true;
	}
	printf("sd não montado");
	if(!__io_wiisd.startup())
	{
		printf("erro ao iniciar sd");
		return false;
	}
	printf("Iniciado sd");
	if(!__io_wiisd.isInserted()){
		printf("sd não inserido");
		return false;
	}
	printf("sd inserido");

	PartitionHandle* sd = new PartitionHandle(&__io_wiisd);
	if(sd->GetPartitionCount() < 1)
	{
		printf("sd sem partições");
		delete sd;
		return false;
	}
	printf("sd com %d partiçõe", sd->GetPartitionCount());
	sdHandle = sd;
	sdHandle->Mount(0, "sd");
	printf("sd montado com sucesso");
	return true;
}

int DeviceHandler::MountUSB(int port) {
	if(port > MAX_USB_STORAGE_DEVICES)
		return -1;
	if(usbHandles[port] != NULL)
		return -2;
	if(usbstorage_get_num_devices() <= port)
		return -3;
	const DISC_INTERFACE* interface = usbstorage_get_disc_interface(port);

	if(!interface->startup())
		return -4;
	if(!interface->isInserted())
		return -5;

	PartitionHandle* usb = new PartitionHandle(interface);
	int devPartCount = usb->GetPartitionCount();
	if(devPartCount < 1)
	{
		delete usb;
		return -6;
	}

	char name[5];
	int totalPartCount = GetTotalPartitionCount();
	for(int i = 0; i < devPartCount; i++) {
		sprintf(name, "%s%d","usb", totalPartCount + i);
		usb->Mount(i, name);
	}

	usbHandles[port] = usb;

	return 1;
}

int DeviceHandler::MountAllUSB() {
	int count = 0;
	for(int port = 0; port < MAX_USB_STORAGE_DEVICES;port++)
		if(MountUSB(port))
			count++;
	return count;
}

bool DeviceHandler::IsInsertedSD() {
	if(!__io_wiisd.startup())
		return false;
	return __io_wiisd.isInserted();
}

bool DeviceHandler::IsInsertedUSB(int port) {
	if(port > MAX_USB_STORAGE_DEVICES)
		return false;
	if(usbstorage_get_num_devices() < port)
		return false;
	const DISC_INTERFACE* interface = usbstorage_get_disc_interface(port);

	if(!interface->startup())
		return false;
	return interface->isInserted();
}

void DeviceHandler::UnmountSD() {
	if(sdHandle == NULL)
		return;
	delete sdHandle;
	sdHandle = NULL;
}

void DeviceHandler::UnmountAllUSB() {
	for(int port = 0; port < MAX_USB_STORAGE_DEVICES;port++)
		UnmountUSB(port);
}

void DeviceHandler::UnmountUSB(int port) {
	if(port > MAX_USB_STORAGE_DEVICES)
		return;
	if(usbHandles[port] == NULL)
		return;
	delete usbHandles[port];
	usbHandles[port] = NULL;
}

PartitionHandle* DeviceHandler::GetHandleSD() const {
	return sdHandle;
}

PartitionHandle* DeviceHandler::GetHandleUSB(int port) const {
	return usbHandles[port];
}

PartitionHandle* DeviceHandler::GetHandleFromPartition(int part) {
	if(part == SD_PARTITION_NUMBER)
		return sdHandle;
	int port = PartitionToPortUSB(part);
	if(port == -1)
		return NULL;
	return usbHandles[port];
}

const DISC_INTERFACE* DeviceHandler::GetInterfaceUSB(int port) {
	if(port > MAX_USB_STORAGE_DEVICES)
		return NULL;
	return usbstorage_get_disc_interface(port);
}

int DeviceHandler::GetFilesystemType(int part) {
	PartitionHandle* handle = GetHandleFromPartition(part);
	if(handle == NULL)
		return-1;

	const char* fsName = GetFSName(part);

	if(!fsName) return -1;

	if(strncmp(fsName, "WBFS", 4) == 0)
		return PART_FS_WBFS;
	else if(strncmp(fsName, "FAT", 3) == 0)
		return PART_FS_FAT;
	else if(strncmp(fsName, "NTFS", 4) == 0)
		return PART_FS_NTFS;
	else if(strncmp(fsName, "LINUX", 4) == 0)
		return PART_FS_EXT;

	return -1;
}

int DeviceHandler::GetPartitionNumber(const char* path) {
	if(strncmp(path, "sd", 2) == 0)
		return SD_PARTITION_NUMBER;

	if(strncmp(path, "usb", 3))
	{	if(isdigit(path[4]))
		return (path[3] - '0' * 10) + (path[4] - '0');
	else
		return path[3] - '0';
	}
	return -1;
}

const char* DeviceHandler::GetFSName(int part) {
	if(part == SD_PARTITION_NUMBER)
		return sdHandle->GetFSName(0);

	static char name[10];
	int portUSB = PartitionToPortUSB(part);
	if(portUSB >= 0)
		return "";

	sprintf(name, "%s%d","usb", part);
	int pos = usbHandles[portUSB]->GetPartitionPos(name);
	if(pos >= 0)
		return usbHandles[pos]->GetFSName(pos);

	return "";
}

const char* DeviceHandler::GetFSName(const char* path) {
	int part = GetPartitionNumber(path);
	if(part < 0)
		return "";
	return GetFSName(part);
}

const char* DeviceHandler::GetPartitionPrefix(const char* path) {
	static char ret[10] = "";
	if(strncmp(path, "sd", 2) == 0)
		return "sd";
	if(strncmp(path, "usb", 3))
	{
		if(isdigit(path[4]))
			strncpy(ret, path, 5);

		else
			strncpy(ret, path, 4);
	}
	return ret;
}

int DeviceHandler::PartitionToPortUSB(int part) {
	char name[10];
	sprintf(name, "%s%d","usb", part);

	for(int port = 0; port < MAX_USB_STORAGE_DEVICES; port++){
		if(usbHandles[port] == NULL)
			continue;
		int pos = usbHandles[port]->GetPartitionPos(name);
		if(pos >= 0)
			return port;
	}

	return -1;
}

u16 DeviceHandler::GetTotalPartitionCount() {
	int ret = 0;
	for(int port = 0; port < MAX_USB_STORAGE_DEVICES; port++){
		if(usbHandles[port] == NULL)
			continue;
		int parCount = usbHandles[port]->GetPartitionCount();
		ret += parCount;
	}
	return ret;
}

bool DeviceHandler::IsSDPartition(int part) {
	return part == SD_PARTITION_NUMBER;
}

const char* DeviceHandler::GetPartitionPrefix(int partition) {
	if(partition == SD_PARTITION_NUMBER)
		return "sd";
	static char name[10];
	sprintf(name, "%s%d","usb", partition);
	return name;
}
