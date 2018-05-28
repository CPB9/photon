#include "UiTest.h"
#include "SerialStream.h"
#include "UdpStream.h"

#include <photon/groundcontrol/AllowUnsafeMessageType.h>

#include <bmcl/String.h>

#include <tclap/CmdLine.h>

#include <cstdint>

using namespace photon;

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);

using namespace decode;

const char* usage =
"Examples:\n"
"photon-client --device udp:192.168.0.111:6666\n"
"photon-client --device udp:93.175.77.123:8888\n"
"photon-client --device serial:/dev/ttyUSB0:9600\n"
"photon-client --device serial:COM5:115200:8N1\n"
;

using asio::ip::udp;

int main(int argc, char** argv)
{
    TCLAP::CmdLine cmdLine(usage);
    TCLAP::ValueArg<std::string> deviceArg("d", "device", "Device", true, "udp,127.0.0.1,6666", "device string");
    TCLAP::ValueArg<uint64_t> srcArg("m", "mcc-id", "Mcc id", false, 1, "number");
    TCLAP::ValueArg<uint64_t> destArg("u", "uav-id", "Uav id", false, 2, "number");

    cmdLine.add(&deviceArg);
    cmdLine.add(&srcArg);
    cmdLine.add(&destArg);
    cmdLine.parse(argc, argv);


    auto parts = bmcl::split(deviceArg.getValue(), ':');

    if (parts.empty()) {
        BMCL_CRITICAL() << "invalid device string: " << deviceArg.getValue();
        return -1;
    }

    const std::string& devType = parts[0];
    asio::io_service ioService;

    if (devType == "serial") {
        asio::serial_port serial(ioService);

        if (parts.size() < 2) {
            BMCL_CRITICAL() << "no serial device path specified: " << deviceArg.getValue();
            return -1;
        }

        const std::string& serialPort = parts[1];

        if (parts.size() < 3) {
            BMCL_CRITICAL() << "no serial device baud rate specified: " << deviceArg.getValue();
            return -1;
        }

        const std::string& baudString = parts[2];

        char* end;
        unsigned long serialBaud = std::strtoul(baudString.c_str(), &end, 10);
        if (end != (baudString.data() + baudString.size())) {
            BMCL_CRITICAL() << "invalid serial device baud rate: " << baudString;
            return -1;
        }

        asio::serial_port::stop_bits::type stopBitsType = asio::serial_port::stop_bits::one;
        asio::serial_port::parity::type parity = asio::serial_port::parity::none;
        unsigned int characterSize = 8;

        if (parts.size() >= 4) {
            const std::string& serialOpts = parts[3];
            if (serialOpts.size() != 3) {
                BMCL_CRITICAL() << "invalid serial options: " << serialOpts;
                return -1;
            }
            char numBitsChar = serialOpts[0];
            char parityChar = serialOpts[1];
            char stopBitsChar = serialOpts[2];

            switch (numBitsChar) {
            case '5':
                characterSize = 5;
                break;
            case '6':
                characterSize = 6;
                break;
            case '7':
                characterSize = 7;
                break;
            case '8':
                characterSize = 8;
                break;
            default:
                BMCL_CRITICAL() << "invalid number of bits: " << numBitsChar;
                return -1;
            }

            switch (parityChar) {
            case 'n':
            case 'N':
                parity = asio::serial_port::parity::none;
                break;
            case 'o':
            case 'O':
                parity = asio::serial_port::parity::odd;
                break;
            case 'e':
            case 'E':
                parity = asio::serial_port::parity::even;
                break;
            default:
                BMCL_CRITICAL() << "invalid parity: " << parityChar;
                return -1;
            }

            switch (stopBitsChar) {
            case '1':
                stopBitsType = asio::serial_port::stop_bits::one;
                break;
            case '2':
                stopBitsType = asio::serial_port::stop_bits::two;
                break;
            default:
                BMCL_CRITICAL() << "invalid stop bits num: " << stopBitsChar;
                return -1;
            }
        }


        asio::error_code err;
        serial.open(serialPort, err);
        if (err) {
            BMCL_CRITICAL() << "could not open serial port: " << err.message();
            return err.value();
        }

        serial.set_option(asio::serial_port::baud_rate(serialBaud), err);
        if (err) {
            BMCL_CRITICAL() << "could not set serial baud rate " << serialBaud << ": " << err.message();
            return err.value();
        }

        serial.set_option(asio::serial_port::character_size(characterSize), err);
        if (err) {
            BMCL_CRITICAL() << "could not set serial character size " << characterSize << ": " << err.message();
            return err.value();
        }

        serial.set_option(asio::serial_port::parity(parity), err);
        if (err) {
            BMCL_CRITICAL() << "could not set serial parity " << parity << ": " << err.message();
            return err.value();
        }

        serial.set_option(asio::serial_port::stop_bits(stopBitsType), err);
        if (err) {
            BMCL_CRITICAL() << "could not set serial stop bits " << stopBitsType << ": " << err.message();
            return err.value();
        }

        return runUiTest<SerialStream>(argc, argv, srcArg.getValue(), destArg.getValue(), std::move(serial));
    } else if (devType == "udp") {
        if (parts.size() < 2) {
            BMCL_CRITICAL() << "no ip address specified: " << deviceArg.getValue();
            return -1;
        }

        const std::string& ipAddr = parts[1];

        if (parts.size() < 3) {
            BMCL_CRITICAL() << "no port specified: " << deviceArg.getValue();
            return -1;
        }

        asio::error_code err;
        asio::ip::address address = asio::ip::make_address(ipAddr, err);
        if (err) {
            BMCL_CRITICAL() << "invalid ip address " << ipAddr << ": " << err.message();
            return err.value();
        }

        const std::string& portString = parts[2];

        char* end;
        unsigned long port = std::strtoul(portString.c_str(), &end, 10);
        if (end != (portString.data() + portString.size())) {
            BMCL_CRITICAL() << "invalid port: " << portString;
            return -1;
        }
        if (port > std::numeric_limits<std::uint16_t>::max()) {
            BMCL_CRITICAL() << "invalid port: " << port;
            return -1;
        }

        udp::endpoint endpoint(address, port);
        udp::socket socket(ioService);
        socket.open(udp::v4(), err);
        if (err) {
            BMCL_CRITICAL() << "error opening socket: " << err.message();
            return err.value();
        }

        return runUiTest<UdpStream>(argc, argv, srcArg.getValue(), destArg.getValue(), std::move(socket), std::move(endpoint));
    }

    BMCL_CRITICAL() << "invalid device type: " << devType;
    return -1;
}

