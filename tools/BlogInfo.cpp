#include <photon/groundcontrol/ProjectUpdate.h>
#include <photongen/groundcontrol/blog/MsgKind.hpp>
#include <decode/core/Utils.h>

#include <bmcl/Bytes.h>
#include <bmcl/StringView.h>
#include <bmcl/MemReader.h>
#include <bmcl/Result.h>
#include <bmcl/FileUtils.h>
#include <bmcl/Buffer.h>
#include <bmcl/Logging.h>

#include <tclap/CmdLine.h>

#include <cstring>
#include <cstdlib>

struct BlogMsg {
    BlogMsg(std::size_t offset, photon::OnboardTime time, bmcl::Bytes data)
        : offset(offset)
        , time(time)
        , data(data)
    {
    }

    std::size_t offset;
    photon::OnboardTime time;
    bmcl::Bytes data;
};

template <typename B>
class BlogParser {
public:

    B& base();

    bool parse(bmcl::Bytes data);

    bool handleDeviceName(std::size_t offset, bmcl::StringView name);
    bool handleSerializedProject(std::size_t offset, bmcl::Bytes projectData);
    bool handleBrokenPart(std::size_t offset, bmcl::Bytes data);
    bool handlePvuCmd(const BlogMsg& msg);
    bool handleTmMsg(const BlogMsg& msg);
    bool handleFwtCmd(const BlogMsg& msg);
};

template <typename B>
inline bool BlogParser<B>::parse(bmcl::Bytes data)
{
    static char magicPrefix[4] = {'b', 'l', 'o', 'g'};
    bmcl::MemReader reader(data);
    if (reader.sizeLeft() < sizeof(magicPrefix)) {
        return false;
    }
    if (std::memcmp(magicPrefix, reader.current(), sizeof(magicPrefix)) != 0) {
        return false;
    }
    reader.skip(sizeof(magicPrefix));

    auto nameRv = decode::deserializeString(&reader);
    if (nameRv.isErr()) {
        return false;
    }
    if (!base().handleDeviceName(reader.current() - reader.start(), nameRv.unwrap())) {
        return true;
    }

    uint64_t projectSize = 0;
    if (!reader.readVarUint(&projectSize)) {
        return false;
    }
    if (reader.sizeLeft() < projectSize) {
        return false;
    }
    if (!base().handleSerializedProject(reader.current() - reader.start(), bmcl::Bytes(reader.current(), projectSize))) {
        return true;
    }
    reader.skip(projectSize);

    constexpr uint8_t sepFirstPart = 0x63;
    constexpr uint8_t sepSecondPart = 0xc1;
    const uint8_t* lastOk = reader.current();
    while (reader.readableSize() != 0) {
        const uint8_t* start = reader.current();
        const uint8_t* current = start;
        const uint8_t* end = reader.end();
        current = std::find(current, end, sepFirstPart);
        const uint8_t* csStart = current;
        if (current == end) {
            base().handleBrokenPart(reader.start() - lastOk, bmcl::Bytes(lastOk, current));
            return true;
        }
        current++;
        if (current == end) {
            base().handleBrokenPart(reader.start() - lastOk, bmcl::Bytes(lastOk, current));
            return true;
        }
        if (*current != sepSecondPart) {
            reader.skip(current - start);
            continue;
        }
        current++;
        reader.skip(current - start);

        photongen::blog::MsgKind kind;

        photon::CoderState state(photon::OnboardTime::now());
        if (!photongenDeserializeBlogMsgKind(&kind, &reader, &state)) {
            continue;
        }
        uint64_t time = 0;
        if (!reader.readVarUint(&time)) {
            continue;
        }
        uint64_t size = 0;
        if (!reader.readVarUint(&size)) {
            continue;
        }
        if (reader.readableSize() < size) {
            continue;
        }
        bmcl::Bytes chunk(reader.current(), size);
        std::size_t offset = reader.current() - reader.start();
        BlogMsg msg(offset, photon::OnboardTime(time), chunk);
        switch (kind) {
        case photongen::blog::MsgKind::PvuCmd:
            base().handlePvuCmd(msg);
            break;
        case photongen::blog::MsgKind::TmMsg:
            base().handleTmMsg(msg);
            break;
        case photongen::blog::MsgKind::FwtCmd:
            base().handleFwtCmd(msg);
            break;
        }
        reader.skip(size);
        if (lastOk != start) {
            base().handleBrokenPart(csStart - lastOk, bmcl::Bytes(lastOk, csStart));
        }

        lastOk = reader.current();
    }

    return true;
}

