#include "core.h"
#include "daemon.h"

#define DEV_USB_DIR "/dev/bus/usb"

int main(int argc, char* argv[])
{
    scan_ports(DEV_USB_DIR);
    // return daemon_init(argc, argv);
}
