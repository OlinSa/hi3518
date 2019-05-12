#include <string.h>
#include "rtmp_server.h"
#include "log.h"

RtmpServer::RtmpServer()
    : r(NULL), bufferInfo(NULL), h264Wrap(NULL)
{
}

RtmpServer::~RtmpServer()
{
    if (r)
    {
        RTMP_Close(r);
    }
}
bool RtmpServer::Init(std::string url, unsigned int cacheSize)
{
    r = RTMP_Alloc();
    if (!r)
    {
        LOG_ERR("Alloc RTMP failed");
        return false;
    }
    RTMP_Init(r);
    if (!RTMP_SetupURL(r, (char *)(url.c_str())))
    {
        RTMP_Log(RTMP_LOGERROR, "SetupURL(%s) failed", url.c_str());
        return false;
    }
    RTMP_EnableWrite(r);

    if (!RTMP_Connect(r, NULL))
    {
        LOG_ERR("connect failed");
        return false;
    }

    if (!RTMP_ConnectStream(r, 0))
    {
        LOG_ERR("connect stream failed");
        return false;
    }
    if (!(bufferInfo = new BUFFER_INFO()))
    {
        return false;
    }
    if (!(bufferInfo->buff = new unsigned char[cacheSize]))
    {
        delete bufferInfo;
        bufferInfo = NULL;
        return false;
    }

    bufferInfo->pos = 0;
    bufferInfo->buffSize = cacheSize;
    bufferInfo->alreadSeen = 0;

    if (NULL == (h264Wrap = new H264Wrap()))
    {
        return false;
    }
    fps = 25;
    tickGap = 1000 / fps;

    return true;
}
bool RtmpServer::PublishPacket(const NaluUnit &naluUnit)
{
    RTMPMetaData metricData;
    switch (naluUnit.type)
    {
    case NALU_TYPE_SPS:
    {
        metricData = {naluUnit.data, naluUnit.size};
        if (!SetSPS(metricData))
        {
            LOG_ERR("set sps failed");
            return false;
        }
    }
    break;
    case NALU_TYPE_PPS:
    {
        metricData = {naluUnit.data, naluUnit.size};
        if (!SetPPS(metricData))
        {
            LOG_ERR("set pps failed");
            return false;
        }
    }
    break;
    default:
    {

        LOG_DEBUG("nalu: startcode:%d,type:%d, data:%p, dataSize:%d",
                  naluUnit.startCode, naluUnit.type, naluUnit.data, naluUnit.size);

        bool isKeyFrame = (naluUnit.type == NALU_TYPE_IDR) ? true : false;
        if (isKeyFrame)
        {
            RTMPMetaData sps,pps;

            if(GetSPS(sps) && GetPPS(pps)) {
                if (false == SendVideoSpsPps(sps, pps))
                {
                    LOG_ERR("send sps pps failed");
                }
    
            }else {
                LOG_ERR("Get Metric data failed");
            }
        }

        if (!SendH264Packet(naluUnit.data, naluUnit.size, isKeyFrame, tick))
        {
            LOG_ERR("send H264 pack failed");
            return false;
        }
        tick += tickGap;
        unsigned int now = RTMP_GetTime();
        if ((tickGap + lastUpdate) > now)
        {
            LOG_DEBUG("sleep %d ms", tickGap + lastUpdate - now);
            msleep(tickGap + lastUpdate - now);
        }
        lastUpdate = now;
    }
    }
    return true;
}

//                pos          alreadSeen      bufSize
// |  #############| ############| #############|
ssize_t RtmpServer::PublishH264(const char *buf, ssize_t buf_size, void *arg)
{
    NaluUnit naluUnit;
    ssize_t size = 0;
    if (size > bufferInfo->buffSize - bufferInfo->alreadSeen)
    {
        size = bufferInfo->buffSize - bufferInfo->alreadSeen;
    }
    else
    {
        size = buf_size;
    }
    memcpy(bufferInfo->buff + bufferInfo->pos, buf, size);
    bufferInfo->alreadSeen += size;

    int nextFramePos;
    while ((nextFramePos = h264Wrap->GetNaluFromBuffer(naluUnit, bufferInfo->buff + bufferInfo->pos, bufferInfo->alreadSeen - bufferInfo->pos)) > 0)
    {
        LOG_DEBUG("nalu: startcode:%d,type:%d, data:%p, dataSize:%d,nextFramePos:%d",
                  naluUnit.startCode, naluUnit.type, naluUnit.data, naluUnit.size, nextFramePos);

        bufferInfo->pos += nextFramePos;

        if (false == PublishPacket(naluUnit))
        {
            LOG_ERR("publish packet failed");
        }
    }

    //归置media pos
    if (bufferInfo->pos > 0)
    {
        memmove(bufferInfo->buff, bufferInfo->buff + bufferInfo->pos, bufferInfo->alreadSeen - bufferInfo->pos); //memcpy会有内存重叠问题
        bufferInfo->alreadSeen -= bufferInfo->pos;
        bufferInfo->pos = 0;
    }
    else
    {
        bufferInfo->pos = 0;
        bufferInfo->alreadSeen = 0;
    }

    return size;
}

