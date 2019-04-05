#include "UiTest.h"

#include <photon/groundcontrol/SerialStream.h>
#include <photon/groundcontrol/AllowUnsafeMessageType.h>

#include <bmcl/Logging.h>

#include <tclap/CmdLine.h>

using namespace photon;

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);

using namespace decode;

int main(int argc, char** argv)
{
    TCLAP::CmdLine cmdLine("SerialTest");
    TCLAP::ValueArg<std::string> serialArg("s", "serial", "Read from serial", true, "", "serial device");
    TCLAP::ValueArg<unsigned> baudArg("b", "baud", "Serial baud rate", false, 9600, "baud rate");
    TCLAP::ValueArg<uint64_t> srcArg("m", "mcc-id", "Mcc id", false, 1, "number");
    TCLAP::ValueArg<uint64_t> destArg("u", "uav-id", "Uav id", false, 2, "number");

    cmdLine.add(&serialArg);
    cmdLine.add(&baudArg);
    cmdLine.add(&srcArg);
    cmdLine.add(&destArg);
    cmdLine.parse(argc, argv);

    asio::io_context ioService;
    asio::serial_port serial(ioService);

    asio::error_code err;
    serial.open(serialArg.getValue(), err);
    if (err) {
        BMCL_CRITICAL() << "could not open serial port: " << err.message();
        return err.value();
    }

    serial.set_option(asio::serial_port::baud_rate(baudArg.getValue()), err);
    if (err) {
        BMCL_CRITICAL() << "could not set baud rate " << baudArg.getValue() << ": " << err.message();
        return err.value();
    }


    return runUiTest<SerialStream>(argc, argv, srcArg.getValue(), destArg.getValue(), std::move(serial));
}
