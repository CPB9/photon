#include "photon/tm/Tm.Component.h"
#include "photon/exc/Exc.Component.h"
#include "photon/fwt/Fwt.Component.h"

#include <bmcl/Logging.h>

#include <tclap/CmdLine.h>

#include <chrono>
#include <thread>
#include <cstring>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

constexpr std::size_t maxPacketSize = 1024;

static uint8_t temp[maxPacketSize];

int main(int argc, char* argv[])
{
    PhotonTm_Init();
    PhotonExc_Init();
    PhotonFwt_Init();

    TCLAP::CmdLine cmdLine("Photon model", ' ', "0.1");
    TCLAP::ValueArg<in_port_t> portArg("p", "port", "Port", true, 6666, "number");
    TCLAP::ValueArg<unsigned> tickArg("t", "tick", "Tick period", false, 100, "milliseconds");

    cmdLine.add(&portArg);
    cmdLine.add(&tickArg);
    cmdLine.parse(argc, argv);

    in_port_t port = portArg.getValue();
    struct sockaddr_in host;
    struct sockaddr_in from;
    socklen_t addrLen = sizeof(from);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        BMCL_CRITICAL() << "Could not create udp socket";
        return -1;
    }

    struct timeval sockTimeout;
    sockTimeout.tv_sec = 0;
    sockTimeout.tv_usec = 10;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &sockTimeout, sizeof(sockTimeout)) == -1) {
        BMCL_CRITICAL() << "Error setting sockopt";
        return -1;
    }

    std::memset((char *) &host, 0, sizeof(host));
    host.sin_family = AF_INET;
    host.sin_port = htons(port);
    host.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr*)&host, sizeof(host)) == -1) {
        BMCL_CRITICAL() << "Could not create bind to port " << port;
        return -1;
    }

    bool canSend = false;
    std::chrono::milliseconds tickTimeout(tickArg.getValue());
    while (true) {
        std::this_thread::sleep_for(tickTimeout);

        ssize_t recvSize = recvfrom(sock, temp, sizeof(temp), 0, (struct sockaddr*)&from, &addrLen);
        if (recvSize != -1) {
            canSend = true;
            BMCL_DEBUG() << "Recieved " << recvSize << " bytes";
            PhotonExc_AcceptInput(temp, recvSize);
        } else {
            if (errno != EWOULDBLOCK) {
                BMCL_WARNING() << "Error recieving";
            }
        }

        std::size_t genSize = PhotonExc_GenOutput(temp, sizeof(temp));

        if (!canSend || genSize == 0) {
            continue;
        }

        BMCL_DEBUG() << "Sending " << genSize << " bytes";

        if (sendto(sock, temp, sizeof(temp), 0, (struct sockaddr*)&from, addrLen) == -1) {
            BMCL_WARNING() << "Error sending reply";
        }
    }
}
