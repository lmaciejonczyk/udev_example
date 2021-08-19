#include <stdio.h>
#include <unistd.h>
#include <libudev.h>
#include <string.h>
#include <stdbool.h>

struct usb_device_info {
	char dev_name[15];
	char id_vendor[5];
	char id_product[5];
};

bool get_usb_device_info(struct udev_device *dev, struct usb_device_info *dev_info)
{
	const char *prop_value;

	prop_value = udev_device_get_property_value(dev, "DEVNAME");
	if (prop_value == NULL) {
		return false;
	}
	strcpy(dev_info->dev_name, prop_value);

	dev = udev_device_get_parent(dev);
	dev = udev_device_get_parent(dev);

	prop_value = udev_device_get_sysattr_value(dev, "idVendor");
	if (prop_value == NULL) {
		return false;
	}
	strcpy(dev_info->id_vendor, prop_value);

	prop_value = udev_device_get_sysattr_value(dev, "idProduct");
	if (prop_value == NULL) {
		return false;
	}
	strcpy(dev_info->id_product, prop_value);

	return true;
}

static void print_all_attributes(struct udev_device *dev, const char *key)
{
	struct udev_list_entry *props = udev_device_get_properties_list_entry(dev);
	while (props != NULL) {
		printf("%s = %s\n", udev_list_entry_get_name(props),
		       udev_list_entry_get_value(props));
		props = udev_list_entry_get_next(props);
	}

	printf("\n");
	printf("ID_SERIAL: %s\n", udev_device_get_property_value(dev, "ID_SERIAL"));
	// printf("syspath: %s\n", udev_device_get_syspath(dev));
	// printf("sysname: %s\n", udev_device_get_sysname(dev));
	// printf("sysnum: %s\n", udev_device_get_sysnum(dev));
	// printf("devpath: %s\n", udev_device_get_devpath(dev));
	printf("devnode: %s\n", udev_device_get_devnode(dev));
	// printf("devtype: %s\n", udev_device_get_devtype(dev));
	// printf("subsystem: %s\n", udev_device_get_subsystem(dev));
	// printf("driver: %s\n", udev_device_get_driver(dev));
	// printf("devtype: %s\n", udev_device_get_devtype(dev));
	printf("idVendor: %s\n", udev_device_get_sysattr_value(dev, "idVendor"));
	printf("serial: %s\n", udev_device_get_sysattr_value(dev, "serial"));
	printf("\n");

	// props = udev_device_get_sysattr_list_entry(dev);
	// while (props != NULL) {
	// 	printf("%s = %s\n", udev_list_entry_get_name(props),
	// 	       udev_device_get_sysattr_value(dev, udev_list_entry_get_name(props)));
	// 	props = udev_list_entry_get_next(props);
	// }
}

int monitor(void)
{
	struct udev *udev;
	struct udev_device *dev;
	struct udev_monitor *mon;
	int fd;
	fd_set fds;
	struct timeval tv;
	int ret;

	printf("zaczynamy\n");

	udev = udev_new();
	if (!udev) {
		printf("Can't create udev\n");
		return 1;
	}

	mon = udev_monitor_new_from_netlink(udev, "kernel");
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

				printf("I: ACTION=%s\n", action);

				if (action != NULL && strcmp(action, "add") == 0) {
					struct usb_device_info dev_info;
					if (get_usb_device_info(dev, &dev_info)) {
						printf("dev_name: %s\n", dev_info.dev_name);
						printf("vendor id: %s\n", dev_info.id_vendor);
						printf("product id: %s\n", dev_info.id_product);
					} else {
						printf("ERR: getting device info failed!");
					}
				}

				// printf("syspath: %s\n", udev_device_get_syspath(dev));
				// printf("sysname: %s\n", udev_device_get_sysname(dev));
				// printf("sysnum: %s\n", udev_device_get_sysnum(dev));
				// printf("devpath: %s\n", udev_device_get_devpath(dev));
				// printf("devnode: %s\n", udev_device_get_devnode(dev));
				// printf("devtype: %s\n", udev_device_get_devtype(dev));
				// printf("subsystem: %s\n", udev_device_get_subsystem(dev));
				// printf("driver: %s\n", udev_device_get_driver(dev));
				// printf("devtype: %s\n", udev_device_get_devtype(dev));
				// printf("sysattr: %s\n",
				//        udev_device_get_sysattr_value(dev, "idVendor"));

				// udev_list_entry_foreach(entry,
				// 			udev_device_get_devlinks_list_entry(dev))
				// {
				// 	const char *name = udev_list_entry_get_name(entry);

				// 	printf("name: %s\n", name);
				// 	printf("I: ACTION=%s\n", action);
				// 	printf("I: DEVNODE=%s\n", udev_device_get_devnode(dev));
				// 	printf("---\n");
				// }

				// struct udev_device *child;
				// struct udev_device *parent;

				// for (child = dev; child != NULL;
				//      child = udev_device_get_parent(child)) {
				// 	print_all_attributes(child, "key");
				// }

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

void print_tty_name()
{
}

int main()
{
	monitor();

	return 0;
}