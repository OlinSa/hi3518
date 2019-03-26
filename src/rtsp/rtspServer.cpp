#include "DynamicRTSPServer.hh"
#include "version.hh"
#include "rtspServer.h"
#include "log.h"
#include "liveH264VideoSource.h"
#include "liveH264VideoServerMediaSubsession.h"

static void announceStream(RTSPServer* rtsp_server, ServerMediaSession *sms, const char *stream_name)
{
  char *url = rtsp_server->rtspURL(sms);
  UsageEnvironment &env = rtsp_server->envir();
  env << stream_name<<"\n";
  env << "Play this stream using the URL: "<< url<<"\n";
  delete[] url;
}

bool RtspServer::Init()
{
    _scheduler = BasicTaskScheduler::createNew();
    _env = BasicUsageEnvironment::createNew(*_scheduler);

    UserAuthenticationDatabase *authDB = NULL;
    if (!_userName.empty() || !_passWD.empty())
    {
        authDB = new UserAuthenticationDatabase;
        authDB->addUserRecord(_userName.c_str(), _passWD.c_str());
    }
    if (NULL == (_rtspServer = RTSPServer::createNew(*_env, _port, authDB)))
    {
        LOG_ERR("Failed to create RTSP server:%s", _env->getResultMsg());
        return false;
    }

    return true;
}

bool RtspServer::SetLiveStream(STREAM_MEM_INPUT memInput, void *arg)
{
    LiveH264VideoSource *videoSource = NULL;
    ServerMediaSession *sms = ServerMediaSession::createNew(*_env, memInput.streamName.c_str(), 0, "live test");
                                                        
    sms->addSubsession(LiveH264VideoServerMeidaSubsession::createNew(*_env, videoSource, memInput.getDataFunc, arg));
    _rtspServer->addServerMediaSession(sms);

    announceStream(_rtspServer, sms, memInput.streamName.c_str());

    return true;
}

bool RtspServer::DoLoop()
{
    *_env << "LIVE555 Media Server\n";
    *_env << "\tversion " << MEDIA_SERVER_VERSION_STRING
         << " (LIVE555 Streaming Media library version "
         << LIVEMEDIA_LIBRARY_VERSION_STRING << ").\n";

    char *urlPrefix = _rtspServer->rtspURLPrefix();
    *_env << "Play streams from this server using the URL\n\t"
         << urlPrefix << "<filename>\nwhere <filename> is a file present in the current directory.\n";
    *_env << "Each file's type is inferred from its name suffix:\n";
    *_env << "\t\".264\" => a H.264 Video Elementary Stream file\n";
    *_env << "\t\".265\" => a H.265 Video Elementary Stream file\n";
    *_env << "\t\".aac\" => an AAC Audio (ADTS format) file\n";
    *_env << "\t\".ac3\" => an AC-3 Audio file\n";
    *_env << "\t\".amr\" => an AMR Audio file\n";
    *_env << "\t\".dv\" => a DV Video file\n";
    *_env << "\t\".m4e\" => a MPEG-4 Video Elementary Stream file\n";
    *_env << "\t\".mkv\" => a Matroska audio+video+(optional)subtitles file\n";
    *_env << "\t\".mp3\" => a MPEG-1 or 2 Audio file\n";
    *_env << "\t\".mpg\" => a MPEG-1 or 2 Program Stream (audio+video) file\n";
    *_env << "\t\".ogg\" or \".ogv\" or \".opus\" => an Ogg audio and/or video file\n";
    *_env << "\t\".ts\" => a MPEG Transport Stream file\n";
    *_env << "\t\t(a \".tsx\" index file - if present - provides server 'trick play' support)\n";
    *_env << "\t\".vob\" => a VOB (MPEG-2 video with AC-3 audio) file\n";
    *_env << "\t\".wav\" => a WAV Audio file\n";
    *_env << "\t\".webm\" => a WebM audio(Vorbis)+video(VP8) file\n";
    *_env << "See http://www.live555.com/mediaServer/ for additional documentation.\n";

    // Also, attempt to create a HTTP server for RTSP-over-HTTP tunneling.
    // Try first with the default HTTP port (80), and then with the alternative HTTP
    // port numbers (8000 and 8080).

    if (_rtspServer->setUpTunnelingOverHTTP(80) || _rtspServer->setUpTunnelingOverHTTP(8000) || _rtspServer->setUpTunnelingOverHTTP(8080))
    {
        *_env << "(We use port " << _rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling, or for HTTP live streaming (for indexed Transport Stream files only).)\n";
    }
    else
    {
        *_env << "(RTSP-over-HTTP tunneling is not available.)\n";
    }
    _env->taskScheduler().doEventLoop(); // does not return

    return true;
}