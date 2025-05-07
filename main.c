#include "sonarpen.h"

// Declaration of SPmouse_HID function (but not its implementation)
extern int SPmouse_HID();

int main() {
    // Call SPmouse_HID to initialize the system
    int result = SPmouse_HID();
    return result;
}