template <typename B>
inline B& BlogParser<B>::base()
{
    return *static_cast<B*>(this);
}

template <typename B>
inline bool BlogParser<B>::handleDeviceName(std::size_t offset, bmcl::StringView name)
{
    (void)offset;
    (void)name;
    return true;
}

template <typename B>
inline bool BlogParser<B>::handleSerializedProject(std::size_t offset, bmcl::Bytes data)
{
    (void)offset;
    (void)data;
    return true;
}

template <typename B>
inline bool BlogParser<B>::handleBrokenPart(std::size_t offset, bmcl::Bytes data)
{
    (void)offset;
    (void)data;
    return true;
}

template <typename B>
inline bool BlogParser<B>::handlePvuCmd(const BlogMsg& msg)
{
    (void)msg;
    return true;
}

template <typename B>
inline bool BlogParser<B>::handleTmMsg(const BlogMsg& msg)
{
    (void)msg;
    return true;
}

template <typename B>
inline bool BlogParser<B>::handleFwtCmd(const BlogMsg& msg)
{
    (void)msg;
    return true;
}

class BlogInfo : public BlogParser<BlogInfo> {
public:
    struct KindInfo {
        KindInfo()
            : num(0)
            , size(0)
        {
        }

        void add(const BlogMsg& msg)
        {
            num++;
            size += msg.data.size();
        }

        std::size_t num;
        std::size_t size;
    };

    BlogInfo()
    {
    }

    bool handleDeviceName(std::size_t offset, bmcl::StringView name)
    {
        std::cout << "Device name: " << name.toStdString() << std::endl;
        return true;
    }

    bool handleSerializedProject(std::size_t offset, bmcl::Bytes projectData)
    {
        std::cout << "Project size: " << projectData.size() << " bytes" << std::endl;
        return true;
    }

    bool handleBrokenPart(std::size_t offset, bmcl::Bytes data)
    {
        brokenInfo.num++;
        brokenInfo.size += data.size();
        return true;
    }

    bool handlePvuCmd(const BlogMsg& msg)
    {
        pvuCmdInfo.add(msg);
        return true;
    }

    bool handleTmMsg(const BlogMsg& msg)
    {
        tmMsgInfo.add(msg);
        return true;
    }

    bool handleFwtCmd(const BlogMsg& msg)
    {
        fwtCmdInfo.add(msg);
        return true;
    }

    KindInfo brokenInfo;
    KindInfo pvuCmdInfo;
    KindInfo tmMsgInfo;
    KindInfo fwtCmdInfo;
};

std::ostream& operator<<(std::ostream& os, const BlogInfo::KindInfo& info)
{
    os << info.num << " (" << info.size << " bytes)";
    return os;
}

const char* usage = "photon-blog-info path/to/logfile.pblog";

int main(int argc, char** argv)
{
    TCLAP::CmdLine cmdLine(usage);
    TCLAP::UnlabeledValueArg<std::string> pathArg("path", "Path to file", true, "", "path");

    cmdLine.add(&pathArg);
    cmdLine.parse(argc, argv);

    auto fileRv = bmcl::readFileIntoBuffer(pathArg.getValue().c_str());
    if (fileRv.isErr()) {
        BMCL_CRITICAL() << "failed to read file";
        return 1;
    }

    std::cout << "File size: " << fileRv.unwrap().size() << " bytes" << std::endl;

    BlogInfo info;
    if (!info.parse(fileRv.unwrap())) {
        BMCL_CRITICAL() << "failed to parse blog";
        return 1;
    }

    std::cout << "Pvu commands: " << info.pvuCmdInfo << std::endl;
    std::cout << "Tm messages: " << info.tmMsgInfo << std::endl;
    std::cout << "Fwt commands: " << info.fwtCmdInfo << std::endl;
    std::cout << "Broken segments: " << info.brokenInfo <<  std::endl;

    return 0;
}
