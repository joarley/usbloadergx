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
#include <malloc.h>
#include <unistd.h>
#include <string.h>
#include <ogc/mutex.h>
#include <ogc/system.h>
#include <sdcard/wiisd_io.h>
#include <sdcard/gcsd.h>
#include "settings/CSettings.h"
#include "usbloader/usb_new.h"
#include "DeviceHandler.hpp"
#include "usbloader/wbfs.h"
#include "system/IosLoader.h"

DeviceHandler * DeviceHandler::instance = NULL;

DeviceHandler::~DeviceHandler()
{
	UnMountAll();
}

DeviceHandler * DeviceHandler::Instance()
{
	if (instance == NULL)
	{
		instance = new DeviceHandler();
	}
	return instance;
}

void DeviceHandler::DestroyInstance()
{
	if(instance)
	{
		delete instance;
	}
	instance = NULL;
}

bool DeviceHandler::MountAll()
{
	bool result = false;

	for(u32 i = SD; i < MAXDEVICES; i++)
	{
		if(Mount(i))
			result = true;
	}

	return result;
}

void DeviceHandler::UnMountAll()
{
	for(u32 i = SD; i < MAXDEVICES; i++)
		UnMount(i);

	if(sd)
		delete sd;
	if(usb0)
		delete usb0;
	if(usb1)
		delete usb1;
	if(usb2)
		delete usb2;
	if(usb3)
		delete usb3;
	if(usb4)
		delete usb4;
	if(usb5)
		delete usb5;
	if(usb6)
		delete usb6;
	if(usb7)
		delete usb7;

	sd = NULL;
	usb0 = NULL;
	usb1 = NULL;
	usb2 = NULL;
	usb3 = NULL;
	usb4 = NULL;
	usb5 = NULL;
	usb6 = NULL;
	usb7 = NULL;
}

bool DeviceHandler::Mount(int dev)
{
	if(dev == SD)
		return MountSD();

	else if(dev >= USB1 && dev <= USB8)
		return MountUSB(dev-USB1);

	return false;
}

bool DeviceHandler::IsInserted(int dev)
{
	if(dev == SD)
		return SD_Inserted() && sd->IsMounted(0);

	else if(dev >= USB1 && dev <= USB8)
	{
		int portPart = PartitionToPortPartition(dev-USB1);
		PartitionHandle *usb = instance->GetUSBHandleFromPartition(dev-USB1);
		if(usb)
			return usb->IsMounted(portPart);
	}

	return false;
}

void DeviceHandler::UnMount(int dev)
{
	if(dev == SD)
		UnMountSD();

	else if(dev >= USB1 && dev <= USB8)
		UnMountUSB(dev-USB1);
}

bool DeviceHandler::MountSD()
{
	if(!sd)
		sd = new PartitionHandle(&__io_wiisd);

	if(sd->GetPartitionCount() < 1)
	{
		delete sd;
		sd = NULL;
		return false;
	}

	//! Mount only one SD Partition
	return sd->Mount(0, DeviceName[SD], true);
}

