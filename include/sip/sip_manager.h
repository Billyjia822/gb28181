#ifndef GB28181_SIP_MANAGER_H
#define GB28181_SIP_MANAGER_H

#include <string>
#include <memory>
#include <functional>
#include <vector>

namespace gb28181 {

// 前向声明
class MediaSessionManager;

// SIP事件回调类型
using SipEventCallback = std::function<void(const std::string& event, const std::string& data)>;

// 媒体会话事件回调
using MediaSessionEventCallback = std::function<void(const std::string& sessionId,
                                                     const std::string& state,
                                                     const std::string& event)>;

class SipManager {
public:
    SipManager();
    ~SipManager();

    // 初始化SIP
    bool Initialize(const std::string& localIp, int localPort,
                    const std::string& deviceId, const std::string& realm);

    // 注册到SIP服务器
    bool RegisterToServer(const std::string& serverIp, int serverPort,
                         const std::string& username, const std::string& password);

    // 注销
    bool Unregister();

    // 发送心跳
    bool SendHeartbeat();

    // 处理SIP消息
    void ProcessMessage();

    // 设置事件回调
    void SetEventCallback(SipEventCallback callback);

    // 设置媒体会话事件回调
    void SetMediaSessionEventCallback(MediaSessionEventCallback callback);

    // 获取本地IP
    std::string GetLocalIp() const;

    // 获取本地端口
    int GetLocalPort() const;

    // 获取设备ID
    std::string GetDeviceId() const;

    // 发送MESSAGE消息
    bool SendMessage(const std::string& to, const std::string& content);

    // 发送响应
    bool SendResponse(int tid, int statusCode, const std::string& reason);

    // PTZ云台控制 - 方向控制
    // command: 0=停止, 1=上, 2=下, 3=左, 4=右, 5=左上, 6=左下, 7=右上, 8=右下
    // speed: 速度(1-255)
    bool PtzControl(const std::string& channelId, int command, int speed);

    // PTZ云台控制 - 变焦控制
    // command: 0=停止, 1=放大, 2=缩小
    // speed: 速度(1-255)
    bool PtzZoom(const std::string& channelId, int command, int speed);

    // PTZ云台控制 - 预置位
    // command: 0=删除, 1=调用, 2=设置
    // presetId: 预置位ID(1-255)
    bool PtzPreset(const std::string& channelId, int command, int presetId);

    // 获取媒体会话管理器
    MediaSessionManager* GetMediaSessionManager();

    // 获取所有活跃的媒体会话
    std::vector<std::string> GetActiveMediaSessions();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gb28181

#endif // GB28181_SIP_MANAGER_H
