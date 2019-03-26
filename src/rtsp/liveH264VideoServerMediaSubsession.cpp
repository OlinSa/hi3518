#include "liveH264VideoSource.h"
#include "liveH264VideoServerMediaSubsession.h"

LiveH264VideoServerMeidaSubsession::LiveH264VideoServerMeidaSubsession(UsageEnvironment &env, FramedSource *_source, GetDataFunc _cb, void *_arg)
    : OnDemandServerMediaSubsession(env, True), source(_source), sdpLine(NULL), dummyRTPSink(NULL), done(0), getDataFunc(_cb), arg(_arg)
{
}
LiveH264VideoServerMeidaSubsession::~LiveH264VideoServerMeidaSubsession()
{
    if (sdpLine)
    {
        free(sdpLine);
    }
}

LiveH264VideoServerMeidaSubsession *LiveH264VideoServerMeidaSubsession::createNew(UsageEnvironment &env, FramedSource *source, GetDataFunc cb, void *arg)
{
    return new LiveH264VideoServerMeidaSubsession(env, source, cb, arg);
}
FramedSource *LiveH264VideoServerMeidaSubsession::createNewStreamSource(unsigned clientSessionId,
                                                                        unsigned &estBitrate)
{
    return H264VideoStreamFramer::createNew(envir(), new LiveH264VideoSource(envir(), getDataFunc, arg));
}
RTPSink *LiveH264VideoServerMeidaSubsession::createNewRTPSink(Groupsock *rtpGroupsock,
                                                              unsigned char rtpPayloadTypeIfDynamic,
                                                              FramedSource *inputSource)
{
    return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
char const *LiveH264VideoServerMeidaSubsession::getAuxSDPLine(RTPSink *rtpSink,
                                                              FramedSource *inputSource)
{
    if (sdpLine)
        return sdpLine;
    dummyRTPSink = rtpSink;
    dummyRTPSink->startPlaying(*inputSource, 0, 0);
    chkForAuxSDPLine(this);

    envir().taskScheduler().doEventLoop(&done);
    sdpLine = strdup(rtpSink->auxSDPLine());
    dummyRTPSink->stopPlaying();
    return sdpLine;
}
void LiveH264VideoServerMeidaSubsession::afterPlayingDummy(void *ptr)
{
    LiveH264VideoServerMeidaSubsession *THIS = (LiveH264VideoServerMeidaSubsession *)ptr;
    THIS->done = 0xff;
}
void LiveH264VideoServerMeidaSubsession::chkForAuxSDPLine(void *ptr)
{
    LiveH264VideoServerMeidaSubsession *This = (LiveH264VideoServerMeidaSubsession *)ptr;

    This->chkForAuxSDPLine1();
}
void LiveH264VideoServerMeidaSubsession::chkForAuxSDPLine1()
{
    if (dummyRTPSink->auxSDPLine())
    {
        done = 0xff;
    }
    else
    {
        double delay = 1000.0 / (FRAME_PER_SEC); // ms
        int to_delay = delay * 1000;             // us
        nextTask() = envir().taskScheduler().scheduleDelayedTask(to_delay, chkForAuxSDPLine, this);
    }
}