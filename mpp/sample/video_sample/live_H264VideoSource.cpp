#include "live_H264VideoSource.hh"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>

#define MKFIFO_PATH "/tmp/H264_fifo"
#define REV_BUF_SIZE (100 * 1024LL)

using namespace std;

#define mSleep(ms) usleep(ms * 1000)

LIVE_H264VideoSource::LIVE_H264VideoSource(UsageEnvironment &env, std::string filename)
	: FramedSource(env), m_pToken(NULL)
{
	if (access(MKFIFO_PATH, F_OK) == -1)
	{
		int res = mkfifo(MKFIFO_PATH, 0777);
		if (res != 0)
		{
			printf("[RTSPStream] Create fifo failed.\n");
			return;
		}
	}
	pipe_fd = open(MKFIFO_PATH, O_RDONLY | O_CREAT);
}

LIVE_H264VideoSource::~LIVE_H264VideoSource()
{
	envir().taskScheduler().unscheduleDelayedTask(m_pToken);

	envir() << "destruct H264Videosource";
}

void LIVE_H264VideoSource::doGetNextFrame()
{
	//calacture by fps;
	double delay = 1000.0 / (FRAME_PER_SEC); //ms
	int to_delay = delay * 1000;			 //us

	m_pToken = envir().taskScheduler().scheduleDelayedTask(to_delay, getNextFrame, this);
}
unsigned int LIVE_H264VideoSource::maxFrameSize()
{
	return 1024 * 200;
}

void LIVE_H264VideoSource::getNextFrame(void *ptr)
{
	((LIVE_H264VideoSource *)ptr)->GetFrameData();
}
void LIVE_H264VideoSource::GetFrameData()
{
	gettimeofday(&fPresentationTime, 0);

	if (-1 != pipe_fd)
	{
		fFrameSize = read(pipe_fd, fTo, fMaxSize);
	}
	else
	{
		envir() << "pipe bad\n";
		fFrameSize = 0;
	}

	printf("[MEDIA SERVER] GetFrameData len = [%d],fMaxSize = [%d]\n", fFrameSize, fMaxSize);

	afterGetting(this);
}