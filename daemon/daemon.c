#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT 5037

void start_daemon()
{
    int sd = 0, ret = 0;
    sd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sin;

    sin.sin_family = AF_INET;
    sin.sin_port = htons(DEFAULT_PORT);
    sin.sin_addr.s_addr = INADDR_ANY;

    socklen_t sinlen = sizeof(struct sockaddr);
    ret = bind(sd, (struct sockaddr*)&sin, sinlen);

    listen(sd, 10);
}
