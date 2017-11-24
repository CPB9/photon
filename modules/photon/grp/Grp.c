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
