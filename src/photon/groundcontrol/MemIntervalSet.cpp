/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/groundcontrol/MemIntervalSet.h"

namespace photon {

IntervalComparison MemInterval::mergeIntoIfIntersects(MemInterval* other)
{
    if (start() <= other->start()) {
        if (end() < other->start()) {
            return IntervalComparison::Before;
        }
        if (end() <= other->end()) {
            other->_start  = _start;
            return IntervalComparison::Intersects;
        }
        *other = *this;
        return IntervalComparison::Intersects;
    }
    if (start() > other->end()) {
        return IntervalComparison::After;
    }
    if (end() <= other->end()) {
        return IntervalComparison::Intersects;
    }
    other->_end = end();
    return IntervalComparison::Intersects;
}

void MemIntervalSet::add(MemInterval interval)
{
    if (_intervals.size() == 0) {
        _intervals.push_back(interval);
        return;
    }

    auto begin = _intervals.begin();
    auto it = begin;
    auto end = _intervals.end();

    while (it < end) {
        IntervalComparison rv =  interval.mergeIntoIfIntersects(&(*it));
        if (rv == IntervalComparison::Before) {
            _intervals.insert(it, interval);
            return;
        } else if (rv == IntervalComparison::Intersects) {
            auto merged = it;
            it++;
            auto removeBegin = it;
            while (it < end) {
                rv = it->mergeIntoIfIntersects(&(*merged));
                BMCL_ASSERT(rv != IntervalComparison::Before);
                if (rv == IntervalComparison::After) {
                    break;
                }
                it++;
            }
            _intervals.erase(removeBegin, it);
            return;
        } else {
            it++;
            continue;
        }
    }
    _intervals.insert(it, interval);
}

//TODO: calculate on addition
std::size_t MemIntervalSet::dataSize() const
{
    std::size_t s = 0;
    for (const MemInterval& i : _intervals) {
        s += i.size();
    }
    return s;
}
}
