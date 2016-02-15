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
#ifndef DEVICE_HANDLER_HPP_
#define DEVICE_HANDLER_HPP_

#include "PartitionHandle.h"
#include "usbloader/usb_new.h"

/**
 * libogc device names.
 */
enum
{
	SD = 0,
	USB1,
	USB2,
	USB3,
	USB4,
	USB5,
	USB6,
	USB7,
	USB8,
	MAXDEVICES
};

/**
 * libogc device names.
 */
const char DeviceName[MAXDEVICES][8] =
{
	"sd",
	"usb1",
	"usb2",
	"usb3",
	"usb4",
	"usb5",
	"usb6",
	"usb7",
	"usb8",
};

class DeviceHandler
{
	public:
		static DeviceHandler * Instance();
		static void DestroyInstance();

		bool MountAll();
		void UnMountAll();
		bool Mount(int dev);
		bool IsInserted(int dev);
		void UnMount(int dev);

		//! Individual Mounts/UnMounts...
		bool MountSD();
		bool MountAllUSB(bool spinUp = true);
		bool SD_Inserted() { if(sd) return sd->IsInserted(); return false; }
		bool USB0_Inserted() { if(usb0) return usb0->IsInserted(); return false; }
		bool USB1_Inserted() { if(usb1) return usb1->IsInserted(); return false; }
		bool USB2_Inserted() { if(usb2) return usb2->IsInserted(); return false; }
		bool USB3_Inserted() { if(usb3) return usb3->IsInserted(); return false; }
		bool USB4_Inserted() { if(usb4) return usb4->IsInserted(); return false; }
		bool USB5_Inserted() { if(usb5) return usb5->IsInserted(); return false; }
		bool USB6_Inserted() { if(usb6) return usb6->IsInserted(); return false; }
		bool USB7_Inserted() { if(usb7) return usb7->IsInserted(); return false; }
		void UnMountSD() { if(sd) delete sd; sd = NULL; }
		void UnMountUSB(int pos);
		void UnMountAllUSB();
		PartitionHandle * GetSDHandle() const { return sd; }
		PartitionHandle * GetUSB0Handle() const { return usb0; }
		PartitionHandle * GetUSB1Handle() const { return usb1; }
		PartitionHandle * GetUSB2Handle() const { return usb2; }
		PartitionHandle * GetUSB3Handle() const { return usb3; }
		PartitionHandle * GetUSB4Handle() const { return usb4; }
		PartitionHandle * GetUSB5Handle() const { return usb5; }
		PartitionHandle * GetUSB6Handle() const { return usb6; }
		PartitionHandle * GetUSB7Handle() const { return usb7; }
		PartitionHandle * GetUSBHandleFromPartition(int part) const;
		static const DISC_INTERFACE *GetUSB0Interface() { return &__io_usbstorage2_port0; }
		static const DISC_INTERFACE *GetUSB1Interface() { return &__io_usbstorage2_port1; }
		static const DISC_INTERFACE *GetUSB2Interface() { return &__io_usbstorage2_port2; }
		static const DISC_INTERFACE *GetUSB3Interface() { return &__io_usbstorage2_port3; }
		static const DISC_INTERFACE *GetUSB4Interface() { return &__io_usbstorage2_port4; }
		static const DISC_INTERFACE *GetUSB5Interface() { return &__io_usbstorage2_port5; }
		static const DISC_INTERFACE *GetUSB6Interface() { return &__io_usbstorage2_port6; }
		static const DISC_INTERFACE *GetUSB7Interface() { return &__io_usbstorage2_port7; }
		static int GetFilesystemType(int dev);
		static const char * GetFSName(int dev);
		static int PathToDriveType(const char * path);
		static const char * PathToFSName(const char * path) { return GetFSName(PathToDriveType(path)); }
		static const char * GetDevicePrefix(const char * path);
		static int PartitionToUSBPort(int part);
		static u16 GetUSBPartitionCount();
		static int PartitionToPortPartition(int part);
	private:
		DeviceHandler() : sd(0), gca(0), gcb(0), usb0(0), usb1(0)
		 	, usb2(0), usb3(0), usb4(0), usb5(0), usb6(0), usb7(0){ }
		~DeviceHandler();
		bool MountUSB(int part);

		static DeviceHandler *instance;

		PartitionHandle * sd;
		PartitionHandle * gca;
		PartitionHandle * gcb;
		PartitionHandle * usb0;
		PartitionHandle * usb1;
		PartitionHandle * usb2;
		PartitionHandle * usb3;
		PartitionHandle * usb4;
		PartitionHandle * usb5;
		PartitionHandle * usb6;
		PartitionHandle * usb7;
};

#endif
