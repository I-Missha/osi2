#include "main.h"

int main() {
    int server_fd = create_server_fd(PROXY_PORT);
    if (server_fd == -1) {
        perror(strerror(errno));
        return SOCKET_ERROR;
    }
    /*while (1) {*/
    /*}*/
    return 0;
}
