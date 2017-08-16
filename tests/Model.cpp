#include "photon/tm/Tm.Component.h"
#include "photon/exc/Exc.Component.h"
#include "photon/fwt/Fwt.Component.h"
#include "photon/core/Logging.h"
#include "photon/core/Core.Component.h"

#include <tclap/CmdLine.h>

#include <chrono>
#include <thread>
#include <cstring>
#include <cinttypes>

#ifdef _WIN32
# include <winsock2.h>
#else
# include <netinet/in.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <sys/ioctl.h>
#endif

#define _PHOTON_FNAME "Model.cpp"

#ifdef _WIN32
typedef SOCKET SockType;
typedef int SockLenType;
constexpr SockType invalidSock = INVALID_SOCKET;
constexpr int socketError = SOCKET_ERROR;
#else
typedef int SockType;
typedef socklen_t SockLenType;
constexpr SockType invalidSock = -1;
constexpr int socketError = -1;
#endif

constexpr std::size_t maxPacketSize = 1024;

static char temp[maxPacketSize];

int main(int argc, char* argv[])
{
    Photon_Init();

    TCLAP::CmdLine cmdLine("Photon model", ' ', "0.1");
    TCLAP::ValueArg<uint16_t> portArg("p", "port", "Port", false, 6666, "number");
    TCLAP::ValueArg<unsigned> tickArg("t", "tick", "Tick period", false, 100, "milliseconds");

    cmdLine.add(&portArg);
    cmdLine.add(&tickArg);
    cmdLine.parse(argc, argv);

#ifdef _WIN32
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Error initializing winsock2: %d", WSAGetLastError());
        return -1;
    }
#endif

begin:

    uint16_t port = portArg.getValue();
    struct sockaddr_in host;
    struct sockaddr_in from;
    SockLenType addrLen = sizeof(from);

    SockType sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == invalidSock) {
        PHOTON_CRITICAL("Could not create udp socket");
        return -1;
    }

#ifdef _WIN32
    unsigned long nonBlocking = 1;
    if (ioctlsocket(sock, FIONBIO, &nonBlocking) == socketError) {
#else
    int nonBlocking = 1;
    if (ioctl(sock, FIONBIO, &nonBlocking) == socketError) {
#endif
        PHOTON_CRITICAL("Error making socket non blocking");
        return -1;
    }

    std::memset((char *) &host, 0, sizeof(host));
    host.sin_family = AF_INET;
    host.sin_port = htons(port);
    host.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr*)&host, sizeof(host)) == socketError) {
        PHOTON_CRITICAL("Could not create bind to port" PRId16 " ", port);
        return -1;
    }

    bool canSend = false;
    std::chrono::milliseconds tickTimeout(tickArg.getValue());
    while (true) {
        std::this_thread::sleep_for(tickTimeout);

        auto recvSize = recvfrom(sock, temp, sizeof(temp), 0, (struct sockaddr*)&from, &addrLen);
        if (recvSize != socketError) {
            canSend = true;
            //PHOTON_DEBUG("Recieved %zi bytes", recvSize);
            PhotonExc_AcceptInput(temp, recvSize);
        } else {
#ifdef _WIN32
            int err = WSAGetLastError();
            if (err == WSAECONNRESET) {
                closesocket(sock);
                goto begin;
            }
            if (err != WSAEWOULDBLOCK) {
#else
            if (errno != EWOULDBLOCK) {
#endif
                PHOTON_WARNING("Error recieving: %d", err);
            }
        }

        Photon_Tick();

        const  PhotonExcMsg* msg = PhotonExc_GetMsg();

        if (!canSend) {
            continue;
        }

        //PHOTON_DEBUG("Sending %zu bytes", genSize);

#ifdef _WIN32
        if (sendto(sock, (const char*)msg->data, msg->size, 0, (struct sockaddr*)&from, addrLen) == socketError) {
#else
        if (sendto(sock, msg->data, msg->size, 0, (struct sockaddr*)&from, addrLen) == socketError) {
#endif
            PHOTON_WARNING("Error sending reply");
        } else {
            PhotonExc_PrepareNextMsg();
        }
    }
}
