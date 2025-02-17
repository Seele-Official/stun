#include "stun.h"

#include <sys/socket.h>

int main(){
    LOG.set_enable(true);
    
    int socketfd = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(3478);
    local.sin_addr.s_addr = 1; // INADDR_ANY
    if (bind(socketfd, reinterpret_cast<sockaddr*>(&local), sizeof(local)) < 0){
        perror("bind failed");
        return 1;
    }

    while (true) {
        sockaddr_in remote{};
        socklen_t len = sizeof(remote);
        std::array<uint8_t, 1024> msg;
        ssize_t n = recvfrom(socketfd, msg.data(), sizeof(msg), 0, reinterpret_cast<sockaddr*>(&remote), &len);
        if (n < 0){
            perror("recvfrom failed");
            return 1;
        }

        stunMessage message{msg.data()};
        LOG.log("received message from {}:{}\n", my_inet_ntoa(remote.sin_addr.s_addr), my_ntohs(remote.sin_port));
        trval_stunMessage(message);
    }


}