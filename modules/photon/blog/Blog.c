#include "photongen/onboard/blog/Blog.Component.h"
#include "photongen/onboard/clk/Clk.Component.h"
#include "photon/core/Logging.h"

#define _PHOTON_FNAME "blog/Blog.c"

void PhotonBlog_Init()
{
    _photonBlog.pvuCmdLogEnabled = true;
    _photonBlog.tmMsgLogEnabled = true;
    _photonBlog.fwtCmdLogEnabled = true;
    PhotonBlog_HandleBeginLog();
}

void PhotonBlog_Tick()
{
}

static bool isLogEnabled(PhotonBlogMsgKind kind)
{
    switch (kind) {
    case PhotonBlogMsgKind_PvuCmd:
        return _photonBlog.pvuCmdLogEnabled;
    case PhotonBlogMsgKind_TmMsg:
        return _photonBlog.tmMsgLogEnabled;
    case PhotonBlogMsgKind_FwtCmd:
        return _photonBlog.fwtCmdLogEnabled;
    }
    return false;
}

void PhotonBlog_EnableLog(PhotonBlogMsgKind kind, bool isEnabled)
{
    switch (kind) {
    case PhotonBlogMsgKind_PvuCmd:
        _photonBlog.pvuCmdLogEnabled = isEnabled;
        break;
    case PhotonBlogMsgKind_TmMsg:
        _photonBlog.tmMsgLogEnabled = isEnabled;
        break;
    case PhotonBlogMsgKind_FwtCmd:
        _photonBlog.fwtCmdLogEnabled = isEnabled;
        break;
    }
}

#define BLOG_SEPARATOR 0x63c1

static void logMsg(PhotonBlogMsgKind kind, const void* data, size_t size)
{
    if (!isLogEnabled(kind)) {
        return;
    }
    uint8_t buf[2 + 8 + 8];
    PhotonWriter dest;
    PhotonWriter_Init(&dest, buf, sizeof(buf));
    PhotonWriter_WriteU16Be(&dest, BLOG_SEPARATOR);
    if (PhotonBlogMsgKind_Serialize(kind, &dest) != PhotonError_Ok) {
        PHOTON_WARNING("failed to write binary log kind");
        return;
    }
    if (PhotonWriter_WriteVaruint(&dest, PhotonClk_GetTickTime()) != PhotonError_Ok) {
        PHOTON_WARNING("failed to write binary log time");
        return;
    }

    PhotonBlog_HandleLogData(dest.start, dest.current - dest.start);
    PhotonBlog_HandleLogData(data, size);
}

void PhotonBlog_LogTmMsg(const void* data, size_t size)
{
    logMsg(PhotonBlogMsgKind_TmMsg, data, size);
}

void PhotonBlog_LogPvuCmd(const void* data, size_t size)
{
    logMsg(PhotonBlogMsgKind_PvuCmd, data, size);
}

void PhotonBlog_LogFwtCmd(const void* data, size_t size)
{
    logMsg(PhotonBlogMsgKind_FwtCmd, data, size);
}

PhotonError PhotonBlog_ExecCmd_EnableLog(PhotonBlogMsgKind kind, bool isEnabled)
{
    PhotonBlog_EnableLog(kind, isEnabled);
    return PhotonError_Ok;
}

#ifdef PHOTON_STUB

#include <stdio.h>
#include <time.h>
#include <assert.h>

static const char* blogPathFormat = "photon-model-" PHOTON_DEVICE_NAME "-blog_%s.pblog";

FILE* logFile = NULL;

void PhotonBlog_HandleBeginLog()
{
    time_t t = time(NULL);
    char timeStr[100];
    size_t timeLen = strftime(timeStr, sizeof(timeStr), "%Y-%m-%d_%H:%M:%S", localtime(&t));
    assert(timeLen != 0);
    char pathStr[200 + sizeof(PHOTON_DEVICE_NAME)];
    int pathLen = snprintf(pathStr, sizeof(pathStr), blogPathFormat, timeStr);
    assert(pathLen > 0);

    PHOTON_DEBUG("%s", pathStr);

    logFile = fopen(pathStr, "w");
    assert(logFile);
    fflush(logFile);
}

void PhotonBlog_HandleLogData(const void* data, size_t size)
{
    fwrite(data, 1, size, logFile);
}

#endif

#undef _PHOTON_FNAME
