#include "photon/gc/Exchange.h"
#include "photon/gc/Crc.h"

#include <bmcl/Endian.h>

namespace photon {

constexpr uint16_t separator = 0x9c3e;
constexpr uint8_t firstSepPart = (separator & 0xf0) >> 8;
constexpr uint8_t secondSepPart = separator & 0x0f;
constexpr std::size_t bufSize = 65536 * 4;

Exchange::Exchange()
    : _assemblyState(PacketAssemblyState::None)
    , _input(bufSize)
    , _output(bufSize)
{
}

Exchange::~Exchange()
{
}

void Exchange::acceptInput(bmcl::Bytes input)
{
    if (input.size() > _input.writableSize()) {
        //TODO: handle input buffer overflow
        _assemblyState = PacketAssemblyState::None;
    }
    _input.write(input);
    processInput();
}

std::size_t Exchange::genOutput(void* dest, std::size_t size)
{
    size = std::min(size, _output.readableSize());
    _output.read(dest, size);
    return size;
}

bool Exchange::findSepAndSkip()
{

    bmcl::RingBuffer::Chunks chunks = _input.readableChunks();

    const uint8_t* it = chunks.first.begin();
    const uint8_t* next;

    while (true) {
        it = std::find(it, chunks.first.end(), firstSepPart);
        if (it == chunks.first.end()) {
            break;
        }
        next = it + 1;
        if (next == chunks.first.end()) {
            if (!chunks.second.isEmpty() && chunks.second[0] == secondSepPart) {
                handleJunk(chunks.first.sliceFromBack(1));
                _input.erase(it - chunks.first.begin() + 2);
                return true;
            }
            break;
        }
        if (*next == secondSepPart) {
            handleJunk(bmcl::Bytes(chunks.first.data(), it));
            _input.erase(it - chunks.first.begin() + 2);
            return true;
        }
    }

    it = chunks.second.begin();
    while (true) {
        it = std::find(it, chunks.second.end(), firstSepPart);
        next = it + 1;
        if (next >= chunks.second.end()) {
            handleJunk(chunks.first);
            handleJunk(chunks.second);
            _input.clear();
            return false;
        }
        if (*next == secondSepPart) {
            handleJunk(chunks.first);
            handleJunk(bmcl::Bytes(chunks.second.data(), it));
            _input.erase(chunks.first.size() + (it - chunks.second.begin()) + 2);
            return true;
        }
    }
}

void Exchange::processInput()
{
    while (true) {
        switch (_assemblyState) {
        case PacketAssemblyState::None:
            if (_input.readableSize() < 2) {
                return;
            }
            if (!findSepAndSkip()) {
                return;
            }
            _assemblyState = PacketAssemblyState::SepSkipped;
        case PacketAssemblyState::SepSkipped:
            if (_input.readableSize() < 2) {
                return;
            }
            _input.read(&_packetSize, 2);
            _packetSize = le16toh(_packetSize) + 2;
            _assemblyState = PacketAssemblyState::SizeFound;
        case PacketAssemblyState::SizeFound:
            if (_input.readableSize() < (_packetSize)) {
                return;
            }
            //TODO: remove new
            uint8_t* packet = new uint8_t[_packetSize];
            _input.peek(packet, _packetSize);
            Crc16 crc;
            crc.update(packet, _packetSize - 2);
            if (crc.get() != le16dec(packet + _packetSize - 2)) {
                handleJunk(bmcl::Bytes(packet, _packetSize));
            } else {
                handleIncoming(bmcl::Bytes(packet + 2, _packetSize - 4));
            }
            delete [] packet;

            _input.erase(_packetSize);
            _assemblyState = PacketAssemblyState::None;
        }
    }
}

void Exchange::encodePacket(bmcl::Bytes data)
{
    //TODO: check max size
    Crc16 crc;
    uint16_t size = data.size();
    uint8_t firstSizePart = size & 0x0f;
    uint8_t secondSizePart = (size & 0xf0) >> 8;
    crc.update(firstSizePart);
    crc.update(secondSizePart);
    uint8_t sepAndSize[4] = {firstSepPart, secondSepPart, firstSizePart, secondSizePart};
    _output.write(sepAndSize, 4);
    _output.write(data);
    crc.update(data.data(), data.size());
    _output.writeUint16Le(crc.get());
}
}
