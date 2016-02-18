#ifndef DEVICE_HANDLER_HPP_
#define DEVICE_HANDLER_HPP_

#include "PartitionHandle.h"
#include "usbloader/usb_new.h"

#define SD_PARTITION_NUMBER 99999

class DeviceHandler
{
	public:
		static DeviceHandler * Instance();
		static void DestroyInstance();

		bool MountAll();
		void UnMountAll();

		bool MountSD();
		int MountUSB(int port);
		int MountAllUSB();

		bool IsInsertedSD();
		bool IsInsertedUSB(int port);

		void UnmountSD();
		void UnmountAllUSB();
		void UnmountUSB(int port);

		PartitionHandle * GetHandleSD() const;
		PartitionHandle * GetHandleUSB(int port) const;
		PartitionHandle * GetHandleFromPartition(int part);
		const DISC_INTERFACE *GetInterfaceUSB(int port);
		int GetFilesystemType(int part);
		const char * GetFSName(int part);
		int GetPartitionNumber(const char * path);
		const char * GetFSName(const char * path);
		const char * GetPartitionPrefix(const char * path);
		bool IsSDPartition(int part);
		int PartitionToPortUSB(int part);
		const char * GetPartitionPrefix(int partition);
		u16 GetTotalPartitionCount();
	private:
		static DeviceHandler *instance;

		PartitionHandle * sdHandle;
		PartitionHandle * usbHandles[MAX_USB_STORAGE_DEVICES];

		DeviceHandler();
		~DeviceHandler();
};

#endif
