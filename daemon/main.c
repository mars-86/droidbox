#include "daemon.h"

int main(int argc, char* argv[])
{
    int status;
    status = daemon_init(argc, argv);

    return status;
}
