#ifndef SRC_RTSP_DYNAMIC_RTSP_H
#define SRC_RTSP_DYNAMIC_RTSP_H

#include "RTSPServerSupportingHTTPStreaming.hh"

class DynamicRTSPServer : public RTSPServerSupportingHTTPStreaming
{
  public:
    static DynamicRTSPServer *createNew(
        UsageEnvironment &env, Port rtspPort = 554,
        UserAuthenticationDatabase *authDatabase = NULL,
        unsigned reclamationTestSeconds = 65);

  protected:
    DynamicRTSPServer(UsageEnvironment &env,
                      int ourSocket, Port ourPort,
                      UserAuthenticationDatabase *authDatabase,
                      unsigned reclamationTestSeconds);
    // called only by createNew();
    virtual ~DynamicRTSPServer();

    virtual ServerMediaSession *
    lookupServerMediaSession(char const *streamName, Boolean isFirstLookupInSession);
};

#endif
