#ifndef GB28181_SIP_TRANSPORT_H
#define GB28181_SIP_TRANSPORT_H

#include <string>
#include <functional>

namespace gb28181 {

using TransportReceiveCallback = std::function<void(const std::string& data, const std::string& fromIp, int fromPort)>;

class SipTransport {
public:
    SipTransport();
    ~SipTransport();

    // 启动传输层
    bool Start(const std::string& localIp, int localPort);

    // 停止传输层
    void Stop();

    // 发送数据
    bool Send(const std::string& data, const std::string& destIp, int destPort);

    // 设置接收回调
    void SetReceiveCallback(TransportReceiveCallback callback);

    // 处理接收数据
    void Process();

private:
    class Impl;
    Impl* impl_;
};

} // namespace gb28181

#endif // GB28181_SIP_TRANSPORT_H
