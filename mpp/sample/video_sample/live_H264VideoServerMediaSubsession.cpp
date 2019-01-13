#include "live_H264VideoServerMediaSubsession.hh"

LIVE_H264VideoServerMediaSubsession::LIVE_H264VideoServerMediaSubsession(UsageEnvironment & env, FramedSource * source, const std::string& filename)
	:OnDemandServerMediaSubsession(env, True)
{
	this->filename = filename;
	m_pSource = source;
	m_pSDPLine = 0;
}

LIVE_H264VideoServerMediaSubsession::~LIVE_H264VideoServerMediaSubsession()
{
	if(m_pSDPLine) {
		free(m_pSDPLine);
	}
}

char const * LIVE_H264VideoServerMediaSubsession::getAuxSDPLine(RTPSink * rtpSink, FramedSource * inputSource)
{
	if(m_pSDPLine) return m_pSDPLine;

	m_pDummyRTPSink = rtpSink;
	m_pDummyRTPSink->startPlaying(*inputSource, 0 , 0);
	chkForAuxSDPLine(this);
	m_done = 0;

	envir().taskScheduler().doEventLoop(&m_done);
	m_pSDPLine = strdup(m_pDummyRTPSink->auxSDPLine());

	m_pDummyRTPSink->stopPlaying();
	return m_pSDPLine;
}
FramedSource * LIVE_H264VideoServerMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned & estBitrate)
{
    return H264VideoStreamFramer::createNew(envir(), new LIVE_H264VideoSource(envir(), filename));

}
RTPSink * LIVE_H264VideoServerMediaSubsession::createNewRTPSink(Groupsock * rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource * inputSource)
{
	return H264VideoRTPSink::createNew(envir(),  rtpGroupsock, rtpPayloadTypeIfDynamic);
}

LIVE_H264VideoServerMediaSubsession *LIVE_H264VideoServerMediaSubsession::createNew(UsageEnvironment & env, FramedSource * source, const std::string& filename)
{
	return new LIVE_H264VideoServerMediaSubsession(env,  source, filename);
}

void LIVE_H264VideoServerMediaSubsession::afterPlayingDummy(void * ptr)
{
	LIVE_H264VideoServerMediaSubsession *THIS = (LIVE_H264VideoServerMediaSubsession *)ptr;
	THIS->m_done = 0xff;
}

void LIVE_H264VideoServerMediaSubsession::chkForAuxSDPLine(void * ptr)
{
	LIVE_H264VideoServerMediaSubsession * This = (LIVE_H264VideoServerMediaSubsession *)ptr;
 
	This->chkForAuxSDPLine1();
}
void LIVE_H264VideoServerMediaSubsession::chkForAuxSDPLine1()
{
	
	if (m_pDummyRTPSink->auxSDPLine())
	{
		m_done = 0xff;
	}
	else
	{
		double delay = 1000.0 / (FRAME_PER_SEC);  // ms
		int to_delay = delay * 1000;  // us

		nextTask() = envir().taskScheduler().scheduleDelayedTask(to_delay, chkForAuxSDPLine, this);
	}

}