bool RtmpServer::SetSPS(const RTMPMetaData &metricData)
{
    RTMPMetaData sps;
    std::map<std::string, RTMPMetaData>::iterator it = metricDataMap.find("sps");
    if (it != metricDataMap.end())
    {
        delete it->second.data;
    }
    if (!(sps.data = new unsigned char[metricData.len]))
    {
        LOG_ERR("alloc memory failed");
        return false;
    }
    memcpy(sps.data, metricData.data, metricData.len);
    metricDataMap["sps"] = metricData;
    return true;
}
bool RtmpServer::SetPPS(const RTMPMetaData &metricData)
{
    RTMPMetaData pps;
    std::map<std::string, RTMPMetaData>::iterator it = metricDataMap.find("pps");
    if (it != metricDataMap.end())
    {
        delete it->second.data;
    }
    if (!(pps.data = new unsigned char[metricData.len]))
    {
        LOG_ERR("alloc memory failed");
        return false;
    }
    memcpy(pps.data, metricData.data, metricData.len);
    metricDataMap["pps"] = metricData;
    return true;
}
bool RtmpServer::GetSPS(RTMPMetaData &metricData)
{
    std::map<std::string, RTMPMetaData>::iterator it = metricDataMap.find("sps");
    if (it != metricDataMap.end())
    {
        metricData = it->second;
        return true;
    }
    return false;
}

bool RtmpServer::GetPPS(RTMPMetaData &metricData)
{
    std::map<std::string, RTMPMetaData>::iterator it = metricDataMap.find("pps");
    if (it != metricDataMap.end())
    {
        metricData = it->second;
        return true;
    }
    return false;
}

// --------------------------------------------------------------------------------------
//     FLV Header(FLAG(3Byte) + VERSION(1BYTE) + STEAMINFO(1BYTE) + HEADERLENGTH(4BYTE))
// --------------------------------------------------------------------------------------
//     FLV Body
// --------------------------------------------------------------------------------------

bool RtmpServer::SendH264Packet(unsigned char *data, int size, bool isKeyFrame, unsigned int timeStamp)
{
    int i = 0;

    if (!data || size < 11)
    {
        return false;
    }
    unsigned char *body = new unsigned char[size + 9];
    if (!body)
    {
        LOG_ERR("new body failed");
        return false;
    }
    memset(body, 0, size + 9);
    if (isKeyFrame)
    {
        body[i++] = 0x17; // 1:Iframe  7:AVC
        body[i++] = 0x01; // AVC NALU
        body[i++] = 0x00;
        body[i++] = 0x00;
        body[i++] = 0x00;
    }
    else
    {
        body[i++] = 0x27; // 2:Pframe  7:AVC
        body[i++] = 0x01; // AVC NALU
        body[i++] = 0x00;
        body[i++] = 0x00;
        body[i++] = 0x00;
    }
    // NALU size
    body[i++] = size >> 24 & 0xff;
    body[i++] = size >> 16 & 0xff;
    body[i++] = size >> 8 & 0xff;
    body[i++] = size & 0xff;
    // NALU data
    memcpy(&body[i], data, size);
    int ret = SendPacket(RTMP_PACKET_TYPE_VIDEO, body, i + size, timeStamp);
    delete[] body;
    return ret ? true : false;
}

int RtmpServer::SendPacket(unsigned int packetType, unsigned char *data, unsigned int size, unsigned int timestamp)
{
    RTMPPacket *packet = reinterpret_cast<RTMPPacket*>(new char[RTMP_HEAD_SIZE + size]);
    // RTMPPacket *packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE + size);
    if (!packet)
    {
        LOG_ERR("malloc RTMPPacket failed:%d", RTMP_HEAD_SIZE + size);
        return -1;
    }
    memset(packet, 0, RTMP_HEAD_SIZE);

    packet->m_body = (char *)(packet) + RTMP_HEAD_SIZE;
    packet->m_nBodySize = size;
    memcpy(packet->m_body, data, size);
    packet->m_hasAbsTimestamp = 0;
    packet->m_packetType = packetType;
    packet->m_nInfoField2 = r->m_stream_id;
    packet->m_nChannel = 0x04;

    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    if (RTMP_PACKET_TYPE_AUDIO == packetType && size != 4)
    {
        packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    }
    packet->m_nTimeStamp = timestamp;
    /*发送*/
    int nRet = 0;
    if (RTMP_IsConnected(r))
    {
        nRet = RTMP_SendPacket(r, packet, TRUE); /*TRUE为放进发送队列,FALSE是不放进发送队列,直接发送*/
    }
    /*释放内存*/
    delete[] packet;
    return nRet;
}

bool RtmpServer::SendVideoSpsPps(const RTMPMetaData &sps,const RTMPMetaData &pps)
{
    RTMPPacket *packet = (RTMPPacket *)new unsigned char[RTMP_HEAD_SIZE + 1024];
    if(!packet) {
        LOG_ERR("create RTMP Packet failed");
        return false;
    }
    memset(packet, 0, RTMP_HEAD_SIZE + 1024);
    packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
    unsigned char *body = (unsigned char *)packet->m_body;

    int i = 0;
    body[i++] = 0x17;
    body[i++] = 0x00;

    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;

    /*AVCDecoderConfigurationRecord*/
    body[i++] = 0x01;
    body[i++] = sps.data[1];
    body[i++] = sps.data[2];
    body[i++] = sps.data[3];
    body[i++] = 0xff;

    /*sps*/
    body[i++] = 0xe1;
    body[i++] = (sps.len >> 8) & 0xff;
    body[i++] = sps.len & 0xff;
    memcpy(&body[i], sps.data, sps.len);
    i += sps.len;

    /*pps*/
    body[i++] = 0x01;
    body[i++] = (pps.len >> 8) & 0xff;
    body[i++] = (pps.len) & 0xff;
    memcpy(&body[i], pps.data, pps.len);
    i += pps.len;

    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = i;
    packet->m_nChannel = 0x04;
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    packet->m_nInfoField2 = r->m_stream_id;

    int ret = RTMP_SendPacket(r, packet, TRUE);

    delete[] packet;

    return ret ? true : false;
}