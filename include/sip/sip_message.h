#ifndef GB28181_SIP_MESSAGE_H
#define GB28181_SIP_MESSAGE_H

#include <string>
#include <map>
#include <memory>

namespace gb28181 {

enum class SipMessageType {
    REGISTER,
    MESSAGE,
    INVITE,
    BYE,
    ACK,
    OPTIONS,
    INFO,
    NOTIFY
};

enum class SipMethodType {
    CATALOG,      // 目录查询
    DEVICE_INFO,  // 设备信息查询
    DEVICE_STATUS,// 设备状态查询
    RECORD_INFO,  // 录像查询
    RECORD,       // 录像回放
    PTZ,          // 云台控制
    ALARM,        // 报警
    DEVICE_CONTROL // 设备控制
};

class SipMessage {
public:
    SipMessage();
    ~SipMessage();

    // 创建SIP消息
    static std::shared_ptr<SipMessage> Create(SipMessageType type);

    // 设置消息头
    void SetHeader(const std::string& key, const std::string& value);

    // 获取消息头
    std::string GetHeader(const std::string& key) const;

    // 设置消息体
    void SetBody(const std::string& body);

    // 获取消息体
    std::string GetBody() const;

    // 设置方法类型
    void SetMethodType(SipMethodType type);

    // 获取方法类型
    SipMethodType GetMethodType() const;

    // 序列化为字符串
    std::string ToString() const;

    // 从字符串解析
    static std::shared_ptr<SipMessage> FromString(const std::string& data);

private:
    SipMessageType type_;
    SipMethodType methodType_;
    std::map<std::string, std::string> headers_;
    std::string body_;
};

} // namespace gb28181

#endif // GB28181_SIP_MESSAGE_H
