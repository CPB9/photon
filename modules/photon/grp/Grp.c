#include "photongen/onboard/grp/Grp.Component.h"
#include "photon/core/Logging.h"

#define _PHOTON_FNAME "grp/Grp.c"

#ifdef PHOTON_STUB

bool isSameGroup(uint64_t group)
{
    return (_photonGrp.group.type != PhotonOptionGrpGrpIdType_None && _photonGrp.group.data.someOptionGrpGrpId._1 == group);
}

bool hasGroup()
{
    return (_photonGrp.group.type != PhotonOptionGrpGrpIdType_None);
}

bool isMe(uint64_t member)
{
    return PhotonExc_SelfAddress() == member;
}

bool isLeader()
{
    return (_photonGrp.group.type != PhotonOptionGrpGrpIdType_None
         && _photonGrp.leader.type == PhotonOptionGrpUavIdType_Some
         && isMe(_photonGrp.leader.data.someOptionGrpUavId._1));
}

bool PhotonGrp_IsPacketForMe(uint64_t dest)
{
    return isMe(dest) || isSameGroup(dest);
}

void addMember(uint64_t member)
{
    for (uint64_t i = 0; i < _photonGrp.members.size; ++i)
    {
        if (_photonGrp.members.data[i] == member)
            return;
    }
    _photonGrp.members.data[_photonGrp.members.size] = member;
    ++_photonGrp.members.size;
}

void removeMember(uint64_t member)
{
    bool deleted = false;
    for (uint64_t i = 0; i < _photonGrp.members.size; ++i)
    {
        if (deleted)
        {
            _photonGrp.members.data[i - 1] = _photonGrp.members.data[i];
            continue;
        }
        if (_photonGrp.members.data[i] == member)
        {
            deleted = true;
        }
    }

    if (deleted)
        _photonGrp.members.size--;
}

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
    _photonGrp.timeouts.ping = 0;
    _photonGrp.timeouts.election = 0;
    _photonGrp.timeouts.election_rand = 0;
    _photonGrp.timeout_elapsed = 0;
}

void PhotonGrp_Init()
{
    clearState();
}

void PhotonGrp_Tick()
{
}

PhotonError PhotonGrp_ExecCmd_ShareTest(uint64_t group, bool enable)
{
    return PhotonError_Ok;
}

PhotonError PhotonGrp_ExecCmd_ShareMsgs(uint64_t group, uint64_t from, const PhotonDynArrayOfU8MaxSize500* cmd)
{
    return PhotonError_Ok;
}

PhotonError PhotonGrp_ExecCmd_SetTimeouts(uint64_t group, uint64_t ping, uint64_t lost)
{
    if (!isSameGroup(group))
        return PhotonError_InvalidDeviceId;

    _photonGrp.timeouts.ping = ping;
    _photonGrp.timeouts.election = ping*lost;
    PHOTON_INFO("SetTimeouts: group(%" PRIu64 "), ping(%" PRIu64 "), lost(" PRIu64 ")", group, ping, lost);
    return PhotonError_Ok;
}

PhotonError PhotonGrp_ExecCmd_CreateGroup(uint64_t group, PhotonDynArrayOfGrpUavIdMaxSize10 const* members)
{
    if (hasGroup())
        return PhotonError_InvalidDeviceId;

    clearState();
    _photonGrp.group.type = PhotonOptionGrpGrpIdType_Some;
    _photonGrp.group.data.someOptionGrpGrpId._1 = group;
    _photonGrp.members.size = members->size;

    PHOTON_INFO("CreateGroup: group(%" PRIu64 "), count(%" PRIu64 ")", group, _photonGrp.members.size);
    for(uint64_t i = 0; i < _photonGrp.members.size; ++i)
    {
        _photonGrp.members.data[i] = members->data[i];
        PHOTON_INFO("CreateGroup: group(%" PRIu64 "), index(%" PRIu64 "), member(%" PRIu64 ")", group, i, _photonGrp.members.data[i]);
    }

    PhotonTm_SetStatusEnabled(PHOTON_GRP_COMPONENT_ID, PHOTON_GRP_STATUS_ALL_ID, true);
    PhotonTm_SetStatusEnabled(PHOTON_GRP_COMPONENT_ID, PHOTON_GRP_STATUS_LEADER_ID, true);
    PhotonTm_SetStatusEnabled(PHOTON_GRP_COMPONENT_ID, PHOTON_GRP_STATUS_MEMBERS_ID, true);
    PhotonTm_SetStatusEnabled(PHOTON_GRP_COMPONENT_ID, PHOTON_GRP_STATUS_ELECT_ID, true);

    return PhotonError_Ok;
}

