#ifndef LIVE_H264VIDEOSOURCE_HH
#define LIVE_H264VIDEOSOURCE_HH

#include <iostream>
#include <iostream>
#include <fstream>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "FramedSource.hh"

#define FRAME_PER_SEC 25

class LIVE_H264VideoSource: public FramedSource
{
public:
    LIVE_H264VideoSource(UsageEnvironment & env, std::string filename = "");
    ~LIVE_H264VideoSource();
public:
	virtual void doGetNextFrame();
	virtual unsigned int maxFrameSize();
 
	static void getNextFrame(void * ptr);
	void GetFrameData();
 
private:
	void *m_pToken;
    int pipe_fd;
};


#endif
