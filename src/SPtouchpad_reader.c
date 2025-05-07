#include "sonarpen.h"

/*this page contains the code for fetching touchpad coordinates*/

/**
 * @brief Initializes the touchpad device by opening it and creating a libevdev context.
 * 
 * This function attempts to open the specified device file in non-blocking mode and initializes
 * a libevdev context from the file descriptor. If successful, it prints device information.
 * 
 * @param dev Pointer to a pointer of libevdev structure where the initialized context will be stored.
 * @param device_path Path to the device file to be opened.
 * 
 * @return 0 on success, 1 on failure.
 */
int init_touchpad_device(struct libevdev **dev, const char *device_path) {
    int fd, rc;

    fd = open(device_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    rc = libevdev_new_from_fd(fd, dev);
    if (rc < 0) {
        fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
        close(fd);
        return 1;
    }

    printf("Touchpad device information:\n");
    printf("  Name: %s\n", libevdev_get_name(*dev));
    printf("  ID: bus %d, vendor 0x%x, product 0x%x, version 0x%x\n",
           libevdev_get_id_bustype(*dev),
           libevdev_get_id_vendor(*dev),
           libevdev_get_id_product(*dev),
           libevdev_get_id_version(*dev));

    return 0;
}

/**
 * @brief Processes touchpad events and prints coordinates.
 * 
 * This function reads events from the touchpad device and prints the X and Y coordinates
 * if the events are of type EV_ABS and have codes ABS_X or ABS_Y. The function continues
 * to read events until there are no more events to process.
 * 
 * @param dev Pointer to the libevdev context representing the touchpad device.
 */
void process_touchpad_events(struct libevdev *dev) {
    int rc;
    do {
        struct input_event ev;
        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc == 0) {
            if (ev.type == EV_ABS) {
                if (ev.code == ABS_X) {
                    printf("X: %d\n", ev.value);
                } else if (ev.code == ABS_Y) {
                    printf("Y: %d\n", ev.value);
                }
            }
        } else if (rc != -EAGAIN) {
            fprintf(stderr, "Error: %s\n", strerror(-rc));
        }
    } while (rc == 1 || rc == 0 || rc == -EAGAIN);
}
