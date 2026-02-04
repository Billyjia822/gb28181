#include "sip/sdp_negotiator.h"
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <algorithm>

namespace gb28181 {

SdpNegotiator::SdpNegotiator() : sessionVersion_(0) {
    sessionId_ = GenerateSessionId();
}

SdpNegotiator::~SdpNegotiator() {
}

std::string SdpNegotiator::CreateSdpAnswer(
    const std::string& localIp,
    int rtpPort,
    SdpMediaFormat videoFormat,
    SdpMediaFormat audioFormat) {

    std::stringstream ss;

    // v= 协议版本
    ss << "v=0\r\n";

    // o= 会话发起者
    ss << "o=- " << sessionId_ << " " << ++sessionVersion_ << " IN IP4 " << localIp << "\r\n";

    // s= 会话名称
    ss << "s=Play\r\n";

    // c= 连接信息
    ss << "c=IN IP4 " << localIp << "\r\n";

    // t= 时间信息（0表示无限期）
    ss << "t=0 0\r\n";

    // 视频媒体描述
    int videoPayload = GetVideoPayloadType(videoFormat);
    ss << "m=video " << rtpPort << " RTP/AVP " << videoPayload << "\r\n";
    ss << "a=rtpmap:" << videoPayload << " " << GetFormatName(videoFormat) << "/90000\r\n";

    // H.264/H.265需要添加fmtp
    if (videoFormat == SdpMediaFormat::H264) {
        ss << "a=fmtp:" << videoPayload << " profile-level-id=42e01f;packetization-mode=1\r\n";
    } else if (videoFormat == SdpMediaFormat::H265) {
        ss << "a=fmtp:" << videoPayload << " profile-id=1\r\n";
    } else if (videoFormat == SdpMediaFormat::PS) {
        // PS流不需要fmtp
    }

    // 音频媒体描述
    int audioPayload = GetAudioPayloadType(audioFormat);
    int audioPort = rtpPort + 2; // 音频端口通常在视频端口+2
    ss << "m=audio " << audioPort << " RTP/AVP " << audioPayload << "\r\n";
    ss << "a=rtpmap:" << audioPayload << " " << GetFormatName(audioFormat) << "/8000/1\r\n";

    return ss.str();
}

std::string SdpNegotiator::CreateSdpOffer(
    const std::string& localIp,
    const std::string& remoteIp,
    int rtpPort,
    SdpMediaFormat videoFormat,
    SdpMediaFormat audioFormat) {

    std::stringstream ss;

    // v= 协议版本
    ss << "v=0\r\n";

    // o= 会话发起者
    ss << "o=- " << sessionId_ << " " << ++sessionVersion_ << " IN IP4 " << localIp << "\r\n";

    // s= 会话名称
    ss << "s=Play\r\n";

    // c= 连接信息
    ss << "c=IN IP4 " << localIp << "\r\n";

    // t= 时间信息
    ss << "t=0 0\r\n";

    // 视频媒体描述
    int videoPayload = GetVideoPayloadType(videoFormat);
    ss << "m=video " << rtpPort << " RTP/AVP " << videoPayload << "\r\n";
    ss << "c=IN IP4 " << remoteIp << "\r\n";
    ss << "a=rtpmap:" << videoPayload << " " << GetFormatName(videoFormat) << "/90000\r\n";

    if (videoFormat == SdpMediaFormat::H264) {
        ss << "a=fmtp:" << videoPayload << " profile-level-id=42e01f;packetization-mode=1\r\n";
    } else if (videoFormat == SdpMediaFormat::H265) {
        ss << "a=fmtp:" << videoPayload << " profile-id=1\r\n";
    }

    // 音频媒体描述
    int audioPayload = GetAudioPayloadType(audioFormat);
    int audioPort = rtpPort + 2;
    ss << "m=audio " << audioPort << " RTP/AVP " << audioPayload << "\r\n";
    ss << "c=IN IP4 " << remoteIp << "\r\n";
    ss << "a=rtpmap:" << audioPayload << " " << GetFormatName(audioFormat) << "/8000/1\r\n";

    return ss.str();
}

SdpSessionInfo SdpNegotiator::ParseSdp(const std::string& sdpStr) {
    SdpSessionInfo sessionInfo;

    std::istringstream ss(sdpStr);
    std::string line;

    while (std::getline(ss, line)) {
        if (line.empty()) continue;

        // 移除行尾的\r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.length() < 2) continue;

        char type = line[0];
        std::string content = line.substr(2);

        switch (type) {
            case 'v': // 版本
                sessionInfo.version = content;
                break;

            case 'o': // 会话发起者
                sessionInfo.origin = content;
                break;

            case 's': // 会话名称
                sessionInfo.sessionName = content;
                break;

            case 'c': // 连接信息
                sessionInfo.connectionInfo = content;
                break;

            case 't': // 时间信息
                sessionInfo.timing = content;
                break;

            case 'm': // 媒体描述
            {
                SdpMediaInfo mediaInfo;
                std::istringstream mediaStream(content);
                std::string token;

                // media类型
                if (mediaStream >> token) mediaInfo.type = token;
                // 端口
                if (mediaStream >> token) mediaInfo.port = std::stoi(token);
                // 传输协议
                if (mediaStream >> token) mediaInfo.transport = token;
                // 负载类型
                while (mediaStream >> token) {
                    mediaInfo.payloadTypes.push_back(std::stoi(token));
                }

                sessionInfo.mediaInfos.push_back(mediaInfo);
                break;
            }

            case 'a': // 属性
            {
                if (!sessionInfo.mediaInfos.empty()) {
                    std::string attrName, attrValue;
                    size_t pos = content.find(':');
                    if (pos != std::string::npos) {
                        attrName = content.substr(0, pos);
                        attrValue = content.substr(pos + 1);

                        if (attrName == "rtpmap") {
                            sessionInfo.mediaInfos.back().rtpmap = attrValue;
                        } else if (attrName == "fmtp") {
                            sessionInfo.mediaInfos.back().fmtp = attrValue;
                        }
                    }
                }
                break;
            }

            default:
                break;
        }
    }

    return sessionInfo;
}

int SdpNegotiator::GetVideoPayloadType(SdpMediaFormat format) {
    switch (format) {
        case SdpMediaFormat::H264:
            return 96;
        case SdpMediaFormat::H265:
            return 98;
        case SdpMediaFormat::PS:
            return 99;
        default:
            return 96;
    }
}

int SdpNegotiator::GetAudioPayloadType(SdpMediaFormat format) {
    switch (format) {
        case SdpMediaFormat::PCMU:
            return 0;
        case SdpMediaFormat::PCMA:
            return 8;
        case SdpMediaFormat::AAC:
            return 97;
        default:
            return 8;
    }
}

std::string SdpNegotiator::GetFormatName(SdpMediaFormat format) {
    switch (format) {
        case SdpMediaFormat::H264:
            return "H264";
        case SdpMediaFormat::H265:
            return "H265";
        case SdpMediaFormat::PS:
            return "MP2T";
        case SdpMediaFormat::PCMU:
            return "PCMU";
        case SdpMediaFormat::PCMA:
            return "PCMA";
        case SdpMediaFormat::AAC:
            return "AAC";
        default:
            return "H264";
    }
}

std::string SdpNegotiator::CreateMediaLine(const SdpMediaInfo& media) {
    std::stringstream ss;
    ss << "m=" << media.type << " " << media.port << " " << media.transport;

    for (size_t i = 0; i < media.payloadTypes.size(); i++) {
        if (i > 0) ss << " ";
        ss << media.payloadTypes[i];
    }

    ss << "\r\n";
    return ss.str();
}

std::string SdpNegotiator::CreateRtpmap(int payloadType, const std::string& codec, int clockRate, int channels) {
    std::stringstream ss;
    ss << "a=rtpmap:" << payloadType << " " << codec << "/" << clockRate;
    if (channels > 1) {
        ss << "/" << channels;
    }
    ss << "\r\n";
    return ss.str();
}

std::string SdpNegotiator::CreateFmtp(int payloadType, const std::string& params) {
    std::stringstream ss;
    ss << "a=fmtp:" << payloadType << " " << params << "\r\n";
    return ss.str();
}

std::string SdpNegotiator::GenerateSessionId() {
    // 使用时间戳生成会话ID
    std::stringstream ss;
    ss << time(nullptr);
    return ss.str();
}

} // namespace gb28181
