#include "photon/groundcontrol/StreamFromString.h"
#include "photon/groundcontrol/SerialStream.h"
#include "photon/groundcontrol/UdpStream.h"

#include <bmcl/String.h>
#include <bmcl/Result.h>

#include <asio/io_context.hpp>

namespace photon {

bmcl::Result<caf::actor, std::string> spawnActorFromDeviceString(caf::actor_system& system, asio::io_context& ioService, const std::string& devString)
{
    using asio::ip::udp;

    auto parts = bmcl::split(devString, ':');

    if (parts.empty()) {
        return "invalid device string: " + devString;
    }

    const std::string& devType = parts[0];

    caf::actor stream;
    if (devType == "serial") {
        asio::serial_port serial(ioService);

        if (parts.size() < 2) {
            return "no serial device path specified: " + devString;
        }

        const std::string& serialPort = parts[1];

        if (parts.size() < 3) {
            return "no serial device baud rate specified: " + devString;
        }

        const std::string& baudString = parts[2];

        char* end;
        unsigned long serialBaud = std::strtoul(baudString.c_str(), &end, 10);
        if (end != (baudString.data() + baudString.size())) {
            return "invalid serial device baud rate: " + baudString;
        }

        asio::serial_port::stop_bits::type stopBitsType = asio::serial_port::stop_bits::one;
        asio::serial_port::parity::type parity = asio::serial_port::parity::none;
        unsigned int characterSize = 8;

        if (parts.size() >= 4) {
            const std::string& serialOpts = parts[3];
            if (serialOpts.size() != 3) {
                return "invalid serial options: " + serialOpts;
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
                return std::string("invalid number of bits: ") + numBitsChar;
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
                return std::string("invalid parity: ") + parityChar;
            }

            switch (stopBitsChar) {
            case '1':
                stopBitsType = asio::serial_port::stop_bits::one;
                break;
            case '2':
                stopBitsType = asio::serial_port::stop_bits::two;
                break;
            default:
                return std::string("invalid stop bits num: ") + stopBitsChar;
            }
        }


        asio::error_code err;
        serial.open(serialPort, err);
        if (err) {
            return "could not open serial port: " + err.message();
        }

        serial.set_option(asio::serial_port::baud_rate(serialBaud), err);
        if (err) {
            std::ostringstream stream;
            stream << "could not set serial baud rate " << serialBaud << ": " << err.message();
            return stream.str();
        }

        serial.set_option(asio::serial_port::character_size(characterSize), err);
        if (err) {
            std::ostringstream stream;
            stream << "could not set serial character size " << characterSize << ": " << err.message();
            return stream.str();
        }

        serial.set_option(asio::serial_port::parity(parity), err);
        if (err) {
            std::ostringstream stream;
            stream << "could not set serial parity " << parity << ": " << err.message();
            return stream.str();
        }

        serial.set_option(asio::serial_port::stop_bits(stopBitsType), err);
        if (err) {
            std::ostringstream stream;
            stream << "could not set serial stop bits " << stopBitsType << ": " << err.message();
            return stream.str();
        }

        return system.spawn<SerialStream>(std::move(serial));
    } else if (devType == "udp") {
        if (parts.size() < 2) {
            return "no ip address specified: " + devString;
        }

        const std::string& ipAddr = parts[1];

        if (parts.size() < 3) {
            return "no port specified: " + devString;
        }

        asio::error_code err;
        asio::ip::address address = asio::ip::make_address(ipAddr, err);
        if (err) {
            return "invalid ip address " + ipAddr + ": " + err.message();
        }

        const std::string& portString = parts[2];

        char* end;
        unsigned long port = std::strtoul(portString.c_str(), &end, 10);
        if (end != (portString.data() + portString.size())) {
            return "invalid port: " + portString;
        }
        if (port > std::numeric_limits<std::uint16_t>::max()) {
            return "invalid port: " + std::to_string(port);
        }

        udp::endpoint endpoint(address, port);
        udp::socket socket(ioService);
        socket.open(udp::v4(), err);
        if (err) {
            return "error opening socket: " + err.message();
        }

        return system.spawn<UdpStream>(std::move(socket), std::move(endpoint));
    }
    return "invalid device type: " + devType;
}
}
