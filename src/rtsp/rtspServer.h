#ifndef SRC_RTSP_RTSP_SERVER_H
#define SRC_RTSP_RTSP_SERVER_H

/**
 * @file rtsp_server.h 
 * @brief Rtsp Server实现
 * @author Olin
 * @date 2019-03-12
 */

#include <iostream>
#include <string>
#include <BasicUsageEnvironment.hh>
#include "DynamicRTSPServer.hh"
#include "liveH264VideoSource.h"
#include "liveH264VideoServerMediaSubsession.h"
#include "version.hh"

enum MEDIA_TRAN_TYPE
{
    METHOD_TRANSFER_FILE,
    METHOD_TRANSFER_MEM
};

typedef struct
{
    std::string streamName;
    GetDataFunc getDataFunc;
} STREAM_MEM_INPUT;

class RtspServer
{
  public:
    RtspServer(int port, std::string userName, std::string passWD) : _port(port), _userName(userName),
                                                                     _passWD(passWD), _scheduler(NULL), _env(NULL), _rtspServer(NULL) {}
    ~RtspServer() {
        if(_scheduler) {
            delete _scheduler;
        }
    }

    bool Init();
    bool SetLiveStream(STREAM_MEM_INPUT memInput, void *arg);

    bool DoLoop();

  protected:
    int _port;
    std::string _userName;
    std::string _passWD;
    TaskScheduler *_scheduler;
    UsageEnvironment *_env;
    RTSPServer *_rtspServer;
};

#endif
