#include "photongen/onboard/tm/Tm.Component.h"
#include "photongen/onboard/exc/Exc.Component.h"
#include "photongen/onboard/fwt/Fwt.Component.h"
#include "photon/core/Logging.h"
#include "photongen/onboard/core/Core.Component.h"

#include <tclap/CmdLine.h>

#include <bmcl/Assert.h>

#include <chrono>
#include <thread>
#include <cstring>
#include <cinttypes>

#ifdef _WIN32
 #include <winsock2.h>
#else
 #include <netinet/in.h>
 #include <sys/ioctl.h>
 #include <sys/socket.h>
 #include <sys/types.h>
 #include <unistd.h>
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

static char _temp[maxPacketSize];

int main(int argc, char* argv[])
{
    Photon_Init();

    TCLAP::CmdLine cmdLine("Photon model", ' ', "0.1");
    TCLAP::ValueArg<uint64_t> mccArg("m", "mcc-id", "Mcc id", false, 0, "number");
    TCLAP::ValueArg<uint64_t> uavArg("u", "uav-id", "Uav id", false, 1, "number");
    TCLAP::ValueArg<uint16_t> portArg("p", "port", "Port", false, 6666, "number");
    TCLAP::ValueArg<unsigned> tickArg("t", "tick", "Tick period", false, 100, "milliseconds");

    cmdLine.add(&mccArg);
    cmdLine.add(&uavArg);
    cmdLine.add(&portArg);
    cmdLine.add(&tickArg);
    cmdLine.parse(argc, argv);

#ifdef _WIN32
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Error initializing winsock2: %d", WSAGetLastError());
        return -1;
    }

begin:
#endif

    uint16_t port = portArg.getValue();
    struct sockaddr_in host;
    struct sockaddr_in from;
    SockLenType addrLen = sizeof(from);

    SockType sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == invalidSock) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
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
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        PHOTON_CRITICAL("Error making socket non blocking");
        return -1;
    }

    std::memset((char *) &host, 0, sizeof(host));
    host.sin_family = AF_INET;
    host.sin_port = htons(port);
    host.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr*)&host, sizeof(host)) == socketError) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        PHOTON_CRITICAL("Could not create bind to port" PRId16 " ", port);
        return -1;
    }

    PhotonExc_SetAddress(uavArg.getValue());
    PhotonExcDevice* dev;
    auto rv = PhotonExc_RegisterGroundControl(mccArg.getValue(), &dev);
    BMCL_ASSERT(rv == PhotonExcClientError_Ok);
    bool canSend = false;
    std::chrono::milliseconds tickTimeout(tickArg.getValue());
    while (true) {
        std::this_thread::sleep_for(tickTimeout);

        auto recvSize = recvfrom(sock, _temp, sizeof(_temp), 0, (struct sockaddr*)&from, &addrLen);
        if (recvSize != socketError) {
            canSend = true;
            //PHOTON_DEBUG("Recieved %zi bytes", recvSize);
            PhotonExcDevice_AcceptInput(dev, _temp, recvSize);
        } else {
#ifdef _WIN32
            int err = WSAGetLastError();
            if (err == WSAECONNRESET) {
                closesocket(sock);
                goto begin;
            }
            if (err != WSAEWOULDBLOCK) {
#else
            int err = errno;
            if (err != EWOULDBLOCK) {
#endif
                PHOTON_WARNING("Error recieving: %d", err);
            }
        }

        Photon_Tick();

        if (!canSend) {
            continue;
        }

        //PHOTON_DEBUG("Sending %zu bytes", genSize);

        PhotonWriter writer;
        PhotonWriter_Init(&writer, _temp, sizeof(_temp));
        PhotonError err = PhotonExcDevice_GenNextPacket(dev, &writer);
        (void)err;
        const void* data = writer.start;
        std::size_t size = writer.current - writer.start;

#ifdef _WIN32
        if (sendto(sock, (const char*)data, size, 0, (struct sockaddr*)&from, addrLen) == socketError) {
#else
        if (sendto(sock, data, size, 0, (struct sockaddr*)&from, addrLen) == socketError) {
#endif
            PHOTON_WARNING("Error sending reply");
        }
    }
}

#undef _PHOTON_FNAME
