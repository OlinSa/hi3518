#include <iostream>
#include "rtsp_service.hh"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MKFIFO_PATH "/tmp/H264_fifo"
#define MEDIA_SERVER_VERSION_STRING "0.91"

using namespace std;

RtspService::RtspService()
{
}
RtspService::~RtspService()
{
    if(pipe_fd) {
        close(pipe_fd);
    }
}
bool RtspService::Init()
{

    /***************************
     * Init FIFO    
    ****************************/
    pipe_fd = open(MKFIFO_PATH, O_WRONLY | O_CREAT, 0644);
    if (-1 == pipe_fd)
    {
        cout<<"open"<< MKFIFO_PATH<<"failed"<<endl;
        return NULL;
    }

    scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);

    UserAuthenticationDatabase *authDB = NULL;
#ifdef ACCESS_CONTROL
    // To implement client access control to the RTSP server, do the following:
    authDB = new UserAuthenticationDatabase;
    authDB->addUserRecord("username1", "password1"); // replace these with real strings
    // Repeat the above with each <username>, <password> that you wish to allow
    // access to the server.
#endif

    portNumBits rtspServerPortNum = 554;
    rtspServer = RTSPServer::createNew(*env, rtspServerPortNum, authDB);
    if (rtspServer == NULL)
    {
        rtspServerPortNum = 8554;
        rtspServer = RTSPServer::createNew(*env, rtspServerPortNum, authDB);
    }
    if (rtspServer == NULL)
    {
        *env << "Failed to create RTSP server:" << env->getResultMsg() << "\n";
        return false;
    }

    LIVE_H264VideoSource *videoSource = NULL;

    ServerMediaSession *sms = ServerMediaSession::createNew(*env, "live", 0, "live test");
    sms->addSubsession(LIVE_H264VideoServerMediaSubsession::createNew(*env, videoSource, "test.264"));
    rtspServer->addServerMediaSession(sms);

    char *url = rtspServer->rtspURL(sms);
    *env << "using url \"" << url << "\"\n";
    delete[] url;

    *env << "LIVE555 Media Server\n";
    *env << "\tversion " << MEDIA_SERVER_VERSION_STRING
         << " (LIVE555 Streaming Media library version "
         << LIVEMEDIA_LIBRARY_VERSION_STRING << ").\n";

    char *urlPrefix = rtspServer->rtspURLPrefix();
    *env << "Play streams from this server using the URL\n\t"
         << urlPrefix << "<filename>\nwhere <filename> is a file present in the current directory.\n";
    *env << "Each file's type is inferred from its name suffix:\n";
    *env << "\t\".264\" => a H.264 Video Elementary Stream file\n";
    *env << "\t\".265\" => a H.265 Video Elementary Stream file\n";
    *env << "\t\".aac\" => an AAC Audio (ADTS format) file\n";
    *env << "\t\".ac3\" => an AC-3 Audio file\n";
    *env << "\t\".amr\" => an AMR Audio file\n";
    *env << "\t\".dv\" => a DV Video file\n";
    *env << "\t\".m4e\" => a MPEG-4 Video Elementary Stream file\n";
    *env << "\t\".mkv\" => a Matroska audio+video+(optional)subtitles file\n";
    *env << "\t\".mp3\" => a MPEG-1 or 2 Audio file\n";
    *env << "\t\".mpg\" => a MPEG-1 or 2 Program Stream (audio+video) file\n";
    *env << "\t\".ogg\" or \".ogv\" or \".opus\" => an Ogg audio and/or video file\n";
    *env << "\t\".ts\" => a MPEG Transport Stream file\n";
    *env << "\t\t(a \".tsx\" index file - if present - provides server 'trick play' support)\n";
    *env << "\t\".vob\" => a VOB (MPEG-2 video with AC-3 audio) file\n";
    *env << "\t\".wav\" => a WAV Audio file\n";
    *env << "\t\".webm\" => a WebM audio(Vorbis)+video(VP8) file\n";
    *env << "See http://www.live555.com/mediaServer/ for additional documentation.\n";

    // Also, attempt to create a HTTP server for RTSP-over-HTTP tunneling.
    // Try first with the default HTTP port (80), and then with the alternative HTTP
    // port numbers (8000 and 8080).

    if (rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8000) || rtspServer->setUpTunnelingOverHTTP(8080))
    {
        *env << "(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling, or for HTTP live streaming (for indexed Transport Stream files only).)\n";
    }
    else
    {
        *env << "(RTSP-over-HTTP tunneling is not available.)\n";
    }

    return true;
}

ssize_t RtspService::PutMemInBuffer(const void *buf, size_t count)
{

}