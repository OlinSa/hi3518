#ifndef SRC_RTSP_VIDEO_SOURCE_H
#define SRC_RTSP_VIDEO_SOURCE_H

#define FRAME_PER_SEC 25

#include <iostream>
#include <fstream>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "FramedSource.hh"

typedef ssize_t (*GetDataFunc)(const void *&buf, ssize_t max, void *arg);

class LiveH264VideoSource : public FramedSource
{
  public:
    LiveH264VideoSource(UsageEnvironment &env, GetDataFunc cb, void *arg);
    ~LiveH264VideoSource();

    virtual unsigned maxFrameSize() const;
    virtual void doGetNextFrame();

    static void getNextFrame(void *ptr);
    void getFrameData();
private:
    TaskToken token;
    GetDataFunc getDataFunc;
    void *arg;
};

#endif