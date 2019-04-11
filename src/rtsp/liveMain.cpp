#include <iostream>
#include <string>
#include "log.h"
#include "rtspServer.h"

using namespace std;

typedef struct {
    unsigned char *buffer;
    ssize_t size;
    unsigned int offset;
}MEM_BUFFER_T;

unsigned char *get_media(const char *filename, ssize_t &file_size)
{
  FILE *fp = fopen(filename, "rb");
  if(!fp) return NULL;
  fseek(fp, 0, SEEK_END);
  file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  unsigned char *databuf = new unsigned char[file_size];
  if(!databuf) {
    fclose(fp);
    return NULL;
  }
  if(0 >= fread(databuf, 1, file_size, fp)) {
    databuf = NULL;
    fclose(fp);
    return NULL;
  }

  fclose(fp);
  return databuf;
}

ssize_t GetDataFun(const void *&buf, ssize_t max ,void *arg)
{
    ssize_t len;
    MEM_BUFFER_T *bufferInfo = static_cast<MEM_BUFFER_T*>(arg);
    len = bufferInfo->size - bufferInfo->offset;
    if(len > max) {
        len = max;
    }

    buf = bufferInfo->buffer + bufferInfo->offset;
    bufferInfo->offset += len;
    return len;
}


int main(int argc, char *argv[])
{
    const char *mediaFileName = "test.264";

    int port = 8554;
    string userName, passWd;

    STREAM_MEM_INPUT memInput = {"live", GetDataFun};

    RtspServer *rtspServer = new RtspServer(port, userName, passWd);
    if (!rtspServer)
    {
        LOG_ERR("new RtspServer failed");
        return -1;
    }

    MEM_BUFFER_T bufferInfo;
    
    if(NULL == (bufferInfo.buffer = get_media(mediaFileName, bufferInfo.size))){
        LOG_ERR("get buffer from %s failed", mediaFileName);
        return -1;
    }

    bufferInfo.offset = 0;

    LOG_INFO("get %s success", mediaFileName);
    if (!rtspServer->Init())
    {
        LOG_ERR("rtsp server init failed");
        goto do_exit;
    }

    if(!rtspServer->SetLiveStream(memInput, &bufferInfo)) {
        LOG_ERR("set live stream failed");
        goto do_exit;
    }

    rtspServer->DoLoop();

do_exit:
    delete rtspServer;
    return 0;
}