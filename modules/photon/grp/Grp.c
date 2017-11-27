#include "photon/grp/Grp.Component.h"
#include "photon/core/Logging.h"

#define _PHOTON_FNAME "grp/Grp.c"

#ifdef PHOTON_STUB

void clearState()
{
    _photonGrp.commit = 0;
    _photonGrp.lastLogIdx = 0;
    _photonGrp.lastLogTerm = 0;
    _photonGrp.term = 0;
    _photonGrp.members.size = 0;
    _photonGrp.group.type = PhotonOptionGrpGrpIdType_None;
    _photonGrp.leader.type = PhotonOptionGrpUavIdType_None;
    _photonGrp.state = PhotonGrpMemberState_Follower;
    _photonGrp.votedFor.type = PhotonOptionGrpUavIdType_None;
}

void PhotonGrp_Init()
{
    clearState();
}

void PhotonGrp_Tick()
{
}

PhotonError PhotonGrp_SetGroupAddress(uint64_t address)
{
    PHOTON_DEBUG("SetGroupAddress (%" PRIu64 ")", address);
    return PhotonError_Ok;
}

PhotonError PhotonGrp_CreateGroup(uint64_t group, PhotonDynArrayOfGrpUavIdMaxSize10 const* members)
{
    clearState();
    _photonGrp.group.type = PhotonOptionGrpGrpIdType_Some;
    _photonGrp.group.data.someOptionGrpGrpId._1 = group;
    _photonGrp.members.size = members->size;

    PHOTON_DEBUG("CreateGroup: group(%" PRIu64 "), count(%" PRIu64 ")", group, _photonGrp.members.size);
    for(uint64_t i = 0; i < _photonGrp.members.size; ++i)
    {
        _photonGrp.members.data[i] = members->data[i];
        PHOTON_DEBUG("CreateGroup: group(%" PRIu64 "), index(%" PRIu64 "), member(%" PRIu64 ")", group, i, _photonGrp.members.data[i]);
    }
    return PhotonError_Ok;
}

PhotonError PhotonGrp_DeleteGroup(uint64_t group)
{
    PHOTON_DEBUG("DeleteGroup: group(%" PRIu64 ")", group);
    clearState();
    return PhotonError_Ok;
}

PhotonError PhotonGrp_AddMember(uint64_t group, uint64_t member, PhotonGrpReqCfgRep* rv)
{
    if (_photonGrp.group.type == PhotonOptionGrpGrpIdType_None || _photonGrp.group.data.someOptionGrpGrpId._1 != group)
        return PhotonError_InvalidDeviceId;
    if (_photonGrp.members.size >= sizeof(_photonGrp.members.data)/sizeof(_photonGrp.members.data[0]))
        return PhotonError_InvalidSize;
    _photonGrp.members.data[_photonGrp.members.size] = member;
    _photonGrp.members.size++;
    PHOTON_DEBUG("AddMember: group(%" PRIu64 "), member(%" PRIu64 ")", group, member);
    return PhotonError_Ok;
}

PhotonError PhotonGrp_RemoveMember(uint64_t group, uint64_t member, PhotonGrpReqCfgRep* rv)
{
    if (_photonGrp.group.type == PhotonOptionGrpGrpIdType_None || _photonGrp.group.data.someOptionGrpGrpId._1 != group)
        return PhotonError_InvalidDeviceId;
    if (_photonGrp.members.size == 0)
        return PhotonError_InvalidSize;
    _photonGrp.members.size--;
    PHOTON_DEBUG("RemoveMember: group(%" PRIu64 "), member(%" PRIu64 ")", group, member);
    return PhotonError_Ok;
}

PhotonError PhotonGrp_ReqVote(uint64_t term, uint64_t lastLogIdx, uint64_t lastLogTerm, PhotonGrpReqVoteRep* rv)
{
    PHOTON_DEBUG("ReqVote: term(%" PRIu64 "), lastLogIdx(%" PRIu64 "), lastLogTerm(%" PRIu64 ")", term, lastLogIdx, lastLogTerm);
    return PhotonError_Ok;
}

PhotonError PhotonGrp_ReqAppendEntry(uint64_t term, uint64_t prevLogIdx, uint64_t prevLogTerm, uint64_t leaderCommit, PhotonDynArrayOfGrpLogEntryMaxSize10 const* entries, PhotonGrpReqAppendEntryRep* rv)
{
    PHOTON_DEBUG("ReqAppendEntry: term(%" PRIu64 "), prevLogIdx(%" PRIu64 "), prevLogTerm(%" PRIu64 "), leaderCommit(%" PRIu64 ")", term, prevLogIdx, prevLogTerm, leaderCommit);
    return PhotonError_Ok;
}


#endif

#undef _PHOTON_FNAME
