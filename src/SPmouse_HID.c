#include "sonarpen.h"
#include <fcntl.h>
#include <linux/uinput.h>
#include <math.h>

// Define emit function
void emit(int fd, int type, int code, int value) {
    struct input_event ev;
    ev.type = type;
    ev.code = code;
    ev.value = value;
    gettimeofday(&ev.time, NULL);
    if (write(fd, &ev, sizeof(ev)) < 0) {
        perror("emit: write");
    }
}

// Define setup_uinput_device function
int setup_uinput_device(void) {
    struct uinput_setup usetup;
    struct uinput_abs_setup abs_setup;

    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Opening uinput device");
        return -1;
    }

    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    ioctl(fd, UI_SET_ABSBIT, ABS_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_Y);
    ioctl(fd, UI_SET_ABSBIT, ABS_PRESSURE);
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_TOOL_PEN);
    ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);

    memset(&abs_setup, 0, sizeof(abs_setup));
    abs_setup.code = ABS_X;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = 32767;
    abs_setup.absinfo.resolution = 100;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);

    abs_setup.code = ABS_Y;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);

    abs_setup.code = ABS_PRESSURE;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = 255;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);

    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1209;
    usetup.id.product = 0x5678;
    strcpy(usetup.name, "Virtual Pen Tablet");

    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);
fprintf("innitialized");
    return fd;
}

// Main function for SPmouse_HID
int SPmouse_HID() {
    static float last_volume = 0.0;

    AudioCapture audio_capture = {0};
    if (init_audio_capture(&audio_capture) < 0) {
        fprintf(stderr, "Failed to initialize audio capture.\n");
        return 1;
    }

    struct libevdev *touchpad_dev = NULL;
    const char *touchpad_path = "/dev/input/event7";
    if (init_touchpad_device(&touchpad_dev, touchpad_path) != 0) {
        cleanup_audio_capture(&audio_capture);
        return 1;
    }

    int uinput_fd = setup_uinput_device();
    if (uinput_fd < 0) {
        libevdev_free(touchpad_dev);
        cleanup_audio_capture(&audio_capture);
        return 1;
    }

    if (init_audio_playback() < 0) {
        libevdev_free(touchpad_dev);
        cleanup_audio_capture(&audio_capture);
        ioctl(uinput_fd, UI_DEV_DESTROY);
        close(uinput_fd);
        return 1;
    }

    while (1) {
        float volume = capture_audio(&audio_capture);
        if (volume < 0) {
            break;
        }

        last_volume = volume;
        printf("Microphone Volume (RMS): %.2f\n", volume);

        struct input_event ev;
        int rc = libevdev_next_event(touchpad_dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        if (rc == 0) {
            if (ev.type == EV_ABS) {
                if (ev.code == ABS_X) {
                    emit(uinput_fd, EV_ABS, ABS_X, ev.value);
                } else if (ev.code == ABS_Y) {
                    emit(uinput_fd, EV_ABS, ABS_Y, ev.value);
                }
            }

            int pressure = (int)(last_volume * 255.0f / MAX_RMS_VALUE);
            if (pressure > 255) pressure = 255;
            emit(uinput_fd, EV_ABS, ABS_PRESSURE, pressure);

            emit(uinput_fd, EV_SYN, SYN_REPORT, 0);
        }

        if (play_tone(2000.0) < 0) {
            break;
        }
    }

    libevdev_free(touchpad_dev);
    cleanup_audio_capture(&audio_capture);
    cleanup_audio_playback();
    ioctl(uinput_fd, UI_DEV_DESTROY);
    close(uinput_fd);

    return 0;
}
