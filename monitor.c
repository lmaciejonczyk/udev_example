#include <stdio.h>
#include <unistd.h>
#include <libudev.h>
#include <string.h>

int WaitForUsbDeviceRenumeration(const char *radioUrl)
{
	struct udev *udev;
	struct udev_device *dev;
	struct udev_monitor *mon;
	int fd;
	fd_set fds;
	struct timeval tv;
	int ret;

	udev = udev_new();
	if (!udev) {
		fprintf(stderr, "Can't create udev\n");
		return 1;
	}

	mon = udev_monitor_new_from_netlink(udev, "udev");
	udev_monitor_filter_add_match_subsystem_devtype(mon, "tty", NULL);
	udev_monitor_enable_receiving(mon);
	fd = udev_monitor_get_fd(mon);

	while (1) {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		tv.tv_sec = 0;
		tv.tv_usec = 0;

		ret = select(fd + 1, &fds, NULL, NULL, &tv);
		if (ret > 0 && FD_ISSET(fd, &fds)) {
			dev = udev_monitor_receive_device(mon);
			if (dev) {
				struct udev_list_entry *entry;
				const char *action = udev_device_get_action(dev);

				if (strncmp(action, "add", strlen("add")) == 0) {
					udev_list_entry_foreach(
						entry, udev_device_get_devlinks_list_entry(dev))
					{
						const char *name = udev_list_entry_get_name(entry);
						if (strncmp(name, radioUrl, strlen(radioUrl)) ==
						    0) {
							printf("name: %s\n", name);
							printf("I: ACTION=%s\n", action);
							printf("I: DEVNODE=%s\n",
							       udev_device_get_devnode(dev));
							printf("---\n");
							goto exit;
						}
					}
				}

				/* free dev */
				udev_device_unref(dev);
			}
		}
		/* 500 milliseconds */
		usleep(500 * 1000);
	}

exit:
	udev_device_unref(dev);
	udev_unref(udev);

	return 0;
}

int main()
{
	const char *radioUrl =
		"/dev/serial/by-id/usb-Nordic_Semiconductor_ASA_Thread_Co-Processor_3F3CD0D4D2D23CF3-if00";

	WaitForUsbDeviceRenumeration(radioUrl);

	return 0;
}