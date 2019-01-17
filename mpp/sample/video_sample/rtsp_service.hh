#ifndef LIVE_RTSPSERVICE_HH
#define LIVE_RTSPSERVICE_HH

#include <unistd.h>
#include "BasicUsageEnvironment.hh"
#include "UsageEnvironment.hh"
#include "live_H264VideoSource.hh"
#include "live_H264VideoServerMediaSubsession.hh"
#include "liveMedia_version.hh"

class RtspService
{
public:
    RtspService();
    virtual ~RtspService();
    bool Init();
    ssize_t PutMemInBuffer(const void *buf, size_t count);
private:
    UsageEnvironment *env;
    TaskScheduler *scheduler;
    RTSPServer *rtspServer;
    int pipe_fd;
};

#endif