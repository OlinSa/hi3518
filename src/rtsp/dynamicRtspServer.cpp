#include "dynamicRtspServer.h"
#include "liveMedia.hh"

static ServerMediaSession *createNewSMS(UsageEnvironment &env,
                                        char const *fileName, FILE * /*fid*/);
DynamicRTSPServer *DynamicRTSPServer::createNew(
    UsageEnvironment &env, Port rtspPort,
    UserAuthenticationDatabase *authDatabase,
    unsigned reclamationTestSeconds)
{
    int ourSocket = setUpOurSocket(env, rtspPort);
    if (-1 == ourSocket)
        return NULL;
    return new DynamicRTSPServer(env, ourSocket, rtspPort, authDatabase, reclamationTestSeconds);
}

DynamicRTSPServer::DynamicRTSPServer(UsageEnvironment &env,
                                     int ourSocket, Port ourPort,
                                     UserAuthenticationDatabase *authDatabase,
                                     unsigned reclamationTestSeconds) : RTSPServerSupportingHTTPStreaming(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds)
{
}

DynamicRTSPServer::~DynamicRTSPServer() {}

ServerMediaSession *DynamicRTSPServer::lookupServerMediaSession(char const *streamName, Boolean isFirstLookupInSession)
{
    FILE *fid = fopen(streamName, "rb");
    bool fileExists = fid != NULL;

    ServerMediaSession *sms = RTSPServer::lookupServerMediaSession(streamName);
    bool smsExists = sms != NULL;

    if (!fileExists)
    {
        if (smsExists)
        {
            removeServerMediaSession(sms);
            sms = NULL;
        }
        return NULL;
    }
    else
    {
        if (smsExists && isFirstLookupInSession)
        {
            removeServerMediaSession(sms);
            sms = NULL;
        }
        if (sms == NULL)
        {
            sms = createNewSMS(envir(), streamName, fid);
            addServerMediaSession(sms);
        }
        fclose(fid);
        return sms;
    }
}

#define NEW_SMS(description) do {\
char const* descStr = description\
    ", streamed by the LIVE555 Media Server";\
sms = ServerMediaSession::createNew(env, fileName, fileName, descStr);\
} while(0)

static ServerMediaSession *createNewSMS(UsageEnvironment &env,
                                        char const *fileName, FILE * /*fid*/)
{

    char const *extension = strrchr(fileName, '.');
    if(NULL == extension) return NULL;

    ServerMediaSession *sms = NULL;
    Boolean reuseSource = False;

    extension += 1;
    
    if(0 == strcmp(extension, "264")) {
        NEW_SMS("H.264 Video");
        OutPacketBuffer::maxSize = 100000;
        sms->addSubsession(H264VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    }else if(0 == strcmp(extension, "265")) {
        NEW_SMS("H.265 Video");
        OutPacketBuffer::maxSize = 100000;
        sms->addSubsession(H265VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    }
    return sms;
}