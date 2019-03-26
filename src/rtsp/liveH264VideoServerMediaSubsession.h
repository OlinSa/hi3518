#ifndef SRC_RTSP_RTSP_SERVER_MEDIA_SUBSESSION_H
#define SRC_RTSP_RTSP_SERVER_MEDIA_SUBSESSION_H

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"

#include "OnDemandServerMediaSubsession.hh"

class LiveH264VideoServerMeidaSubsession:public OnDemandServerMediaSubsession
{
public:
    LiveH264VideoServerMeidaSubsession(UsageEnvironment& env, FramedSource *source, GetDataFunc getDataFunc, void *arg);
    ~LiveH264VideoServerMeidaSubsession();
    static LiveH264VideoServerMeidaSubsession *createNew(UsageEnvironment & env, FramedSource * source, GetDataFunc getDataFunc, void *arg);
protected:
    
    virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate);
    virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
				    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource);
    virtual char const* getAuxSDPLine(RTPSink* rtpSink,
				    FramedSource* inputSource);
    static void afterPlayingDummy(void * ptr);
    static void chkForAuxSDPLine(void * ptr);
    void chkForAuxSDPLine1();
private:
    FramedSource * source;
    char *sdpLine;
    RTPSink * dummyRTPSink;
    char done;
    GetDataFunc getDataFunc;
    void *arg;
};


#endif