static inline bool USBSpinUp()
{
	bool started0 = false;
	bool started1 = false;
	bool started2 = false;
	bool started3 = false;
	bool started4 = false;
	bool started5 = false;
	bool started6 = false;
	bool started7 = false;
	int retries = 400;

	const DISC_INTERFACE * handle0 = NULL;
	const DISC_INTERFACE * handle1 = NULL;
	const DISC_INTERFACE * handle2 = NULL;
	const DISC_INTERFACE * handle3 = NULL;
	const DISC_INTERFACE * handle4 = NULL;
	const DISC_INTERFACE * handle5 = NULL;
	const DISC_INTERFACE * handle6 = NULL;
	const DISC_INTERFACE * handle7 = NULL;
	handle0 = DeviceHandler::GetUSB0Interface();
	handle1 = DeviceHandler::GetUSB1Interface();
	handle2 = DeviceHandler::GetUSB2Interface();
	handle3 = DeviceHandler::GetUSB3Interface();
	handle4 = DeviceHandler::GetUSB4Interface();
	handle5 = DeviceHandler::GetUSB5Interface();
	handle6 = DeviceHandler::GetUSB6Interface();
	handle7 = DeviceHandler::GetUSB7Interface();


	// wait 20 sec for the USB to spin up...stupid slow ass HDD
	do
	{
		if(handle0)
			started0 = (handle0->startup() && handle0->isInserted());

		if(handle1)
			started1 = (handle1->startup() && handle1->isInserted());

		if(handle2)
			started2 = (handle2->startup() && handle2->isInserted());

		if(handle3)
			started3 = (handle3->startup() && handle3->isInserted());

		if(handle4)
			started4 = (handle4->startup() && handle4->isInserted());

		if(handle5)
			started5 = (handle5->startup() && handle5->isInserted());

		if(handle6)
			started6 = (handle6->startup() && handle6->isInserted());

		if(handle7)
			started7 = (handle7->startup() && handle7->isInserted());

		if(   (!handle0 || started0)
		   && (!handle1 || started1)
		 	 && (!handle2 || started2)
		 	 && (!handle3 || started3)
		 	 && (!handle4 || started4)
		 	 && (!handle5 || started5)
		 	 && (!handle6 || started6)
		 	 && (!handle7 || started7)) {
			break;
		}
		usleep(50000);
	}
	while(--retries > 0);

	return (started0 || started1 || started2 || started3 || started4 || started5
		|| started6 || started7);
}

bool DeviceHandler::MountUSB(int pos)
{
	if(!usb0 && !usb1 && !usb2  && !usb3
		 && !usb4  && !usb5  && !usb6  && !usb7)
		return false;

	if(pos >= GetUSBPartitionCount())
		return false;

	int portPart = PartitionToPortPartition(pos);

	if(PartitionToUSBPort(pos) == 0 && usb0)
		return usb0->Mount(portPart, DeviceName[USB1+pos]);
	else if(PartitionToUSBPort(pos) == 1 && usb1)
		return usb1->Mount(portPart, DeviceName[USB1+pos]);
	else  if(PartitionToUSBPort(pos) == 2 && usb2)
		return usb2->Mount(portPart, DeviceName[USB1+pos]);
	else  if(PartitionToUSBPort(pos) == 3 && usb3)
		return usb3->Mount(portPart, DeviceName[USB1+pos]);
	else  if(PartitionToUSBPort(pos) == 4 && usb4)
		return usb4->Mount(portPart, DeviceName[USB1+pos]);
	else  if(PartitionToUSBPort(pos) == 5 && usb5)
		return usb5->Mount(portPart, DeviceName[USB1+pos]);
	else  if(PartitionToUSBPort(pos) == 6 && usb6)
		return usb6->Mount(portPart, DeviceName[USB1+pos]);
	else  if(PartitionToUSBPort(pos) == 7 && usb7)
		return usb7->Mount(portPart, DeviceName[USB1+pos]);

	return false;
}

bool DeviceHandler::MountAllUSB(bool spinup)
{
	if(spinup && !USBSpinUp())
		return false;

	if(!usb0)
		usb0 = new PartitionHandle(GetUSB0Interface());
	if(!usb1 && IOS_GetVersion() >= 200)
		usb1 = new PartitionHandle(GetUSB1Interface());
	if(!usb2 && IOS_GetVersion() >= 200)
		usb2 = new PartitionHandle(GetUSB2Interface());
	if(!usb3 && IOS_GetVersion() >= 200)
		usb3 = new PartitionHandle(GetUSB3Interface());
	if(!usb4 && IOS_GetVersion() >= 200)
		usb4 = new PartitionHandle(GetUSB4Interface());
	if(!usb5 && IOS_GetVersion() >= 200)
		usb5 = new PartitionHandle(GetUSB5Interface());
	if(!usb6 && IOS_GetVersion() >= 200)
		usb6 = new PartitionHandle(GetUSB6Interface());
	if(!usb7 && IOS_GetVersion() >= 200)
		usb7 = new PartitionHandle(GetUSB7Interface());


	if(usb0 && usb0->GetPartitionCount() < 1)
	{
		delete usb0;
		usb0 = NULL;
	}
	if(usb1 && usb1->GetPartitionCount() < 1)
	{
		delete usb1;
		usb1 = NULL;
	}
	if(usb2 && usb2->GetPartitionCount() < 1)
	{
		delete usb2;
		usb2 = NULL;
	}
	if(usb3 && usb3->GetPartitionCount() < 1)
	{
		delete usb3;
		usb3 = NULL;
	}
	if(usb4 && usb4->GetPartitionCount() < 1)
	{
		delete usb4;
		usb4 = NULL;
	}
	if(usb5 && usb5->GetPartitionCount() < 1)
	{
		delete usb5;
		usb5 = NULL;
	}
	if(usb6 && usb6->GetPartitionCount() < 1)
	{
		delete usb6;
		usb6 = NULL;
	}
	if(usb7 && usb7->GetPartitionCount() < 1)
	{
		delete usb1;
		usb6 = NULL;
	}

	bool result = false;
	int partCount = GetUSBPartitionCount();

	for(int i = 0; i < partCount; i++)
	{
		if(MountUSB(i))
			result = true;
	}

	return result;
}

