#include "photon/tm/Tm.Component.h"
#include "photon/exc/Exc.Component.h"
#include "photon/fwt/Fwt.Component.h"
#include "photon/core/Logging.h"

#include <tclap/CmdLine.h>

#include <chrono>
#include <thread>
#include <cstring>
#include <cinttypes>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define _PHOTON_FNAME "Model.cpp"

constexpr std::size_t maxPacketSize = 1024;

static uint8_t temp[maxPacketSize];

int main(int argc, char* argv[])
{
    PhotonTm_Init();
    PhotonExc_Init();
    PhotonFwt_Init();

    TCLAP::CmdLine cmdLine("Photon model", ' ', "0.1");
    TCLAP::ValueArg<uint16_t> portArg("p", "port", "Port", true, 6666, "number");
    TCLAP::ValueArg<unsigned> tickArg("t", "tick", "Tick period", false, 100, "milliseconds");

    cmdLine.add(&portArg);
    cmdLine.add(&tickArg);
    cmdLine.parse(argc, argv);

    uint16_t port = portArg.getValue();
    struct sockaddr_in host;
    struct sockaddr_in from;
    socklen_t addrLen = sizeof(from);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        PHOTON_CRITICAL("Could not create udp socket");
        return -1;
    }

    struct timeval sockTimeout;
    sockTimeout.tv_sec = 0;
    sockTimeout.tv_usec = 10;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &sockTimeout, sizeof(sockTimeout)) == -1) {
        PHOTON_CRITICAL("Error setting sockopt");
        return -1;
    }

    std::memset((char *) &host, 0, sizeof(host));
    host.sin_family = AF_INET;
    host.sin_port = htons(port);
    host.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr*)&host, sizeof(host)) == -1) {
        PHOTON_CRITICAL("Could not create bind to port" PRId16 " ", port);
        return -1;
    }

    bool canSend = false;
    std::chrono::milliseconds tickTimeout(tickArg.getValue());
    while (true) {
        std::this_thread::sleep_for(tickTimeout);

        ssize_t recvSize = recvfrom(sock, temp, sizeof(temp), 0, (struct sockaddr*)&from, &addrLen);
        if (recvSize != -1) {
            canSend = true;
            PHOTON_DEBUG("Recieved %zi bytes", recvSize);
            PhotonExc_AcceptInput(temp, recvSize);
        } else {
            if (errno != EWOULDBLOCK) {
                PHOTON_WARNING("Error recieving");
            }
        }

        std::size_t genSize = PhotonExc_GenOutput(temp, sizeof(temp));

        if (!canSend || genSize == 0) {
            continue;
        }

        PHOTON_DEBUG("Sending %zu bytes", genSize);

        if (sendto(sock, temp, sizeof(temp), 0, (struct sockaddr*)&from, addrLen) == -1) {
            PHOTON_WARNING("Error sending reply");
        }
    }
}
