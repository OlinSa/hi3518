#ifndef LIVE_H264VIDEOSERVERMEDIASUBSESSION_HH
#define LIVE_H264VIDEOSERVERMEDIASUBSESSION_HH

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"

#include "OnDemandServerMediaSubsession.hh"
#include "live_H264VideoSource.hh"

class LIVE_H264VideoServerMediaSubsession:public OnDemandServerMediaSubsession
{
public:
    LIVE_H264VideoServerMediaSubsession(UsageEnvironment& env, FramedSource *source, const std::string & filename);
    ~LIVE_H264VideoServerMediaSubsession();

    //overwrite virtual function
    virtual char const* getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource);
  	virtual RTPSink * createNewRTPSink(Groupsock * rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource * inputSource);
    virtual FramedSource * createNewStreamSource(unsigned clientSessionId, unsigned & estBitrate);

    static LIVE_H264VideoServerMediaSubsession *createNew(UsageEnvironment & env, FramedSource * source, const std::string &filename);
    static void afterPlayingDummy(void * ptr);
    static void chkForAuxSDPLine(void * ptr);
    void chkForAuxSDPLine1();
private:
    std::string filename;
    FramedSource * m_pSource;
    char *m_pSDPLine;
    RTPSink * m_pDummyRTPSink;
    char m_done;
};


#endif
