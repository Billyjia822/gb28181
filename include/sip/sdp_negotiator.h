#ifndef GB28181_SDP_NEGOTIATOR_H
#define GB28181_SDP_NEGOTIATOR_H

#include <string>
#include <vector>
#include <map>

namespace gb28181 {

/**
 * @brief SDP媒体格式
 */
enum class SdpMediaFormat {
    H264,      // H.264视频
    H265,      // H.265视频
    PS,        // MPEG-2 PS流
    PCMU,      // G.711 u-law音频
    PCMA,      // G.711 A-law音频
    AAC        // AAC音频
};

/**
 * @brief SDP媒体信息
 */
struct SdpMediaInfo {
    std::string type;              // media类型：video/audio
    int port;                      // 端口号
    std::string transport;         // 传输协议：RTP/AVP
    std::vector<int> payloadTypes; // 负载类型
    std::string rtpmap;            // rtpmap属性
    std::string fmtp;              // fmtp属性
};

/**
 * @brief SDP会话信息
 */
struct SdpSessionInfo {
    std::string version;           // 协议版本
    std::string origin;            // 会话发起者
    std::string sessionName;       // 会话名称
    std::string connectionInfo;    // 连接信息
    std::string timing;            // 时间信息
    std::vector<SdpMediaInfo> mediaInfos; // 媒体信息
};

/**
 * @brief SDP协商器
 * 用于GB28181视频流的SDP协商
 */
class SdpNegotiator {
public:
    SdpNegotiator();
    ~SdpNegotiator();

    /**
     * @brief 创建SDP应答
     * @param localIp 本地IP
     * @param rtpPort RTP端口
     * @param videoFormat 视频格式
     * @param audioFormat 音频格式
     * @return SDP字符串
     */
    std::string CreateSdpAnswer(
        const std::string& localIp,
        int rtpPort,
        SdpMediaFormat videoFormat,
        SdpMediaFormat audioFormat = SdpMediaFormat::PCMA
    );

    /**
     * @brief 创建SDP邀请（用于主动发起会话）
     * @param localIp 本地IP
     * @param remoteIp 远程IP
     * @param rtpPort RTP端口
     * @param videoFormat 视频格式
     * @param audioFormat 音频格式
     * @return SDP字符串
     */
    std::string CreateSdpOffer(
        const std::string& localIp,
        const std::string& remoteIp,
        int rtpPort,
        SdpMediaFormat videoFormat,
        SdpMediaFormat audioFormat = SdpMediaFormat::PCMA
    );

    /**
     * @brief 解析SDP
     * @param sdpStr SDP字符串
     * @return SDP会话信息
     */
    SdpSessionInfo ParseSdp(const std::string& sdpStr);

    /**
     * @brief 获取视频负载类型
     * @param format 视频格式
     * @return 负载类型编号
     */
    static int GetVideoPayloadType(SdpMediaFormat format);

    /**
     * @brief 获取音频负载类型
     * @param format 音频格式
     * @return 负载类型编号
     */
    static int GetAudioPayloadType(SdpMediaFormat format);

    /**
     * @brief 获取格式编码名称
     * @param format 媒体格式
     * @return 编码名称
     */
    static std::string GetFormatName(SdpMediaFormat format);

private:
    /**
     * @brief 创建媒体描述行
     */
    std::string CreateMediaLine(const SdpMediaInfo& media);

    /**
     * @brief 创建rtpmap属性
     */
    std::string CreateRtpmap(int payloadType, const std::string& codec, int clockRate, int channels = 1);

    /**
     * @brief 创建fmtp属性（用于H.264/H.265）
     */
    std::string CreateFmtp(int payloadType, const std::string& params);

    /**
     * @brief 生成会话ID
     */
    std::string GenerateSessionId();

private:
    std::string sessionId_;
    int sessionVersion_;
};

} // namespace gb28181

#endif // GB28181_SDP_NEGOTIATOR_H
