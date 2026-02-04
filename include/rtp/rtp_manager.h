#ifndef GB28181_RTP_MANAGER_H
#define GB28181_RTP_MANAGER_H

#include <string>
#include <memory>
#include <functional>
#include <vector>

namespace gb28181 {

enum class RtpPayloadType {
    PCMU = 0,      // G.711 u-law
    PCMA = 8,      // G.711 A-law
    H264 = 96,     // H.264 video
    H265 = 98,     // H.265 video
    PS = 99        // MPEG-2 PS (GB28181)
};

struct RtpPacket {
    uint8_t version;
    bool padding;
    bool extension;
    uint8_t csrcCount;
    bool marker;
    RtpPayloadType payloadType;
    uint16_t sequenceNumber;
    uint32_t timestamp;
    uint32_t ssrc;
    std::vector<uint8_t> payload;
};

using RtpReceiveCallback = std::function<void(const RtpPacket& packet, const std::string& fromIp, int fromPort)>;

class RtpManager {
public:
    RtpManager();
    ~RtpManager();

    // 初始化RTP管理器
    bool Initialize(const std::string& localIp, int basePort);

    // 开始RTP会话
    bool StartSession(const std::string& remoteIp, int remotePort, RtpPayloadType payloadType);

    // 停止RTP会话
    void StopSession();

    // 发送RTP数据包
    bool SendPacket(const uint8_t* data, size_t len, bool marker = false);

    // 发送PS流数据
    bool SendPsData(const uint8_t* data, size_t len);

    // 设置接收回调
    void SetReceiveCallback(RtpReceiveCallback callback);

    // 处理接收数据
    void Process();

    // 获取SSRC
    uint32_t GetSsrc() const;

    // 设置SSRC
    void SetSsrc(uint32_t ssrc);

private:
    class Impl;
    Impl* impl_;
};

} // namespace gb28181

#endif // GB28181_RTP_MANAGER_H
