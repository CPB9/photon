#include "photon/grp/Grp.Component.h"
#include "photon/core/Logging.h"

#define _PHOTON_FNAME "grp/Grp.c"

#ifdef PHOTON_STUB

void PhotonGrp_Init()
{
}

void PhotonGrp_Tick()
{
}

PhotonError PhotonGrp_SetGroupAddress(uint64_t address)
{
    PHOTON_INFO("SetGroupAddress (%" PRIu64 ")", address);
    return PhotonError_Ok;
}

PhotonError PhotonGrp_CreateGroup(uint64_t group, PhotonDynArrayOfGrpUavIdMaxSize10 const* members)
{
    PHOTON_INFO("CreateGroup: group(%" PRIu64 "), count(%" PRIu64 ")", group, members->size);
    for(uint64_t i = 0; i < members->size; ++i)
    {
        PHOTON_INFO("CreateGroup: group(%" PRIu64 "), index(%" PRIu64 "), member(%" PRIu64 ")", group, i, members->data[i]);
    }
    return PhotonError_Ok;
}

PhotonError PhotonGrp_DeleteGroup(uint64_t group)
{
    PHOTON_INFO("DeleteGroup: group(%" PRIu64 ")", group);
    return PhotonError_Ok;
}

PhotonError PhotonGrp_AddMember(uint64_t group, uint64_t member, PhotonGrpReqCfgRep* rv)
{
    PHOTON_INFO("AddMember: group(%" PRIu64 "), member(%" PRIu64 ")", group, member);
    return PhotonError_Ok;
}

PhotonError PhotonGrp_RemoveMember(uint64_t group, uint64_t member, PhotonGrpReqCfgRep* rv)
{
    PHOTON_INFO("RemoveMember: group(%" PRIu64 "), member(%" PRIu64 ")", group, member);
    return PhotonError_Ok;
}

PhotonError PhotonGrp_ReqVote(uint64_t term, uint64_t lastLogIdx, uint64_t lastLogTerm, PhotonGrpReqVoteRep* rv)
{
    PHOTON_INFO("ReqVote: term(%" PRIu64 "), lastLogIdx(%" PRIu64 "), lastLogTerm(%" PRIu64 ")", term, lastLogIdx, lastLogTerm);
    return PhotonError_Ok;
}

PhotonError PhotonGrp_ReqAppendEntry(uint64_t term, uint64_t prevLogIdx, uint64_t prevLogTerm, uint64_t leaderCommit, PhotonDynArrayOfGrpLogEntryMaxSize10 const* entries, PhotonGrpReqAppendEntryRep* rv)
{
    PHOTON_INFO("ReqAppendEntry: term(%" PRIu64 "), prevLogIdx(%" PRIu64 "), prevLogTerm(%" PRIu64 "), leaderCommit(%" PRIu64 ")", term, prevLogIdx, prevLogTerm, leaderCommit);
    return PhotonError_Ok;
}


#endif

#undef _PHOTON_FNAME
