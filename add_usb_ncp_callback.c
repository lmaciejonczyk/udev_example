#include <libudev.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

static struct udev_device *get_child(struct udev *udev, struct udev_device *parent,
				     const char *subsystem)
{
	struct udev_device *child = NULL;
	struct udev_enumerate *enumerate = udev_enumerate_new(udev);

	udev_enumerate_add_match_parent(enumerate, parent);
	udev_enumerate_add_match_subsystem(enumerate, subsystem);
	udev_enumerate_scan_devices(enumerate);

	struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
	struct udev_list_entry *entry;

	udev_list_entry_foreach(entry, devices)
	{
		const char *path = udev_list_entry_get_name(entry);
		child = udev_device_new_from_syspath(udev, path);
		break;
	}

	udev_enumerate_unref(enumerate);
	return child;
}

static void enumerate_usb_mass_storage(struct udev *udev)
{
	struct udev_enumerate *enumerate = udev_enumerate_new(udev);

	udev_enumerate_add_match_subsystem(enumerate, "scsi");
	udev_enumerate_add_match_property(enumerate, "DEVTYPE", "scsi_device");
	udev_enumerate_scan_devices(enumerate);

	struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
	struct udev_list_entry *entry;

	udev_list_entry_foreach(entry, devices)
	{
		const char *path = udev_list_entry_get_name(entry);
		struct udev_device *scsi = udev_device_new_from_syspath(udev, path);

		struct udev_device *block = get_child(udev, scsi, "block");
		struct udev_device *scsi_disk = get_child(udev, scsi, "scsi_disk");

		struct udev_device *usb =
			udev_device_get_parent_with_subsystem_devtype(scsi, "usb", "usb_device");

		if (block && scsi_disk && usb) {
			printf("block = %s, usb = %s:%s, scsi = %s\n",
			       udev_device_get_devnode(block),
			       udev_device_get_sysattr_value(usb, "idVendor"),
			       udev_device_get_sysattr_value(usb, "idProduct"),
			       udev_device_get_sysattr_value(scsi, "vendor"));
		}

		if (block) {
			udev_device_unref(block);
		}

		if (scsi_disk) {
			udev_device_unref(scsi_disk);
		}

		udev_device_unref(scsi);
	}

	udev_enumerate_unref(enumerate);
}

static struct udev_device *device_from_fd(struct udev *udev)
{
	int mSockFd = -1;
	struct udev_device *device;
	struct stat buf;

	mSockFd = open(
		"/dev/serial/by-id/usb-Nordic_Semiconductor_ASA_Thread_Co-Processor_3F3CD0D4D2D23CF3-if00",
		O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (mSockFd < 0) {
		printf("failed to open file, errno: %d", errno);
	}

	if (fstat(mSockFd, &buf) < 0) {
		printf("failed to stat fd %d", mSockFd);
	}

	device = udev_device_new_from_devnum(udev, 'c', buf.st_rdev);
	if (device == NULL) {
		printf("could not create udev device for fd %d", mSockFd);
	}

	close(mSockFd);

	return device;
}

int main()
{
	struct udev *udev = udev_new();

	enumerate_usb_mass_storage(udev);

	udev_unref(udev);

	printf("koniec\n");

	return 0;
}