void DeviceHandler::UnMountUSB(int pos)
{
	if(pos >= GetUSBPartitionCount())
		return;

	int portPart = PartitionToPortPartition(pos);

	if(PartitionToUSBPort(pos) == 0 && usb0)
		return usb0->UnMount(portPart);
	if(PartitionToUSBPort(pos) == 1 && usb1)
		return usb1->UnMount(portPart);
	if(PartitionToUSBPort(pos) == 2 && usb2)
		return usb2->UnMount(portPart);
	if(PartitionToUSBPort(pos) == 3 && usb3)
		return usb3->UnMount(portPart);
	if(PartitionToUSBPort(pos) == 4 && usb4)
		return usb4->UnMount(portPart);
	if(PartitionToUSBPort(pos) == 5 && usb5)
		return usb5->UnMount(portPart);
	if(PartitionToUSBPort(pos) == 6 && usb6)
		return usb6->UnMount(portPart);
	if(PartitionToUSBPort(pos) == 7 && usb7)
		return usb7->UnMount(portPart);
}

void DeviceHandler::UnMountAllUSB()
{
	int partCount = GetUSBPartitionCount();

	for(int i = 0; i < partCount; i++)
		UnMountUSB(i);

	delete usb0;
	usb0 = NULL;
	delete usb1;
	usb1 = NULL;
	delete usb2;
	usb2 = NULL;
	delete usb3;
	usb3 = NULL;
	delete usb4;
	usb4 = NULL;
	delete usb5;
	usb5 = NULL;
	delete usb6;
	usb6 = NULL;
	delete usb7;
	usb7 = NULL;
}

int DeviceHandler::PathToDriveType(const char * path)
{
	if(!path)
		return -1;

	for(int i = SD; i < MAXDEVICES; i++)
	{
		if(strncasecmp(path, DeviceName[i], strlen(DeviceName[i])) == 0)
			return i;
	}

	return -1;
}

