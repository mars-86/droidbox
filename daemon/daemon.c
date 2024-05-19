#include "daemon.h"
#include <stdio.h>

#define PATH "/tmp/droidbox.sock"

void on_listen(void* usr)
{
    const char* path = (const char*)usr;

    printf("Daemon listening on %s\n", path);
}

int daemon_init(int argc, char* argv[])
{
    local_server_t* daemon = server_local_init(NULL);

    if (!daemon)
        return 1;

    server_local_listen(daemon, PATH, on_listen);

    server_local_destroy(daemon);

    return 0;
}