PhotonError PhotonGrp_ExecCmd_DeleteGroup(uint64_t group)
{
    if (!isSameGroup(group))
        return PhotonError_InvalidDeviceId;

    PHOTON_INFO("DeleteGroup: group(%" PRIu64 ")", group);
    clearState();

    PhotonTm_SetStatusEnabled(PHOTON_GRP_COMPONENT_ID, PHOTON_GRP_STATUS_ALL_ID, false);
    PhotonTm_SetStatusEnabled(PHOTON_GRP_COMPONENT_ID, PHOTON_GRP_STATUS_LEADER_ID, true);
    PhotonTm_SetStatusEnabled(PHOTON_GRP_COMPONENT_ID, PHOTON_GRP_STATUS_MEMBERS_ID, false);
    PhotonTm_SetStatusEnabled(PHOTON_GRP_COMPONENT_ID, PHOTON_GRP_STATUS_ELECT_ID, false);

    return PhotonError_Ok;
}

PhotonError PhotonGrp_ExecCmd_AddMember(uint64_t group, uint64_t member, PhotonGrpReqCfgRep* rv)
{
    if (!isSameGroup(group))
        return PhotonError_InvalidDeviceId;

    addMember(member);
    PHOTON_INFO("AddMember: group(%" PRIu64 "), member(%" PRIu64 ")", group, member);
    return PhotonError_Ok;
}

PhotonError PhotonGrp_ExecCmd_RemoveMember(uint64_t group, uint64_t member, PhotonGrpReqCfgRep* rv)
{
    PHOTON_INFO("RemoveMember: group(%" PRIu64 "), member(%" PRIu64 ")", group, member);

    if (!isSameGroup(group))
        return PhotonError_InvalidDeviceId;

    if (_photonGrp.members.size == 0)
        return PhotonError_InvalidSize;

    removeMember(member);
    return PhotonError_Ok;
}

PhotonError PhotonGrp_ExecCmd_JoinGroup(uint64_t group, PhotonDynArrayOfGrpUavIdMaxSize10 const* members, PhotonGrpReqCfgRep* rv)
{
    PHOTON_INFO("JoinGroup: group(%" PRIu64 "), members(%" PRIu64 ")", group, members->size);

    if (isSameGroup(group))
        return PhotonError_InvalidDeviceId;

    clearState();
    _photonGrp.group.type = PhotonOptionGrpGrpIdType_Some;
    _photonGrp.group.data.someOptionGrpGrpId._1 = group;
    _photonGrp.members.size = 0;

    for(uint64_t i = 0; i < members->size; ++i)
    {
        addMember(members->data[i]);
    }
    addMember(PhotonExc_SelfAddress());
    return PhotonError_Ok;
}

PhotonError PhotonGrp_ExecCmd_Execute(uint64_t group, const PhotonDynArrayOfU8MaxSize500* cmd, PhotonGrpReqCfgRep* rv)
{
    return PhotonError_Ok;
}

PhotonError PhotonGrp_ExecCmd_ReqVote(uint64_t group, uint64_t term, uint64_t lastLogIdx, uint64_t lastLogTerm, bool isPre)
{
    if (!isSameGroup(group))
        return PhotonError_InvalidDeviceId;

    PHOTON_INFO("ReqVote: group(%" PRIu64 "), term(%" PRIu64 "), lastLogIdx(%" PRIu64 "), lastLogTerm(%" PRIu64 ")", group, term, lastLogIdx, lastLogTerm);
    return PhotonError_Ok;
}

PhotonError PhotonGrp_ExecCmd_ReqAppendEntry(uint64_t group, uint64_t term, uint64_t prevLogIdx, uint64_t prevLogTerm, uint64_t leaderCommit, uint64_t lastCfgSeen, PhotonDynArrayOfGrpLogEntryMaxSize10 const* entries)
{
    if (!isSameGroup(group))
        return PhotonError_InvalidDeviceId;

    PHOTON_INFO("ReqAppendEntry: group(%" PRIu64 "), term(%" PRIu64 "), prevLogIdx(%" PRIu64 "), prevLogTerm(%" PRIu64 "), leaderCommit(%" PRIu64 ")", group, term, prevLogIdx, prevLogTerm, leaderCommit);
    return PhotonError_Ok;
}

PhotonError PhotonGrp_ExecCmd_ReqVoteRep(uint64_t group, uint64_t term, PhotonGrpVote vote)
{
    if (!isSameGroup(group))
        return PhotonError_InvalidDeviceId;

    PHOTON_INFO("ReqVoteRep: group(%" PRIu64 "), term(%" PRIu64 "), lastLogIdx(%" PRIu64 "), lastLogTerm(%" PRIu64 ")", group, term, vote);
    return PhotonError_Ok;
}

PhotonError PhotonGrp_ExecCmd_ReqAppendEntryRep(uint64_t group, uint64_t term, bool success, uint64_t currentIdx)
{
    if (!isSameGroup(group))
        return PhotonError_InvalidDeviceId;

    PHOTON_INFO("ReqAppendEntryRep: group(%" PRIu64 "), term(%" PRIu64 "), success(%" PRIu64 "), currentIdx(%" PRIu64 ")", group, term, success, currentIdx);
    return PhotonError_Ok;
}

#endif

#undef _PHOTON_FNAME