const char * DeviceHandler::GetFSName(int dev)
{
	if(dev == SD && DeviceHandler::instance->sd)
	{
		return DeviceHandler::instance->sd->GetFSName(0);
	}
	else
	{
		int partCount0 = 0;
		int partCount1 = 0;
		int partCount2 = 0;
		int partCount3 = 0;
		int partCount4 = 0;
		int partCount5 = 0;
		int partCount6 = 0;
		int partCount7 = 0;

		if(DeviceHandler::instance->usb0)
			partCount0 += DeviceHandler::instance->usb0->GetPartitionCount();
		if(DeviceHandler::instance->usb1)
			partCount1 += DeviceHandler::instance->usb1->GetPartitionCount();
		if(DeviceHandler::instance->usb2)
			partCount2 += DeviceHandler::instance->usb2->GetPartitionCount();
		if(DeviceHandler::instance->usb3)
			partCount3 += DeviceHandler::instance->usb3->GetPartitionCount();
		if(DeviceHandler::instance->usb4)
			partCount4 += DeviceHandler::instance->usb4->GetPartitionCount();
		if(DeviceHandler::instance->usb5)
			partCount5 += DeviceHandler::instance->usb5->GetPartitionCount();
		if(DeviceHandler::instance->usb6)
			partCount6 += DeviceHandler::instance->usb6->GetPartitionCount();
		if(DeviceHandler::instance->usb7)
			partCount7 += DeviceHandler::instance->usb7->GetPartitionCount();


    dev -= partCount0;
		if(dev <= 0){
			return DeviceHandler::instance->usb0->GetFSName(dev-USB1);
		}
		dev -= partCount1;
		if(dev <= 0)
			return DeviceHandler::instance->usb0->GetFSName(dev-USB1);

		dev -= partCount2;
		if(dev <= 0)
			return DeviceHandler::instance->usb0->GetFSName(dev-USB2);

		dev -= partCount3;
		if(dev <= 0)
			return DeviceHandler::instance->usb0->GetFSName(dev-USB3);

		dev -= partCount4;
		if(dev <= 0)
			return DeviceHandler::instance->usb0->GetFSName(dev-USB4);

		dev -= partCount5;
		if(dev <= 0)
			return DeviceHandler::instance->usb0->GetFSName(dev-USB5);

		dev -= partCount6;
		if(dev <= 0)
			return DeviceHandler::instance->usb0->GetFSName(dev-USB6);

		dev -= partCount7;
		if(dev <= 0)
			return DeviceHandler::instance->usb0->GetFSName(dev-USB7);
	}

	return "";
}

const char * DeviceHandler::GetDevicePrefix(const char * path)
{
	if(PathToDriveType(path) == -1)
		return "";
	return DeviceName[PathToDriveType(path)];
}

int DeviceHandler::GetFilesystemType(int dev)
{
	if(!instance)
		return -1;

	const char *FSName = GetFSName(dev);
	if(!FSName) return -1;

	if(strncmp(FSName, "WBFS", 4) == 0)
		return PART_FS_WBFS;
	else if(strncmp(FSName, "FAT", 3) == 0)
		return PART_FS_FAT;
	else if(strncmp(FSName, "NTFS", 4) == 0)
		return PART_FS_NTFS;
	else if(strncmp(FSName, "LINUX", 4) == 0)
		return PART_FS_EXT;

	return -1;
}


u16 DeviceHandler::GetUSBPartitionCount()
{
	if(!instance)
		return 0;

		int partCount0 = 0;
		int partCount1 = 0;
		int partCount2 = 0;
		int partCount3 = 0;
		int partCount4 = 0;
		int partCount5 = 0;
		int partCount6 = 0;
		int partCount7 = 0;

		if(DeviceHandler::instance->usb0)
			partCount0 += DeviceHandler::instance->usb0->GetPartitionCount();
		if(DeviceHandler::instance->usb1)
			partCount1 += DeviceHandler::instance->usb1->GetPartitionCount();
		if(DeviceHandler::instance->usb2)
			partCount2 += DeviceHandler::instance->usb2->GetPartitionCount();
		if(DeviceHandler::instance->usb3)
			partCount3 += DeviceHandler::instance->usb3->GetPartitionCount();
		if(DeviceHandler::instance->usb4)
			partCount4 += DeviceHandler::instance->usb4->GetPartitionCount();
		if(DeviceHandler::instance->usb5)
			partCount5 += DeviceHandler::instance->usb5->GetPartitionCount();
		if(DeviceHandler::instance->usb6)
			partCount6 += DeviceHandler::instance->usb6->GetPartitionCount();
		if(DeviceHandler::instance->usb7)
			partCount7 += DeviceHandler::instance->usb7->GetPartitionCount();

	return partCount0+partCount1+partCount2+
		partCount3+partCount4+partCount5+partCount6+partCount7;
}

