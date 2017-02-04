#pragma once

#include "photon/Config.h"

#include <bmcl/ArrayView.h>
#include <bmcl/Buffer.h>
#include <bmcl/Option.h>

#include <array>

namespace bmcl {
class MemWriter;
class MemReader;
}

namespace photon {

struct MemInterval {
    enum MergeResult {
        Before,
        Merged,
        After
    };

    MemInterval()
    {
    }

    MemInterval(std::size_t start, std::size_t end)
        : _start(start)
        , _end(end)
    {
        BMCL_ASSERT(start <= end);
    }

    std::size_t start() const
    {
        return _start;
    }

    std::size_t end() const
    {
        return _end;
    }

    MergeResult mergeIntoIfIntersects(MemInterval* other)
    {
        if (start() <= other->start()) {
            if (end() < other->start()) {
                return Before;
            }
            if (end() <= other->end()) {
                other->_start  = _start;
                return Merged;
            }
            *other = *this;
            return Merged;
        }
        if (start() > other->end()) {
            return After;
        }
        if (end() <= other->end()) {
            return Merged;
        }
        other->_end = end();
        return Merged;
    }

    std::size_t size() const
    {
        return _end - _start;
    }

    bool operator==(const MemInterval& other) const
    {
        return _start == other._start && _end == other._end;
    }

private:
    std::size_t _start;
    std::size_t _end;
};

class MemIntervalSet {
public:
    MemIntervalSet()
    {
    }

    MemIntervalSet(const std::vector<MemInterval>& vec)
        : _chunks(std::move(vec))
    {
    }

    void add(MemInterval chunk)
    {
        if (_chunks.size() == 0) {
            _chunks.push_back(chunk);
            return;
        }

        auto begin = _chunks.begin();
        auto it = begin;
        auto end = _chunks.end();

        while (it < end) {
            MemInterval::MergeResult rv =  chunk.mergeIntoIfIntersects(&(*it));
            if (rv == MemInterval::Before) {
                _chunks.insert(it, chunk);
                return;
            } else if (rv == MemInterval::Merged) {
                auto merged = it;
                it++;
                auto removeBegin = it;
                while (it < end) {
                    rv = it->mergeIntoIfIntersects(&(*merged));
                    BMCL_ASSERT(rv != MemInterval::Before);
                    if (rv == MemInterval::After) {
                        break;
                    }
                    it++;
                }
                _chunks.erase(removeBegin, it);
                return;
            } else {
                it++;
                continue;
            }
        }
        _chunks.insert(it, chunk);
    }

    void reset()
    {
        _chunks.clear();
    }

    const std::vector<MemInterval>& chunks() const
    {
        return _chunks;
    }

private:
    std::vector<MemInterval> _chunks;
};

struct StartCmdRndGen;

class FwtState {
public:
    FwtState();
    ~FwtState();

    void startDownload();

    bool isTransfering();
    void acceptResponse(bmcl::Bytes packet);
    bmcl::Option<bmcl::Bytes> nextPacket();

private:
    bmcl::Bytes encodePacket(void (FwtState::*gen)(bmcl::MemWriter*));

    void acceptChunkResponse(bmcl::MemReader* src);
    void acceptHashResponse(bmcl::MemReader* src);
    void acceptStartResponse(bmcl::MemReader* src);
    void acceptStopResponse(bmcl::MemReader* src);

    void genHashCmd(bmcl::MemWriter* dest);
    void genChunkCmd(bmcl::MemWriter* dest);
    void genStartCmd(bmcl::MemWriter* dest);
    void genStopCmd(bmcl::MemWriter* dest);

    bmcl::Option<std::array<uint8_t, 64>> _hash;

    MemIntervalSet _acceptedChunks;
    std::vector<MemInterval> _requestedChunks;
    bmcl::Buffer _desc;

    bool _isTransfering;
    bool _hasStartCommandPassed;
    bool _hasDownloaded;
    std::unique_ptr<StartCmdRndGen> _startCmdState;
    uint8_t _temp[20];
};
}
