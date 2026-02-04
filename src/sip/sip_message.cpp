#include "sip/sip_message.h"
#include <sstream>
#include <algorithm>

namespace gb28181 {

SipMessage::SipMessage()
    : type_(SipMessageType::MESSAGE), methodType_(SipMethodType::DEVICE_INFO) {
}

SipMessage::~SipMessage() {
}

std::shared_ptr<SipMessage> SipMessage::Create(SipMessageType type) {
    auto msg = std::make_shared<SipMessage>();
    msg->type_ = type;
    return msg;
}

void SipMessage::SetHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

std::string SipMessage::GetHeader(const std::string& key) const {
    auto it = headers_.find(key);
    return (it != headers_.end()) ? it->second : "";
}

void SipMessage::SetBody(const std::string& body) {
    body_ = body;
}

std::string SipMessage::GetBody() const {
    return body_;
}

void SipMessage::SetMethodType(SipMethodType type) {
    methodType_ = type;
}

SipMethodType SipMessage::GetMethodType() const {
    return methodType_;
}

std::string SipMessage::ToString() const {
    std::stringstream ss;

    // 根据消息类型生成SIP消息
    switch (type_) {
        case SipMessageType::REGISTER:
            ss << "REGISTER sip:" << GetHeader("Domain") << " SIP/2.0\r\n";
            break;
        case SipMessageType::MESSAGE:
            ss << "MESSAGE sip:" << GetHeader("To") << " SIP/2.0\r\n";
            break;
        case SipMessageType::INVITE:
            ss << "INVITE sip:" << GetHeader("To") << " SIP/2.0\r\n";
            break;
        case SipMessageType::BYE:
            ss << "BYE sip:" << GetHeader("To") << " SIP/2.0\r\n";
            break;
        case SipMessageType::ACK:
            ss << "ACK sip:" << GetHeader("To") << " SIP/2.0\r\n";
            break;
        default:
            ss << "MESSAGE sip:" << GetHeader("To") << " SIP/2.0\r\n";
            break;
    }

    // 添加通用头字段
    ss << "Via: SIP/2.0/UDP " << GetHeader("LocalIp") << ":" << GetHeader("LocalPort")
       << ";rport;branch=z9hG4bK" << GetHeader("Branch") << "\r\n";
    ss << "From: <sip:" << GetHeader("From") << ">;tag=" << GetHeader("FromTag") << "\r\n";
    ss << "To: <sip:" << GetHeader("To") << ">" << "\r\n";
    ss << "Call-ID: " << GetHeader("CallId") << "\r\n";
    ss << "CSeq: " << GetHeader("CSeq") << " ";
    switch (type_) {
        case SipMessageType::REGISTER: ss << "REGISTER"; break;
        case SipMessageType::MESSAGE: ss << "MESSAGE"; break;
        case SipMessageType::INVITE: ss << "INVITE"; break;
        case SipMessageType::BYE: ss << "BYE"; break;
        case SipMessageType::ACK: ss << "ACK"; break;
        default: ss << "MESSAGE"; break;
    }
    ss << "\r\n";
    ss << "Max-Forwards: 70\r\n";
    ss << "User-Agent: GB28181 Device\r\n";
    ss << "Content-Type: Application/MANSCDP+xml\r\n";
    ss << "Content-Length: " << body_.length() << "\r\n";
    ss << "\r\n";
    ss << body_;

    return ss.str();
}

std::shared_ptr<SipMessage> SipMessage::FromString(const std::string& data) {
    auto msg = std::make_shared<SipMessage>();

    // 简单解析SIP消息
    std::istringstream ss(data);
    std::string line;

    // 解析请求行
    if (std::getline(ss, line)) {
        if (line.find("REGISTER") != std::string::npos) {
            msg->type_ = SipMessageType::REGISTER;
        } else if (line.find("MESSAGE") != std::string::npos) {
            msg->type_ = SipMessageType::MESSAGE;
        } else if (line.find("INVITE") != std::string::npos) {
            msg->type_ = SipMessageType::INVITE;
        } else if (line.find("BYE") != std::string::npos) {
            msg->type_ = SipMessageType::BYE;
        } else if (line.find("ACK") != std::string::npos) {
            msg->type_ = SipMessageType::ACK;
        }
    }

    // 解析头字段
    while (std::getline(ss, line) && line != "\r") {
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            // 去除首尾空格
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            msg->headers_[key] = value;
        }
    }

    // 解析消息体
    std::string body;
    while (std::getline(ss, line)) {
        body += line + "\n";
    }
    msg->body_ = body;

    return msg;
}

} // namespace gb28181