int DeviceHandler::PartitionToUSBPort(int part)
{
	if(!DeviceHandler::instance)
		return 0;

		int partCount0 = 0;
		int partCount1 = 0;
		int partCount2 = 0;
		int partCount3 = 0;
		int partCount4 = 0;
		int partCount5 = 0;
		int partCount6 = 0;
		int partCount7 = 0;

		if(DeviceHandler::instance->usb0)
			partCount0 += DeviceHandler::instance->usb0->GetPartitionCount();
		if(DeviceHandler::instance->usb1)
			partCount1 += DeviceHandler::instance->usb1->GetPartitionCount();
		if(DeviceHandler::instance->usb2)
			partCount2 += DeviceHandler::instance->usb2->GetPartitionCount();
		if(DeviceHandler::instance->usb3)
			partCount3 += DeviceHandler::instance->usb3->GetPartitionCount();
		if(DeviceHandler::instance->usb4)
			partCount4 += DeviceHandler::instance->usb4->GetPartitionCount();
		if(DeviceHandler::instance->usb5)
			partCount5 += DeviceHandler::instance->usb5->GetPartitionCount();
		if(DeviceHandler::instance->usb6)
			partCount6 += DeviceHandler::instance->usb6->GetPartitionCount();
		if(DeviceHandler::instance->usb7)
			partCount7 += DeviceHandler::instance->usb7->GetPartitionCount();


		part -= partCount0;
		if(part <= 0)
			return 0;

		part -= partCount1;
		if(part <= 0)
			return 1;

		part -= partCount2;
		if(part <= 0)
			return 2;

		part -= partCount3;
		if(part <= 0)
			return 3;

		part -= partCount4;
		if(part <= 0)
			return 4;

		part -= partCount5;
		if(part <= 0)
			return 5;

		part -= partCount6;
		if(part <= 0)
			return 6;

		part -= partCount7;
		if(part <= 0)
			return 7;

		return 0;
}

int DeviceHandler::PartitionToPortPartition(int part)
{
	if(!DeviceHandler::instance)
		return 0;

		int partCount0 = 0;
		int partCount1 = 0;
		int partCount2 = 0;
		int partCount3 = 0;
		int partCount4 = 0;
		int partCount5 = 0;
		int partCount6 = 0;
		int partCount7 = 0;

		if(DeviceHandler::instance->usb0)
			partCount0 += DeviceHandler::instance->usb0->GetPartitionCount();
		if(DeviceHandler::instance->usb1)
			partCount1 += DeviceHandler::instance->usb1->GetPartitionCount();
		if(DeviceHandler::instance->usb2)
			partCount2 += DeviceHandler::instance->usb2->GetPartitionCount();
		if(DeviceHandler::instance->usb3)
			partCount3 += DeviceHandler::instance->usb3->GetPartitionCount();
		if(DeviceHandler::instance->usb4)
			partCount4 += DeviceHandler::instance->usb4->GetPartitionCount();
		if(DeviceHandler::instance->usb5)
			partCount5 += DeviceHandler::instance->usb5->GetPartitionCount();
		if(DeviceHandler::instance->usb6)
			partCount6 += DeviceHandler::instance->usb6->GetPartitionCount();
		if(DeviceHandler::instance->usb7)
			partCount7 += DeviceHandler::instance->usb7->GetPartitionCount();


		part -= partCount0;
		if(part <= 0)
			return part + partCount0;

		part -= partCount1;
		if(part <= 0)
			return part + partCount1;

		part -= partCount2;
		if(part <= 0)
			return part + partCount2;

		part -= partCount3;
		if(part <= 0)
			return part + partCount3;

		part -= partCount4;
		if(part <= 0)
			return part + partCount4;

		part -= partCount5;
		if(part <= 0)
			return part + partCount5;

		part -= partCount6;
		if(part <= 0)
			return part + partCount6;

		part -= partCount7;
		if(part <= 0)
			return part + partCount7;

		return 0;
}

PartitionHandle *DeviceHandler::GetUSBHandleFromPartition(int part) const
{
	if(PartitionToUSBPort(part) == 0)
		return usb0;
	if(PartitionToUSBPort(part) == 1)
		return usb1;
	if(PartitionToUSBPort(part) == 2)
		return usb2;
	if(PartitionToUSBPort(part) == 3)
		return usb3;
	if(PartitionToUSBPort(part) == 4)
		return usb4;
	if(PartitionToUSBPort(part) == 5)
		return usb5;
	if(PartitionToUSBPort(part) == 6)
		return usb6;
	if(PartitionToUSBPort(part) == 7)
		return usb7;
	return usb0;
}
