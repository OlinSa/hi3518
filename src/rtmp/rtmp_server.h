/**
 * Description:This is the rtmp send interface which can push the stream to the rtmp server
 * Author: Jian.Cui
 * Date: Apirl 19,2019 00:36:00
*/

#ifndef SRC_RTMP_RTMP_H
#define SRC_RTMP_RTMP_H

#include <string>
#include <map>
#include "librtmp/rtmp_sys.h"
#include "librtmp/log.h"
#include "h264_wrap.h"

#define BUFSIZE_DEFAULT 200000
//定义包头长度，RTMP_MAX_HEADER_SIZE=18
#define RTMP_HEAD_SIZE (sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE)
//存储Nal单元数据的buffer大小
#define BUFFER_SIZE 32768
//搜寻Nal单元时的一些标志
#define GOT_A_NAL_CROSS_BUFFER BUFFER_SIZE + 1
#define GOT_A_NAL_INCLUDE_A_BUFFER BUFFER_SIZE + 2
#define NO_MORE_BUFFER_TO_READ BUFFER_SIZE + 3

typedef struct
{
    unsigned char *data;
    int len;
} RTMPMetaData;

typedef struct
{
    int pos;                //数据解析索引位置
    unsigned char *buff;
    int buffSize;           //缓存size
    int alreadSeen;         //数据大小
} BUFFER_INFO;

class RtmpServer
{
public:
    RtmpServer();
    ~RtmpServer();
    bool Init(std::string url, unsigned int cacheSize);
    ssize_t PublishH264(const char *buf, ssize_t bufSize, void *arg);  
    
protected:
    bool SetSPS(const RTMPMetaData &metricData);
    bool SetPPS(const RTMPMetaData &metricData);
    bool GetSPS(RTMPMetaData &metricData);
    bool GetPPS(RTMPMetaData &metricData);
    bool PublishPacket(const NaluUnit &naluUnit);
    bool SendH264Packet(unsigned char *data, int size, bool isKeyFrame, unsigned int timeStamp);
    int SendPacket(unsigned int packetType, unsigned char *data, unsigned int size, unsigned int timestamp);
    /**
     * @brief Send sps and pps to remote 
     *
     * @param [in] sps
     * @param [in] pps
     * @return 
     * @see None.
     */
    bool SendVideoSpsPps(const RTMPMetaData &sps,const RTMPMetaData &pps);
private:
    RTMP *r;
    BUFFER_INFO *bufferInfo;
    H264Wrap *h264Wrap;
    std::map<std::string, RTMPMetaData> metricDataMap;
    int fps;
    int width;
    int height;
    int tick;
    int tickGap;
    unsigned int lastUpdate = 0;
};

#endif