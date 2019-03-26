#include "liveH264VideoSource.h"
#include "log.h"

LiveH264VideoSource::LiveH264VideoSource(UsageEnvironment &env, GetDataFunc cb, void *_arg)
    : FramedSource(env), token(NULL), getDataFunc(cb), arg(_arg)
{
}
LiveH264VideoSource::~LiveH264VideoSource()
{
    if (token)
    {
        envir().taskScheduler().unscheduleDelayedTask(token);
    }
}

unsigned LiveH264VideoSource::maxFrameSize() const
{
    return 1024 * 100;
}
void LiveH264VideoSource::doGetNextFrame()
{
    double delay = 1000.0 / (FRAME_PER_SEC);
    int to_delay = delay * 1000;
    token = envir().taskScheduler().scheduleDelayedTask(to_delay, getNextFrame, this);
}

void LiveH264VideoSource::getNextFrame(void *ptr)
{
    ((LiveH264VideoSource *)ptr)->getFrameData();
}
void LiveH264VideoSource::getFrameData()
{
    const void *buff;
    gettimeofday(&fPresentationTime, 0);
    ssize_t size = getDataFunc(buff, fMaxSize, arg);
    LOG_DEBUG("buff:%p,size:%d， fMaxSize:%d", buff,size,fMaxSize);
    if (size > 0)
    {
        if (size > fMaxSize)
        {
            fFrameSize = fMaxSize;
        }
        else
        {
            fFrameSize = size;
        }
        memcpy(fTo, buff, fFrameSize);
    }
    else
    {
        LOG_ERR("Get Buffer failed");
        fFrameSize = 0;
    }
    LOG_DEBUG("[MEDIA SERVER] GetFrameData len = [%d],fMaxSize = [%d]\n", fFrameSize, fMaxSize);
    afterGetting(this);
}