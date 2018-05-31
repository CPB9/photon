#include <photon/groundcontrol/AllowUnsafeMessageType.h>
#include <photon/groundcontrol/Atoms.h>
#include <photon/groundcontrol/StreamFromString.h>
#include <photon/groundcontrol/GroundControl.h>
#include <photon/groundcontrol/DfuState.h>

#include <decode/core/Foreach.h>

#include <bmcl/String.h>
#include <bmcl/Result.h>
#include <bmcl/Logging.h>
#include <bmcl/StringView.h>
#include <bmcl/ColorStream.h>

#include <tclap/CmdLine.h>

#include <asio/io_service.hpp>

#include <caf/actor_system_config.hpp>
#include <caf/actor_system.hpp>
#include <caf/send.hpp>
#include <caf/actor_ostream.hpp>

#include <cstdint>

using namespace photon;


class DfuActor : public caf::event_based_actor {
public:
    using SetGc = caf::atom_constant<caf::atom("dfusetgc")>;
    using ExecCmd = caf::atom_constant<caf::atom("dfuexecc")>;

    DfuActor(caf::actor_config& cfg, const caf::actor& sink)
        : caf::event_based_actor(cfg)
        , _sink(sink)
    {
    }

    caf::behavior make_behavior() override
    {
        return caf::behavior{
            [this](SetGc, const caf::actor& gc) {
                _gc = gc;
            },
            [this](ExecCmd, const std::vector<std::string>& cmds) {
                if (cmds.empty()) {
                    exitWithError("no command specified");
                    return;
                }

                if (cmds[0] == "status" && cmds.size() == 1) {
                    printCommandMsg("Requesting status");
                    request(_gc, caf::infinite, RequestDfuStatus::value).then([this](const Rc<const DfuStatus>& status) {
                        printStatus(status.get());
                        quit();
                    });
                    return;
                }

                std::string joined;
                decode::foreachList(cmds, [&](const std::string& s) {
                    joined += s;
                }, [&](const std::string&) {
                    joined += ' ';
                });
                exitWithError("unknown cmd (" + joined + ")");
            },
        };
    }

    void printCommandMsg(bmcl::StringView msg)
    {
        bmcl::ColorStdOut out;
        out << "> " << bmcl::ColorAttr::Bright  << bmcl::ColorAttr::FgGreen << msg.toStdString()
                    << bmcl::ColorAttr::Reset << std::endl;
    }

    void printStatus(const DfuStatus* status)
    {
        uint64_t maxAddr = 0;
        std::size_t fwLenWidth = 0;
        std::size_t fwIdWidth = std::max(fwLenWidth, std::to_string(status->sectorDesc.size() - 1).size());
        for (const photongen::dfu::SectorDesc& desc : status->sectorDesc) {
            maxAddr = std::max(maxAddr, desc.start());
            fwLenWidth = std::max(fwLenWidth, std::to_string(desc.size()).size());
        }

        std::size_t addrWidth = 4;
        if (maxAddr > std::numeric_limits<std::uint16_t>::max()) {
            if (maxAddr > std::numeric_limits<std::uint32_t>::max()) {
                addrWidth = 16;
            } else {
                addrWidth = 8;
            }
        }

        bmcl::ColorStdOut out;

        std::size_t width = 13 + (fwIdWidth + 1) + 11 + fwLenWidth + (2 + addrWidth);
        std::string lineSep(width, '-');
        lineSep += '\n';
        out << lineSep;
        for (std::size_t i = 0; i < status->sectorDesc.size(); i++) {
            const photongen::dfu::SectorDesc& desc = status->sectorDesc[i];
            if (i == status->sectorData.currentSector()) {
                out << "| *";
            } else {
                out << "|  ";
            }
            out << std::setw(fwIdWidth) << i;

            out << " | ";
            switch (desc.kind()) {
                case photongen::dfu::SectorType::Bootloader:
                    out << " bootloader";
                    break;
                case photongen::dfu::SectorType::SectorData:
                    out << "sector data";
                    break;
                case photongen::dfu::SectorType::Firmware:
                    out << "   firmware";
                    break;
                case photongen::dfu::SectorType::UserData:
                    out << "  user data";
                    break;
            }

            out << " | 0x";
            out << std::hex << std::setw(addrWidth) << std::setfill('0') << desc.start();

            out << " | ";
            out << std::dec << std::setw(fwLenWidth) << std::setfill(' ') << desc.size();

            out << " |";
            out << std::endl;
            out << lineSep;
        }
    }

    void exitWithError(bmcl::StringView error)
    {
        bmcl::ColorStdOut out;
        out << bmcl::ColorAttr::FgRed << "Error" << bmcl::ColorAttr::Reset << ": " << error.toStdString() << std::endl;
        quit();
    }

    const char* name() const override
    {
        return "dfuactor";
    }

    void on_exit() override
    {
        send_exit(_gc, caf::exit_reason::normal);
        send_exit(_sink, caf::exit_reason::normal);
        destroy(_gc);
        destroy(_sink);
    }

private:
    //TODO: set
    caf::actor _gc;
    caf::actor _sink;
};

const char* usage =
"Examples:\n"
;

int main(int argc, char** argv)
{
    TCLAP::CmdLine cmdLine(usage);
    TCLAP::ValueArg<std::string> deviceArg("d", "device", "Device", true, "udp,127.0.0.1,6666", "device string");
    TCLAP::ValueArg<uint64_t> srcArg("m", "mcc-id", "Mcc id", false, 1, "number");
    TCLAP::ValueArg<uint64_t> destArg("u", "uav-id", "Uav id", false, 2, "number");
    TCLAP::UnlabeledMultiArg<std::string> cmdArg("cmd", "Dfu commands", true, "command list");

    cmdLine.add(&deviceArg);
    cmdLine.add(&srcArg);
    cmdLine.add(&destArg);
    cmdLine.add(&cmdArg);
    cmdLine.parse(argc, argv);

    asio::io_service ioService;
    caf::actor_system_config cfg;
    caf::actor_system system(cfg);

    auto rv = spawnActorFromDeviceString(system, ioService, deviceArg.getValue());
    if (rv.isErr()) {
        BMCL_CRITICAL() << rv.unwrapErr();
        return -1;
    }

    caf::actor& stream = rv.unwrap();
    caf::actor dfu = system.spawn<DfuActor>(stream);
    caf::actor gc = system.spawn<GroundControl>(srcArg.getValue(), destArg.getValue(), stream, dfu);
    caf::anon_send(dfu, DfuActor::SetGc::value, gc);
    caf::anon_send(stream, SetStreamDestAtom::value, gc);

    caf::anon_send(stream, photon::StartAtom::value);
    caf::anon_send(gc, photon::StartAtom::value);

    caf::anon_send(dfu, DfuActor::ExecCmd::value, cmdArg.getValue());
    system.await_all_actors_done();
    return 0;
}


